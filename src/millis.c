#include "millis.h"

volatile uint8_t _millisMutex = 0;
volatile uint16_t _milliseconds = 0;

inline uint16_t _millis()
{
    while (_millisMutex)
        ;
    return _milliseconds;
}

inline uint16_t millis()
{
    return _millis();
}

inline void delay(uint16_t ms)
{
    uint16_t startTime = _millis();
    while (_millis() - startTime < ms)
        ;
}