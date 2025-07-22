//
// RealConnectedNetwork.ned - Actually connected Ultra Ethernet networks
//

// Simple 2-host directly connected network
network TwoHostConnected
{
    parameters:
        @display("bgb=600,300");
        
    submodules:
        host[2]: UltraEthernetHost {
            @display("p=200+200*index,150");
        }
        
    connections:
        // Direct connection between hosts
        host[0].ethg++ <--> {delay = 1us; datarate = 800Gbps;} <--> host[1].ethg++;
}

// 4-host star network with central switch
network FourHostStar
{
    parameters:
        @display("bgb=600,600");
        
    submodules:
        host[4]: UltraEthernetHost {
            @display("p=300+150*cos(2*3.14159*index/4),300+150*sin(2*3.14159*index/4)");
        }
        
        switch: UltraEthernetSwitch {
            @display("p=300,300;i=device/switch");
            numPorts = 4;
        }
        
    connections:
        for i=0..3 {
            host[i].ethg++ <--> {delay = 100ns; datarate = 800Gbps;} <--> switch.ethg[i];
        }
}

// 16-host switched network
network SixteenHostSwitched
{
    parameters:
        @display("bgb=800,800");
        
    submodules:
        host[16]: UltraEthernetHost {
            @display("p=100+150*(index%4),100+150*(index/4)");
        }
        
        switch: UltraEthernetSwitch {
            @display("p=400,400;i=device/switch");
            numPorts = 16;
        }
        
    connections:
        for i=0..15 {
            host[i].ethg++ <--> {delay = 100ns; datarate = 800Gbps;} <--> switch.ethg[i];
        }
}

// Large scale leaf-spine network
network LeafSpineScaled
{
    parameters:
        int numHosts = default(64);
        int hostsPerLeaf = default(16);
        int numLeafSwitches = default(4);  // numHosts / hostsPerLeaf
        int numSpineSwitches = default(4);
        @display("bgb=1200,800");
        
    submodules:
        host[numHosts]: UltraEthernetHost {
            @display("p=100+100*(index%8),200+80*(index/8)");
        }
        
        leafSwitch[numLeafSwitches]: UltraEthernetSwitch {
            @display("p=200+250*index,150");
            numPorts = hostsPerLeaf + numSpineSwitches;
        }
        
        spineSwitch[numSpineSwitches]: UltraEthernetSwitch {
            @display("p=300+200*index,50");
            numPorts = numLeafSwitches;
        }
        
    connections:
        // Connect hosts to leaf switches
        for i=0..numHosts-1 {
            host[i].ethg++ <--> {delay = 100ns; datarate = 800Gbps;} <--> leafSwitch[i / hostsPerLeaf].ethg++;
        }
        
        // Connect leaf switches to spine switches (full mesh)
        for i=0..numLeafSwitches-1, for j=0..numSpineSwitches-1 {
            leafSwitch[i].ethg++ <--> {delay = 500ns; datarate = 800Gbps;} <--> spineSwitch[j].ethg++;
        }
}