//
// UltraEthernetLink.cc - Ultra Ethernet Link Layer Implementation
//

#include "UltraEthernetLink.h"

Define_Module(UltraEthernetLink);

UltraEthernetLink::UltraEthernetLink() {
    llrTimer = nullptr;
    nextLlrSequence = 0;
    expectedLlrSequence = 0;
    globalSeqCounter = 0;
    linkCapacity = 0;
    totalBytesTransmitted = 0;
    utilizationWindowStart = 0;
}

UltraEthernetLink::~UltraEthernetLink() {
    cancelAndDelete(llrTimer);

    // Clean up legacy buffer safely
    for (auto& entry : llrRetransmissionBuffer) {
        if (entry.second.packet != nullptr) {
            // Check if packet is still owned by this component
            if (entry.second.packet->getOwner() == this) {
                delete entry.second.packet;
            } else {
                // Packet is owned by another component, just drop our reference
                entry.second.packet = nullptr;
            }
        }
    }

    // Clean up per-flow buffers safely
    for (auto& flowBuffer : llrRetransmissionBufferPerFlow) {
        for (auto& seqEntry : flowBuffer.second) {
            if (seqEntry.second.packet != nullptr) {
                // Check if packet is still owned by this component
                if (seqEntry.second.packet->getOwner() == this) {
                    delete seqEntry.second.packet;
                } else {
                    // Packet is owned by another component, just drop our reference
                    seqEntry.second.packet = nullptr;
                }
            }
        }
    }
}

void UltraEthernetLink::initialize() {
    // Read configuration parameters
    llrEnabled = par("llrEnabled").boolValue();
    llrTimeout = par("llrTimeout").doubleValue();
    maxRetransmissions = par("maxRetransmissions").intValue();
    priCompressionRatio = par("priCompressionRatio").doubleValue();
    linkLatency = par("linkLatency").doubleValue();

    // Read link capacity for utilization calculation
    linkCapacity = par("linkSpeed").doubleValue();  // in bits per second
    utilizationWindowStart = simTime();

    // Initialize statistics
    packetsTransmitted = registerSignal("packetsTransmitted");
    packetsReceived = registerSignal("packetsReceived");
    llrRetransmissions = registerSignal("llrRetransmissions");
    compressionRatio = registerSignal("compressionRatio");
    linkUtilization = registerSignal("linkUtilization");
    globalSeqNum = registerSignal("globalSeqNum");
    
    // Initialize timer
    llrTimer = new cMessage("llrTimer");
}

void UltraEthernetLink::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == llrTimer) {
            handleLlrTimeout();
        }
    } else {
        if (msg->getArrivalGate()->isName("networkIn")) {
            // Packet from network layer
            UETPacket *pkt = check_and_cast<UETPacket*>(msg);
            processFromNetwork(pkt);
        } else if (msg->getArrivalGate()->isName("phyIn")) {
            // Packet from physical layer
            cPacket *pkt = check_and_cast<cPacket*>(msg);
            processFromPhy(pkt);
        }
    }
}

void UltraEthernetLink::processFromNetwork(UETPacket *pkt) {
    // Apply PRI compression if enabled
    if (priCompressionRatio > 0) {
        applyPriCompression(pkt);
    }

    // Add LLR sequence number if enabled
    if (llrEnabled) {
        // Generate flow ID if not already set
        uint32_t flowId = pkt->getFlowId();
        if (flowId == 0) {
            flowId = generateFlowId(pkt->getSrcAddr(), pkt->getDestAddr());
            pkt->setFlowId(flowId);
        }

        // Get next sequence number for this flow
        uint64_t& nextSeq = nextLlrSeqPerFlow[flowId];
        pkt->setAckSequence(nextSeq++);

        // Set global sequence for debugging and emit signal
        pkt->setSequenceNum(globalSeqCounter);
        emit(globalSeqNum, globalSeqCounter);
        globalSeqCounter++;

        // Store for potential retransmission per flow
        LlrRetransmissionEntry entry;
        entry.packet = pkt->dup();  // dup() already creates a copy owned by this module
        entry.timestamp = simTime();
        entry.retransmissionCount = 0;

        llrRetransmissionBufferPerFlow[flowId][pkt->getAckSequence()] = entry;

        // Schedule timeout if not already scheduled
        if (!llrTimer->isScheduled()) {
            scheduleAt(simTime() + llrTimeout, llrTimer);
        }
    }

    // Send to physical layer
    if (linkLatency > 0) {
        sendDelayed(pkt, linkLatency, "phyOut");
    } else {
        send(pkt, "phyOut");
    }

    // Track bytes for utilization calculation
    totalBytesTransmitted += pkt->getByteLength();

    emit(packetsTransmitted, 1);
    updateLinkUtilization();
}

