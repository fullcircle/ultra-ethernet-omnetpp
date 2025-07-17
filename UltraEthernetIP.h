//
// UltraEthernetIP.h - Ultra Ethernet Network Layer
//

#ifndef __ULTRAETHERNET_IP_H
#define __ULTRAETHERNET_IP_H

#include <omnetpp.h>
#include <map>
#include <vector>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

struct RoutingEntry {
    int destAddr;
    std::vector<int> nextHops;
    int metric;
    int packetsForwarded;
    simtime_t lastUsed;
};

class UltraEthernetIP : public cSimpleModule {
private:
    // Configuration parameters
    simtime_t routingLatency;
    bool loadBalancingEnabled;
    int routingTableSize;
    simtime_t routingUpdateInterval;
    
    // Statistics
    simsignal_t packetsForwarded;
    simsignal_t packetsDropped;
    simsignal_t routingTableSizeSignal;
    simsignal_t forwardingLatency;
    
    // Internal state
    cMessage *routingTimer;
    std::map<int, RoutingEntry> routingTable;
    
    // Routing functions
    void initializeRoutingTable();
    void updateRoutingTable();
    bool routePacket(UETPacket *pkt);
    
    // Message processing
    void processFromTransport(UETPacket *pkt);
    void processFromLink(UETPacket *pkt);
    
public:
    UltraEthernetIP();
    virtual ~UltraEthernetIP();
    
    // Public interface for routing table management
    void addRoutingEntry(int destAddr, int nextHop, int metric);
    void removeRoutingEntry(int destAddr);
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif