//
// SimpleTest.ned - Simple test network for UltraEthernetPhy
//

network SimpleTest {
    submodules:
        phy1: UltraEthernetPhy {
            @display("p=100,100");
        }
        
        phy2: UltraEthernetPhy {
            @display("p=300,100");
        }
        
    connections:
        phy1.linkOut --> phy2.linkIn;
        phy2.linkOut --> phy1.linkIn;
}