void UltraEthernetLink::processFromPhy(cPacket *pkt) {
    emit(packetsReceived, 1);

    // Check if this is an LLR acknowledgment
    LLRAck *llrAck = dynamic_cast<LLRAck*>(pkt);
    if (llrAck) {
        processLlrAck(llrAck);
        delete llrAck;
        return;
    }

    // Regular data packet
    UETPacket *uetPkt = check_and_cast<UETPacket*>(pkt);

    // Handle LLR if enabled
    if (llrEnabled) {
        uint32_t flowId = uetPkt->getFlowId();

        // Initialize expected sequence for this flow if not seen before
        if (expectedLlrSeqPerFlow.find(flowId) == expectedLlrSeqPerFlow.end()) {
            expectedLlrSeqPerFlow[flowId] = uetPkt->getAckSequence();
        }

        uint64_t expectedSeq = expectedLlrSeqPerFlow[flowId];

        if (uetPkt->getAckSequence() == expectedSeq) {
            // In-order packet for this flow
            expectedLlrSeqPerFlow[flowId]++;
            sendLlrAckForFlow(flowId, uetPkt->getAckSequence(), true, uetPkt->getSrcAddr());

            // Decompress if needed
            if (priCompressionRatio > 0) {
                applyPriDecompression(uetPkt);
            }

            send(uetPkt, "networkOut");

            // Check reorder buffer for next packets from this flow
            processReorderBufferForFlow(flowId);

        } else if (uetPkt->getAckSequence() > expectedSeq) {
            // Out-of-order packet for this flow - buffer it
            // For now, request retransmission (could be enhanced to buffer)
            sendLlrAckForFlow(flowId, expectedSeq, false, uetPkt->getSrcAddr());
            delete uetPkt;

        } else {
            // Duplicate packet for this flow - ack but drop
            sendLlrAckForFlow(flowId, uetPkt->getAckSequence(), true, uetPkt->getSrcAddr());
            delete uetPkt;
        }

    } else {
        // No LLR, forward directly
        if (priCompressionRatio > 0) {
            applyPriDecompression(uetPkt);
        }
        send(uetPkt, "networkOut");
    }
}

void UltraEthernetLink::processLlrAck(LLRAck *ack) {
    int seqNum = ack->getAcknowledgedSeq();
    uint32_t flowId = ack->getPathId();  // Flow ID is embedded in pathId

    if (ack->getAckType() == 0) {  // Positive ACK
        // Try to remove from per-flow buffer first
        bool foundInFlowBuffer = false;
        if (llrRetransmissionBufferPerFlow.find(flowId) != llrRetransmissionBufferPerFlow.end()) {
            auto& flowBuffer = llrRetransmissionBufferPerFlow[flowId];
            auto seqIt = flowBuffer.find((uint64_t)seqNum);
            if (seqIt != flowBuffer.end()) {
                delete seqIt->second.packet;
                flowBuffer.erase(seqIt);
                foundInFlowBuffer = true;
            }
        }

        // If not found in flow buffer, try legacy buffer for backward compatibility
        if (!foundInFlowBuffer) {
            auto it = llrRetransmissionBuffer.find(seqNum);
            if (it != llrRetransmissionBuffer.end()) {
                delete it->second.packet;
                llrRetransmissionBuffer.erase(it);
            }
        }
    } else {  // Negative ACK (NACK)
        // Try to retransmit from per-flow buffer first
        bool foundInFlowBuffer = false;
        if (llrRetransmissionBufferPerFlow.find(flowId) != llrRetransmissionBufferPerFlow.end()) {
            auto& flowBuffer = llrRetransmissionBufferPerFlow[flowId];
            auto seqIt = flowBuffer.find((uint64_t)seqNum);
            if (seqIt != flowBuffer.end()) {
                UETPacket *retransmit = seqIt->second.packet->dup();
                send(retransmit, "phyOut");

                seqIt->second.retransmissionCount++;
                seqIt->second.timestamp = simTime();

                emit(llrRetransmissions, 1);
                emit(packetsTransmitted, 1);
                foundInFlowBuffer = true;
            }
        }

        // If not found in flow buffer, try legacy buffer
        if (!foundInFlowBuffer) {
            auto it = llrRetransmissionBuffer.find(seqNum);
            if (it != llrRetransmissionBuffer.end()) {
                UETPacket *retransmit = it->second.packet->dup();
                send(retransmit, "phyOut");

                it->second.retransmissionCount++;
                it->second.timestamp = simTime();

                emit(llrRetransmissions, 1);
                emit(packetsTransmitted, 1);
            }
        }
    }
}

