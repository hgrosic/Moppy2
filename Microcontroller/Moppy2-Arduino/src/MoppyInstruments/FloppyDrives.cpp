/*
 * FloppyDrives.cpp
 *
 * Output for controlling floppy drives.  The _original_ Moppy instrument!
 */
#include "MoppyInstrument.h"
#include "FloppyDrives.h"

namespace instruments
{

  /*NOTE: The arrays below contain unused zero-indexes to avoid having to do extra
 * math to shift the 1-based subAddresses to 0-based indexes here. The i-th index
 * of each array corresponds to the the i-th floppy drive (except the extra 0 index).
 *
 */

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_ARCH_ESP32)
  /*An array of maximum track positions for each floppy drive.  3.5" Floppies have
 80 tracks, 5.25" have 50.  These should be doubled, because each tick is now
 half a position (use 158 and 98).
 NOTE: Index zero of this array controls the "resetAll" function, and should be the
 same as the largest value in this array
 */
  unsigned int FloppyDrives::MIN_POSITION[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int FloppyDrives::MAX_POSITION[] = {158, 158, 158, 158, 158, 158, 158, 158, 158, 158};
  // Array to keep track of the state of each Step pin for toggle purposes.
  unsigned int FloppyDrives::currentStepState[] = {0, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
  // Array to keep track of the state of each Direction pin (LOW = forward, HIGH=reverse).
  unsigned int FloppyDrives::currentDirState[] = {0, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
  // Array to track the current position of each floppy head.
  unsigned int FloppyDrives::currentPosition[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  // Current period assigned to each drive.  0 = off.  Each period is two-ticks (as defined by
  // TIMER_RESOLUTION in MoppyInstrument.h) long.
  unsigned int FloppyDrives::currentPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  // Tracks the current tick-count for each drive (see FloppyDrives::tick() below)
  unsigned int FloppyDrives::currentTick[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  // The period originally set by incoming messages (prior to any modifications from pitch-bending)
  unsigned int FloppyDrives::originalPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif ARDUINO_AVR_MEGA2560
  unsigned int FloppyDrives::MIN_POSITION[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int FloppyDrives::MAX_POSITION[] = {158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158, 158};
  unsigned int FloppyDrives::currentStepState[] = {0, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
  unsigned int FloppyDrives::currentDirState[] = {0, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW};
  unsigned int FloppyDrives::currentPosition[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int FloppyDrives::currentPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int FloppyDrives::currentTick[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int FloppyDrives::originalPeriod[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#elif ARDUINO_ARCH_ESP8266
  unsigned int FloppyDrives::MIN_POSITION[] = {0, 0, 0, 0, 0};
  unsigned int FloppyDrives::MAX_POSITION[] = {158, 158, 158, 158, 158};
  unsigned int FloppyDrives::currentStepState[] = {0, LOW, LOW, LOW, LOW};
  unsigned int FloppyDrives::currentDirState[] = {0, LOW, LOW, LOW, LOW};
  unsigned int FloppyDrives::currentPosition[] = {0, 0, 0, 0, 0};
  unsigned int FloppyDrives::currentPeriod[] = {0, 0, 0, 0, 0};
  unsigned int FloppyDrives::currentTick[] = {0, 0, 0, 0, 0};
  unsigned int FloppyDrives::originalPeriod[] = {0, 0, 0, 0, 0};
#endif

#ifdef ARDUINO_AVR_UNO
  // Array of STEP pin numbers for the used board pinout 
  const unsigned int FloppyDrives::STEP_PIN[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18};
  // Array of DIR pin numbers for the used board pinout
  const unsigned int FloppyDrives::DIR_PIN[] = {0, 3, 5, 7, 9, 11, 13, 15, 17, 19};
#elif ARDUINO_AVR_MEGA2560
  const unsigned int FloppyDrives::STEP_PIN[] = {0, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52};
  const unsigned int FloppyDrives::DIR_PIN[] = {0, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51, 53};
#elif ARDUINO_ARCH_ESP32
  const unsigned int FloppyDrives::STEP_PIN[] = {0, 15, 4, 17, 18, 21, 23, 33, 26, 14};
  const unsigned int FloppyDrives::DIR_PIN[] = {0, 2, 16, 5, 19, 22, 32, 25, 27, 12};
#elif ARDUINO_ARCH_ESP8266
  const unsigned int FloppyDrives::STEP_PIN[] = {0, 5, 0, 15, 12};
  const unsigned int FloppyDrives::DIR_PIN[] = {0, 4, 2, 13, 14};
#endif

  void FloppyDrives::setup()
  {
    // Prepare pins
    for (byte d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      pinMode(STEP_PIN[d], OUTPUT);
      pinMode(DIR_PIN[d], OUTPUT);
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
      startupSound(FIRST_DRIVE);
      delay(500);
      resetAll();
    }
  }

  // Play startup sound to confirm drive functionality
  void FloppyDrives::startupSound(byte driveNum)
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
        currentPeriod[driveNum] = chargeNotes[i++];
      }
    }
  }

  //
  //// Message Handlers
  //

  void FloppyDrives::sys_reset()
  {
    resetAll();
  }

  void FloppyDrives::sys_sequenceStop()
  {
    haltAllDrives();
  }

  void FloppyDrives::dev_reset(uint8_t subAddress)
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

  void FloppyDrives::dev_noteOn(uint8_t subAddress, uint8_t payload[])
  {
    if (payload[0] <= MAX_FLOPPY_NOTE)
    {
      currentPeriod[subAddress] = originalPeriod[subAddress] = noteDoubleTicks[payload[0]];
    }
  }

  void FloppyDrives::dev_noteOff(uint8_t subAddress, uint8_t payload[])
  {
    currentPeriod[subAddress] = originalPeriod[subAddress] = 0;
  }

  void FloppyDrives::dev_bendPitch(uint8_t subAddress, uint8_t payload[])
  {
    // A value from -8192 to 8191 representing the pitch deflection
    int16_t bendDeflection = payload[0] << 8 | payload[1];

    // A whole octave of bend would double the frequency (halve the the period) of notes
    // Calculate bend based on BEND_OCTAVES from MoppyInstrument.h and percentage of deflection
    //currentPeriod[subAddress] = originalPeriod[subAddress] / 1.4;
    currentPeriod[subAddress] = originalPeriod[subAddress] / pow(2.0, BEND_OCTAVES * (bendDeflection / (float)8192));
  }

  void FloppyDrives::deviceMessage(uint8_t subAddress, uint8_t command, uint8_t payload[])
  {
    switch (command)
    {
    case NETBYTE_DEV_SETMOVEMENT:
      setMovement(subAddress, payload[0] == 0); // MIDI bytes only go to 127, so * 2
      break;
    }
  }

  void FloppyDrives::setMovement(byte driveNum, bool movementEnabled)
  {
    if (movementEnabled)
    {
      MIN_POSITION[driveNum] = 0;
      MAX_POSITION[driveNum] = 158;
    }
    else
    {
      MIN_POSITION[driveNum] = 79;
      MAX_POSITION[driveNum] = 81;
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
  void ICACHE_RAM_ATTR FloppyDrives::tick()
  {
#elif ARDUINO_ARCH_ESP32
  void IRAM_ATTR FloppyDrives::tick()
  {
#else
  void FloppyDrives::tick()
  {
#endif
    /*
   For each drive, count the number of
   ticks that pass, and toggle the pin if the current period is reached.
   */
    for (byte d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      if (currentPeriod[d] > 0)
      {
        currentTick[d]++;
        if (currentTick[d] >= currentPeriod[d])
        {
          togglePin(d);
          currentTick[d] = 0;
        }
      }
    }
  }

#ifdef ARDUINO_ARCH_ESP8266
  void ICACHE_RAM_ATTR FloppyDrives::togglePin(byte driveNum)
  {
#elif ARDUINO_ARCH_ESP32
  void IRAM_ATTR FloppyDrives::togglePin(byte driveNum)
  {
#else
  void FloppyDrives::togglePin(byte driveNum)
  {
#endif

    //Switch directions if end has been reached
    if (currentPosition[driveNum] >= MAX_POSITION[driveNum])
    {
      currentDirState[driveNum] = HIGH;
      digitalWrite(DIR_PIN[driveNum], HIGH);
    }
    else if (currentPosition[driveNum] <= MIN_POSITION[driveNum])
    {
      currentDirState[driveNum] = LOW;
      digitalWrite(DIR_PIN[driveNum], LOW);
    }

    //Update currentPosition
    if (currentDirState[driveNum] == HIGH)
    {
      currentPosition[driveNum]--;
    }
    else
    {
      currentPosition[driveNum]++;
    }

    //Pulse the STEP pin
    digitalWrite(STEP_PIN[driveNum], currentStepState[driveNum]);
    currentStepState[driveNum] = ~currentStepState[driveNum];
  }
#pragma GCC pop_options

  //
  //// UTILITY FUNCTIONS
  //

  //Not used now, but good for debugging...
  void FloppyDrives::blinkLED()
  {
    digitalWrite(13, HIGH); // set the LED on
    delay(250);             // wait for a second
    digitalWrite(13, LOW);
  }

  // Immediately stops all drives
  void FloppyDrives::haltAllDrives()
  {
    for (byte d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      currentPeriod[d] = 0;
    }
  }

  //For a given floppy number, runs the read-head all the way back to 0
  void FloppyDrives::reset(byte driveNum)
  {
    currentPeriod[driveNum] = 0; // Stop note

    digitalWrite(DIR_PIN[driveNum], HIGH); // Go in reverse
    for (unsigned int s = 0; s < MAX_POSITION[driveNum]; s += 2)
    { // Half max because we're stepping directly (no toggle)
      digitalWrite(STEP_PIN[driveNum], HIGH);
      digitalWrite(STEP_PIN[driveNum], LOW);
      delay(5);
    }
    currentPosition[driveNum] = 0;    // We're reset.
    currentStepState[driveNum] = LOW; // Last step state is LOW
    digitalWrite(DIR_PIN[driveNum], LOW);
    currentDirState[driveNum] = LOW; // Ready to go forward.
    setMovement(driveNum, true);     // Set movement to true by default
  }

  // Resets all the drives simultaneously
  void FloppyDrives::resetAll()
  {

    // Stop all drives and set to reverse
    for (byte d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      currentPeriod[d] = 0;
      digitalWrite(DIR_PIN[d], HIGH);
    }

    // Reset all drives together
    for (unsigned int s = 0; s < MAX_POSITION[0]; s += 2)
    { //Half max because we're stepping directly (no toggle); grab max from index 0
      for (byte d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
      {
        digitalWrite(STEP_PIN[d], HIGH);
        digitalWrite(STEP_PIN[d], LOW);
      }
      delay(5);
    }

    // Return tracking to ready state
    for (byte d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      currentPosition[d] = 0;    // We're reset.
      currentStepState[d] = LOW; // Last step state is LOW
      digitalWrite(DIR_PIN[d], LOW);
      currentDirState[d] = LOW; // Ready to go forward.
      setMovement(d, true);     // Set movement to true by default
    }
  }
} // namespace instruments
