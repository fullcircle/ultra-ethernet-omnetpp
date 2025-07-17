//
// UETTransport.h - Ultra Ethernet Transport Layer
//

#ifndef __UET_TRANSPORT_H
#define __UET_TRANSPORT_H

#include <omnetpp.h>
#include <map>
#include <queue>
#include <vector>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

class UETTransport : public cSimpleModule {
public:
    enum ProfileType {
        AI_BASE = 0,
        AI_FULL = 1,
        HPC = 2
    };
    
    enum OperationType {
        RDMA_SEND = 0,
        RDMA_WRITE = 1,
        RDMA_READ = 2,
        ATOMIC_FETCH_ADD = 3,
        ATOMIC_COMPARE_SWAP = 4,
        COLLECTIVE_ALLREDUCE = 5,
        COLLECTIVE_BROADCAST = 6
    };

private:
    // Configuration
    ProfileType profileType;
    bool packetSprayingEnabled;
    bool reorderingEnabled;
    int maxReorderBuffer;
    
    // Connection management (ephemeral connections)
    struct ConnectionState {
        uint32_t nextTxSeq;
        uint32_t nextRxSeq;
        std::map<uint32_t, UETPacket*> reorderBuffer;
        std::queue<UETPacket*> retransmitQueue;
        simtime_t lastActivity;
        bool isActive;
    };
    
    std::map<uint32_t, ConnectionState> connections;
    
    // Congestion control
    struct FlowState {
        double congestionWindow;
        double ssthresh;
        simtime_t rtt;
        std::vector<int> availablePaths;
        int currentPath;
        double sendingRate;
    };
    
    std::map<uint32_t, FlowState> flows;
    
    // Security contexts (job-based)
    struct SecurityContext {
        uint64_t jobId;
        std::vector<uint8_t> sharedKey;
        bool encryptionEnabled;
    };
    
    std::map<uint64_t, SecurityContext> securityContexts;
    
    // Performance metrics
    simsignal_t endToEndLatency;
    simsignal_t throughput;
    simsignal_t reorderingEvents;
    simsignal_t pathSwitchingEvents;
    
    // Timers
    cMessage *connectionCleanupTimer;
    cMessage *congestionControlTimer;
    
public:
    UETTransport();
    virtual ~UETTransport();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    
    // Transport operations
    void sendRDMAOperation(OperationType op, uint32_t destAddr, 
                          uint64_t localAddr, uint64_t remoteAddr, 
                          size_t length, uint32_t tag);
    
    void handleIncomingPacket(UETPacket *pkt);
    void processReorderBuffer(uint32_t flowId);
    
    // Packet spraying and multipath
    int selectPath(uint32_t flowId);
    void updatePathMetrics(uint32_t flowId, int pathId, simtime_t rtt);
    
    // Congestion control
    void updateCongestionWindow(uint32_t flowId, bool congestionDetected);
    double calculateSendingRate(uint32_t flowId);
    void handleCongestionFeedback(UETPacket *pkt);
    
    // Security
    UETPacket* encryptPacket(UETPacket *pkt, uint64_t jobId);
    UETPacket* decryptPacket(UETPacket *pkt, uint64_t jobId);
    
    // Connection management
    void createEphemeralConnection(uint32_t flowId);
    void cleanupInactiveConnections();
    
    // AI/HPC specific features
    void handleDeferrableSend(UETPacket *pkt);  // AI Full profile
    void processCollectiveOperation(UETPacket *pkt);
    void handleAtomicOperation(UETPacket *pkt);
};

#endif