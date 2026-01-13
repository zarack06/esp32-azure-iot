#include "oled_text.h"
#include "Oled/oled.h"
#include "fonts/font5x7.h" 
#include <string.h>
#ifdef __cplusplus
extern "C" {    
#endif

static void oled_put_char_5x7(uint8_t page, uint8_t col, char c)
{
    if (c < 32 || c > 126) c = '?';

    uint8_t buf[6];
    memcpy(buf, font5x7[c - 32], 5);
    buf[5] = 0x00;  // 1 cột trống bắt buộc

    oled_set_cursor(page, col);
    oled_data(buf, 6);
}
void oled_put_string_5x7(uint8_t page, uint8_t col, const char *str)
{
    if (page >= 8 || col >= 128) return;

    uint8_t x = col;

    while (*str && x <= (128 - 6)) {
        oled_put_char_5x7(page, x, *str++);
        x += 6;
    }

    // XÓA PHẦN DƯ CÒN LẠI CỦA DÒNG 
    if (x < 128) {
        uint8_t zero[128] = {0};
        oled_set_cursor(page, x);
        oled_data(zero, 128 - x);
    }
}

#ifdef __cplusplus
}
#endif