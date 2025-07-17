//
// UltraEthernetLink.h - Ultra Ethernet Link Layer
//

#ifndef __ULTRAETHERNET_LINK_H
#define __ULTRAETHERNET_LINK_H

#include <omnetpp.h>
#include <map>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

struct LlrRetransmissionEntry {
    UETPacket* packet;
    simtime_t timestamp;
    int retransmissionCount;
};

class UltraEthernetLink : public cSimpleModule {
private:
    // Configuration parameters
    bool llrEnabled;
    simtime_t llrTimeout;
    int maxRetransmissions;
    double priCompressionRatio;
    simtime_t linkLatency;
    
    // Statistics
    simsignal_t packetsTransmitted;
    simsignal_t packetsReceived;
    simsignal_t llrRetransmissions;
    simsignal_t compressionRatio;
    simsignal_t linkUtilization;
    
    // Internal state
    cMessage *llrTimer;
    int nextLlrSequence;
    int expectedLlrSequence;
    std::map<int, LlrRetransmissionEntry> llrRetransmissionBuffer;
    
    // Message processing
    void processFromNetwork(UETPacket *pkt);
    void processFromPhy(cPacket *pkt);
    void processLlrAck(LLRAck *ack);
    
    // LLR operations
    void sendLlrAck(int seqNum, bool positive);
    void handleLlrTimeout();
    
    // PRI compression
    void applyPriCompression(UETPacket *pkt);
    void applyPriDecompression(UETPacket *pkt);
    
    // Statistics
    void updateLinkUtilization();
    
public:
    UltraEthernetLink();
    virtual ~UltraEthernetLink();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif