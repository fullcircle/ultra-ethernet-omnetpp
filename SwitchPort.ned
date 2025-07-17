//
// SwitchPort.ned - Switch Port Module
//

simple SwitchPort {
    parameters:
        double processingLatency @unit(s) = default(10ns);
        
        @display("i=block/port");
        
    gates:
        input fabricIn;
        output fabricOut;
        input ethIn;
        output ethOut;
}