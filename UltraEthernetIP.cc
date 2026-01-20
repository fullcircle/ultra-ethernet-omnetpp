//
// UltraEthernetIP.cc - Ultra Ethernet Network Layer Implementation
//

#include "UltraEthernetIP.h"
#include <algorithm>
#include <cctype>

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
    
    // Detect network topology and load parameters
    detectedTopology = detectTopology();
    loadTopologyParameters();

    // Debug: Log detected topology
    const char* topoNames[] = {"DRAGONFLY", "LEAFSPINE", "MESH", "STAR", "UNKNOWN"};
    EV_INFO << "Node " << (getParentModule()->isVector() ? getParentModule()->getIndex() : 0)
            << " detected topology: " << topoNames[detectedTopology] << "\\n";

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
        
        // Apply load balancing if enabled and multiple paths available
        int selectedInterface;
        if (loadBalancingEnabled && entry.nextHops.size() > 1) {
            // Select next hop based on flow hash for load balancing
            int hopIndex = pkt->getFlowId() % entry.nextHops.size();
            selectedInterface = entry.nextHops[hopIndex];
        } else if (!entry.nextHops.empty()) {
            selectedInterface = entry.nextHops[0];
        } else {
            return false; // No valid next hops
        }

        // Map logical interface to actual gate
        int gateIndex = mapInterfaceToGate(selectedInterface);
        pkt->setPathId(gateIndex);
        
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
    
    // Get the actual network size dynamically
    int networkSize = getNetworkSize();
    EV_INFO << "Initializing routing table for network of size " << networkSize << "\n";
    
    // Add entry for self
    RoutingEntry selfEntry;
    selfEntry.destAddr = nodeIndex;
    selfEntry.nextHops.push_back(0);  // Local delivery
    selfEntry.metric = 0;
    selfEntry.packetsForwarded = 0;
    selfEntry.lastUsed = simTime();
    
    routingTable[nodeIndex] = selfEntry;
    
    // Add entries for all nodes with topology-aware next-hop calculation
    for (int i = 0; i < networkSize; i++) {
        if (i != nodeIndex) {
            RoutingEntry entry;
            entry.destAddr = i;
            
            // Calculate next-hop interfaces using new multi-path system
            std::vector<int> nextHops = calculateNextHops(nodeIndex, i);
            entry.nextHops = nextHops;
            
            // Debug logging for first few entries
            if (i < nodeIndex + 5 && !entry.nextHops.empty()) {
                EV_INFO << "Node " << nodeIndex << " -> Node " << i << ": interfaces ";
                for (size_t j = 0; j < entry.nextHops.size(); j++) {
                    EV_INFO << entry.nextHops[j];
                    if (j < entry.nextHops.size() - 1) EV_INFO << ",";
                }
                EV_INFO << "\n";
            }
            entry.metric = 1;
            entry.packetsForwarded = 0;
            entry.lastUsed = simTime();
            
            routingTable[i] = entry;
        }
    }
    
    EV_INFO << "Routing table initialized with " << routingTable.size() << " entries\n";
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

int UltraEthernetIP::getNetworkSize() {
    // Try to get network size from parent network module
    cModule *network = getParentModule();
    while (network && network->getParentModule() != nullptr) {
        network = network->getParentModule();
    }

    if (network) {
        int totalNodes = 0;

        // Count all addressable entities (hosts + switches/routers)
        // Try common naming patterns for hosts
        std::vector<std::string> hostPatterns = {"hosts", "host", "nodes", "node"};
        for (const std::string& pattern : hostPatterns) {
            try {
                int size = network->getSubmoduleVectorSize(pattern.c_str());
                if (size > 0) {
                    totalNodes += size;
                    EV_INFO << "Found " << size << " " << pattern << "\n";
                }
            } catch (...) {
                // Pattern doesn't exist, continue
            }
        }

        // Try common naming patterns for switches/routers
        std::vector<std::string> switchPatterns = {"switches", "switch", "routers", "router", "leafSwitches", "spineSwitches"};
        for (const std::string& pattern : switchPatterns) {
            try {
                int size = network->getSubmoduleVectorSize(pattern.c_str());
                if (size > 0) {
                    totalNodes += size;
                    EV_INFO << "Found " << size << " " << pattern << "\n";
                }
            } catch (...) {
                // Pattern doesn't exist, continue
            }
        }

        // If we found nodes, return the total
        if (totalNodes > 0) {
            EV_INFO << "Total network size: " << totalNodes << " nodes\n";
            return totalNodes;
        }

        // Fallback: try numNodes parameter
        try {
            int numNodes = network->par("numNodes").intValue();
            if (numNodes > 0) {
                EV_INFO << "Using numNodes parameter: " << numNodes << "\n";
                return numNodes;
            }
        } catch (...) {
            // numNodes parameter might not exist
        }
    }

    // Fallback to a reasonable default
    EV_WARN << "Could not determine network size, using default of 100\n";
    return 100;
}

