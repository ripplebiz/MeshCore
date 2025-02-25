#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#if defined(NRF52_PLATFORM)
  #include <InternalFileSystem.h>
#elif defined(ESP32)
  #include <SPIFFS.h>
#endif

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/AdvertDataHelpers.h>
#include <helpers/TxtDataHelpers.h>
#include <RTClib.h>

/* ------------------------------ Config -------------------------------- */

#define FIRMWARE_VER_TEXT   "v5 (build: 25 Feb 2025)"

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

#define MIN_LOCAL_ADVERT_INTERVAL   60

#if defined(HELTEC_LORA_V3)
  #include <helpers/HeltecV3Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static HeltecV3Board board;
#elif defined(HELTEC_LORA_V2)
  #include <helpers/HeltecV2Board.h>
  #include <helpers/CustomSX1276Wrapper.h>
  static HeltecV2Board board;
#elif defined(ARDUINO_XIAO_ESP32C3)
  #include <helpers/XiaoC3Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  #include <helpers/CustomSX1268Wrapper.h>
  static XiaoC3Board board;
#elif defined(SEEED_XIAO_S3) || defined(LILYGO_T3S3)
  #include <helpers/ESP32Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static ESP32Board board;
#elif defined(RAK_4631)
  #include <helpers/nrf52/RAK4631Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static RAK4631Board board;
#else
  #error "need to provide a 'board' object"
#endif

#define PACKET_LOG_FILE  "/packet_log"

/* ------------------------------ Code -------------------------------- */

// Believe it or not, this std C function is busted on some platforms!
static uint32_t _atoi(const char* sp) {
  uint32_t n = 0;
  while (*sp && *sp >= '0' && *sp <= '9') {
    n *= 10;
    n += (*sp++ - '0');
  }
  return n;
}

#define CMD_GET_STATUS      0x01

#define RESP_SERVER_LOGIN_OK      0   // response to ANON_REQ

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
  uint16_t n_full_events, reserved1;
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

struct NodePrefs {  // persisted to file
  float airtime_factor;
  char node_name[32];
  double node_lat, node_lon;
  char password[16];
  float freq;
  uint8_t tx_power_dbm;
  uint8_t disable_fwd;
  uint8_t advert_interval;   // minutes / 2
  uint8_t unused;
  float rx_delay_base;
  float tx_delay_factor;
  char guest_password[16];
  float direct_tx_delay_factor;
};

class MyMesh : public mesh::Mesh {
  RadioLibWrapper* my_radio;
  FILESYSTEM* _fs;
  mesh::MainBoard* _board;
  unsigned long next_local_advert;
  bool _logging;
  NodePrefs _prefs;
  uint8_t reply_data[MAX_PACKET_PAYLOAD];
  ClientInfo known_clients[MAX_CLIENTS];

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
        stats.last_rssi = (int16_t) my_radio->getLastRSSI();
        stats.n_packets_recv = my_radio->getPacketsRecv();
        stats.n_packets_sent = my_radio->getPacketsSent();
        stats.total_air_time_secs = getTotalAirTime() / 1000;
        stats.total_up_time_secs = _ms->getMillis() / 1000;
        stats.n_sent_flood = getNumSentFlood();
        stats.n_sent_direct = getNumSentDirect();
        stats.n_recv_flood = getNumRecvFlood();
        stats.n_recv_direct = getNumRecvDirect();
        stats.n_full_events = getNumFullEvents();
        stats.reserved1 = 0;
        stats.n_direct_dups = ((SimpleMeshTables *)getTables())->getNumDirectDups();
        stats.n_flood_dups = ((SimpleMeshTables *)getTables())->getNumFloodDups();

        memcpy(&reply_data[4], &stats, sizeof(stats));

