#include <stdio.h>  
#include "cJSON.h"
#include "esp_log.h"
#include "azure_iot/azure_iot.h"
#include "azure_iot/azure_tx_task.h"

#define TAG "AZURE_SERVICE"

static void (*led_cb)(int status, const char* place, int time) = NULL;

static void extract_rid(const char *topic, char *rid, size_t len)
{
    const char *p = strstr(topic, "$rid=");
    memset(rid, 0, len);
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

void azure_service_register_led_callback(void (*cb)(int status, const char* place, int time))
{
    led_cb = cb;
}

void azure_service_init(void)
{
    ESP_LOGI(TAG, "Azure service init");
}

/* ================= DIRECT METHOD ================= */


static void handle_direct_method(const char *topic, const char *payload)
{
    if (!strstr(topic, "control_led")) return;

    // 1. Parse chuỗi JSON
    cJSON *root = cJSON_Parse(payload);
    if (root == NULL) {
        ESP_LOGE(TAG, "JSON Parse Error");
        return;
    }

    // 2. Lấy dữ liệu từ các key 
    cJSON *status_obj = cJSON_GetObjectItem(root, "status");
    cJSON *place_obj  = cJSON_GetObjectItem(root, "place");
    cJSON *time_obj   = cJSON_GetObjectItem(root, "time");

    int status = (status_obj && cJSON_IsNumber(status_obj)) ? status_obj->valueint : 0;
    int time   = (time_obj && cJSON_IsNumber(time_obj))     ? time_obj->valueint   : 0;
    char *place = (place_obj && cJSON_IsString(place_obj))  ? place_obj->valuestring : "unknown";

    ESP_LOGI(TAG, "Azure Command -> Status: %d, Place: %s, Time: %d", status, place, time);

    // 3. Thực thi callback LED
    if (led_cb) {
        led_cb(status, place, time);
    }

    // --- Giữ nguyên phần phản hồi RID của bạn ---
    char rid[32] = {0};
    extract_rid(topic, rid, sizeof(rid));
    char resp_topic[64];
    snprintf(resp_topic, sizeof(resp_topic), "$iothub/methods/res/200/?$rid=%s", rid); 
    
    char resp_payload[128]; // Tăng kích thước buffer vì nội dung dài hơn
    snprintf(resp_payload, sizeof(resp_payload), 
             "{\"status\":\"success\", \"place\":\"%s\", \"time_set\":%d}", 
             place, time);
    
    azure_tx_send_topic(resp_topic, resp_payload);

    // 4. Giải phóng bộ nhớ cJSON (BẮT BUỘC)
    cJSON_Delete(root);
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
// test format from azure
// {
//     "status": 1,
//     "place": "be1",
//     "time": 5
// }