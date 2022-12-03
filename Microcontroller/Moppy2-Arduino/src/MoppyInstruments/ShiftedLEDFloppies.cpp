/*
 * ShiftedLEDFloppies.cpp
 *
 * Output for controlling Floppies and LEDs via shifter(s).
 */
#include "ShiftedLEDFloppies.h"
#include "MoppyInstrument.h"
namespace instruments {

uint8_t ShiftedLEDFloppies::stepBits = 0;       // Bits that represent the current state of the step pins
uint8_t ShiftedLEDFloppies::directionBits = 0;  // Bits that represent the current state of the direction pins
uint8_t ShiftedLEDFloppies::ledBits = 0;        // Bits that represent the current state of the LEDs (on/off)

/*An array of maximum track positions for each floppy drive.  3.5" Floppies have
 80 tracks, 5.25" have 50.  These should be doubled, because each tick is now
 half a position (use 158 and 98).
 NOTE: Index zero of this array controls the "resetAll" function, and should be the
 same as the largest value in this array
 */
unsigned int ShiftedLEDFloppies::MAX_POSITION[] = {158, 158, 158, 158, 158, 158, 158, 158};
unsigned int ShiftedLEDFloppies::MIN_POSITION[] = {0, 0, 0, 0, 0, 0, 0, 0};
// ^ Use 81 and 79 for in-place playing

//Array to track the current position of each floppy head.
unsigned int ShiftedLEDFloppies::currentPosition[] = {0, 0, 0, 0, 0, 0, 0, 0};

// Current period assigned to each floppy.  0 = off.  Each period is two-ticks (as defined by
// TIMER_RESOLUTION in MoppyInstrument.h) long.
unsigned int ShiftedLEDFloppies::currentPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0};

// Tracks the current tick-count for each floppy (see ShiftedLEDFloppies::tick() below)
unsigned int ShiftedLEDFloppies::currentTick[] = {0, 0, 0, 0, 0, 0, 0, 0};

// The period originally set by incoming messages (prior to any modifications from pitch-bending)
unsigned int ShiftedLEDFloppies::originalPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0};

void ShiftedLEDFloppies::setup() {
    pinMode(LATCH_PIN, OUTPUT);
    SPI.begin();
    SPI.beginTransaction(SPISettings(16000000, LSBFIRST, SPI_MODE0)); // We're never ending this, hopefully that's okay...

    // With all pins setup, let's do a first run reset
    resetAll();
    delay(500); // Wait a half second for safety

    // Setup timer to handle interrupts for floppy driving
    MoppyTimer::initialize(TIMER_RESOLUTION, tick);

    // If MoppyConfig wants a startup sound, play the startupSound on the
    // first floppy.
    if (PLAY_STARTUP_SOUND) {
        startupSound(0);
        delay(500);
        resetAll();
    }
}

// Play startup sound to confirm floppy functionality
void ShiftedLEDFloppies::startupSound(byte floppyIndex) {
    unsigned int chargeNotes[] = {
        noteDoubleTicks[31],
        noteDoubleTicks[36],
        noteDoubleTicks[38],
        noteDoubleTicks[43],
        0};
    byte i = 0;
    unsigned long lastRun = 0;
    while (i < 5) {
        if (millis() - 200 > lastRun) {
            lastRun = millis();
            currentPeriod[floppyIndex] = chargeNotes[i++];
        }
    }
}

//
//// Message Handlers
//

void ShiftedLEDFloppies::sys_reset() {
    resetAll();
}

void ShiftedLEDFloppies::sys_sequenceStop() {
    haltAllFloppies();
}

void ShiftedLEDFloppies::dev_reset(uint8_t subAddress) {
    if (subAddress == 0x00) {
        resetAll();
    } else {
        reset(subAddress);
    }
}

void ShiftedLEDFloppies::dev_noteOn(uint8_t subAddress, uint8_t payload[]) {
    if (payload[0] <= MAX_FLOPPY_NOTE) {
        currentPeriod[subAddress - 1] = originalPeriod[subAddress - 1] = noteDoubleTicks[payload[0]];
        bitClear(ledBits, subAddress - 1);
        shiftBits();
    }
};

void ShiftedLEDFloppies::dev_noteOff(uint8_t subAddress, uint8_t payload[]) {
    currentPeriod[subAddress - 1] = originalPeriod[subAddress - 1] = 0;
    bitSet(ledBits, subAddress - 1);
    shiftBits();
};

void ShiftedLEDFloppies::dev_bendPitch(uint8_t subAddress, uint8_t payload[]) {
    // A value from -8192 to 8191 representing the pitch deflection
    int16_t bendDeflection = payload[0] << 8 | payload[1];

    // A whole octave of bend would double the frequency (halve the the period) of notes
    // Calculate bend based on BEND_OCTAVES from MoppyInstrument.h and percentage of deflection
    //currentPeriod[subAddress] = originalPeriod[subAddress] / 1.4;
    currentPeriod[subAddress - 1] = originalPeriod[subAddress - 1] / pow(2.0, BEND_OCTAVES * (bendDeflection / (float)8192));
};

void ShiftedLEDFloppies::deviceMessage(uint8_t subAddress, uint8_t command, uint8_t payload[]) {
    switch (command) {
    case NETBYTE_DEV_DISABLELEDS:
        // TODO: Implement disabling leds
        break;
    case NETBYTE_DEV_SETMOVEMENT:
        setMovement(subAddress - 1, payload[0] == 0); // MIDI bytes only go to 127, so * 2
        break;
    }
}