        return 4 + sizeof(stats);  //  reply_len
      }
    }
    // unknown command
    return 0;  // reply_len
  }

  void checkAdvertInterval() {
    if (_prefs.advert_interval * 2 < MIN_LOCAL_ADVERT_INTERVAL) {
      _prefs.advert_interval = 0;  // turn it off, now that device has been manually configured
    }
  }

  void updateAdvertTimer() {
    if (_prefs.advert_interval > 0) {  // schedule local advert timer
      next_local_advert = futureMillis((uint32_t)_prefs.advert_interval * 2 * 60 * 1000);
    } else {
      next_local_advert = 0;  // stop the timer
    }
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

protected:
  float getAirtimeBudgetFactor() const override {
    return _prefs.airtime_factor;
  }

  bool allowPacketForward(const mesh::Packet* packet) override {
    return !_prefs.disable_fwd;
  }

  const char* getLogDateTime() override { 
    static char tmp[32];
    uint32_t now = getRTCClock()->getCurrentTime();
    DateTime dt = DateTime(now);
    sprintf(tmp, "%02d:%02d:%02d - %d/%d/%d U", dt.hour(), dt.minute(), dt.second(), dt.day(), dt.month(), dt.year());
    return tmp;
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
      } else if (sender_timestamp > client->last_timestamp) {  // prevent replay attacks 
        client->last_timestamp = sender_timestamp;
        client->last_activity = getRTCClock()->getCurrentTime();

        // len can be > original length, but 'text' will be padded with zeroes
        data[len] = 0; // need to make a C string again, with null terminator

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

        uint8_t temp[166];
        handleCommand(sender_timestamp, (const char *) &data[5], (char *) &temp[5]);
        int text_len = strlen((char *) &temp[5]);
        if (text_len > 0) {
          uint32_t timestamp = getRTCClock()->getCurrentTimeUnique();
          if (timestamp == sender_timestamp) {
            // WORKAROUND: the two timestamps need to be different, in the CLI view
            timestamp++;
          }
          memcpy(temp, &timestamp, 4);   // mostly an extra blob to help make packet_hash unique
          temp[4] = (TXT_TYPE_PLAIN << 2);   // TODO: change this to TXT_TYPE_CLI_DATA soon

          // calc expected ACK reply
          //mesh::Utils::sha256((uint8_t *)&expected_ack_crc, 4, temp, 5 + text_len, self_id.pub_key, PUB_KEY_SIZE);

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
  MyMesh(mesh::MainBoard& board, RadioLibWrapper& radio, mesh::MillisecondClock& ms, mesh::RNG& rng, mesh::RTCClock& rtc, SimpleMeshTables& tables)
     : mesh::Mesh(radio, ms, rng, rtc, *new StaticPoolPacketManager(32), tables), _board(&board)
  {
    my_radio = &radio;
    memset(known_clients, 0, sizeof(known_clients));
    next_local_advert = 0;
    _logging = false;

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
    _prefs.tx_power_dbm = LORA_TX_POWER;
    _prefs.advert_interval = 1;  // default to 2 minutes for NEW installs
  }

  float getFreqPref() const { return _prefs.freq; }
  uint8_t getTxPowerPref() const { return _prefs.tx_power_dbm; }

  void begin(FILESYSTEM* fs) {
    mesh::Mesh::begin();
    _fs = fs;
    // load persisted prefs
    if (_fs->exists("/node_prefs")) {
      File file = _fs->open("/node_prefs");
      if (file) {
        file.read((uint8_t *) &_prefs, sizeof(_prefs));
        file.close();
      }
    }

    updateAdvertTimer();
  }

  void savePrefs() {
#if defined(NRF52_PLATFORM)
    File file = _fs->open("/node_prefs", FILE_O_WRITE);
    if (file) { file.seek(0); file.truncate(); }
#else
    File file = _fs->open("/node_prefs", "w", true);
#endif
    if (file) {
      file.write((const uint8_t *)&_prefs, sizeof(_prefs));
      file.close();
    }
  }

  void sendSelfAdvertisement(int delay_millis) {
    mesh::Packet* pkt = createSelfAdvert();
    if (pkt) {
      sendFlood(pkt, delay_millis);
    } else {
      MESH_DEBUG_PRINTLN("ERROR: unable to create advertisement packet!");
    }
  }

  void loop() {
    mesh::Mesh::loop();

    if (next_local_advert && millisHasNowPassed(next_local_advert)) {
      mesh::Packet* pkt = createSelfAdvert();
      if (pkt) {
        sendZeroHop(pkt);
      }

      updateAdvertTimer();   // schedule next local advert
    }
  }

  void handleCommand(uint32_t sender_timestamp, const char* command, char reply[]) {
    while (*command == ' ') command++;   // skip leading spaces

    if (memcmp(command, "reboot", 6) == 0) {
      board.reboot();  // doesn't return
    } else if (memcmp(command, "advert", 6) == 0) {
      sendSelfAdvertisement(400);
      strcpy(reply, "OK - Advert sent");
    } else if (memcmp(command, "clock sync", 10) == 0) {
      uint32_t curr = getRTCClock()->getCurrentTime();
      if (sender_timestamp > curr) {
        getRTCClock()->setCurrentTime(sender_timestamp + 1);
        strcpy(reply, "OK - clock set");
      } else {
        strcpy(reply, "ERR: clock cannot go backwards");
      }
    } else if (memcmp(command, "start ota", 9) == 0) {
      if (_board->startOTAUpdate()) {
        strcpy(reply, "OK");
      } else {
        strcpy(reply, "Error");
      }
    } else if (memcmp(command, "clock", 5) == 0) {
      uint32_t now = getRTCClock()->getCurrentTime();
      DateTime dt = DateTime(now);
      sprintf(reply, "%02d:%02d - %d/%d/%d UTC", dt.hour(), dt.minute(), dt.day(), dt.month(), dt.year());
    } else if (memcmp(command, "time ", 5) == 0) {  // set time (to epoch seconds)
      uint32_t secs = _atoi(&command[5]);
      uint32_t curr = getRTCClock()->getCurrentTime();
      if (secs > curr) {
        getRTCClock()->setCurrentTime(secs);
        strcpy(reply, "(OK - clock set!)");
      } else {
        strcpy(reply, "(ERR: clock cannot go backwards)");
      }
    } else if (memcmp(command, "password ", 9) == 0) {
      // change admin password
      StrHelper::strncpy(_prefs.password, &command[9], sizeof(_prefs.password));
      checkAdvertInterval();
      savePrefs();
      sprintf(reply, "password now: %s", _prefs.password);   // echo back just to let admin know for sure!!
    } else if (memcmp(command, "set ", 4) == 0) {
      const char* config = &command[4];
      if (memcmp(config, "af ", 3) == 0) {
        _prefs.airtime_factor = atof(&config[3]);
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "advert.interval ", 16) == 0) {
        int mins = _atoi(&config[16]);
        if (mins > 0 && mins < MIN_LOCAL_ADVERT_INTERVAL) {
          sprintf(reply, "Error: min is %d mins", MIN_LOCAL_ADVERT_INTERVAL);
        } else if (mins > 240) {
          strcpy(reply, "Error: max is 240 mins");
        } else {
          _prefs.advert_interval = (uint8_t)(mins / 2);
          updateAdvertTimer();
          savePrefs();
          strcpy(reply, "OK");
        }
      } else if (memcmp(config, "guest.password ", 15) == 0) {
        StrHelper::strncpy(_prefs.guest_password, &config[15], sizeof(_prefs.guest_password));
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "name ", 5) == 0) {
        StrHelper::strncpy(_prefs.node_name, &config[5], sizeof(_prefs.node_name));
        checkAdvertInterval();
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "repeat ", 7) == 0) {
        _prefs.disable_fwd = memcmp(&config[7], "off", 3) == 0;
        savePrefs();
        strcpy(reply, _prefs.disable_fwd ? "OK - repeat is now OFF" : "OK - repeat is now ON");
      } else if (memcmp(config, "lat ", 4) == 0) {
        _prefs.node_lat = atof(&config[4]);
        checkAdvertInterval();
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "lon ", 4) == 0) {
        _prefs.node_lon = atof(&config[4]);
        checkAdvertInterval();
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "rxdelay ", 8) == 0) {
        float db = atof(&config[8]);
        if (db >= 0) {
          _prefs.rx_delay_base = db;
          savePrefs();
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Error, cannot be negative");
        }
      } else if (memcmp(config, "txdelay ", 8) == 0) {
        float f = atof(&config[8]);
        if (f >= 0) {
          _prefs.tx_delay_factor = f;
          savePrefs();
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Error, cannot be negative");
        }
      } else if (memcmp(config, "direct.txdelay ", 15) == 0) {
        float f = atof(&config[15]);
        if (f >= 0) {
          _prefs.direct_tx_delay_factor = f;
          savePrefs();
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Error, cannot be negative");
        }
      } else if (memcmp(config, "tx ", 3) == 0) {
        _prefs.tx_power_dbm = atoi(&config[3]);
        savePrefs();
        strcpy(reply, "OK - reboot to apply");
      } else if (sender_timestamp == 0 && memcmp(config, "freq ", 5) == 0) {
        _prefs.freq = atof(&config[5]);
        savePrefs();
        strcpy(reply, "OK - reboot to apply");
      } else {
        sprintf(reply, "unknown config: %s", config);
      }
    } else if (sender_timestamp == 0 && strcmp(command, "erase") == 0) {
  #if defined(NRF52_PLATFORM)
      bool s = InternalFS.format();
  #elif defined(ESP32)
      bool s = SPIFFS.format();
  #else
    #error "need to implement file system erase"
  #endif
      sprintf(reply, "File system erase: %s", s ? "OK" : "Err");
    } else if (memcmp(command, "ver", 3) == 0) {
      strcpy(reply, FIRMWARE_VER_TEXT);
    } else if (memcmp(command, "log start", 9) == 0) {
      _logging = true;
      strcpy(reply, "   logging on");
    } else if (memcmp(command, "log stop", 8) == 0) {
      _logging = false;
      strcpy(reply, "   logging off");
    } else if (memcmp(command, "log erase", 9) == 0) {
      _fs->remove(PACKET_LOG_FILE);
      strcpy(reply, "   log erased");
    } else if (sender_timestamp == 0 && memcmp(command, "log", 3) == 0) {
      File f = _fs->open(PACKET_LOG_FILE);
      if (f) {
        while (f.available()) {
          int c = f.read();
          if (c < 0) break;
          Serial.print((char)c);
        }
        f.close();
      }
      strcpy(reply, "   EOF");
    } else {
      sprintf(reply, "Unknown: %s (commands: reboot, advert, clock, set, ver, password, start ota)", command);
    }
  }
};

