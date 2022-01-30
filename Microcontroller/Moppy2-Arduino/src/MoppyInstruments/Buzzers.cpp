/*
 * Buzzers.cpp
 *
 * Output for controlling PC buzzers.
 */
#include "MoppyInstrument.h"
#include "Buzzers.h"

namespace instruments
{
  // TODO Edit comments for both .cpp and .h

  /*NOTE: The arrays below contain unused zero-indexes to avoid having to do extra
 * math to shift the 1-based subAddresses to 0-based indexes here.  Unlike the previous
 * version of Moppy, we *will* be doing math to calculate which drive maps to which pin,
 * so there are as many values as drives (plus the extra zero-index)
 */

  /*An array of maximum track positions for each floppy drive.  3.5" Floppies have
 80 tracks, 5.25" have 50.  These should be doubled, because each tick is now
 half a position (use 158 and 98).
 NOTE: Index zero of this array controls the "resetAll" function, and should be the
 same as the largest value in this array
 */

  /*Array to keep track of state of each pin.  Even indexes track the control-pins for toggle purposes.  Odd indexes
 track direction-pins.  LOW = forward, HIGH=reverse
 */
  int Buzzers::currentState[] = {0, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};

  const unsigned int Buzzers::buzzerPins[] = {0, 15, 2, 4, 16, 17, 5, 18, 19, 21, 22, 23, 32, 33, 25, 26, 27, 14, 12, 13}; // 19 Buzzers max

                                             

  // Current period assigned to each drive.  0 = off.  Each period is two-ticks (as defined by
  // TIMER_RESOLUTION in MoppyInstrument.h) long.
  unsigned int Buzzers::currentPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  // Tracks the current tick-count for each drive (see FloppyDrives::tick() below)
  unsigned int Buzzers::currentTick[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  // The period originally set by incoming messages (prior to any modifications from pitch-bending)
  unsigned int Buzzers::originalPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  void Buzzers::setup()
  {

    // Prepare pins (0 and 1 are reserved for Serial communications)
    for (int i = FIRST_BUZZER; i <= LAST_BUZZER; i++)
    {
      pinMode(buzzerPins[i], OUTPUT);
    }

    // With all pins setup, let's do a first run reset
    resetAll();
    delay(500); // Wait a half second for safety

    // Setup timer to handle interrupts for floppy driving
    MoppyTimer::initialize(TIMER_RESOLUTION, tick);

    // If MoppyConfig wants a startup sound, play the startupSound on the
    // first drive.
    if (PLAY_STARTUP_SOUND)
    {
      startupSound(FIRST_BUZZER);
      delay(500);
      resetAll();
    }
  }

  // Play startup sound to confirm drive functionality
  void Buzzers::startupSound(byte buzzerNum)
  {
    unsigned int chargeNotes[] = {
        noteDoubleTicks[31],
        noteDoubleTicks[36],
        noteDoubleTicks[38],
        noteDoubleTicks[43],
        0};
    byte i = 0;
    unsigned long lastRun = 0;
    while (i < 5)
    {
      if (millis() - 200 > lastRun)
      {
        lastRun = millis();
        currentPeriod[buzzerNum] = chargeNotes[i++];
      }
    }
  }

  //
  //// Message Handlers
  //

  void Buzzers::sys_reset()
  {
    resetAll();
  }

  void Buzzers::sys_sequenceStop()
  {
    haltAllBuzzers();
  }

  void Buzzers::dev_reset(uint8_t subAddress)
  {
    if (subAddress == 0x00)
    {
      resetAll();
    }
    else
    {
      reset(subAddress);
    }
  }

  void Buzzers::dev_noteOn(uint8_t subAddress, uint8_t payload[])
  {
    if (payload[0] <= MAX_BUZZER_NOTE)
    {
      currentPeriod[subAddress] = originalPeriod[subAddress] = noteDoubleTicks[payload[0]];
    }
  }

  void Buzzers::dev_noteOff(uint8_t subAddress, uint8_t payload[])
  {
    currentPeriod[subAddress] = originalPeriod[subAddress] = 0;
  }

  void Buzzers::dev_bendPitch(uint8_t subAddress, uint8_t payload[])
  {
    // A value from -8192 to 8191 representing the pitch deflection
    int16_t bendDeflection = payload[0] << 8 | payload[1];

    // A whole octave of bend would double the frequency (halve the the period) of notes
    // Calculate bend based on BEND_OCTAVES from MoppyInstrument.h and percentage of deflection
    //currentPeriod[subAddress] = originalPeriod[subAddress] / 1.4;
    currentPeriod[subAddress] = originalPeriod[subAddress] / pow(2.0, BEND_OCTAVES * (bendDeflection / (float)8192));
  }

  void Buzzers::deviceMessage(uint8_t subAddress, uint8_t command, uint8_t payload[])
  {
    switch (command)
    {
    case NETBYTE_DEV_SETMOVEMENT:
      //setMovement(subAddress, payload[0] == 0); // MIDI bytes only go to 127, so * 2
      break;
    }
  }

//
//// Floppy driving functions
//

/*
Called by the timer interrupt at the specified resolution.  Because this is called extremely often,
it's crucial that any computations here be kept to a minimum!

Additionally, the ICACHE_RAM_ATTR helps avoid crashes with WiFi libraries, but may increase speed generally anyway
 */
#pragma GCC push_options
#pragma GCC optimize("Ofast") // Required to unroll this loop, but useful to try to keep this speedy
#ifdef ARDUINO_ARCH_ESP8266
  void ICACHE_RAM_ATTR Buzzers::tick()
  {
#elif ARDUINO_ARCH_ESP32
  void IRAM_ATTR Buzzers::tick()
  {
#else
  void Buzzers::tick()
  {
#endif
    /*
   For each drive, count the number of
   ticks that pass, and toggle the pin if the current period is reached.
   */
    for (unsigned int i = FIRST_BUZZER; i <= LAST_BUZZER; i++)
    {
      if (currentPeriod[i] > 0)
      {
        currentTick[i]++;
        if (currentTick[i] >= currentPeriod[i])
        {
          togglePin(buzzerPins[i]);
          currentTick[i] = 0;
        }
      }
    }
  }

  void Buzzers::togglePin(byte pin)
  {
    digitalWrite(pin, currentState[pin]);
    currentState[pin] = ~currentState[pin];
  }
#pragma GCC pop_options

  //
  //// UTILITY FUNCTIONS
  //

  //Not used now, but good for debugging...
  void Buzzers::blinkLED()
  {
    digitalWrite(13, HIGH); // set the LED on
    delay(250);             // wait for a second
    digitalWrite(13, LOW);
  }

  // Immediately stops all drives
  void Buzzers::haltAllBuzzers()
  {
    for (unsigned int i = FIRST_BUZZER; i <= LAST_BUZZER; i++)
    {
      currentPeriod[i] = 0;
    }
  }

  //For a given floppy number, runs the read-head all the way back to 0
  void Buzzers::reset(byte buzzerNum)
  {
    currentPeriod[buzzerNum] = 0; // Stop note
    currentState[buzzerNum] = LOW;
    digitalWrite(buzzerPins[buzzerNum], LOW);
  }

  // Resets all the drives simultaneously
  void Buzzers::resetAll()
  {

    // Stop all drives and set to reverse
    for (byte i = FIRST_BUZZER; i <= LAST_BUZZER; i++)
    {
      currentPeriod[i] = 0; // Stop note
      currentState[i] = LOW;
      digitalWrite(buzzerPins[i], LOW);
    }
  }

} // namespace instruments
