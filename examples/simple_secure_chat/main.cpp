#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#if defined(NRF52_PLATFORM)
  #include <InternalFileSystem.h>
#elif defined(ESP32)
  #include <SPIFFS.h>
#endif

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/RadioLibWrappers.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include <RTClib.h>

/* ---------------------------------- CONFIGURATION ------------------------------------- */

#define FIRMWARE_VER_TEXT   "v2 (build: 4 Feb 2025)"

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

#ifndef MAX_CONTACTS
  #define MAX_CONTACTS         100
#endif

#include <helpers/BaseChatMesh.h>

#define SEND_TIMEOUT_BASE_MILLIS          500
#define FLOOD_SEND_TIMEOUT_FACTOR         16.0f
#define DIRECT_SEND_PERHOP_FACTOR         6.0f
#define DIRECT_SEND_PERHOP_EXTRA_MILLIS   250

#define  PUBLIC_GROUP_PSK  "izOH6cXN6mrJ5e26oRXNcg=="

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
#elif defined(LILYGO_TLORA)
  #include <helpers/LilyGoTLoraBoard.h>
  #include <helpers/CustomSX1276Wrapper.h>
  static LilyGoTLoraBoard board;
#elif defined(RAK_4631)
  #include <helpers/nrf52/RAK4631Board.h>
  #include <helpers/CustomSX1262Wrapper.h>
  static RAK4631Board board;
#elif defined(T1000_E)
  #include <helpers/nrf52/T1000eBoard.h>
  #include <helpers/CustomLR1110Wrapper.h>
  static T1000eBoard board;
#elif defined(FAKETEC)
  #include <helpers/nrf52/faketecBoard.h>
  #include <helpers/CustomSX1262Wrapper.h>
  #include <helpers/CustomLLCC68Wrapper.h>
  static faketecBoard board;
#else
  #error "need to provide a 'board' object"
#endif

// Believe it or not, this std C function is busted on some platforms!
static uint32_t _atoi(const char* sp) {
  uint32_t n = 0;
  while (*sp && *sp >= '0' && *sp <= '9') {
    n *= 10;
    n += (*sp++ - '0');
  }
  return n;
}

/* -------------------------------------------------------------------------------------- */

struct NodePrefs {  // persisted to file
  float airtime_factor;
  char node_name[32];
  double node_lat, node_lon;
  float freq;
  uint8_t tx_power_dbm;
  uint8_t unused[3];
};

class MyMesh : public BaseChatMesh, ContactVisitor {
  FILESYSTEM* _fs;
  NodePrefs _prefs;
  uint32_t expected_ack_crc;
  ChannelDetails* _public;
  unsigned long last_msg_sent;
  ContactInfo* curr_recipient;
  char command[512+10];
  uint8_t tmp_buf[256];
  char hex_buf[512];

  const char* getTypeName(uint8_t type) const {
    if (type == ADV_TYPE_CHAT) return "Chat";
    if (type == ADV_TYPE_REPEATER) return "Repeater";
    if (type == ADV_TYPE_ROOM) return "Room";
    return "??";  // unknown
  }

  void loadContacts() {
    if (_fs->exists("/contacts")) {
      File file = _fs->open("/contacts");
      if (file) {
        bool full = false;
        while (!full) {
          ContactInfo c;
          uint8_t pub_key[32];
          uint8_t unused;
          uint32_t reserved;

          bool success = (file.read(pub_key, 32) == 32);
          success = success && (file.read((uint8_t *) &c.name, 32) == 32);
          success = success && (file.read(&c.type, 1) == 1);
          success = success && (file.read(&c.flags, 1) == 1);
          success = success && (file.read(&unused, 1) == 1);
          success = success && (file.read((uint8_t *) &reserved, 4) == 4);
          success = success && (file.read((uint8_t *) &c.out_path_len, 1) == 1);
          success = success && (file.read((uint8_t *) &c.last_advert_timestamp, 4) == 4);
          success = success && (file.read(c.out_path, 64) == 64);
          c.gps_lat = c.gps_lon = 0;   // not yet supported

          if (!success) break;  // EOF

          c.id = mesh::Identity(pub_key);
          c.lastmod = 0;
          if (!addContact(c)) full = true;
        }
        file.close();
      }
    }
  }

