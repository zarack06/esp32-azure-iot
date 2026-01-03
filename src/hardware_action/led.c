#include "led.h"
#include "oled_text.h"
#include "esp_log.h"
#include "oled.h"
static const char *TAG = "HW_CTRL";

void hardware_led_set(int state)
{
    if (state == 1) {
        oled_put_string_5x7(7, 0, "LED Status: ON ");
        ESP_LOGI(TAG, "LED ON");
    } else {
        oled_put_string_5x7(7, 0, "LED Status: OFF");
        ESP_LOGI(TAG, "LED OFF");
    }
}
