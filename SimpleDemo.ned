//
// SimpleDemo.ned - Simple animated demonstration
//

network SimpleDemo {
    submodules:
        host1: UltraEthernetHost {
            @display("p=100,100;i=device/server;is=l");
        }
        
        host2: UltraEthernetHost {
            @display("p=300,100;i=device/server;is=l");
        }
        
        host3: UltraEthernetHost {
            @display("p=100,300;i=device/server;is=l");
        }
        
        host4: UltraEthernetHost {
            @display("p=300,300;i=device/server;is=l");
        }
        
    connections:
        // Simple ring topology for visualization
        host1.ethg[0] <--> {delay=1us; datarate=800Gbps; @display("ls=blue,3");} <--> host2.ethg[0];
        host2.ethg[1] <--> {delay=1us; datarate=800Gbps; @display("ls=green,3");} <--> host3.ethg[0];
        host3.ethg[1] <--> {delay=1us; datarate=800Gbps; @display("ls=red,3");} <--> host4.ethg[0];
        host4.ethg[1] <--> {delay=1us; datarate=800Gbps; @display("ls=yellow,3");} <--> host1.ethg[1];
}