  void saveContacts() {
#if defined(NRF52_PLATFORM)
    File file = _fs->open("/contacts", FILE_O_WRITE);
    if (file) { file.seek(0); file.truncate(); }
#else
    File file = _fs->open("/contacts", "w", true);
#endif
    if (file) {
      ContactsIterator iter;
      ContactInfo c;
      uint8_t unused = 0;
      uint32_t reserved = 0;

      while (iter.hasNext(this, c)) {
        bool success = (file.write(c.id.pub_key, 32) == 32);
        success = success && (file.write((uint8_t *) &c.name, 32) == 32);
        success = success && (file.write(&c.type, 1) == 1);
        success = success && (file.write(&c.flags, 1) == 1);
        success = success && (file.write(&unused, 1) == 1);
        success = success && (file.write((uint8_t *) &reserved, 4) == 4);
        success = success && (file.write((uint8_t *) &c.out_path_len, 1) == 1);
        success = success && (file.write((uint8_t *) &c.last_advert_timestamp, 4) == 4);
        success = success && (file.write(c.out_path, 64) == 64);

        if (!success) break;  // write failed
      }
      file.close();
    }
  }

  void setClock(uint32_t timestamp) {
    uint32_t curr = getRTCClock()->getCurrentTime();
    if (timestamp > curr) {
      getRTCClock()->setCurrentTime(timestamp);
      Serial.println("   (OK - clock set!)");
    } else {
      Serial.println("   (ERR: clock cannot go backwards)");
    }
  }

  void importCard(const char* command) {
    while (*command == ' ') command++;   // skip leading spaces
    if (memcmp(command, "meshcore://", 11) == 0) {
      command += 11;  // skip the prefix
      char *ep = strchr(command, 0);  // find end of string
      while (ep > command) {
        ep--;
        if (mesh::Utils::isHexChar(*ep)) break;  // found tail end of card
        *ep = 0;  // remove trailing spaces and other junk
      }
      int len = strlen(command);
      if (len % 2 == 0) {
        len >>= 1;  // halve, for num bytes
        if (mesh::Utils::fromHex(tmp_buf, len, command)) {
          importContact(tmp_buf, len);
          return;
        }
      }
    }
    Serial.println("   error: invalid format");
  }

protected:
  float getAirtimeBudgetFactor() const override {
    return _prefs.airtime_factor;
  }

  int calcRxDelay(float score, uint32_t air_time) const override {
    return 0;  // disable rxdelay
  }

  void onDiscoveredContact(ContactInfo& contact, bool is_new) override {
    // TODO: if not in favs,  prompt to add as fav(?)

    Serial.printf("ADVERT from -> %s\n", contact.name);
    Serial.printf("  type: %s\n", getTypeName(contact.type));
    Serial.print("   public key: "); mesh::Utils::printHex(Serial, contact.id.pub_key, PUB_KEY_SIZE); Serial.println();

    saveContacts();
  }

  void onContactPathUpdated(const ContactInfo& contact) override {
    Serial.printf("PATH to: %s, path_len=%d\n", contact.name, (int32_t) contact.out_path_len);
    saveContacts();
  }

  bool processAck(const uint8_t *data) override {
    if (memcmp(data, &expected_ack_crc, 4) == 0) {     // got an ACK from recipient
      Serial.printf("   Got ACK! (round trip: %d millis)\n", _ms->getMillis() - last_msg_sent);
      // NOTE: the same ACK can be received multiple times!
      expected_ack_crc = 0;  // reset our expected hash, now that we have received ACK
      return true;
    }

    //uint32_t crc;
    //memcpy(&crc, data, 4);
    //MESH_DEBUG_PRINTLN("unknown ACK received: %08X (expected: %08X)", crc, expected_ack_crc);
    return false;
  }

