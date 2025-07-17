//
// UltraEthernetIP.ned - Ultra Ethernet Network Layer
//

simple UltraEthernetIP {
    parameters:
        double routingLatency @unit(s) = default(10ns);
        bool loadBalancingEnabled = default(true);
        int routingTableSize = default(1000);
        double routingUpdateInterval @unit(s) = default(1s);
        
        // Statistics
        @signal[packetsForwarded](type=long);
        @signal[packetsDropped](type=long);
        @signal[routingTableSize](type=long);
        @signal[forwardingLatency](type=simtime_t);
        
        @statistic[packetsForwarded](title="Packets Forwarded"; record=count,sum);
        @statistic[packetsDropped](title="Packets Dropped"; record=count,sum);
        @statistic[routingTableSize](title="Routing Table Size"; record=mean,max);
        @statistic[forwardingLatency](title="Forwarding Latency"; record=mean,max);
        
        @display("i=block/network");
        
    gates:
        input transportIn;
        output transportOut;
        input linkIn;
        output linkOut;
}