#include "UdpBridge.h"
#include <WiFi.h>

namespace mesh {

UdpBridge::UdpBridge(UDPBridgePrefs* prefs){
    //_listening = false;
    _prefs = prefs;
    _waitingForNetwork = false;
}

void UdpBridge::start(){

    if(this->_prefs->flags.network_bridge){

        if(WiFi.status() == WL_CONNECTED){
            Serial.println("udp can start, connected");
            //if(setupListener()){
                _waitingForNetwork = false;
            /*} else {
                _waitingForNetwork = true;
            }*/

        } else {
            Serial.println("udp can't start, wait for network");
            _waitingForNetwork = true;
        }
    }
}

void UdpBridge::loop(){

    if(_waitingForNetwork){
        /*if(this->_prefs->flags.network_bridge){
            if(WiFi.status() == WL_CONNECTED){
                if(setupListener()){
                    _waitingForNetwork = false;
                } else {
                    _waitingForNetwork = true;
                }

            } else {
                _waitingForNetwork = true;
            }
        }*/
    }

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


bool UdpBridge::setupListener(){

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

        return true;
    } else {
        Serial.println("UDP failed to start network listener");

        return false;
    }
}

}


