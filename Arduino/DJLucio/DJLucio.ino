/*
*  Project     DJ Hero - Lucio
*  @author     David Madison
*  @link       github.com/dmadison/DJHero-Lucio
*  @license    GPLv3 - Copyright (c) 2018 David Madison
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
*/

#include <NintendoExtensionCtrl.h>
#include <Mouse.h>
#include <Keyboard.h>

#include <EEPROM.h>

// User Settings
const int8_t HorizontalSens = 5;  // Mouse sensitivity multipler - 6 max
const int8_t VerticalSens   = 2;  // Mouse sensitivity multipler - 6 max

// Tuning Options
const unsigned long UpdateRate = 4;          // Controller polling rate, in milliseconds (ms)
const unsigned long DetectTime = 1000;       // Time before a connected controller is considered stable (ms)
const unsigned long ConnectRate = 500;       // Rate to attempt reconnections, in ms
const unsigned long ConfigThreshold = 3000;  // Time the euphoria and green buttons must be held to set a new config (ms)
const unsigned long EffectsTimeout = 1200;   // Timeout for the effects tracker, in ms
const uint8_t       EffectThreshold = 10;    // Threshold to trigger abilities from the fx dial, 10 = 1/3rd of a revolution

// Debug Flags (uncomment to add)
// #define DEBUG                // Enable to use any prints
// #define DEBUG_RAW            // See the raw data from the turntable
// #define DEBUG_HID            // See HID inputs as they're pressed/released
// #define DEBUG_COMMS          // Follow the controller connect and update calls
// #define DEBUG_CONTROLDETECT  // Trace the controller detect pin functions
// #define DEBUG_CONFIG         // Debug the config read/set functionality

// ---------------------------------------------------------------------------

#include "DJLucio_Util.h"  // Utility classes

DJTurntableController dj;

DJTurntableController::TurntableExpansion * mainTable = &dj.right;
DJTurntableController::TurntableExpansion * altTable = &dj.left;

MouseButton fire(MOUSE_LEFT);
MouseButton boop(MOUSE_RIGHT);
KeyboardButton reload('r');

KeyboardButton ultimate('q');
KeyboardButton amp('e');
KeyboardButton crossfade(KEY_LEFT_SHIFT);

KeyboardButton emotes('c');

KeyboardButton moveForward('w');
KeyboardButton moveLeft('a');
KeyboardButton moveBack('s');
KeyboardButton moveRight('d');
KeyboardButton jump(' ');

EffectHandler fx(dj, EffectsTimeout);

const uint8_t DetectPin = 4;  // Pulled low by EXTERNAL pull-down (not optional!)
const uint8_t SafetyPin = 9;  // Pulled high by internal pull-up
const uint8_t LEDPin = 17;    // RX LED on the Pro Micro

const boolean LEDInverted = true;  // Inverted on the Pro Micro (LOW is lit)

LEDHandler LED(LEDPin, LEDInverted);
ConnectionHelper controller(dj, DetectPin, UpdateRate, DetectTime, ConnectRate);
TurntableConfig config(dj, &DJTurntableController::buttonEuphoria, &DJTurntableController::TurntableExpansion::buttonGreen, 3000);

void setup() {
	#ifdef DEBUG
	Serial.begin(115200);
	while (!Serial);  // Wait for connection
	#endif

	pinMode(SafetyPin, INPUT_PULLUP);

	if (digitalRead(SafetyPin) == LOW) {
		for (;;);  // Safety loop!
	}

	startMultiplexer();  // Enable the multiplexer, currently being used as a level shifter (to be removed)

	LED.begin();  // Set LED pin mode
	config.read();  // Set expansion pointers from EEPROM config
	controller.begin();  // Initialize controller bus and detect pins

	while (!controller.isReady()) {
		DEBUG_PRINTLN(F("Couldn't connect to turntable!"));
		delay(500);
	}
	DEBUG_PRINTLN(F("Connected! Starting..."));
}

void loop() {
	if (controller.isReady()) {
		djController();
		config.check();
		LED.update();
	}
}

void djController() {
	// Dual turntables
	if (dj.getNumTurntables() == 2) {
		if (!dj.buttonMinus()) {  // Button to disable aiming (for position correction)
			aiming(mainTable->turntable(), altTable->turntable());
		}

		// Movement
		jump.press(mainTable->buttonRed());

		// Weapons
		fire.press(mainTable->buttonGreen() || mainTable->buttonBlue());  // Outside buttons
		boop.press(altTable->buttonGreen() || altTable->buttonRed() || altTable->buttonBlue());
	}

	// Single turntable (either side)
	else if (dj.getNumTurntables() == 1) {
		// Aiming
		if (dj.buttonMinus()) {  // Vertical selector
			aiming(0, dj.turntable());
		}
		else {
			aiming(dj.turntable(), 0);
		}

		// Movement
		jump.press(dj.buttonRed());

		// Weapons
		fire.press(dj.buttonGreen());
		boop.press(dj.buttonBlue());
	}

	// --Base Station Abilities--
	fx.update();

	// Movement
	joyWASD(dj.joyX(), dj.joyY());

	// Weapons
	reload.press(fx.changed(EffectThreshold) && fx.getTotal() < 0);

	// Abilities
	ultimate.press(dj.buttonEuphoria());
	amp.press(fx.changed(EffectThreshold) && fx.getTotal() > 0);
	crossfade.press(dj.crossfadeSlider() > 1);

	// Fun stuff!
	emotes.press(dj.buttonPlus());

	// --Cleanup--
	if (fx.changed(EffectThreshold)) {
		fx.reset();  // Already used abilities, reset to 0
	}
}

void aiming(int8_t xIn, int8_t yIn) {
	const int8_t MaxInput = 20;  // Ignore values above this threshold as extranous
	static_assert(HorizontalSens * MaxInput <= 127, "Your sensitivity is too high!");  // Check for signed overflow (int8_t)
	static_assert(VerticalSens   * MaxInput <= 127, "Your sensitivity is too high!");

	static int8_t lastAim[2] = { 0, 0 };
	int8_t * aim[2] = { &xIn, &yIn };  // Store in array for iterative access

	// Iterate through X/Y
	for (int i = 0; i < 2; i++) {
		// Check if above max threshold
		if (abs(*aim[i]) >= MaxInput) {
			*aim[i] = lastAim[i];
		}

		// Set 'last' value to current
		lastAim[i] = *aim[i];
	}

	Mouse.move(xIn * HorizontalSens, yIn * VerticalSens);

	#ifdef DEBUG_HID
	if (xIn != 0 || yIn != 0) {
		DEBUG_PRINT("Moved the mouse {");
		DEBUG_PRINT(xIn * HorizontalSens);
		DEBUG_PRINT(", ");
		DEBUG_PRINT(yIn * VerticalSens);
		DEBUG_PRINTLN("}");
	}
	#endif
}

void joyWASD(uint8_t x, uint8_t y) {
	const uint8_t JoyCenter = 32;
	const uint8_t JoyDeadzone = 6;  // +/-, centered at 32 in (0-63)

	moveLeft.press(x < JoyCenter - JoyDeadzone);
	moveRight.press(x > JoyCenter + JoyDeadzone);

	moveForward.press(y > JoyCenter + JoyDeadzone);
	moveBack.press(y < JoyCenter - JoyDeadzone);
}

void startMultiplexer() {
	Wire.begin();
	Wire.beginTransmission(0x70);
	Wire.write(1);
	Wire.endTransmission();
}
