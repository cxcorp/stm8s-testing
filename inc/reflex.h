#ifndef _RELFEX_H
#define _REFLEX_H

#include <stdint.h>

#define REFLEX_LED_PORT GPIOA
#define REFLEX_LED_PIN GPIO_PIN_3

#define REFLEX_SENSE_PORT GPIOD
#define REFLEX_SENSE_PIN GPIO_PIN_3
#define REFLEX_SENSE_AIN 4

void reflex_init();
/** Poll the reflex sensor's ADC */
uint16_t reflex_poll();

#endif