//
// ScalableNetworks.ned - Large-scale data center network topologies
//

import inet.node.ethernet.EthernetHost;

//=============================================================================
// Base Scalable Network Module
//=============================================================================

module ScalableUltraEthernetNetwork
{
    parameters:
        int numNodes = default(100);
        string topologyType = default("LEAF_SPINE");
        int switchRadix = default(64);
        int hostsPerSwitch = default(32);
        
        // PARSIM partition support
        int numPartitions = default(4);
        bool parallelSimulation = default(true);
        
        // Calculate number of switches needed based on topology
        int numSwitches = default(int((numNodes + hostsPerRouter - 1) / hostsPerRouter));
        
        // Topology-specific parameters
        int numSpineSwitches = default(4);
        int hostsPerLeaf = default(32);
        bool fullMesh = default(false);
        int meshDegree = default(4);
        int dimensions = default(2);
        bool wrapAround = default(true);
        int routersPerGroup = default(16);
        int hostsPerRouter = default(8);
        int globalConnections = default(4);
        
        @display("bgb=800,600");
        
    submodules:
        // Create actual host modules for traffic generation
        hosts[numNodes]: UltraEthernetHost {
            @display("p=100,100,m,20,100,100");
        }
        
        // Create switch modules based on topology requirements
        switches[numSwitches]: UltraEthernetSwitch {
            numPorts = parent.switchRadix;
            @display("p=300,300,m,10,80,80");
        }
        
        // Topology generator for connection setup
        topologyGenerator: <topologyType + "Topology"> like TopologyGenerator {
            parameters:
                numNodes = parent.numNodes;
                switchRadix = parent.switchRadix;
                hostsPerSwitch = parent.hostsPerSwitch;
                topologyType = parent.topologyType;
                
                // PARSIM partition parameters
                numPartitions = parent.numPartitions;
                parallelSimulation = parent.parallelSimulation;
                
                // Leaf-Spine specific
                numSpineSwitches = parent.numSpineSwitches;
                hostsPerLeaf = parent.hostsPerLeaf;
                
                // Mesh specific  
                fullMesh = parent.fullMesh;
                meshDegree = parent.meshDegree;
                
                // Torus specific
                dimensions = parent.dimensions;
                wrapAround = parent.wrapAround;
                
                // Dragonfly specific
                routersPerGroup = parent.routersPerGroup;
                hostsPerRouter = parent.hostsPerRouter;
                globalConnections = parent.globalConnections;
                
                @display("p=50,50;i=block/cogwheel");
        }
        
    connections allowunconnected:
        // Connections will be established dynamically by topology generator
}

//=============================================================================
// 100-Node Test Networks
//=============================================================================

network UltraEthernet_100_LeafSpine extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 100;
        topologyType = "LeafSpine";
        numSpineSwitches = 4;
        hostsPerLeaf = 25;
        switchRadix = 64;
}

network UltraEthernet_100_Mesh extends ScalableUltraEthernetNetwork  
{
    parameters:
        numNodes = 100;
        topologyType = "Mesh";
        fullMesh = false;
        meshDegree = 6;
        hostsPerSwitch = 25;
}

network UltraEthernet_100_Torus2D extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 100;
        topologyType = "Torus";
        dimensions = 2;
        wrapAround = true;
}

network UltraEthernet_100_Dragonfly extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 100;
        topologyType = "Dragonfly"; 
        routersPerGroup = 8;
        hostsPerRouter = 8;
        globalConnections = 4;
}

//=============================================================================
// 1000-Node Test Networks
//=============================================================================

network UltraEthernet_1K_LeafSpine extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 1024;
        topologyType = "LeafSpine";
        numSpineSwitches = 16;
        hostsPerLeaf = 32;
        switchRadix = 64;
}

network UltraEthernet_1K_Mesh extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 1024;
        topologyType = "Mesh";
        fullMesh = false;
        meshDegree = 8;
        hostsPerSwitch = 32;
}

network UltraEthernet_1K_Torus3D extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 1024;
        topologyType = "Torus";
        dimensions = 3;
        wrapAround = true;
}

network UltraEthernet_1K_Dragonfly extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 1024;
        topologyType = "Dragonfly";
        routersPerGroup = 16;
        hostsPerRouter = 8;
        globalConnections = 6;
}

//=============================================================================
// 10K-Node Test Networks (for extreme scale testing)
//=============================================================================

network UltraEthernet_10K_LeafSpine extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 10000;
        topologyType = "LeafSpine";
        numSpineSwitches = 64;
        hostsPerLeaf = 48;
        switchRadix = 128;
}

network UltraEthernet_10K_Dragonfly extends ScalableUltraEthernetNetwork
{
    parameters:
        numNodes = 10000;
        topologyType = "Dragonfly";
        routersPerGroup = 25;
        hostsPerRouter = 10;
        globalConnections = 8;
}