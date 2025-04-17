#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#if defined(NRF52_PLATFORM)
  #include <InternalFileSystem.h>
#elif defined(ESP32)
  #include <SPIFFS.h>
  #include <WiFi.h>
  #include <AsyncUDP.h>
  #include <Vector.h>
  #include <Packet.h>
  #include <Dispatcher.h>
#endif

#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include <helpers/AdvertDataHelpers.h>
#include <helpers/TxtDataHelpers.h>
#include <helpers/CommonCLI.h>
#include <RTClib.h>
#include <target.h>

/* ------------------------------ Config -------------------------------- */

#ifndef FIRMWARE_BUILD_DATE
  #define FIRMWARE_BUILD_DATE   "7 Apr 2025"
#endif

#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION   "v1.4.3"
#endif

#ifndef LORA_FREQ
  #define LORA_FREQ   915.0
#endif
#ifndef LORA_BW
  #define LORA_BW     250
#endif
#ifndef LORA_SF
  #define LORA_SF     10
#endif
#ifndef LORA_CR
  #define LORA_CR      5
#endif
#ifndef LORA_TX_POWER
  #define LORA_TX_POWER  20
#endif

#ifndef ADVERT_NAME
  #define  ADVERT_NAME   "repeater"
#endif
#ifndef ADVERT_LAT
  #define  ADVERT_LAT  0.0
#endif
#ifndef ADVERT_LON
  #define  ADVERT_LON  0.0
#endif

#ifndef ADMIN_PASSWORD
  #define  ADMIN_PASSWORD  "password"
#endif

#ifndef WIFI_ENABLE
  #define WIFI_ENABLE false
#endif

#ifndef WIFI_BRIDGE_HOST
  #define WIFI_BRIDGE_HOST false
#endif

#ifndef WIFI_BRIDGE_SSID
  #define WIFI_BRIDGE_SSID "meshcore-bridge"
#endif

#ifndef WIFI_BRIDGE_PASSWORD
  #define WIFI_BRIDGE_PASSWORD "password"
#endif

#ifndef UDP_SERVER_ENABLE
  #define UDP_SERVER_ENABLE false
#endif

#ifndef UDP_SERVER_PORT
  #define UDP_SERVER_PORT 8000
#endif

#ifndef UDP_PACKET_BUFFER_SIZE
  #define UDP_PACKET_BUFFER_SIZE 10
#endif

#ifdef DISPLAY_CLASS
  #include <helpers/ui/SSD1306Display.h>

  static DISPLAY_CLASS  display;

  #include "UITask.h"
  static UITask ui_task(display);
#endif

#define FIRMWARE_ROLE "repeater"

#define PACKET_LOG_FILE  "/packet_log"

/* ------------------------------ Code -------------------------------- */

#define CMD_GET_STATUS      0x01

#define RESP_SERVER_LOGIN_OK      0   // response to ANON_REQ

#ifdef WiFi_h
void wifiEventCb(arduino_event_t* event);
#endif

struct RepeaterStats {
  uint16_t batt_milli_volts;
  uint16_t curr_tx_queue_len;
  uint16_t curr_free_queue_len;
  int16_t  last_rssi;
  uint32_t n_packets_recv;
  uint32_t n_packets_sent;
  uint32_t total_air_time_secs;
  uint32_t total_up_time_secs;
  uint32_t n_sent_flood, n_sent_direct;
  uint32_t n_recv_flood, n_recv_direct;
  uint16_t n_full_events;
  int16_t  last_snr;   // x 4
  uint16_t n_direct_dups, n_flood_dups;
};

struct ClientInfo {
  mesh::Identity id;
  uint32_t last_timestamp, last_activity;
  uint8_t secret[PUB_KEY_SIZE];
  bool    is_admin;
  int8_t  out_path_len;
  uint8_t out_path[MAX_PATH_SIZE];
};

#define MAX_CLIENTS   4

// NOTE: need to space the ACK and the reply text apart (in CLI)
#define CLI_REPLY_DELAY_MILLIS  1500