void UltraEthernetLink::sendLlrAck(int seqNum, bool positive) {
    LLRAck *ack = new LLRAck("LLRAck");
    ack->setAcknowledgedSeq(seqNum);
    ack->setAckType(positive ? 0 : 1);
    ack->setPathId(0);
    
    send(ack, "phyOut");
    emit(packetsTransmitted, 1);
}

void UltraEthernetLink::handleLlrTimeout() {
    bool hasOutstandingPackets = false;

    // Check for timed-out packets in per-flow buffers
    for (auto& flowBuffer : llrRetransmissionBufferPerFlow) {
        uint32_t flowId = flowBuffer.first;
        auto& seqMap = flowBuffer.second;

        auto seqIt = seqMap.begin();
        while (seqIt != seqMap.end()) {
            if (simTime() - seqIt->second.timestamp > llrTimeout) {
                if (seqIt->second.retransmissionCount < maxRetransmissions) {
                    // Retransmit packet
                    UETPacket *retransmit = seqIt->second.packet->dup();
                    send(retransmit, "phyOut");

                    seqIt->second.retransmissionCount++;
                    seqIt->second.timestamp = simTime();

                    emit(llrRetransmissions, 1);
                    emit(packetsTransmitted, 1);

                    hasOutstandingPackets = true;
                    ++seqIt;
                } else {
                    // Max retransmissions reached, drop packet
                    delete seqIt->second.packet;
                    seqIt = seqMap.erase(seqIt);
                }
            } else {
                hasOutstandingPackets = true;
                ++seqIt;
            }
        }
    }

    // Also check legacy buffer for backward compatibility
    auto it = llrRetransmissionBuffer.begin();
    while (it != llrRetransmissionBuffer.end()) {
        if (simTime() - it->second.timestamp > llrTimeout) {
            if (it->second.retransmissionCount < maxRetransmissions) {
                // Retransmit packet
                UETPacket *retransmit = it->second.packet->dup();
                send(retransmit, "phyOut");

                it->second.retransmissionCount++;
                it->second.timestamp = simTime();

                emit(llrRetransmissions, 1);
                emit(packetsTransmitted, 1);

                hasOutstandingPackets = true;
                ++it;
            } else {
                // Max retransmissions reached, drop packet
                delete it->second.packet;
                it = llrRetransmissionBuffer.erase(it);
            }
        } else {
            hasOutstandingPackets = true;
            ++it;
        }
    }

    // Schedule next timeout check if there are outstanding packets
    if (hasOutstandingPackets) {
        scheduleAt(simTime() + llrTimeout, llrTimer);
    }
}

void UltraEthernetLink::applyPriCompression(UETPacket *pkt) {
    // Simulate PRI compression
    int originalSize = pkt->getByteLength();
    int compressedSize = (int)(originalSize * (1.0 - priCompressionRatio));
    
    pkt->setByteLength(compressedSize);
    
    // Record compression statistics
    double actualRatio = (double)(originalSize - compressedSize) / originalSize;
    emit(compressionRatio, actualRatio);
}

