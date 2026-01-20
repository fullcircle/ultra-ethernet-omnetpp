//
// INCProcessor.ned - In-Network Collectives Processor
//

simple INCProcessor {
    parameters:
        bool enabled = default(true);
        double processingLatency @unit(s) = default(100ns);
        int maxConcurrentOperations = default(16);
        int bufferSize = default(1048576);  // 1MB in bytes
        
        // Statistics
        @signal[operationsProcessed](type=long);
        @signal[operationsDropped](type=long);
        @signal[processingLatency](type=simtime_t);
        @signal[bufferUtilization](type=double);
        
        @statistic[operationsProcessed](title="Operations Processed"; record=count,sum);
        @statistic[operationsDropped](title="Operations Dropped"; record=count,sum);
        @statistic[processingLatency](title="Processing Latency"; record=mean,max,histogram);
        @statistic[bufferUtilization](title="Buffer Utilization"; record=mean,max);
        
        @display("i=block/process");
        
    gates:
        input fabricIn;
        output fabricOut;
}