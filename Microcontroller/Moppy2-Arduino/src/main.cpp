#include <Arduino.h>
#include "MoppyConfig.h"
#include "MoppyInstruments/MoppyInstrument.h"

/**********
 * MoppyInstruments handle the sound-creation logic for your setup.  The
 * instrument class provides a systemMessage handler function and a deviceMessage
 * handler function for handling messages received by the network and a tick
 * function for precise timing events.
 *
 * Configure the appropriate instrument class for your setup in MoppyConfig.h
 */

// Floppy drives directly connected to the Arduino's digital pins
#ifdef INSTRUMENT_FLOPPIES
#include "MoppyInstruments/FloppyDrives.h"
MoppyInstrument *instrument = new instruments::FloppyDrives();
#endif

// Buzzers directly connected to the Arduino's digital pins
#ifdef INSTRUMENT_BUZZERS
#include "MoppyInstruments/Buzzers.h"
MoppyInstrument *instrument = new instruments::Buzzers();
#endif

// Hard drives connected to L293/298 motor drivers
#ifdef INSTRUMENT_HARDDRIVES
#include "MoppyInstruments/HardDrives.h"
MoppyInstrument *instrument = new instruments::HardDrives();
#endif

// EasyDriver stepper motor driver
#ifdef INSTRUMENT_EASYDRIVER
#include "MoppyInstruments/EasyDrivers.h"
MoppyInstrument *instrument = new instruments::EasyDrivers();
#endif

// L298N stepper motor driver
#ifdef INSTRUMENT_L298N
#include "MoppyInstruments/L298N.h"
MoppyInstrument *instrument = new instruments::L298N();
#endif

// A single device (e.g. xylophone, drums, etc.) connected to shift registers
#ifdef INSTRUMENT_SHIFT_REGISTER
#include "MoppyInstruments/ShiftRegister.h"
MoppyInstrument *instrument = new instruments::ShiftRegister();
#endif

// Floppy drives connected to 74HC595 shift registers
#ifdef INSTRUMENT_SHIFTED_FLOPPIES
#include "MoppyInstruments/ShiftedFloppyDrives.h"
MoppyInstrument *instrument = new instruments::ShiftedFloppyDrives();
#endif

/**********
 * MoppyNetwork classes receive messages sent by the Controller application,
 * parse them, and use the data to call the appropriate handler as implemented
 * in the instrument class defined above.
 *
 * Configure the appropriate networking class for your setup in MoppyConfig.h
 */

// Standard Arduino HardwareSerial implementation
#ifdef NETWORK_SERIAL
#include "MoppyNetworks/MoppySerial.h"
MoppySerial network = MoppySerial(instrument);
#endif

//// UDP Implementation using some sort of network stack?  (Not implemented yet)
#ifdef NETWORK_UDP
#include "MoppyNetworks/MoppyUDP.h"
MoppyUDP network = MoppyUDP(instrument);
#endif

//// ESP-Now Implementation
#ifdef NETWORK_ESPNOW
#include "MoppyNetworks/MoppyESPNow.h"
MoppyESPNow network = MoppyESPNow(instrument);
#endif

//// Standard Arduino HardwareSerial ---> ESP-Now Gateway Implementation
#ifdef NETWORK_ESPNOW_GATEWAY
#include "MoppyNetworks/MoppyESPNowGateway.h"
MoppyESPNowGateway network = MoppyESPNowGateway();
#endif

//The setup function is called once at startup of the sketch
void setup()
{
    #ifndef INSTRUMENT_GATEWAY
    // Call setup() on the instrument to allow to to prepare for action
    instrument->setup();
    #endif

    // Tell the network to start receiving messages
    network.begin();
}

// The loop function is called in an endless loop
void loop()
{
	// Endlessly read messages on the network.  The network implementation
	// will call the system or device handlers on the intrument whenever a message is received.
    network.readMessages();
}
