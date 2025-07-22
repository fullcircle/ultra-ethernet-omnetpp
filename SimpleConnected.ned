//
// SimpleConnected.ned - Simple working connected network
//

// Test network with 2 hosts connected by a simple channel
network TwoHostChannel
{
    parameters:
        @display("bgb=600,300");
        
    submodules:
        hostA: UltraEthernetHost_Test {
            @display("p=200,150");
        }
        
        hostB: UltraEthernetHost_Test {
            @display("p=400,150");
        }
        
    connections:
        // Direct connection between hosts using simple delay channel
        hostA.out --> {delay = 1us; datarate = 800Gbps;} --> hostB.in;
        hostB.out --> {delay = 1us; datarate = 800Gbps;} --> hostA.in;
}

// Network with multiple hosts sharing a common bus
network SharedBus
{
    parameters:
        int numHosts = default(4);
        @display("bgb=800,400");
        
    submodules:
        host[numHosts]: UltraEthernetHost_Test {
            @display("p=100+100*index,200");
        }
}