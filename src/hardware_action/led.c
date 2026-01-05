#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include <string.h>
#include "oled_text.h"
#include "esp_log.h"
#include "oled.h"
#include "config.h" 
#include "driver/gpio.h"


static const char *TAG = "HW_CTRL"; 
static TimerHandle_t led2_timer = NULL;
static TimerHandle_t led15_timer = NULL;
 
static void gpio_timer_cb(TimerHandle_t xTimer)
{
    int gpio = (int)pvTimerGetTimerID(xTimer);
    gpio_set_level(gpio, 0);

    ESP_LOGI(TAG, "GPIO %d auto OFF by timer", gpio);
}

void hardware_init_timers(void)
{
    if (!led2_timer) {
        led2_timer = xTimerCreate(
            "T_LED2",
            pdMS_TO_TICKS(1000),
            pdFALSE,
            (void*)LED_BUILTIN,
            gpio_timer_cb
        );
    }

    if (!led15_timer) {
        led15_timer = xTimerCreate(
            "T_LED15",
            pdMS_TO_TICKS(1000),
            pdFALSE,
            (void*)SWITCH_GPIO,
            gpio_timer_cb
        );
    }
}
static void control_gpio_with_timer(TimerHandle_t timer, int gpio, int time_sec)
{
    if (!timer) {
        ESP_LOGE(TAG, "Timer not initialized for GPIO %d", gpio);
        return;
    }

    gpio_set_level(gpio, 1);

    if (time_sec > 0) {
        xTimerStop(timer, 0);  // đảm bảo sạch
        xTimerChangePeriod(
            timer,
            pdMS_TO_TICKS(time_sec * 1000),
            0
        );
    }
} 
void led2_on_nonblock(int time_sec) {
    control_gpio_with_timer(led2_timer, LED_BUILTIN, time_sec);
}

void led15_on_nonblock(int time_sec) {
    control_gpio_with_timer(led15_timer, SWITCH_GPIO, time_sec);
} 
typedef struct {
    const char* place;
    void (*callback)(int);
} PlaceCallback;

void hardware_led_set(int status, const char* place, int time)
{
        ESP_LOGI(TAG, "LED Command - Status: %d, Place: %s, Time: %d", status, place, time);
    if (status == 1) {
        oled_put_string_5x7(7, 0, "LED Status: ON ");
        led2_set(place, time);
        // ESP_LOGI(TAG, "LED ON");
    } else {
        led2_set(place, time);
        oled_put_string_5x7(7, 0, "LED Status: OFF");
        // ESP_LOGI(TAG, "LED OFF");
    }
};

void gpios_all_init(void)
{
    gpio_config_t io_conf = {
        // Sử dụng toán tử OR (|) để cấu hình nhiều chân cùng lúc
        .pin_bit_mask = (1ULL << LED_BUILTIN) | (1ULL << SWITCH_GPIO), 
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Trạng thái mặc định
    gpio_set_level(LED_BUILTIN, 0);
    gpio_set_level(SWITCH_GPIO, 0);
} 

void led2_off(int time)
{
    gpio_set_level(LED_BUILTIN, 0); 
}
void led15_off(int time)
{
    gpio_set_level(SWITCH_GPIO, 0); 
}
void led2_toggle(void)
{
   if (gpio_get_level(LED_BUILTIN)) {
        gpio_set_level(LED_BUILTIN, 0);
    } else {
        gpio_set_level(LED_BUILTIN, 1);
    }
}

static const PlaceCallback place_callbacks[] = {
    {"Be1", led2_on_nonblock},
    {"Be2", led15_on_nonblock},
    {"Be1_off", led2_off},
    {"Be2_off", led15_off},
};

void led2_set(const char* place, int time)
{
    for (int i = 0; i < sizeof(place_callbacks) / sizeof(PlaceCallback); i++) {
        if (strcmp(place, place_callbacks[i].place) == 0) {
            if (place_callbacks[i].callback) {
                place_callbacks[i].callback(time);
            }
            return;
        }
    }    
}
bool led2_get_state(void)
{
    return gpio_get_level(LED_BUILTIN);
}
