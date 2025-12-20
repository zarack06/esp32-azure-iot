#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void oled_init(void);
void oled_clear(void);
void oled_set_cursor(uint8_t page, uint8_t col); 
/* CORE DRAW FUNCTION */
void oled_draw_bitmap(uint8_t page,
                      uint8_t col,
                      const uint8_t *bitmap,
                      uint8_t width,
                      uint8_t height);
#ifdef __cplusplus
}
#endif
