//
// UltraEthernetLink.h - Ultra Ethernet Link Layer
//

#ifndef __ULTRAETHERNET_LINK_H
#define __ULTRAETHERNET_LINK_H

#include <omnetpp.h>
#include <map>
#include <unordered_map>
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
    simsignal_t globalSeqNum;
    
    // Internal state
    cMessage *llrTimer;

    // Legacy global counters (kept for backward compatibility and debugging)
    int nextLlrSequence;
    int expectedLlrSequence;
    uint64_t globalSeqCounter;  // Global sequence for debugging

    // Flow-based state maps
    std::unordered_map<uint32_t, uint64_t> nextLlrSeqPerFlow;
    std::unordered_map<uint32_t, uint64_t> expectedLlrSeqPerFlow;
    std::unordered_map<uint32_t, std::map<uint64_t, LlrRetransmissionEntry>> llrRetransmissionBufferPerFlow;

    // Legacy buffer (kept for backward compatibility)
    std::map<int, LlrRetransmissionEntry> llrRetransmissionBuffer;

    // Bandwidth tracking for proper utilization calculation
    double linkCapacity;  // Link capacity in bits per second
    long totalBytesTransmitted;
    simtime_t utilizationWindowStart;
    
    // Message processing
    void processFromNetwork(UETPacket *pkt);
    void processFromPhy(cPacket *pkt);
    void processLlrAck(LLRAck *ack);
    
    // LLR operations
    void sendLlrAck(int seqNum, bool positive);
    void sendLlrAckForFlow(uint32_t flowId, uint64_t seqNum, bool positive, uint32_t originalSrcAddr);
    void handleLlrTimeout();
    void processReorderBufferForFlow(uint32_t flowId);
    uint32_t generateFlowId(uint32_t srcAddr, uint32_t destAddr, uint32_t sessionId = 0);
    
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