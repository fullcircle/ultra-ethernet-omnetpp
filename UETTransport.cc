//
// UETTransport.cc - Ultra Ethernet Transport Implementation
//

#include "UETTransport.h"

Define_Module(UETTransport);

UETTransport::UETTransport() {
    rdmaTimer = nullptr;
    congestionWindow = 10;
    nextSequenceNum = 0;
    expectedSequenceNum = 0;
}

UETTransport::~UETTransport() {
    cancelAndDelete(rdmaTimer);
    for (auto& pkt : reorderBuffer) {
        delete pkt.second;
    }
    for (auto& entry : retransmissionBuffer) {
        delete entry.second.packet;
    }
}

void UETTransport::initialize() {
    // Read configuration parameters
    std::string profileStr = par("profileType").stringValue();
    if (profileStr == "AI_BASE") profileType = AI_BASE;
    else if (profileStr == "AI_FULL") profileType = AI_FULL;
    else if (profileStr == "HPC") profileType = HPC;
    else profileType = AI_FULL;
    
    packetSprayingEnabled = par("packetSprayingEnabled").boolValue();
    reorderingEnabled = par("reorderingEnabled").boolValue();
    maxReorderBuffer = par("maxReorderBuffer").intValue();
    congestionWindow = par("initialCongestionWindow").intValue();
    rdmaTimeout = par("rdmaTimeout").doubleValue();
    maxRetransmissions = par("maxRetransmissions").intValue();
    
    // Initialize statistics
    packetsTransmitted = registerSignal("packetsTransmitted");
    packetsReceived = registerSignal("packetsReceived");
    retransmissions = registerSignal("retransmissions");
    congestionWindowSignal = registerSignal("congestionWindow");
    roundTripTime = registerSignal("roundTripTime");
    
    // Initialize timer
    rdmaTimer = new cMessage("rdmaTimer");
}

void UETTransport::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == rdmaTimer) {
            handleRdmaTimeout();
        }
    } else {
        if (msg->getArrivalGate()->isName("appIn")) {
            // Message from application
            UETPacket *pkt = check_and_cast<UETPacket*>(msg);
            processFromApplication(pkt);
        } else if (msg->getArrivalGate()->isName("networkIn")) {
            // Message from network
            UETPacket *pkt = check_and_cast<UETPacket*>(msg);
            processFromNetwork(pkt);
        }
    }
}

void UETTransport::processFromApplication(UETPacket *pkt) {
    // Add transport header
    pkt->setSequenceNum(nextSequenceNum++);
    pkt->setTransportType(DATA);
    pkt->setFlowId(generateFlowId());
    pkt->setTimestamp(simTime().raw());
    
    // Apply packet spraying if enabled
    if (packetSprayingEnabled && profileType == AI_FULL) {
        applyPacketSpraying(pkt);
    }
    
    // Store for potential retransmission
    if (profileType != AI_BASE) {
        RetransmissionEntry entry;
        entry.packet = pkt->dup();
        entry.timestamp = simTime();
        entry.retransmissionCount = 0;
        retransmissionBuffer[pkt->getSequenceNum()] = entry;
        
        // Schedule timeout
        if (!rdmaTimer->isScheduled()) {
            scheduleAt(simTime() + rdmaTimeout, rdmaTimer);
        }
    }
    
    send(pkt, "networkOut");
    emit(packetsTransmitted, 1);
    emit(congestionWindowSignal, congestionWindow);
}

void UETTransport::processFromNetwork(UETPacket *pkt) {
    emit(packetsReceived, 1);
    
    if (pkt->getTransportType() == ACK) {
        processAcknowledgment(pkt);
        delete pkt;
        return;
    }
    
    // Handle reordering if enabled
    if (reorderingEnabled && profileType == AI_FULL) {
        if (pkt->getSequenceNum() == expectedSequenceNum) {
            // In-order packet
            processInOrderPacket(pkt);
            expectedSequenceNum++;
            
            // Check reorder buffer for next packets
            processReorderBuffer();
        } else if (pkt->getSequenceNum() > expectedSequenceNum) {
            // Out-of-order packet - buffer it
            if (reorderBuffer.size() < maxReorderBuffer) {
                reorderBuffer[pkt->getSequenceNum()] = pkt;
            } else {
                // Buffer full, drop packet
                delete pkt;
            }
        } else {
            // Duplicate packet, drop it
            delete pkt;
        }
    } else {
        // No reordering, process directly
        processInOrderPacket(pkt);
    }
    
    // Send acknowledgment
    sendAcknowledgment(pkt->getSequenceNum());
}

