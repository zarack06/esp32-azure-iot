// #include <stdio.h>
// #include <string.h>  
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h" 
// #include "driver/i2c.h"
// #include "esp_log.h" 
// /* ================= I2C ================= */
// #include "text/oled_text.h"
// #include "Oled/oled.h"
// #include "I2c/Display.h"
// #include "I2c/aht10.h" 
// #include "Wifi/wifi_manager.h"
// #include "azure/Json/azure_iot.h"
// /* ================= main ================= */

// extern "C" void app_main()
// {   vTaskDelay(pdMS_TO_TICKS(100));  // ⭐ RẤT QUAN TRỌNG
//     wifi_init_sta();

//     // wait up to 10s for WiFi
//     if (!wifi_manager_is_connected()) {
//         wifi_wait_connected(10000);
//     }

//     char count_str[16];
      
//     float temperature = 0.0f;
//     float humidity = 0.0f;
//     esp_err_t err = aht10_init();
//     if (err != ESP_OK) {
//        esp_log_level_set("AHT10", ESP_LOG_ERROR);
//        ESP_LOGE("AHT10", "Failed to initialize AHT10 sensor: %d", err);
//     }
//     i2c_master_init(); 
//     oled_init();
//     oled_clear();
//     int count = 0; 

//     // If WiFi is already connected, start Azure IoT client
//     if (wifi_manager_is_connected()) {
//         ESP_LOGI("WIFI", "Connected to WiFi successfully");
//         if (!azure_iot_is_connected()) {
//             esp_err_t rc = azure_iot_connect();
//             if (rc != ESP_OK) {
//                 esp_log_level_set("AZURE_IOT", ESP_LOG_ERROR);
//                 ESP_LOGE("AZURE_IOT", "Failed to start Azure IoT client: %d", rc);
//             }
//         }
//     } else {
//         ESP_LOGE("WIFI", "Failed to connect to WiFi");
//     }

//     while (1) {
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         oled_put_string_5x7(0, 0,"986"); 
//         oled_put_string_8x16(2, 0, "A");
//         vTaskDelay(pdMS_TO_TICKS(5000));
//         aht10_read(&temperature, &humidity);
//         // xTaskCreate(aht10_read(&temperature, &humidity), "aht10_read_task", 2048, NULL, 5, NULL);
//         count++;
//         oled_clear();
//         snprintf(count_str, sizeof(count_str), "tem=%.1f,hum=%.1f", temperature, humidity);
//         oled_put_string_5x7(0, 0, count_str); 
//         oled_put_string_8x16(2, 0, "BCDEFGHIJKLMN");
//         vTaskDelay(pdMS_TO_TICKS(5000));

//         if (wifi_manager_is_connected()) {
//             if (!azure_iot_is_connected()) {
//                 esp_err_t rc = azure_iot_connect();
//                 if (rc == ESP_OK) {
//                     ESP_LOGI("AZURE_IOT", "Azure IoT client started");
//                 } else {
//                     ESP_LOGE("AZURE_IOT", "azure_iot_connect failed: %d", rc);
//                 }
//             } else {
//                 esp_log_level_set("AZURE_IOT", ESP_LOG_INFO);
//                 ESP_LOGI("AZURE_IOT", "Connected to Azure IoT Hub - sending telemetry");
//                 azure_iot_send_telemetry(temperature, humidity);
//             }
//         } else {
//             ESP_LOGW("WIFI", "WiFi disconnected");
//         }
//     }

// }
//==========================

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "cJSON.h" // Thư viện xử lý JSON
// Các header của Azure (tùy thuộc vào phiên bản SDK bạn tải)
#include "azure_iot_hub_client.h"
#include "cJSON.h"

// Hàm gửi dữ liệu
void send_sensor_data(AzureIoTHubClient_t *xAzureIoTHubClient) {
    char payload[256];
    
    // 1. Tạo JSON bằng cJSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "nhiet_do", 30.5);
    cJSON_AddNumberToObject(root, "do_am", 80);
    
    // 2. Chuyển sang chuỗi (Unformatted để tiết kiệm dung lượng)
    char *json_str = cJSON_PrintUnformatted(root);
    strncpy(payload, json_str, sizeof(payload));
    
    // 3. Gửi lên Azure
    AzureIoTHubClient_SendTelemetry(xAzureIoTHubClient, 
                                     (uint8_t *)payload, 
                                     strlen(payload), 
                                     NULL, 
                                     eAzureIoTHubMessagePriorityNormal, 
                                     NULL);

    // 4. Dọn dẹp bộ nhớ
    cJSON_Delete(root);
    free(json_str);
}
extern "C" 
void app_main(void) {
    // 1. Khởi tạo Wi-Fi (Hàm này có sẵn trong ví dụ của ESP-IDF)
    // 2. Khởi tạo Azure IoT Hub Client
    
    while (1) {
        char *payload = create_json_data();
        
        ESP_LOGI(TAG, "Gửi dữ liệu: %s", payload);

        // Giả sử xAzureIoTHubClient đã được setup thành công
        /* AzureIoTHubClient_SendTelemetry(&xAzureIoTHubClient, 
                                        (uint8_t *)payload, 
                                        strlen(payload), 
                                        NULL, 
                                        eAzureIoTHubMessageQoS1, 
                                        NULL);
        */

        free(payload); // Giải phóng bộ nhớ sau khi gửi
        vTaskDelay(pdMS_TO_TICKS(10000)); // Đợi 10 giây gửi 1 lần
    }
} 