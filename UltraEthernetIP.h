//
// UltraEthernetIP.h - Ultra Ethernet Network Layer
//

#ifndef __ULTRAETHERNET_IP_H
#define __ULTRAETHERNET_IP_H

#include <omnetpp.h>
#include <map>
#include <vector>
#include <string>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

enum TopoType {
    TOPO_DRAGONFLY,
    TOPO_LEAFSPINE,
    TOPO_MESH,
    TOPO_STAR,
    TOPO_UNKNOWN
};

struct TopoParams {
    // Dragonfly parameters
    int dragonflyHostsPerRouter = 8;
    int dragonflyRoutersPerGroup = 8;

    // Leaf-Spine parameters
    int leafSpineHostsPerLeaf = 32;
    int leafSpineNumSpines = 8;

    // Mesh parameters
    int meshHostsPerSwitch = 32;
    int meshDegree = 8;
    int meshDimensions = 2;

    // Star parameters (minimal)
    int starNumHosts = 4;
};

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

    // Topology detection and parameters
    TopoType detectedTopology;
    TopoParams topoParams;

    // Routing functions
    void initializeRoutingTable();
    void updateRoutingTable();
    bool routePacket(UETPacket *pkt);
    int getNetworkSize();
    TopoType detectTopology();
    void loadTopologyParameters();
    std::vector<int> calculateNextHops(int src, int dest);
    int mapInterfaceToGate(int logicalInterface);
    int calculateNextHopInterface(int src, int dest, int networkSize);
    
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