void UETTransport::processInOrderPacket(UETPacket *pkt) {
    // Calculate RTT if this is a response to our packet
    auto it = retransmissionBuffer.find(pkt->getSequenceNum());
    if (it != retransmissionBuffer.end()) {
        simtime_t rtt = simTime() - it->second.timestamp;
        emit(roundTripTime, rtt);
        
        // Update congestion window based on RTT
        updateCongestionWindow(rtt);
        
        // Remove from retransmission buffer
        delete it->second.packet;
        retransmissionBuffer.erase(it);
    }
    
    send(pkt, "appOut");
}

void UETTransport::processReorderBuffer() {
    auto it = reorderBuffer.find(expectedSequenceNum);
    while (it != reorderBuffer.end()) {
        processInOrderPacket(it->second);
        reorderBuffer.erase(it);
        expectedSequenceNum++;
        it = reorderBuffer.find(expectedSequenceNum);
    }
}

void UETTransport::processAcknowledgment(UETPacket *ack) {
    int seqNum = ack->getSequenceNum();
    auto it = retransmissionBuffer.find(seqNum);
    if (it != retransmissionBuffer.end()) {
        // Calculate RTT
        simtime_t rtt = simTime() - it->second.timestamp;
        emit(roundTripTime, rtt);
        
        // Update congestion window
        updateCongestionWindow(rtt);
        
        // Remove from retransmission buffer
        delete it->second.packet;
        retransmissionBuffer.erase(it);
    }
}

void UETTransport::sendAcknowledgment(int seqNum) {
    UETPacket *ack = new UETPacket("ACK");
    ack->setTransportType(ACK);
    ack->setSequenceNum(seqNum);
    ack->setTimestamp(simTime().raw());
    
    send(ack, "networkOut");
    emit(packetsTransmitted, 1);
}

void UETTransport::handleRdmaTimeout() {
    // Handle retransmissions
    auto it = retransmissionBuffer.begin();
    while (it != retransmissionBuffer.end()) {
        if (simTime() - it->second.timestamp > rdmaTimeout) {
            if (it->second.retransmissionCount < maxRetransmissions) {
                // Retransmit packet
                UETPacket *retransmit = it->second.packet->dup();
                send(retransmit, "networkOut");
                
                it->second.retransmissionCount++;
                it->second.timestamp = simTime();
                
                emit(retransmissions, 1);
                emit(packetsTransmitted, 1);
                
                // Reduce congestion window on timeout
                congestionWindow = std::max(1, congestionWindow / 2);
                emit(congestionWindowSignal, congestionWindow);
                
                ++it;
            } else {
                // Max retransmissions reached, drop packet
                delete it->second.packet;
                it = retransmissionBuffer.erase(it);
            }
        } else {
            ++it;
        }
    }
    
    // Schedule next timeout check
    if (!retransmissionBuffer.empty()) {
        scheduleAt(simTime() + rdmaTimeout, rdmaTimer);
    }
}

void UETTransport::applyPacketSpraying(UETPacket *pkt) {
    // Simple packet spraying: set spray path hint
    pkt->setSprayPath(intuniform(0, 3));  // 4 possible paths
}

void UETTransport::updateCongestionWindow(simtime_t rtt) {
    // Simple congestion control based on RTT
    static simtime_t baseRtt = 0.001;  // 1ms base RTT
    
    if (rtt < baseRtt * 1.5) {
        // Low RTT, increase window
        congestionWindow = std::min(congestionWindow + 1, 64);
    } else if (rtt > baseRtt * 2.0) {
        // High RTT, decrease window
        congestionWindow = std::max(congestionWindow - 1, 1);
    }
}

int UETTransport::generateFlowId() {
    // Simple flow ID generation
    return (getParentModule()->isVector() ? getParentModule()->getIndex() : 0) * 10000 + intuniform(0, 9999);
}

void UETTransport::finish() {
    // Record final statistics
}