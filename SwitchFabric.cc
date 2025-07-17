//
// SwitchFabric.cc - Switch Fabric Implementation
//

#include <omnetpp.h>
#include "UltraEthernetMsg_m.h"

using namespace omnetpp;

class SwitchFabric : public cSimpleModule {
private:
    int numPorts;
    simtime_t switchingLatency;
    double bandwidth;
    
protected:
    virtual void initialize() override {
        numPorts = par("numPorts").intValue();
        switchingLatency = par("switchingLatency").doubleValue();
        bandwidth = par("bandwidth").doubleValue();
    }
    
    virtual void handleMessage(cMessage *msg) override {
        UETPacket *pkt = check_and_cast<UETPacket*>(msg);
        
        // Simple switching: forward to appropriate port
        int arrivalPort = msg->getArrivalGate()->getIndex();
        int destPort = pkt->getDestAddr() % numPorts;
        
        // Check if this is an INC packet that should go to INC processor
        INCPacket *incPkt = dynamic_cast<INCPacket*>(pkt);
        if (incPkt) {
            sendDelayed(pkt, switchingLatency, "incOut");
        } else {
            sendDelayed(pkt, switchingLatency, "portOut", destPort);
        }
    }
};

Define_Module(SwitchFabric);