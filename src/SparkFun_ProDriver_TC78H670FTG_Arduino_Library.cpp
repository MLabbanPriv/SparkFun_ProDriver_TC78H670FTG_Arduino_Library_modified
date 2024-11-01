/*
  This is a library written for the SparkFun ProDriver TC78H670FTG Stepper Motor Driver
  
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/16836

  Written by Pete Lewis @ SparkFun Electronics, July 6th, 2020

  The SparkFun ProDriver is a stepper motor driver that can be controlled via "clock in mode" or "serial mode".
  This library provides control of the following features:
    -setup (includes mode and resolution select)
    -movement commands via clock-in mode and serial mode
    -max current control (as a percentage of VREF - which is set with a trimpot on the product)
    -change step resolution (using SET_EN, UP-DOWN and CLK)

  https://github.com/sparkfun/SparkFun_ProDriver_TC78H670FTG_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.13

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SparkFun_ProDriver_TC78H670FTG_Arduino_Library.h"
#include "Arduino.h"
#include <stdint.h>

//****************************************************************************//
//
//  Constructor
//
//    Initalizes settings to default.
//
//****************************************************************************//
PRODRIVER::PRODRIVER( void )
{
	//Construct with these default settings if nothing is specified

	//Select control mode
	settings.controlMode = PRODRIVER_MODE_CLOCKIN; // PRODRIVER_MODE_CLOCKIN or PRODRIVER_MODE_SERIAL
	settings.stepResolutionMode = PRODRIVER_STEP_RESOLUTION_FIXED_FULL; // many options, see header file or ds pg 8
  settings.stepResolution = PRODRIVER_STEP_RESOLUTION_1_1; // IC default on bootup
  
  //Select default settings for SERIAL MODE specific settings
  settings.phaseA = PRODRIVER_PHASE_PLUS;
  settings.phaseB = PRODRIVER_PHASE_PLUS;
  settings.currentLimA = 1023; // max
  settings.currentLimB = 1023; // max
  settings.torque = PRODRIVER_TRQ_100;
  settings.openDetection = PRODRIVER_OPD_OFF;
  settings.mixedDecayA = PRODRIVER_MD_FAST_37;
  settings.mixedDecayB = PRODRIVER_MD_FAST_37;
  settings.phasePosition = 1;
  settings.fastSerialMode = false; // default to false

  //Select default arduino pin numbers for hardware connections
	settings.mode0Pin =   PRODRIVER_DEFAULT_PIN_MODE_0;
  settings.mode1Pin =   PRODRIVER_DEFAULT_PIN_MODE_1;
  settings.mode2Pin =   PRODRIVER_DEFAULT_PIN_MODE_2;
  settings.mode3Pin =   PRODRIVER_DEFAULT_PIN_MODE_3;
  settings.enablePin =  PRODRIVER_DEFAULT_PIN_EN;
  settings.standbyPin = PRODRIVER_DEFAULT_PIN_STBY;
  settings.errorPin =   PRODRIVER_DEFAULT_PIN_ERROR;

  //statuses/flags
  settings.enableStatus = PRODRIVER_STATUS_DISABLED;  
  settings.standbyStatus = PRODRIVER_STATUS_STANDBY_ON;
  settings.errorFlag = false; // false = no error
}

//Initializes the motor driver with basic settings
//Returns false if error is detected (i.e. ERR pin is pulled low by the IC)
bool PRODRIVER::begin( void )
{
  pinSetup(); // sets arduino pins to necessary initial pinModes and statuses
  controlModeSelect(); // "boots up" IC with correct statuses on MODE pins

  return errorStat(); //We're all setup!
}

