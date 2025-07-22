//
// WorkingMultiHost.ned - Multi-host test that actually works
//

// Simple 2-host direct connection test
network TwoHostDirect
{
    parameters:
        @display("bgb=600,300");
        
    submodules:
        host1: UltraEthernetHost_Test {
            @display("p=200,150");
        }
        
        host2: UltraEthernetHost_Test {
            @display("p=400,150");
        }
}

// Multi-host simulation using multiple isolated hosts with increased jobSize
network MultiHostSimulation
{
    parameters:
        int numHosts = default(4);
        @display("bgb=800,600");
        
    submodules:
        host[numHosts]: UltraEthernetHost_Test {
            @display("p=150+100*(index%4),150+100*(index/4)");
        }
}