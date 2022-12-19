#include "MoppyESPNowGateway.h"

#if !defined ARDUINO_ARCH_ESP8266 && !defined ARDUINO_ARCH_ESP32
#else

/*
 * ESP-Now gateway implementation. Data incomming on serial is relayed
 * as ESP-Now broadcasts.
 */
MoppyESPNowGateway::MoppyESPNowGateway() {
}

void MoppyESPNowGateway::begin() {
    Serial.begin(115200);
    // Set ESP32 as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    //Serial.print("Detected MAC address of gateway: ");
    //Serial.println(WiFi.macAddress());
    // Initilize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        //Serial.println("Initializing ESP-NOW failed");
        return;
    }
    // Register callback functions
    if (esp_now_register_recv_cb(&MoppyESPNowGateway::onDataReceived) != ESP_OK) {
        //Serial.println("Registering ESP-NOW Receive Callback failed");
        return;
    }
    //Serial.println("Registering ESP-NOW Receive Callback successful");
    if (esp_now_register_send_cb(&MoppyESPNowGateway::onDataSent) != ESP_OK) {
        //Serial.println("Registering ESP-NOW Send Callback failed");
        return;
    }
    //Serial.println("Registering ESP-NOW Send Callback successful");
    //Serial.println("Gateway is up and running");
}

// Callback function executed when data is received
void MoppyESPNowGateway::onDataReceived(const uint8_t * macAddr, const uint8_t * incomingData, int dataLength) {
   // Directly relay message to serial
   Serial.write(incomingData, dataLength);
}

// Callback function executed when data is sent
void MoppyESPNowGateway::onDataSent(const uint8_t * macAddr, esp_now_send_status_t status) {
    //if (status != ESP_NOW_SEND_SUCCESS) {
    //    Serial.println("Broadcast message could not be sent");
    //}
}

/* MoppyMessages contain the following bytes:
 *  0    - START_BYTE (always 0x4d)
 *  1    - Device address (0x00 for system-wide messages)
 *  2    - Sub address (Ignored for system-wide messages)
 *  3    - Size of message body (number of bytes following this one)
 *  4    - Command byte
 *  5... - Optional payload
 */
void MoppyESPNowGateway::readMessages() {
    // If we're waiting for position 4, then we know how many bytes we're waiting for, no need
    // to start reading until they're all there.
    // TODO: This will break for large messages because the Arduino buffer size is only 64 bytes.
    // This should be optimized a bit.
    while ((messagePos != 4 && Serial.available()) || (messagePos == 4 && Serial.available() >= messageBuffer[3])) {
        switch (messagePos) {
        case 0:
            if (Serial.read() == START_BYTE) {
                messageBuffer[0] = START_BYTE;
                messagePos++;
            }
            break;
        case 1:
        case 2:
        case 3:
            messageBuffer[messagePos] = Serial.read();
            messagePos++;
            break;
        case 4:
            // Read command and payload
            Serial.readBytes(messageBuffer + 4, messageBuffer[3]);
            // Broadcast message over ESP-Now
            broadcastESPNowMessage();

            messagePos = 0; // Start looking for a new message
        }
    }
}

void MoppyESPNowGateway::broadcastESPNowMessage() {
    esp_now_peer_info_t broadcastPeerInfo = {};
    memcpy(&broadcastPeerInfo.peer_addr, broadcastMacAddress, 6);
    if (!esp_now_is_peer_exist(broadcastMacAddress)) {
      esp_now_add_peer(&broadcastPeerInfo);
    }
    esp_now_send(broadcastMacAddress, messageBuffer, 4 + messageBuffer[3]);
}

#endif /* ARDUINO_ARCH_ESP8266 or ARDUINO_ARCH_ESP32 */
