/*
 * ShiftedLEDFloppies.h
 * Floppies and LEDs connected to shift register(s)
 */

#ifndef SRC_MOPPYINSTRUMENTS_SHIFTEDLEDFLOPPIES_H_
#define SRC_MOPPYINSTRUMENTS_SHIFTEDLEDFLOPPIES_H_

#include "../MoppyConfig.h"
#include "../MoppyNetworks/MoppyNetwork.h"
#include "MoppyInstrument.h"
#include "MoppyTimer.h"
#include <Arduino.h>
#include <SPI.h>
namespace instruments {
class ShiftedLEDFloppies : public MoppyInstrument {
public:
    void setup();
    static const int LATCH_PIN = 2; //RCLK

protected:
    void sys_sequenceStop() override;
    void sys_reset() override;

    void dev_reset(uint8_t subAddress) override;
    void dev_noteOn(uint8_t subAddress, uint8_t payload[]) override;
    void dev_noteOff(uint8_t subAddress, uint8_t payload[]) override;
    void dev_bendPitch(uint8_t subAddress, uint8_t payload[]) override;
    void deviceMessage(uint8_t subAddress, uint8_t command, uint8_t payload[]);

private:
    static const byte LAST_FLOPPY = 8; // Number of floppys being used. This determines the size of some arrays.
    // Maximum note number to attempt to play on floppy.  It's possible higher notes may work.
    // but they may also cause instability.
    static const byte MAX_FLOPPY_NOTE = 71;
    static unsigned int MAX_POSITION[LAST_FLOPPY];
    static unsigned int MIN_POSITION[LAST_FLOPPY];
    static unsigned int currentPosition[LAST_FLOPPY];
    static uint8_t stepBits;        // Bits that represent the current state of the step pins
    static uint8_t directionBits;   // Bits that represent the current state of the direction pins
    static uint8_t ledBits;         // Bits that represent the current state of LED of each floppy (on/off)
    static unsigned int currentPeriod[LAST_FLOPPY];
    static unsigned int currentTick[LAST_FLOPPY];
    static unsigned int originalPeriod[LAST_FLOPPY];

    static void tick();
    static void resetAll();
    static void togglePin(uint8_t floppyIndex);
    static void shiftBits();
    static void haltAllFloppies();
    static void reset(uint8_t floppyIndex);
    static void blinkLED();
    static void startupSound(uint8_t floppyIndex);
    static void setMovement(byte driveIndex, bool movementEnabled);
};
} // namespace instruments

#endif /* SRC_MOPPYINSTRUMENTS_SHIFTEDLEDFLOPPIES_H_ */
