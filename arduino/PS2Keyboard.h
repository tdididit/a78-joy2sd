/*
  PS2Keyboard.h - PS2Keyboard library
  Copyright (c) 2007 Free Software Foundation.  All right reserved.
  Written by Christian Weichel <info@32leaves.net>

  ** Modified for use with Arduino 13 by L. Abraham Smith, <n3bah@microcompdesign.com> * 

  ** Modified to include: shift, alt, caps_lock and caps_lock light by Bill Oldfield *

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef PS2Keyboard_h
#define PS2Keyboard_h


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/*
 * PS2 keyboard "make" codes to check for certain keys.
 */

// Give these codes that aren't used by anything else
// Making all the control key codes above 0x80 makes it simple to check for
// printable characters at the calling level.
#define PS2_KC_BKSP    0x80
#define PS2_KC_UP      0x81
#define PS2_KC_DOWN    0x82
#define PS2_KC_LEFT    0x83
#define PS2_KC_RIGHT   0x84
#define PS2_KC_PGDN    0x85
#define PS2_KC_PGUP    0x86
#define PS2_KC_END     0x87
#define PS2_KC_HOME    0x88
#define PS2_KC_INS     0x89
#define PS2_KC_DEL     0x8A
#define PS2_KC_ESC     0x8B
#define PS2_KC_CLON    0x8C // caps_lock on
#define PS2_KC_CLOFF   0x8D // caps_lock off


#include "binary.h"
// typedef uint8_t boolean;
typedef uint8_t byte;

/*
 * This PIN is hardcoded in the init routine later on. If you change this
 * make sure you change the interrupt initialization as well.
 */
#define PS2_INT_PIN 3

/**
 * Purpose: Provides an easy access to PS2 keyboards
 * Author:  Christian Weichel
 */
class PS2Keyboard {

  private:
    int  m_dataPin;
    byte m_charBuffer;

  public:
  	/**
  	 * This constructor does basically nothing. Please call the begin(int)
  	 * method before using any other method of this class.
  	 */
  	PS2Keyboard();

    /**
     * Starts the keyboard "service" by registering the external interrupt.
     * setting the pin modes correctly and driving those needed to high.
     * The propably best place to call this method is in the setup routine.
     */
    void begin(int dataPin);

    /**
     * Returns true if there is a char to be read, false if not.
     */
    bool available();

    /**
     * Sends a reset command to the keyboard and re-initialises all the control
     * variables within the PS2Keybaord code.
     */
    void reset();

    /**
     * Returns the char last read from the keyboard. If the user has pressed two
     * keys between calls to this method, only the later one will be availble. Once
     * the char has been read, the buffer will be cleared.
     * If there is no char availble, 0 is returned.
     */
    byte read();

    /**
     * Returns the status of the <ctrl> key, the <alt> key, the <shift> key and the
     * caps_lock state. Note that shift and caps_lock are handled within the
     * Ps2Keyboard code (and the return value from read() is already modified), but
     * being able to read them here may be useful.
     * This routine is optional BUT MUST ONLY be read after available() has returned
     * true and BEFORE read() is called to retrieve the character. Reading it after
     * the call to read() will return unpredictable values.
     */
    byte read_extra();
    
 //   void kbd_set_lights(byte val);

};

#endif
