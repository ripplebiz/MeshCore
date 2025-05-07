#include <Arduino.h>
#include "CommonCLI.h"
#include "TxtDataHelpers.h"
#include <RTClib.h>

// Believe it or not, this std C function is busted on some platforms!
static uint32_t _atoi(const char* sp) {
  uint32_t n = 0;
  while (*sp && *sp >= '0' && *sp <= '9') {
    n *= 10;
    n += (*sp++ - '0');
  }
  return n;
}

void CommonCLI::loadPrefs(FILESYSTEM* fs) {
  if (fs->exists("/com_prefs")) {
    loadPrefsInt(fs, "/com_prefs");   // new filename
  } else if (fs->exists("/node_prefs")) {
    loadPrefsInt(fs, "/node_prefs");
    savePrefs(fs);  // save to new filename
    fs->remove("/node_prefs");  // remove old
  }
}

void CommonCLI::loadPrefsInt(FILESYSTEM* fs, const char* filename) {
#if defined(RP2040_PLATFORM)
  File file = fs->open(filename, "r");
#else
  File file = fs->open(filename);
#endif
  if (file) {
    uint8_t pad[8];

    file.read((uint8_t *) &_prefs->airtime_factor, sizeof(_prefs->airtime_factor));  // 0
    file.read((uint8_t *) &_prefs->node_name, sizeof(_prefs->node_name));  // 4
    file.read(pad, 4);   // 36
    file.read((uint8_t *) &_prefs->node_lat, sizeof(_prefs->node_lat));  // 40
    file.read((uint8_t *) &_prefs->node_lon, sizeof(_prefs->node_lon));  // 48
    file.read((uint8_t *) &_prefs->password[0], sizeof(_prefs->password));  // 56
    file.read((uint8_t *) &_prefs->freq, sizeof(_prefs->freq));   // 72
    file.read((uint8_t *) &_prefs->tx_power_dbm, sizeof(_prefs->tx_power_dbm));  // 76
    file.read((uint8_t *) &_prefs->disable_fwd, sizeof(_prefs->disable_fwd));  // 77
    file.read((uint8_t *) &_prefs->advert_interval, sizeof(_prefs->advert_interval));  // 78
    file.read((uint8_t *) pad, 1);  // 79  was 'unused'
    file.read((uint8_t *) &_prefs->rx_delay_base, sizeof(_prefs->rx_delay_base));  // 80
    file.read((uint8_t *) &_prefs->tx_delay_factor, sizeof(_prefs->tx_delay_factor));  // 84
    file.read((uint8_t *) &_prefs->guest_password[0], sizeof(_prefs->guest_password));  // 88
    file.read((uint8_t *) &_prefs->direct_tx_delay_factor, sizeof(_prefs->direct_tx_delay_factor));  // 104
    file.read(pad, 4);   // 108
    file.read((uint8_t *) &_prefs->sf, sizeof(_prefs->sf));  // 112
    file.read((uint8_t *) &_prefs->cr, sizeof(_prefs->cr));  // 113
    file.read((uint8_t *) &_prefs->allow_read_only, sizeof(_prefs->allow_read_only));  // 114
    file.read((uint8_t *) &_prefs->reserved2, sizeof(_prefs->reserved2));  // 115
    file.read((uint8_t *) &_prefs->bw, sizeof(_prefs->bw));  // 116
    file.read(pad, 4);   // 120
    file.read((uint8_t *) &_prefs->flood_max, sizeof(_prefs->flood_max));   // 124
    file.read((uint8_t *) &_prefs->flood_advert_interval, sizeof(_prefs->flood_advert_interval));  // 125

    // sanitise bad pref values
    _prefs->rx_delay_base = constrain(_prefs->rx_delay_base, 0, 20.0f);
    _prefs->tx_delay_factor = constrain(_prefs->tx_delay_factor, 0, 2.0f);
    _prefs->direct_tx_delay_factor = constrain(_prefs->direct_tx_delay_factor, 0, 2.0f);
    _prefs->airtime_factor = constrain(_prefs->airtime_factor, 0, 9.0f);
    _prefs->freq = constrain(_prefs->freq, 400.0f, 2500.0f);
    _prefs->bw = constrain(_prefs->bw, 62.5f, 500.0f);
    _prefs->sf = constrain(_prefs->sf, 7, 12);
    _prefs->cr = constrain(_prefs->cr, 5, 8);
    _prefs->tx_power_dbm = constrain(_prefs->tx_power_dbm, 1, 30);

    file.close();
  }
}

