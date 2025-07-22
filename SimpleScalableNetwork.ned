//
// SimpleScalableNetwork.ned - Working scalable network with manual instantiation
//

network SimpleScalableNetwork_4
{
    parameters:
        @display("bgb=800,600");
        
    submodules:
        host[4]: UltraEthernetHost {
            @display("p=100+150*index,200");
        }
        
        switch: UltraEthernetSwitch {
            @display("p=400,100");
        }
        
    connections allowunconnected:
        // Connect each host to the switch
        for i=0..3 {
            host[i].ethg++ <--> switch.ethg++;
        }
}

network SimpleScalableNetwork_16
{
    parameters:
        @display("bgb=1200,800");
        
    submodules:
        host[16]: UltraEthernetHost {
            @display("p=100+100*(index%4),200+100*(index/4)");
        }
        
        leafSwitch[4]: UltraEthernetSwitch {
            @display("p=200+200*index,100");
        }
        
        spineSwitch[2]: UltraEthernetSwitch {
            @display("p=400+200*index,50");
        }
        
    connections allowunconnected:
        // Connect hosts to leaf switches (4 hosts per leaf)
        for i=0..15 {
            host[i].ethg++ <--> leafSwitch[i/4].ethg++;
        }
        
        // Connect leaf switches to spine switches (full mesh)
        for i=0..3, for j=0..1 {
            leafSwitch[i].ethg++ <--> spineSwitch[j].ethg++;
        }
}

network SimpleScalableNetwork_64
{
    parameters:
        @display("bgb=1600,1200");
        
    submodules:
        host[64]: UltraEthernetHost {
            @display("p=100+100*(index%8),300+80*(index/8)");
        }
        
        leafSwitch[8]: UltraEthernetSwitch {
            @display("p=150+150*index,200");
        }
        
        spineSwitch[4]: UltraEthernetSwitch {
            @display("p=300+200*index,100");
        }
        
    connections allowunconnected:
        // Connect hosts to leaf switches (8 hosts per leaf)
        for i=0..63 {
            host[i].ethg++ <--> leafSwitch[i/8].ethg++;
        }
        
        // Connect leaf switches to spine switches (full mesh)
        for i=0..7, for j=0..3 {
            leafSwitch[i].ethg++ <--> spineSwitch[j].ethg++;
        }
}