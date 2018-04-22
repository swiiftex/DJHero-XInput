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

enum HID_Input_Type { KEYBOARD, MOUSE };

class button {
public:
	const char key;
	const HID_Input_Type lib;

	button(char k, HID_Input_Type t = KEYBOARD) : key(k), lib(t) {}

	void press(boolean state = true) {
		if (state == pressed) {
			return; // Nothing to see here, folks
		}

		switch (lib) {
		case KEYBOARD:
			state ? Keyboard.press(key) : Keyboard.release(key);
			break;
		case MOUSE:
			state ? Mouse.press(key) : Mouse.release(key);
			break;
		}

		pressed = state;
	}

	void release() {
		press(false);
	}

private:
	boolean pressed = 0;
};

class RateLimiter {
public:
	RateLimiter(long rate) : UpdateRate(rate) {}

	boolean ready() {
		long timeNow = millis();
		if (timeNow - lastUpdate >= UpdateRate) {
			lastUpdate = timeNow;
			return true;
		}
		return false;
	}

	void reset() {
		lastUpdate = millis();
	}

	const long UpdateRate = 0;  // Rate limit, in ms
private:
	long lastUpdate = 0;
};

// Rate limit functions, all times in milliseconds (ms)
RateLimiter pollRate(4);  // Controller update rate
RateLimiter reconnectRate(500);  // Controller reconnect rate
RateLimiter fxTimeout(1200);  // Timeout for the fx tracker to be zero'd

#define MAIN_TABLE right

#if MAIN_TABLE==right
#define ALT_TABLE left
#elif MAIN_TABLE==left
#define ALT_TABLE right
#endif

DJTurntableController dj;

DJTurntableController::TurntableExpansion * mainTable = &dj.MAIN_TABLE;
DJTurntableController::TurntableExpansion * altTable = &dj.ALT_TABLE;

DJTurntableController::EffectRollover fx(dj);
int16_t fxTotal = 0;

button fire(MOUSE_LEFT, MOUSE);
button boop(MOUSE_RIGHT, MOUSE);
button reload('r');

button ultimate('q');
button amp('e');
button crossfade(KEY_LEFT_SHIFT);

button emotes('c');

button moveForward('w');
button moveLeft('a');
button moveBack('s');
button moveRight('d');
button jump(' ');

// Debug Flags (uncomment to add)
// #define DEBUG // Enable to use any prints
// #define DEBUG_RAW

#ifdef DEBUG
#define DEBUG_PRINT(x)   do {Serial.print(x);}   while(0)
#define DEBUG_PRINTLN(x) do {Serial.println(x);} while(0)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

void setup() {
	#ifdef DEBUG
	Serial.begin(115200);
	while (!Serial);  // Wait for connection
	#endif

	pinMode(9, INPUT_PULLUP);

	if (digitalRead(9) == LOW) {
		for (;;);  // Safety loop!
	}

	Wire.begin();
	startMultiplexer();

	while (!dj.connect()) {
		DEBUG_PRINTLN(F("Couldn't connect to turntable!"));
		delay(500);
	}
	DEBUG_PRINTLN(F("Connected! Starting..."));
}

void loop() {
	if (controllerReady()) {
		djController();
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
	// Movement
	joyWASD(dj.joyX(), dj.joyY());

	// Weapons
	reload.press(effectChange() && fxTotal < 0);

	// Abilities
	ultimate.press(dj.buttonEuphoria());
	amp.press(effectChange() && fxTotal > 0);
	crossfade.press(dj.crossfadeSlider() > 1);

	// Fun stuff!
	emotes.press(dj.buttonPlus());

	// --Cleanup--
	resetEffect();
}

void aiming(int8_t xIn, int8_t yIn) {
	const int8_t HorizontalSens = 5;
	const int8_t VerticalSens = 2;
	const int8_t MaxInput = 20;  // Get rid of extranous values

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
}

void joyWASD(uint8_t x, uint8_t y) {
	button * movementKeys[4] = { &moveRight, &moveLeft, &moveForward, &moveBack }; // X+, X-, Y+, Y-
	const uint8_t joyDeadzone = 7; // +/-, centered at 32

	uint8_t joyXY[2] = { x, y };

	for (int i = 0; i < 2; i++) {
		movementKeys[i * 2]->press(joyXY[i] >= 32 + joyDeadzone);
		movementKeys[i * 2 + 1]->press(joyXY[i] <= 32 - joyDeadzone);
	}
}

boolean effectChange() {
	const int8_t FxThreshold = 10;  // ~1/3rd of a revolution
	const int8_t MaxChange = 5;

	int8_t fxChange = fx.getChange();  // Change since last update

	if (fxChange != 0) {
		fxTimeout.reset();  // Keep alive
	}
	else {
		if (fxTimeout.ready()) {
			fxTotal = 0;
		}
	}

	if (fxChange > MaxChange) {  // Assumed spurious
		fxChange = 0;
	}
	fxTotal += fxChange;

	return abs(fxTotal) >= FxThreshold;
}

void resetEffect() {  // Resets the effect total 
	if (effectChange()) {
		fxTotal = 0;
	}
}

boolean controllerReady() {
	static boolean connected = true;

	// Attempt to reconnect at a regular interval
	if (!connected && reconnectRate.ready()) {
		connected = dj.reconnect();
	}
	
	// If connected, update at a regular interval
	if (connected && pollRate.ready()) {
		connected = dj.update();  // New data
		if (!connected) {
			releaseAll();  // Something went wrong, clear current presses
		}
		else {
			#ifdef DEBUG_RAW
			dj.printDebug();
			#endif
		}
		return connected;
	}

	return false;
}

void releaseAll() {
	fire.release();
	boop.release();
	reload.release();

	ultimate.release();
	amp.release();
	crossfade.release();
	
	emotes.release();

	moveForward.release();
	moveLeft.release();
	moveBack.release();
	moveRight.release();
	jump.release();
}

void startMultiplexer() {
	Wire.beginTransmission(0x70);
	Wire.write(1);
	Wire.endTransmission();
}
