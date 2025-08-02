#include "UdpBridge.h"
#include <WiFi.h>

namespace mesh {

UdpBridge::UdpBridge(NodePrefs* prefs, mesh::LocalIdentity* identity, mesh::Dispatcher* dispatcher, mesh::RTCClock* clock){
    //_listening = false;
    _nodePrefs = prefs;
    _prefs = &prefs->udpBridge;
    _identity = identity;
    _dispatcher = dispatcher;
    _clock = clock;
    _waitingForNetwork = false;
}

void UdpBridge::start(){

    if(this->_prefs->flags.network_bridge){

        if(WiFi.status() == WL_CONNECTED){
            Serial.println("udp can start, connected");
            if(setupListener()){
                _waitingForNetwork = false;
            } else {
                Serial.println("udp failed to start");
                _waitingForNetwork = true;
            }

        } else {
            Serial.println("udp can't start, wait for network");
            _waitingForNetwork = true;
        }
    }

    /*if(this->_prefs->flags.tx_bridge || this->_prefs->flags.rx_bridge){
        this->_outboundPackets = Vector<mesh::BridgePacket>(this->_outboundBuffer);
    }*/
}

void UdpBridge::loop(){

    if(_waitingForNetwork){
        if(this->_prefs->flags.network_bridge){
            if(WiFi.status() == WL_CONNECTED){
                Serial.println("udp can start now");
                _waitingForNetwork = false;
                if(setupListener()){
                    Serial.println("udp listener success");
                    _waitingForNetwork = false;
                } else {
                    _waitingForNetwork = true;
                    Serial.println("udp failed to start");
                }

            } else {
                //Serial.println("Still waiting");
                //_waitingForNetwork = true;
            }
        }
    } else if(!_inboundPackets.empty()) {
        // Network is connected

        while(!_inboundPackets.empty()) {
            mesh::BridgePacket bpacket = _inboundPackets.back();

            _dispatcher->processBridgePacket( bpacket.packet );
            _dispatcher->releasePacket( bpacket.packet );
            _inboundPackets.pop_back();

        }
    }

}

void UdpBridge::stop(){
    if(this->_udp.connected()){
        this->_udp.close();
    }
}

void UdpBridge::onMeshPacketRx(mesh::Packet* packet){

    if(!_waitingForNetwork && this->_prefs->flags.rx_bridge){
        Serial.println("onMeshPacketRx - bridging to udp");
        bridgeMeshPacket(packet, PACKET_SOURCE_UDP_BRIDGE);
    }
}

void UdpBridge::onMeshPacketTx(mesh::Packet* packet){
    
    if(!_waitingForNetwork && this->_prefs->flags.tx_bridge){
        Serial.println("onMeshPacketTx - bridging to udp");

        bridgeMeshPacket(packet, PACKET_SOURCE_UDP_BRIDGE);
    }
}


bool UdpBridge::setupListener(){

    this->_inboundPackets = Vector<mesh::BridgePacket>(this->_inboundBuffer);

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

            bufferRawBridgePacket( packet.data(), packet.length(), PACKET_SOURCE_UDP_BRIDGE);

            Serial.printf("UDP queue Size: %d max: %d\n", _inboundPackets.size(), _inboundPackets.max_size());
        });

        return true;
    } else {
        Serial.println("UDP failed to start network listener");

        return false;
    }
}



