#include "UdpBridge.h"

namespace mesh {


void UdpBridge::start(){
    if(this->_is_running || !this->_enabled){ return; }

    this->inboundPackets = Vector<mesh::BridgePacket*>(this->inboundBuffer);

    if (this->_udp.listen(this->_server_port)) {

        this->_is_running = true;

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

        mesh::Packet* pkt = this->obtainNewPacket();
        pkt->readFrom(packet.data(), packet.length());

        Serial.printf("Size: %d max: %d\n", inboundUdpPackets.size(), inboundUdpPackets.max_size());

        if(!inboundUdpPackets.full()){

            inboundUdpPackets.push_back(pkt);
            Serial.println("queued udp packet");

        } else {
            this->releasePacket(pkt);
        }
        });

        Serial.print("UDP ready");
    }
}

void UdpBridge::loop(){

}

void UdpBridge::onMeshPacketRx(mesh::Packet* packet){

}

void UdpBridge::onMeshPacketTx(mesh::Packet* packet){

}

}