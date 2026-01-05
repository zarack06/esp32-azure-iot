#include "esp_log.h"

extern "C" {
#include "oled.h"
#include "aht10.h"
#include "azure_service.h"
#include "wifi_manager.h"
#include "system_init.h"
#include "azure_iot.h"
#include "hardware_action/led.h"
#include "sensor_task.h"
#include "telemetry/telemetry_task.h"
#include "azure_tx_task.h"
#include "helper_time.h"
#include "i2c_manager.h"
}

static const char *TAG = "APP_MAIN";

extern "C" void app_main(void)
{
    // Allow system & logs to stabilize after boot
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "===== System Boot Start =====");

    /* ================= Hardware Init ================= */
    system_init();
    gpios_all_init();
    hardware_init_timers();

    /* ================= I2C & Peripherals ================= */
    i2c_manager_init();
    vTaskDelay(pdMS_TO_TICKS(100));

    if (aht10_init() != ESP_OK) {
        ESP_LOGW(TAG, "AHT10 not ready, sensor task will retry");
    }

    oled_init();
    oled_clear();

    /* ================= Network & Time ================= */
    wifi_init();
    time_sync_init();

    /* ================= Azure IoT ================= */
    azure_service_register_led_callback(hardware_led_set);
    azure_service_init();
    azure_iot_init();
    azure_iot_start();

    /* ================= Tasks ================= */
    azure_tx_task_start();

    QueueHandle_t sensor_q = sensor_task_start();
    telemetry_task_start(sensor_q);

    xTaskCreate(
        oled_update_err,
        "oled_err_task",
        4096,
        NULL,
        3,
        NULL
    );

    ESP_LOGI(TAG, "===== System Boot Completed =====");
}