#if defined(NRF52_PLATFORM)
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);
#elif defined(P_LORA_SCLK)
SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif
StdRNG fast_rng;
SimpleMeshTables tables;

#ifdef ESP32
ESP32RTCClock fallback_clock;
#else
VolatileRTCClock fallback_clock; 
#endif
AutoDiscoverRTCClock rtc_clock(fallback_clock);

MyMesh the_mesh(board, *new WRAPPER_CLASS(radio, board), *new ArduinoMillis(), fast_rng, rtc_clock, tables);

void halt() {
  while (1) ;
}

static char command[80];

void setup() {
  Serial.begin(115200);
  delay(1000);

  board.begin();
#ifdef ESP32
  fallback_clock.begin();
#endif
  rtc_clock.begin(Wire);

#ifdef SX126X_DIO3_TCXO_VOLTAGE
  float tcxo = SX126X_DIO3_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif
  
#if defined(NRF52_PLATFORM)
  SPI.setPins(P_LORA_MISO, P_LORA_SCLK, P_LORA_MOSI);
  SPI.begin();
#elif defined(P_LORA_SCLK)
  spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI);
#endif
  int status = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    delay(5000);
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    halt();
  }

  radio.setCRC(0);

#ifdef SX126X_CURRENT_LIMIT
  radio.setCurrentLimit(SX126X_CURRENT_LIMIT);
