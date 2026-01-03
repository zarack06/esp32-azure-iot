#include "azure_iot.h"
#include "azure_service.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "az_iot_hub_client.h"
#include "config.h"
#include "azure_iot_internal.h"
#include "helper_time.h"
#define TAG "AZURE_IOT"

static az_iot_hub_client client;
static esp_mqtt_client_handle_t mqtt_handle;
static bool is_mqtt_connected = false;

/* ================= MQTT EVENT ================= */

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        is_mqtt_connected = true;
        esp_mqtt_client_subscribe(mqtt_handle,
            "$iothub/methods/POST/#", 1);
        break;

    case MQTT_EVENT_DATA:
        azure_service_on_mqtt_message(
            event->topic, event->topic_len,
            event->data, event->data_len);
        break;

    case MQTT_EVENT_DISCONNECTED:
        is_mqtt_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        set_last_error("MQTT Lost!");
        break;

    default:
        break;
    }
}

/* ================= INIT ================= */

void azure_iot_init(void)
{
    az_iot_hub_client_options options =
        az_iot_hub_client_options_default();

    az_result res = az_iot_hub_client_init(
        &client,
        az_span_create_from_str(AZ_HOST_NAME),
        az_span_create_from_str(AZ_DEVICE_ID),
        &options);

    if (az_result_failed(res)) {
        ESP_LOGE(TAG, "Azu client init failed");
        set_last_error("Azu client failed");
    }
}

void azure_iot_start(void)
{
    char mqtt_client_id[128];
    az_iot_hub_client_get_client_id(
        &client, mqtt_client_id, sizeof(mqtt_client_id), NULL);

    esp_mqtt_client_config_t cfg = {};
    char uri[128];
    snprintf(uri, sizeof(uri), "mqtts://%s", AZ_HOST_NAME);

    cfg.broker.address.uri = uri;
    cfg.credentials.client_id = mqtt_client_id;
    cfg.credentials.username = AZ_USERNAME;
    cfg.credentials.authentication.password = AZ_PASSWORD;
    cfg.broker.verification.certificate = AZURE_CA_PEM;
    cfg.broker.verification.skip_cert_common_name_check = true;

    mqtt_handle = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(
        mqtt_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);  // đăng ký nhận lệnh điều khiển
    esp_mqtt_client_start(mqtt_handle);
}
 
void azure_iot_publish_payload(const char *payload) {
    static az_result last_err = AZ_OK;
    char topic[256];
    if (!is_mqtt_connected || payload == NULL) return;
    // "$.ct=application%2Fjson&$.ce=utf-8" là từ khóa để Azure tự giải mã Base64
    const char* property_bag = "$.ct=application%2Fjson&$.ce=utf-8";
    // Tự lấy topic nội bộ 
    az_result res = az_iot_hub_client_telemetry_get_publish_topic(
        &client, 
        NULL, 
        topic, 
        sizeof(topic), 
        NULL);

    if (!az_result_succeeded(res)) {
        if (last_err != res) {
            ESP_LOGW(TAG,"Telemetry topic error changed: 0x%08x",res);
            last_err = res;
        }
        return;
    }
    last_err = AZ_OK; 
    // 2. Tính toán độ dài hiện tại của topic
    size_t current_len = strlen(topic);

    // 3. Sử dụng snprintf để nối chuỗi an toàn
    // Ta ghi đè từ vị trí topic + current_len
    int written = snprintf(topic + current_len, 
                            sizeof(topic) - current_len, 
                            "%s", 
                            property_bag);

    // 4. Kiểm tra xem việc nối chuỗi có bị cắt cụt (truncated) không
    if (written >= (sizeof(topic) - current_len)) {
        ESP_LOGE("AZURE", "Topic buffer quá nhỏ để chứa Metadata!");
        set_last_error("buffer Full");
        return; 
    }
    // 5. Publish
    esp_mqtt_client_publish(mqtt_handle, topic, payload, 0, 1, 0);  
}
 
void azure_iot_publish_topic(const char *topic, const char *payload)
{
    if (!is_mqtt_connected) return;
    
    esp_mqtt_client_publish(
        mqtt_handle,
        topic,
        payload,
        0,
        1,
        0);
}

bool azure_iot_is_connected(void)
{
    return is_mqtt_connected;
} 


 