  void onMessageRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const char *text) override {
    Serial.printf("(%s) MSG -> from %s\n", pkt->isRouteDirect() ? "DIRECT" : "FLOOD", from.name);
    Serial.printf("   %s\n", text);

    if (strcmp(text, "clock sync") == 0) {  // special text command
      setClock(sender_timestamp + 1);
    }
  }

  void onCommandDataRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const char *text) override {
  }
  void onSignedMessageRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const uint8_t *sender_prefix, const char *text) override {
  }

  void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char *text) override {
    if (pkt->isRouteDirect()) {
      Serial.printf("PUBLIC CHANNEL MSG -> (Direct!)\n");
    } else {
      Serial.printf("PUBLIC CHANNEL MSG -> (Flood) hops %d\n", pkt->path_len);
    }
    Serial.printf("   %s\n", text);
  }

  void onContactResponse(const ContactInfo& contact, const uint8_t* data, uint8_t len) override {
    // not supported
  }

  uint32_t calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const override {
    return SEND_TIMEOUT_BASE_MILLIS + (FLOOD_SEND_TIMEOUT_FACTOR * pkt_airtime_millis);
  }
  uint32_t calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const override {
    return SEND_TIMEOUT_BASE_MILLIS + 
         ( (pkt_airtime_millis*DIRECT_SEND_PERHOP_FACTOR + DIRECT_SEND_PERHOP_EXTRA_MILLIS) * (path_len + 1));
  }

  void onSendTimeout() override {
    Serial.println("   ERROR: timed out, no ACK.");
  }

