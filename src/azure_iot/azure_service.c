#include <stdio.h>  
#include "esp_log.h"
#include "azure_iot/azure_iot.h"
#include "azure_iot/azure_tx_task.h"

#define TAG "AZURE_SERVICE"

static void (*led_cb)(int) = NULL;

static void extract_rid(const char *topic, char *rid, size_t len)
{
    const char *p = strstr(topic, "$rid=");
    if (!p) return;

    p += 5; // Nhảy qua chuỗi "$rid=" 

    int i = 0;
    //   Gặp ký tự không phải là chữ/số (như dấu '{' của payload) 
    while (i < len) {
        // Nếu gặp ký tự đặc biệt (không phải chữ số/chữ cái) thì dừng
        if (p[i] == '{' || p[i] == '&' || p[i] == ' ' || p[i] == '\r' || p[i] == '\n') {
            break;
        }
        rid[i] = p[i];
        i++;
    } 
     ESP_LOGI(TAG, "rid %s", rid);
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
     char resp_payload[32];
    snprintf(resp_payload, sizeof(resp_payload),
         "{\"result\":\"da chuyen led\"}");  // string send to azure after action 
    azure_tx_send_topic(resp_topic, resp_payload);  // đợi thay thế
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
