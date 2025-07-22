//
// SimpleTrafficTest.ned - Simple working traffic test
//

network SimpleTrafficTest {
    submodules:
        host1: UltraEthernetHost {
            @display("p=200,100");
        }
        
        host2: UltraEthernetHost {
            @display("p=400,100");
        }
        
    connections allowunconnected:
        // Connect the physical layers directly for loopback testing
}