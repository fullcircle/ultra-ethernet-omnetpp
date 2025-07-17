//
// UltraEthernetHost_Test.ned - Test network with single host
//

network UltraEthernetHost_Test {
    submodules:
        host: UltraEthernetHost {
            @display("p=200,200");
        }
}