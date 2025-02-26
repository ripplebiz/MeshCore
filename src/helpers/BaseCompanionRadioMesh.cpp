#include <helpers/BaseCompanionRadioMesh.h>

void BaseCompanionRadioMesh::loadContacts() {
  if (_fs->exists("/contacts3")) {
    File file = _fs->open("/contacts3");
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

void BaseCompanionRadioMesh::saveContacts() {
#if defined(NRF52_PLATFORM)
  File file = _fs->open("/contacts3", FILE_O_WRITE);
  if (file) { file.seek(0); file.truncate(); }
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

int BaseCompanionRadioMesh::getBlobByKey(const uint8_t key[], int key_len, uint8_t dest_buf[]) {
  char path[64];
  char fname[18];

  if (key_len > 8) key_len = 8;   // just use first 8 bytes (prefix)
  mesh::Utils::toHex(fname, key, key_len);
  sprintf(path, "/bl/%s", fname);

  if (_fs->exists(path)) {
    File f = _fs->open(path);
    if (f) {
      int len = f.read(dest_buf, 255);  // currently MAX 255 byte blob len supported!!
      f.close();
      return len;
    }
  }
  return 0;   // not found
}

bool BaseCompanionRadioMesh::putBlobByKey(const uint8_t key[], int key_len, const uint8_t src_buf[], int len) {
  char path[64];
  char fname[18];

  if (key_len > 8) key_len = 8;   // just use first 8 bytes (prefix)
  mesh::Utils::toHex(fname, key, key_len);
  sprintf(path, "/bl/%s", fname);

#if defined(NRF52_PLATFORM)
  File f = _fs->open(path, FILE_O_WRITE);
  if (f) { f.seek(0); f.truncate(); }
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

void BaseCompanionRadioMesh::writeOKFrame() {
  uint8_t buf[1];
  buf[0] = RESP_CODE_OK;
  _serial->writeFrame(buf, 1);
}
void BaseCompanionRadioMesh::writeErrFrame() {
  uint8_t buf[1];
  buf[0] = RESP_CODE_ERR;
  _serial->writeFrame(buf, 1);
}

void BaseCompanionRadioMesh::writeDisabledFrame() {
  uint8_t buf[1];
  buf[0] = RESP_CODE_DISABLED;
  _serial->writeFrame(buf, 1);
}

void BaseCompanionRadioMesh::writeContactRespFrame(uint8_t code, const ContactInfo& contact) {
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

void BaseCompanionRadioMesh::updateContactFromFrame(ContactInfo& contact, const uint8_t* frame, int len) {
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

void BaseCompanionRadioMesh::addToOfflineQueue(const uint8_t frame[], int len) {
  if (offline_queue_len >= OFFLINE_QUEUE_SIZE) {
    MESH_DEBUG_PRINTLN("ERROR: offline_queue is full!");
  } else {
    offline_queue[offline_queue_len].len = len;
    memcpy(offline_queue[offline_queue_len].buf, frame, len);
    offline_queue_len++;
  }
}
int BaseCompanionRadioMesh::getFromOfflineQueue(uint8_t frame[]) {
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

void BaseCompanionRadioMesh::soundBuzzer() {
  // TODO
}

void BaseCompanionRadioMesh::onDiscoveredContact(ContactInfo& contact, bool is_new) {
  if (_serial->isConnected()) {
    out_frame[0] = PUSH_CODE_ADVERT;
    memcpy(&out_frame[1], contact.id.pub_key, PUB_KEY_SIZE);
    _serial->writeFrame(out_frame, 1 + PUB_KEY_SIZE);
  } else {
    soundBuzzer();
  }

  saveContacts();
}

void BaseCompanionRadioMesh::onContactPathUpdated(const ContactInfo& contact) {
  out_frame[0] = PUSH_CODE_PATH_UPDATED;
  memcpy(&out_frame[1], contact.id.pub_key, PUB_KEY_SIZE);
  _serial->writeFrame(out_frame, 1 + PUB_KEY_SIZE);   // NOTE: app may not be connected

  saveContacts();
}

bool BaseCompanionRadioMesh::processAck(const uint8_t *data) {
  // TODO: see if matches any in a table
  if (memcmp(data, &expected_ack_crc, 4) == 0) {     // got an ACK from recipient
    out_frame[0] = PUSH_CODE_SEND_CONFIRMED;
    memcpy(&out_frame[1], data, 4);
    uint32_t trip_time = _ms->getMillis() - last_msg_sent;
    memcpy(&out_frame[5], &trip_time, 4);
    _serial->writeFrame(out_frame, 9);

    // NOTE: the same ACK can be received multiple times!
    expected_ack_crc = 0;  // reset our expected hash, now that we have received ACK
    return true;
  }
  return false;
}

void BaseCompanionRadioMesh::onMessageRecv(const ContactInfo& from, uint8_t path_len, uint32_t sender_timestamp, const char *text) {
  int i = 0;
  out_frame[i++] = RESP_CODE_CONTACT_MSG_RECV;
  memcpy(&out_frame[i], from.id.pub_key, 6); i += 6;  // just 6-byte prefix
  out_frame[i++] = path_len;
  out_frame[i++] = TXT_TYPE_PLAIN;
  memcpy(&out_frame[i], &sender_timestamp, 4); i += 4;
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
}

void BaseCompanionRadioMesh::onChannelMessageRecv(const mesh::GroupChannel& channel, int in_path_len, uint32_t timestamp, const char *text) {
  int i = 0;
  out_frame[i++] = RESP_CODE_CHANNEL_MSG_RECV;
  out_frame[i++] = 0;  // FUTURE: channel_idx (will just be 'public' for now)
  out_frame[i++] = in_path_len < 0 ? 0xFF : in_path_len;
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
}

void BaseCompanionRadioMesh::onContactResponse(const ContactInfo& contact, const uint8_t* data, uint8_t len) {
  uint32_t sender_timestamp;
  memcpy(&sender_timestamp, data, 4);

  if (pending_login && memcmp(&pending_login, contact.id.pub_key, 4) == 0) { // check for login response
    // yes, is response to pending sendLogin()
    pending_login = 0;

    int i = 0;
    if (memcmp(&data[4], "OK", 2) == 0) {    // legacy Repeater login OK response
      out_frame[i++] = PUSH_CODE_LOGIN_SUCCESS;
      out_frame[i++] = 0;  // legacy: is_admin = false
    } else if (data[4] == RESP_SERVER_LOGIN_OK) {   // new login response
      //  keep_alive_interval  = data[5] * 16
      out_frame[i++] = PUSH_CODE_LOGIN_SUCCESS;
      out_frame[i++] = data[6];  // permissions (eg. is_admin)
    } else {
      out_frame[i++] = PUSH_CODE_LOGIN_FAIL;
      out_frame[i++] = 0;  // reserved
    }
    memcpy(&out_frame[i], contact.id.pub_key, 6); i += 6;  // pub_key_prefix
    _serial->writeFrame(out_frame, i);
  } else if (len > 4 && pending_status && memcmp(&pending_status, contact.id.pub_key, 4) == 0) { // check for status response
    // yes, is response to pending sendStatusRequest()
    pending_status = 0;

    int i = 0;
    out_frame[i++] = PUSH_CODE_STATUS_RESPONSE;
    out_frame[i++] = 0;  // reserved
    memcpy(&out_frame[i], contact.id.pub_key, 6); i += 6;  // pub_key_prefix
    memcpy(&out_frame[i], &data[4], len - 4); i += (len - 4);
    _serial->writeFrame(out_frame, i);
  }
}

void BaseCompanionRadioMesh::onRawDataRecv(mesh::Packet* packet) {
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


void BaseCompanionRadioMesh::onSendTimeout() {
}

void BaseCompanionRadioMesh::begin(FILESYSTEM& fs, mesh::RNG& trng) {
  _fs = &fs;

  BaseChatMesh::begin();

#if defined(NRF52_PLATFORM)
  _identity_store = new IdentityStore(fs, "");
#else
  _identity_store = new IdentityStore(fs, "/identity");
#endif

  loadMainIdentity(trng);

  // load persisted prefs
  if (_fs->exists("/node_prefs")) {
    File file = _fs->open("/node_prefs");
    if (file) {
      file.read((uint8_t *) &_prefs, sizeof(_prefs));
      file.close();
    }
  }

  // init 'blob store' support
  _fs->mkdir("/bl");

  loadContacts();
  _public = addChannel(_psk); // pre-configure Andy's public channel

  _phy->setFrequency(_prefs.freq);
  _phy->setSpreadingFactor(_prefs.sf);
  _phy->setBandwidth(_prefs.bw);
  _phy->setCodingRate(_prefs.cr);
  _phy->setOutputPower(_prefs.tx_power_dbm);
}

void BaseCompanionRadioMesh::startInterface(BaseSerialInterface& serial) {
  _serial = &serial;
  serial.enable();
}

void BaseCompanionRadioMesh::savePrefs() {
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

void BaseCompanionRadioMesh::handleCmdFrame(size_t len) {
  if (cmd_frame[0] == CMD_DEVICE_QEURY && len >= 2) {  // sent when app establishes connection
    app_target_ver = cmd_frame[1];   // which version of protocol does app understand

    int i = 0;
    out_frame[i++] = RESP_CODE_DEVICE_INFO;
    out_frame[i++] = FIRMWARE_VER_CODE;
    memset(&out_frame[i], 0, 6); i += 6;  // reserved
    memset(&out_frame[i], 0, 12);
    strcpy((char *) &out_frame[i], FIRMWARE_BUILD_DATE);
    i += 12;
    const char* name = _board->getManufacturerName();
    int tlen = strlen(name);
    memcpy(&out_frame[i], name, tlen); i += tlen;
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

    int32_t lat, lon, alt = 0;
    lat = (_prefs.node_lat * 1000000.0);
    lon = (_prefs.node_lon * 1000000.0);
    memcpy(&out_frame[i], &lat, 4); i += 4;
    memcpy(&out_frame[i], &lon, 4); i += 4;
    memcpy(&out_frame[i], &alt, 4); i += 4;
    
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
    if (recipient && attempt < 4 && txt_type == TXT_TYPE_PLAIN) {
      char *text = (char *) &cmd_frame[i];
      int tlen = len - i;
      uint32_t est_timeout;
      text[tlen] = 0;  // ensure null
      int result = sendMessage(*recipient, msg_timestamp, attempt, text, expected_ack_crc, est_timeout);
      // TODO: add expected ACK to table
      if (result == MSG_SEND_FAILED) {
        writeErrFrame();
      } else {
        last_msg_sent = _ms->getMillis();

        out_frame[0] = RESP_CODE_SENT;
        out_frame[1] = (result == MSG_SEND_SENT_FLOOD) ? 1 : 0;
        memcpy(&out_frame[2], &expected_ack_crc, 4);
        memcpy(&out_frame[6], &est_timeout, 4);
        _serial->writeFrame(out_frame, 10);
      }
    } else {
      writeErrFrame(); // unknown recipient, or unsuported TXT_TYPE_*
    }
  } else if (cmd_frame[0] == CMD_SEND_CHANNEL_TXT_MSG) {  // send GroupChannel msg
    int i = 1;
    uint8_t txt_type = cmd_frame[i++];  // should be TXT_TYPE_PLAIN
    uint8_t channel_idx = cmd_frame[i++];   // reserved future
    uint32_t msg_timestamp;
    memcpy(&msg_timestamp, &cmd_frame[i], 4); i += 4;
    const char *text = (char *) &cmd_frame[i];

    if (txt_type == TXT_TYPE_PLAIN && sendGroupMessage(msg_timestamp, *_public, _prefs.node_name, text, len - i)) {   // hard-coded to 'public' channel for now
      writeOKFrame();
    } else {
      writeErrFrame();
    }
  } else if (cmd_frame[0] == CMD_GET_CONTACTS) {  // get Contact list
    if (_iter_started) {
      writeErrFrame();   // iterator is currently busy
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
      _prefs.node_lat = ((double)lat) / 1000000.0;
      _prefs.node_lon = ((double)lon) / 1000000.0;
      savePrefs();
      writeOKFrame();
    } else {
      writeErrFrame();  // invalid geo coordinate
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
      writeErrFrame();
    }
  } else if (cmd_frame[0] == CMD_SEND_SELF_ADVERT) {
    auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
    if (pkt) {
      if (len >= 2 && cmd_frame[1] == 1) {   // optional param (1 = flood, 0 = zero hop)
        sendFlood(pkt);
      } else {
        sendZeroHop(pkt);
      }
      writeOKFrame();
    } else {
      writeErrFrame();
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
      writeErrFrame();  // unknown contact
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
        writeErrFrame();  // table is full!
      }
    }
  } else if (cmd_frame[0] == CMD_REMOVE_CONTACT) {
    uint8_t* pub_key = &cmd_frame[1];
    ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
    if (recipient && removeContact(*recipient)) {
      saveContacts();
      writeOKFrame();
    } else {
      writeErrFrame();  // not found, or unable to remove
    }
  } else if (cmd_frame[0] == CMD_SHARE_CONTACT) {
    uint8_t* pub_key = &cmd_frame[1];
    ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
    if (recipient && shareContactZeroHop(*recipient)) {
      writeOKFrame();
    } else {
      writeErrFrame();  // not found, or unable to send
    }
  } else if (cmd_frame[0] == CMD_EXPORT_CONTACT) {
    if (len < 1 + PUB_KEY_SIZE) {
      // export SELF
      auto pkt = createSelfAdvert(_prefs.node_name, _prefs.node_lat, _prefs.node_lon);
      if (pkt) {
        out_frame[0] = RESP_CODE_EXPORT_CONTACT;
        uint8_t out_len =  pkt->writeTo(&out_frame[1]);
        releasePacket(pkt);  // undo the obtainNewPacket()
        _serial->writeFrame(out_frame, out_len + 1);
      } else {
        writeErrFrame();  // Error
      }
    } else {
      uint8_t* pub_key = &cmd_frame[1];
      ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
      uint8_t out_len;
      if (recipient && (out_len = exportContact(*recipient, &out_frame[1])) > 0) {
        out_frame[0] = RESP_CODE_EXPORT_CONTACT;
        _serial->writeFrame(out_frame, out_len + 1);
      } else {
        writeErrFrame();  // not found
      }
    }
  } else if (cmd_frame[0] == CMD_IMPORT_CONTACT && len > 2+32+64) {
    if (importContact(&cmd_frame[1], len - 1)) {
      writeOKFrame();
    } else {
      writeErrFrame();
    }
  } else if (cmd_frame[0] == CMD_SYNC_NEXT_MESSAGE) {
    int out_len;
    if ((out_len = getFromOfflineQueue(out_frame)) > 0) {
      _serial->writeFrame(out_frame, out_len);
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

      _phy->setFrequency(_prefs.freq);
      _phy->setSpreadingFactor(_prefs.sf);
      _phy->setBandwidth(_prefs.bw);
      _phy->setCodingRate(_prefs.cr);
      MESH_DEBUG_PRINTLN("OK: CMD_SET_RADIO_PARAMS: f=%d, bw=%d, sf=%d, cr=%d", freq, bw, (uint32_t)sf, (uint32_t)cr);

      writeOKFrame();
    } else {
      MESH_DEBUG_PRINTLN("Error: CMD_SET_RADIO_PARAMS: f=%d, bw=%d, sf=%d, cr=%d", freq, bw, (uint32_t)sf, (uint32_t)cr);
      writeErrFrame();
    }
  } else if (cmd_frame[0] == CMD_SET_RADIO_TX_POWER) {
    if (cmd_frame[1] > MAX_LORA_TX_POWER) {
      writeErrFrame();
    } else {
      _prefs.tx_power_dbm = cmd_frame[1];
      savePrefs();
      _phy->setOutputPower(_prefs.tx_power_dbm);
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
  } else if (cmd_frame[0] == CMD_REBOOT && memcmp(&cmd_frame[1], "reboot", 6) == 0) {
    _board->reboot();
  } else if (cmd_frame[0] == CMD_GET_BATTERY_VOLTAGE) {
    uint8_t reply[3];
    reply[0] = RESP_CODE_BATTERY_VOLTAGE;
    uint16_t battery_millivolts = _board->getBattMilliVolts();
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
        writeErrFrame();
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
        writeErrFrame();
      }
    } else {
      writeErrFrame();  // flood, not supported (yet)
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
        writeErrFrame();
      } else {
        pending_status = 0;
        memcpy(&pending_login, recipient->id.pub_key, 4);  // match this to onContactResponse()
        out_frame[0] = RESP_CODE_SENT;
        out_frame[1] = (result == MSG_SEND_SENT_FLOOD) ? 1 : 0;
        memcpy(&out_frame[2], &pending_login, 4);
        memcpy(&out_frame[6], &est_timeout, 4);
        _serial->writeFrame(out_frame, 10);
      }
    } else {
      writeErrFrame();  // contact not found
    }
  } else if (cmd_frame[0] == CMD_SEND_STATUS_REQ && len >= 1+PUB_KEY_SIZE) {
    uint8_t* pub_key = &cmd_frame[1];
    ContactInfo* recipient = lookupContactByPubKey(pub_key, PUB_KEY_SIZE);
    if (recipient) {
      uint32_t est_timeout;
      int result = sendStatusRequest(*recipient, est_timeout);
      if (result == MSG_SEND_FAILED) {
        writeErrFrame();
      } else {
        pending_login = 0;
        memcpy(&pending_status, recipient->id.pub_key, 4);  // match this to onContactResponse()
        out_frame[0] = RESP_CODE_SENT;
        out_frame[1] = (result == MSG_SEND_SENT_FLOOD) ? 1 : 0;
        memcpy(&out_frame[2], &pending_status, 4);
        memcpy(&out_frame[6], &est_timeout, 4);
        _serial->writeFrame(out_frame, 10);
      }
    } else {
      writeErrFrame();  // contact not found
    }
  } else {
    writeErrFrame();
    MESH_DEBUG_PRINTLN("ERROR: unknown command: %02X", cmd_frame[0]);
  }
}

void BaseCompanionRadioMesh::loop() {
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
  }
}