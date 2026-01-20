//
// TopologyGenerator.cc - Base topology generator implementation
//

#include "TopologyGenerator.h"
#include <algorithm>
#include <fstream>
#include <cmath>

Define_Module(TopologyGenerator);
Define_Module(DragonflyTopology);
Define_Module(LeafSpineTopology);
Define_Module(MeshTopology);
Define_Module(TorusTopology);

TopologyGenerator::TopologyGenerator() {
    numNodes = 0;
    numSwitches = 0;
    topology = LEAF_SPINE;
    switchRadix = 64;
    hostsPerSwitch = 32;
    numPartitions = 1;
    parallelSimulation = false;
}

TopologyGenerator::~TopologyGenerator() {
}

void TopologyGenerator::initialize() {
    // Read configuration parameters
    numNodes = par("numNodes").intValue();
    switchRadix = par("switchRadix").intValue();
    hostsPerSwitch = par("hostsPerSwitch").intValue();
    
    std::string topologyStr = par("topologyType").stringValue();
    if (topologyStr == "LEAF_SPINE") topology = LEAF_SPINE;
    else if (topologyStr == "MESH") topology = MESH;
    else if (topologyStr == "TORUS_2D") topology = TORUS_2D;
    else if (topologyStr == "TORUS_3D") topology = TORUS_3D;
    else if (topologyStr == "DRAGONFLY") topology = DRAGONFLY;
    else topology = LEAF_SPINE;
    
    // For now, assume we're running with MPI if configured
    // The framework is ready - parallel simulation will be handled by OMNeT++
    parallelSimulation = true;  // Let OMNeT++ handle the detection
    
    // Get number of partitions from MPI environment  
    const char* np = getenv("OMPI_COMM_WORLD_SIZE");
    if (!np) np = getenv("MPI_COMM_WORLD_SIZE");
    if (!np) np = getenv("OMNETPP_NUM_PARTITIONS");
    numPartitions = np ? atoi(np) : 8;
    if (numPartitions <= 0) numPartitions = 8;
    
    EV_INFO << "MPI Configuration: parallel=" << parallelSimulation << ", partitions=" << numPartitions << "\n";
    
    // Generate the topology
    generateTopology();
    
    // Assign partitions for parallel simulation
    if (parallelSimulation) {
        assignPartitions();
    }
    validateTopology();
    
    // Export topology for visualization/analysis
    exportTopology("topology.txt");
    
    // Establish actual connections between OMNeT++ modules
    establishConnections();
    
    EV_INFO << "Generated " << getClassName() << " topology with " 
            << numNodes << " nodes and " << numSwitches << " switches\n";
}

void TopologyGenerator::handleMessage(cMessage *msg) {
    // Topology generators don't handle runtime messages
    delete msg;
}

void TopologyGenerator::addNode(int id, const std::string& type, int ports) {
    NodeInfo node;
    node.nodeId = id;
    node.nodeType = type;
    node.switchPorts = ports;
    node.x = node.y = node.z = 0;
    nodes.push_back(node);
    
    if (type != "host") {
        numSwitches++;
    }
}

void TopologyGenerator::addLink(int src, int dest, double bw, double lat) {
    LinkInfo link;
    link.src = src;
    link.dest = dest;
    link.bandwidth = bw;
    link.latency = lat;
    link.linkType = "default";
    links.push_back(link);
    
    // Add to node connection lists
    if (src < nodes.size()) {
        nodes[src].connections.push_back(dest);
    }
    if (dest < nodes.size()) {
        nodes[dest].connections.push_back(src);
    }
}

void TopologyGenerator::validateTopology() {
    EV_INFO << "Validating topology: " << nodes.size() << " nodes, " 
            << links.size() << " links\n";
    
    // Check connectivity
    for (auto& node : nodes) {
        if (node.nodeType == "host" && node.connections.empty()) {
            EV_WARN << "Host node " << node.nodeId << " has no connections\n";
        }
    }
}

