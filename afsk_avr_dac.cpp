#include <Tone4921.h>

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

// RWB todo make cnditional on AVR_DAC4921
#ifdef AVR
#include "config.h"
#ifdef DAC_CS_PIN

#include "afsk_avr.h"
#include "afsk_pic32.h"
#include "pin.h"
#include "radio_hx1.h"
#include <Arduino.h>
#include <stdint.h>
#include <Tone4921.h>

static const uint32_t SAMPLE_RATE = 144000;
Tone4921 toneGen(DAC_CS_PIN, SAMPLE_RATE);

// Module consts
static const uint16_t BAUD_RATE       = 1200;
static const uint8_t SAMPLES_PER_BAUD = (SAMPLE_RATE / BAUD_RATE);


// Module globals
volatile static unsigned char current_byte;
volatile static unsigned char current_sample_in_baud;    // 1 bit = SAMPLES_PER_BAUD samples
volatile static bool go = false;                         // Modem is on
volatile static unsigned int packet_pos;                 // Next bit to be sent out
volatile static int sendHighTone;

#ifdef DEBUG_MODEM
volatile static int overruns = 0;
volatile static uint32_t isr_calls = 0;
volatile static uint32_t avg_isr_time = 0;
volatile static uint16_t fast_isr_time = 65535;
volatile static uint16_t slow_isr_time = 0;
volatile static unsigned int slow_packet_pos;
volatile static unsigned char slow_sample_in_baud;
#endif

// The radio (class defined in config.h)
static RadioHx1 radio;

volatile static unsigned int afsk_packet_size = 0;
volatile static const uint8_t *afsk_packet;

void afsk_callback();

// Exported functions

void afsk_setup()
{
  // Start radio
  radio.setup();
  toneGen.setCallback(&afsk_callback);
  toneGen.setGain(DAC_GAIN);
  toneGen.start();
}

void afsk_send(const uint8_t *buffer, int len)
{
  afsk_packet_size = len;
  afsk_packet = buffer;
  packet_pos = 0;
}

int afsk_busy()
{
  return go;
}

void afsk_start() {
  pin_write(LED_PIN, HIGH);
  toneGen.setFrequency(1200);
  sendHighTone = false;
  toneGen.setEnabled(true);
  
  // Key the radio
  radio.ptt_on();
  go = true;
}

void afsk_stop() {
  go = false;
  radio.ptt_off();    // Release PTT
  pin_write(LED_PIN, LOW);
  toneGen.setEnabled(false);
}

// This is called at SAMPLE_RATES as a callback to the Tone4921 library.
void afsk_callback()
{
  if (go) {
    // If done sending packet
    if (packet_pos == afsk_packet_size) {
      afsk_stop();
      return;
    }
      
    // RWB TODO set interrupt rate to baud
    // If sent SAMPLES_PER_BAUD already, go to the next bit
    if (current_sample_in_baud == 0) {    // Load up next bit
      if ((packet_pos & 7) == 0)          // Load up next byte
        current_byte = afsk_packet[packet_pos >> 3];
      else
        current_byte = current_byte / 2;  // ">>1" forces int conversion
      if ((current_byte & 1) == 0) {
        // Toggle tone (1200 <> 2200)
	sendHighTone = !sendHighTone;
	toneGen.setFrequency(sendHighTone ? 2200 : 1200);
      }
    }
 
    if(++current_sample_in_baud == SAMPLES_PER_BAUD) {
      current_sample_in_baud = 0;
      packet_pos++;
    }
  }
}

#endif // DAC_CS_PIN
#endif // AVR

 

