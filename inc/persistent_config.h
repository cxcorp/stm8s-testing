#ifndef _PERSIST_CONFIG_H
#define _PERSIST_CONFIG_H

#include <stm8s.h>

uint16_t config_read_reflex_threshold();
void config_write_reflex_threshold(uint16_t value);

#endif