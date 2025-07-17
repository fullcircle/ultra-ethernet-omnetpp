//
// UltraEthernetPhy.cc - Physical Layer Implementation
//

#include "UltraEthernetPhy.h"

Define_Module(UltraEthernetPhy);

UltraEthernetPhy::UltraEthernetPhy() {
    transmissionTimer = nullptr;
}

UltraEthernetPhy::~UltraEthernetPhy() {
    cancelAndDelete(transmissionTimer);
}

void UltraEthernetPhy::initialize() {
    // Read configuration parameters
    linkSpeed = par("linkSpeed").doubleValue();
    fecOverhead = par("fecOverhead").doubleValue();
    errorRate = par("errorRate").doubleValue();
    fecCorrectionBits = par("fecCorrectionBits").intValue();
    fecEnabled = par("fecEnabled").boolValue();
    
    // Initialize statistics
    fecCorrections = registerSignal("fecCorrections");
    uncorrectableErrors = registerSignal("uncorrectableErrors");
    linkUtilization = registerSignal("linkUtilization");
    
    // Initialize transmission timer
    transmissionTimer = new cMessage("transmissionTimer");
}

void UltraEthernetPhy::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == transmissionTimer) {
            scheduleNextTransmission();
        }
    } else {
        cPacket *pkt = check_and_cast<cPacket*>(msg);
        
        if (msg->getArrivalGate()->isName("linkIn")) {
            // Packet from link layer - transmit
            processTransmission(pkt);
        } else if (msg->getArrivalGate()->isName("ethIn")) {
            // Packet from network - receive
            if (simulateChannelErrors(pkt)) {
                send(pkt, "linkOut");
            } else {
                // Packet dropped due to uncorrectable errors
                emit(uncorrectableErrors, 1);
                delete pkt;
            }
        } else {
            // Unknown gate
            delete pkt;
        }
    }
}

void UltraEthernetPhy::processTransmission(cPacket *pkt) {
    // Apply FEC encoding if enabled
    if (fecEnabled) {
        applyFEC(pkt);
    }
    
    // Calculate transmission delay
    double bits = pkt->getBitLength() * (1.0 + fecOverhead);
    simtime_t txDelay = bits / linkSpeed;
    
    // Schedule transmission
    transmissionQueue.push(pkt);
    if (!transmissionTimer->isScheduled()) {
        scheduleAt(simTime() + txDelay, transmissionTimer);
    }
    
    // Update utilization statistics
    updateLinkUtilization();
}

bool UltraEthernetPhy::simulateChannelErrors(cPacket *pkt) {
    if (!fecEnabled) return true;
    
    // Simulate bit errors based on packet size and error rate
    double packetErrorProb = 1.0 - pow(1.0 - errorRate, pkt->getBitLength());
    
    if (uniform(0, 1) < packetErrorProb) {
        // Simulate FEC correction
        int errorBits = geometric(errorRate);
        if (errorBits <= fecCorrectionBits) {
            emit(fecCorrections, errorBits);
            return true;  // Correctable
        } else {
            return false; // Uncorrectable
        }
    }
    
    return true;  // No errors
}

void UltraEthernetPhy::applyFEC(cPacket *pkt) {
    // Add FEC overhead to packet
    int originalBits = pkt->getBitLength();
    int fecBits = (int)(originalBits * fecOverhead);
    pkt->setBitLength(originalBits + fecBits);
}

void UltraEthernetPhy::scheduleNextTransmission() {
    if (!transmissionQueue.empty()) {
        cPacket *pkt = transmissionQueue.front();
        transmissionQueue.pop();
        
        // Send on first ethernet port if available, otherwise drop
        if (gateSize("ethOut") > 0) {
            send(pkt, "ethOut", 0);
        } else {
            // No external connections, drop packet
            delete pkt;
        }
        
        if (!transmissionQueue.empty()) {
            cPacket *nextPkt = transmissionQueue.front();
            double bits = nextPkt->getBitLength();
            simtime_t txDelay = bits / linkSpeed;
            scheduleAt(simTime() + txDelay, transmissionTimer);
        }
    }
}

void UltraEthernetPhy::updateLinkUtilization() {
    // Calculate and emit link utilization
    double utilization = (double)transmissionQueue.size() / 100.0;  // Normalized
    emit(linkUtilization, utilization);
}

void UltraEthernetPhy::finish() {
    // Record final statistics
}