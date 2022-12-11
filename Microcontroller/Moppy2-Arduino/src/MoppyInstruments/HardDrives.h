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
    static uint8_t currentDriveState[];
    static uint32_t currentTick[];
    static const uint8_t A_PIN[];
    static const uint8_t B_PIN[];
    static const uint32_t PULSE_LENGTH[];
    // First drive being used for floppies, and the last drive.  Used for calculating
    // step and direction pins.
    static const uint8_t FIRST_DRIVE = 1;
    static const uint8_t LAST_DRIVE = 4; //TODO
    
    // Maximum note number to attempt to play on floppy drives.  It's possible higher notes may work,
    // but they may also cause instability.
    static const uint8_t MAX_FLOPPY_NOTE = 71;

    static void resetAll();
    static void haltAllDrives();
    static void reset(uint8_t driveNum);
    static void tick();
    static void blinkLED();
    static void startupSound(uint8_t driveNum);
    static void energizeCoil(uint8_t driveNum, uint8_t direction);
    static void deenergizeCoil(uint8_t driveNum);
  };
}

#endif /* SRC_MOPPYINSTRUMENTS_HARDDRIVES_H_ */
