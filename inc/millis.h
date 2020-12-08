#ifndef _MILLIS_H
#define _MILLIS_H

#include <stdint.h>

extern volatile uint8_t _millisMutex;
extern volatile uint16_t _milliseconds;

inline uint16_t millis();

inline void delay(uint16_t ms);

#endif