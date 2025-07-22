//
// TopologyGenerator.cc - Base topology generator implementation
//

#include "TopologyGenerator.h"
#include <algorithm>
#include <fstream>
#include <cmath>

Define_Module(TopologyGenerator);

TopologyGenerator::TopologyGenerator() {
    numNodes = 0;
    numSwitches = 0;
    topology = LEAF_SPINE;
    switchRadix = 64;
    hostsPerSwitch = 32;
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
    
    // Generate the topology
    generateTopology();
    validateTopology();
    
    // Export topology for visualization/analysis
    exportTopology("topology.txt");
    
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