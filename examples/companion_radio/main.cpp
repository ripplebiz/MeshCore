#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#if defined(NRF52_PLATFORM)
  #include <InternalFileSystem.h>
#elif defined(RP2040_PLATFORM)
  #include <LittleFS.h>
#elif defined(ESP32)
  #include <SPIFFS.h>
#endif

#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include <helpers/BaseSerialInterface.h>
#include "NodePrefs.h"
#include <RTClib.h>
#include <target.h>

/* ---------------------------------- CONFIGURATION ------------------------------------- */

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
#ifndef MAX_LORA_TX_POWER
  #define MAX_LORA_TX_POWER  LORA_TX_POWER
#endif

#ifndef MAX_CONTACTS
  #define MAX_CONTACTS         100
#endif

#ifndef OFFLINE_QUEUE_SIZE
  #define OFFLINE_QUEUE_SIZE  16
#endif

#ifndef BLE_NAME_PREFIX
  #define BLE_NAME_PREFIX  "MeshCore-"
#endif

#include <helpers/BaseChatMesh.h>

#define SEND_TIMEOUT_BASE_MILLIS          500
#define FLOOD_SEND_TIMEOUT_FACTOR         16.0f
#define DIRECT_SEND_PERHOP_FACTOR         6.0f
#define DIRECT_SEND_PERHOP_EXTRA_MILLIS   250

#define  PUBLIC_GROUP_PSK  "izOH6cXN6mrJ5e26oRXNcg=="

#ifdef DISPLAY_CLASS      // TODO: refactor this -- move to variants/*/target
  #include "UITask.h"
  #ifdef ST7735
    #include <helpers/ui/ST7735Display.h>
  #elif ST7789
    #include <helpers/ui/ST7789Display.h>
  #elif defined(HAS_GxEPD)
    #include <helpers/ui/GxEPDDisplay.h>
  #else
    #include <helpers/ui/SSD1306Display.h>
  #endif

  #if defined(HELTEC_LORA_V3) && defined(ST7735)
    static DISPLAY_CLASS display(&board.periph_power);   // peripheral power pin is shared
  #else
    static DISPLAY_CLASS display;
  #endif

  #define HAS_UI
#endif

#if defined(HAS_UI)
  #include "UITask.h"

  static UITask ui_task(&board);
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

/*------------ Frame Protocol --------------*/

#define FIRMWARE_VER_CODE    5

#ifndef FIRMWARE_BUILD_DATE
  #define FIRMWARE_BUILD_DATE   "9 May 2025"
#endif

#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION   "v1.6.0"
#endif

#define CMD_APP_START              1
#define CMD_SEND_TXT_MSG           2
#define CMD_SEND_CHANNEL_TXT_MSG   3
#define CMD_GET_CONTACTS           4   // with optional 'since' (for efficient sync)
#define CMD_GET_DEVICE_TIME        5
#define CMD_SET_DEVICE_TIME        6
#define CMD_SEND_SELF_ADVERT       7
#define CMD_SET_ADVERT_NAME        8
#define CMD_ADD_UPDATE_CONTACT     9
#define CMD_SYNC_NEXT_MESSAGE     10
#define CMD_SET_RADIO_PARAMS      11
#define CMD_SET_RADIO_TX_POWER    12
#define CMD_RESET_PATH            13
#define CMD_SET_ADVERT_LATLON     14
#define CMD_REMOVE_CONTACT        15
#define CMD_SHARE_CONTACT         16
#define CMD_EXPORT_CONTACT        17
#define CMD_IMPORT_CONTACT        18
#define CMD_REBOOT                19
#define CMD_GET_BATTERY_VOLTAGE   20
#define CMD_SET_TUNING_PARAMS     21
#define CMD_DEVICE_QEURY          22
#define CMD_EXPORT_PRIVATE_KEY    23
#define CMD_IMPORT_PRIVATE_KEY    24
#define CMD_SEND_RAW_DATA         25
#define CMD_SEND_LOGIN            26
#define CMD_SEND_STATUS_REQ       27
#define CMD_HAS_CONNECTION        28
#define CMD_LOGOUT                29   // 'Disconnect'
#define CMD_GET_CONTACT_BY_KEY    30
#define CMD_GET_CHANNEL           31
#define CMD_SET_CHANNEL           32
#define CMD_SIGN_START            33
#define CMD_SIGN_DATA             34
#define CMD_SIGN_FINISH           35
#define CMD_SEND_TRACE_PATH       36
#define CMD_SET_DEVICE_PIN        37
#define CMD_SET_OTHER_PARAMS      38
#define CMD_SEND_TELEMETRY_REQ    39
#define CMD_GET_CUSTOM_VARS       40
#define CMD_SET_CUSTOM_VAR        41

#define RESP_CODE_OK                0
#define RESP_CODE_ERR               1
#define RESP_CODE_CONTACTS_START    2   // first reply to CMD_GET_CONTACTS
#define RESP_CODE_CONTACT           3   // multiple of these (after CMD_GET_CONTACTS)
#define RESP_CODE_END_OF_CONTACTS   4   // last reply to CMD_GET_CONTACTS
#define RESP_CODE_SELF_INFO         5   // reply to CMD_APP_START
#define RESP_CODE_SENT              6   // reply to CMD_SEND_TXT_MSG
#define RESP_CODE_CONTACT_MSG_RECV  7   // a reply to CMD_SYNC_NEXT_MESSAGE (ver < 3)
#define RESP_CODE_CHANNEL_MSG_RECV  8   // a reply to CMD_SYNC_NEXT_MESSAGE (ver < 3)
#define RESP_CODE_CURR_TIME         9   // a reply to CMD_GET_DEVICE_TIME
#define RESP_CODE_NO_MORE_MESSAGES 10   // a reply to CMD_SYNC_NEXT_MESSAGE
#define RESP_CODE_EXPORT_CONTACT   11
#define RESP_CODE_BATTERY_VOLTAGE  12   // a reply to a CMD_GET_BATTERY_VOLTAGE
#define RESP_CODE_DEVICE_INFO      13   // a reply to CMD_DEVICE_QEURY
#define RESP_CODE_PRIVATE_KEY      14   // a reply to CMD_EXPORT_PRIVATE_KEY
#define RESP_CODE_DISABLED         15
#define RESP_CODE_CONTACT_MSG_RECV_V3  16   // a reply to CMD_SYNC_NEXT_MESSAGE (ver >= 3)
#define RESP_CODE_CHANNEL_MSG_RECV_V3  17   // a reply to CMD_SYNC_NEXT_MESSAGE (ver >= 3)
#define RESP_CODE_CHANNEL_INFO     18   // a reply to CMD_GET_CHANNEL
#define RESP_CODE_SIGN_START       19
#define RESP_CODE_SIGNATURE        20
#define RESP_CODE_CUSTOM_VARS      21

// these are _pushed_ to client app at any time
#define PUSH_CODE_ADVERT            0x80
#define PUSH_CODE_PATH_UPDATED      0x81
#define PUSH_CODE_SEND_CONFIRMED    0x82
#define PUSH_CODE_MSG_WAITING       0x83
#define PUSH_CODE_RAW_DATA          0x84
#define PUSH_CODE_LOGIN_SUCCESS     0x85
#define PUSH_CODE_LOGIN_FAIL        0x86
#define PUSH_CODE_STATUS_RESPONSE   0x87
#define PUSH_CODE_LOG_RX_DATA       0x88
#define PUSH_CODE_TRACE_DATA        0x89
#define PUSH_CODE_NEW_ADVERT        0x8A
#define PUSH_CODE_TELEMETRY_RESPONSE  0x8B

#define ERR_CODE_UNSUPPORTED_CMD      1
#define ERR_CODE_NOT_FOUND            2
#define ERR_CODE_TABLE_FULL           3
#define ERR_CODE_BAD_STATE            4
#define ERR_CODE_FILE_IO_ERROR        5
#define ERR_CODE_ILLEGAL_ARG          6

/* -------------------------------------------------------------------------------------- */

#define REQ_TYPE_GET_STATUS          0x01   // same as _GET_STATS
#define REQ_TYPE_KEEP_ALIVE          0x02
#define REQ_TYPE_GET_TELEMETRY_DATA  0x03

#define MAX_SIGN_DATA_LEN    (8*1024)   // 8K

