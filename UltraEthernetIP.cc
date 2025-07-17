//
// UltraEthernetIP.cc - Ultra Ethernet Network Layer Implementation
//

#include "UltraEthernetIP.h"

Define_Module(UltraEthernetIP);

UltraEthernetIP::UltraEthernetIP() {
    routingTimer = nullptr;
}

UltraEthernetIP::~UltraEthernetIP() {
    cancelAndDelete(routingTimer);
}

void UltraEthernetIP::initialize() {
    // Read configuration parameters
    routingLatency = par("routingLatency").doubleValue();
    loadBalancingEnabled = par("loadBalancingEnabled").boolValue();
    routingTableSize = par("routingTableSize").intValue();
    routingUpdateInterval = par("routingUpdateInterval").doubleValue();
    
    // Initialize statistics
    packetsForwarded = registerSignal("packetsForwarded");
    packetsDropped = registerSignal("packetsDropped");
    routingTableSizeSignal = registerSignal("routingTableSize");
    forwardingLatency = registerSignal("forwardingLatency");
    
    // Initialize routing table
    initializeRoutingTable();
    
    // Schedule routing updates
    routingTimer = new cMessage("routingTimer");
    scheduleAt(simTime() + routingUpdateInterval, routingTimer);
}

void UltraEthernetIP::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == routingTimer) {
            updateRoutingTable();
            scheduleAt(simTime() + routingUpdateInterval, routingTimer);
        }
    } else {
        UETPacket *pkt = check_and_cast<UETPacket*>(msg);
        
        if (msg->getArrivalGate()->isName("transportIn")) {
            // Packet from transport layer - route to link layer
            processFromTransport(pkt);
        } else if (msg->getArrivalGate()->isName("linkIn")) {
            // Packet from link layer - route to transport layer
            processFromLink(pkt);
        }
    }
}

void UltraEthernetIP::processFromTransport(UETPacket *pkt) {
    simtime_t processingStart = simTime();
    
    // Add IP header information
    pkt->setSrcAddr(getParentModule()->isVector() ? getParentModule()->getIndex() : 0);
    
    // Route the packet
    if (routePacket(pkt)) {
        // Apply artificial routing delay
        if (routingLatency > 0) {
            sendDelayed(pkt, routingLatency, "linkOut");
        } else {
            send(pkt, "linkOut");
        }
        
        emit(packetsForwarded, 1);
        emit(forwardingLatency, simTime() - processingStart);
    } else {
        // No route found, drop packet
        emit(packetsDropped, 1);
        delete pkt;
    }
}

void UltraEthernetIP::processFromLink(UETPacket *pkt) {
    simtime_t processingStart = simTime();
    
    // Check if packet is for this node
    if (pkt->getDestAddr() == (getParentModule()->isVector() ? getParentModule()->getIndex() : 0)) {
        // Deliver to transport layer
        send(pkt, "transportOut");
        emit(packetsForwarded, 1);
        emit(forwardingLatency, simTime() - processingStart);
    } else {
        // Forward packet (if this is a switch)
        if (routePacket(pkt)) {
            if (routingLatency > 0) {
                sendDelayed(pkt, routingLatency, "linkOut");
            } else {
                send(pkt, "linkOut");
            }
            emit(packetsForwarded, 1);
            emit(forwardingLatency, simTime() - processingStart);
        } else {
            emit(packetsDropped, 1);
            delete pkt;
        }
    }
}

bool UltraEthernetIP::routePacket(UETPacket *pkt) {
    int dest = pkt->getDestAddr();
    
    // Look up routing table
    auto it = routingTable.find(dest);
    if (it != routingTable.end()) {
        RoutingEntry &entry = it->second;
        
        // Apply load balancing if enabled
        if (loadBalancingEnabled && entry.nextHops.size() > 1) {
            // Select next hop based on flow hash
            int hopIndex = pkt->getFlowId() % entry.nextHops.size();
            pkt->setPathId(entry.nextHops[hopIndex]);
        } else if (!entry.nextHops.empty()) {
            pkt->setPathId(entry.nextHops[0]);
        }
        
        // Update routing statistics
        entry.packetsForwarded++;
        entry.lastUsed = simTime();
        
        return true;
    }
    
    return false;
}

void UltraEthernetIP::initializeRoutingTable() {
    // Initialize basic routing table
    // In a real implementation, this would be populated by a routing protocol
    
    int nodeIndex = getParentModule()->isVector() ? getParentModule()->getIndex() : 0;
    
    // Add entry for self
    RoutingEntry selfEntry;
    selfEntry.destAddr = nodeIndex;
    selfEntry.nextHops.push_back(0);  // Local delivery
    selfEntry.metric = 0;
    selfEntry.packetsForwarded = 0;
    selfEntry.lastUsed = simTime();
    
    routingTable[nodeIndex] = selfEntry;
    
    // Add default entries for demonstration
    // In practice, these would be learned via routing protocols
    for (int i = 0; i < 10; i++) {
        if (i != nodeIndex) {
            RoutingEntry entry;
            entry.destAddr = i;
            entry.nextHops.push_back(i % 4);  // Simple hash-based routing
            entry.metric = 1;
            entry.packetsForwarded = 0;
            entry.lastUsed = simTime();
            
            routingTable[i] = entry;
        }
    }
    
    emit(routingTableSizeSignal, (int)routingTable.size());
}

void UltraEthernetIP::updateRoutingTable() {
    // Periodic routing table updates
    // In a real implementation, this would handle:
    // - Link state updates
    // - Topology changes
    // - Metric updates
    
    // For now, just update statistics
    emit(routingTableSizeSignal, (int)routingTable.size());
    
    // Age out old entries
    auto it = routingTable.begin();
    while (it != routingTable.end()) {
        if (simTime() - it->second.lastUsed > 10.0) {  // 10 second timeout
            it = routingTable.erase(it);
        } else {
            ++it;
        }
    }
}

void UltraEthernetIP::addRoutingEntry(int destAddr, int nextHop, int metric) {
    RoutingEntry entry;
    entry.destAddr = destAddr;
    entry.nextHops.push_back(nextHop);
    entry.metric = metric;
    entry.packetsForwarded = 0;
    entry.lastUsed = simTime();
    
    routingTable[destAddr] = entry;
    emit(routingTableSizeSignal, (int)routingTable.size());
}

void UltraEthernetIP::removeRoutingEntry(int destAddr) {
    auto it = routingTable.find(destAddr);
    if (it != routingTable.end()) {
        routingTable.erase(it);
        emit(routingTableSizeSignal, (int)routingTable.size());
    }
}

void UltraEthernetIP::finish() {
    // Record final statistics
}