/*
 * MoppyConfig.h
 */

#ifndef SRC_MOPPYCONFIG_H_
#define SRC_MOPPYCONFIG_H_

////
// Moppy Device configuration
////

////
// Uncomment **ONLY ONE** of these instruments
////

//#define INSTRUMENT_FLOPPIES
//#define INSTRUMENT_BUZZERS
//#define INSTRUMENT_HARDDRIVES
//#define INSTRUMENT_EASYDRIVER
//#define INSTRUMENT_L298N
//#define INSTRUMENT_SHIFTED_FLOPPIES
#define INSTRUMENT_SHIFTED_LED_BUZZERS
//#define INSTRUMENT_SHIFT_REGISTER
//#define INSTRUMENT_GATEWAY

////
// Uncomment **ONLY ONE** of these networks
// If the device is a **GATEWAY**, make sure that INSTRUMENT_GATEWAY is defined as well  
////

//#define NETWORK_SERIAL
//#define NETWORK_UDP
#define NETWORK_ESPNOW
//#define NETWORK_ESPNOW_GATEWAY

// Play a startup sound when the Arduino is initialized.  Helpful for 
// debugging purposes, but can be turned off once everything is working.
// If the device is a **GATEWAY**, this define is ignored.
#define PLAY_STARTUP_SOUND true

// Device address for this microcontroller (only messages sent to this address
// will be processed.
// If the device is a **GATEWAY**, this define is ignored.
#define DEVICE_ADDRESS 0x01

// Minimum and maximum sub-addresses that messages will be processed for.
// E.g. if you have 8 drives this would be 1 and 8. If you have 16, 1 and 16.
// If the device is a **GATEWAY**, this define is ignored.
#define MIN_SUB_ADDRESS 1
#define MAX_SUB_ADDRESS 12


#endif /* SRC_MOPPYCONFIG_H_ */
