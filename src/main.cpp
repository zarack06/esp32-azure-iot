 
//===========================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "esp_log.h" 
#include "config.h"
 
// Azure IoT Middleware headers
extern "C" {
    #include "az_iot_hub_client.h"
    #include "az_result.h"
    #include "az_span.h"
    #include "mqtt_client.h"
    /* ================= I2C ================= */
    #include "oled_text.h"
    #include "oled.h"
    #include "Display.h"
    #include "aht10.h"
    #include "azure_service.h"
    #include "wifi_manager.h"
    #include "system_init.h"
}
static const char *TAG = "AZURE_IOT_ESP32";
  
extern "C" void app_main(void) {
    char payload[128];
    // 1. Khởi tạo bộ nhớ Flash (NVS)
    system_init(); 
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
        if (is_mqtt_connected_func()) {
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
              vTaskDelay(pdMS_TO_TICKS(10000)); 
        } else {
            ESP_LOGW(TAG, "Chưa kết nối MQTT, không thể gửi dữ liệu");
        } 
    }
}