void CommonCLI::savePrefs(FILESYSTEM* fs) {
#if defined(NRF52_PLATFORM)
  File file = fs->open("/com_prefs", FILE_O_WRITE);
  if (file) { file.seek(0); file.truncate(); }
#elif defined(RP2040_PLATFORM)
  File file = fs->open("/com_prefs", "w");
#else
  File file = fs->open("/com_prefs", "w", true);
#endif
  if (file) {
    uint8_t pad[8];
    memset(pad, 0, sizeof(pad));

    file.write((uint8_t *) &_prefs->airtime_factor, sizeof(_prefs->airtime_factor));  // 0
    file.write((uint8_t *) &_prefs->node_name, sizeof(_prefs->node_name));  // 4
    file.write(pad, 4);   // 36
    file.write((uint8_t *) &_prefs->node_lat, sizeof(_prefs->node_lat));  // 40
    file.write((uint8_t *) &_prefs->node_lon, sizeof(_prefs->node_lon));  // 48
    file.write((uint8_t *) &_prefs->password[0], sizeof(_prefs->password));  // 56
    file.write((uint8_t *) &_prefs->freq, sizeof(_prefs->freq));   // 72
    file.write((uint8_t *) &_prefs->tx_power_dbm, sizeof(_prefs->tx_power_dbm));  // 76
    file.write((uint8_t *) &_prefs->disable_fwd, sizeof(_prefs->disable_fwd));  // 77
    file.write((uint8_t *) &_prefs->advert_interval, sizeof(_prefs->advert_interval));  // 78
    file.write((uint8_t *) pad, 1);  // 79  was 'unused'
    file.write((uint8_t *) &_prefs->rx_delay_base, sizeof(_prefs->rx_delay_base));  // 80
    file.write((uint8_t *) &_prefs->tx_delay_factor, sizeof(_prefs->tx_delay_factor));  // 84
    file.write((uint8_t *) &_prefs->guest_password[0], sizeof(_prefs->guest_password));  // 88
    file.write((uint8_t *) &_prefs->direct_tx_delay_factor, sizeof(_prefs->direct_tx_delay_factor));  // 104
    file.write(pad, 4);   // 108
    file.write((uint8_t *) &_prefs->sf, sizeof(_prefs->sf));  // 112
    file.write((uint8_t *) &_prefs->cr, sizeof(_prefs->cr));  // 113
    file.write((uint8_t *) &_prefs->allow_read_only, sizeof(_prefs->allow_read_only));  // 114
    file.write((uint8_t *) &_prefs->reserved2, sizeof(_prefs->reserved2));  // 115
    file.write((uint8_t *) &_prefs->bw, sizeof(_prefs->bw));  // 116
    file.write(pad, 4);   // 120
    file.write((uint8_t *) &_prefs->flood_max, sizeof(_prefs->flood_max));   // 124
    file.write((uint8_t *) &_prefs->flood_advert_interval, sizeof(_prefs->flood_advert_interval));  // 125

    file.close();
  }
}

#define MIN_LOCAL_ADVERT_INTERVAL   60

void CommonCLI::checkAdvertInterval() {
  if (_prefs->advert_interval * 2 < MIN_LOCAL_ADVERT_INTERVAL) {
    _prefs->advert_interval = 0;  // turn it off, now that device has been manually configured
  }
}

