/*
 * MoppyESPNow.h
 *
 */
#if !defined ARDUINO_ARCH_ESP8266 && !defined ARDUINO_ARCH_ESP32
// This will only work with ESP8266 or ESP32
#else
#ifndef SRC_MOPPYNETWORKS_MOPPYESPNOW_H_
#define SRC_MOPPYNETWORKS_MOPPYESPNOW_H_

#include "../MoppyConfig.h"
#include "../MoppyMessageConsumer.h"
#include "Arduino.h"
#include "MoppyNetwork.h"
#ifdef ARDUINO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#elif ARDUINO_ARCH_ESP32
#include <WiFi.h>
#endif
#include <esp_now.h>
#include <esp_wifi.h>
#include <stdint.h>
#include <queue>

#define MOPPY_MAX_PACKET_LENGTH ESP_NOW_MAX_DATA_LEN
#define MOPPY_WIFI_CHANNEL 1

class MoppyESPNow {
public:
    MoppyESPNow(MoppyMessageConsumer *messageConsumer);
    void begin();
    void readMessages();

private:
    MoppyMessageConsumer * targetConsumer;
    static std::queue<uint8_t> messageQueue;                // Queue for the incoming messages
    static uint8_t messageBuffer[MOPPY_MAX_PACKET_LENGTH];  // Buffer for the current message
    uint8_t messagePos = 0;                                 // Tracks current message read position 
    volatile static bool sendingCompleted;                  // Signalizes that new data can be sent
    static uint8_t gwMacAddress[6];                // MAC Address of the ESP-Now gateway to which to respond to
    const uint8_t pongBytes[8] = {START_BYTE, 0x00, 0x00, 0x04, 0x81, DEVICE_ADDRESS, MIN_SUB_ADDRESS, MAX_SUB_ADDRESS};
    void sendPong();
    static void onDataReceived(const uint8_t * macAddr, const uint8_t * incomingData, int dataLength);
    static void onDataSent(const uint8_t * macAddr, esp_now_send_status_t status);
};

#endif /* SRC_MOPPYNETWORKS_MOPPYESPNOW_H_ */
#endif /* ARDUINO_ARCH_ESP8266 or ARDUINO_ARCH_ESP32 */
