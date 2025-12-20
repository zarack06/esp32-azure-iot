#include "oled_text.h"
#include "../Oled/oled.h"
#include "../fonts/font5x7.h"
#include "../fonts/font8x16.h"
#ifdef __cplusplus
extern "C" {    
#endif

void oled_put_char_5x7(uint8_t page, uint8_t col, char c)
{
    if (c < 0x20 || c > 0x7E) return;

    oled_draw_bitmap(
        page,
        col,
        font5x7[c - 0x20],
        FONT5X7_WIDTH,
        FONT5X7_HEIGHT
    );
}

void oled_put_char_8x16(uint8_t page, uint8_t col, char c)
{
    if (c < 0x20 || c > 0x7E) return;

    oled_draw_bitmap(
        page,
        col,
        font8x16[c - 0x20],
        FONT8X16_WIDTH,
        FONT8X16_HEIGHT
    );
}
void oled_put_string_5x7(uint8_t page, uint8_t col, const char *str)
{
    while (*str) {
        oled_put_char_5x7(page, col, *str++);
        col += FONT5X7_WIDTH + 1; // 1 cột spacing
    }
}
void oled_put_string_8x16(uint8_t page, uint8_t col, const char *str)
{
    while (*str) {
        oled_put_char_8x16(page, col, *str++);
        col += FONT8X16_WIDTH + 1; // 1 cột spacing
    }
}
#ifdef __cplusplus
}
#endif