class MyMesh : public BaseChatMesh {
  FILESYSTEM* _fs;
  IdentityStore* _identity_store;
  NodePrefs _prefs;
  uint32_t pending_login;
  uint32_t pending_status;
  uint32_t pending_telemetry;
  BaseSerialInterface* _serial;
  ContactsIterator _iter;
  uint32_t _iter_filter_since;
  uint32_t _most_recent_lastmod;
  uint32_t _active_ble_pin;
  bool  _iter_started;
  uint8_t app_target_ver;
  uint8_t* sign_data;
  uint32_t sign_data_len;
  uint8_t cmd_frame[MAX_FRAME_SIZE+1];
  uint8_t out_frame[MAX_FRAME_SIZE+1];
  CayenneLPP telemetry;

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };
  int offline_queue_len;
  Frame offline_queue[OFFLINE_QUEUE_SIZE];

  struct AckTableEntry {
    unsigned long msg_sent;
    uint32_t ack;
  };
  #define EXPECTED_ACK_TABLE_SIZE   8
  AckTableEntry  expected_ack_table[EXPECTED_ACK_TABLE_SIZE];  // circular table
  int next_ack_idx;

  void loadMainIdentity() {
    if (!_identity_store->load("_main", self_id)) {
      self_id = radio_new_identity();  // create new random identity
      int count = 0;
      while (count < 10 && (self_id.pub_key[0] == 0x00 || self_id.pub_key[0] == 0xFF)) {  // reserved id hashes
        self_id = radio_new_identity(); count++;
      }
      saveMainIdentity(self_id);
    }
  }

  bool saveMainIdentity(const mesh::LocalIdentity& identity) {
    return _identity_store->save("_main", identity);
  }

  void loadContacts() {
    if (_fs->exists("/contacts3")) {
    #if defined(RP2040_PLATFORM)
      File file = _fs->open("/contacts3", "r");
    #else
      File file = _fs->open("/contacts3");
    #endif
      if (file) {
        bool full = false;
        while (!full) {
          ContactInfo c;
          uint8_t pub_key[32];
          uint8_t unused;

          bool success = (file.read(pub_key, 32) == 32);
          success = success && (file.read((uint8_t *) &c.name, 32) == 32);
          success = success && (file.read(&c.type, 1) == 1);
          success = success && (file.read(&c.flags, 1) == 1);
          success = success && (file.read(&unused, 1) == 1);
          success = success && (file.read((uint8_t *) &c.sync_since, 4) == 4);  // was 'reserved'
          success = success && (file.read((uint8_t *) &c.out_path_len, 1) == 1);
          success = success && (file.read((uint8_t *) &c.last_advert_timestamp, 4) == 4);
          success = success && (file.read(c.out_path, 64) == 64);
          success = success && (file.read((uint8_t *) &c.lastmod, 4) == 4);
          success = success && (file.read((uint8_t *) &c.gps_lat, 4) == 4);
          success = success && (file.read((uint8_t *) &c.gps_lon, 4) == 4);

          if (!success) break;  // EOF

          c.id = mesh::Identity(pub_key);
          if (!addContact(c)) full = true;
        }
        file.close();
      }
    }
  }

  void saveContacts() {
#if defined(NRF52_PLATFORM)
    File file = _fs->open("/contacts3", FILE_O_WRITE);
    if (file) { file.seek(0); file.truncate(); }
#elif defined(RP2040_PLATFORM)
    File file = _fs->open("/contacts3", "w");
#else
    File file = _fs->open("/contacts3", "w", true);
#endif
    if (file) {
      ContactsIterator iter;
      ContactInfo c;
      uint8_t unused = 0;

      while (iter.hasNext(this, c)) {
        bool success = (file.write(c.id.pub_key, 32) == 32);
        success = success && (file.write((uint8_t *) &c.name, 32) == 32);
        success = success && (file.write(&c.type, 1) == 1);
        success = success && (file.write(&c.flags, 1) == 1);
        success = success && (file.write(&unused, 1) == 1);
        success = success && (file.write((uint8_t *) &c.sync_since, 4) == 4);
        success = success && (file.write((uint8_t *) &c.out_path_len, 1) == 1);
        success = success && (file.write((uint8_t *) &c.last_advert_timestamp, 4) == 4);
        success = success && (file.write(c.out_path, 64) == 64);
        success = success && (file.write((uint8_t *) &c.lastmod, 4) == 4);
        success = success && (file.write((uint8_t *) &c.gps_lat, 4) == 4);
        success = success && (file.write((uint8_t *) &c.gps_lon, 4) == 4);

        if (!success) break;  // write failed
      }
      file.close();
    }
  }

  void loadChannels() {
    if (_fs->exists("/channels2")) {
    #if defined(RP2040_PLATFORM)
    File file = _fs->open("/channels2", "r");
    #else
      File file = _fs->open("/channels2");
    #endif
      if (file) {
        bool full = false;
        uint8_t channel_idx = 0;
        while (!full) {
          ChannelDetails ch;
          uint8_t unused[4];

          bool success = (file.read(unused, 4) == 4);
          success = success && (file.read((uint8_t *) ch.name, 32) == 32);
          success = success && (file.read((uint8_t *) ch.channel.secret, 32) == 32);

          if (!success) break;  // EOF

          if (setChannel(channel_idx, ch)) {
            channel_idx++;
          } else {
            full = true;
          }
        }
        file.close();
      }
    }
  }

  void saveChannels() {
  #if defined(NRF52_PLATFORM)
    File file = _fs->open("/channels2", FILE_O_WRITE);
    if (file) { file.seek(0); file.truncate(); }
  #elif defined(RP2040_PLATFORM)
    File file = _fs->open("/channels2", "w");
  #else
    File file = _fs->open("/channels2", "w", true);
  #endif
    if (file) {
      uint8_t channel_idx = 0;
      ChannelDetails ch;
      uint8_t unused[4];
      memset(unused, 0, 4);
    
      while (getChannel(channel_idx, ch)) {
        bool success = (file.write(unused, 4) == 4);
        success = success && (file.write((uint8_t *) ch.name, 32) == 32);
        success = success && (file.write((uint8_t *) ch.channel.secret, 32) == 32);
    
        if (!success) break;  // write failed
        channel_idx++;
      }
      file.close();
    }
  }
    
  int  getBlobByKey(const uint8_t key[], int key_len, uint8_t dest_buf[]) override {
    char path[64];
    char fname[18];

    if (key_len > 8) key_len = 8;   // just use first 8 bytes (prefix)
    mesh::Utils::toHex(fname, key, key_len);
    sprintf(path, "/bl/%s", fname);

    if (_fs->exists(path)) {
    #if defined(RP2040_PLATFORM)
      File f = _fs->open(path, "r");
    #else
      File f = _fs->open(path);
    #endif
      if (f) {
        int len = f.read(dest_buf, 255);  // currently MAX 255 byte blob len supported!!
        f.close();
        return len;
      }
    }
    return 0;   // not found
  }

  bool putBlobByKey(const uint8_t key[], int key_len, const uint8_t src_buf[], int len) override {
    char path[64];
    char fname[18];

    if (key_len > 8) key_len = 8;   // just use first 8 bytes (prefix)
    mesh::Utils::toHex(fname, key, key_len);
    sprintf(path, "/bl/%s", fname);

  #if defined(NRF52_PLATFORM)
    File f = _fs->open(path, FILE_O_WRITE);
    if (f) { f.seek(0); f.truncate(); }
  #elif defined(RP2040_PLATFORM)
    File f = _fs->open(path, "w");
  #else
    File f = _fs->open(path, "w", true);
  #endif
    if (f) {
      int n = f.write(src_buf, len);
      f.close();
      if (n == len) return true;  // success!

      _fs->remove(path);   // blob was only partially written!
    }
    return false;  // error
  }

  void writeOKFrame() {
    uint8_t buf[1];
    buf[0] = RESP_CODE_OK;
    _serial->writeFrame(buf, 1);
  }
  void writeErrFrame(uint8_t err_code) {
    uint8_t buf[2];
    buf[0] = RESP_CODE_ERR;
    buf[1] = err_code;
    _serial->writeFrame(buf, 2);
  }

  void writeDisabledFrame() {
    uint8_t buf[1];
    buf[0] = RESP_CODE_DISABLED;
    _serial->writeFrame(buf, 1);
  }

  void writeContactRespFrame(uint8_t code, const ContactInfo& contact) {
    int i = 0;
    out_frame[i++] = code;
    memcpy(&out_frame[i], contact.id.pub_key, PUB_KEY_SIZE); i += PUB_KEY_SIZE;
    out_frame[i++] = contact.type;
    out_frame[i++] = contact.flags;
    out_frame[i++] = contact.out_path_len;
    memcpy(&out_frame[i], contact.out_path, MAX_PATH_SIZE); i += MAX_PATH_SIZE;
    StrHelper::strzcpy((char *) &out_frame[i], contact.name, 32); i += 32;
    memcpy(&out_frame[i], &contact.last_advert_timestamp, 4); i += 4;
    memcpy(&out_frame[i], &contact.gps_lat, 4); i += 4;
    memcpy(&out_frame[i], &contact.gps_lon, 4); i += 4;
    memcpy(&out_frame[i], &contact.lastmod, 4); i += 4;
    _serial->writeFrame(out_frame, i);
  }

  void updateContactFromFrame(ContactInfo& contact, const uint8_t* frame, int len) {
    int i = 0;
    uint8_t code = frame[i++];  // eg. CMD_ADD_UPDATE_CONTACT
    memcpy(contact.id.pub_key, &frame[i], PUB_KEY_SIZE); i += PUB_KEY_SIZE;
    contact.type = frame[i++];
    contact.flags = frame[i++];
    contact.out_path_len = frame[i++];
    memcpy(contact.out_path, &frame[i], MAX_PATH_SIZE); i += MAX_PATH_SIZE;
    memcpy(contact.name, &frame[i], 32); i += 32;
    memcpy(&contact.last_advert_timestamp, &frame[i], 4); i += 4;
    if (i + 8 >= len) {  // optional fields
      memcpy(&contact.gps_lat, &frame[i], 4); i += 4;
      memcpy(&contact.gps_lon, &frame[i], 4); i += 4;
    }
  }

  void addToOfflineQueue(const uint8_t frame[], int len) {
    if (offline_queue_len >= OFFLINE_QUEUE_SIZE) {
      MESH_DEBUG_PRINTLN("ERROR: offline_queue is full!");
    } else {
      offline_queue[offline_queue_len].len = len;
      memcpy(offline_queue[offline_queue_len].buf, frame, len);
      offline_queue_len++;
    }
  }
  int getFromOfflineQueue(uint8_t frame[]) {
    if (offline_queue_len > 0) {   // check offline queue
      size_t len = offline_queue[0].len;   // take from top of queue
      memcpy(frame, offline_queue[0].buf, len);

      offline_queue_len--;
      for (int i = 0; i < offline_queue_len; i++) {   // delete top item from queue
        offline_queue[i] = offline_queue[i + 1];
      }
      return len;
    }
    return 0;  // queue is empty
  }

  void soundBuzzer() {
    // TODO
  }

