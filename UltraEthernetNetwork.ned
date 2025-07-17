//
// UltraEthernetNetwork.ned - Network topology definitions
//

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;

// Ultra Ethernet Host Module
module UltraEthernetHost {
    parameters:
        @node();
        @display("i=device/server;bgb=400,500");
        
        // Configurable parameters
        string profileType = default("AI_FULL");  // AI_BASE, AI_FULL, HPC
        double linkSpeed @unit(bps) = default(800Gbps);
        bool llrEnabled = default(true);
        bool incEnabled = default(true);
        
    gates:
        inout ethg[] @labels(EtherFrame-conn);
        
    submodules:
        app: AIHPCApplication {
            @display("p=200,50");
        }
        
        transport: UETTransport {
            @display("p=200,150");
        }
        
        networkLayer: UltraEthernetIP {
            @display("p=200,250");
        }
        
        link: UltraEthernetLink {
            @display("p=200,350");
        }
        
        phy: UltraEthernetPhy {
            @display("p=200,450");
        }
        
    connections:
        app.transportOut --> transport.appIn;
        app.transportIn <-- transport.appOut;
        
        transport.networkOut --> networkLayer.transportIn;
        transport.networkIn <-- networkLayer.transportOut;
        
        networkLayer.linkOut --> link.networkIn;
        networkLayer.linkIn <-- link.networkOut;
        
        link.phyOut --> phy.linkIn;
        link.phyIn <-- phy.linkOut;
        
        for i=0..sizeof(ethg)-1 {
            phy.ethOut[i] --> ethg[i];
            phy.ethIn[i] <-- ethg[i];
        }
}

// Ultra Ethernet Switch with INC support
module UltraEthernetSwitch {
    parameters:
        @node();
        @display("i=device/switch;bgb=300,200");
        
        int numPorts = default(64);
        bool incProcessingEnabled = default(true);
        double switchingLatency @unit(s) = default(100ns);
        
    gates:
        inout ethg[numPorts] @labels(EtherFrame-conn);
        
    submodules:
        switchFabric: SwitchFabric {
            @display("p=150,100");
            numPorts = parent.numPorts;
        }
        
        incProcessor: INCProcessor {
            @display("p=150,150");
            enabled = parent.incProcessingEnabled;
        }
        
        ports[numPorts]: SwitchPort {
            @display("p=50,50,r,50");
        }
        
    connections:
        for i=0..numPorts-1 {
            ports[i].fabricOut --> switchFabric.portIn[i];
            ports[i].fabricIn <-- switchFabric.portOut[i];
            ports[i].ethOut --> ethg[i];
            ports[i].ethIn <-- ethg[i];
        }
        
        switchFabric.incOut --> incProcessor.fabricIn;
        switchFabric.incIn <-- incProcessor.fabricOut;
}

// Large-scale cluster network
network UltraEthernetCluster {
    parameters:
        int numNodes = default(1024);
        int switchRadix = default(64);
        string topologyType = default("DRAGONFLY");
        
        // Parallel simulation support
        int numPartitions = default(4);
        bool parallelSimulation = default(true);
        
    submodules:
        configurator: Ipv4NetworkConfigurator {
            @display("p=50,50");
        }
        
        visualizer: IntegratedCanvasVisualizer {
            @display("p=50,150");
        }
        
        hosts[numNodes]: UltraEthernetHost {
            @display("p=100,100,m,20,100,100");
        }
        
        switches[numNodes/switchRadix]: UltraEthernetSwitch {
            numPorts = switchRadix;
        }
        
        // Performance measurement and analysis
        performanceAnalyzer: PerformanceAnalyzer {
            @display("p=50,250");
        }
        
    connections allowunconnected:
        // Topology connections defined by topology generator
        // See UltraEthernetTopology.cc for implementation
}