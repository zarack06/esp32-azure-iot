 
//===========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "config.h"
 
// Azure IoT Middleware headers
extern "C" {
    #include "az_iot_hub_client.h"
    #include "az_result.h"
    #include "az_span.h"
    #include "mqtt_client.h"
    /* ================= I2C ================= */
    #include "text/oled_text.h"
    #include "Oled/oled.h"
    #include "I2c/Display.h"
    #include "I2c/aht10.h" 
}
static const char *TAG = "AZURE_IOT_ESP32";
 

// biến toàn
az_iot_hub_client client;  
static esp_mqtt_client_handle_t mqtt_handle;
static bool is_mqtt_connected = false; 
 /* --- Biến điều khiển Wi-Fi --- */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

void setup_azure() {
    // Khởi tạo client cực kỳ đơn giản, không cần 10 tham số!
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    az_result res = az_iot_hub_client_init(&client, 
                                           az_span_create_from_str(const_cast<char*>(AZ_HOST_NAME) ), 
                                           az_span_create_from_str(const_cast<char*>(AZ_DEVICE_ID) ), 
                                           &options);
    if (az_result_succeeded(res)) {
        printf("Khởi tạo Azure Embedded C SDK thành công!\n");
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
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            // Kiểm tra xem có phải lệnh điều khiển "control_led" không
            if (strncmp(event->topic, "$iothub/methods/POST/control_led", strlen("$iothub/methods/POST/control_led")) == 0) {
                ESP_LOGW(TAG, "Đang xử lý lệnh control_led...");
                
                if (strstr(event->data, "\"status\":1")) {
                    ESP_LOGW(TAG, "=== LỆNH: BẬT LED ===");
                    // Thêm code bật GPIO của bạn ở đây
                } else {
                    ESP_LOGW(TAG, "=== LỆNH: TẮT LED ===");
                    // Thêm code tắt GPIO của bạn ở đây
                }

                // Phản hồi lại cho Azure (Bắt buộc để Azure không báo Timeout)
                // Trong thực tế cần parse $rid từ topic, nhưng tạm thời gửi response 200 đơn giản
                char *response_topic = "$iothub/methods/res/200/?$rid=1";
                esp_mqtt_client_publish(mqtt_handle, response_topic, "{}", 0, 1, 0);
            }
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
    // A. Khởi tạo Azure Client
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    az_result res = az_iot_hub_client_init(&client, 
                                           az_span_create_from_str(const_cast<char*>(AZ_HOST_NAME)), 
                                           az_span_create_from_str(const_cast<char*>(AZ_DEVICE_ID)), 
                                           &options);
    if (!az_result_succeeded(res)) {
        ESP_LOGE(TAG, "Khởi tạo Azure Client thất bại!");
        return;
    }

    // B. Lấy Client ID cho MQTT
    char mqtt_client_id[128];
    az_result resqt = az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id), NULL);

    // C. Cấu hình MQTT Client
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
// Hàm xử lý sự kiện Wi-Fi
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Đang thử kết nối lại Wi-Fi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Khởi tạo Wi-Fi
void wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi đã kết nối thành công!");
}

// Đồng bộ thời gian qua SNTP (Azure bắt buộc phải có thời gian đúng)
void time_sync_init(void) {
    ESP_LOGI(TAG, "Đang lấy thời gian từ Internet...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    time_t now = 0;
    struct tm timeinfo = {};
    while (timeinfo.tm_year < (2024 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    ESP_LOGI(TAG, "Thời gian hiện tại: %s", asctime(&timeinfo));
}
extern "C" void app_main(void) {
    // 1. Khởi tạo bộ nhớ Flash (NVS)
    char payload[128];
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Kết nối Wi-Fi & Đồng bộ giờ
    wifi_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    time_sync_init();
    setup_azure();
    // 3. Khởi động MQTT đến Azure IoT Hub
    start_azure_mqtt();
    //3.1 lấy thong so
    float temperature = 0.0f;
    float humidity = 0.0f;
    esp_err_t err = aht10_init();
    if (err != ESP_OK) {
       esp_log_level_set("AHT10", ESP_LOG_ERROR);
       ESP_LOGE("AHT10", "Failed to initialize AHT10 sensor: %d", err);
    } 
    // khởi tạo OLED
    // i2c_master_init();
    oled_init();
    oled_clear(); 
    int count = 0;
    // 4. Vòng lặp gửi dữ liệu
    while (1) {
        if (is_mqtt_connected) {
            esp_err_t err2 = aht10_read(&temperature, &humidity); 
            sprintf(payload, "NhietDo: %.2f", temperature);
            oled_clear();  
            oled_put_string_5x7(3, 0,payload);  
            sprintf(payload, "DoAm: %.2f", humidity);
            oled_put_string_5x7(4, 0,payload);  
            send_telemetry(temperature, humidity); // Gửi dữ liệu cảm biến
             ESP_LOGE(TAG, "Gửi dữ liệu rồi: temperature=%.2f, humidity=%.2f", temperature, humidity);
              ESP_LOGE(TAG, "goi roi doi 10'");
              count++;
              if (count > 1) 
              vTaskDelay(pdMS_TO_TICKS(100000));
               if (count > 3) 
              vTaskDelay(pdMS_TO_TICKS(500000));// gởi it thôi tốn tiền :D
        } else {
            ESP_LOGW(TAG, "Chưa kết nối MQTT, không thể gửi dữ liệu");
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Gửi mỗi 10 giây
    }
}