protected:
  float getAirtimeBudgetFactor() const override {
    return _prefs.airtime_factor;
  }

  int calcRxDelay(float score, uint32_t air_time) const override {
    if (_prefs.rx_delay_base <= 0.0f) return 0;
    return (int) ((pow(_prefs.rx_delay_base, 0.85f - score) - 1.0) * air_time);
  }

  void logRxRaw(float snr, float rssi, const uint8_t raw[], int len) override {
    if (_serial->isConnected() && len+3 <= MAX_FRAME_SIZE) {
      int i = 0;
      out_frame[i++] = PUSH_CODE_LOG_RX_DATA;
      out_frame[i++] = (int8_t)(snr * 4);
      out_frame[i++] = (int8_t)(rssi);
      memcpy(&out_frame[i], raw, len); i += len;

      _serial->writeFrame(out_frame, i);
    }
  }

  bool isAutoAddEnabled() const override {
    return (_prefs.manual_add_contacts & 1) == 0;
  }

  void onDiscoveredContact(ContactInfo& contact, bool is_new) override {
    if (_serial->isConnected()) {
      if (!isAutoAddEnabled() && is_new) {
        writeContactRespFrame(PUSH_CODE_NEW_ADVERT, contact);
      } else {
        out_frame[0] = PUSH_CODE_ADVERT;
        memcpy(&out_frame[1], contact.id.pub_key, PUB_KEY_SIZE);
        _serial->writeFrame(out_frame, 1 + PUB_KEY_SIZE);
      }
    } else {
      soundBuzzer();
    }

    saveContacts();
  }

  void onContactPathUpdated(const ContactInfo& contact) override {
    out_frame[0] = PUSH_CODE_PATH_UPDATED;
    memcpy(&out_frame[1], contact.id.pub_key, PUB_KEY_SIZE);
    _serial->writeFrame(out_frame, 1 + PUB_KEY_SIZE);   // NOTE: app may not be connected

    saveContacts();
  }

  bool processAck(const uint8_t *data) override {
    // see if matches any in a table
    for (int i = 0; i < EXPECTED_ACK_TABLE_SIZE; i++) {
      if (memcmp(data, &expected_ack_table[i].ack, 4) == 0) {     // got an ACK from recipient
        out_frame[0] = PUSH_CODE_SEND_CONFIRMED;
        memcpy(&out_frame[1], data, 4);
        uint32_t trip_time = _ms->getMillis() - expected_ack_table[i].msg_sent;
        memcpy(&out_frame[5], &trip_time, 4);
        _serial->writeFrame(out_frame, 9);

        // NOTE: the same ACK can be received multiple times!
        expected_ack_table[i].ack = 0;  // clear expected hash, now that we have received ACK
        return true;
      }
    }
    return checkConnectionsAck(data);
  }

  void queueMessage(const ContactInfo& from, uint8_t txt_type, mesh::Packet* pkt, uint32_t sender_timestamp, const uint8_t* extra, int extra_len, const char *text) {
    int i = 0;
    if (app_target_ver >= 3) {
      out_frame[i++] = RESP_CODE_CONTACT_MSG_RECV_V3;
      out_frame[i++] = (int8_t)(pkt->getSNR() * 4);
      out_frame[i++] = 0;  // reserved1
      out_frame[i++] = 0;  // reserved2
    } else {
      out_frame[i++] = RESP_CODE_CONTACT_MSG_RECV;
    }
    memcpy(&out_frame[i], from.id.pub_key, 6); i += 6;  // just 6-byte prefix
    uint8_t path_len = out_frame[i++] = pkt->isRouteFlood() ? pkt->path_len : 0xFF;
    out_frame[i++] = txt_type;
    memcpy(&out_frame[i], &sender_timestamp, 4); i += 4;
    if (extra_len > 0) {
      memcpy(&out_frame[i], extra, extra_len); i += extra_len;
    }
    int tlen = strlen(text);   // TODO: UTF-8 ??
    if (i + tlen > MAX_FRAME_SIZE) {
      tlen = MAX_FRAME_SIZE - i;
    }
    memcpy(&out_frame[i], text, tlen); i += tlen;
    addToOfflineQueue(out_frame, i);

    if (_serial->isConnected()) {
      uint8_t frame[1];
      frame[0] = PUSH_CODE_MSG_WAITING;  // send push 'tickle'
      _serial->writeFrame(frame, 1);
    } else {
      soundBuzzer();
    }
  #ifdef HAS_UI
    ui_task.newMsg(path_len, from.name, text, offline_queue_len);
  #endif
  }

  void onMessageRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const char *text) override {
    markConnectionActive(from);   // in case this is from a server, and we have a connection
    queueMessage(from, TXT_TYPE_PLAIN, pkt, sender_timestamp, NULL, 0, text);
  }

  void onCommandDataRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const char *text) override {
    markConnectionActive(from);   // in case this is from a server, and we have a connection
    queueMessage(from, TXT_TYPE_CLI_DATA, pkt, sender_timestamp, NULL, 0, text);
  }

  void onSignedMessageRecv(const ContactInfo& from, mesh::Packet* pkt, uint32_t sender_timestamp, const uint8_t *sender_prefix, const char *text) override {
    markConnectionActive(from);
    saveContacts();   // from.sync_since change needs to be persisted
    queueMessage(from, TXT_TYPE_SIGNED_PLAIN, pkt, sender_timestamp, sender_prefix, 4, text);
  }

  void onChannelMessageRecv(const mesh::GroupChannel& channel, mesh::Packet* pkt, uint32_t timestamp, const char *text) override {
    int i = 0;
    if (app_target_ver >= 3) {
      out_frame[i++] = RESP_CODE_CHANNEL_MSG_RECV_V3;
      out_frame[i++] = (int8_t)(pkt->getSNR() * 4);
      out_frame[i++] = 0;  // reserved1
      out_frame[i++] = 0;  // reserved2
    } else {
      out_frame[i++] = RESP_CODE_CHANNEL_MSG_RECV;
    }

    out_frame[i++] = findChannelIdx(channel);
    uint8_t path_len = out_frame[i++] = pkt->isRouteFlood() ? pkt->path_len : 0xFF;

    out_frame[i++] = TXT_TYPE_PLAIN;
    memcpy(&out_frame[i], &timestamp, 4); i += 4;
    int tlen = strlen(text);   // TODO: UTF-8 ??
    if (i + tlen > MAX_FRAME_SIZE) {
      tlen = MAX_FRAME_SIZE - i;
    }
    memcpy(&out_frame[i], text, tlen); i += tlen;
    addToOfflineQueue(out_frame, i);

    if (_serial->isConnected()) {
      uint8_t frame[1];
      frame[0] = PUSH_CODE_MSG_WAITING;  // send push 'tickle'
      _serial->writeFrame(frame, 1);
    } else {
      soundBuzzer();
    }
  #ifdef HAS_UI
    ui_task.newMsg(path_len, "Public", text, offline_queue_len);
  #endif
  }

  uint8_t onContactRequest(const ContactInfo& contact, uint32_t sender_timestamp, const uint8_t* data, uint8_t len, uint8_t* reply) override {
    if (data[0] == REQ_TYPE_GET_TELEMETRY_DATA) {
      uint8_t permissions = 0;
      uint8_t cp = contact.flags >> 1;   // LSB used as 'favourite' bit (so only use upper bits)

      if (_prefs.telemetry_mode_base == TELEM_MODE_ALLOW_ALL) {
        permissions = TELEM_PERM_BASE;
      } else if (_prefs.telemetry_mode_base == TELEM_MODE_ALLOW_FLAGS) {
        permissions = cp & TELEM_PERM_BASE;
      }

      if (_prefs.telemetry_mode_loc == TELEM_MODE_ALLOW_ALL) {
        permissions |= TELEM_PERM_LOCATION;
      } else if (_prefs.telemetry_mode_loc == TELEM_MODE_ALLOW_FLAGS) {
        permissions |= cp & TELEM_PERM_LOCATION;
      }

      if (permissions & TELEM_PERM_BASE) {   // only respond if base permission bit is set
        telemetry.reset();
        telemetry.addVoltage(TELEM_CHANNEL_SELF, (float)board.getBattMilliVolts() / 1000.0f);
        // query other sensors -- target specific
        sensors.querySensors(permissions, telemetry);

        memcpy(reply, &sender_timestamp, 4);   // reflect sender_timestamp back in response packet (kind of like a 'tag')

        uint8_t tlen = telemetry.getSize();
        memcpy(&reply[4], telemetry.getBuffer(), tlen);
        return 4 + tlen;
      }
    }
    return 0;  // unknown
  }

  void onContactResponse(const ContactInfo& contact, const uint8_t* data, uint8_t len) override {
    uint32_t tag;
    memcpy(&tag, data, 4);

    if (pending_login && memcmp(&pending_login, contact.id.pub_key, 4) == 0) { // check for login response
      // yes, is response to pending sendLogin()
      pending_login = 0;

      int i = 0;
      if (memcmp(&data[4], "OK", 2) == 0) {    // legacy Repeater login OK response
        out_frame[i++] = PUSH_CODE_LOGIN_SUCCESS;
        out_frame[i++] = 0;  // legacy: is_admin = false
      } else if (data[4] == RESP_SERVER_LOGIN_OK) {   // new login response
        uint16_t keep_alive_secs = ((uint16_t)data[5]) * 16;
        if (keep_alive_secs > 0) {
          startConnection(contact, keep_alive_secs);
        }
        out_frame[i++] = PUSH_CODE_LOGIN_SUCCESS;
        out_frame[i++] = data[6];  // permissions (eg. is_admin)
      } else {
        out_frame[i++] = PUSH_CODE_LOGIN_FAIL;
        out_frame[i++] = 0;  // reserved
      }
      memcpy(&out_frame[i], contact.id.pub_key, 6); i += 6;  // pub_key_prefix
      _serial->writeFrame(out_frame, i);
    } else if (len > 4 &&   // check for status response
      pending_status && memcmp(&pending_status, contact.id.pub_key, 4) == 0   // legacy matching scheme
      // FUTURE: tag == pending_status
    ) {
      pending_status = 0;

      int i = 0;
      out_frame[i++] = PUSH_CODE_STATUS_RESPONSE;
      out_frame[i++] = 0;  // reserved
      memcpy(&out_frame[i], contact.id.pub_key, 6); i += 6;  // pub_key_prefix
      memcpy(&out_frame[i], &data[4], len - 4); i += (len - 4);
      _serial->writeFrame(out_frame, i);
    } else if (len > 4 && tag == pending_telemetry) {  // check for telemetry response
      pending_telemetry = 0;

      int i = 0;
      out_frame[i++] = PUSH_CODE_TELEMETRY_RESPONSE;
      out_frame[i++] = 0;  // reserved
      memcpy(&out_frame[i], contact.id.pub_key, 6); i += 6;  // pub_key_prefix
      memcpy(&out_frame[i], &data[4], len - 4); i += (len - 4);
      _serial->writeFrame(out_frame, i);
    }
  }

  void onRawDataRecv(mesh::Packet* packet) override {
    if (packet->payload_len + 4 > sizeof(out_frame)) {
      MESH_DEBUG_PRINTLN("onRawDataRecv(), payload_len too long: %d", packet->payload_len);
      return;
    }
    int i = 0;
    out_frame[i++] = PUSH_CODE_RAW_DATA;
    out_frame[i++] = (int8_t)(_radio->getLastSNR() * 4);
    out_frame[i++] = (int8_t)(_radio->getLastRSSI());
    out_frame[i++] = 0xFF;   // reserved (possibly path_len in future)
    memcpy(&out_frame[i], packet->payload, packet->payload_len); i += packet->payload_len;

    if (_serial->isConnected()) {
      _serial->writeFrame(out_frame, i);
    } else {
      MESH_DEBUG_PRINTLN("onRawDataRecv(), data received while app offline");
    }
  }

  void onTraceRecv(mesh::Packet* packet, uint32_t tag, uint32_t auth_code, uint8_t flags, const uint8_t* path_snrs, const uint8_t* path_hashes, uint8_t path_len) override {
    int i = 0;
    out_frame[i++] = PUSH_CODE_TRACE_DATA;
    out_frame[i++] = 0;   // reserved
    out_frame[i++] = path_len;
    out_frame[i++] = flags;
    memcpy(&out_frame[i], &tag, 4); i += 4;
    memcpy(&out_frame[i], &auth_code, 4); i += 4;
    memcpy(&out_frame[i], path_hashes, path_len); i += path_len;
    memcpy(&out_frame[i], path_snrs, path_len); i += path_len;
    out_frame[i++] = (int8_t)(packet->getSNR() * 4);   // extra/final SNR (to this node)

    if (_serial->isConnected()) {
      _serial->writeFrame(out_frame, i);
    } else {
      MESH_DEBUG_PRINTLN("onTraceRecv(), data received while app offline");
    }
  }

  uint32_t calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const override {
    return SEND_TIMEOUT_BASE_MILLIS + (FLOOD_SEND_TIMEOUT_FACTOR * pkt_airtime_millis);
  }
  uint32_t calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const override {
    return SEND_TIMEOUT_BASE_MILLIS +
         ( (pkt_airtime_millis*DIRECT_SEND_PERHOP_FACTOR + DIRECT_SEND_PERHOP_EXTRA_MILLIS) * (path_len + 1));
  }

  void onSendTimeout() override {
  }