void CommonCLI::handleCommand(uint32_t sender_timestamp, const char* command, char* reply) {
    while (*command == ' ') command++;   // skip leading spaces

    if (strlen(command) > 4 && command[2] == '|') {  // optional prefix (for companion radio CLI)
      memcpy(reply, command, 3);  // reflect the prefix back
      reply += 3;
      command += 3;
    }

    if (memcmp(command, "reboot", 6) == 0) {
      _board->reboot();  // doesn't return
    } else if (memcmp(command, "advert", 6) == 0) {
      _callbacks->sendSelfAdvertisement(400);
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
      if (!_board->startOTAUpdate(_prefs->node_name, reply)) {
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
    } else if (memcmp(command, "neighbors", 9) == 0) {
      _callbacks->formatNeighborsReply(reply);
    } else if (memcmp(command, "password ", 9) == 0) {
      // change admin password
      StrHelper::strncpy(_prefs->password, &command[9], sizeof(_prefs->password));
      checkAdvertInterval();
      savePrefs();
      sprintf(reply, "password now: %s", _prefs->password);   // echo back just to let admin know for sure!!
    } else if (memcmp(command, "get ", 4) == 0) {
      const char* config = &command[4];
      if (memcmp(config, "af", 2) == 0) {
        sprintf(reply, "> %s", StrHelper::ftoa(_prefs->airtime_factor));
      } else if (memcmp(config, "allow.read.only", 15) == 0) {
        sprintf(reply, "> %s", _prefs->allow_read_only ? "on" : "off");
      } else if (memcmp(config, "flood.advert.interval", 21) == 0) {
        sprintf(reply, "> %d", ((uint32_t) _prefs->flood_advert_interval));
      } else if (memcmp(config, "advert.interval", 15) == 0) {
        sprintf(reply, "> %d", ((uint32_t) _prefs->advert_interval) * 2);
      } else if (memcmp(config, "guest.password", 14) == 0) {
        sprintf(reply, "> %s", _prefs->guest_password);
      } else if (memcmp(config, "name", 4) == 0) {
        sprintf(reply, "> %s", _prefs->node_name);
      } else if (memcmp(config, "repeat", 6) == 0) {
        sprintf(reply, "> %s", _prefs->disable_fwd ? "off" : "on");
      } else if (memcmp(config, "lat", 3) == 0) {
        sprintf(reply, "> %s", StrHelper::ftoa(_prefs->node_lat));
      } else if (memcmp(config, "lon", 3) == 0) {
        sprintf(reply, "> %s", StrHelper::ftoa(_prefs->node_lon));
      } else if (memcmp(config, "radio", 5) == 0) {
        char freq[16], bw[16];
        strcpy(freq, StrHelper::ftoa(_prefs->freq));
        strcpy(bw, StrHelper::ftoa(_prefs->bw));
        sprintf(reply, "> %s,%s,%d,%d", freq, bw, (uint32_t)_prefs->sf, (uint32_t)_prefs->cr);
      } else if (memcmp(config, "rxdelay", 7) == 0) {
        sprintf(reply, "> %s", StrHelper::ftoa(_prefs->rx_delay_base));
      } else if (memcmp(config, "txdelay", 7) == 0) {
        sprintf(reply, "> %s", StrHelper::ftoa(_prefs->tx_delay_factor));
      } else if (memcmp(config, "flood.max", 9) == 0) {
        sprintf(reply, "> %d", (uint32_t)_prefs->flood_max);
      } else if (memcmp(config, "direct.txdelay", 14) == 0) {
        sprintf(reply, "> %s", StrHelper::ftoa(_prefs->direct_tx_delay_factor));
      } else if (memcmp(config, "tx", 2) == 0 && (config[2] == 0 || config[2] == ' ')) {
        sprintf(reply, "> %d", (uint32_t) _prefs->tx_power_dbm);
      } else if (memcmp(config, "freq", 4) == 0) {
        sprintf(reply, "> %s", StrHelper::ftoa(_prefs->freq));
      } else if (memcmp(config, "public.key", 10) == 0) {
        strcpy(reply, "> ");
        mesh::Utils::toHex(&reply[2], _callbacks->getSelfIdPubKey(), PUB_KEY_SIZE);
      } else if (memcmp(config, "role", 4) == 0) {
        sprintf(reply, "> %s", _callbacks->getRole());
      } else {
        sprintf(reply, "??: %s", config);
      }
    } else if (memcmp(command, "set ", 4) == 0) {
      const char* config = &command[4];
      if (memcmp(config, "af ", 3) == 0) {
        _prefs->airtime_factor = atof(&config[3]);
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "allow.read.only ", 16) == 0) {
        _prefs->allow_read_only = memcmp(&config[16], "on", 2) == 0;
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "flood.advert.interval ", 22) == 0) {
        int hours = _atoi(&config[22]);
        if (hours > 0 && hours < 3) {
          sprintf(reply, "Error: min is 3 hours");
        } else if (hours > 48) {
          strcpy(reply, "Error: max is 48 hours");
        } else {
          _prefs->flood_advert_interval = (uint8_t)(hours);
          _callbacks->updateFloodAdvertTimer();
          savePrefs();
          strcpy(reply, "OK");
        }
      } else if (memcmp(config, "advert.interval ", 16) == 0) {
        int mins = _atoi(&config[16]);
        if (mins > 0 && mins < MIN_LOCAL_ADVERT_INTERVAL) {
          sprintf(reply, "Error: min is %d mins", MIN_LOCAL_ADVERT_INTERVAL);
        } else if (mins > 240) {
          strcpy(reply, "Error: max is 240 mins");
        } else {
          _prefs->advert_interval = (uint8_t)(mins / 2);
          _callbacks->updateAdvertTimer();
          savePrefs();
          strcpy(reply, "OK");
        }
      } else if (memcmp(config, "guest.password ", 15) == 0) {
        StrHelper::strncpy(_prefs->guest_password, &config[15], sizeof(_prefs->guest_password));
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "name ", 5) == 0) {
        StrHelper::strncpy(_prefs->node_name, &config[5], sizeof(_prefs->node_name));
        checkAdvertInterval();
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "repeat ", 7) == 0) {
        _prefs->disable_fwd = memcmp(&config[7], "off", 3) == 0;
        savePrefs();
        strcpy(reply, _prefs->disable_fwd ? "OK - repeat is now OFF" : "OK - repeat is now ON");
      } else if (memcmp(config, "radio ", 6) == 0) {
        strcpy(tmp, &config[6]);
        const char *parts[4];
        int num = mesh::Utils::parseTextParts(tmp, parts, 4);
        float freq  = num > 0 ? atof(parts[0]) : 0.0f;
        float bw    = num > 1 ? atof(parts[1]) : 0.0f;
        uint8_t sf  = num > 2 ? atoi(parts[2]) : 0;
        uint8_t cr  = num > 3 ? atoi(parts[3]) : 0;
        if (freq >= 300.0f && freq <= 2500.0f && sf >= 7 && sf <= 12 && cr >= 5 && cr <= 8 && bw >= 7.0f && bw <= 500.0f) {
          _prefs->sf = sf;
          _prefs->cr = cr;
          _prefs->freq = freq;
          _prefs->bw = bw;
          _callbacks->savePrefs();
          strcpy(reply, "OK - reboot to apply");
        } else {
          strcpy(reply, "Error, invalid radio params");
        }
      } else if (memcmp(config, "lat ", 4) == 0) {
        _prefs->node_lat = atof(&config[4]);
        checkAdvertInterval();
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "lon ", 4) == 0) {
        _prefs->node_lon = atof(&config[4]);
        checkAdvertInterval();
        savePrefs();
        strcpy(reply, "OK");
      } else if (memcmp(config, "rxdelay ", 8) == 0) {
        float db = atof(&config[8]);
        if (db >= 0) {
          _prefs->rx_delay_base = db;
          savePrefs();
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Error, cannot be negative");
        }
      } else if (memcmp(config, "txdelay ", 8) == 0) {
        float f = atof(&config[8]);
        if (f >= 0) {
          _prefs->tx_delay_factor = f;
          savePrefs();
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Error, cannot be negative");
        }
      } else if (memcmp(config, "flood.max ", 10) == 0) {
        uint8_t m = atoi(&config[10]);
        if (m <= 64) {
          _prefs->flood_max = m;
          savePrefs();
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Error, max 64");
        }
      } else if (memcmp(config, "direct.txdelay ", 15) == 0) {
        float f = atof(&config[15]);
        if (f >= 0) {
          _prefs->direct_tx_delay_factor = f;
          savePrefs();
          strcpy(reply, "OK");
        } else {
          strcpy(reply, "Error, cannot be negative");
        }
      } else if (memcmp(config, "tx ", 3) == 0) {
        _prefs->tx_power_dbm = atoi(&config[3]);
        savePrefs();
        _callbacks->setTxPower(_prefs->tx_power_dbm);
        strcpy(reply, "OK");
      } else if (sender_timestamp == 0 && memcmp(config, "freq ", 5) == 0) {
        _prefs->freq = atof(&config[5]);
        savePrefs();
        strcpy(reply, "OK - reboot to apply");
      } else {
        sprintf(reply, "unknown config: %s", config);
      }
    } else if (sender_timestamp == 0 && strcmp(command, "erase") == 0) {
      bool s = _callbacks->formatFileSystem();
      sprintf(reply, "File system erase: %s", s ? "OK" : "Err");
    } else if (memcmp(command, "ver", 3) == 0) {
      sprintf(reply, "%s (Build: %s)", _callbacks->getFirmwareVer(), _callbacks->getBuildDate());
    } else if (memcmp(command, "log start", 9) == 0) {
      _callbacks->setLoggingOn(true);
      strcpy(reply, "   logging on");
    } else if (memcmp(command, "log stop", 8) == 0) {
      _callbacks->setLoggingOn(false);
      strcpy(reply, "   logging off");
    } else if (memcmp(command, "log erase", 9) == 0) {
      _callbacks->eraseLogFile();
      strcpy(reply, "   log erased");
    } else if (sender_timestamp == 0 && memcmp(command, "log", 3) == 0) {
      _callbacks->dumpLogFile();
      strcpy(reply, "   EOF");
    } else {
      sprintf(reply, "Unknown: %s", command);
    }
}
