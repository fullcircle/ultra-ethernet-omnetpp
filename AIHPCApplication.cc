//
// AIHPCApplication.cc - AI/HPC Application Implementation
//

#include "AIHPCApplication.h"

Define_Module(AIHPCApplication);

AIHPCApplication::AIHPCApplication() {
    trafficTimer = nullptr;
    sequenceNumber = 0;
}

AIHPCApplication::~AIHPCApplication() {
    cancelAndDelete(trafficTimer);
}

void AIHPCApplication::initialize() {
    // Read configuration parameters
    std::string workloadStr = par("workloadType").stringValue();
    std::string patternStr = par("communicationPattern").stringValue();
    
    if (workloadStr == "AI_TRAINING") workloadType = AI_TRAINING;
    else if (workloadStr == "AI_INFERENCE") workloadType = AI_INFERENCE;
    else if (workloadStr == "HPC_SIMULATION") workloadType = HPC_SIMULATION;
    else workloadType = AI_TRAINING;
    
    if (patternStr == "ALLREDUCE") commPattern = ALLREDUCE;
    else if (patternStr == "ALLGATHER") commPattern = ALLGATHER;
    else if (patternStr == "BROADCAST") commPattern = BROADCAST;
    else commPattern = ALLREDUCE;
    
    messageSize = par("messageSize").intValue();
    jobSize = par("jobSize").intValue();
    communicationIntensity = par("communicationIntensity").doubleValue();
    trafficStartTime = par("trafficStartTime").doubleValue();
    trafficRate = par("trafficRate").doubleValue();
    
    // Initialize statistics
    messagesSent = registerSignal("messagesSent");
    messagesReceived = registerSignal("messagesReceived");
    throughput = registerSignal("throughput");
    latency = registerSignal("latency");
    
    // Initialize traffic timer
    trafficTimer = new cMessage("trafficTimer");
    scheduleAt(trafficStartTime, trafficTimer);
}

void AIHPCApplication::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == trafficTimer) {
            generateTraffic();
            // Schedule next traffic generation
            scheduleAt(simTime() + 0.1, trafficTimer);
        }
    } else {
        UETPacket *pkt = check_and_cast<UETPacket*>(msg);
        processReceivedMessage(pkt);
        delete pkt;
    }
}

void AIHPCApplication::generateTraffic() {
    switch (workloadType) {
        case AI_TRAINING:
            generateAITrainingWorkload();
            break;
        case AI_INFERENCE:
            generateAIInferenceWorkload();
            break;
        case HPC_SIMULATION:
            generateHPCSimulationWorkload();
            break;
        default:
            generateAITrainingWorkload();
    }
}

void AIHPCApplication::generateAITrainingWorkload() {
    // AI Training: periodic AllReduce operations
    if (uniform(0, 1) < communicationIntensity) {
        switch (commPattern) {
            case ALLREDUCE:
                initiateAllReduce();
                break;
            case ALLGATHER:
                initiateAllGather();
                break;
            case BROADCAST:
                initiateBroadcast();
                break;
            default:
                initiateAllReduce();
        }
    }
}

void AIHPCApplication::generateAIInferenceWorkload() {
    // AI Inference: request-response pattern
    if (uniform(0, 1) < communicationIntensity) {
        int dest = intuniform(0, jobSize - 1);
        sendMessage(dest, messageSize, "INFERENCE_REQUEST");
    }
}

void AIHPCApplication::generateHPCSimulationWorkload() {
    // HPC: mixed collective and point-to-point operations
    if (uniform(0, 1) < communicationIntensity) {
        if (uniform(0, 1) < 0.3) {
            // 30% collective operations
            initiateAllReduce();
        } else {
            // 70% point-to-point
            int dest = intuniform(0, jobSize - 1);
            sendMessage(dest, messageSize, "HPC_DATA");
        }
    }
}

void AIHPCApplication::sendMessage(int dest, int size, const char* type) {
    UETPacket *pkt = new UETPacket(type);
    pkt->setByteLength(size);
    pkt->setDestAddr(dest);
    pkt->setSrcAddr(getParentModule()->isVector() ? getParentModule()->getIndex() : 0);
    pkt->setSequenceNum(sequenceNumber++);
    pkt->setTimestamp(simTime().raw());
    
    sentTimes[pkt->getSequenceNum()] = simTime();
    
    send(pkt, "transportOut");
    emit(messagesSent, 1);
}

void AIHPCApplication::processReceivedMessage(UETPacket* pkt) {
    emit(messagesReceived, 1);
    
    // Calculate latency if we sent this message
    auto it = sentTimes.find(pkt->getSequenceNum());
    if (it != sentTimes.end()) {
        simtime_t latencyTime = simTime() - it->second;
        emit(latency, latencyTime);
        sentTimes.erase(it);
    }
    
    // Calculate throughput
    double currentThroughput = (double)pkt->getByteLength() * 8.0 / SIMTIME_DBL(simTime());
    emit(throughput, currentThroughput);
}

void AIHPCApplication::initiateAllReduce() {
    // Simplified AllReduce: send to all peers
    for (int i = 0; i < jobSize; i++) {
        if (i != (getParentModule()->isVector() ? getParentModule()->getIndex() : 0)) {
            sendMessage(i, messageSize, "ALLREDUCE");
        }
    }
}

void AIHPCApplication::initiateAllGather() {
    // Simplified AllGather: send to all peers
    for (int i = 0; i < jobSize; i++) {
        if (i != (getParentModule()->isVector() ? getParentModule()->getIndex() : 0)) {
            sendMessage(i, messageSize, "ALLGATHER");
        }
    }
}

void AIHPCApplication::initiateBroadcast() {
    // Simplified Broadcast: send to all peers
    for (int i = 0; i < jobSize; i++) {
        if (i != (getParentModule()->isVector() ? getParentModule()->getIndex() : 0)) {
            sendMessage(i, messageSize, "BROADCAST");
        }
    }
}

void AIHPCApplication::finish() {
    // Record final statistics
}