public:

  MyMesh(mesh::Radio& radio, mesh::RNG& rng, mesh::RTCClock& rtc, SimpleMeshTables& tables)
     : BaseChatMesh(radio, *new ArduinoMillis(), rng, rtc, *new StaticPoolPacketManager(16), tables), _serial(NULL),
       telemetry(MAX_PACKET_PAYLOAD - 4)
  {
    _iter_started = false;
    offline_queue_len = 0;
    app_target_ver = 0;
    _identity_store = NULL;
    pending_login = pending_status = pending_telemetry = 0;
    next_ack_idx = 0;
    sign_data = NULL;

    // defaults
    memset(&_prefs, 0, sizeof(_prefs));
    _prefs.airtime_factor = 1.0;    // one half
    strcpy(_prefs.node_name, "NONAME");
    _prefs.freq = LORA_FREQ;
    _prefs.sf = LORA_SF;
    _prefs.bw = LORA_BW;
    _prefs.cr = LORA_CR;
    _prefs.tx_power_dbm = LORA_TX_POWER;
    //_prefs.rx_delay_base = 10.0f;  enable once new algo fixed
  }

  void loadPrefsInt(const char* filename) {
#if defined(RP2040_PLATFORM)
    File file = _fs->open(filename, "r");
#else
    File file = _fs->open(filename);
#endif
    if (file) {
      uint8_t pad[8];

      file.read((uint8_t *) &_prefs.airtime_factor, sizeof(float));  // 0
      file.read((uint8_t *) _prefs.node_name, sizeof(_prefs.node_name));  // 4
      file.read(pad, 4);   // 36
      file.read((uint8_t *) &sensors.node_lat, sizeof(sensors.node_lat));  // 40
      file.read((uint8_t *) &sensors.node_lon, sizeof(sensors.node_lon));  // 48
      file.read((uint8_t *) &_prefs.freq, sizeof(_prefs.freq));   // 56
      file.read((uint8_t *) &_prefs.sf, sizeof(_prefs.sf));  // 60
      file.read((uint8_t *) &_prefs.cr, sizeof(_prefs.cr));  // 61
      file.read((uint8_t *) &_prefs.reserved1, sizeof(_prefs.reserved1));  // 62
      file.read((uint8_t *) &_prefs.manual_add_contacts, sizeof(_prefs.manual_add_contacts));  // 63
      file.read((uint8_t *) &_prefs.bw, sizeof(_prefs.bw));  // 64
      file.read((uint8_t *) &_prefs.tx_power_dbm, sizeof(_prefs.tx_power_dbm));  // 68
      file.read((uint8_t *) &_prefs.telemetry_mode_base, sizeof(_prefs.telemetry_mode_base));  // 69
      file.read((uint8_t *) &_prefs.telemetry_mode_loc, sizeof(_prefs.telemetry_mode_loc));  // 70
      file.read(pad, 1);  // 71
      file.read((uint8_t *) &_prefs.rx_delay_base, sizeof(_prefs.rx_delay_base));  // 72
      file.read(pad, 4);   // 76
      file.read((uint8_t *) &_prefs.ble_pin, sizeof(_prefs.ble_pin));  // 80

      // sanitise bad pref values
      _prefs.rx_delay_base = constrain(_prefs.rx_delay_base, 0, 20.0f);
      _prefs.airtime_factor = constrain(_prefs.airtime_factor, 0, 9.0f);
      _prefs.freq = constrain(_prefs.freq, 400.0f, 2500.0f);
      _prefs.bw = constrain(_prefs.bw, 62.5f, 500.0f);
      _prefs.sf = constrain(_prefs.sf, 7, 12);
      _prefs.cr = constrain(_prefs.cr, 5, 8);
      _prefs.tx_power_dbm = constrain(_prefs.tx_power_dbm, 1, MAX_LORA_TX_POWER);

      file.close();
    }
  }

  void begin(FILESYSTEM& fs, bool has_display) {
    _fs = &fs;

    BaseChatMesh::begin();

  #if defined(NRF52_PLATFORM)
    _identity_store = new IdentityStore(fs, "");
  #elif defined(RP2040_PLATFORM)
    _identity_store = new IdentityStore(fs, "/identity");
    _identity_store->begin();
  #else
    _identity_store = new IdentityStore(fs, "/identity");
  #endif

    loadMainIdentity();

    // use hex of first 4 bytes of identity public key as default node name
    char pub_key_hex[10];
    mesh::Utils::toHex(pub_key_hex, self_id.pub_key, 4);
    strcpy(_prefs.node_name, pub_key_hex);

    // load persisted prefs
    if (_fs->exists("/new_prefs")) {
      loadPrefsInt("/new_prefs");   // new filename
    } else if (_fs->exists("/node_prefs")) {
      loadPrefsInt("/node_prefs");
      savePrefs();  // save to new filename
      _fs->remove("/node_prefs");  // remove old
    }

  #ifdef BLE_PIN_CODE
    if (_prefs.ble_pin == 0) {
    #ifdef HAS_UI
      if (has_display) {
        StdRNG  rng;
        _active_ble_pin = rng.nextInt(100000, 999999);  // random pin each session
      } else {
        _active_ble_pin = BLE_PIN_CODE;  // otherwise static pin
      }
    #else
      _active_ble_pin = BLE_PIN_CODE;  // otherwise static pin
    #endif
    } else {
      _active_ble_pin = _prefs.ble_pin;
    }
  #else
    _active_ble_pin = 0;
  #endif

    // init 'blob store' support
    _fs->mkdir("/bl");

    loadContacts();
    addChannel("Public", PUBLIC_GROUP_PSK); // pre-configure Andy's public channel
    loadChannels();

    radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
    radio_set_tx_power(_prefs.tx_power_dbm);
  }

  const char* getNodeName() { return _prefs.node_name; }
  NodePrefs* getNodePrefs() { 
    return &_prefs; 
  }
  uint32_t getBLEPin() { return _active_ble_pin; }

  void startInterface(BaseSerialInterface& serial) {
    _serial = &serial;
    serial.enable();
  }

  void savePrefs() {
#if defined(NRF52_PLATFORM)
    File file = _fs->open("/new_prefs", FILE_O_WRITE);
    if (file) { file.seek(0); file.truncate(); }
#elif defined(RP2040_PLATFORM)
    File file = _fs->open("/new_prefs", "w");
#else
    File file = _fs->open("/new_prefs", "w", true);
#endif
    if (file) {
      uint8_t pad[8];
      memset(pad, 0, sizeof(pad));

      file.write((uint8_t *) &_prefs.airtime_factor, sizeof(float));  // 0
      file.write((uint8_t *) _prefs.node_name, sizeof(_prefs.node_name));  // 4
      file.write(pad, 4);   // 36
      file.write((uint8_t *) &sensors.node_lat, sizeof(sensors.node_lat));  // 40
      file.write((uint8_t *) &sensors.node_lon, sizeof(sensors.node_lon));  // 48
      file.write((uint8_t *) &_prefs.freq, sizeof(_prefs.freq));   // 56
      file.write((uint8_t *) &_prefs.sf, sizeof(_prefs.sf));  // 60
      file.write((uint8_t *) &_prefs.cr, sizeof(_prefs.cr));  // 61
      file.write((uint8_t *) &_prefs.reserved1, sizeof(_prefs.reserved1));  // 62
      file.write((uint8_t *) &_prefs.manual_add_contacts, sizeof(_prefs.manual_add_contacts));  // 63
      file.write((uint8_t *) &_prefs.bw, sizeof(_prefs.bw));  // 64
      file.write((uint8_t *) &_prefs.tx_power_dbm, sizeof(_prefs.tx_power_dbm));  // 68
      file.write((uint8_t *) &_prefs.telemetry_mode_base, sizeof(_prefs.telemetry_mode_base));  // 69
      file.write((uint8_t *) &_prefs.telemetry_mode_loc, sizeof(_prefs.telemetry_mode_loc));  // 70
      file.write(pad, 1);  // 71
      file.write((uint8_t *) &_prefs.rx_delay_base, sizeof(_prefs.rx_delay_base));  // 72
      file.write(pad, 4);   // 76
      file.write((uint8_t *) &_prefs.ble_pin, sizeof(_prefs.ble_pin));  // 80

      file.close();
    }
  }

  void handleCmdFrame(size_t len) {
    if (cmd_frame[0] == CMD_DEVICE_QEURY && len >= 2) {  // sent when app establishes connection
      app_target_ver = cmd_frame[1];   // which version of protocol does app understand

      int i = 0;
      out_frame[i++] = RESP_CODE_DEVICE_INFO;
      out_frame[i++] = FIRMWARE_VER_CODE;
      out_frame[i++] = MAX_CONTACTS / 2;        // v3+
      out_frame[i++] = MAX_GROUP_CHANNELS;      // v3+
      memcpy(&out_frame[i], &_prefs.ble_pin, 4); i += 4;
      memset(&out_frame[i], 0, 12);
      strcpy((char *) &out_frame[i], FIRMWARE_BUILD_DATE); i += 12;
      StrHelper::strzcpy((char *) &out_frame[i], board.getManufacturerName(), 40); i += 40;
      StrHelper::strzcpy((char *) &out_frame[i], FIRMWARE_VERSION, 20); i += 20;
      _serial->writeFrame(out_frame, i);
    } else if (cmd_frame[0] == CMD_APP_START && len >= 8) {   // sent when app establishes connection, respond with node ID
      //  cmd_frame[1..7]  reserved future
      char* app_name = (char *) &cmd_frame[8];
      cmd_frame[len] = 0;  // make app_name null terminated
      MESH_DEBUG_PRINTLN("App %s connected", app_name);

      _iter_started = false;   // stop any left-over ContactsIterator
      int i = 0;
      out_frame[i++] = RESP_CODE_SELF_INFO;
      out_frame[i++] = ADV_TYPE_CHAT;   // what this node Advert identifies as (maybe node's pronouns too?? :-)
      out_frame[i++] = _prefs.tx_power_dbm;
      out_frame[i++] = MAX_LORA_TX_POWER;
      memcpy(&out_frame[i], self_id.pub_key, PUB_KEY_SIZE); i += PUB_KEY_SIZE;

      int32_t lat, lon;
      lat = (sensors.node_lat * 1000000.0);
      lon = (sensors.node_lon * 1000000.0);
      memcpy(&out_frame[i], &lat, 4); i += 4;
      memcpy(&out_frame[i], &lon, 4); i += 4;
      out_frame[i++] = 0;  // reserved
      out_frame[i++] = 0;  // reserved
      out_frame[i++] = (_prefs.telemetry_mode_loc << 2) | (_prefs.telemetry_mode_base);  // v5+
      out_frame[i++] = _prefs.manual_add_contacts;

      uint32_t freq = _prefs.freq * 1000;
      memcpy(&out_frame[i], &freq, 4); i += 4;
      uint32_t bw = _prefs.bw*1000;
      memcpy(&out_frame[i], &bw, 4); i += 4;
      out_frame[i++] = _prefs.sf;
      out_frame[i++] = _prefs.cr;

      int tlen = strlen(_prefs.node_name);   // revisit: UTF_8 ??
      memcpy(&out_frame[i], _prefs.node_name, tlen); i += tlen;
      _serial->writeFrame(out_frame, i);
    } else if (cmd_frame[0] == CMD_SEND_TXT_MSG && len >= 14) {
      int i = 1;
      uint8_t txt_type = cmd_frame[i++];
      uint8_t attempt = cmd_frame[i++];
      uint32_t msg_timestamp;
      memcpy(&msg_timestamp, &cmd_frame[i], 4); i += 4;
      uint8_t* pub_key_prefix = &cmd_frame[i]; i += 6;
      ContactInfo* recipient = lookupContactByPubKey(pub_key_prefix, 6);
      if (recipient && (txt_type == TXT_TYPE_PLAIN || txt_type == TXT_TYPE_CLI_DATA)) {
        char *text = (char *) &cmd_frame[i];
        int tlen = len - i;
        uint32_t est_timeout;
        text[tlen] = 0;  // ensure null
        int result;
        uint32_t expected_ack;
        if (txt_type == TXT_TYPE_CLI_DATA) {
          result = sendCommandData(*recipient, msg_timestamp, attempt, text, est_timeout);
          expected_ack = 0;  // no Ack expected
        } else {
          result = sendMessage(*recipient, msg_timestamp, attempt, text, expected_ack, est_timeout);
        }
        // TODO: add expected ACK to table
        if (result == MSG_SEND_FAILED) {
          writeErrFrame(ERR_CODE_TABLE_FULL);
        } else {
          if (expected_ack) {
            expected_ack_table[next_ack_idx].msg_sent = _ms->getMillis();  // add to circular table
            expected_ack_table[next_ack_idx].ack = expected_ack;
            next_ack_idx = (next_ack_idx + 1) % EXPECTED_ACK_TABLE_SIZE;
          }

          out_frame[0] = RESP_CODE_SENT;
          out_frame[1] = (result == MSG_SEND_SENT_FLOOD) ? 1 : 0;
          memcpy(&out_frame[2], &expected_ack, 4);
          memcpy(&out_frame[6], &est_timeout, 4);
          _serial->writeFrame(out_frame, 10);
        }
      } else {
        writeErrFrame(recipient == NULL ? ERR_CODE_NOT_FOUND : ERR_CODE_UNSUPPORTED_CMD); // unknown recipient, or unsuported TXT_TYPE_*
      }
    } else if (cmd_frame[0] == CMD_SEND_CHANNEL_TXT_MSG) {  // send GroupChannel msg
      int i = 1;
      uint8_t txt_type = cmd_frame[i++];  // should be TXT_TYPE_PLAIN
      uint8_t channel_idx = cmd_frame[i++];
      uint32_t msg_timestamp;
      memcpy(&msg_timestamp, &cmd_frame[i], 4); i += 4;
      const char *text = (char *) &cmd_frame[i];

      if (txt_type != TXT_TYPE_PLAIN) {
        writeErrFrame(ERR_CODE_UNSUPPORTED_CMD);
      } else {
        ChannelDetails channel;
        bool success = getChannel(channel_idx, channel);
        if (success && sendGroupMessage(msg_timestamp, channel.channel, _prefs.node_name, text, len - i)) {
          writeOKFrame();
        } else {
          writeErrFrame(ERR_CODE_NOT_FOUND);  // bad channel_idx
        }
      }
    } else if (cmd_frame[0] == CMD_GET_CONTACTS) {  // get Contact list
      if (_iter_started) {
        writeErrFrame(ERR_CODE_BAD_STATE);   // iterator is currently busy
      } else {
        if (len >= 5) {   // has optional 'since' param
          memcpy(&_iter_filter_since, &cmd_frame[1], 4);
        } else {
          _iter_filter_since = 0;
        }

        uint8_t reply[5];
        reply[0] = RESP_CODE_CONTACTS_START;
        uint32_t count = getNumContacts();  // total, NOT filtered count
        memcpy(&reply[1], &count, 4);
        _serial->writeFrame(reply, 5);

        // start iterator
        _iter = startContactsIterator();
        _iter_started = true;
        _most_recent_lastmod = 0;
      }
    } else if (cmd_frame[0] == CMD_SET_ADVERT_NAME && len >= 2) {
      int nlen = len - 1;
      if (nlen > sizeof(_prefs.node_name)-1) nlen = sizeof(_prefs.node_name)-1;  // max len
      memcpy(_prefs.node_name, &cmd_frame[1], nlen);
      _prefs.node_name[nlen] = 0; // null terminator
      savePrefs();
      writeOKFrame();
    } else if (cmd_frame[0] == CMD_SET_ADVERT_LATLON && len >= 9) {
      int32_t lat, lon, alt = 0;
      memcpy(&lat, &cmd_frame[1], 4);
      memcpy(&lon, &cmd_frame[5], 4);
      if (len >= 13) {
        memcpy(&alt, &cmd_frame[9], 4);  // for FUTURE support
      }
      if (lat <= 90*1E6 && lat >= -90*1E6 && lon <= 180*1E6 && lon >= -180*1E6) {
        sensors.node_lat = ((double)lat) / 1000000.0;
        sensors.node_lon = ((double)lon) / 1000000.0;
        savePrefs();
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_ILLEGAL_ARG);  // invalid geo coordinate
      }
    } else if (cmd_frame[0] == CMD_GET_DEVICE_TIME) {
      uint8_t reply[5];
      reply[0] = RESP_CODE_CURR_TIME;
      uint32_t now = getRTCClock()->getCurrentTime();
      memcpy(&reply[1], &now, 4);
      _serial->writeFrame(reply, 5);
    } else if (cmd_frame[0] == CMD_SET_DEVICE_TIME && len >= 5) {
      uint32_t secs;
      memcpy(&secs, &cmd_frame[1], 4);
      uint32_t curr = getRTCClock()->getCurrentTime();
      if (secs >= curr) {
        getRTCClock()->setCurrentTime(secs);
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_ILLEGAL_ARG);
      }
    } else if (cmd_frame[0] == CMD_SEND_SELF_ADVERT) {
      auto pkt = createSelfAdvert(_prefs.node_name, sensors.node_lat, sensors.node_lon);
      if (pkt) {
        if (len >= 2 && cmd_frame[1] == 1) {   // optional param (1 = flood, 0 = zero hop)
          sendFlood(pkt);
        } else {
          sendZeroHop(pkt);
        }
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_TABLE_FULL);
      }
    } else if (cmd_frame[0] == CMD_RESET_PATH && len >= 1+32) {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      if (recipient) {
        recipient->out_path_len = -1;
        //recipient->lastmod = ??   shouldn't be needed, app already has this version of contact
        saveContacts();
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);  // unknown contact
      }
    } else if (cmd_frame[0] == CMD_ADD_UPDATE_CONTACT && len >= 1+32+2+1) {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      if (recipient) {
        updateContactFromFrame(*recipient, cmd_frame, len);
        //recipient->lastmod = ??   shouldn't be needed, app already has this version of contact
        saveContacts();
        writeOKFrame();
      } else {
        ContactInfo contact;
        updateContactFromFrame(contact, cmd_frame, len);
        contact.lastmod = getRTCClock()->getCurrentTime();
        contact.sync_since = 0;
        if (addContact(contact)) {
          saveContacts();
          writeOKFrame();
        } else {
          writeErrFrame(ERR_CODE_TABLE_FULL);
        }
      }
    } else if (cmd_frame[0] == CMD_REMOVE_CONTACT) {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      if (recipient && removeContact(*recipient)) {
        saveContacts();
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);  // not found, or unable to remove
      }
    } else if (cmd_frame[0] == CMD_SHARE_CONTACT) {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      if (recipient) {
        if (shareContactZeroHop(*recipient)) {
          writeOKFrame();
        } else {
          writeErrFrame(ERR_CODE_TABLE_FULL);  // unable to send
        }
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);
      }
    } else if (cmd_frame[0] == CMD_GET_CONTACT_BY_KEY) {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* contact = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      if (contact) {
        writeContactRespFrame(RESP_CODE_CONTACT, *contact);
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);  // not found
      }
    } else if (cmd_frame[0] == CMD_EXPORT_CONTACT) {
      if (len < 1 + PUB_KEY_SIZE) {
        // export SELF
        auto pkt = createSelfAdvert(_prefs.node_name, sensors.node_lat, sensors.node_lon);
        if (pkt) {
          pkt->header |= ROUTE_TYPE_FLOOD;  // would normally be sent in this mode

          out_frame[0] = RESP_CODE_EXPORT_CONTACT;
          uint8_t out_len =  pkt->writeTo(&out_frame[1]);
          releasePacket(pkt);  // undo the obtainNewPacket()
          _serial->writeFrame(out_frame, out_len + 1);
        } else {
          writeErrFrame(ERR_CODE_TABLE_FULL);  // Error
        }
      } else {
        uint8_t* pub_key = &cmd_frame[1];
        ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
        uint8_t out_len;
        if (recipient && (out_len = exportContact(*recipient, &out_frame[1])) > 0) {
          out_frame[0] = RESP_CODE_EXPORT_CONTACT;
          _serial->writeFrame(out_frame, out_len + 1);
        } else {
          writeErrFrame(ERR_CODE_NOT_FOUND);  // not found
        }
      }
    } else if (cmd_frame[0] == CMD_IMPORT_CONTACT && len > 2+32+64) {
      if (importContact(&cmd_frame[1], len - 1)) {
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_ILLEGAL_ARG);
      }
    } else if (cmd_frame[0] == CMD_SYNC_NEXT_MESSAGE) {
      int out_len;
      if ((out_len = getFromOfflineQueue(out_frame)) > 0) {
        _serial->writeFrame(out_frame, out_len);
        #ifdef HAS_UI
          ui_task.msgRead(offline_queue_len);
        #endif
      } else {
        out_frame[0] = RESP_CODE_NO_MORE_MESSAGES;
        _serial->writeFrame(out_frame, 1);
      }
    } else if (cmd_frame[0] == CMD_SET_RADIO_PARAMS) {
      int i = 1;
      uint32_t freq;
      memcpy(&freq, &cmd_frame[i], 4); i += 4;
      uint32_t bw;
      memcpy(&bw, &cmd_frame[i], 4); i += 4;
      uint8_t sf = cmd_frame[i++];
      uint8_t cr = cmd_frame[i++];

      if (freq >= 300000 && freq <= 2500000 && sf >= 7 && sf <= 12 && cr >= 5 && cr <= 8 && bw >= 7000 && bw <= 500000) {
        _prefs.sf = sf;
        _prefs.cr = cr;
        _prefs.freq = (float)freq / 1000.0;
        _prefs.bw = (float)bw / 1000.0;
        savePrefs();

        radio_set_params(_prefs.freq, _prefs.bw, _prefs.sf, _prefs.cr);
        MESH_DEBUG_PRINTLN("OK: CMD_SET_RADIO_PARAMS: f=%d, bw=%d, sf=%d, cr=%d", freq, bw, (uint32_t)sf, (uint32_t)cr);

        writeOKFrame();
      } else {
        MESH_DEBUG_PRINTLN("Error: CMD_SET_RADIO_PARAMS: f=%d, bw=%d, sf=%d, cr=%d", freq, bw, (uint32_t)sf, (uint32_t)cr);
        writeErrFrame(ERR_CODE_ILLEGAL_ARG);
      }
    } else if (cmd_frame[0] == CMD_SET_RADIO_TX_POWER) {
      if (cmd_frame[1] > MAX_LORA_TX_POWER) {
        writeErrFrame(ERR_CODE_ILLEGAL_ARG);
      } else {
        _prefs.tx_power_dbm = cmd_frame[1];
        savePrefs();
        radio_set_tx_power(_prefs.tx_power_dbm);
        writeOKFrame();
      }
    } else if (cmd_frame[0] == CMD_SET_TUNING_PARAMS) {
      int i = 1;
      uint32_t rx, af;
      memcpy(&rx, &cmd_frame[i], 4); i += 4;
      memcpy(&af, &cmd_frame[i], 4); i += 4;
      _prefs.rx_delay_base = ((float)rx) / 1000.0f;
      _prefs.airtime_factor = ((float)af) / 1000.0f;
      savePrefs();
      writeOKFrame();
    } else if (cmd_frame[0] == CMD_SET_OTHER_PARAMS) {
      _prefs.manual_add_contacts = cmd_frame[1];
      if (len >= 3) {
        _prefs.telemetry_mode_base = cmd_frame[2] & 0x03;       // v5+
        _prefs.telemetry_mode_loc = (cmd_frame[2] >> 2) & 0x03;
      }
      savePrefs();
      writeOKFrame();
    } else if (cmd_frame[0] == CMD_REBOOT && memcmp(&cmd_frame[1], "reboot", 6) == 0) {
      board.reboot();
    } else if (cmd_frame[0] == CMD_GET_BATTERY_VOLTAGE) {
      uint8_t reply[3];
      reply[0] = RESP_CODE_BATTERY_VOLTAGE;
      uint16_t battery_millivolts = board.getBattMilliVolts();
      memcpy(&reply[1], &battery_millivolts, 2);
      _serial->writeFrame(reply, 3);
    } else if (cmd_frame[0] == CMD_EXPORT_PRIVATE_KEY) {
      #if ENABLE_PRIVATE_KEY_EXPORT
        uint8_t reply[65];
        reply[0] = RESP_CODE_PRIVATE_KEY;
        self_id.writeTo(&reply[1], 64);
        _serial->writeFrame(reply, 65);
      #else
        writeDisabledFrame();
      #endif
    } else if (cmd_frame[0] == CMD_IMPORT_PRIVATE_KEY && len >= 65) {
      #if ENABLE_PRIVATE_KEY_IMPORT
        mesh::LocalIdentity identity;
        identity.readFrom(&cmd_frame[1], 64);
        if (saveMainIdentity(identity)) {
          self_id = identity;
          writeOKFrame();
        } else {
          writeErrFrame(ERR_CODE_FILE_IO_ERROR);
        }
      #else
        writeDisabledFrame();
      #endif
    } else if (cmd_frame[0] == CMD_SEND_RAW_DATA && len >= 6) {
      int i = 1;
      int8_t path_len = cmd_frame[i++];
      if (path_len >= 0 && i + path_len + 4 <= len) {  // minimum 4 byte payload
        uint8_t* path = &cmd_frame[i]; i += path_len;
        auto pkt = createRawData(&cmd_frame[i], len - i);
        if (pkt) {
          sendDirect(pkt, path, path_len);
          writeOKFrame();
        } else {
          writeErrFrame(ERR_CODE_TABLE_FULL);
        }
      } else {
        writeErrFrame(ERR_CODE_UNSUPPORTED_CMD);  // flood, not supported (yet)
      }
    } else if (cmd_frame[0] == CMD_SEND_LOGIN && len >= 1+PUB_KEY_SIZE) {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      char *password = (char *) &cmd_frame[1+PUB_KEY_SIZE];
      cmd_frame[len] = 0;  // ensure null terminator in password
      if (recipient) {
        uint32_t est_timeout;
        int result = sendLogin(*recipient, password, est_timeout);
        if (result == MSG_SEND_FAILED) {
          writeErrFrame(ERR_CODE_TABLE_FULL);
        } else {
          pending_telemetry = pending_status = 0;
          memcpy(&pending_login, recipient->id.pub_key, 4);  // match this to onContactResponse()
          out_frame[0] = RESP_CODE_SENT;
          out_frame[1] = (result == MSG_SEND_SENT_FLOOD) ? 1 : 0;
          memcpy(&out_frame[2], &pending_login, 4);
          memcpy(&out_frame[6], &est_timeout, 4);
          _serial->writeFrame(out_frame, 10);
        }
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);  // contact not found
      }
    } else if (cmd_frame[0] == CMD_SEND_STATUS_REQ && len >= 1+PUB_KEY_SIZE) {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      if (recipient) {
        uint32_t tag, est_timeout;
        int result = sendRequest(*recipient, REQ_TYPE_GET_STATUS, tag, est_timeout);
        if (result == MSG_SEND_FAILED) {
          writeErrFrame(ERR_CODE_TABLE_FULL);
        } else {
          pending_telemetry = pending_login = 0;
          // FUTURE:  pending_status = tag;  // match this in onContactResponse()
          memcpy(&pending_status, recipient->id.pub_key, 4);  // legacy matching scheme
          out_frame[0] = RESP_CODE_SENT;
          out_frame[1] = (result == MSG_SEND_SENT_FLOOD) ? 1 : 0;
          memcpy(&out_frame[2], &tag, 4);
          memcpy(&out_frame[6], &est_timeout, 4);
          _serial->writeFrame(out_frame, 10);
        }
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);  // contact not found
      }
    } else if (cmd_frame[0] == CMD_SEND_TELEMETRY_REQ && len >= 4+PUB_KEY_SIZE) {
      uint8_t* pub_key = &cmd_frame[4];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      if (recipient) {
        uint32_t tag, est_timeout;
        int result = sendRequest(*recipient, REQ_TYPE_GET_TELEMETRY_DATA, tag, est_timeout);
        if (result == MSG_SEND_FAILED) {
          writeErrFrame(ERR_CODE_TABLE_FULL);
        } else {
          pending_status = pending_login = 0;
          pending_telemetry = tag;  // match this in onContactResponse()
          out_frame[0] = RESP_CODE_SENT;
          out_frame[1] = (result == MSG_SEND_SENT_FLOOD) ? 1 : 0;
          memcpy(&out_frame[2], &tag, 4);
          memcpy(&out_frame[6], &est_timeout, 4);
          _serial->writeFrame(out_frame, 10);
        }
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);  // contact not found
      }
    } else if (cmd_frame[0] == CMD_HAS_CONNECTION && len >= 1+PUB_KEY_SIZE) {
      uint8_t* pub_key = &cmd_frame[1];
      if (hasConnectionTo(pub_key)) {
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);
      }
    } else if (cmd_frame[0] == CMD_LOGOUT && len >= 1+PUB_KEY_SIZE) {
      uint8_t* pub_key = &cmd_frame[1];
      stopConnection(pub_key);
      writeOKFrame();
    } else if (cmd_frame[0] == CMD_GET_CHANNEL && len >= 2) {
      uint8_t channel_idx = cmd_frame[1];
      ChannelDetails channel;
      if (getChannel(channel_idx, channel)) {
        int i = 0;
        out_frame[i++] = RESP_CODE_CHANNEL_INFO;
        out_frame[i++] = channel_idx;
        strcpy((char *)&out_frame[i], channel.name); i += 32;
        memcpy(&out_frame[i], channel.channel.secret, 16); i += 16;   // NOTE: only 128-bit supported
        _serial->writeFrame(out_frame, i);
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);
      }
    } else if (cmd_frame[0] == CMD_SET_CHANNEL && len >= 2+32+32) {
      writeErrFrame(ERR_CODE_UNSUPPORTED_CMD);  // not supported (yet)
    } else if (cmd_frame[0] == CMD_SET_CHANNEL && len >= 2+32+16) {
      uint8_t channel_idx = cmd_frame[1];
      ChannelDetails channel;
      StrHelper::strncpy(channel.name, (char *) &cmd_frame[2], 32);
      memset(channel.channel.secret, 0, sizeof(channel.channel.secret));
      memcpy(channel.channel.secret, &cmd_frame[2+32], 16);   // NOTE: only 128-bit supported
      if (setChannel(channel_idx, channel)) {
        saveChannels();
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_NOT_FOUND);  // bad channel_idx
      }
    } else if (cmd_frame[0] == CMD_SIGN_START) {
      out_frame[0] = RESP_CODE_SIGN_START;
      out_frame[1] = 0;  // reserved
      uint32_t len = MAX_SIGN_DATA_LEN;
      memcpy(&out_frame[2], &len, 4);
      _serial->writeFrame(out_frame, 6);

      if (sign_data) {
        free(sign_data);
      }
      sign_data = (uint8_t *) malloc(MAX_SIGN_DATA_LEN);
      sign_data_len = 0;
    } else if (cmd_frame[0] == CMD_SIGN_DATA && len > 1) {
      if (sign_data == NULL || sign_data_len + (len - 1) > MAX_SIGN_DATA_LEN) {
        writeErrFrame(sign_data == NULL ? ERR_CODE_BAD_STATE : ERR_CODE_TABLE_FULL);  // error: too long
      } else {
        memcpy(&sign_data[sign_data_len], &cmd_frame[1], len - 1);
        sign_data_len += (len - 1);
        writeOKFrame();
      }
    } else if (cmd_frame[0] == CMD_SIGN_FINISH) {
      if (sign_data) {
        self_id.sign(&out_frame[1], sign_data, sign_data_len);

        free(sign_data);  // don't need sign_data now
        sign_data = NULL;

        out_frame[0] = RESP_CODE_SIGNATURE;
        _serial->writeFrame(out_frame, 1 + SIGNATURE_SIZE);
      } else {
        writeErrFrame(ERR_CODE_BAD_STATE);
      }
    } else if (cmd_frame[0] == CMD_SEND_TRACE_PATH && len > 10 && len - 10 < MAX_PATH_SIZE) {
      uint32_t tag, auth;
      memcpy(&tag, &cmd_frame[1], 4);
      memcpy(&auth, &cmd_frame[5], 4);
      auto pkt = createTrace(tag, auth, cmd_frame[9]);
      if (pkt) {
        uint8_t path_len = len - 10;
        sendDirect(pkt, &cmd_frame[10], path_len);

        uint32_t t = _radio->getEstAirtimeFor(pkt->payload_len + pkt->path_len + 2);
        uint32_t est_timeout = calcDirectTimeoutMillisFor(t, path_len);

        out_frame[0] = RESP_CODE_SENT;
        out_frame[1] = 0;
        memcpy(&out_frame[2], &tag, 4);
        memcpy(&out_frame[6], &est_timeout, 4);
        _serial->writeFrame(out_frame, 10);
      } else {
        writeErrFrame(ERR_CODE_TABLE_FULL);
      }
    } else if (cmd_frame[0] == CMD_SET_DEVICE_PIN && len >= 5) {

      // get pin from command frame
      uint32_t pin;
      memcpy(&pin, &cmd_frame[1], 4);

      // ensure pin is zero, or a valid 6 digit pin
      if(pin == 0 || (pin >= 100000 && pin <= 999999)){
        _prefs.ble_pin = pin;
        savePrefs();
        writeOKFrame();
      } else {
        writeErrFrame(ERR_CODE_ILLEGAL_ARG);
      }
      
    } else if (cmd_frame[0] == CMD_GET_CUSTOM_VARS) {
      out_frame[0] = RESP_CODE_CUSTOM_VARS;
      char* dp = (char *) &out_frame[1];
      for (int i = 0; i < sensors.getNumSettings() && dp - (char *) &out_frame[1] < 140; i++) {
        if (i > 0) { *dp++ = ','; }
        strcpy(dp, sensors.getSettingName(i)); dp = strchr(dp, 0);
        *dp++ = ':';
        strcpy(dp, sensors.getSettingValue(i)); dp = strchr(dp, 0);
      }
      _serial->writeFrame(out_frame, dp - (char *)out_frame);
    } else if (cmd_frame[0] == CMD_SET_CUSTOM_VAR && len >= 4) {
      cmd_frame[len] =  0;
      char* sp = (char *) &cmd_frame[1];
      char* np = strchr(sp, ':');  // look for separator char
      if (np) {
        *np++ = 0;   // modify 'cmd_frame', replace ':' with null
        bool success = sensors.setSettingValue(sp, np);
        if (success) {
          writeOKFrame();
        } else {
          writeErrFrame(ERR_CODE_ILLEGAL_ARG);
        }
      } else {
        writeErrFrame(ERR_CODE_ILLEGAL_ARG);
      }
    } else {
      writeErrFrame(ERR_CODE_UNSUPPORTED_CMD);
      MESH_DEBUG_PRINTLN("ERROR: unknown command: %02X", cmd_frame[0]);
    }
  }

  void loop() {
    BaseChatMesh::loop();

    size_t len = _serial->checkRecvFrame(cmd_frame);
    if (len > 0) {
      handleCmdFrame(len);
    } else if (_iter_started    // check if our ContactsIterator is 'running'
      && !_serial->isWriteBusy()    // don't spam the Serial Interface too quickly!
    ) {
      ContactInfo contact;
      if (_iter.hasNext(this, contact)) {
        if (contact.lastmod > _iter_filter_since) {  // apply the 'since' filter
          writeContactRespFrame(RESP_CODE_CONTACT, contact);
          if (contact.lastmod > _most_recent_lastmod) {
            _most_recent_lastmod = contact.lastmod;   // save for the RESP_CODE_END_OF_CONTACTS frame
          }
        }
      } else {  // EOF
        out_frame[0] = RESP_CODE_END_OF_CONTACTS;
        memcpy(&out_frame[1], &_most_recent_lastmod, 4);  // include the most recent lastmod, so app can update their 'since'
        _serial->writeFrame(out_frame, 5);
        _iter_started = false;
      }
    } else if (!_serial->isWriteBusy()) {
      checkConnections();
    }

  #ifdef HAS_UI
    ui_task.setHasConnection(_serial->isConnected());
    ui_task.loop();
  #endif
  }
};

