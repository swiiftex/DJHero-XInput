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

#ifndef DJSpinRhythm_HID_h
#define DJSpinRhythm_HID_h
#include <XInput.h>
#include "DJSpinRhythm_Util.h"
#ifdef DEBUG_HID
#define D_HID(x)   DEBUG_PRINT(x)
#define D_HIDLN(x) DEBUG_PRINTLN(x)
#else
#define D_HID(x)
#define D_HIDLN(x)
#endif

// HID_Button: Handles HID button state to prevent input spam
class HID_Button {
public:
	HID_Button(uint16_t k) : key(k) {
		// If linked list is empty, set both head and tail
		if (head == nullptr) {
			head = this;
			tail = this;
		}
		// If linked list is *not* empty, set the 'next' ptr of the tail
		// and update the tail
		else {
			tail->next = this;
			tail = this;
		}
	}

	~HID_Button() {
		// If we're at the start of the list...
		if (this == head) {
			// Option #1: Only element in the list
			if (this == tail) {
				head = nullptr;
				tail = nullptr;  // List is now empty
			}
			// Option #2: First element in the list,
			// but not *only* element
			else {
				head = next;  // Set head to next, and we're done
			}
			return;
		}

		// Option #3: Somewhere else in the list.
		// Iterate through to find it
		HID_Button * ptr = head;

		while (ptr != nullptr) {
			if (ptr->next == this) {  // FOUND!
				ptr->next = next;  // Set the previous "next" as this entry's "next" (skip this object)
				break;  // Stop searching
			}
			ptr = ptr->next;  // Not found. Next entry...
		}

		// Option #4: Last entry in the list
		if (this == tail) {
			tail = ptr;  // Set the tail as the previous entry
		}
	}

	void press() {
		set(true);
	}

	void release() {
		set(false);
	}

	void set(boolean state) {
		if (state == pressed) {
			return; // Nothing to see here, folks
		}

		sendState(state);
		pressed = state;
	}

	boolean isPressed() const {
		return pressed;
	}

	// Release all buttons, using the linked list
	static void releaseAll() {
		HID_Button * ptr = head;

		while (ptr != nullptr) {
			ptr->release();
			ptr = ptr->next;
		}
	}

	const uint16_t key;
private:
	static HID_Button * head;
	static HID_Button * tail;

	virtual void sendState(boolean state) = 0;
	boolean pressed = 0;
	HID_Button * next = nullptr;
};

// Allocate space for the static pointers
HID_Button * HID_Button::head = nullptr;
HID_Button * HID_Button::tail = nullptr;

// HID_Button: Sending controller
class ControllerButton : public HID_Button {
public:
	using HID_Button::HID_Button;
private:
	void sendState(boolean state) {
		state ? XInput.press(key) : XInput.release(key);
	}
};


#endif
