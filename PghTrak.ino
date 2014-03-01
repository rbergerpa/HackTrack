/* trackuino copyright (C) 2010  EA5HAV Javi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Trackuino custom libs
#include "config.h"
#include "afsk_avr.h"
#include "afsk_avr_dac.h"
#include "afsk_pic32.h"
#include "aprs.h"
#include "buzzer.h"
#include "gps.h"
#include "pin.h"
#include "power.h"
#include "sensors_avr.h"
#include "sensors_pic32.h"

// upport for MCP4921 DAC (RWB)
#include <SPI.h>
#include <Tone4921.h>

// Arduino/AVR libs
#include <Wire.h>
#include <WProgram.h>

// Module constants
static const uint32_t VALID_POS_TIMEOUT = 2000;  // ms

// Module variables
static int32_t next_aprs = 0;



void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pin_write(LED_PIN, LOW);

  Serial.begin(GPS_BAUDRATE);
#ifdef DEBUG_RESET
  Serial.println("RESET");
#endif

// RWB
//   buzzer_setup();

  afsk_setup();
  gps_setup();

  sensors_setup();

#ifdef DEBUG_SENS
  Serial.print("Ti=");
  Serial.print(sensors_int_lm60());
  Serial.print(", Te=");
  Serial.print(sensors_ext_lm60());
  Serial.print(", Vin=");
  Serial.println(sensors_vin());
#endif

  // Do not start until we get a valid time reference
  // for slotted transmissions.
  if (APRS_SLOT >= 0) {
    do {
      while (! Serial.available())
        power_save();
    } while (! gps_decode(Serial.read()));
    
    next_aprs = millis() + 1000 *
      (APRS_PERIOD - (gps_seconds + APRS_PERIOD - APRS_SLOT) % APRS_PERIOD);
  }
  else {
    next_aprs = millis();
  }  
  // TODO: beep while we get a fix, maybe indicating the number of
  // visible satellites by a series of short beeps?
}

void get_pos()
{
  // Get a valid position from the GPS
  int valid_pos = 0;
  uint32_t timeout = millis();
  do {
    if (Serial.available())
      valid_pos = gps_decode(Serial.read());
  } while ( (millis() - timeout < VALID_POS_TIMEOUT) && ! valid_pos) ;

// rwb
//  if (valid_pos) {
//    if (gps_altitude > BUZZER_ALTITUDE) {
//     buzzer_off();   // In space, no one can hear you buzz
//    } else {
//      buzzer_on();
//    }
//  }
}

void loop()
{
 // Time for another APRS frame
  if ((int32_t) (millis() - next_aprs) >= 0) {
    get_pos();
    aprs_send();
    next_aprs += APRS_PERIOD * 1000L;
 
 while (afsk_busy()) ;

#ifdef DEBUG_MODEM
    // Show modem ISR stats from the previous transmission
    afsk_debug();
#endif
  }

//  power_save(); // Incoming GPS data or interrupts will wake us up
}