// pinSetup( void )
// setup Arduino pins as inputs/outputs as needed, and to default settings (disabled, standby)
bool PRODRIVER::pinSetup( void )
{
  // enable is active high 
  // OUTPUT, HIGH = enabled (Must also have hardware switch set to "USER")
  // OUTPUT, LOW = disabled (on-board pulldown will also disable)
  // note, hardware slide switch can also over-ride this when set to position "OFF"
  pinMode(settings.enablePin, OUTPUT);
  digitalWrite(settings.enablePin, LOW); // disabled
  settings.enableStatus = PRODRIVER_STATUS_DISABLED; // update settings so we can check if needed

  // standby is active low. 
  // OUTPUT, LOW = standby
  // OUTPUT, HIGH = not in standby
  pinMode(settings.standbyPin, OUTPUT);
  digitalWrite(settings.standbyPin, LOW); // hold in standby, until we're ready to go
  settings.standbyStatus = PRODRIVER_STATUS_STANDBY_ON;

  // error is active low 
  // This will always be an input pin (on your microcontroller)
  // note, this pin on the IC has a dual purpose (both EN and ERR).
  pinMode(settings.errorPin, INPUT); // input without pullup 

  // all mode pins to output low
  pinMode(settings.mode0Pin, OUTPUT);
  digitalWrite(settings.mode0Pin, LOW);
  pinMode(settings.mode1Pin, OUTPUT);
  digitalWrite(settings.mode1Pin, LOW);
  pinMode(settings.mode2Pin, OUTPUT);
  digitalWrite(settings.mode2Pin, LOW);
  pinMode(settings.mode3Pin, OUTPUT);
  digitalWrite(settings.mode3Pin, LOW);      

  return errorStat();

}

