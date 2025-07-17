//
// AIHPCApplication.h - AI/HPC Application Layer
//

#ifndef __AI_HPC_APPLICATION_H  
#define __AI_HPC_APPLICATION_H

#include <omnetpp.h>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

class AIHPCApplication : public cSimpleModule {
public:
    enum WorkloadType {
        AI_TRAINING = 0,
        AI_INFERENCE = 1,
        HPC_SIMULATION = 2,
        DISTRIBUTED_DATABASE = 3
    };
    
    enum CommunicationPattern {
        ALLREDUCE = 0,
        ALLGATHER = 1,
        BROADCAST = 2,
        POINT_TO_POINT = 3,
        PARAMETER_SERVER = 4
    };

private:
    // Configuration
    WorkloadType workloadType;
    CommunicationPattern communicationPattern;
    int jobSize;                    // Number of participating nodes
    double messageSize;             // Average message size
    double communicationIntensity;  // Communication frequency
    
    // Job management
    struct Job {
        uint64_t jobId;
        simtime_t startTime;
        simtime_t deadline;
        std::vector<int> participatingNodes;
        int completedOperations;
        int totalOperations;
        bool isCompleted;
    };
    
    std::map<uint64_t, Job> activeJobs;
    uint64_t nextJobId;
    
    // Performance measurement
    simsignal_t jobCompletionTime;
    simsignal_t throughput;
    simsignal_t communicationOverhead;
    
    // Timers
    cMessage *jobGenerationTimer;
    cMessage *communicationTimer;
    
public:
    AIHPCApplication();
    virtual ~AIHPCApplication();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    
    // Workload generation
    void generateNewJob();
    void scheduleNextJob();
    
    // Communication patterns
    void executeAllReduce(uint64_t jobId);
    void executeAllGather(uint64_t jobId);
    void executeBroadcast(uint64_t jobId);
    void executePointToPoint(uint64_t jobId);
    void executeParameterServer(uint64_t jobId);
    
    // AI-specific workloads
    void generateAITrainingWorkload();
    void generateAIInferenceWorkload();
    
    // HPC-specific workloads
    void generateHPCSimulationWorkload();
    
    // Performance measurement
    void recordJobCompletion(uint64_t jobId);
    void updateThroughputMetrics();
};

#endif