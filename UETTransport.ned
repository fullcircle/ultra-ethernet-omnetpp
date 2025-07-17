//
// UETTransport.ned - Ultra Ethernet Transport Layer
//

simple UETTransport {
    parameters:
        string profileType = default("AI_FULL");  // AI_BASE, AI_FULL, HPC
        bool packetSprayingEnabled = default(true);
        bool reorderingEnabled = default(true);
        int maxReorderBuffer = default(256);
        int initialCongestionWindow = default(10);
        double rdmaTimeout @unit(s) = default(1us);
        int maxRetransmissions = default(3);
        
        // Statistics
        @signal[packetsTransmitted](type=long);
        @signal[packetsReceived](type=long);
        @signal[retransmissions](type=long);
        @signal[congestionWindow](type=long);
        @signal[roundTripTime](type=simtime_t);
        
        @statistic[packetsTransmitted](title="Packets Transmitted"; record=count,sum);
        @statistic[packetsReceived](title="Packets Received"; record=count,sum);
        @statistic[retransmissions](title="Retransmissions"; record=count,sum);
        @statistic[congestionWindow](title="Congestion Window"; record=mean,max);
        @statistic[roundTripTime](title="Round Trip Time"; record=mean,max,histogram);
        
        @display("i=block/transport");
        
    gates:
        input appIn;
        output appOut;
        input networkIn;
        output networkOut;
}