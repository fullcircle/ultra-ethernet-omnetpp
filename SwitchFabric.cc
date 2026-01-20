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
        cPacket *pkt = check_and_cast<cPacket*>(msg);
        
        // Handle different packet types
        UETPacket *uetPkt = dynamic_cast<UETPacket*>(pkt);
        LLRAck *llrAck = dynamic_cast<LLRAck*>(pkt);
        
        if (llrAck) {
            // LLRAck packets should be forwarded to the original sender
            // Route based on destAddr field (the original packet's sender)
            int destPort = llrAck->getDestAddr() % numPorts;
            sendDelayed(pkt, switchingLatency, "portOut", destPort);
        } else if (uetPkt) {
            // Regular UET packets
            int arrivalPort = msg->getArrivalGate()->getIndex();
            int destPort = uetPkt->getDestAddr() % numPorts;
            
            // Check if this is an INC packet that should go to INC processor
            INCPacket *incPkt = dynamic_cast<INCPacket*>(uetPkt);
            if (incPkt) {
                sendDelayed(pkt, switchingLatency, "incOut");
            } else {
                sendDelayed(pkt, switchingLatency, "portOut", destPort);
            }
        } else {
            // Unknown packet type, drop it
            EV_WARN << "SwitchFabric: Unknown packet type, dropping" << endl;
            delete pkt;
        }
    }
};

Define_Module(SwitchFabric);