#pragma once
#include <stdint.h>  
#include <stdbool.h>

#define FONT5X7_WIDTH   5
#define FONT5X7_HEIGHT  7
#define FONT5X7_FIRST   32   // ' '
#define FONT5X7_LAST    126  // '~'

extern const uint8_t font5x7[][FONT5X7_WIDTH];

void oled_put_string(uint8_t page, uint8_t col, const char *str);