class MyMesh : public mesh::Mesh, public CommonCLICallbacks {
  FILESYSTEM* _fs;
  unsigned long next_local_advert, next_flood_advert;
  bool _logging;
  NodePrefs _prefs;
  CommonCLI _cli;
  uint8_t reply_data[MAX_PACKET_PAYLOAD];
  ClientInfo known_clients[MAX_CLIENTS];

  #ifdef WiFi_h
  AsyncUDP udp;
  mesh::Packet* inboundUdpBuffer[UDP_PACKET_BUFFER_SIZE];
  Vector<mesh::Packet*> inboundUdpPackets;
  bool _isServerRunning;
  #endif

  ClientInfo* putClient(const mesh::Identity& id) {
    uint32_t min_time = 0xFFFFFFFF;
    ClientInfo* oldest = &known_clients[0];
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (known_clients[i].last_activity < min_time) {
        oldest = &known_clients[i];
        min_time = oldest->last_activity;
      }
      if (id.matches(known_clients[i].id)) return &known_clients[i];  // already known
    }

    oldest->id = id;
    oldest->out_path_len = -1;  // initially out_path is unknown
    oldest->last_timestamp = 0;
    self_id.calcSharedSecret(oldest->secret, id);   // calc ECDH shared secret
    return oldest;
  }

  int handleRequest(ClientInfo* sender, uint8_t* payload, size_t payload_len) {
    uint32_t now = getRTCClock()->getCurrentTimeUnique();
    memcpy(reply_data, &now, 4);   // response packets always prefixed with timestamp

    switch (payload[0]) {
      case CMD_GET_STATUS: {   // guests can also access this now
        RepeaterStats stats;
        stats.batt_milli_volts = board.getBattMilliVolts();
        stats.curr_tx_queue_len = _mgr->getOutboundCount();
        stats.curr_free_queue_len = _mgr->getFreeCount();
        stats.last_rssi = (int16_t) radio_driver.getLastRSSI();
        stats.n_packets_recv = radio_driver.getPacketsRecv();
        stats.n_packets_sent = radio_driver.getPacketsSent();
        stats.total_air_time_secs = getTotalAirTime() / 1000;
        stats.total_up_time_secs = _ms->getMillis() / 1000;
        stats.n_sent_flood = getNumSentFlood();
        stats.n_sent_direct = getNumSentDirect();
        stats.n_recv_flood = getNumRecvFlood();
        stats.n_recv_direct = getNumRecvDirect();
        stats.n_full_events = getNumFullEvents();
        stats.last_snr = (int16_t)(radio_driver.getLastSNR() * 4);
        stats.n_direct_dups = ((SimpleMeshTables *)getTables())->getNumDirectDups();
        stats.n_flood_dups = ((SimpleMeshTables *)getTables())->getNumFloodDups();

        memcpy(&reply_data[4], &stats, sizeof(stats));

        return 4 + sizeof(stats);  //  reply_len
      }
    }
    // unknown command
    return 0;  // reply_len
  }

  mesh::Packet* createSelfAdvert() {
    uint8_t app_data[MAX_ADVERT_DATA_SIZE];
    uint8_t app_data_len;
    {
      AdvertDataBuilder builder(ADV_TYPE_REPEATER, _prefs.node_name, _prefs.node_lat, _prefs.node_lon);
      app_data_len = builder.encodeTo(app_data);
    }

   return createAdvert(self_id, app_data, app_data_len);
  }

  File openAppend(const char* fname) {
  #if defined(NRF52_PLATFORM)
    return _fs->open(fname, FILE_O_WRITE);
  #else
    return _fs->open(fname, "a", true);
  #endif
  }

#ifdef WiFi_h

  void onSendPacket(mesh::Packet* pkt) override {
    if(_prefs.udp_bridge_enable && _isServerRunning){ 
    
      Serial.println("Bridging outbound packet to network");

      uint8_t pktBuffer[256];
      uint8_t pktLen = pkt->writeTo(pktBuffer);
      
      udp.broadcastTo( pktBuffer, pktLen, _prefs.udp_bridge_server_port);

    }
  }

