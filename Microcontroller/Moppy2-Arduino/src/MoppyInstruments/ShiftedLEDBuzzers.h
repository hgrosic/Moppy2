/*
 * ShiftedLEDBuzzers.h
 * Buzzers and LEDs connected to shift register(s)
 */

#ifndef SRC_MOPPYINSTRUMENTS_SHIFTEDLEDBUZZERS_H_
#define SRC_MOPPYINSTRUMENTS_SHIFTEDLEDBUZZERS_H_

#include "../MoppyConfig.h"
#include "../MoppyNetworks/MoppyNetwork.h"
#include "MoppyInstrument.h"
#include "MoppyTimer.h"
#include <Arduino.h>
#include <SPI.h>
namespace instruments {
class ShiftedLEDBuzzers : public MoppyInstrument {
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
    static const byte LAST_BUZZER = 12; // Number of buzzers being used. This determines the size of some arrays.
    // Maximum note number to attempt to play on buzzer.  It's possible higher notes may work.
    // but they may also cause instability.
    static const byte MAX_BUZZER_NOTE = 200;

    static uint16_t stateBits;     // Bits that represent the current state of buzzer (high/low)
    static uint16_t ledBits; // Bits that represent the current state of LED of each buzzer (on/off)
    static unsigned int currentPeriod[LAST_BUZZER];
    static unsigned int currentTick[LAST_BUZZER];
    static unsigned int originalPeriod[LAST_BUZZER];

    static void tick();
    static void resetAll();
    static void togglePin(uint8_t buzzerIndex);
    static void shiftBits();
    static void haltAllBuzzers();
    static void reset(uint8_t buzzerIndex);
    static void blinkLED();
    static void startupSound(uint8_t buzzerIndex);
};
} // namespace instruments

#endif /* SRC_MOPPYINSTRUMENTS_SHIFTEDLEDBUZZERS_H_ */