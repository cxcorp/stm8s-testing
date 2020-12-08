#include <stm8/stm8s.h>
#include "reflex.h"
#include "millis.h"

void reflex_init()
{
    // REFLEX_LED = PA3
    GPIO_Init(REFLEX_LED_PORT, REFLEX_LED_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    // REFLEX_SENSE = PD3/AIN4
    GPIO_Init(REFLEX_SENSE_PORT, REFLEX_SENSE_PIN, GPIO_MODE_IN_FL_NO_IT);

    // set adc to single conversion mode
    ADC1->CR1 &= ~ADC1_CR1_CONT;
    // set right-alignment for data registers
    ADC1->CR2 |= ADC1_CR2_ALIGN;
}

uint16_t reflex_poll()
{
    // Turn on reflex LED
    GPIO_WriteHigh(REFLEX_LED_PORT, REFLEX_LED_PIN);
    // reflex phototransistor seems to stabilize in ~600 micros
    delay(1);

    // Sample the ADC

    // specify the analog channel(s) we want to sample
    ADC1->CSR |= REFLEX_SENSE_AIN;
    // start conversion
    ADC1->CR1 |= ADC1_CR1_ADON;
    // wait for it to finish
    while ((ADC1->CSR | ADC1_CSR_EOC) == 0)
        ;

    // we're in right-alignment mode, LSB needs to be read first
    uint8_t adcLsb = ADC1->DRL;
    uint8_t adcMsb = ADC1->DRH;
    // clear the end-of-conversion bit
    ADC1->CSR &= ~ADC1_CSR_EOC;

    // Sample done, turn off reflex LED
    GPIO_WriteLow(REFLEX_LED_PORT, REFLEX_LED_PIN);
    // again wait for phototransistor to go down
    delay(1);

    uint16_t val = adcLsb | (((uint16_t)adcMsb) << 8);
    return val;
}