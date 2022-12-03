/*
 * ShiftedLEDBuzzers.cpp
 *
 * Output for controlling buzzers and LEDs via shifter(s).
 */
#include "ShiftedLEDBuzzers.h"
#include "MoppyInstrument.h"
namespace instruments {

uint16_t ShiftedLEDBuzzers::stateBits = 0;  // Bits that represent the current state of the buzzers (high/low)
uint16_t ShiftedLEDBuzzers::ledBits = 0;    // Bits that represent the current state of the LEDs (on/off)

// Current period assigned to each buzzer.  0 = off.  Each period is two-ticks (as defined by
// TIMER_RESOLUTION in MoppyInstrument.h) long.
unsigned int ShiftedLEDBuzzers::currentPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0};

// Tracks the current tick-count for each buzzer (see ShiftedLEDBuzzers::tick() below)
unsigned int ShiftedLEDBuzzers::currentTick[] = {0, 0, 0, 0, 0, 0, 0, 0};

// The period originally set by incoming messages (prior to any modifications from pitch-bending)
unsigned int ShiftedLEDBuzzers::originalPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0};

void ShiftedLEDBuzzers::setup() {
    pinMode(LATCH_PIN, OUTPUT);
    SPI.begin();
    SPI.beginTransaction(SPISettings(16000000, LSBFIRST, SPI_MODE0)); // We're never ending this, hopefully that's okay...

    // With all pins setup, let's do a first run reset
    resetAll();
    delay(500); // Wait a half second for safety

    // Setup timer to handle interrupts for buzzer driving
    MoppyTimer::initialize(TIMER_RESOLUTION, tick);

    // If MoppyConfig wants a startup sound, play the startupSound on the
    // first drive.
    if (PLAY_STARTUP_SOUND) {
        startupSound(0);
        delay(500);
        resetAll();
    }
}

// Play startup sound to confirm buzzer functionality
void ShiftedLEDBuzzers::startupSound(byte buzzerIndex) {
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
            currentPeriod[buzzerIndex] = chargeNotes[i++];
        }
    }
}

//
//// Message Handlers
//

void ShiftedLEDBuzzers::sys_reset() {
    resetAll();
}

void ShiftedLEDBuzzers::sys_sequenceStop() {
    haltAllBuzzers();
}

void ShiftedLEDBuzzers::dev_reset(uint8_t subAddress) {
    if (subAddress == 0x00) {
        resetAll();
    } else {
        reset(subAddress);
    }
}

void ShiftedLEDBuzzers::dev_noteOn(uint8_t subAddress, uint8_t payload[]) {
    if (payload[0] <= MAX_BUZZER_NOTE) {
        currentPeriod[subAddress - 1] = originalPeriod[subAddress - 1] = noteDoubleTicks[payload[0]];
        bitSet(ledBits, subAddress - 1);
        shiftBits();
    }
};

void ShiftedLEDBuzzers::dev_noteOff(uint8_t subAddress, uint8_t payload[]) {
    currentPeriod[subAddress - 1] = originalPeriod[subAddress - 1] = 0;
    bitClear(ledBits, subAddress - 1);
    shiftBits();
};

void ShiftedLEDBuzzers::dev_bendPitch(uint8_t subAddress, uint8_t payload[]) {
    // A value from -8192 to 8191 representing the pitch deflection
    int16_t bendDeflection = payload[0] << 8 | payload[1];

    // A whole octave of bend would double the frequency (halve the the period) of notes
    // Calculate bend based on BEND_OCTAVES from MoppyInstrument.h and percentage of deflection
    //currentPeriod[subAddress] = originalPeriod[subAddress] / 1.4;
    currentPeriod[subAddress - 1] = originalPeriod[subAddress - 1] / pow(2.0, BEND_OCTAVES * (bendDeflection / (float)8192));
};

void ShiftedLEDBuzzers::deviceMessage(uint8_t subAddress, uint8_t command, uint8_t payload[]) {
    switch (command) {
    case NETBYTE_DEV_DISABLELEDS:
        // TODO: Implement disabling leds
        break;
    }
}