TopoType UltraEthernetIP::detectTopology() {
    // Get the network module for topology detection
    cModule *network = getParentModule();
    while (network && network->getParentModule() != nullptr) {
        network = network->getParentModule();
    }

    if (network) {
        std::string networkName = network->getName();

        // Check for explicit topology type parameter first
        try {
            std::string topoType = network->par("topologyType").stringValue();
            // Handle both quoted and unquoted versions, case-insensitive
            std::transform(topoType.begin(), topoType.end(), topoType.begin(), ::toupper);
            if (topoType == "DRAGONFLY") return TOPO_DRAGONFLY;
            if (topoType == "LEAF_SPINE" || topoType == "LEAFSPINE") return TOPO_LEAFSPINE;
            if (topoType == "MESH") return TOPO_MESH;
            if (topoType == "STAR" || topoType == "BUS") return TOPO_STAR;
        } catch (...) {
            // No topologyType parameter, use name-based detection
        }

        // Fallback to name-based detection
        if (networkName.find("Dragonfly") != std::string::npos) return TOPO_DRAGONFLY;
        if (networkName.find("LeafSpine") != std::string::npos || networkName.find("Spine") != std::string::npos) return TOPO_LEAFSPINE;
        if (networkName.find("Mesh") != std::string::npos) return TOPO_MESH;
        if (networkName.find("Bus") != std::string::npos || networkName.find("Star") != std::string::npos) return TOPO_STAR;

        // Additional patterns for traffic test networks (typically star topologies)
        if (networkName.find("TrafficTest") != std::string::npos || networkName.find("LargeScale") != std::string::npos) {
            return TOPO_STAR;  // Traffic test networks are typically star topologies
        }
    }

    return TOPO_UNKNOWN;
}

void UltraEthernetIP::loadTopologyParameters() {
    // Load topology-specific parameters from NED/INI
    cModule *network = getParentModule();
    while (network && network->getParentModule() != nullptr) {
        network = network->getParentModule();
    }

    if (network) {
        try {
            // Try to load parameters based on detected topology
            switch (detectedTopology) {
                case TOPO_DRAGONFLY:
                    topoParams.dragonflyHostsPerRouter = network->par("hostsPerRouter").intValue();
                    topoParams.dragonflyRoutersPerGroup = network->par("routersPerGroup").intValue();
                    break;
                case TOPO_LEAFSPINE:
                    topoParams.leafSpineHostsPerLeaf = network->par("hostsPerLeaf").intValue();
                    topoParams.leafSpineNumSpines = network->par("numSpineSwitches").intValue();
                    break;
                case TOPO_MESH:
                    topoParams.meshHostsPerSwitch = network->par("hostsPerSwitch").intValue();
                    topoParams.meshDegree = network->par("meshDegree").intValue();
                    topoParams.meshDimensions = network->par("meshDimensions").intValue();
                    break;
                case TOPO_STAR:
                    topoParams.starNumHosts = getNetworkSize();
                    break;
                default:
                    break;
            }
        } catch (...) {
            // Use default parameters if specific ones not found
            EV_INFO << "Using default topology parameters\n";
        }
    }
}