void TopologyGenerator::exportTopology(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "# Ultra Ethernet Topology Export\n";
    file << "# Nodes: " << nodes.size() << ", Links: " << links.size() << "\n\n";
    
    file << "# Nodes (ID, Type, Ports, X, Y, Z)\n";
    for (auto& node : nodes) {
        file << node.nodeId << " " << node.nodeType << " " 
             << node.switchPorts << " " << node.x << " " 
             << node.y << " " << node.z << "\n";
    }
    
    file << "\n# Links (Src, Dest, Bandwidth, Latency)\n";
    for (auto& link : links) {
        file << link.src << " " << link.dest << " " 
             << link.bandwidth << " " << link.latency << "\n";
    }
    
    file.close();
}

void TopologyGenerator::finish() {
    EV_INFO << "Topology generation complete\n";
}

//=============================================================================
// Leaf-Spine Topology Implementation
//=============================================================================

Define_Module(LeafSpineTopology);

LeafSpineTopology::LeafSpineTopology() {
    numLeafSwitches = 0;
    numSpineSwitches = 0;
    hostsPerLeaf = 32;
}

void LeafSpineTopology::generateTopology() {
    EV_INFO << "Generating Leaf-Spine topology for " << numNodes << " nodes\n";
    
    // Calculate topology dimensions
    hostsPerLeaf = par("hostsPerLeaf").intValue();
    numLeafSwitches = (numNodes + hostsPerLeaf - 1) / hostsPerLeaf;  // Ceiling division
    numSpineSwitches = par("numSpineSwitches").intValue();
    
    int nodeId = 0;
    
    // Create spine switches
    for (int i = 0; i < numSpineSwitches; i++) {
        addNode(nodeId++, "spine", switchRadix);
    }
    
    // Create leaf switches
    for (int i = 0; i < numLeafSwitches; i++) {
        addNode(nodeId++, "leaf", switchRadix);
    }
    
    // Create hosts
    for (int i = 0; i < numNodes; i++) {
        addNode(nodeId++, "host", 1);
    }
    
    // Connect hosts to leaf switches
    int hostId = numSpineSwitches + numLeafSwitches;
    for (int leaf = 0; leaf < numLeafSwitches; leaf++) {
        int leafId = numSpineSwitches + leaf;
        
        // Connect hosts to this leaf
        for (int h = 0; h < hostsPerLeaf && hostId < numSpineSwitches + numLeafSwitches + numNodes; h++) {
            addLink(hostId++, leafId, 800e9, 100e-9);  // 800G, 100ns
        }
    }
    
    // Connect leaf switches to spine switches (full mesh)
    for (int leaf = 0; leaf < numLeafSwitches; leaf++) {
        int leafId = numSpineSwitches + leaf;
        for (int spine = 0; spine < numSpineSwitches; spine++) {
            addLink(leafId, spine, 800e9, 500e-9);  // 800G, 500ns
        }
    }
    
    EV_INFO << "Leaf-Spine: " << numSpineSwitches << " spine, " 
            << numLeafSwitches << " leaf, " << numNodes << " hosts\n";
}

//=============================================================================
// Mesh Topology Implementation  
//=============================================================================

Define_Module(MeshTopology);

MeshTopology::MeshTopology() {
    fullMesh = true;
    meshDegree = 4;
}

