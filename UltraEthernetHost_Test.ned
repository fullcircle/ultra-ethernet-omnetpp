//
// UltraEthernetHost_Test.ned - Test network with single host
//

network UltraEthernetHost_Test {
    gates:
        inout ethg @labels(EtherFrame-conn);

    submodules:
        host: UltraEthernetHost {
            @display("p=200,200");
        }

    connections:
        ethg <--> host.ethg[0];
}