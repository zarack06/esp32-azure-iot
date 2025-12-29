#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "azure_iot/azure_iot.h"

#define TAG "AZURE_SERVICE"

static void (*led_cb)(int) = NULL;

static void extract_rid(const char *topic, char *rid, size_t len)
{
    const char *p = strstr(topic, "$rid=");
    if (!p || len == 0) return;

    snprintf(rid, len, "%s", p + 5);
}

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

    char rid[16] = {0};
    extract_rid(topic, rid, sizeof(rid));

    char resp_topic[64];
    snprintf(resp_topic, sizeof(resp_topic),
             "$iothub/methods/res/200/?$rid=%s", rid);

    azure_iot_publish(resp_topic, "{}");
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
