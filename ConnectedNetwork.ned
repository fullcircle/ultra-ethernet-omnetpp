//
// ConnectedNetwork.ned - Actually connected multi-host networks
//

// Simple bus network - all hosts connected to shared medium
network BusNetwork
{
    parameters:
        int numHosts = default(4);
        @display("bgb=800,400");
        
    submodules:
        
        host[numHosts]: UltraEthernetHost {
            @display("p=100+100*index,200");
        }
        
        switch: UltraEthernetSwitch {
            @display("p=400,100");
            numPorts = parent.numHosts;
        }
        
    connections:
        for i=0..numHosts-1 {
            host[i].ethg++ <--> switch.ethg[i];
        }
}

// Star network with central switch
network StarNetwork  
{
    parameters:
        int numHosts = default(16);
        @display("bgb=1000,800");
        
    submodules:
        
        host[numHosts]: UltraEthernetHost {
            @display("p=500+300*cos(2*3.14159*index/numHosts),400+200*sin(2*3.14159*index/numHosts)");
        }
        
        centralSwitch: UltraEthernetSwitch {
            @display("p=500,400;i=device/switch");
            numPorts = parent.numHosts;
        }
        
    connections:
        for i=0..numHosts-1 {
            host[i].ethg++ <--> centralSwitch.ethg[i];
        }
}

// Leaf-Spine network with actual connections
network LeafSpineConnected
{
    parameters:
        int numHosts = default(64);
        int hostsPerLeaf = default(16);
        int numLeafSwitches = default((numHosts + hostsPerLeaf - 1) / hostsPerLeaf);
        int numSpineSwitches = default(4);
        @display("bgb=1200,800");
        
    submodules:
        
        host[numHosts]: UltraEthernetHost {
            @display("p=100+150*(index%8),400+80*(index/8)");
        }
        
        leafSwitch[numLeafSwitches]: UltraEthernetSwitch {
            @display("p=200+200*index,200");
            numPorts = parent.hostsPerLeaf + parent.numSpineSwitches;
        }
        
        spineSwitch[numSpineSwitches]: UltraEthernetSwitch {
            @display("p=300+200*index,100");
            numPorts = parent.numLeafSwitches;
        }
        
    connections:
        // Connect hosts to leaf switches
        for i=0..numHosts-1 {
            host[i].ethg++ <--> leafSwitch[i / hostsPerLeaf].ethg++;
        }
        
        // Connect leaf switches to spine switches (full mesh)
        for i=0..numLeafSwitches-1, for j=0..numSpineSwitches-1 {
            leafSwitch[i].ethg++ <--> spineSwitch[j].ethg++;
        }
}