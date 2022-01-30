/*
 * Buzzers.h
 *
 */

#ifndef SRC_MOPPYINSTRUMENTS_BUZZERS_H_
#define SRC_MOPPYINSTRUMENTS_BUZZERS_H_

#include <Arduino.h>
#include "MoppyTimer.h"
#include "MoppyInstrument.h"
#include "../MoppyConfig.h"
#include "../MoppyNetworks/MoppyNetwork.h"

namespace instruments {
  class Buzzers : public MoppyInstrument {
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
    static int currentState[];
    static unsigned int currentPeriod[];
    static unsigned int currentTick[];
    static unsigned int originalPeriod[];
    
    static const unsigned int buzzerPins[];

    // First drive being used for floppies, and the last drive.  Used for calculating
    // step and direction pins.
    static const byte FIRST_BUZZER = 1;
    static const byte LAST_BUZZER = 19; //TODO Set this one to a correct 19           This sketch can handle only up to 9 drives (the max for Arduino Uno)

    // Maximum note number to attempt to play on floppy drives.  It's possible higher notes may work,
    // but they may also cause instability.
    static const byte MAX_BUZZER_NOTE = 71;

    static void resetAll();
    static void togglePin(byte pin);
    static void haltAllBuzzers();
    static void reset(byte buzzerNum);
    static void tick();
    static void blinkLED();
    static void startupSound(byte buzzerNum);
  };
}

#endif /* SRC_MOPPYINSTRUMENTS_BUZZERS_H_ */
