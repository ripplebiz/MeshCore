#pragma once

#include <Bridge.h>
#include <Dispatcher.h>
#include <AsyncUDP.h>

#include "UdpBridgeDetails.h"

namespace mesh {


class UdpBridge : public mesh::Bridge {

protected:
  bool _listening;
  AsyncUDP _udp;
  mesh::UDPBridgePrefs* _prefs;
  mesh::Dispatcher* _dispatcher;

public:
  UdpBridge(mesh::UDPBridgePrefs* prefs);
  void start() override;
  void loop() override;
  void stop();
  void onMeshPacketRx(mesh::Packet* packet) override;
  void onMeshPacketTx(mesh::Packet* packet) override;
};


};