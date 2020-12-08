#ifndef _SERIAL_H
#define _SERIAL_H

#include <stm8/stm8s.h>

void Serial_begin();
void Serial_print(const char *str);
void Serial_println(const char *str);
char Serial_readchar();
uint16_t Serial_nreadline(char *buffer, uint16_t buffer_size);

#endif