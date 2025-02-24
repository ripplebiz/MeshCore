#pragma once

#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>

#if defined(NRF52_PLATFORM)
  #include <InternalFileSystem.h>
#elif defined(ESP32)
  #include <SPIFFS.h>
#endif

#include <RadioLib.h>
#include <helpers/RadioLibWrappers.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/StaticPoolPacketManager.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include <helpers/BaseSerialInterface.h>
#include <RTClib.h>

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
#elif defined(T1000_E)
  #include <helpers/nrf52/T1000eBoard.h>
  #include <helpers/CustomLR1110Wrapper.h>
  static T1000eBoard board;
#else
  #error "need to provide a 'board' object"
#endif

#include <helpers/BaseChatMesh.h>

#ifndef MAX_CONTACTS
  #define MAX_CONTACTS         100
#endif

#ifndef OFFLINE_QUEUE_SIZE
  #define OFFLINE_QUEUE_SIZE  16
#endif

#define SEND_TIMEOUT_BASE_MILLIS          500
#define FLOOD_SEND_TIMEOUT_FACTOR         16.0f
#define DIRECT_SEND_PERHOP_FACTOR         6.0f
#define DIRECT_SEND_PERHOP_EXTRA_MILLIS   250

#ifndef MAX_LORA_TX_POWER
  #define MAX_LORA_TX_POWER  20
#endif
/*------------ Frame Protocol --------------*/

#define FIRMWARE_VER_CODE    1
#define FIRMWARE_BUILD_DATE   "19 Feb 2025"

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

#define RESP_CODE_OK                0
#define RESP_CODE_ERR               1
#define RESP_CODE_CONTACTS_START    2   // first reply to CMD_GET_CONTACTS
#define RESP_CODE_CONTACT           3   // multiple of these (after CMD_GET_CONTACTS)
#define RESP_CODE_END_OF_CONTACTS   4   // last reply to CMD_GET_CONTACTS
#define RESP_CODE_SELF_INFO         5   // reply to CMD_APP_START
#define RESP_CODE_SENT              6   // reply to CMD_SEND_TXT_MSG
#define RESP_CODE_CONTACT_MSG_RECV  7   // a reply to CMD_SYNC_NEXT_MESSAGE
#define RESP_CODE_CHANNEL_MSG_RECV  8   // a reply to CMD_SYNC_NEXT_MESSAGE
#define RESP_CODE_CURR_TIME         9   // a reply to CMD_GET_DEVICE_TIME
#define RESP_CODE_NO_MORE_MESSAGES 10   // a reply to CMD_SYNC_NEXT_MESSAGE
#define RESP_CODE_EXPORT_CONTACT   11
#define RESP_CODE_BATTERY_VOLTAGE  12   // a reply to a CMD_GET_BATTERY_VOLTAGE
#define RESP_CODE_DEVICE_INFO      13   // a reply to CMD_DEVICE_QEURY
#define RESP_CODE_PRIVATE_KEY      14   // a reply to CMD_EXPORT_PRIVATE_KEY
#define RESP_CODE_DISABLED         15

// these are _pushed_ to client app at any time
#define PUSH_CODE_ADVERT            0x80
#define PUSH_CODE_PATH_UPDATED      0x81
#define PUSH_CODE_SEND_CONFIRMED    0x82
#define PUSH_CODE_MSG_WAITING       0x83
#define PUSH_CODE_RAW_DATA          0x84
#define PUSH_CODE_LOGIN_SUCCESS     0x85
#define PUSH_CODE_LOGIN_FAIL        0x86
#define PUSH_CODE_STATUS_RESPONSE   0x87

/* -------------------------------------------------------------------------------------- */

struct NodePrefs {  // persisted to file
  float airtime_factor;
  char node_name[32];
  double node_lat, node_lon;
  float freq;
  uint8_t sf;
  uint8_t cr;
  uint8_t reserved1;
  uint8_t reserved2;
  float bw;
  uint8_t tx_power_dbm;
  uint8_t unused[3];
  float rx_delay_base;
};

class BaseCompanionRadioMesh : public BaseChatMesh {
  FILESYSTEM* _fs;
  RADIO_CLASS* _phy;
  IdentityStore* _identity_store;
  uint32_t expected_ack_crc;  // TODO: keep table of expected ACKs
  uint32_t pending_login;
  uint32_t pending_status;
  mesh::GroupChannel* _public;
  BaseSerialInterface* _serial;
  unsigned long last_msg_sent;
  ContactsIterator _iter;
  uint32_t _iter_filter_since;
  uint32_t _most_recent_lastmod;
  bool  _iter_started;
  uint8_t app_target_ver;
  uint8_t cmd_frame[MAX_FRAME_SIZE+1];
  uint8_t out_frame[MAX_FRAME_SIZE+1];
  const char * _psk;

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  int offline_queue_len;
  Frame offline_queue[OFFLINE_QUEUE_SIZE];