public:

  MyMesh(RadioLibWrapper& radio, mesh::RNG& rng, mesh::RTCClock& rtc, SimpleMeshTables& tables)
     : BaseChatMesh(radio, *new ArduinoMillis(), rng, rtc, *new StaticPoolPacketManager(16), tables)
  {
    // defaults
    memset(&_prefs, 0, sizeof(_prefs));
    _prefs.airtime_factor = 2.0;    // one third
    strcpy(_prefs.node_name, "NONAME");
    _prefs.freq = LORA_FREQ;
    _prefs.tx_power_dbm = LORA_TX_POWER;

    command[0] = 0;
    curr_recipient = NULL;
  }

  float getFreqPref() const { return _prefs.freq; }
  uint8_t getTxPowerPref() const { return _prefs.tx_power_dbm; }

  void begin(FILESYSTEM& fs) {
    _fs = &fs;

    BaseChatMesh::begin();

  #if defined(NRF52_PLATFORM)
    IdentityStore store(fs, "");
  #else
    IdentityStore store(fs, "/identity");
  #endif
    if (!store.load("_main", self_id, _prefs.node_name, sizeof(_prefs.node_name))) {  // legacy: node_name was from identity file
      self_id = mesh::LocalIdentity(getRNG());  // create new random identity
      store.save("_main", self_id);
    }

    // load persisted prefs
    if (_fs->exists("/node_prefs")) {
      File file = _fs->open("/node_prefs");
      if (file) {
        file.read((uint8_t *) &_prefs, sizeof(_prefs));
        file.close();
      }
    }

    loadContacts();
    _public = addChannel("Public", PUBLIC_GROUP_PSK); // pre-configure Andy's public channel
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

  void showWelcome() {
    Serial.println("===== MeshCore Chat Terminal =====");
    Serial.println();
    Serial.printf("WELCOME  %s\n", _prefs.node_name);
    Serial.println("   (enter 'help' for basic commands)");
    Serial.println();
  }

  void sendSelfAdvert(int delay_millis) {
    auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
    if (pkt) {
      sendFlood(pkt, delay_millis);
    }
  }

  // ContactVisitor
  void onContactVisit(const ContactInfo& contact) override {
    Serial.printf("   %s - ", contact.name);
    char tmp[40];
    int32_t secs = contact.last_advert_timestamp - getRTCClock()->getCurrentTime();
    AdvertTimeHelper::formatRelativeTimeDiff(tmp, secs, false);
    Serial.println(tmp);
  }

  void handleCommand(const char* command) {
    while (*command == ' ') command++;  // skip leading spaces

    if (memcmp(command, "send ", 5) == 0) {
      if (curr_recipient) {
        const char *text = &command[5];
        uint32_t est_timeout;

        int result = sendMessage(*curr_recipient, getRTCClock()->getCurrentTime(), 0, text, expected_ack_crc, est_timeout);
        if (result == MSG_SEND_FAILED) {
          Serial.println("   ERROR: unable to send.");
        } else {
          last_msg_sent = _ms->getMillis();
          Serial.printf("   (message sent - %s)\n", result == MSG_SEND_SENT_FLOOD ? "FLOOD" : "DIRECT");
        }
      } else {
        Serial.println("   ERROR: no recipient selected (use 'to' cmd).");
      }
    } else if (memcmp(command, "public ", 7) == 0) {  // send GroupChannel msg
      uint8_t temp[5+MAX_TEXT_LEN+32];
      uint32_t timestamp = getRTCClock()->getCurrentTime();
      memcpy(temp, &timestamp, 4);   // mostly an extra blob to help make packet_hash unique
      temp[4] = 0;  // attempt and flags

      sprintf((char *) &temp[5], "%s: %s", _prefs.node_name, &command[7]);  // <sender>: <msg>
      temp[5 + MAX_TEXT_LEN] = 0;  // truncate if too long

      int len = strlen((char *) &temp[5]);
      auto pkt = createGroupDatagram(PAYLOAD_TYPE_GRP_TXT, _public->channel, temp, 5 + len);
      if (pkt) {
        sendFlood(pkt);
        Serial.println("   Sent.");
      } else {
        Serial.println("   ERROR: unable to send");
      }
    } else if (memcmp(command, "list", 4) == 0) {  // show Contact list, by most recent
      int n = 0;
      if (command[4] == ' ') {  // optional param, last 'N'
        n = atoi(&command[5]);
      }
      scanRecentContacts(n, this);
    } else if (strcmp(command, "clock") == 0) {    // show current time
      uint32_t now = getRTCClock()->getCurrentTime();
      DateTime dt = DateTime(now);
      Serial.printf(   "%02d:%02d - %d/%d/%d UTC\n", dt.hour(), dt.minute(), dt.day(), dt.month(), dt.year());
    } else if (memcmp(command, "time ", 5) == 0) {  // set time (to epoch seconds)
      uint32_t secs = _atoi(&command[5]);
      setClock(secs);
    } else if (memcmp(command, "to ", 3) == 0) {  // set current recipient
      curr_recipient = searchContactsByPrefix(&command[3]);
      if (curr_recipient) {
        Serial.printf("   Recipient %s now selected.\n", curr_recipient->name);
      } else {
        Serial.println("   Error: Name prefix not found.");
      }
    } else if (strcmp(command, "to") == 0) {    // show current recipient
      if (curr_recipient) {
         Serial.printf("   Current: %s\n", curr_recipient->name);
      } else {
         Serial.println("   Err: no recipient selected");
      }
    } else if (strcmp(command, "advert") == 0) {
      auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
      if (pkt) {
        sendZeroHop(pkt);
        Serial.println("   (advert sent, zero hop).");
      } else {
        Serial.println("   ERR: unable to send");
      }
    } else if (strcmp(command, "reset path") == 0) {
      if (curr_recipient) {
        resetPathTo(*curr_recipient);
        saveContacts();
        Serial.println("   Done.");
      }
    } else if (memcmp(command, "card", 4) == 0) {
      Serial.printf("Hello %s\n", _prefs.node_name);
      auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
      if (pkt) {
        uint8_t len =  pkt->writeTo(tmp_buf);
        releasePacket(pkt);  // undo the obtainNewPacket()

        mesh::Utils::toHex(hex_buf, tmp_buf, len);
        Serial.println("Your MeshCore biz card:");
        Serial.print("meshcore://"); Serial.println(hex_buf);
        Serial.println();
      } else {
        Serial.println("  Error");
      }
    } else if (memcmp(command, "import ", 7) == 0) {
      importCard(&command[7]);
    } else if (memcmp(command, "set ", 4) == 0) {
      const char* config = &command[4];
      if (memcmp(config, "af ", 3) == 0) {
        _prefs.airtime_factor = atof(&config[3]);
        savePrefs();
        Serial.println("  OK");
      } else if (memcmp(config, "name ", 5) == 0) {
        StrHelper::strncpy(_prefs.node_name, &config[5], sizeof(_prefs.node_name));
        savePrefs();
        Serial.println("  OK");
      } else if (memcmp(config, "lat ", 4) == 0) {
        _prefs.node_lat = atof(&config[4]);
        savePrefs();
        Serial.println("  OK");
      } else if (memcmp(config, "lon ", 4) == 0) {
        _prefs.node_lon = atof(&config[4]);
        savePrefs();
        Serial.println("  OK");
      } else if (memcmp(config, "tx ", 3) == 0) {
        _prefs.tx_power_dbm = atoi(&config[3]);
        savePrefs();
        Serial.println("  OK - reboot to apply");
      } else if (memcmp(config, "freq ", 5) == 0) {
        _prefs.freq = atof(&config[5]);
        savePrefs();
        Serial.println("  OK - reboot to apply");
      } else {
        Serial.printf("  ERROR: unknown config: %s\n", config);
      }
    } else if (memcmp(command, "ver", 3) == 0) {
      Serial.println(FIRMWARE_VER_TEXT);
    } else if (memcmp(command, "help", 4) == 0) {
      Serial.println("Commands:");
      Serial.println("   set {name|lat|lon|freq|tx|af} {value}");
      Serial.println("   card");
      Serial.println("   import {biz card}");
      Serial.println("   clock");
      Serial.println("   time <epoch-seconds>");
      Serial.println("   list {n}");
      Serial.println("   to <recipient name or prefix>");
      Serial.println("   to");
      Serial.println("   send <text>");
      Serial.println("   advert");
      Serial.println("   reset path");
      Serial.println("   public <text>");
    } else {
      Serial.print("   ERROR: unknown command: "); Serial.println(command);
    }
  }

  void loop() {
    BaseChatMesh::loop();

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

      handleCommand(command);
      command[0] = 0;  // reset command buffer
    }
  }
};