void UltraEthernetLink::applyPriDecompression(UETPacket *pkt) {
    // Simulate PRI decompression
    int compressedSize = pkt->getByteLength();
    int originalSize = (int)(compressedSize / (1.0 - priCompressionRatio));
    
    pkt->setByteLength(originalSize);
}

void UltraEthernetLink::updateLinkUtilization() {
    // Calculate actual link utilization based on bandwidth usage
    simtime_t currentTime = simTime();
    simtime_t timeWindow = currentTime - utilizationWindowStart;

    if (timeWindow > 0 && linkCapacity > 0) {
        // Calculate actual data rate in bits per second
        double actualDataRate = (double)totalBytesTransmitted * 8.0 / SIMTIME_DBL(timeWindow);

        // Calculate utilization as a fraction of link capacity
        double utilization = actualDataRate / linkCapacity;
        utilization = std::min(1.0, std::max(0.0, utilization));  // Clamp to [0,1]

        emit(linkUtilization, utilization);
    }
}

uint32_t UltraEthernetLink::generateFlowId(uint32_t srcAddr, uint32_t destAddr, uint32_t sessionId) {
    // Generate flow ID using hash combination of addresses and session
    // This ensures different flows have unique IDs
    // Use module ID instead of getIndex() to avoid vector index issues
    int moduleId = getId();
    return (srcAddr << 16) ^ (destAddr << 8) ^ (sessionId & 0xFF) ^ (moduleId & 0xFF);
}

void UltraEthernetLink::sendLlrAckForFlow(uint32_t flowId, uint64_t seqNum, bool positive, uint32_t originalSrcAddr) {
    LLRAck *ack = new LLRAck("LLRAck");
    ack->setAcknowledgedSeq((int)seqNum);  // Convert to legacy format
    ack->setAckType(positive ? 0 : 1);
    ack->setPathId(flowId & 0xFFFF);  // Embed flow info in pathId

    // Set routing information: ACK goes back to original sender
    ack->setDestAddr(originalSrcAddr);  // Destination is original packet's source
    int myIndex = getParentModule()->isVector() ? getParentModule()->getIndex() : 0;
    ack->setSrcAddr(myIndex);  // Source is this host

    send(ack, "phyOut");
    emit(packetsTransmitted, 1);
}

void UltraEthernetLink::processReorderBufferForFlow(uint32_t flowId) {
    // Check if there are buffered packets for this flow that can now be delivered
    if (llrRetransmissionBufferPerFlow.find(flowId) == llrRetransmissionBufferPerFlow.end()) {
        return;  // No buffer for this flow
    }

    uint64_t expectedSeq = expectedLlrSeqPerFlow[flowId];
    auto& flowBuffer = llrRetransmissionBufferPerFlow[flowId];
    auto it = flowBuffer.find(expectedSeq);

    while (it != flowBuffer.end()) {
        UETPacket *bufferedPkt = it->second.packet;

        // Decompress if needed
        if (priCompressionRatio > 0) {
            applyPriDecompression(bufferedPkt);
        }

        // Send the original packet directly (no need for extra dup)
        send(bufferedPkt, "networkOut");

        // Remove from buffer (packet is now owned by the network)
        flowBuffer.erase(it);

        // Move to next expected sequence
        expectedLlrSeqPerFlow[flowId]++;
        expectedSeq = expectedLlrSeqPerFlow[flowId];
        it = flowBuffer.find(expectedSeq);
    }
}

void UltraEthernetLink::finish() {
    // Record final statistics
    EV_INFO << "Flow-based LLR statistics:" << endl;
    EV_INFO << "Active flows: " << nextLlrSeqPerFlow.size() << endl;

    for (auto& flow : nextLlrSeqPerFlow) {
        EV_INFO << "Flow " << flow.first << ": sent=" << flow.second
                << ", expected=" << expectedLlrSeqPerFlow[flow.first] << endl;
    }
}