public:

  void handleUdpPackets(){

    if(!_isServerRunning){ return; }

    if(!inboundUdpPackets.empty()){
      Serial.printf("Processing %d udp packets\n", inboundUdpPackets.size());
    }

    while(!inboundUdpPackets.empty()){
      mesh::Packet* pkt = inboundUdpPackets.front();
      inboundUdpPackets.remove(0);

      mesh::DispatcherAction action = onRecvPacket(pkt);
      if (action == ACTION_RELEASE) {
        Serial.println("release udp packet");
        _mgr->free(pkt);
      } else if (action == ACTION_MANUAL_HOLD) {
        Serial.println("hold");
        // sub-class is wanting to manually hold Packet instance, and call releasePacket() at appropriate time
      } else {   // ACTION_RETRANSMIT*
        uint8_t priority = (action >> 24) - 1;
        uint32_t _delay = action & 0xFFFFFF;

        Serial.println("bridging udp packet to mesh");

        _mgr->queueOutbound(pkt, priority, futureMillis(_delay));
      }
    }

  }

  void startServer(){

    if(_isServerRunning || !_prefs.udp_bridge_enable){ return; }

    inboundUdpPackets = Vector<mesh::Packet*>(inboundUdpBuffer);

    if (udp.listen(_prefs.udp_bridge_server_port)) {

      _isServerRunning = true;
      Serial.println("UDP Listening on IP: ");
      Serial.println(WiFi.localIP());
      udp.onPacket([this](AsyncUDPPacket packet) {
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

  void wifiEventHandler(arduino_event_t* event) {
    switch (event->event_id) {
      case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println("ESP32 Connected to WiFi Network");
        this->startServer();
        break;
      case  SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("ESP32 got IP");
        this->startServer();
        break;
      case SYSTEM_EVENT_AP_START:
        Serial.println("ESP32 soft AP started");
        this->startServer();
        break;
      case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        Serial.println("Station connected to ESP32 soft AP");
        break;
      case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        Serial.println("Station disconnected from ESP32 soft AP");
        break;
      case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        Serial.println("IP assigned to soft AP client");
        this->startServer();
        break;
      default:
        Serial.printf("wifi event %d \n", (uint8_t) event->event_id );
        break;
    }
  }

#endif


protected:
  float getAirtimeBudgetFactor() const override {
    return _prefs.airtime_factor;
  }

  bool allowPacketForward(const mesh::Packet* packet) override {
    if (_prefs.disable_fwd) return false;
    if (packet->isRouteFlood() && packet->path_len >= _prefs.flood_max) return false;
    return true;
  }

  const char* getLogDateTime() override {
    static char tmp[32];
    uint32_t now = getRTCClock()->getCurrentTime();
    DateTime dt = DateTime(now);
    sprintf(tmp, "%02d:%02d:%02d - %d/%d/%d U", dt.hour(), dt.minute(), dt.second(), dt.day(), dt.month(), dt.year());
    return tmp;
  }

  void logRxRaw(float snr, float rssi, const uint8_t raw[], int len) override {
  #if MESH_PACKET_LOGGING
    Serial.print(getLogDateTime());
    Serial.print(" RAW: ");
    mesh::Utils::printHex(Serial, raw, len);
    Serial.println();
  #endif
  }

  void logRx(mesh::Packet* pkt, int len, float score) override {
    if (_logging) {
      File f = openAppend(PACKET_LOG_FILE);
      if (f) {
        f.print(getLogDateTime());
        f.printf(": RX, len=%d (type=%d, route=%s, payload_len=%d) SNR=%d RSSI=%d score=%d",
          len, pkt->getPayloadType(), pkt->isRouteDirect() ? "D" : "F", pkt->payload_len,
          (int)_radio->getLastSNR(), (int)_radio->getLastRSSI(), (int)(score*1000));

        if (pkt->getPayloadType() == PAYLOAD_TYPE_PATH || pkt->getPayloadType() == PAYLOAD_TYPE_REQ
          || pkt->getPayloadType() == PAYLOAD_TYPE_RESPONSE || pkt->getPayloadType() == PAYLOAD_TYPE_TXT_MSG) {
          f.printf(" [%02X -> %02X]\n", (uint32_t)pkt->payload[1], (uint32_t)pkt->payload[0]);
        } else {
          f.printf("\n");
        }
        f.close();
      }
    }
  }
  void logTx(mesh::Packet* pkt, int len) override {
    if (_logging) {
      File f = openAppend(PACKET_LOG_FILE);
      if (f) {
        f.print(getLogDateTime());
        f.printf(": TX, len=%d (type=%d, route=%s, payload_len=%d)",
          len, pkt->getPayloadType(), pkt->isRouteDirect() ? "D" : "F", pkt->payload_len);

        if (pkt->getPayloadType() == PAYLOAD_TYPE_PATH || pkt->getPayloadType() == PAYLOAD_TYPE_REQ
          || pkt->getPayloadType() == PAYLOAD_TYPE_RESPONSE || pkt->getPayloadType() == PAYLOAD_TYPE_TXT_MSG) {
          f.printf(" [%02X -> %02X]\n", (uint32_t)pkt->payload[1], (uint32_t)pkt->payload[0]);
        } else {
          f.printf("\n");
        }
        f.close();
      }
    }
  }
  void logTxFail(mesh::Packet* pkt, int len) override {
    if (_logging) {
      File f = openAppend(PACKET_LOG_FILE);
      if (f) {
        f.print(getLogDateTime());
        f.printf(": TX FAIL!, len=%d (type=%d, route=%s, payload_len=%d)\n",
          len, pkt->getPayloadType(), pkt->isRouteDirect() ? "D" : "F", pkt->payload_len);
        f.close();
      }
    }
  }

  int calcRxDelay(float score, uint32_t air_time) const override {
    if (_prefs.rx_delay_base <= 0.0f) return 0;
    return (int) ((pow(_prefs.rx_delay_base, 0.85f - score) - 1.0) * air_time);
  }

  uint32_t getRetransmitDelay(const mesh::Packet* packet) override {
    uint32_t t = (_radio->getEstAirtimeFor(packet->path_len + packet->payload_len + 2) * _prefs.tx_delay_factor);
    return getRNG()->nextInt(0, 6)*t;
  }
  uint32_t getDirectRetransmitDelay(const mesh::Packet* packet) override {
    uint32_t t = (_radio->getEstAirtimeFor(packet->path_len + packet->payload_len + 2) * _prefs.direct_tx_delay_factor);
    return getRNG()->nextInt(0, 6)*t;
  }

  void onAnonDataRecv(mesh::Packet* packet, uint8_t type, const mesh::Identity& sender, uint8_t* data, size_t len) override {
    if (type == PAYLOAD_TYPE_ANON_REQ) {  // received an initial request by a possible admin client (unknown at this stage)
      uint32_t timestamp;
      memcpy(&timestamp, data, 4);

      bool is_admin;
      data[len] = 0;  // ensure null terminator
      if (strcmp((char *) &data[4], _prefs.password) == 0) {  // check for valid password
        is_admin = true;
      } else if (strcmp((char *) &data[4], _prefs.guest_password) == 0) {  // check guest password
        is_admin = false;
      } else {
    #if MESH_DEBUG
        MESH_DEBUG_PRINTLN("Invalid password: %s", &data[4]);
    #endif
        return;
      }

      auto client = putClient(sender);  // add to known clients (if not already known)
      if (timestamp <= client->last_timestamp) {
        MESH_DEBUG_PRINTLN("Possible login replay attack!");
        return;  // FATAL: client table is full -OR- replay attack
      }

      MESH_DEBUG_PRINTLN("Login success!");
      client->last_timestamp = timestamp;
      client->last_activity = getRTCClock()->getCurrentTime();
      client->is_admin = is_admin;

      uint32_t now = getRTCClock()->getCurrentTimeUnique();
      memcpy(reply_data, &now, 4);   // response packets always prefixed with timestamp
    #if 0
      memcpy(&reply_data[4], "OK", 2);   // legacy response
    #else
      reply_data[4] = RESP_SERVER_LOGIN_OK;
      reply_data[5] = 0;  // NEW: recommended keep-alive interval (secs / 16)
      reply_data[6] = is_admin ? 1 : 0;
      reply_data[7] = 0;  // FUTURE: reserved
      getRNG()->random(&reply_data[8], 4);   // random blob to help packet-hash uniqueness
  #endif

      if (packet->isRouteFlood()) {
        // let this sender know path TO here, so they can use sendDirect(), and ALSO encode the response
        mesh::Packet* path = createPathReturn(sender, client->secret, packet->path, packet->path_len,
                                              PAYLOAD_TYPE_RESPONSE, reply_data, 12);
        if (path) sendFlood(path);
      } else {
        mesh::Packet* reply = createDatagram(PAYLOAD_TYPE_RESPONSE, sender, client->secret, reply_data, 12);
        if (reply) {
          if (client->out_path_len >= 0) {  // we have an out_path, so send DIRECT
            sendDirect(reply, client->out_path, client->out_path_len);
          } else {
            sendFlood(reply);
          }
        }
      }
    }
  }

  int  matching_peer_indexes[MAX_CLIENTS];

  int searchPeersByHash(const uint8_t* hash) override {
    int n = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (known_clients[i].id.isHashMatch(hash)) {
        matching_peer_indexes[n++] = i;  // store the INDEXES of matching contacts (for subsequent 'peer' methods)
      }
    }
    return n;
  }

  void getPeerSharedSecret(uint8_t* dest_secret, int peer_idx) override {
    int i = matching_peer_indexes[peer_idx];
    if (i >= 0 && i < MAX_CLIENTS) {
      // lookup pre-calculated shared_secret
      memcpy(dest_secret, known_clients[i].secret, PUB_KEY_SIZE);
    } else {
      MESH_DEBUG_PRINTLN("getPeerSharedSecret: Invalid peer idx: %d", i);
    }
  }

  void onPeerDataRecv(mesh::Packet* packet, uint8_t type, int sender_idx, const uint8_t* secret, uint8_t* data, size_t len) override {
    int i = matching_peer_indexes[sender_idx];
    if (i < 0 || i >= MAX_CLIENTS) {  // get from our known_clients table (sender SHOULD already be known in this context)
      MESH_DEBUG_PRINTLN("onPeerDataRecv: invalid peer idx: %d", i);
      return;
    }
    auto client = &known_clients[i];
    if (type == PAYLOAD_TYPE_REQ) {  // request (from a Known admin client!)
      uint32_t timestamp;
      memcpy(&timestamp, data, 4);

      if (timestamp > client->last_timestamp) {  // prevent replay attacks
        int reply_len = handleRequest(client, &data[4], len - 4);
        if (reply_len == 0) return;  // invalid command

        client->last_timestamp = timestamp;
        client->last_activity = getRTCClock()->getCurrentTime();

        if (packet->isRouteFlood()) {
          // let this sender know path TO here, so they can use sendDirect(), and ALSO encode the response
          mesh::Packet* path = createPathReturn(client->id, secret, packet->path, packet->path_len,
                                                PAYLOAD_TYPE_RESPONSE, reply_data, reply_len);
          if (path) sendFlood(path);
        } else {
          mesh::Packet* reply = createDatagram(PAYLOAD_TYPE_RESPONSE, client->id, secret, reply_data, reply_len);
          if (reply) {
            if (client->out_path_len >= 0) {  // we have an out_path, so send DIRECT
              sendDirect(reply, client->out_path, client->out_path_len);
            } else {
              sendFlood(reply);
            }
          }
        }
      } else {
        MESH_DEBUG_PRINTLN("onPeerDataRecv: possible replay attack detected");
      }
    } else if (type == PAYLOAD_TYPE_TXT_MSG && len > 5 && client->is_admin) {   // a CLI command
      uint32_t sender_timestamp;
      memcpy(&sender_timestamp, data, 4);  // timestamp (by sender's RTC clock - which could be wrong)
      uint flags = (data[4] >> 2);   // message attempt number, and other flags

      if (!(flags == TXT_TYPE_PLAIN || flags == TXT_TYPE_CLI_DATA)) {
        MESH_DEBUG_PRINTLN("onPeerDataRecv: unsupported text type received: flags=%02x", (uint32_t)flags);
      } else if (sender_timestamp >= client->last_timestamp) {  // prevent replay attacks
        bool is_retry = (sender_timestamp == client->last_timestamp);
        client->last_timestamp = sender_timestamp;
        client->last_activity = getRTCClock()->getCurrentTime();

        // len can be > original length, but 'text' will be padded with zeroes
        data[len] = 0; // need to make a C string again, with null terminator

        if (flags == TXT_TYPE_PLAIN) {  // for legacy CLI, send Acks
          uint32_t ack_hash;    // calc truncated hash of the message timestamp + text + sender pub_key, to prove to sender that we got it
          mesh::Utils::sha256((uint8_t *) &ack_hash, 4, data, 5 + strlen((char *)&data[5]), client->id.pub_key, PUB_KEY_SIZE);

          mesh::Packet* ack = createAck(ack_hash);
          if (ack) {
            if (client->out_path_len < 0) {
              sendFlood(ack);
            } else {
              sendDirect(ack, client->out_path, client->out_path_len);
            }
          }
        }

        uint8_t temp[166];
        if (is_retry) {
          temp[0] = 0;
        } else {
          _cli.handleCommand(sender_timestamp, (const char *) &data[5], (char *) &temp[5]);
        }
        int text_len = strlen((char *) &temp[5]);
        if (text_len > 0) {
          uint32_t timestamp = getRTCClock()->getCurrentTimeUnique();
          if (timestamp == sender_timestamp) {
            // WORKAROUND: the two timestamps need to be different, in the CLI view
            timestamp++;
          }
          memcpy(temp, &timestamp, 4);   // mostly an extra blob to help make packet_hash unique
          temp[4] = (TXT_TYPE_CLI_DATA << 2);   // NOTE: legacy was: TXT_TYPE_PLAIN

          auto reply = createDatagram(PAYLOAD_TYPE_TXT_MSG, client->id, secret, temp, 5 + text_len);
          if (reply) {
            if (client->out_path_len < 0) {
              sendFlood(reply, CLI_REPLY_DELAY_MILLIS);
            } else {
              sendDirect(reply, client->out_path, client->out_path_len, CLI_REPLY_DELAY_MILLIS);
            }
          }
        }
      } else {
        MESH_DEBUG_PRINTLN("onPeerDataRecv: possible replay attack detected");
      }
    }
  }

  bool onPeerPathRecv(mesh::Packet* packet, int sender_idx, const uint8_t* secret, uint8_t* path, uint8_t path_len, uint8_t extra_type, uint8_t* extra, uint8_t extra_len) override {
    // TODO: prevent replay attacks
    int i = matching_peer_indexes[sender_idx];

    if (i >= 0 && i < MAX_CLIENTS) {  // get from our known_clients table (sender SHOULD already be known in this context)
      MESH_DEBUG_PRINTLN("PATH to client, path_len=%d", (uint32_t) path_len);
      auto client = &known_clients[i];
      memcpy(client->out_path, path, client->out_path_len = path_len);  // store a copy of path, for sendDirect()
    } else {
      MESH_DEBUG_PRINTLN("onPeerPathRecv: invalid peer idx: %d", i);
    }

    // NOTE: no reciprocal path send!!
    return false;
  }

public:
  MyMesh(mesh::MainBoard& board, mesh::Radio& radio, mesh::MillisecondClock& ms, mesh::RNG& rng, mesh::RTCClock& rtc, mesh::MeshTables& tables)
     : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(32), tables),
      _cli(board, this, &_prefs, this)
  {
    memset(known_clients, 0, sizeof(known_clients));
    next_local_advert = next_flood_advert = 0;
    _logging = false;
    #ifdef WiFi_h
    _isServerRunning = false;
    #endif
    // defaults
    memset(&_prefs, 0, sizeof(_prefs));
    _prefs.airtime_factor = 1.0;    // one half
    _prefs.rx_delay_base =   0.0f;  // turn off by default, was 10.0;
    _prefs.tx_delay_factor = 0.5f;   // was 0.25f
    StrHelper::strncpy(_prefs.node_name, ADVERT_NAME, sizeof(_prefs.node_name));
    _prefs.node_lat = ADVERT_LAT;
    _prefs.node_lon = ADVERT_LON;
    StrHelper::strncpy(_prefs.password, ADMIN_PASSWORD, sizeof(_prefs.password));
    _prefs.freq = LORA_FREQ;
    _prefs.sf = LORA_SF;
    _prefs.bw = LORA_BW;
    _prefs.cr = LORA_CR;
    _prefs.tx_power_dbm = LORA_TX_POWER;
    _prefs.advert_interval = 1;  // default to 2 minutes for NEW installs
    _prefs.flood_advert_interval = 3;   // 3 hours
    _prefs.flood_max = 64;

    _prefs.wifi_enable = WIFI_ENABLE;
    _prefs.wifi_ap_enable = WIFI_BRIDGE_HOST;
    StrHelper::strncpy(_prefs.wifi_ssid, WIFI_BRIDGE_SSID, sizeof(_prefs.wifi_ssid));
    StrHelper::strncpy(_prefs.wifi_password, WIFI_BRIDGE_PASSWORD, sizeof(_prefs.wifi_password));
    _prefs.udp_bridge_enable = UDP_SERVER_ENABLE;
    _prefs.udp_bridge_server_port = UDP_SERVER_PORT;
  }

  CommonCLI* getCLI() { return &_cli; }

  void begin(FILESYSTEM* fs) {
    mesh::Mesh::begin();
    _fs = fs;
    // load persisted prefs
    _cli.loadPrefs(_fs);

    radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
    radio_set_tx_power(_prefs.tx_power_dbm);

#ifdef WiFi_h

    if(_prefs.wifi_ap_enable){
    
      WiFi.disconnect(true);
      Serial.println("wifi ap starting");
      
      WiFi.onEvent(wifiEventCb);
      WiFi.mode(WIFI_MODE_AP);
      WiFi.softAP(_prefs.wifi_ssid, _prefs.wifi_password);
    
    } else if(_prefs.wifi_enable) {

      WiFi.disconnect(true);
      Serial.println("wifi client starting");
      WiFi.onEvent(wifiEventCb);
      WiFi.mode(WIFI_MODE_STA);
      WiFi.begin(_prefs.wifi_ssid, _prefs.wifi_password);

    }
#endif

    updateAdvertTimer();
    updateFloodAdvertTimer();
  }

  const char* getFirmwareVer() override { return FIRMWARE_VERSION; }
  const char* getBuildDate() override { return FIRMWARE_BUILD_DATE; }
  const char* getRole() override { return FIRMWARE_ROLE; }
  const char* getNodeName() { return _prefs.node_name; }

  void savePrefs() override {
    _cli.savePrefs(_fs);
  }

  bool formatFileSystem() override {
#if defined(NRF52_PLATFORM)
    return InternalFS.format();
#elif defined(ESP32)
    return SPIFFS.format();
#else
  #error "need to implement file system erase"
    return false;
#endif
  }

  void sendSelfAdvertisement(int delay_millis) override {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) {
      sendFlood(pkt, delay_millis);
    } else {
      MESH_DEBUG_PRINTLN("ERROR: unable to create advertisement packet!");
    }
  }

  void updateAdvertTimer() override {
    if (_prefs.advert_interval > 0) {  // schedule local advert timer
      next_local_advert = futureMillis( ((uint32_t)_prefs.advert_interval) * 2 * 60 * 1000);
    } else {
      next_local_advert = 0;  // stop the timer
    }
  }
  void updateFloodAdvertTimer() override {
    if (_prefs.flood_advert_interval > 0) {  // schedule flood advert timer
      next_flood_advert = futureMillis( ((uint32_t)_prefs.flood_advert_interval) * 60 * 60 * 1000);
    } else {
      next_flood_advert = 0;  // stop the timer
    }
  }

  void setLoggingOn(bool enable) override { _logging = enable; }

  void eraseLogFile() override {
    _fs->remove(PACKET_LOG_FILE);
  }

  void dumpLogFile() override {
    File f = _fs->open(PACKET_LOG_FILE);
    if (f) {
      while (f.available()) {
        int c = f.read();
        if (c < 0) break;
        Serial.print((char)c);
      }
      f.close();
    }
  }

  void setTxPower(uint8_t power_dbm) override {
    radio_set_tx_power(power_dbm);
  }

  void loop() {
#ifdef WiFi_h
    handleUdpPackets();
#endif
    mesh::Mesh::loop();

    if (next_flood_advert && millisHasNowPassed(next_flood_advert)) {
      mesh::Packet* pkt = createSelfAdvert();
      if (pkt) sendFlood(pkt);

      updateFloodAdvertTimer();   // schedule next flood advert
      updateAdvertTimer();   // also schedule local advert (so they don't overlap)
    } else if (next_local_advert && millisHasNowPassed(next_local_advert)) {
      mesh::Packet* pkt = createSelfAdvert();
      if (pkt) sendZeroHop(pkt);

      updateAdvertTimer();   // schedule next local advert
    }
  #ifdef DISPLAY_CLASS
    ui_task.loop();
  #endif
  }
};

