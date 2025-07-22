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
            EV << "AIHPCApplication: Traffic timer fired at " << simTime() << " for node " << getParentModule()->getIndex() << endl;
            generateTraffic();
            // Schedule next traffic generation (reduced frequency to avoid congestion)
            scheduleAt(simTime() + 1.0, trafficTimer);
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
    double random = uniform(0, 1);
    EV << "AIHPCApplication: generateAITrainingWorkload - random=" << random << " intensity=" << communicationIntensity << endl;
    if (random < communicationIntensity) {
        EV << "AIHPCApplication: Initiating communication pattern " << commPattern << endl;
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
    } else {
        EV << "AIHPCApplication: Skipping communication due to intensity check" << endl;
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
    EV << "AIHPCApplication: sendMessage called - dest=" << dest << " size=" << size << " type=" << type << endl;
    UETPacket *pkt = new UETPacket(type);
    pkt->setByteLength(size);
    pkt->setDestAddr(dest);
    pkt->setSrcAddr(getParentModule()->isVector() ? getParentModule()->getIndex() : 0);
    pkt->setSequenceNum(sequenceNumber++);
    pkt->setTimestamp(simTime().raw());
    
    sentTimes[pkt->getSequenceNum()] = simTime();
    
    EV << "AIHPCApplication: Sending packet with seq=" << pkt->getSequenceNum() << " from=" << pkt->getSrcAddr() << " to=" << pkt->getDestAddr() << endl;
    send(pkt, "transportOut");
    emit(messagesSent, 1);
    
    // Also record scalar directly to ensure it's captured
    recordScalar("messagesActuallySent", sequenceNumber);
    EV << "AIHPCApplication: Message sent and statistics emitted" << endl;
}

void AIHPCApplication::processReceivedMessage(UETPacket* pkt) {
    EV << "AIHPCApplication: processReceivedMessage called for packet seq=" << pkt->getSequenceNum() << " at node " << getParentModule()->getIndex() << endl;
    emit(messagesReceived, 1);
    
    // Record scalar directly to ensure it's captured
    static int receivedCount = 0;
    receivedCount++;
    recordScalar("messagesActuallyReceived", receivedCount);
    
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
    int myIndex = getParentModule()->isVector() ? getParentModule()->getIndex() : 0;
    EV << "AIHPCApplication: initiateAllReduce - myIndex=" << myIndex << " jobSize=" << jobSize << endl;
    for (int i = 0; i < jobSize; i++) {
        if (i != myIndex) {
            EV << "AIHPCApplication: Sending message to destination " << i << endl;
            sendMessage(i, messageSize, "ALLREDUCE");
        } else {
            EV << "AIHPCApplication: Skipping self (index " << i << ")" << endl;
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
    EV << "AIHPCApplication: finish() called for node " << getParentModule()->getIndex() << endl;
    EV << "AIHPCApplication: sequenceNumber=" << sequenceNumber << " sentTimes.size()=" << sentTimes.size() << endl;
    
    // Force record statistics if we know messages were sent
    if (sequenceNumber > 0) {
        recordScalar("finalMessagesSent", sequenceNumber);
        recordScalar("finalSentTimesRemaining", (double)sentTimes.size());
    }
}