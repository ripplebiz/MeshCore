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
  UDPBridgePrefs* _prefs;
  mesh::Dispatcher* _dispatcher;

public:
  UdpBridge(UDPBridgePrefs* prefs);
  void start() override;
  void loop() override;
  void stop();
  void onMeshPacketRx(mesh::Packet* packet) override;
  void onMeshPacketTx(mesh::Packet* packet) override;
};


};