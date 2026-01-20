//
// TopologyGenerator.h - Base class for data center topology generation
//

#ifndef __TOPOLOGY_GENERATOR_H
#define __TOPOLOGY_GENERATOR_H

#include <omnetpp.h>
#include <vector>
#include <map>
#include <string>

using namespace omnetpp;

enum TopologyType {
    LEAF_SPINE,
    MESH,
    TORUS_2D,
    TORUS_3D,
    DRAGONFLY,
    FAT_TREE,
    HYPERCUBE
};

struct NodeInfo {
    int nodeId;
    std::string nodeType;  // "host", "tor", "leaf", "spine", "core"
    int switchPorts;
    std::vector<int> connections;
    int x, y, z;  // Position coordinates for geometric topologies
    int partitionId;  // MPI partition assignment
};

struct LinkInfo {
    int src, dest;
    double bandwidth;
    double latency;
    std::string linkType;  // "host-tor", "leaf-spine", "inter-group", etc.
};

class TopologyGenerator : public cSimpleModule {
protected:
    // Configuration parameters
    int numNodes;
    int numSwitches;
    TopologyType topology;
    int switchRadix;
    int hostsPerSwitch;
    
    // MPI partition configuration
    int numPartitions;
    bool parallelSimulation;
    
    // Generated topology
    std::vector<NodeInfo> nodes;
    std::vector<LinkInfo> links;
    std::map<std::string, int> topologyParams;
    
    // Virtual methods for topology-specific generation
    virtual void generateTopology() {}
    virtual void calculatePaths() {}
    virtual void optimizeTopology() {}
    
    // Helper methods
    void addNode(int id, const std::string& type, int ports = 64);
    void addLink(int src, int dest, double bw = 800e9, double lat = 1e-6);
    void validateTopology();
    void exportTopology(const std::string& filename);
    void establishConnections();  // Establish actual OMNeT++ module connections
    
    // MPI partition methods
    virtual void assignPartitions();
    void assignDragonflyPartitions();
    void assignLeafSpinePartitions();
    void assignMeshPartitions();
    void assignTorusPartitions();
    void assignBalancedPartitions();
    void balancePartitions();
    void optimizePartitionCommunication();
    int getNodePartition(int nodeId);
    
public:
    TopologyGenerator();
    virtual ~TopologyGenerator();
    
    // Public interface
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    
    // Accessors
    const std::vector<NodeInfo>& getNodes() const { return nodes; }
    const std::vector<LinkInfo>& getLinks() const { return links; }
    int getNumNodes() const { return numNodes; }
    int getNumSwitches() const { return numSwitches; }
};

// Leaf-Spine Topology Generator
class LeafSpineTopology : public TopologyGenerator {
private:
    int numLeafSwitches;
    int numSpineSwitches;
    int hostsPerLeaf;
    
protected:
    virtual void generateTopology() override;
    
public:
    LeafSpineTopology();
};

// Full Mesh Topology Generator  
class MeshTopology : public TopologyGenerator {
private:
    bool fullMesh;
    int meshDegree;
    
protected:
    virtual void generateTopology() override;
    
public:
    MeshTopology();
};

// 2D/3D Torus Topology Generator
class TorusTopology : public TopologyGenerator {
private:
    int dimensions;
    int torusX, torusY, torusZ;
    bool wrapAround;
    
protected:
    virtual void generateTopology() override;
    
public:
    TorusTopology();
};

// Dragonfly Topology Generator
class DragonflyTopology : public TopologyGenerator {
private:
    int numGroups;
    int routersPerGroup;
    int hostsPerRouter;
    int globalConnections;
    
protected:
    virtual void generateTopology() override;
    
public:
    DragonflyTopology();
};

#endif