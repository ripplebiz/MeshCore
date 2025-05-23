#pragma once

#include <Bridge.h>
#include <AsyncUDP.h>

namespace mesh {


class UdpBridge : Bridge {

protected:
  AsyncUDP _udp;
  uint16_t _server_port;

public:
  UdpBridge(bool bridge_all, uint16_t port, );
  void start() override;
  void loop() override;
  void onMeshPacketRx(mesh::Packet* packet) override;
  void onMeshPacketTx(mesh::Packet* packet) override;
};


};