//
//// Buzzer control functions
//

/*
Called by the timer interrupt at the specified resolution.  Because this is called extremely often,
it's crucial that any computations here be kept to a minimum!

Additionally, the ICACHE_RAM_ATTR helps avoid crashes with WiFi libraries, but may increase speed generally anyway
 */
#pragma GCC push_options
#pragma GCC optimize("Ofast") // Required to unroll this loop, but useful to try to keep this speedy
#ifdef ARDUINO_ARCH_ESP8266
void ICACHE_RAM_ATTR ShiftedLEDBuzzers::tick() {
#elif ARDUINO_ARCH_ESP32
void IRAM_ATTR ShiftedLEDBuzzers::tick() {
#else
void ShiftedLEDBuzzers::tick() {
#endif
    bool shiftNeeded = false; // True if bits need to be written to registers
    /*
   For each buzzer, count the number of ticks that pass, and toggle the pin if the current period is reached.
   */

    for (int b = 0; b < LAST_BUZZER; b++) {
        if (currentPeriod[b] > 0) {
            if (++currentTick[b] >= currentPeriod[b]) {
                togglePin(b);
                shiftNeeded = true;
                currentTick[b] = 0;
            }
        }
    }

    if (shiftNeeded) {
        shiftBits();
    }
}

#ifdef ARDUINO_ARCH_ESP8266
void ICACHE_RAM_ATTR ShiftedLEDBuzzers::togglePin(uint8_t buzzerIndex) {
#elif ARDUINO_ARCH_ESP32
void IRAM_ATTR ShiftedLEDBuzzers::togglePin(uint8_t buzzerIndex) {
#else
void ShiftedLEDBuzzers::togglePin(uint8_t buzzerIndex) {
#endif

    // Toggle state bit of buzzer
    stateBits ^= (1 << buzzerIndex);
}

#ifdef ARDUINO_ARCH_ESP8266
void ICACHE_RAM_ATTR ShiftedLEDBuzzers::shiftBits() {
#elif ARDUINO_ARCH_ESP32
void IRAM_ATTR ShiftedLEDBuzzers::shiftBits() {
#else
void ShiftedLEDBuzzers::shiftBits() {
#endif
uint32_t transferMessage = ledBits << 12 | stateBits;
uint8_t transferByte1 = transferMessage         & 0xFF;
uint8_t transferByte2 = transferMessage >> 8    & 0xFF;
uint8_t transferByte3 = transferMessage >> 16   & 0xFF;

//uint8_t transferByte1 = stateBits & 0xFF;
//uint8_t transferByte2 = ((stateBits >> 8 & 0b00001111) | (ledBits & 0b00001111) << 4) & 0xFF;
//uint8_t transferByte3 = ledBits >> 4 & 0xFF;


#ifdef ARDUINO_AVR_UNO
    PORTD &= B11101111;
#else
    digitalWrite(LATCH_PIN, LOW);
#endif

    SPI.transfer(transferByte1);
    SPI.transfer(transferByte2);
    SPI.transfer(transferByte3);

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

// Immediately stops all buzzers
void ShiftedLEDBuzzers::haltAllBuzzers() {
    for (byte d = 0; d < LAST_BUZZER; d++) {
        currentPeriod[d] = 0;
    }
    ledBits = 0;
    shiftBits();
}

// Reset a selected buzzer
void ShiftedLEDBuzzers::reset(uint8_t buzzerIndex) {
    currentPeriod[buzzerIndex] = 0;
    bitClear(stateBits, buzzerIndex);
    bitClear(ledBits, buzzerIndex);
    shiftBits();
}

// Resets all the drives simultaneously
void ShiftedLEDBuzzers::resetAll() {

    // Stop all buzzers and turn off LEDs
    for (byte d = 0; d < LAST_BUZZER; d++) {
        currentPeriod[d] = 0;
    }
    stateBits = 0;
    ledBits = 0;
    shiftBits();
}
} // namespace instruments