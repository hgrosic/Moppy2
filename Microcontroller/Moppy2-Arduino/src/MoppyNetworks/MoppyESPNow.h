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
#include <stdint.h>

#define MOPPY_MAX_PACKET_LENGTH ESP_NOW_MAX_DATA_LEN

class MoppyESPNow {
public:
    MoppyESPNow(MoppyMessageConsumer *messageConsumer);
    void begin();
    void readMessages();

private:
    MoppyMessageConsumer * targetConsumer;
    static uint8_t messageBuffer[MOPPY_MAX_PACKET_LENGTH]; // Max message length for Moppy messages is 259
    static uint8_t messageLength;
    static bool newDataAvailable;                  // Flag if there is newdata to be processed
    static uint8_t gwMacAddress[6];
    const uint8_t pongBytes[8] = {START_BYTE, 0x00, 0x00, 0x04, 0x81, DEVICE_ADDRESS, MIN_SUB_ADDRESS, MAX_SUB_ADDRESS};
    void parseMessage(uint8_t message[], int length);
    void sendPong();
    static void onDataReceived(const uint8_t * macAddr, const uint8_t * incomingData, int dataLength);
    static void onDataSent(const uint8_t * macAddr, esp_now_send_status_t status);
};

#endif /* SRC_MOPPYNETWORKS_MOPPYESPNOW_H_ */
#endif /* ARDUINO_ARCH_ESP8266 or ARDUINO_ARCH_ESP32 */
