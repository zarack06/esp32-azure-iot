#include "azure_service.h"
#include "esp_log.h"
#include <string.h>

#define TAG "AZURE_SERVICE"

static void (*led_cb)(int) = NULL;

void azure_service_register_led_callback(void (*cb)(int))
{
    led_cb = cb;
}

void azure_service_init(void)
{
    ESP_LOGI(TAG, "Azure service init");
}

/* ================= DIRECT METHOD ================= */

static void handle_direct_method(
    const char *topic, const char *payload)
{
    if (!strstr(topic, "control_led")) return;

    int status = strstr(payload, "\"status\":1") ? 1 : 0;

    ESP_LOGI(TAG, "LED command: %d", status);

    if (led_cb) {
        led_cb(status);
    }

    /* TODO: parse real $rid */
}

/* ================= ENTRY POINT ================= */

void azure_service_on_mqtt_message(
    const char *topic, int topic_len,
    const char *payload, int payload_len)
{
    ESP_LOGI(TAG, "Azure message: %.*s",
             payload_len, payload);

    handle_direct_method(topic, payload);
}
