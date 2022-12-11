/*
 * HardDrives.cpp
 * @author : hgrosic
 * Output for controlling hard drives as drums via L293D!
 */
#include "MoppyInstrument.h"
#include "HardDrives.h"

namespace instruments
{

  /*NOTE: The arrays below contain unused zero-indexes to avoid having to do extra
 * math to shift the 1-based subAddresses to 0-based indexes here. The i-th index
 * of each array corresponds to the the i-th hard drive (except the extra 0 index).
 *
 */

  // Array to keep track of the state of each drive.
  uint8_t HardDrives::currentDriveState[] = {0, LOW, LOW, LOW, LOW};
  // Current period assigned to each drive.  0 = off.  Each period is two-ticks (as defined by
  // TIMER_RESOLUTION in MoppyInstrument.h) long.
  uint32_t HardDrives::currentTick[] = {0, 0, 0, 0, 0};
  const uint32_t HardDrives::PULSE_LENGTH[] = {625, 625, 625, 625, 625};

#ifdef ARDUINO_AVR_UNO
  // Array of A pin numbers for the used board pinout (input A of the L293D) 
  const uint8_t HardDrives::A_PIN[] = {0, 2, 4, 6, 8};
  // Array of B pin numbers for the used board pinout (input B of the L293D) 
  const uint8_t HardDrives::B_PIN[] = {0, 3, 5, 7, 9};
#elif ARDUINO_ARCH_ESP32
  const uint8_t HardDrives::A_PIN[] = {0, 15, 4, 17, 18};
  const uint8_t HardDrives::B_PIN[] = {0, 2, 16, 5, 19};
#elif ARDUINO_ARCH_ESP8266
  const uint8_t HardDrives::A_PIN[] = {0, 5, 0, 15, 12};
  const uint8_t HardDrives::B_PIN[] = {0, 4, 2, 13, 14};
#endif

  void HardDrives::setup()
  {
    // Prepare pins
    for (uint8_t d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      pinMode(A_PIN[d], OUTPUT);
      pinMode(B_PIN[d], OUTPUT);
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
  void HardDrives::startupSound(uint8_t driveNum)
  {
    currentDriveState[driveNum] = HIGH;
  }

  //
  //// Message Handlers
  //

  void HardDrives::sys_reset()
  {
    resetAll();
  }

  void HardDrives::sys_sequenceStop()
  {
    haltAllDrives();
  }

  void HardDrives::dev_reset(uint8_t subAddress)
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

  void HardDrives::dev_noteOn(uint8_t subAddress, uint8_t payload[])
  {
    //blinkLED(); // TODO
    currentDriveState[subAddress] = HIGH; //TODO Reenable
    //energizeCoil(subAddress, 0);
  }

  void HardDrives::dev_noteOff(uint8_t subAddress, uint8_t payload[])
  {
    //energizeCoil(subAddress, 1);
    if (currentTick[subAddress] >= 2*PULSE_LENGTH[subAddress])
    {
      currentDriveState[subAddress] = LOW;
    }
  }

  void HardDrives::dev_bendPitch(uint8_t subAddress, uint8_t payload[])
  {
    //// A value from -8192 to 8191 representing the pitch deflection
    //int16_t bendDeflection = payload[0] << 8 | payload[1];
    //
    //// A whole octave of bend would double the frequency (halve the the period) of notes
    //// Calculate bend based on BEND_OCTAVES from MoppyInstrument.h and percentage of deflection
    ////currentPeriod[subAddress] = originalPeriod[subAddress] / 1.4;
    //currentPeriod[subAddress] = originalPeriod[subAddress] / pow(2.0, BEND_OCTAVES * (bendDeflection / (float)8192));
  }

  void HardDrives::deviceMessage(uint8_t subAddress, uint8_t command, uint8_t payload[])
  {
    switch (command)
    {
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
  void ICACHE_RAM_ATTR HardDrives::tick()
#elif ARDUINO_ARCH_ESP32
  void IRAM_ATTR HardDrives::tick()
#else
  void HardDrives::tick()
#endif
  {
    /*
   For each drive, count the number of
   ticks that pass, and toggle the pin if the current period is reached.
   */
    for (uint8_t d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      if (currentDriveState[d] == HIGH)
      {
        currentTick[d]++;
        if (currentTick[d] <= PULSE_LENGTH[d])
        {
          energizeCoil(d, 0);
        }
        else if (currentTick[d] <= PULSE_LENGTH[d]<<1)
        {
          energizeCoil(d, 1);
        }
        else
        {
          currentDriveState[d] = LOW;
          deenergizeCoil(d);
        }
      }
      else
      {
        currentTick[d] = 0;
      }
    }
  }
  
  inline void HardDrives::energizeCoil(uint8_t driveNum, uint8_t direction)
  {
    digitalWrite(A_PIN[driveNum], !direction);
    digitalWrite(B_PIN[driveNum], direction);
  }

  inline void HardDrives::deenergizeCoil(uint8_t driveNum)
  {
    digitalWrite(A_PIN[driveNum], LOW);
    digitalWrite(B_PIN[driveNum], LOW);
  }

#pragma GCC pop_options

  //
  //// UTILITY FUNCTIONS
  //

  //Not used now, but good for debugging...
  void HardDrives::blinkLED()
  {
    digitalWrite(13, HIGH); // set the LED on
    delay(15);             // wait for a second
    digitalWrite(13, LOW);
  }

  // Immediately stops all drives
  void HardDrives::haltAllDrives()
  {
    for (uint8_t d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      currentDriveState[d] = LOW;
    }
  }

  //For a given floppy number, runs the read-head all the way back to 0
  void HardDrives::reset(uint8_t driveNum)
  {
    currentDriveState[driveNum] = LOW; // Stop note
    currentTick[driveNum] = 0; 
    energizeCoil(driveNum, 0);
    delay(25);
    energizeCoil(driveNum, 1);
    delay(25);
    deenergizeCoil(driveNum);
    delay(5);
  }

  // Resets all the drives simultaneously
  void HardDrives::resetAll()
  {
    // Stop all drives and set to reverse
    for (uint8_t d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      currentDriveState[d] = LOW; // Stop note
      currentTick[d] = 0;
      energizeCoil(d, 0);
    }
    delay(25);
    for (uint8_t d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      energizeCoil(d, 1);
    }
    delay(25);
    for (uint8_t d = FIRST_DRIVE; d <= LAST_DRIVE; d++)
    {
      deenergizeCoil(d);
    }
    delay(5);
  }
} // namespace instruments
