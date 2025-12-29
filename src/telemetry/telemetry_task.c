#include "esp_log.h"
#include "freertos/FreeRTOS.h" 
#include "sensor_data/sensor_data.h"
#include "azure_iot.h"
#include "wifi_manager.h"
#include "oled.h"
#include "oled_text.h"
#include "telemetry_task.h"

static QueueHandle_t queue;

static void telemetry_task(void *pv) {
    sensor_data_t data;
    char total_payload[512]; 

    while (1) {
        // Đợi 30 giây
        vTaskDelay(pdMS_TO_TICKS(30000));

        // CHỈ LÀM VIỆC KHI CÓ DỮ LIỆU TRONG QUEUE
        int msg_waiting = uxQueueMessagesWaiting(queue);
        if (azure_iot_is_connected() && msg_waiting > 0) {
            
            int offset = 0;
            // 1. Ghi dấu mở mảng
            offset += snprintf(total_payload + offset, sizeof(total_payload) - offset, "[");
            
            bool first = true;
            while (xQueueReceive(queue, &data, 0) == pdPASS) {
                int remaining = sizeof(total_payload) - offset;
                int written = snprintf(total_payload + offset, remaining, 
                                       "%s{\"t\":%.2f,\"h\":%.2f}", 
                                       first ? "" : ",", data.temperature, data.humidity);
                
                if (written < remaining) {
                    offset += written;
                    first = false;
                } else {
                    break; // Tràn bộ đệm
                }
            }

            // 2. Ghi dấu đóng mảng 
            if (offset < sizeof(total_payload) - 1) {
                snprintf(total_payload + offset, sizeof(total_payload) - offset, "]");
            }

            // 3. Gửi đi
            azure_iot_send_telemetry_raw(total_payload);
            ESP_LOGI("AZURE", "Gửi thành công %d bản ghi", msg_waiting);
            
        } else if (msg_waiting == 0) {
            ESP_LOGD("AZURE", "Queue trống, không có gì để gửi.");
        }
    }
}

void telemetry_task_start(QueueHandle_t sensor_queue)
{
    queue = sensor_queue;
    xTaskCreate(telemetry_task, "telemetry", 8192, NULL, 4, NULL);
}