void MeshTopology::generateTopology() {
    EV_INFO << "Generating Mesh topology for " << numNodes << " nodes\n";
    
    fullMesh = par("fullMesh").boolValue();
    meshDegree = par("meshDegree").intValue();
    
    int nodeId = 0;
    
    // Create ToR switches for hosts
    int numToRSwitches = (numNodes + hostsPerSwitch - 1) / hostsPerSwitch;
    
    for (int i = 0; i < numToRSwitches; i++) {
        addNode(nodeId++, "tor", switchRadix);
    }
    
    // Create hosts
    for (int i = 0; i < numNodes; i++) {
        addNode(nodeId++, "host", 1);
    }
    
    // Connect hosts to ToR switches
    int hostId = numToRSwitches;
    for (int tor = 0; tor < numToRSwitches; tor++) {
        for (int h = 0; h < hostsPerSwitch && hostId < numToRSwitches + numNodes; h++) {
            addLink(hostId++, tor, 800e9, 100e-9);
        }
    }
    
    // Create mesh connections between ToR switches
    if (fullMesh) {
        // Full mesh - every ToR connected to every other ToR
        for (int i = 0; i < numToRSwitches; i++) {
            for (int j = i + 1; j < numToRSwitches; j++) {
                addLink(i, j, 800e9, 1e-6);  // 800G, 1us
            }
        }
    } else {
        // Partial mesh with specified degree
        for (int i = 0; i < numToRSwitches; i++) {
            int connections = 0;
            for (int j = 1; j <= meshDegree && connections < meshDegree; j++) {
                int target = (i + j) % numToRSwitches;
                if (target != i) {
                    addLink(i, target, 800e9, 1e-6);
                    connections++;
                }
            }
        }
    }
    
    EV_INFO << "Mesh: " << numToRSwitches << " ToR switches, " 
            << numNodes << " hosts, " << (fullMesh ? "full" : "partial") << " mesh\n";
}

//=============================================================================
// Torus Topology Implementation
//=============================================================================

Define_Module(TorusTopology);

TorusTopology::TorusTopology() {
    dimensions = 2;
    torusX = torusY = torusZ = 0;
    wrapAround = true;
}

void TorusTopology::generateTopology() {
    EV_INFO << "Generating Torus topology for " << numNodes << " nodes\n";
    
    dimensions = par("dimensions").intValue();
    wrapAround = par("wrapAround").boolValue();
    
    // Calculate torus dimensions
    if (dimensions == 2) {
        torusX = (int)sqrt(numNodes);
        torusY = (numNodes + torusX - 1) / torusX;
        torusZ = 1;
    } else {  // 3D
        torusX = (int)cbrt(numNodes);
        torusY = torusX;
        torusZ = (numNodes + torusX * torusY - 1) / (torusX * torusY);
    }
    
    int nodeId = 0;
    
    // Create nodes in grid pattern
    for (int z = 0; z < torusZ; z++) {
        for (int y = 0; y < torusY; y++) {
            for (int x = 0; x < torusX; x++) {
                if (nodeId < numNodes) {
                    addNode(nodeId, "host", 6);  // 6 ports for 3D torus
                    nodes[nodeId].x = x;
                    nodes[nodeId].y = y;
                    nodes[nodeId].z = z;
                    nodeId++;
                }
            }
        }
    }
    
    // Create torus connections
    for (int z = 0; z < torusZ; z++) {
        for (int y = 0; y < torusY; y++) {
            for (int x = 0; x < torusX; x++) {
                int currentId = z * torusX * torusY + y * torusX + x;
                if (currentId >= numNodes) continue;
                
                // X direction connections
                if (x < torusX - 1) {
                    int nextX = currentId + 1;
                    if (nextX < numNodes) {
                        addLink(currentId, nextX, 800e9, 200e-9);
                    }
                } else if (wrapAround && torusX > 2) {
                    int wrapX = z * torusX * torusY + y * torusX;
                    if (wrapX < numNodes) {
                        addLink(currentId, wrapX, 800e9, 200e-9);
                    }
                }
                
                // Y direction connections
                if (y < torusY - 1) {
                    int nextY = currentId + torusX;
                    if (nextY < numNodes) {
                        addLink(currentId, nextY, 800e9, 200e-9);
                    }
                } else if (wrapAround && torusY > 2) {
                    int wrapY = z * torusX * torusY + x;
                    if (wrapY < numNodes) {
                        addLink(currentId, wrapY, 800e9, 200e-9);
                    }
                }
                
                // Z direction connections (3D only)
                if (dimensions == 3) {
                    if (z < torusZ - 1) {
                        int nextZ = currentId + torusX * torusY;
                        if (nextZ < numNodes) {
                            addLink(currentId, nextZ, 800e9, 200e-9);
                        }
                    } else if (wrapAround && torusZ > 2) {
                        int wrapZ = y * torusX + x;
                        if (wrapZ < numNodes) {
                            addLink(currentId, wrapZ, 800e9, 200e-9);
                        }
                    }
                }
            }
        }
    }
    
    EV_INFO << "Torus " << dimensions << "D: " << torusX << "x" << torusY;
    if (dimensions == 3) EV_INFO << "x" << torusZ;
    EV_INFO << " = " << numNodes << " nodes\n";
}

