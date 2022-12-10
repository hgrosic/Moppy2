/*
 * HardDrives.h
 *
 */

#ifndef SRC_MOPPYINSTRUMENTS_HARDDRIVES_H_
#define SRC_MOPPYINSTRUMENTS_HARDDRIVES_H_

#include <Arduino.h>
#include "MoppyTimer.h"
#include "MoppyInstrument.h"
#include "../MoppyConfig.h"
#include "../MoppyNetworks/MoppyNetwork.h"

namespace instruments {
  class HardDrives : public MoppyInstrument {
  public:
      void setup();

  protected:
      void sys_sequenceStop() override;
      void sys_reset() override;

      void dev_reset(uint8_t subAddress) override;
      void dev_noteOn(uint8_t subAddress, uint8_t payload[]) override;
      void dev_noteOff(uint8_t subAddress, uint8_t payload[]) override;
      void dev_bendPitch(uint8_t subAddress, uint8_t payload[]) override;

      void deviceMessage(uint8_t subAddress, uint8_t command, uint8_t payload[]);

  private:
    static unsigned int currentDriveState[];
    static unsigned int currentTick[];
    static const unsigned int A_PIN[];
    static const unsigned int B_PIN[];
    static const unsigned int PULSE_LENGTH[];
    // First drive being used for floppies, and the last drive.  Used for calculating
    // step and direction pins.
    #if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_ARCH_ESP32)
    static const byte FIRST_DRIVE = 1;
    static const byte LAST_DRIVE = 4; //TODO
    #elif ARDUINO_AVR_MEGA2560
    static const byte FIRST_DRIVE = 1;
    static const byte LAST_DRIVE = 16;
    #elif ARDUINO_ARCH_ESP8266
    static const byte FIRST_DRIVE = 1
    static const byte LAST_DRIVE = 4
    #endif

    // Maximum note number to attempt to play on floppy drives.  It's possible higher notes may work,
    // but they may also cause instability.
    static const byte MAX_FLOPPY_NOTE = 71;

    static void resetAll();
    static void togglePins(byte driveNum);
    static void haltAllDrives();
    static void reset(byte driveNum);
    static void tick();
    static void blinkLED();
    static void startupSound(byte driveNum);
    static void energizeCoil(byte driveNum, byte direction);
    static void deenergizeCoil(byte driveNum);
  };
}

#endif /* SRC_MOPPYINSTRUMENTS_HARDDRIVES_H_ */
