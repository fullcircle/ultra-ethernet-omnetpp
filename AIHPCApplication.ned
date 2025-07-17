//
// AIHPCApplication.ned - AI/HPC Application Layer
//

simple AIHPCApplication {
    parameters:
        string workloadType = default("AI_TRAINING");  // AI_TRAINING, AI_INFERENCE, HPC_SIMULATION
        string communicationPattern = default("ALLREDUCE");  // ALLREDUCE, ALLGATHER, BROADCAST
        int messageSize @unit(B) = default(1MB);
        int jobSize = default(1024);
        double communicationIntensity = default(0.8);
        double trafficStartTime @unit(s) = default(1s);
        double trafficRate @unit(bps) = default(1Gbps);
        
        // Statistics
        @signal[messagesSent](type=long);
        @signal[messagesReceived](type=long);
        @signal[throughput](type=double);
        @signal[latency](type=simtime_t);
        
        @statistic[messagesSent](title="Messages Sent"; record=count,sum);
        @statistic[messagesReceived](title="Messages Received"; record=count,sum);
        @statistic[throughput](title="Throughput"; record=mean,max);
        @statistic[latency](title="Latency"; record=mean,max,histogram);
        
        @display("i=block/app");
        
    gates:
        output transportOut;
        input transportIn;
}