#if defined(NRF52_PLATFORM)
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, SPI);
#elif defined(LILYGO_TLORA)
SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_0, P_LORA_RESET, P_LORA_DIO_1, spi);
#elif defined(P_LORA_SCLK)
SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif
StdRNG fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(*new WRAPPER_CLASS(radio, board), fast_rng, *new VolatileRTCClock(), tables);

void halt() {
  while (1) ;
}

void setup() {
  Serial.begin(115200);

  board.begin();
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
    Serial.print("ERROR: radio init failed: ");
    Serial.println(status);
    halt();
  }

  radio.setCRC(1);

#ifdef SX126X_CURRENT_LIMIT
  radio.setCurrentLimit(SX126X_CURRENT_LIMIT);
#endif

#ifdef SX126X_DIO2_AS_RF_SWITCH
  radio.setDio2AsRfSwitch(SX126X_DIO2_AS_RF_SWITCH);
#endif

  fast_rng.begin(radio.random(0x7FFFFFFF));

#if defined(NRF52_PLATFORM)
  InternalFS.begin();
  the_mesh.begin(InternalFS);
#elif defined(ESP32)
  SPIFFS.begin(true);
  the_mesh.begin(SPIFFS);
#else
  #error "need to define filesystem"
#endif

  if (LORA_FREQ != the_mesh.getFreqPref()) {
    radio.setFrequency(the_mesh.getFreqPref());
  }
  if (LORA_TX_POWER != the_mesh.getTxPowerPref()) {
    radio.setOutputPower(the_mesh.getTxPowerPref());
  }

  the_mesh.showWelcome();

  // send out initial Advertisement to the mesh
  the_mesh.sendSelfAdvert(1200);   // add slight delay
}

void loop() {
  the_mesh.loop();
}