StdRNG fast_rng;
SimpleMeshTables tables;

MyMesh the_mesh(board, radio_driver, *new ArduinoMillis(), fast_rng, rtc_clock, tables);

void halt() {
  while (1) ;
}

static char command[80];

#ifdef WiFi_h
void wifiEventCb(arduino_event_t* event){
  the_mesh.wifiEventHandler(event);
}
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();

#ifdef DISPLAY_CLASS
  if(display.begin()){
    display.startFrame();
    display.print("Please wait...");
    display.endFrame();
  }
#endif

  if (!radio_init()) { halt(); }

  fast_rng.begin(radio_get_rng_seed());

  FILESYSTEM* fs;
#if defined(NRF52_PLATFORM)
  InternalFS.begin();
  fs = &InternalFS;
  IdentityStore store(InternalFS, "");
#elif defined(ESP32)
  SPIFFS.begin(true);
  fs = &SPIFFS;
  IdentityStore store(SPIFFS, "/identity");
#else
  #error "need to define filesystem"
#endif
  if (!store.load("_main", the_mesh.self_id)) {
    MESH_DEBUG_PRINTLN("Generating new keypair");
    the_mesh.self_id = radio_new_identity();   // create new random identity
    int count = 0;
    while (count < 10 && (the_mesh.self_id.pub_key[0] == 0x00 || the_mesh.self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
      the_mesh.self_id = radio_new_identity(); count++;
    }
    store.save("_main", the_mesh.self_id);
  }

  Serial.print("Repeater ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE); Serial.println();

  command[0] = 0;

  the_mesh.begin(fs);

#ifdef DISPLAY_CLASS
  ui_task.begin(the_mesh.getNodeName(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION);
#endif

  // send out initial Advertisement to the mesh
  the_mesh.sendSelfAdvertisement(16000);
}

void loop() {
  int len = strlen(command);
  while (Serial.available() && len < sizeof(command)-1) {
    char c = Serial.read();
    if (c != '\n') {
      command[len++] = c;
      command[len] = 0;
    }
    Serial.print(c);
  }
  if (len == sizeof(command)-1) {  // command buffer full
    command[sizeof(command)-1] = '\r';
  }

  if (len > 0 && command[len - 1] == '\r') {  // received complete line
    command[len - 1] = 0;  // replace newline with C string null terminator
    char reply[160];
    the_mesh.getCLI()->handleCommand(0, command, reply);  // NOTE: there is no sender_timestamp via serial!
    if (reply[0]) {
      Serial.print("  -> "); Serial.println(reply);
    }

    command[0] = 0;  // reset command buffer
  }

  the_mesh.loop();
}
