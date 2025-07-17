//
// UltraEthernetLink.ned - Ultra Ethernet Link Layer
//

simple UltraEthernetLink {
    parameters:
        bool llrEnabled = default(true);
        double llrTimeout @unit(s) = default(1us);
        int maxRetransmissions = default(3);
        double priCompressionRatio = default(0.2);
        double linkLatency @unit(s) = default(1ns);
        
        // Statistics
        @signal[packetsTransmitted](type=long);
        @signal[packetsReceived](type=long);
        @signal[llrRetransmissions](type=long);
        @signal[compressionRatio](type=double);
        @signal[linkUtilization](type=double);
        
        @statistic[packetsTransmitted](title="Packets Transmitted"; record=count,sum);
        @statistic[packetsReceived](title="Packets Received"; record=count,sum);
        @statistic[llrRetransmissions](title="LLR Retransmissions"; record=count,sum);
        @statistic[compressionRatio](title="Compression Ratio"; record=mean,max);
        @statistic[linkUtilization](title="Link Utilization"; record=mean,max);
        
        @display("i=block/layer");
        
    gates:
        input networkIn;
        output networkOut;
        input phyIn;
        output phyOut;
}