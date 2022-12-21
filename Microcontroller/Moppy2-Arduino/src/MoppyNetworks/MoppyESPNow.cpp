#include "MoppyESPNow.h"

#if !defined ARDUINO_ARCH_ESP8266 && !defined ARDUINO_ARCH_ESP32
#else
/*
 * ESP-Now communication implementation for ESP8266/ESP32 devices.  
 * Instrument has its handler functions called for device and system messages
 */
std::queue<uint8_t> MoppyESPNow::messageQueue; // Queue for the incoming messages
uint8_t MoppyESPNow::messageBuffer[MOPPY_MAX_PACKET_LENGTH]; // Buffer for the current message
volatile bool MoppyESPNow::sendingCompleted = true; // Signalizes that new data can be sent
uint8_t MoppyESPNow::gwMacAddress[6]; // MAC Address of the ESP-Now gateway to which to respond to

MoppyESPNow::MoppyESPNow(MoppyMessageConsumer *messageConsumer) {
    targetConsumer = messageConsumer;
}

void MoppyESPNow::begin() {
    // Serial for debugging
    Serial.begin(115200);
    // Initialize WiFi stack
    WiFi.mode(WIFI_STA);
    // Disable sleep mode
    WiFi.setSleep(false);
    Serial.print("Detected MAC address of instrument: ");
    Serial.println(WiFi.macAddress());
    // Change WiFi channel
    if (esp_wifi_set_channel(MOPPY_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        Serial.println("ERROR - Changing WiFi channel failed");
        return;
    }
    // Initilize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ERROR - Initializing ESP-NOW failed");
        return;
    }
    // Register callback functions
    if (esp_now_register_send_cb(&MoppyESPNow::onDataSent) != ESP_OK) {
        Serial.println("ERROR - Registering ESP-NOW OnSent Callback failed");
        return;
    }
    if (esp_now_register_recv_cb(&MoppyESPNow::onDataReceived) != ESP_OK) {
        Serial.println("ERROR - Registering ESP-NOW OnReceived Callback failed");
        return;
    }
    Serial.println("SUCCES - Instrument is up and running");
}

// Callback function executed when data is received
void MoppyESPNow::onDataReceived(const uint8_t * macAddr, const uint8_t * incomingData, int dataLength) {
    memcpy(gwMacAddress, macAddr, 6); // The message comes from the gateway
    for (uint8_t i = 0; i < dataLength; i++)
    {
        messageQueue.push(incomingData[i]);
    }
}

// Callback function executed when data is sent
void MoppyESPNow::onDataSent(const uint8_t * macAddr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        Serial.println("ERROR - Pong response could not be sent");
    }
    sendingCompleted = true;
}

/* MoppyMessages contain the following bytes:
 *  0    - START_BYTE (always 0x4d)
 *  1    - Device address (0x00 for system-wide messages)
 *  2    - Sub address (Ignored for system-wide messages)
 *  3    - Size of message body (number of bytes following this one)
 *  4    - Command byte
 *  5... - Optional payload
 */
void MoppyESPNow::readMessages() {
    // If we're waiting for position 4, then we know how many bytes we're waiting for, no need
    // to start reading until they're all there.
    while ((messagePos != 4 && !messageQueue.empty()) || (messagePos == 4 && messageQueue.size() >= messageBuffer[3])) {
        switch (messagePos) {
        case 0:
            if (messageQueue.front() == START_BYTE) {
                messageBuffer[messagePos] = messageQueue.front();
                messagePos++;
            }
            messageQueue.pop();
            break;
        case 1:
            if (messageQueue.front() == SYSTEM_ADDRESS || messageQueue.front() == DEVICE_ADDRESS) {
                messageBuffer[messagePos] = messageQueue.front();
                messagePos++; 
            }
            else {
                messagePos = 0; // This message isn't for us
            }
            messageQueue.pop();
            break;
        case 2:
            if (messageQueue.front() == 0x00 || (messageQueue.front() >= MIN_SUB_ADDRESS && messageQueue.front() <= MAX_SUB_ADDRESS)) {
                messageBuffer[messagePos] = messageQueue.front();
                messagePos++; // Valid subAddress, continue
            }
            else {
                messagePos = 0; // Not listening to this subAddress, skip this message
            }
            messageQueue.pop();
            break;
        case 3:
            messageBuffer[messagePos] = messageQueue.front();
            messagePos++;
            messageQueue.pop();
            break;
        case 4:
            // Read command and payload
            for (uint8_t i = 0; i < messageBuffer[3]; i++) {
                messageBuffer[messagePos + i] = messageQueue.front();
                messageQueue.pop();
            }
            // Call appropriate handler
            if (messageBuffer[1] == SYSTEM_ADDRESS) {
                if (messageBuffer[4] == NETBYTE_SYS_PING) {
                    sendPong(); // Respond with pong if requested
                } 
                else {
                    targetConsumer->handleSystemMessage(messageBuffer[4], &messageBuffer[5]);
                }
            } 
            else {
                targetConsumer->handleDeviceMessage(messageBuffer[2], messageBuffer[4], &messageBuffer[5]);
            }
            messagePos = 0; // Start looking for a new message in the queue
        }
    }
}

void MoppyESPNow::sendPong() {
    if (!esp_now_is_peer_exist(gwMacAddress)) {
        esp_now_peer_info_t gwPeerInfo;
        memcpy(&gwPeerInfo.peer_addr, gwMacAddress, 6);
        gwPeerInfo.channel = MOPPY_WIFI_CHANNEL;  
        gwPeerInfo.encrypt = false;
        if (esp_now_add_peer(&gwPeerInfo) != ESP_OK){
            //Serial.println("ERROR - Adding gateway peer failed");
            return;
        }
    }
    sendingCompleted = false;
    esp_now_send(gwMacAddress, pongBytes, 8);
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
