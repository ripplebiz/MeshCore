#pragma once

#include <Bridge.h>
#include <AsyncUDP.h>

#include "UdpBridgeDetails.h"

namespace mesh {


class UdpBridge : Bridge {

protected:
  AsyncUDP _udp;
  uint16_t _server_port;
  UDPBridgeFlags _flags;

public:
  UdpBridge(UDPBridgeFlags flags, uint16_t port=8000, uint8_t* udp_bridge_server_address=NULL);
  void start() override;
  void loop() override;
  void onMeshPacketRx(mesh::Packet* packet) override;
  void onMeshPacketTx(mesh::Packet* packet) override;
};


};