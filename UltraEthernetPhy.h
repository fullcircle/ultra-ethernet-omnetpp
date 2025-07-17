//
// UltraEthernetPhy.h - Physical Layer Implementation
//

#ifndef __ULTRAETHERNET_PHY_H
#define __ULTRAETHERNET_PHY_H

#include <omnetpp.h>
#include <queue>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

class UltraEthernetPhy : public cSimpleModule {
private:
    // Configuration parameters
    double linkSpeed;           // 800G/1600G
    double fecOverhead;         // FEC coding overhead  
    double errorRate;           // Base bit error rate
    int fecCorrectionBits;      // FEC correction capability
    bool fecEnabled;
    
    // Statistics
    simsignal_t fecCorrections;
    simsignal_t uncorrectableErrors;
    simsignal_t linkUtilization;
    
    // Performance optimization
    cMessage *transmissionTimer;
    std::queue<cPacket*> transmissionQueue;
    
public:
    UltraEthernetPhy();
    virtual ~UltraEthernetPhy();
    
protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    
    // Core functionality
    void processTransmission(cPacket *pkt);
    bool simulateChannelErrors(cPacket *pkt);
    void applyFEC(cPacket *pkt);
    void scheduleNextTransmission();
    
    // Performance measurement
    void updateLinkUtilization();
    void recordErrorStatistics(bool corrected);
};

#endif