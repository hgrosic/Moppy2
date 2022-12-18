#include "MoppyESPNow.h"

#if !defined ARDUINO_ARCH_ESP8266 && !defined ARDUINO_ARCH_ESP32
#else

/*
 * Serial communications implementation for Arduino.  Instrument
 * has its handler functions called for device and system messages
 */
MoppyESPNow::MoppyESPNow(MoppyMessageConsumer *messageConsumer) {
    targetConsumer = messageConsumer;
}

void MoppyESPNow::begin() {
    // Serial for debugging
    Serial.begin(115200);
    // Set ESP32 as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    Serial.print("Detected MAC address of instrument: ");
    Serial.println(WiFi.macAddress());
    // Initilize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Initializing ESP-NOW failed");
        return;
    }
    // Register callback functions
    using namespace std::placeholders;
    // Using "bind" because in order to adapt member method for free function passing
    if (esp_now_register_recv_cb(std::bind(&MoppyESPNow::onDataReceived, *this, _1, _2, _3)) != ESP_OK) {
        Serial.println("Registering ESP-NOW Receive Callback failed");
        return;
    }
    Serial.println("Registering ESP-NOW Receive Callback successful");
    if (esp_now_register_send_cb(std::bind(&MoppyESPNow::onDataSent, *this, _1, _2)) != ESP_OK) {
        Serial.println("Registering ESP-NOW Send Callback failed");
        return;
    }
    Serial.println("Registering ESP-NOW Send Callback successful");
}

// Callback function executed when data is received
void MoppyESPNow::onDataReceived(const uint8_t * macAddr, const uint8_t * incomingData, int dataLength) {
    newDataAvailable = true;
    memcpy(gwMacAddress, macAddr, 6);
    messageLength = min(dataLength, MOPPY_MAX_PACKET_LENGTH);
    memcpy(messageBuffer, incomingData, messageLength);
}

// Callback function executed when data is sent
void MoppyESPNow::onDataSent(const uint8_t * macAddr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("Pong response could not be sent");
    }
}

void MoppyESPNow::readMessages() {
    if (newDataAvailable) {
        // Parse
        parseMessage(messageBuffer, messageLength);
    }
}

/* MoppyMessages contain the following bytes:
 *  0    - START_BYTE (always 0x4d)
 *  1    - Device address (0x00 for system-wide messages)
 *  2    - Sub address (Ignored for system-wide messages)
 *  3    - Size of message body (number of bytes following this one)
 *  4    - Command byte
 *  5... - Optional payload
 */
void MoppyESPNow::parseMessage(uint8_t message[], int length) {
    if (length < 5 || message[0] != START_BYTE || length != (4 + message[3])) {
        return; // Message is too short, not a Moppy Message, or wrongly sized
    }

    // Only worry about this if it's addressed to us
    if (message[1] == SYSTEM_ADDRESS) {
        if (messageBuffer[4] == NETBYTE_SYS_PING) {
            sendPong(); // Respond with pong if requested
        } else {
            targetConsumer->handleSystemMessage(messageBuffer[4], &messageBuffer[5]);
        }
    } else if (message[1] == DEVICE_ADDRESS) {
        targetConsumer->handleDeviceMessage(messageBuffer[2], messageBuffer[4], &messageBuffer[5]);
    }
}

void MoppyESPNow::sendPong() {
    esp_now_peer_info_t gwPeerInfo = {};
    memcpy(&gwPeerInfo.peer_addr, gwMacAddress, 6);
    if (!esp_now_is_peer_exist(gwMacAddress)) {
      esp_now_add_peer(&gwPeerInfo);
    }
    esp_err_t result = esp_now_send(gwMacAddress, pongBytes, 8);
    //if (result == ESP_OK)
    //{
    //  Serial.println("Broadcast message success");
    //}
    //else if (result == ESP_ERR_ESPNOW_NOT_INIT)
    //{
    //  Serial.println("ESPNOW not Init.");
    //}
    //else if (result == ESP_ERR_ESPNOW_ARG)
    //{
    //  Serial.println("Invalid Argument");
    //}
    //else if (result == ESP_ERR_ESPNOW_INTERNAL)
    //{
    //  Serial.println("Internal Error");
    //}
    //else if (result == ESP_ERR_ESPNOW_NO_MEM)
    //{
    //  Serial.println("ESP_ERR_ESPNOW_NO_MEM");
    //}
    //else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
    //{
    //  Serial.println("Peer not found.");
    //}
    //else
    //{
    //  Serial.println("Unknown error");
    //}
}

#endif /* ARDUINO_ARCH_ESP8266 or ARDUINO_ARCH_ESP32 */
