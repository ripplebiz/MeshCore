#include "SerialBLEInterface.h"

// Unfortunately Bluefuit setCallback functions don't support setting a userdata void* on a callback,
// so need to store the instance of the BLE interface globally.
SerialBLEInterface* instance = NULL;

void SerialBLEInterface::begin(const char* device_name, uint32_t pin_code) {
  char charpin[20];
  sprintf(charpin, "%d", pin_code);

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.configPrphConn(250, BLE_GAP_EVENT_LENGTH_MIN, 16, 16);  // increase MTU
  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  Bluefruit.setName(device_name);

  Bluefruit.Security.setMITM(true);
  Bluefruit.Security.setPIN(charpin);

  // Configure and start the BLE Uart service
  bleuart.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  bleuart.begin();

  /* Configure BLE Advertising
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 15 will stop advertising after 15s or until connected
   *
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);
  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.setInterval(32, 244);    // advertisement interval (fast, slow) in unit of 0.625 ms (20ms, 152.5ms)
  Bluefruit.Advertising.setFastTimeout(30);      // 30 seconds in fast mode

  // Configure connect/disconnect/advertise callbacks
  instance = this; // Yikes, see note above
  Bluefruit.Periph.setConnectCallback(&SerialBLEInterface::device_connected_callback);
  Bluefruit.Periph.setDisconnectCallback(&SerialBLEInterface::device_disconnected_callback);
  Bluefruit.Advertising.setStopCallback(&SerialBLEInterface::adv_timeout_callback);

  // To be consistent OTA DFU should be added first if it exists
  //bledfu.begin();
}

// ---------- public methods

void SerialBLEInterface::enable() { 
  if (_isEnabled) return;
  BLE_DEBUG_PRINTLN("SerialBLEInterface::enable");
  _isEnabled = true;
  clearBuffers();

  // Start advertising
  startAdv();
}

void SerialBLEInterface::disable() {
  _isEnabled = false;
  instance->clearBuffers();
  BLE_DEBUG_PRINTLN("SerialBLEInterface::disable");

  stopAdv();
}

size_t SerialBLEInterface::writeFrame(const uint8_t src[], size_t len) {
  if (len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("writeFrame(), frame too big, len=%d", len);
    return 0;
  }

  if (_deviceConnected && len > 0) {
    if (send_queue_len >= FRAME_QUEUE_SIZE) {
      BLE_DEBUG_PRINTLN("writeFrame(), send_queue is full!");
      return 0;
    }

    send_queue[send_queue_len].len = len;  // add to send queue
    memcpy(send_queue[send_queue_len].buf, src, len);
    send_queue_len++;

    return len;
  }
  return 0;
}

#define  BLE_WRITE_MIN_INTERVAL   20

bool SerialBLEInterface::isWriteBusy() const {
  return millis() < _last_write + BLE_WRITE_MIN_INTERVAL;   // still too soon to start another write?
}

size_t SerialBLEInterface::checkRecvFrame(uint8_t dest[]) {
  if (send_queue_len > 0   // first, check send queue
    && millis() >= _last_write + BLE_WRITE_MIN_INTERVAL    // space the writes apart
  ) {
    _last_write = millis();
    bleuart.write(send_queue[0].buf, send_queue[0].len);
    BLE_DEBUG_PRINTLN("writeBytes: sz=%d, hdr=%d", (uint32_t)send_queue[0].len, (uint32_t) send_queue[0].buf[0]);

    send_queue_len--;
    for (int i = 0; i < send_queue_len; i++) {   // delete top item from queue
      send_queue[i] = send_queue[i + 1];
    }
  } else {
    int len = bleuart.available();
    if (len > 0) {
      bleuart.readBytes(dest, len);
      BLE_DEBUG_PRINTLN("readBytes: sz=%d, hdr=%d", len, (uint32_t) dest[0]);
      return len;
    }
  }

  return 0;
}

bool SerialBLEInterface::isConnected() const {
  return _deviceConnected;  //pServer != NULL && pServer->getConnectedCount() > 0;
}

void SerialBLEInterface::startAdv() {
  if (Bluefruit.Advertising.isRunning()) {
    return;
  }

  BLE_DEBUG_PRINTLN("SerialBLEInterface -> starting advertising");

  Bluefruit.Advertising.start(60);               // 60 = Stop advertising after 60 seconds
}

void SerialBLEInterface::stopAdv() {
  if (!Bluefruit.Advertising.isRunning()) {
    return;
  }

  BLE_DEBUG_PRINTLN("SerialBLEInterface -> stopping advertising");
  Bluefruit.Advertising.stop();
}

void SerialBLEInterface::device_connected_callback(uint16_t conn_hdl) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface -> connected");
  instance->_deviceConnected = true;
}

void SerialBLEInterface::device_disconnected_callback(uint16_t conn_hdl, uint8_t reason) {
  BLE_DEBUG_PRINTLN("SerialBLEInterface -> disconnected");
  instance->_deviceConnected = false;
}

void SerialBLEInterface::adv_timeout_callback() {
  BLE_DEBUG_PRINTLN("SerialBLEInterface::ble_handler -> adv terminated timeout");
  instance->disable();
}