void ShiftedLEDFloppies::setMovement(byte floppyIndex, bool movementEnabled) {
    if (movementEnabled) {
        MIN_POSITION[floppyIndex] = 0;
        MAX_POSITION[floppyIndex] = 158;
    } else {
        MIN_POSITION[floppyIndex] = 79;
        MAX_POSITION[floppyIndex] = 81;
    }
}

//
//// Floppy control functions
//

/*
Called by the timer interrupt at the specified resolution.  Because this is called extremely often,
it's crucial that any computations here be kept to a minimum!

Additionally, the ICACHE_RAM_ATTR helps avoid crashes with WiFi libraries, but may increase speed generally anyway
 */
#pragma GCC push_options
#pragma GCC optimize("Ofast") // Required to unroll this loop, but useful to try to keep this speedy
#ifdef ARDUINO_ARCH_ESP8266
void ICACHE_RAM_ATTR ShiftedLEDFloppies::tick() {
#elif ARDUINO_ARCH_ESP32
void IRAM_ATTR ShiftedLEDFloppies::tick() {
#else
void ShiftedLEDFloppies::tick() {
#endif
    bool shiftNeeded = false; // True if bits need to be written to registers
    /*
    For each floppy, count the number of ticks that pass, and toggle the pin if the current period is reached.
    */
    for (int f = 0; f < LAST_FLOPPY; f++) {
        if (currentPeriod[f] > 0) {
            if (++currentTick[f] >= currentPeriod[f]) {
                togglePin(f);
                shiftNeeded = true;
                currentTick[f] = 0;
            }
        }
    }

    if (shiftNeeded) {
        shiftBits();
    }
}

#ifdef ARDUINO_ARCH_ESP8266
void ICACHE_RAM_ATTR ShiftedLEDFloppies::togglePin(uint8_t floppyIndex) {
#elif ARDUINO_ARCH_ESP32
void IRAM_ATTR ShiftedLEDFloppies::togglePin(uint8_t floppyIndex) {
#else
void ShiftedLEDFloppies::togglePin(uint8_t floppyIndex) {
#endif
    unsigned int *cPos = &currentPosition[floppyIndex];

    //Switch directions if end has been reached
    if (*cPos >= MAX_POSITION[floppyIndex]) {
        bitSet(directionBits, floppyIndex);
    } else if (*cPos <= MIN_POSITION[floppyIndex]) {
        bitClear(directionBits, floppyIndex);
    }

    //Update currentPosition
    if (bitRead(directionBits, floppyIndex)) {
        (*cPos)--;
    } else {
        (*cPos)++;
    }

    stepBits ^= (1 << floppyIndex);
}

#ifdef ARDUINO_ARCH_ESP8266
void ICACHE_RAM_ATTR ShiftedLEDFloppies::shiftBits() {
#elif ARDUINO_ARCH_ESP32
void IRAM_ATTR ShiftedLEDFloppies::shiftBits() {
#else
void ShiftedLEDFloppies::shiftBits() {
#endif
#ifdef ARDUINO_AVR_UNO
    PORTD &= B11101111;
#else
    digitalWrite(LATCH_PIN, LOW);
#endif
    SPI.transfer(directionBits);
    SPI.transfer(stepBits);
    SPI.transfer(ledBits);

#ifdef ARDUINO_AVR_UNO
    PORTD |= B00010000;
#else
    digitalWrite(LATCH_PIN, HIGH);
#endif
}
#pragma GCC pop_options

//
//// UTILITY FUNCTIONS
//

// Immediately stops all Floppies
void ShiftedLEDFloppies::haltAllFloppies() {
    for (byte d = 0; d < LAST_FLOPPY; d++) {
        currentPeriod[d] = 0;
    }
    ledBits = B11111111;
    shiftBits();
}

//TODO For a given floppy number, runs the read-head all the way back to 0
void ShiftedLEDFloppies::reset(uint8_t floppyIndex) {
    // currentPeriod[driveNum] = 0; // Stop note

    // byte stepPin = driveNum * 2;
    // digitalWrite(stepPin + 1, HIGH);                               // Go in reverse
    // for (unsigned int s = 0; s < MAX_POSITION[driveNum]; s += 2) { //Half max because we're stepping directly (no toggle)
    //     digitalWrite(stepPin, HIGH);
    //     digitalWrite(stepPin, LOW);
    //     delay(5);
    // }
    // currentPosition[driveNum] = 0; // We're reset.
    // currentState[stepPin] = LOW;
    // digitalWrite(stepPin + 1, LOW);
    // currentState[stepPin + 1] = LOW; // Ready to go forward.
}

// Resets all the floppies simultaneously
void ShiftedLEDFloppies::resetAll() {
    // Stop all drives and set to reverse
    for (uint8_t f = 0; f < LAST_FLOPPY; f++) {
        currentPeriod[f] = 0;
    }
    directionBits = B11111111;
    shiftBits();
    // Reset all drives together
    for (unsigned int s = 0; s < MAX_POSITION[0]; s += 2) { //Half max because we're stepping directly (no toggle); grab max from index 0
        stepBits = B11111111;
        shiftBits();
        stepBits = B00000000;
        shiftBits();
        delay(5);
    }
    // Return tracking to ready state
    directionBits = 0x0;
    for (byte f = 0; f < LAST_FLOPPY; f++) {
        setMovement(f, true);   // Turn movement back on by default
        currentPosition[f] = 0; // We're reset.
    }
    ledBits = B11111111;
    shiftBits();
}
} // namespace instruments