//
// UltraEthernetLink.cc - Ultra Ethernet Link Layer Implementation
//

#include "UltraEthernetLink.h"

Define_Module(UltraEthernetLink);

UltraEthernetLink::UltraEthernetLink() {
    llrTimer = nullptr;
    nextLlrSequence = 0;
    expectedLlrSequence = 0;
}

UltraEthernetLink::~UltraEthernetLink() {
    cancelAndDelete(llrTimer);
    for (auto& entry : llrRetransmissionBuffer) {
        delete entry.second.packet;
    }
}

void UltraEthernetLink::initialize() {
    // Read configuration parameters
    llrEnabled = par("llrEnabled").boolValue();
    llrTimeout = par("llrTimeout").doubleValue();
    maxRetransmissions = par("maxRetransmissions").intValue();
    priCompressionRatio = par("priCompressionRatio").doubleValue();
    linkLatency = par("linkLatency").doubleValue();
    
    // Initialize statistics
    packetsTransmitted = registerSignal("packetsTransmitted");
    packetsReceived = registerSignal("packetsReceived");
    llrRetransmissions = registerSignal("llrRetransmissions");
    compressionRatio = registerSignal("compressionRatio");
    linkUtilization = registerSignal("linkUtilization");
    
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
        pkt->setAckSequence(nextLlrSequence++);
        
        // Store for potential retransmission
        LlrRetransmissionEntry entry;
        entry.packet = pkt->dup();
        entry.timestamp = simTime();
        entry.retransmissionCount = 0;
        llrRetransmissionBuffer[pkt->getAckSequence()] = entry;
        
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
        if (uetPkt->getAckSequence() == expectedLlrSequence) {
            // In-order packet
            expectedLlrSequence++;
            sendLlrAck(uetPkt->getAckSequence(), true);
            
            // Decompress if needed
            if (priCompressionRatio > 0) {
                applyPriDecompression(uetPkt);
            }
            
            send(uetPkt, "networkOut");
        } else if (uetPkt->getAckSequence() > expectedLlrSequence) {
            // Out-of-order packet - request retransmission
            sendLlrAck(expectedLlrSequence, false);
            delete uetPkt;
        } else {
            // Duplicate packet - ack but drop
            sendLlrAck(uetPkt->getAckSequence(), true);
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
    
    if (ack->getAckType() == 0) {  // Positive ACK
        // Remove from retransmission buffer
        auto it = llrRetransmissionBuffer.find(seqNum);
        if (it != llrRetransmissionBuffer.end()) {
            delete it->second.packet;
            llrRetransmissionBuffer.erase(it);
        }
    } else {  // Negative ACK (NACK)
        // Retransmit immediately
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

void UltraEthernetLink::sendLlrAck(int seqNum, bool positive) {
    LLRAck *ack = new LLRAck("LLRAck");
    ack->setAcknowledgedSeq(seqNum);
    ack->setAckType(positive ? 0 : 1);
    ack->setPathId(0);
    
    send(ack, "phyOut");
    emit(packetsTransmitted, 1);
}

void UltraEthernetLink::handleLlrTimeout() {
    // Check for timed-out packets
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
                
                ++it;
            } else {
                // Max retransmissions reached, drop packet
                delete it->second.packet;
                it = llrRetransmissionBuffer.erase(it);
            }
        } else {
            ++it;
        }
    }
    
    // Schedule next timeout check
    if (!llrRetransmissionBuffer.empty()) {
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
    // Calculate link utilization based on transmission queue size
    double utilization = (double)llrRetransmissionBuffer.size() / 100.0;  // Normalized
    emit(linkUtilization, utilization);
}

void UltraEthernetLink::finish() {
    // Record final statistics
}