  void loadMainIdentity(mesh::RNG& trng) {
    if (!_identity_store->load("_main", self_id)) {
      self_id = mesh::LocalIdentity(&trng);  // create new random identity
      saveMainIdentity(self_id);
    }
  }

  bool saveMainIdentity(const mesh::LocalIdentity& identity) {
    return _identity_store->save("_main", identity);
  }

  void loadContacts();
  void saveContacts();
  int  getBlobByKey(const uint8_t key[], int key_len, uint8_t dest_buf[]) override;
  bool putBlobByKey(const uint8_t key[], int key_len, const uint8_t src_buf[], int len) override;
  void writeOKFrame();
  void writeErrFrame();
  void writeDisabledFrame();
  void writeContactRespFrame(uint8_t code, const ContactInfo& contact);
  void updateContactFromFrame(ContactInfo& contact, const uint8_t* frame, int len);
  void addToOfflineQueue(const uint8_t frame[], int len);
  int getFromOfflineQueue(uint8_t frame[]);

  void soundBuzzer();

protected:
  NodePrefs _prefs;
  mesh::MainBoard* _board;

  float getAirtimeBudgetFactor() const override {
    return _prefs.airtime_factor;
  }

  int calcRxDelay(float score, uint32_t air_time) const override {
    if (_prefs.rx_delay_base <= 0.0f) return 0;
    return (int) ((pow(_prefs.rx_delay_base, 0.85f - score) - 1.0) * air_time);
  }

  uint32_t calcFloodTimeoutMillisFor(uint32_t pkt_airtime_millis) const override {
    return SEND_TIMEOUT_BASE_MILLIS + (FLOOD_SEND_TIMEOUT_FACTOR * pkt_airtime_millis);
  }
  uint32_t calcDirectTimeoutMillisFor(uint32_t pkt_airtime_millis, uint8_t path_len) const override {
    return SEND_TIMEOUT_BASE_MILLIS + 
         ( (pkt_airtime_millis*DIRECT_SEND_PERHOP_FACTOR + DIRECT_SEND_PERHOP_EXTRA_MILLIS) * (path_len + 1));
  }

  void onDiscoveredContact(ContactInfo& contact, bool is_new) override;
  void onContactPathUpdated(const ContactInfo& contact) override;
  bool processAck(const uint8_t *data) override;
  void onMessageRecv(const ContactInfo& from, uint8_t path_len, uint32_t sender_timestamp, const char *text) override;
  void onChannelMessageRecv(const mesh::GroupChannel& channel, int in_path_len, uint32_t timestamp, const char *text) override;
  void onContactResponse(const ContactInfo& contact, const uint8_t* data, uint8_t len) override;
  void onRawDataRecv(mesh::Packet* packet) override;
  void onSendTimeout() override;

public:

  BaseCompanionRadioMesh(RADIO_CLASS& phy, RadioLibWrapper& rw, mesh::RNG& rng, mesh::RTCClock& rtc, SimpleMeshTables& tables, mesh::MainBoard& board, const char* psk, float freq, uint8_t sf, float bw, uint8_t cr, uint8_t tx_power)
     : BaseChatMesh(rw, *new ArduinoMillis(), rng, rtc, *new StaticPoolPacketManager(16), tables), _serial(NULL), _phy(&phy), _board(&board), _psk(psk)
  {
    _iter_started = false;
    offline_queue_len = 0;
    app_target_ver = 0;
    _identity_store = NULL;
    pending_login = pending_status = 0;

    // defaults
    memset(&_prefs, 0, sizeof(_prefs));
    _prefs.airtime_factor = 1.0;    // one half
    strcpy(_prefs.node_name, "NONAME");
    _prefs.freq = freq;
    _prefs.sf = sf;
    _prefs.bw = bw;
    _prefs.cr = cr;
    _prefs.tx_power_dbm = tx_power;
    //_prefs.rx_delay_base = 10.0f;  enable once new algo fixed
  }

  void begin(FILESYSTEM& fs, mesh::RNG& trng);
  const char* getNodeName() { return _prefs.node_name; }
  void startInterface(BaseSerialInterface& serial);
  void savePrefs();
  void handleCmdFrame(size_t len);
  void loop();
};
