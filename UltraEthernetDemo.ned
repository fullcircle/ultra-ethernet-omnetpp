//
// UltraEthernetDemo.ned - Visual demonstration network
//

network UltraEthernetDemo {
    parameters:
        int numHosts = default(4);
        
    submodules:
        // AI Training Hosts in a visual layout
        host[numHosts]: UltraEthernetHost {
            @display("p=100+150*($i%2),100+200*($i/2);i=device/server;is=l");
        }
        
        // Central Ultra Ethernet Switch
        switch: UltraEthernetSwitch {
            @display("p=175,200;i=device/switch;is=l");
            numPorts = parent.numHosts;
        }
        
        // Performance Analyzer for monitoring
        analyzer: PerformanceAnalyzer {
            @display("p=350,50;i=block/cogwheel;is=l");
        }
        
    connections:
        // Connect all hosts to the central switch
        for i=0..numHosts-1 {
            host[i].ethg[0] <--> {delay=1us; datarate=800Gbps; @display("ls=blue,3");} <--> switch.ethg[i];
        }
}