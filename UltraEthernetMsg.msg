//
// UltraEthernetMsg.msg - Message definitions for UET protocol
//

packet UETPacket {
    // Common UET header fields
    uint32_t flowId;
    uint32_t sequenceNum;
    uint8_t sublayerType;  // 0=Security, 1=PDS, 2=MSS, 3=CMS
    uint16_t pathId;       // For packet spraying
    uint64_t timestamp;    // High-precision timestamp
    
    // Transport layer fields
    uint8_t transportType; // DATA=0, ACK=1, NACK=2
    uint32_t destAddr;
    uint32_t srcAddr;
    uint16_t sprayPath;
    
    // Security sublayer fields
    bool encrypted = false;
    uint64_t jobId;
    uint32_t securitySequence;
    
    // Packet Delivery Sublayer fields
    bool reliableDelivery = true;
    bool ackRequired = false;
    uint32_t ackSequence;
    
    // Message Semantics fields
    uint8_t operationType;  // SEND=0, WRITE=1, READ=2, ATOMIC=3
    uint64_t remoteAddress;
    uint64_t localAddress;
    uint32_t operationTag;
    bool deferrable = false;  // AI Full profile
    
    // Congestion Management fields
    double congestionWindow;
    uint16_t pathVector[];   // Available paths
}

packet LLRAck {
    uint32_t acknowledgedSeq;
    uint8_t ackType;  // ACK=0, NACK=1
    uint16_t pathId;
}

packet INCPacket extends UETPacket {
    uint8_t collectiveType;  // ALLREDUCE=0, BROADCAST=1, etc.
    uint32_t participantCount;
    uint32_t reductionOp;    // SUM=0, MAX=1, MIN=2
    bool isIntermediate = false;
}