#endif

#ifdef SX126X_DIO2_AS_RF_SWITCH
  radio.setDio2AsRfSwitch(SX126X_DIO2_AS_RF_SWITCH);
#endif

  fast_rng.begin(radio.random(0x7FFFFFFF));

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
    RadioNoiseListener rng(radio);
    the_mesh.self_id = mesh::LocalIdentity(&rng);  // create new random identity
    store.save("_main", the_mesh.self_id);
  }

  Serial.print("Repeater ID: ");
  mesh::Utils::printHex(Serial, the_mesh.self_id.pub_key, PUB_KEY_SIZE); Serial.println();

  command[0] = 0;

  the_mesh.begin(fs);

  if (LORA_FREQ != the_mesh.getFreqPref()) {
    radio.setFrequency(the_mesh.getFreqPref());
  }
  if (LORA_TX_POWER != the_mesh.getTxPowerPref()) {
    radio.setOutputPower(the_mesh.getTxPowerPref());
  }

  // send out initial Advertisement to the mesh
  the_mesh.sendSelfAdvertisement(2000);
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
    the_mesh.handleCommand(0, command, reply);  // NOTE: there is no sender_timestamp via serial!
    if (reply[0]) {
      Serial.print("  -> "); Serial.println(reply);
    }

    command[0] = 0;  // reset command buffer
  }

  the_mesh.loop();
}
