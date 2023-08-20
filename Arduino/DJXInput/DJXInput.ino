/*
*  Project     DJ Hero - Spin Rhythm XD Controller
*  @author     David Madison
*  @link       github.com/dmadison/DJHero-SpinRhythm
*  @license    GPLv3 - Copyright (c) 2020 David Madison
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  Forked from the DJ Hero Lucio project: github.com/dmadison/DJHero-Lucio
*/

#include <NintendoExtensionCtrl.h>

// User Settings
const int8_t WheelSensitivity = 20;
const int8_t MaxWheelInput = 127;    // Igrgnore values above this threshold as extranous
const int8_t IgnoreFretsOnWheelSpeed = 99999;  // Don't update the frets if the wheel is going above this speed

// Tuning Options
const unsigned long UpdateRate = 2;          // Controller polling rate, in milliseconds (ms)
const unsigned long DetectTime = 1000;       // Time before a connected controller is considered stable (ms)
const unsigned long ConnectRate = 500;       // Rate to attempt reconnections, in ms
const unsigned long EffectsTimeout = 1200;    // Timeout for the effects tracker, in ms
const uint8_t       EffectThreshold = 1;     // Threshold to trigger input from the fx dial. One revolution = 31.
#define IGNORE_DETECT_PIN                 // Ignore the state of the 'controller detect' pin, for breakouts without one.

// Debug Flags (uncomment to add)
// #define DEBUG                // Enable to use any prints
// #define DEBUG_RAW            // See the raw data from the turntable
// #define DEBUG_HID            // See HID inputs as they're pressed/released
// #define DEBUG_COMMS          // Follow the controller connect and update calls
// #define DEBUG_CONTROLDETECT  // Trace the controller detect pin functions

// ---------------------------------------------------------------------------

#include "DJSpinRhythm_LED.h"   // LED handling classes
#include "DJSpinRhythm_HID.h"   // HID classes (Keyboard, Mouse)
#include "DJSpinRhythm_Controller.h"  // Turntable connection and data helper classes

DJTurntableController dj;
ConnectionHelper controller(dj, DetectPin, UpdateRate, DetectTime, ConnectRate);
XInputControl deckleft(TRIGGER_LEFT);
XInputControl deckright(TRIGGER_RIGHT);
ControllerButton green(BUTTON_A);
ControllerButton red(BUTTON_B);
ControllerButton blue(BUTTON_X);
ControllerButton crossfadeLeft(BUTTON_LB);
ControllerButton crossfadeRight(BUTTON_RB);
XInputControl effect(JOY_RIGHT);
ControllerButton plus(BUTTON_START);
ControllerButton minus(BUTTON_BACK);
ControllerButton euphoria(BUTTON_Y);
ControllerButton up(DPAD_UP);
ControllerButton down(DPAD_DOWN);
ControllerButton left(DPAD_LEFT);
ControllerButton right(DPAD_RIGHT);


EffectHandler fx(dj, EffectsTimeout);

void setup() {
	#ifdef DEBUG
	while (!Serial);  // Wait for connection
	#endif
  XInput.setAutoSend(false);
  XInput.begin();

	#if defined(USB_SERIAL_HID) || defined(__AVR_ATmega32U4__)
	Serial.begin(115200);
	Serial.println("DJ Hero XInput");
	Serial.println("By Owen BK, 2023");
	Serial.println("http://swiiftex.github.io");
	Serial.println("----------------------------");
	#endif

	LED.begin();  // Set LED pin mode
	controller.begin();  // Initialize controller bus and detect pins

	DEBUG_PRINTLN("Initialization finished. Starting program...");
}

void loop() {
	if (controller.isReady()) {
		djController();
	}
	LED.update();
}

void djController() {
	// Turntable Controls
	using TableConfig = DJTurntableController::TurntableConfig;  // borrow enum from library
	const TableConfig config = dj.getTurntableConfig();
	// const uint8_t crossfade = dj.crossfadeSlider();  // use slider as selector

  // FX
  	fx.update();  // update tracker with new data
    if (fx.getTotal() > 0){
      XInput.setJoystick(effect, constrain(fx.getTotal(), 0, 30), 0);
    }
    else if(fx.getTotal() < 0){
      XInput.setJoystick(effect, 0, constrain((fx.getTotal() * - 1), 0, 30));
    }
    else {
      XInput.setJoystick(effect, 0, 0);
    }


  // TURNTABLE
    if (dj.turntable() > 0.1){
      XInput.setTrigger(deckleft, (dj.turntable() * WheelSensitivity));
      XInput.setTrigger(deckright, 0);
    }
    else if(dj.turntable() < -0.1){
      XInput.setTrigger(deckright, ((dj.turntable() * -1) * WheelSensitivity));
      XInput.setTrigger(deckleft, 0);
    }
    else {
      XInput.setTrigger(deckleft, 0);
      XInput.setTrigger(deckright, 0);
    }

  // Frets
  red.set(dj.buttonRed());
  blue.set(dj.buttonBlue());
  green.set(dj.buttonGreen());

  euphoria.set(dj.buttonEuphoria());
  
	// Base Unit Controls
	minus.set(dj.buttonMinus());
	plus.set(dj.buttonPlus());

	// Menu Navigation
  joyWASD(dj.joyX(), dj.joyY());


  // Crossfade
  // 7/8 is centered
  crossfadeRight.set(dj.crossfadeSlider() > 10);
  crossfadeLeft.set(dj.crossfadeSlider() < 5);

  XInput.send();
  XInput.printDebug();
}

  void joyWASD(uint8_t x, uint8_t y) {
	const uint8_t JoyCenter = 32;
	const uint8_t JoyDeadzone = 6;  // +/-, centered at 32 in (0-63)

	left.set(x < JoyCenter - JoyDeadzone);
	right.set(x > JoyCenter + JoyDeadzone);

	up.set(y > JoyCenter + JoyDeadzone);
	down.set(y < JoyCenter - JoyDeadzone);
}