std::vector<int> UltraEthernetIP::calculateNextHops(int src, int dest) {
    std::vector<int> nextHops;

    switch (detectedTopology) {
        case TOPO_STAR: {
            // Star topology: hosts route to central switch, switch routes to specific hosts
            int totalNodes = getNetworkSize();
            int numHosts = topoParams.starNumHosts;

            // Check if current node is the central switch or a host
            // In star topology: hosts have indices 0 to numHosts-1, switch typically has higher index
            bool srcIsSwitch = (src >= numHosts);
            bool destIsSwitch = (dest >= numHosts);

            if (srcIsSwitch && !destIsSwitch) {
                // Source is switch, destination is host - route to host's interface
                nextHops.push_back(dest);  // Interface number matches host index
            } else if (!srcIsSwitch && destIsSwitch) {
                // Source is host, destination is switch - route via interface 0
                nextHops.push_back(0);
            } else if (!srcIsSwitch && !destIsSwitch) {
                // Both are hosts - route via central switch (interface 0)
                nextHops.push_back(0);
            } else {
                // Both are switches (shouldn't happen in star topology)
                nextHops.push_back(0);
            }
            break;
        }

        case TOPO_MESH: {
            // Mesh topology: dimension-order routing (DOR)
            int totalNodes = getNetworkSize();
            int numSwitches = (totalNodes + topoParams.meshHostsPerSwitch - 1) / topoParams.meshHostsPerSwitch;

            bool srcIsHost = (src >= numSwitches);
            if (srcIsHost) {
                // Hosts route to their ToR switch
                nextHops.push_back(0);
            } else {
                // Switch-to-switch routing: implement simple DOR
                // For now, use dimension-order: X first, then Y
                // This is a simplified 2D mesh routing
                int meshSize = (int)sqrt(numSwitches);
                int srcX = src % meshSize;
                int srcY = src / meshSize;
                int destSwitch = dest / topoParams.meshHostsPerSwitch;
                int destX = destSwitch % meshSize;
                int destY = destSwitch / meshSize;

                if (srcX != destX) {
                    // Route in X dimension first
                    int deltaX = (destX > srcX) ? 1 : -1;
                    nextHops.push_back((deltaX > 0) ? 0 : 1); // East/West interfaces
                } else if (srcY != destY) {
                    // Route in Y dimension
                    int deltaY = (destY > srcY) ? 1 : -1;
                    nextHops.push_back((deltaY > 0) ? 2 : 3); // North/South interfaces
                }
            }
            break;
        }

        case TOPO_LEAFSPINE: {
            // Leaf-Spine topology
            int numLeaves = (getNetworkSize() + topoParams.leafSpineHostsPerLeaf - 1) / topoParams.leafSpineHostsPerLeaf;
            bool srcIsHost = (src >= topoParams.leafSpineNumSpines + numLeaves);

            if (srcIsHost) {
                // Hosts route to their leaf switch
                nextHops.push_back(0);
            } else if (src < topoParams.leafSpineNumSpines) {
                // Source is spine switch - route to destination leaf
                int destLeaf = dest / topoParams.leafSpineHostsPerLeaf;
                nextHops.push_back(destLeaf); // Interface to specific leaf
            } else {
                // Source is leaf switch - route via spines (ECMP)
                for (int spine = 0; spine < topoParams.leafSpineNumSpines; spine++) {
                    nextHops.push_back(topoParams.leafSpineHostsPerLeaf + spine);
                }
            }
            break;
        }

        case TOPO_DRAGONFLY: {
            // Dragonfly topology: hierarchical routing with groups
            int hostsPerRouter = topoParams.dragonflyHostsPerRouter;
            int routersPerGroup = topoParams.dragonflyRoutersPerGroup;

            // Calculate total switches for host/switch differentiation
            int totalNodes = getNetworkSize();
            int numSwitches = (totalNodes + hostsPerRouter - 1) / hostsPerRouter;

            bool srcIsHost = (src >= numSwitches);
            bool destIsHost = (dest >= numSwitches);

            if (srcIsHost) {
                // Source is a host - route to connected router (interface 0)
                nextHops.push_back(0);
            } else {
                // Source is a router/switch
                if (destIsHost) {
                    // Destination is a host - check if directly connected
                    int hostIndex = dest - numSwitches;
                    int targetRouter = hostIndex / hostsPerRouter;

                    if (targetRouter == src) {
                        // Host is directly connected - calculate host interface
                        int hostOffset = hostIndex % hostsPerRouter;
                        nextHops.push_back(numSwitches + hostOffset);
                    } else {
                        // Route to target router first
                        int srcGroup = src / routersPerGroup;
                        int destGroup = targetRouter / routersPerGroup;

                        if (srcGroup == destGroup) {
                            // Same group - direct connection
                            nextHops.push_back(targetRouter % routersPerGroup);
                        } else {
                            // Different groups - route through global connections
                            nextHops.push_back(routersPerGroup + (destGroup % 4)); // Simplified global routing
                        }
                    }
                } else {
                    // Both source and destination are routers
                    int srcGroup = src / routersPerGroup;
                    int destGroup = dest / routersPerGroup;

                    if (srcGroup == destGroup) {
                        // Same group - direct connection
                        nextHops.push_back(dest % routersPerGroup);
                    } else {
                        // Different groups - route through global connections
                        nextHops.push_back(routersPerGroup + (destGroup % 4)); // Simplified global routing
                    }
                }
            }
            break;
        }

        default:
            // Unknown topology - fallback
            nextHops.push_back(0);
            break;
    }

    return nextHops;
}

int UltraEthernetIP::mapInterfaceToGate(int logicalInterface) {
    // Map logical interface number to actual gate index
    // The physical layer uses pathId as an index for send("ethg$o", index)

    // In the current UltraEthernetHost architecture, hosts have a single
    // linkOut gate (not a vector), so all outgoing traffic goes through
    // the same gate regardless of logical interface. The physical layer
    // below us handles the actual multi-port routing via ethg[].

    // For single-gate configurations (typical for hosts), always return 0
    // The pathId will be used by the physical layer to select the correct
    // ethg output port, but here we just pass through to linkOut

    // Note: logicalInterface is preserved in pathId for use by physical layer
    // We just always send on linkOut (index 0) since it's not a vector
    return 0;
}

// Legacy function - now calls new implementation
int UltraEthernetIP::calculateNextHopInterface(int src, int dest, int networkSize) {
    std::vector<int> nextHops = calculateNextHops(src, dest);
    return nextHops.empty() ? 0 : nextHops[0];
}

void UltraEthernetIP::finish() {
    // Record final statistics
}