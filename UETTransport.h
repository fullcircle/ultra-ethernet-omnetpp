//
// UETTransport.h - Ultra Ethernet Transport Layer
//

#ifndef __UET_TRANSPORT_H
#define __UET_TRANSPORT_H

#include <omnetpp.h>
#include <map>
#include <queue>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

enum TransportProfileType {
    AI_BASE,
    AI_FULL,
    HPC
};

enum TransportType {
    DATA = 0,
    ACK = 1,
    NACK = 2
};

struct RetransmissionEntry {
    UETPacket* packet;
    simtime_t timestamp;
    int retransmissionCount;
};

class UETTransport : public cSimpleModule {
private:
    // Configuration parameters
    TransportProfileType profileType;
    bool packetSprayingEnabled;
    bool reorderingEnabled;
    int maxReorderBuffer;
    int congestionWindow;
    simtime_t rdmaTimeout;
    int maxRetransmissions;
    
    // Statistics
    simsignal_t packetsTransmitted;
    simsignal_t packetsReceived;
    simsignal_t retransmissions;
    simsignal_t congestionWindowSignal;
    simsignal_t roundTripTime;
    
    // Internal state
    cMessage *rdmaTimer;
    int nextSequenceNum;
    int expectedSequenceNum;
    
    // Buffers
    std::map<int, UETPacket*> reorderBuffer;
    std::map<int, RetransmissionEntry> retransmissionBuffer;
    
    // Message processing
    void processFromApplication(UETPacket *pkt);
    void processFromNetwork(UETPacket *pkt);
    void processInOrderPacket(UETPacket *pkt);
    void processReorderBuffer();
    void processAcknowledgment(UETPacket *ack);
    
    // RDMA operations
    void handleRdmaTimeout();
    void sendAcknowledgment(int seqNum);
    
    // Advanced features
    void applyPacketSpraying(UETPacket *pkt);
    void updateCongestionWindow(simtime_t rtt);
    int generateFlowId();
    
public:
    UETTransport();
    virtual ~UETTransport();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif