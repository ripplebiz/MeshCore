#pragma once
#include <Packet.h>

#include <Vector.h>


#define BRIDGE_PACKET_BUFFER_SIZE 10

namespace mesh {

class BridgePacket{
  uint8_t version;
  float frequency;
  float rssi;
  float snr;
  uint32_t timestamp;
  uint32_t node[32];
  mesh::Packet* packet;
  uint8_t signature[32];
};

class Bridge {

protected:
  //bool _enabled;
  //bool _bridge_all;
  //bool _is_running;
  
  mesh::Packet* _inboundBuffer[BRIDGE_PACKET_BUFFER_SIZE];
  Vector<mesh::Packet*> _inboundPackets;

  public:
  //Bridge(bool bridge_all=false);

  virtual void start() = 0;
  virtual void loop() = 0;
  virtual void onMeshPacketRx(mesh::Packet* packet) = 0;
  virtual void onMeshPacketTx(mesh::Packet* packet) = 0;
  //mesh::BridgePacket* createBridgePacket(mesh::Packet);
  //void sendToMesh(mesh::BridgePacket* bridgePacket);
  
};


};
