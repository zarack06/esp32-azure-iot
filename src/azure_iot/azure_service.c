#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "esp_system.h" 
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "config.h"
#define TAG "AZURE_SERVICE" 
// Azure IoT Middleware headers
#include "az_iot_hub_client.h"
#include "az_result.h"
#include "az_span.h"
#include "mqtt_client.h" 
#include "azure_service.h" 

// biến toàn

az_iot_hub_client client;  
static esp_mqtt_client_handle_t mqtt_handle;
static bool is_mqtt_connected = false;  
// Thêm một định nghĩa function pointer để báo lệnh về main
static void (*on_led_command_callback)(int status) = NULL;

void setup_azure() { 
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    az_result res = az_iot_hub_client_init(
        &client,
        az_span_create_from_str(AZ_HOST_NAME),
        az_span_create_from_str(AZ_DEVICE_ID),
        &options
    );
    if (!az_result_succeeded(res)) {
        ESP_LOGE(TAG, "Azure client init failed");
    }
}
  
void azure_service_register_led_callback(void (*cb)(int)) {
    on_led_command_callback = cb;
}

// Hàm bổ trợ để parse Direct Method (Giúp handler gọn hơn)
static void handle_direct_method(esp_mqtt_event_handle_t event) {
    ESP_LOGI(TAG, "Direct Method nhận được trên Topic: %.*s", event->topic_len, event->topic);

    // 1. Phân loại lệnh (Ví dụ: control_led)
    if (strstr(event->topic, "control_led")) {
        int status = strstr(event->data, "\"status\":1") ? 1 : 0;
        
        ESP_LOGW(TAG, "Lệnh điều khiển LED: %s", status ? "ON" : "OFF");
        
        // Gọi callback để thực hiện lệnh thực tế ở bên ngoài
        if (on_led_command_callback) {
            on_led_command_callback(status);
        }

        // 2. Gửi phản hồi (Response) cho Azure
        // * Sau này Cần parse $rid từ topic gốc để trả lời đúng phiên làm việc
        char *response_topic = "$iothub/methods/res/200/?$rid=1"; 
        esp_mqtt_client_publish(mqtt_handle, response_topic, "{}", 0, 1, 0);
    }
}
/* --- 1. Xử lý sự kiện MQTT --- */ 
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "ĐÃ KẾT NỐI MQTT ĐẾN AZURE IOT HUB!");
            is_mqtt_connected = true; 
            esp_mqtt_client_subscribe(mqtt_handle, "$iothub/methods/POST/#", 1); // nhận Direct Method từ Azure
            break;

        case MQTT_EVENT_DATA:
            // 1. In toàn bộ dữ liệu nhận được ra để kiểm tra (Debug)
            ESP_LOGI(TAG, "Dữ liệu nhận được: %.*s", event->data_len, event->data);
            ESP_LOGI(TAG, "Từ Topic: %.*s", event->topic_len, event->topic);
            //  LOGIC XỬ LÝ DỮ LIỆU NHẬN ĐC
            ESP_LOGI(TAG, "NHẬN DỮ LIỆU TỪ AZURE!");
            handle_direct_method(event); // Xử lý dữ liệu tại hàm riêng
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Mất kết nối MQTT!");
            is_mqtt_connected = false;
            break;
        default: break;
    }
}
/* --- 2. Khởi tạo Azure & MQTT --- */ 
void start_azure_mqtt() { 

    // A. Lấy Client ID cho MQTT
    char mqtt_client_id[128];
    az_result resqt = az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id), NULL);

    // B. Cấu hình MQTT Client
    esp_mqtt_client_config_t mqtt_cfg = {};
    
    // 1. Địa chỉ Server
    char mqtt_uri[256];
    snprintf(mqtt_uri, sizeof(mqtt_uri), "mqtts://%s", AZ_HOST_NAME);
    mqtt_cfg.broker.address.uri = mqtt_uri;
    mqtt_cfg.broker.address.port = 8883;

    // 2. Thông tin đăng nhập
    mqtt_cfg.credentials.client_id = mqtt_client_id;
    mqtt_cfg.credentials.username = AZ_USERNAME;
    mqtt_cfg.credentials.authentication.password = AZ_PASSWORD; 
    // 3. Cấu hình bảo mật (Quan trọng nhất để sửa lỗi 0x8017) 
    mqtt_cfg.broker.verification.certificate = AZURE_CA_PEM;
    mqtt_cfg.broker.verification.certificate_len = strlen(AZURE_CA_PEM) + 1; // +1 để tính cả ký tự kết thúc chuỗi
    mqtt_cfg.broker.verification.skip_cert_common_name_check = true;
    
    // 4. Các thông số phụ để giữ kết nối ổn định
    mqtt_cfg.session.keepalive = 60;
    mqtt_cfg.network.reconnect_timeout_ms = 5000;

    mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_handle, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_handle);
} 

void send_telemetry(float temp, float hum) {
    if (!is_mqtt_connected) return;

    char telemetry_topic[128];
    char payload[128];

    // 1. Khởi tạo thuộc tính tin nhắn (Message Properties)
    az_iot_message_properties props;
    uint8_t props_buf[64];
    az_result res = az_iot_message_properties_init(&props, az_span_create(props_buf, sizeof(props_buf)), 0);
    
    // 2. Thêm định dạng JSON và Encoding UTF-8
    // Azure IoT Explorer cần 2 cái này để tự động parse dữ liệu
    az_iot_message_properties_append(&props, AZ_SPAN_FROM_STR("$.ct"), AZ_SPAN_FROM_STR("application/json"));
    az_iot_message_properties_append(&props, AZ_SPAN_FROM_STR("$.ce"), AZ_SPAN_FROM_STR("utf-8"));

    // 3. Tạo topic có chứa các thuộc tính trên
    // Thay vì truyền NULL, ta truyền &props vào tham số thứ 2
    az_iot_hub_client_telemetry_get_publish_topic(&client, &props, telemetry_topic, sizeof(telemetry_topic), NULL);
    
    // 4. Tạo nội dung JSON
    sprintf(payload, "{\"temperature\": %.2f, \"humidity\": %.2f}", temp, hum);

    // 5. Gửi lên MQTT
    int msg_id = esp_mqtt_client_publish(mqtt_handle, telemetry_topic, payload, 0, 1, 0);
    
    if (msg_id != -1) {
        ESP_LOGI(TAG, "Đã gửi Telemetry (ID: %d) với Topic: %s", msg_id, telemetry_topic);
    } else {
        ESP_LOGE(TAG, "Gửi Telemetry thất bại!");
    }
}  
bool is_mqtt_connected_func(void) {
    return is_mqtt_connected;
}