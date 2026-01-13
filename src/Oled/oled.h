#include <stdint.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

void oled_init(void);
void oled_clear(void);
void oled_data(uint8_t *data, size_t len);
void oled_set_cursor(uint8_t page, uint8_t col); 
void oled_clear_page(uint8_t page);
/* CORE DRAW FUNCTION */
void oled_draw_bitmap(uint8_t page,
                      uint8_t col,
                      const uint8_t *bitmap,
                      uint8_t width,
                      uint8_t height);
#ifdef __cplusplus
}
#endif
