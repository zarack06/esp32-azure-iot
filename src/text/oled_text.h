 
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif 

void oled_put_string_5x7(uint8_t page, uint8_t col, const char *str);
void oled_put_string_8x16(uint8_t page, uint8_t col, const char *str);

#ifdef __cplusplus
}
#endif