// controlModeSelect( void )
// This "boots up" the IC into the desired mode.
// VM (the main motor voltage supply) must be stable
// Must be in standby mode.
// Mode pins must be set to desired mode, then standby must be released.
// see datasheet pg 6
bool PRODRIVER::controlModeSelect( void )
{
  // set mode pins according to desired mode
  switch (settings.controlMode)
  {
  case PRODRIVER_MODE_SERIAL:
    // set mode pins to all LOW for serial mode
    pinMode(settings.mode0Pin, OUTPUT);
    digitalWrite(settings.mode0Pin, LOW);
    pinMode(settings.mode1Pin, OUTPUT);
    digitalWrite(settings.mode1Pin, LOW);
    pinMode(settings.mode2Pin, OUTPUT);
    digitalWrite(settings.mode2Pin, LOW);
    pinMode(settings.mode3Pin, OUTPUT);
    digitalWrite(settings.mode3Pin, LOW);     
    break;

  case PRODRIVER_MODE_CLOCKIN:
    // set mode pins as needed
    // note, when you set controlmode to CLOCKIN,
    // you are also setting the step resolution, which can
    // be a variety of options.

    // grab "bits" from the desired step resolution
    // these will be used to set each mode pin high/low

    // mode0Pin
    if(bitRead(settings.stepResolutionMode, 0))
    {
      pinMode(settings.mode0Pin, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    }
    else{
      pinMode(settings.mode0Pin, OUTPUT);
      digitalWrite(settings.mode0Pin, LOW);
    }

    // mode1Pin
    if(bitRead(settings.stepResolutionMode, 1))
    {
      pinMode(settings.mode1Pin, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    }
    else{
      pinMode(settings.mode1Pin, OUTPUT);
      digitalWrite(settings.mode1Pin, LOW);
    }    

    // mode2Pin
    if(bitRead(settings.stepResolutionMode, 2))
    {
      pinMode(settings.mode2Pin, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    }
    else{
      pinMode(settings.mode2Pin, OUTPUT);
      digitalWrite(settings.mode2Pin, LOW);
    }        

    // mode3Pin
    if(bitRead(settings.stepResolutionMode, 3))
    {
      pinMode(settings.mode3Pin, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    }
    else{
      pinMode(settings.mode3Pin, OUTPUT);
      digitalWrite(settings.mode3Pin, LOW);
    }        
    break;

  default:
    break;

  }
  
  // wait TmodeSU (mode setting setup time) minimum 1 microsecond
  delayMicroseconds(1);

  // release standby (write it HIGH)
  digitalWrite(settings.standbyPin, HIGH);
  settings.standbyStatus = PRODRIVER_STATUS_STANDBY_OFF; // update setting to check as needed

  // wait TmodeHO (mode setting Data hold time) minimum 100 microseconds
  delayMicroseconds(100);

  return errorStat();
}

// errorStat( void )
// checks the status of the ERR net.
// Note, one pin on the IC shares the purpose of both ERR and EN (enable)
// If ERR reads low, then one of three things is wrong:
// Thermal shutdown (TSD) 
// Overcurrent (ISD)
// Motor Load Open (OPD)
// returns true if things are good and there is no error detected

bool PRODRIVER::errorStat( void )
{
  if(digitalRead(settings.errorPin) == true)
  {
    return true; // there is no error, so let's indicate that with a true.
  }
  return false; // if we get here, then errorPin was low, indicating an error
  // false means error!
}

// step( uint16_t steps, bool direction )
// using CLOCKIN mode,
// step the motor a set amount of steps at the desired direction
// will stop if error is detected during stepping
// retuns errorStat()

bool PRODRIVER::step( uint16_t steps, bool direction, uint8_t clockDelay,float delayRatio )
{
  enable();

  // set CW-CWW pin (aka mode3Pin) to the desired direction
  // CW-CCW pin controls the rotation direction of the motor. 
  // When set to H, the current of OUT_A is output first, with a phase difference of 90°. 
  // When set to L, the current of OUT_B is output first with a phase difference of 90°
  if(direction == true)
  {
    pinMode(settings.mode3Pin, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
  }
  else{
    pinMode(settings.mode3Pin, OUTPUT);
    digitalWrite(settings.mode3Pin, LOW);
  }
  
  // step the motor the desired amount of steps
  // each up-edge of the CLK signal (aka mode2Pin) 
  // will shift the motor's electrical angle per step.
  for(uint16_t i = 0 ; i < steps ; i++)
  {
    pinMode(settings.mode2Pin, OUTPUT);
    digitalWrite(settings.mode2Pin, LOW);
    delayMicroseconds(1); // even out the clock signal, error check takes about 2uSec
    delayMicroseconds(clockDelay*delayRatio);
    pinMode(settings.mode2Pin, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    // check for error
    if(errorStat() == false) return false; // error detected, exit out of here!
    delayMicroseconds(clockDelay*(1-delayRatio));
  }
  return errorStat();
}

// changeStepResolution( uint8_t resolution)
// Step resolution can be changed during operating.
// Step resolution can be set by SET_EN pin and UP-DW pin. 
// Step mode is changed synchronously with Step Clock. 
// see datasheet pg 9
bool PRODRIVER::changeStepResolution( uint8_t resolution)
{
  // If user asks to change resolution, but we are already there, then
  // just return errorStat()
  if(settings.stepResolution == resolution) return errorStat();


  // convert pin names to CLOCKIN specific names
  // (for ease of programming)
  uint8_t setEn = settings.mode1Pin;
  uint8_t updw = settings.mode0Pin;
  uint8_t clock = settings.mode2Pin;

  // setup pins for a new step resolution change. Both need to be pulled HIGH
    pinMode(clock, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    pinMode(setEn, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH

  // find out how many shifts we need in either direction.
  uint8_t shift = 0;

  // resolution and settings.stepResolution actually are the divisor of the step resolution fraction,
  // so we can compare the two to find out whether we need to "upshift" or "downshift"
  // if the new desired setting (resolution) is a larger number, then that means we are actually
  // moving from a lower resolution to a higher one (like moving from 1:1 to 1:8)
  if(resolution > settings.stepResolution) // we are moving to a higher resolution
  {
    pinMode(updw, OUTPUT);
    digitalWrite(updw, LOW);
    if ( (resolution / 2) == settings.stepResolution ) shift = 1;
    else if ( (resolution / 4) == settings.stepResolution ) shift = 2;
    else if ( (resolution / 8) == settings.stepResolution ) shift = 3;
    else if ( (resolution / 16) == settings.stepResolution ) shift = 4;
    else if ( (resolution / 32) == settings.stepResolution ) shift = 5;
    else if ( (resolution / 64) == settings.stepResolution ) shift = 6;
    else if ( (resolution / 128) == settings.stepResolution ) shift = 7;
  }
  else{ // we are moding to lower resolution
    pinMode(updw, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    if ( (settings.stepResolution / 2) == resolution ) shift = 1;
    else if ( (settings.stepResolution / 4) == resolution ) shift = 2;
    else if ( (settings.stepResolution / 8) == resolution ) shift = 3;
    else if ( (settings.stepResolution / 16) == resolution ) shift = 4;
    else if ( (settings.stepResolution / 32) == resolution ) shift = 5;
    else if ( (settings.stepResolution / 64) == resolution ) shift = 6;
    else if ( (settings.stepResolution / 128) == resolution ) shift = 7;
  }

  // toggle clock cycles as needed to cause the right amount of "shifts" in resolution
  for(uint8_t i = 0 ; i < shift ; i++)
  {
    pinMode(clock, OUTPUT);
    digitalWrite(clock, LOW);
    delayMicroseconds(2);
    pinMode(clock, INPUT); // let on-board external pullup to 3.3V pull this pin HIGH
    delayMicroseconds(2);
  }

  // we're finished, so let's set SET_EN back to LOW, so it doesn't look at UP-DW anymore
  pinMode(setEn, OUTPUT);
  digitalWrite(setEn, LOW);

  // update setting member so we can compare next time we change
  settings.stepResolution = resolution;
  
  return errorStat();
}

// enable ( void )
// sets the enable pin to HIGH
// but only if we need to (i.e. we are currently disabled)
bool PRODRIVER::enable( void )
{
  // Check to see if we are already enabled. If so, then leave pin alone.
  // If we are not enabled, then enable.
  if(settings.enableStatus == PRODRIVER_STATUS_ENABLED)
  {
    // do nothing, we are already enabled.
    // this helps avoid re-writing the enable pin
    // which actually toggles the pin when you do a digitalWrite()
  }
  else{ // we are not enabled, so let's enable
    digitalWrite(settings.enablePin, HIGH);
    settings.enableStatus = PRODRIVER_STATUS_ENABLED; // update settings so we can check as needed
  }
  return errorStat();
}

// diable( void )
// sets the enable pin to LOW
// but only if we need to (i.e. we are currently enabled)
bool PRODRIVER::disable( void )
{
  // Check to see if we are already disabled. If so, then leave pin alone.
  // If we are not disabled, then disable.
  if(settings.enableStatus == PRODRIVER_STATUS_DISABLED)
  {
    // do nothing, we are already disabled.
    // this helps avoid re-writing the enable pin
    // which actually toggles the pin when you do a digitalWrite()
  }
  else{ // we are not disabled, so let's disable
    digitalWrite(settings.enablePin, LOW);
    settings.enableStatus = PRODRIVER_STATUS_DISABLED; // update settings so we can check as needed
  }  
  return errorStat();
}

// sendSerialCommand ( void )
// send a single serial data command to the driver IC
// Note, you must first adjust all of your desired settings by accessing
// the members within PRODRIVERSetting, and then call this function
// to actually send a fresh new command.
bool PRODRIVER::sendSerialCommand( void )
{
  // first lets construct the 32 bit data package that we need to send.
  // we will do this by taking in all of the settings and plugging them
  // into the correct bits.
  
  uint32_t command = 0; // start fresh

  // set the phase bits
  command |= (settings.phaseA << 2);
  command |= ((uint32_t)settings.phaseB << 18);

  // set the current limits
  command |= (settings.currentLimA << 3);
  command |= ((uint32_t)settings.currentLimB << 19);

  // set the torque bits
  command |= ((uint32_t)settings.torque << 29);

  // set the open detection bit
  command |= ((uint32_t)settings.openDetection << 31);

  // set the mixed decay bits
  command |= settings.mixedDecayA; // bit 0, no shift necessary
  command |= ((uint32_t)settings.mixedDecayB << 16);

  // write the data 
  // check for fast serial mode
  if(settings.fastSerialMode == true)
  {
    pinMode(settings.mode0Pin, OUTPUT);
    pinMode(settings.mode1Pin, OUTPUT);
    pinMode(settings.mode2Pin, OUTPUT);
    for(int i = 0 ; i < 32 ; i++)
    {
      digitalWrite(settings.mode2Pin, HIGH); // clock
      if(bitRead(command, i))
      {
        digitalWrite(settings.mode0Pin, HIGH); // data
      }
      else{
        digitalWrite(settings.mode0Pin, LOW); // data
      }
      digitalWrite(settings.mode2Pin, LOW); // clock
    }

    // write latch "high", then low
    digitalWrite(settings.mode1Pin, HIGH); // latch "HIGH". let on-board external pullup to 3.3V pull this pin HIGH  
    delayMicroseconds(1);
    digitalWrite(settings.mode1Pin, LOW); // latch
    delayMicroseconds(1);
  }
  else{
    for(int i = 0 ; i < 32 ; i++)
    {
      pinMode(settings.mode2Pin, INPUT); // clock "HIGH". let on-board external pullup to 3.3V pull this pin HIGH
      delayMicroseconds(1);
      if(bitRead(command, i))
      {
        pinMode(settings.mode0Pin, INPUT); // data "HIGH". let on-board external pullup to 3.3V pull this pin HIGH
      }
      else{
        pinMode(settings.mode0Pin, OUTPUT); // data LOW
        digitalWrite(settings.mode0Pin, LOW); // data LOW
      }
      delayMicroseconds(1);
      pinMode(settings.mode2Pin, OUTPUT); // clock
      digitalWrite(settings.mode2Pin, LOW); // clock
      delayMicroseconds(1);
    }

    // write latch "high", then low
    pinMode(settings.mode1Pin, INPUT); // latch "HIGH". let on-board external pullup to 3.3V pull this pin HIGH  
    delayMicroseconds(1);
    pinMode(settings.mode1Pin, OUTPUT); // latch
    digitalWrite(settings.mode1Pin, LOW); // latch
    delayMicroseconds(1);
  }

  //Serial.println(command, BIN);
  
  //return errorStat();
  return true;
}

// stepSerial( uint16_t steps, bool direction, uint8_t stepDelay )
// using SERIAL mode,
// step the motor a set amount of steps at the desired direction
// note, only 1:1 stepping, no microstepping supported
// will stop if error is detected during stepping
// retuns errorStat()

bool PRODRIVER::stepSerial(uint16_t steps, bool direction, uint8_t stepDelay)
{
  enable();
  for(int i = 0; i < steps ; i++)
  {
    if(stepSerialSingle(direction) == false) return false;
    delay(stepDelay);
  }
  return true;
}


bool PRODRIVER::stepSerialSingle(bool direction)
{
  // phasePosition should only be 1,2,3 or 4
  // depending on direction, we need to increment or decrement,
  // and roll over if we go beyond min/max
  // EXAMPLE:
  // to move 5 steps forward, the following phasePositions would be used in sequence:
  // 1,2,3,4,1
  // and then (from the last position used,1) to move 5 steps backwards, the following 
  // phasePositions would be used:
  // 4,3,2,1,4

  // check for error
  if(settings.errorFlag) return false;
  
  if(direction == true)
  {
    settings.phasePosition++;
    if(settings.phasePosition > 4) settings.phasePosition = 1; // roll over
  }
  else
  {
    settings.phasePosition--;
    if(settings.phasePosition < 1) settings.phasePosition = 4; // roll over
  }

  switch (settings.phasePosition)
  {
  case 1:
    settings.phaseA = 1;
    settings.phaseB = 1;
    return sendSerialCommand();
    break;

  case 2:
    settings.phaseA = 0;
    settings.phaseB = 1;
    return sendSerialCommand();
    break;

  case 3:
    settings.phaseA = 0;
    settings.phaseB = 0;
    return sendSerialCommand();
    break;

  case 4:
    settings.phaseA = 1;
    settings.phaseB = 0;
    return sendSerialCommand();
    break;        

  default:
    return false;
    break;
  }
}

// setTorque( uint8_t newTorque )
// This is simply a wrapper function to set desired torque setting
// Note, this will only take effect on the motor driver when sendSerialCommand() is called.
// Valid torque options include the following:
// PRODRIVER_TRQ_100 (default set in constructor)
// PRODRIVER_TRQ_75
// PRODRIVER_TRQ_50
// PRODRIVER_TRQ_25
bool PRODRIVER::setTorque( uint8_t newTorque )
{
  settings.torque = newTorque;
  return true;
}

// setCurrentLimit( uint16_t currentLimit )
// This is simply a wrapper function to set desired current limit setting
// Note, this effects current limit on both coils (A and B)
// Note, this will only take effect on the motor driver when sendSerialCommand() is called.
// currentLimit is a 10-bit value, so must be 0-1023
// currentLimit is a percentage of VREF and also limited by torque setting.
bool PRODRIVER::setCurrentLimit( uint16_t currentLimit )
{
  if( (currentLimit < 0) || (currentLimit > 1023) ) // protect against invalid user inputs
  {
    // do nothing, user input is outside of valid range
    return false;
  }
  settings.currentLimA = currentLimit;
  settings.currentLimB = currentLimit;
  return true;
}