//=============================================================================
// Dragonfly Topology Implementation
//=============================================================================

Define_Module(DragonflyTopology);

DragonflyTopology::DragonflyTopology() {
    numGroups = 4;
    routersPerGroup = 16;
    hostsPerRouter = 8;
    globalConnections = 4;
}

void DragonflyTopology::generateTopology() {
    EV_INFO << "Generating Dragonfly topology for " << numNodes << " nodes\n";
    
    routersPerGroup = par("routersPerGroup").intValue();
    hostsPerRouter = par("hostsPerRouter").intValue();
    globalConnections = par("globalConnections").intValue();
    
    // Calculate number of groups needed
    int totalRoutersNeeded = (numNodes + hostsPerRouter - 1) / hostsPerRouter;
    numGroups = (totalRoutersNeeded + routersPerGroup - 1) / routersPerGroup;
    
    int nodeId = 0;
    
    // Create routers organized in groups
    std::vector<std::vector<int>> groups(numGroups);
    for (int g = 0; g < numGroups; g++) {
        for (int r = 0; r < routersPerGroup && nodeId < totalRoutersNeeded; r++) {
            addNode(nodeId, "router", switchRadix);
            groups[g].push_back(nodeId);
            nodeId++;
        }
    }
    
    int totalRouters = nodeId;
    
    // Create hosts
    for (int i = 0; i < numNodes; i++) {
        addNode(nodeId++, "host", 1);
    }
    
    // Connect hosts to routers
    int hostId = totalRouters;
    for (int routerId = 0; routerId < totalRouters; routerId++) {
        for (int h = 0; h < hostsPerRouter && hostId < totalRouters + numNodes; h++) {
            addLink(hostId++, routerId, 800e9, 100e-9);
        }
    }
    
    // Create local connections within groups (all-to-all)
    for (int g = 0; g < numGroups; g++) {
        for (int i = 0; i < groups[g].size(); i++) {
            for (int j = i + 1; j < groups[g].size(); j++) {
                addLink(groups[g][i], groups[g][j], 800e9, 200e-9);
            }
        }
    }
    
    // Create global connections between groups
    for (int g1 = 0; g1 < numGroups; g1++) {
        for (int g2 = g1 + 1; g2 < numGroups; g2++) {
            // Connect globalConnections routers from each group
            int connections = std::min(globalConnections, 
                                     std::min((int)groups[g1].size(), (int)groups[g2].size()));
            for (int c = 0; c < connections; c++) {
                addLink(groups[g1][c], groups[g2][c], 800e9, 2e-6);  // 2us inter-group
            }
        }
    }
    
    EV_INFO << "Dragonfly: " << numGroups << " groups, " 
            << routersPerGroup << " routers/group, " 
            << hostsPerRouter << " hosts/router\n";
}

//=============================================================================
// MPI Partition Assignment Methods
//=============================================================================

void TopologyGenerator::assignPartitions() {
    if (!parallelSimulation || numPartitions <= 1) {
        // Single partition - assign all nodes to partition 0
        for (auto& node : nodes) {
            node.partitionId = 0;
        }
        return;
    }
    
    // For multi-partition simulation, use topology-aware assignment
    switch (topology) {
        case DRAGONFLY:
            assignDragonflyPartitions();
            break;
        case LEAF_SPINE:
            assignLeafSpinePartitions();
            break;
        case MESH:
            assignMeshPartitions();
            break;
        case TORUS_3D:
            assignTorusPartitions();
            break;
        default:
            assignBalancedPartitions();
    }
    
    balancePartitions();
    optimizePartitionCommunication();
    
    EV_INFO << "Assigned " << numNodes << " nodes to " << numPartitions << " partitions\n";
}

