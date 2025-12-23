 
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
// Hàm callback xử lý lệnh LED (được gọi từ Azure Task) 
void my_hardware_control(int status) {
    if (status == 1) {
        // Thực hiện bật LED 
        oled_put_string_5x7(6, 0, "LED Status: ON ");
         ESP_LOGI("AzureContral", "LED ON");
    } else {
        // Thực hiện tắt LED 
        oled_put_string_5x7(6, 0, "LED Status: OFF");
        ESP_LOGI("AzureContral", "LED OFF");
    }
}

//  Đọc Sensor -> Update OLED -> Gửi Azure
void sensor_telemetry_task(void *pvParameters) {
    float temp, hum;
    char payload[32];
    
    while (1) {
        if (is_mqtt_connected_func()) {
            if (aht10_read(&temp, &hum) == ESP_OK) {
                // 1. Cập nhật OLED
                sprintf(payload, "Temp: %.2f C", temp);
                oled_put_string_5x7(3, 0, payload);
                sprintf(payload, "Humid: %.2f %%", hum);
                oled_put_string_5x7(4, 0, payload);
                
                // 2. Gửi Telemetry
                send_telemetry(temp, hum);
                ESP_LOGI("APP", "Data sent to Azure");
            }
        }
        
        // Gửi mỗi 20 giây  
        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}

void azure_service_register_led_callback(void (*cb)(int status));
extern "C" void app_main(void) {

    system_init();
    wifi_init();      
    time_sync_init(); 
    
    // 2. Cấu hình Azure & Đăng ký callback
    setup_azure();
    azure_service_register_led_callback(my_hardware_control);
    start_azure_mqtt();

    // 3. Phần cứng
    aht10_init();
    oled_init();
    oled_clear();
    oled_put_string_5x7(0, 0, "Azure IoT Ready");

    // 4. Tạo Task chạy ngầm cho Telemetry
    // Task này sẽ chạy song song, không ảnh hưởng đến việc nhận lệnh MQTT
    xTaskCreate(sensor_telemetry_task, "sensor_task", 4096, NULL, 5, NULL);
}