#ifdef ESP32
  #ifdef WIFI_SSID
    #include <helpers/esp32/SerialWifiInterface.h>
    SerialWifiInterface serial_interface;
    #ifndef TCP_PORT
      #define TCP_PORT 5000
    #endif
  #elif defined(BLE_PIN_CODE)
    #include <helpers/esp32/SerialBLEInterface.h>
    SerialBLEInterface serial_interface;
  #elif defined(SERIAL_RX)
    #include <helpers/ArduinoSerialInterface.h>
    ArduinoSerialInterface serial_interface;
    HardwareSerial companion_serial(1);
  #else
    #include <helpers/ArduinoSerialInterface.h>
    ArduinoSerialInterface serial_interface;
  #endif
#elif defined(RP2040_PLATFORM)
  //#ifdef WIFI_SSID
  //  #include <helpers/rp2040/SerialWifiInterface.h>
  //  SerialWifiInterface serial_interface;
  //  #ifndef TCP_PORT
  //    #define TCP_PORT 5000
  //  #endif
  // #elif defined(BLE_PIN_CODE)
  //   #include <helpers/rp2040/SerialBLEInterface.h>
  //   SerialBLEInterface serial_interface;
  #if defined(SERIAL_RX)
    #include <helpers/ArduinoSerialInterface.h>
    ArduinoSerialInterface serial_interface;
    HardwareSerial companion_serial(1);
  #else
    #include <helpers/ArduinoSerialInterface.h>
    ArduinoSerialInterface serial_interface;
  #endif
