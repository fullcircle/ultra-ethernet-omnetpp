//
// SwitchPort.cc - Switch Port Implementation
//

#include <omnetpp.h>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

class SwitchPort : public cSimpleModule {
private:
    simtime_t processingLatency;
    
protected:
    virtual void initialize() override {
        processingLatency = par("processingLatency").doubleValue();
    }
    
    virtual void handleMessage(cMessage *msg) override {
        if (msg->getArrivalGate()->isName("fabricIn")) {
            // From fabric to ethernet
            sendDelayed(msg, processingLatency, "ethOut");
        } else if (msg->getArrivalGate()->isName("ethIn")) {
            // From ethernet to fabric
            sendDelayed(msg, processingLatency, "fabricOut");
        }
    }
};

Define_Module(SwitchPort);