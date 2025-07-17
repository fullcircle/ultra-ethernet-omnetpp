//
// AIHPCApplication.h - AI/HPC Application Implementation
//

#ifndef __AIHPC_APPLICATION_H
#define __AIHPC_APPLICATION_H

#include <omnetpp.h>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

enum WorkloadType {
    AI_TRAINING,
    AI_INFERENCE,
    HPC_SIMULATION,
    DISTRIBUTED_DATABASE
};

enum CommunicationPattern {
    ALLREDUCE,
    ALLGATHER,
    BROADCAST,
    POINT_TO_POINT,
    PARAMETER_SERVER
};

class AIHPCApplication : public cSimpleModule {
private:
    // Configuration parameters
    WorkloadType workloadType;
    CommunicationPattern commPattern;
    int messageSize;
    int jobSize;
    double communicationIntensity;
    simtime_t trafficStartTime;
    double trafficRate;
    
    // Statistics
    simsignal_t messagesSent;
    simsignal_t messagesReceived;
    simsignal_t throughput;
    simsignal_t latency;
    
    // Internal state
    cMessage *trafficTimer;
    int sequenceNumber;
    std::map<int, simtime_t> sentTimes;
    
    // Workload generation
    void generateTraffic();
    void generateAITrainingWorkload();
    void generateAIInferenceWorkload();
    void generateHPCSimulationWorkload();
    
    // Message handling
    void sendMessage(int dest, int size, const char* type);
    void processReceivedMessage(UETPacket* pkt);
    
    // Collective operations
    void initiateAllReduce();
    void initiateAllGather();
    void initiateBroadcast();
    
public:
    AIHPCApplication();
    virtual ~AIHPCApplication();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif