//
// SwitchFabric.ned - Switch Fabric Module
//

simple SwitchFabric {
    parameters:
        int numPorts = default(64);
        double switchingLatency @unit(s) = default(100ns);
        double bandwidth @unit(bps) = default(800Gbps);
        
        @display("i=block/switch");
        
    gates:
        input portIn[numPorts];
        output portOut[numPorts];
        input incIn;
        output incOut;
}