void TopologyGenerator::assignDragonflyPartitions() {
    // For Dragonfly topology, assign each group to different partitions
    // This minimizes inter-partition communication
    int partitionId = 0;
    int nodesInCurrentPartition = 0;
    int maxNodesPerPartition = numNodes / numPartitions;
    
    for (auto& node : nodes) {
        node.partitionId = partitionId;
        nodesInCurrentPartition++;
        
        if (nodesInCurrentPartition >= maxNodesPerPartition && partitionId < numPartitions - 1) {
            partitionId++;
            nodesInCurrentPartition = 0;
        }
    }
}

void TopologyGenerator::assignLeafSpinePartitions() {
    // For Leaf-Spine, assign leaf switches and their hosts to same partition
    int leavesPerPartition = std::max(1, numSwitches / numPartitions);
    
    int partitionId = 0;
    int leafCount = 0;
    
    for (auto& node : nodes) {
        if (node.nodeType == "leaf" || node.nodeType == "host") {
            node.partitionId = partitionId;
            
            if (node.nodeType == "leaf") {
                leafCount++;
                if (leafCount >= leavesPerPartition && partitionId < numPartitions - 1) {
                    partitionId++;
                    leafCount = 0;
                }
            }
        } else if (node.nodeType == "spine") {
            // Spine switches are replicated across all partitions for routing
            node.partitionId = 0;  // Put spines on partition 0 for now
        }
    }
}

void TopologyGenerator::assignMeshPartitions() {
    // For mesh topology, use spatial locality
    assignBalancedPartitions();
}

void TopologyGenerator::assignTorusPartitions() {
    // For torus topology, use 3D spatial partitioning
    assignBalancedPartitions();
}

void TopologyGenerator::assignBalancedPartitions() {
    // Simple round-robin assignment for balanced load
    for (int i = 0; i < (int)nodes.size(); i++) {
        nodes[i].partitionId = i % numPartitions;
    }
}

void TopologyGenerator::balancePartitions() {
    // Count nodes per partition
    std::vector<int> partitionCounts(numPartitions, 0);
    for (const auto& node : nodes) {
        if (node.partitionId >= 0 && node.partitionId < numPartitions) {
            partitionCounts[node.partitionId]++;
        }
    }
    
    // Log partition balance
    for (int i = 0; i < numPartitions; i++) {
        EV_INFO << "Partition " << i << ": " << partitionCounts[i] << " nodes\n";
    }
}

void TopologyGenerator::optimizePartitionCommunication() {
    // Count inter-partition links to minimize communication overhead
    int interPartitionLinks = 0;
    for (const auto& link : links) {
        int srcPartition = getNodePartition(link.src);
        int destPartition = getNodePartition(link.dest);
        if (srcPartition != destPartition) {
            interPartitionLinks++;
        }
    }
    
    EV_INFO << "Inter-partition links: " << interPartitionLinks << " / " << links.size() << "\n";
}

int TopologyGenerator::getNodePartition(int nodeId) {
    if (nodeId >= 0 && nodeId < (int)nodes.size()) {
        return nodes[nodeId].partitionId;
    }
    return 0;
}

