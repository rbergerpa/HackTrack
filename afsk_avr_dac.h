#ifdef AVR
#include "config.h"
#ifdef DAC_CS_PIN

#ifndef __AFSK_AVR_DAC_H__
#define __AFSK_AVR_DAC_H__
#include <stdint.h>

void afsk_setup();
void afsk_send(const uint8_t *buffer, int len);
void afsk_start();
int afsk_busy();
void afsk_isr();

#endif // __AFSK_AVR_DAC_H__
#endif // DAC_CS_PIN
#endif // AVR
