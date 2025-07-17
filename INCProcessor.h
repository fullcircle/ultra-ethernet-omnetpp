//
// INCProcessor.h - In-Network Computing Processor
//

#ifndef __INC_PROCESSOR_H
#define __INC_PROCESSOR_H

#include <omnetpp.h>
#include <deque>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

enum CollectiveType {
    ALLREDUCE = 0,
    ALLGATHER = 1,
    BROADCAST = 2,
    REDUCE_SCATTER = 3
};

enum ReductionOperation {
    SUM = 0,
    MAX = 1,
    MIN = 2,
    PROD = 3
};

struct INCOperation {
    INCPacket* packet;
    simtime_t startTime;
    CollectiveType collectiveType;
    int participantCount;
    ReductionOperation reductionOp;
};

class INCProcessor : public cSimpleModule {
private:
    // Configuration parameters
    bool enabled;
    simtime_t processingLatency;
    int maxConcurrentOperations;
    int bufferSize;
    
    // Statistics
    simsignal_t operationsProcessed;
    simsignal_t operationsDropped;
    simsignal_t processingLatencySignal;
    simsignal_t bufferUtilization;
    
    // Internal state
    cMessage *processingTimer;
    std::deque<INCOperation> operationQueue;
    int currentBufferSize;
    int activeOperations;
    
    // Processing functions
    void processIncomingPacket(UETPacket *pkt);
    bool canProcessOperation(INCPacket *pkt);
    void scheduleOperation(INCPacket *pkt);
    void processNextOperation();
    
    // Collective operation processing
    INCPacket* processCollectiveOperation(const INCOperation& op);
    INCPacket* processAllReduce(const INCOperation& op, INCPacket* result);
    INCPacket* processAllGather(const INCOperation& op, INCPacket* result);
    INCPacket* processBroadcast(const INCOperation& op, INCPacket* result);
    INCPacket* processReduceScatter(const INCOperation& op, INCPacket* result);
    
public:
    INCProcessor();
    virtual ~INCProcessor();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif