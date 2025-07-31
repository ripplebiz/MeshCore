#include "UdpBridge.h"

namespace mesh {

UdpBridge::UdpBridge(UDPBridgePrefs* prefs){
    //_listening = false;
    _prefs = prefs;
}

void UdpBridge::start(){

    if(this->_prefs->flags.network_bridge){

        this->_inboundPackets = Vector<mesh::Packet*>(this->_inboundBuffer);

        Serial.println("UDP starting network listner");        
        this->_udp.listen( this->_prefs->port );

        if(this->_udp.connected()){
            //setup udp handler
            Serial.println("UDP started network listener");

            this->_udp.onPacket([this](AsyncUDPPacket packet) {
                Serial.print("UDP Packet Type: ");
                Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
                Serial.print(", From: ");
                Serial.print(packet.remoteIP());
                Serial.print(":");
                Serial.print(packet.remotePort());
                Serial.print(", To: ");
                Serial.print(packet.localIP());
                Serial.print(":");
                Serial.print(packet.localPort());
                Serial.print(", Length: ");
                Serial.print(packet.length());
                Serial.println();

                mesh::Packet* pkt = this->_dispatcher->obtainNewPacket();
                pkt->readFrom(packet.data(), packet.length());

                Serial.printf("Size: %d max: %d\n", _inboundPackets.size(), _inboundPackets.max_size());

                if(!_inboundPackets.full()){

                    _inboundPackets.push_back(pkt);
                    Serial.println("queued udp packet");

                } else {
                    this->_dispatcher->releasePacket(pkt);
                }
            });
        } else {
            Serial.println("UDP failed to start network listener");
        }
    }
}

void UdpBridge::loop(){

}

void UdpBridge::stop(){
    if(this->_udp.connected()){
        this->_udp.close();
    }
}

void UdpBridge::onMeshPacketRx(mesh::Packet* packet){
    Serial.println("onMeshPacketRx");
}

void UdpBridge::onMeshPacketTx(mesh::Packet* packet){
    Serial.println("onMeshPacketTx");
}

}