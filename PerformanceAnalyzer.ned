//
// PerformanceAnalyzer.ned - Performance Analysis Module
//

simple PerformanceAnalyzer {
    parameters:
        double measurementInterval @unit(s) = default(1ms);
        bool enableDetailedStats = default(true);
        
        @display("i=block/cogwheel");
}