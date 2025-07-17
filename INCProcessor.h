//
// INCProcessor.h - In-Network Computing Processor
//

#ifndef __INC_PROCESSOR_H
#define __INC_PROCESSOR_H

#include <omnetpp.h>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

class INCProcessor : public cSimpleModule {
public:
    enum CollectiveType {
        ALLREDUCE = 0,
        BROADCAST = 1,
        REDUCE = 2,
        ALLGATHER = 3,
        SCATTER = 4
    };
    
    enum ReductionOp {
        SUM = 0,
        MAX = 1,
        MIN = 2,
        PROD = 3,
        AND = 4,
        OR = 5
    };

private:
    bool enabled;
    double processingLatency;
    int maxConcurrentOperations;
    
    // Active collective operations
    struct CollectiveOperation {
        uint32_t operationId;
        CollectiveType type;
        ReductionOp reductionOp;
        uint32_t expectedParticipants;
        uint32_t receivedPackets;
        std::vector<double> accumulatedData;
        simtime_t startTime;
    };
    
    std::map<uint32_t, CollectiveOperation> activeOperations;
    
    // Performance metrics
    simsignal_t incLatency;
    simsignal_t incThroughput;
    simsignal_t operationsProcessed;
    
public:
    INCProcessor();
    virtual ~INCProcessor();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    
    // Core INC functionality
    void processCollectivePacket(INCPacket *pkt);
    void performReduction(CollectiveOperation &op, const std::vector<double> &data);
    void generateOutputPackets(CollectiveOperation &op);
    
    // Reduction operations
    double performReductionOp(ReductionOp op, const std::vector<double> &values);
    
    // Performance optimization
    bool canProcessOperation(CollectiveType type);
    void scheduleOperationCompletion(uint32_t operationId);
};

#endif