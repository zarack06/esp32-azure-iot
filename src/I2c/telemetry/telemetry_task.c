#include "esp_log.h"
#include "freertos/FreeRTOS.h" 
#include "sensor_data/sensor_data.h"
#include "azure_iot.h"
#include "wifi_manager.h"
#include "oled.h"
#include "oled_text.h"
#include "telemetry/telemetry_task.h"
#include "azure_tx_task.h"
#include "helper_time.h"
#include <time.h>
static QueueHandle_t queue;

static void telemetry_task(void *pv) {
    sensor_data_t data;
    char total_payload[512]; 
    int tick = 0;
    while (1) {
        int offset = 0;
        int sent_count = 0;
        // Đợi 30 giây
        vTaskDelay(pdMS_TO_TICKS(100)); // ngủ rất ngắn
        tick++;
        if (tick < 300) continue; // đủ 30s mới kiểm tra
        tick = 0;
        // CHỈ LÀM VIỆC KHI CÓ DỮ LIỆU TRONG QUEUE
        if (!azure_iot_is_connected()) continue;
        if (uxQueueMessagesWaiting(queue) == 0) continue;
             
            // 1. Ghi dấu mở mảng
            offset += snprintf(total_payload + offset, sizeof(total_payload) - offset, "[");
            time_t now = (time_t)helper_time_get_unix();
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);  
            bool first = true;
            while (xQueueReceive(queue, &data, 0) == pdPASS) {
                int remaining = sizeof(total_payload) - offset;
                int written = snprintf(total_payload + offset, remaining, 
                                       "%s{\"t\":%.2f,\"h\":%.2f, \"time\":\"%02d:%02d:%02d\"}", 
                                       first ? "" : ",", data.temperature, data.humidity,
                                       timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

                if (written < remaining) {
                    offset += written;
                    sent_count++;
                    first = false;
                } else {
                    ESP_LOGI("AZURE", "tràn bộ đệm= %d", written); 
                    break; // Tràn bộ đệm
                }
            }

            // 2. Ghi dấu đóng mảng 
            if (offset < sizeof(total_payload) - 1) {
                snprintf(total_payload + offset, sizeof(total_payload) - offset, "]");
            }

            // 3. Gửi vào queue
            azure_tx_send_telemetry(total_payload); 
            ESP_LOGI("AZURE", "Gửi thành công %d bản ghi", sent_count); 
    }
}

void telemetry_task_start(QueueHandle_t sensor_queue)
{
    queue = sensor_queue;
    xTaskCreate(telemetry_task, "telemetry", 8192, NULL, 4, NULL);
}
