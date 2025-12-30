 
//===========================================================================  
#include "esp_log.h"  
 
// Azure IoT Middleware headers
extern "C" { 
    /* ================= I2C ================= */ 
    #include "oled.h" 
    #include "aht10.h"
    #include "azure_service.h"
    #include "wifi_manager.h"
    #include "system_init.h"
    #include "azure_iot/azure_iot.h"
    #include "hardware_action/led.h"
    #include "sensor/sensor_task.h"
    #include "telemetry/telemetry_task.h"
    #include "azure_iot/azure_tx_task.h"
     
}
static const char *TAG = "AZURE_IOT_ESP32"; 

extern "C" void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(500));
    system_init();  
    vTaskDelay(pdMS_TO_TICKS(100)); // Đợi I2C bus ổn định
    if (aht10_init() != ESP_OK) {
        ESP_LOGW(TAG, "AHT10 chưa sẵn sàng, Task sẽ thử lại sau...");
    }
    oled_init();
    oled_clear();
    // 2. ƯU TIÊN: Chạy Task Cảm biến và OLED trước   
    wifi_init();      
    time_sync_init(); 
    // 2. Cấu hình Azure & Đăng ký callback
    azure_service_register_led_callback(hardware_led_set);// nhận lệnh từ azure 
    azure_service_init();               // (1) logic nghiệp vụ
    azure_iot_init();                   // (2) Azure IoT client
    azure_iot_start();                  // (3) MQTT connect
    // Task gửi Azure (Ưu tiên thấp hơn) 
    
    //=====================
    azure_tx_task_start(); 
    QueueHandle_t q = sensor_task_start();
    telemetry_task_start(q); 
}