void UdpBridge::bufferRawBridgePacket(const uint8_t* data, const uint8_t length, uint8_t source){

    if(_inboundPackets.full()){ Serial.println("\tDROPPING - inbound udp queue full"); return; }
    

    mesh::Packet* pkt = this->_dispatcher->obtainNewPacket();

    if(pkt){

        pkt->_source = source;
    
        mesh::Identity bsender(&data[BP_NODE_PUB_OFFSET]);
        bool verified = bsender.verify( &data[length-32], data, length-32 );

        if(!verified){
            _dispatcher->releasePacket(pkt);
            Serial.println("\tDROPPING - DANGER inbound udp packet is forged or corrupt");
            return;
        }
        

        mesh::BridgePacket bpacket;
    
        uint8_t idx = 0;
        bpacket.version = data[idx++];
        bpacket.frequency = *((float*) &data[idx+=sizeof(float)]);
        bpacket.sf = data[idx++];
        bpacket.bw = *((float*) &data[idx+=sizeof(float)]);
        bpacket.rssi = *((float*) &data[idx+=sizeof(float)]);
        bpacket.snr = *((float*) &data[idx+=sizeof(float)]);
        bpacket.timestamp = *((uint32_t*) &data[idx+=sizeof(uint32_t)]);
        
        memcpy( bpacket.node, bsender.pub_key, sizeof(bpacket.node) );
        idx+=32;

        bpacket.packetLength = data[idx++];
    
        pkt->readFrom( &data[idx+=bpacket.packetLength], bpacket.packetLength);
        bpacket.packet = pkt;
    
        memcpy( bpacket.signature, &data[length-32], sizeof(bpacket.signature) );

        
        if(!_inboundPackets.full()){
            _inboundPackets.push_back(bpacket);
            Serial.println("ACCEPTING inbound udp packet");
        } else {
            Serial.println("\tDROPPING - inbound udp queue full");
            _dispatcher->releasePacket(pkt);
        }
    } else {
        Serial.println("\tDROPPING inbound udp packet. mesh packet queue full");
    }

}



void UdpBridge::bridgeMeshPacket(mesh::Packet* packet, uint8_t source){

    if(packet->_source == source){
        Serial.println(" dropping from this bridge");
        return;
    }

    //if(!_outboundPackets.full()){ return; }

    mesh::BridgePacket bpacket;


    Serial.println(" mesh packet sent to udp network");
    uint8_t pktBuffer[BP_PACKET_MAX_SIZE];

    
    size_t idx = 0;
    pktBuffer[idx++] = 0x0;                                 // version

    Serial.printf("2   idx = %i\n", idx);

    *((float*) (&pktBuffer[idx+=sizeof(float)])) = _nodePrefs->freq;    // freq
    
    Serial.printf("3   idx = %i\n", idx);

    pktBuffer[idx+=sizeof(uint8_t)] = _nodePrefs->sf;
    
    Serial.printf("4   idx = %i\n", idx);

    *((float*) (&pktBuffer[idx+=sizeof(float)])) = _nodePrefs->bw;
    
    Serial.printf("5   idx = %i\n", idx);

    *((float*) (&pktBuffer[idx+=sizeof(float)])) = 0.0f;    //rssi
    
    Serial.printf("6   idx = %i\n", idx);

    *((float*) (&pktBuffer[idx+=sizeof(float)])) = packet->getSNR();
    
    Serial.printf("7   idx = %i\n", idx);

    *((uint32_t*) (&pktBuffer[idx+=sizeof(uint32_t)])) = _clock->getCurrentTime();
    
    Serial.printf("8   idx = %i\n", idx);


    memcpy( &pktBuffer[idx], _identity->pub_key, 32 );
    idx += 32;
    
    Serial.printf("9   idx = %i\n", idx);


    uint8_t packetLen = packet->getRawLength();
    pktBuffer[idx+=sizeof(uint8_t)] = packetLen;
    
    Serial.printf("10   idx = %i\n", idx);

    uint8_t written = packet->writeTo( &pktBuffer[idx+=packetLen] );

    Serial.printf("   wrote = %i vs expected = %i\n", written, packetLen);
    
    Serial.printf("11   idx = %i\n", idx);


    _identity->sign( &pktBuffer[idx], pktBuffer, idx );
    idx+=32;
    
    Serial.printf("12   idx = %i\n", idx);

    Serial.printf("   buffer.maxlen = %i\n", sizeof(pktBuffer));

    
    if(_prefs->flags.mode == UDP_BRIDGE_MODE_BROADCAST){
        _udp.broadcastTo( pktBuffer, idx, _prefs->port);
    } else {
        //_udp.sendTo()
    }
}

}