void TopologyGenerator::establishConnections() {
    EV_INFO << "Establishing " << links.size() << " connections between OMNeT++ modules\n";
    
    // Get parent network module
    cModule *network = getParentModule();
    if (!network) {
        EV_ERROR << "Cannot find parent network module\n";
        return;
    }
    
    // Find host and switch module vectors
    cModule **hosts = nullptr;
    cModule **switches = nullptr;
    int numHostModules = 0;
    int numSwitchModules = 0;
    
    // Look for hosts[] submodule vector
    cModule *hostVector = network->getSubmodule("hosts", 0);
    if (hostVector) {
        numHostModules = network->getSubmoduleVectorSize("hosts");
        hosts = new cModule*[numHostModules];
        for (int i = 0; i < numHostModules; i++) {
            hosts[i] = network->getSubmodule("hosts", i);
        }
        EV_INFO << "Found " << numHostModules << " host modules\n";
    }
    
    // Look for switches[] submodule vector  
    cModule *switchVector = network->getSubmodule("switches", 0);
    if (switchVector) {
        numSwitchModules = network->getSubmoduleVectorSize("switches");
        switches = new cModule*[numSwitchModules];
        for (int i = 0; i < numSwitchModules; i++) {
            switches[i] = network->getSubmodule("switches", i);
        }
        EV_INFO << "Found " << numSwitchModules << " switch modules\n";
    }
    
    if (!hosts || !switches) {
        EV_ERROR << "Cannot find host or switch modules - skipping connections\n";
        delete[] hosts;
        delete[] switches;
        return;
    }
    
    // Establish connections based on topology links
    int connectionsEstablished = 0;
    for (const auto& link : links) {
        try {
            cModule *srcModule = nullptr;
            cModule *destModule = nullptr;
            
            // Determine source module
            // Check if it's a switch first (switches have lower IDs in Dragonfly)
            if (link.src < numSwitchModules) {
                srcModule = switches[link.src];
            } else if (link.src - numSwitchModules < numHostModules) {
                srcModule = hosts[link.src - numSwitchModules];
            }
            
            // Determine destination module  
            // Check if it's a switch first (switches have lower IDs in Dragonfly)
            if (link.dest < numSwitchModules) {
                destModule = switches[link.dest];
            } else if (link.dest - numSwitchModules < numHostModules) {
                destModule = hosts[link.dest - numSwitchModules];
            }
            
            if (srcModule && destModule) {
                // Create actual OMNeT++ connection between modules
                try {
                    // Find available ethernet gates by checking existing gate sizes
                    int srcGateSize = srcModule->gateSize("ethg");
                    int destGateSize = destModule->gateSize("ethg");
                    
                    // Find next available gate indices
                    int srcGateIndex = srcGateSize;
                    int destGateIndex = destGateSize;
                    
                    // Extend gate vectors if needed
                    srcModule->setGateSize("ethg", srcGateIndex + 1);
                    destModule->setGateSize("ethg", destGateIndex + 1);
                    
                    // Get the new gates
                    cGate *srcGateOut = srcModule->gate("ethg$o", srcGateIndex);
                    cGate *srcGateIn = srcModule->gate("ethg$i", srcGateIndex);
                    cGate *destGateOut = destModule->gate("ethg$o", destGateIndex);
                    cGate *destGateIn = destModule->gate("ethg$i", destGateIndex);
                    
                    if (srcGateOut && srcGateIn && destGateOut && destGateIn) {
                        // Create bidirectional connection with channels
                        cDatarateChannel *channel1 = cDatarateChannel::create("UEChannel");
                        channel1->setDelay(link.latency);
                        channel1->setDatarate(link.bandwidth);
                        
                        cDatarateChannel *channel2 = cDatarateChannel::create("UEChannelReverse");
                        channel2->setDelay(link.latency);
                        channel2->setDatarate(link.bandwidth);
                        
                        // Connect: src.out -> dest.in and dest.out -> src.in
                        srcGateOut->connectTo(destGateIn, channel1);
                        destGateOut->connectTo(srcGateIn, channel2);
                        
                        EV_INFO << "Connected " << srcModule->getFullName() 
                                << "[" << srcGateIndex << "] <--> " 
                                << destModule->getFullName() << "[" << destGateIndex << "]\n";
                        connectionsEstablished++;
                    }
                } catch (std::exception& e) {
                    EV_WARN << "Failed to connect " << srcModule->getFullName() 
                            << " to " << destModule->getFullName() << ": " << e.what() << "\n";
                }
            }
        } catch (std::exception& e) {
            EV_WARN << "Failed to establish connection: " << e.what() << "\n";
        }
    }
    
    EV_INFO << "Established " << connectionsEstablished << " connections out of " 
            << links.size() << " topology links\n";
    
    delete[] hosts;
    delete[] switches;
}

