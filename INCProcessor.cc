//
// INCProcessor.cc - In-Network Computing Processor Implementation
//

#include "INCProcessor.h"

Define_Module(INCProcessor);

INCProcessor::INCProcessor() {
    processingTimer = nullptr;
    currentBufferSize = 0;
    activeOperations = 0;
}

INCProcessor::~INCProcessor() {
    cancelAndDelete(processingTimer);
    for (auto& op : operationQueue) {
        delete op.packet;
    }
}

void INCProcessor::initialize() {
    // Read configuration parameters
    enabled = par("enabled").boolValue();
    processingLatency = par("processingLatency").doubleValue();
    maxConcurrentOperations = par("maxConcurrentOperations").intValue();
    bufferSize = par("bufferSize").intValue();
    
    // Initialize statistics
    operationsProcessed = registerSignal("operationsProcessed");
    operationsDropped = registerSignal("operationsDropped");
    processingLatencySignal = registerSignal("processingLatency");
    bufferUtilization = registerSignal("bufferUtilization");
    
    // Initialize processing timer
    processingTimer = new cMessage("processingTimer");
}

void INCProcessor::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == processingTimer) {
            processNextOperation();
        }
    } else {
        // Incoming packet from switch fabric
        UETPacket *pkt = check_and_cast<UETPacket*>(msg);
        
        if (enabled) {
            processIncomingPacket(pkt);
        } else {
            // INC disabled, forward packet directly
            send(pkt, "fabricOut");
        }
    }
}

void INCProcessor::processIncomingPacket(UETPacket *pkt) {
    // Check if this is an INC packet
    INCPacket *incPkt = dynamic_cast<INCPacket*>(pkt);
    
    if (incPkt) {
        // This is an INC operation
        if (canProcessOperation(incPkt)) {
            scheduleOperation(incPkt);
        } else {
            // Drop packet if buffer is full or too many operations
            emit(operationsDropped, 1);
            delete incPkt;
        }
    } else {
        // Regular packet, forward directly
        send(pkt, "fabricOut");
    }
}

bool INCProcessor::canProcessOperation(INCPacket *pkt) {
    // Check if we can accept this operation
    if (activeOperations >= maxConcurrentOperations) {
        return false;
    }
    
    if (currentBufferSize + pkt->getByteLength() > bufferSize) {
        return false;
    }
    
    return true;
}

void INCProcessor::scheduleOperation(INCPacket *pkt) {
    // Create operation entry
    INCOperation op;
    op.packet = pkt;
    op.startTime = simTime();
    op.collectiveType = (CollectiveType)pkt->getCollectiveType();
    op.participantCount = pkt->getParticipantCount();
    op.reductionOp = (ReductionOperation)pkt->getReductionOp();
    
    // Add to operation queue
    operationQueue.push_back(op);
    currentBufferSize += pkt->getByteLength();
    
    // Update statistics
    emit(bufferUtilization, (double)currentBufferSize / bufferSize);
    
    // Schedule processing if not already running
    if (!processingTimer->isScheduled() && activeOperations < maxConcurrentOperations) {
        scheduleAt(simTime() + processingLatency, processingTimer);
    }
}

void INCProcessor::processNextOperation() {
    if (operationQueue.empty()) {
        return;
    }
    
    // Get next operation
    INCOperation op = operationQueue.front();
    operationQueue.pop_front();
    
    activeOperations++;
    currentBufferSize -= op.packet->getByteLength();
    
    // Process the operation based on type
    INCPacket *result = processCollectiveOperation(op);
    
    if (result) {
        // Send result back to fabric
        send(result, "fabricOut");
        
        // Record statistics
        simtime_t latency = simTime() - op.startTime;
        emit(processingLatencySignal, latency);
        emit(operationsProcessed, 1);
    } else {
        // Operation failed
        emit(operationsDropped, 1);
    }
    
    activeOperations--;
    
    // Update buffer utilization
    emit(bufferUtilization, (double)currentBufferSize / bufferSize);
    
    // Schedule next operation if available
    if (!operationQueue.empty() && activeOperations < maxConcurrentOperations) {
        scheduleAt(simTime() + processingLatency, processingTimer);
    }
    
    // Clean up
    delete op.packet;
}

INCPacket* INCProcessor::processCollectiveOperation(const INCOperation& op) {
    // Create result packet
    INCPacket *result = new INCPacket("INCResult");
    result->setCollectiveType(op.collectiveType);
    result->setParticipantCount(op.participantCount);
    result->setReductionOp(op.reductionOp);
    result->setIsIntermediate(true);
    
    // Copy relevant fields from original packet
    result->setFlowId(op.packet->getFlowId());
    result->setDestAddr(op.packet->getSrcAddr());
    result->setSrcAddr(op.packet->getDestAddr());
    result->setByteLength(op.packet->getByteLength());
    
    // Process based on collective type
    switch (op.collectiveType) {
        case ALLREDUCE:
            result = processAllReduce(op, result);
            break;
        case ALLGATHER:
            result = processAllGather(op, result);
            break;
        case BROADCAST:
            result = processBroadcast(op, result);
            break;
        case REDUCE_SCATTER:
            result = processReduceScatter(op, result);
            break;
        default:
            delete result;
            return nullptr;
    }
    
    return result;
}

INCPacket* INCProcessor::processAllReduce(const INCOperation& op, INCPacket* result) {
    // Simplified AllReduce processing
    // In reality, this would perform actual reduction operations
    
    // Simulate reduction computation delay
    // The result size is typically the same as input for AllReduce
    return result;
}

INCPacket* INCProcessor::processAllGather(const INCOperation& op, INCPacket* result) {
    // Simplified AllGather processing
    // Result size is typically larger (sum of all participants)
    int gatheredSize = op.packet->getByteLength() * op.participantCount;
    result->setByteLength(gatheredSize);
    
    return result;
}

INCPacket* INCProcessor::processBroadcast(const INCOperation& op, INCPacket* result) {
    // Simplified Broadcast processing
    // Result size is same as input
    return result;
}

INCPacket* INCProcessor::processReduceScatter(const INCOperation& op, INCPacket* result) {
    // Simplified ReduceScatter processing
    // Result size is typically smaller (divided among participants)
    int scatteredSize = op.packet->getByteLength() / op.participantCount;
    result->setByteLength(scatteredSize);
    
    return result;
}

void INCProcessor::finish() {
    // Record final statistics
}