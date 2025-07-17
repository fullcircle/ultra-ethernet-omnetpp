//
// UltraEthernetPhy.ned - Physical Layer Module Definition
//

simple UltraEthernetPhy {
    parameters:
        double linkSpeed @unit(bps) = default(800Gbps);
        double fecOverhead = default(0.12);
        double errorRate = default(1e-12);
        int fecCorrectionBits = default(8);
        bool fecEnabled = default(true);
        
        // Statistics
        @signal[fecCorrections](type=long);
        @signal[uncorrectableErrors](type=long);
        @signal[linkUtilization](type=double);
        
        @statistic[fecCorrections](title="FEC Corrections"; record=count,sum);
        @statistic[uncorrectableErrors](title="Uncorrectable Errors"; record=count,sum);
        @statistic[linkUtilization](title="Link Utilization"; record=mean,max);
        
        @display("i=block/tx");
        
    gates:
        input linkIn;
        output linkOut;
        inout ethg[] @labels(EtherFrame-conn);
        
        // For compatibility with the network definition
        input ethIn[] @labels(EtherFrame-conn);
        output ethOut[] @labels(EtherFrame-conn);
}