#elif defined(NRF52_PLATFORM)
  #ifdef BLE_PIN_CODE
    #include <helpers/nrf52/SerialBLEInterface.h>
    SerialBLEInterface serial_interface;
  #else
    #include <helpers/ArduinoSerialInterface.h>
    ArduinoSerialInterface serial_interface;
  #endif
#else
  #error "need to define a serial interface"
#endif

StdRNG fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(radio_driver, fast_rng, *new VolatileRTCClock(), tables); // TODO: test with 'rtc_clock' in target.cpp

void halt() {
  while (1) ;
}

void setup() {
  Serial.begin(115200);

  board.begin();

#ifdef HAS_UI
  DisplayDriver* disp = NULL;
 #ifdef DISPLAY_CLASS
  if (display.begin()) {
    disp = &display;
    disp->startFrame();
    disp->print("Please wait...");
    disp->endFrame();
  }
 #endif
#endif

  if (!radio_init()) { halt(); }

  fast_rng.begin(radio_get_rng_seed());

#if defined(NRF52_PLATFORM)
  InternalFS.begin();
  the_mesh.begin(InternalFS,
    #ifdef HAS_UI
        disp != NULL
    #else
        false
    #endif
  );

#ifdef BLE_PIN_CODE
  char dev_name[32+16];
  sprintf(dev_name, "%s%s", BLE_NAME_PREFIX, the_mesh.getNodeName());
  serial_interface.begin(dev_name, the_mesh.getBLEPin());
#else
  serial_interface.begin(Serial);
#endif
  the_mesh.startInterface(serial_interface);
#elif defined(RP2040_PLATFORM)
  LittleFS.begin();
  the_mesh.begin(LittleFS,
    #ifdef HAS_UI
        disp != NULL
    #else
        false
    #endif
  );

  //#ifdef WIFI_SSID
  //  WiFi.begin(WIFI_SSID, WIFI_PWD);
  //  serial_interface.begin(TCP_PORT);
  // #elif defined(BLE_PIN_CODE)
  //   char dev_name[32+16];
  //   sprintf(dev_name, "%s%s", BLE_NAME_PREFIX, the_mesh.getNodeName());
  //   serial_interface.begin(dev_name, the_mesh.getBLEPin());
  #if defined(SERIAL_RX)
    companion_serial.setPins(SERIAL_RX, SERIAL_TX);
    companion_serial.begin(115200);
    serial_interface.begin(companion_serial);
  #else
    serial_interface.begin(Serial);
  #endif
    the_mesh.startInterface(serial_interface);
#elif defined(ESP32)
  SPIFFS.begin(true);
  the_mesh.begin(SPIFFS,
    #ifdef HAS_UI
        disp != NULL
    #else
        false
    #endif
  );

#ifdef WIFI_SSID
  WiFi.begin(WIFI_SSID, WIFI_PWD);
  serial_interface.begin(TCP_PORT);
#elif defined(BLE_PIN_CODE)
  char dev_name[32+16];
  sprintf(dev_name, "%s%s", BLE_NAME_PREFIX, the_mesh.getNodeName());
  serial_interface.begin(dev_name, the_mesh.getBLEPin());
#elif defined(SERIAL_RX)
  companion_serial.setPins(SERIAL_RX, SERIAL_TX);
  companion_serial.begin(115200);
  serial_interface.begin(companion_serial);
#else
  serial_interface.begin(Serial);
#endif
  the_mesh.startInterface(serial_interface);
#else
  #error "need to define filesystem"
#endif

  sensors.begin();

#ifdef HAS_UI
  ui_task.begin(disp, the_mesh.getNodePrefs(), FIRMWARE_BUILD_DATE, FIRMWARE_VERSION, the_mesh.getBLEPin());
#endif
}

void loop() {
  the_mesh.loop();
  sensors.loop();
}
