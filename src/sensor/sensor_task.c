#include "esp_log.h"
#include <math.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "work_schedule.h" 
#include "aht10.h"
#include "oled.h"
#include "oled_text.h"
#include "sensor_task.h"

static QueueHandle_t sensor_queue;
static float last_read_temp = -999.0;

static void sensor_reading_task(void *pvParameters)
{
    sensor_data_t data;
    char buf[32];
    int error_count = 0; // Biến đếm lỗi  
    while (1) {  
        if (aht10_read(&data.temperature, &data.humidity) == ESP_OK) {

            snprintf(buf, sizeof(buf), "T: %.2f C       ", data.temperature);
            oled_put_string_5x7(5, 0, buf);

            snprintf(buf, sizeof(buf), "Hum: %.2f %%        ", data.humidity);
            oled_put_string_5x7(6, 0, buf);

            if (last_read_temp != -999.0 &&
                fabs(data.temperature - last_read_temp) > 0.5)
            {
                 if (xQueueSend(sensor_queue, &data, 0) == pdPASS) {
                    ESP_LOGI("SENSOR", "Bien dong manh! Da them vao Queue");
                }
            }

            last_read_temp = data.temperature;
        }else {
            error_count++;
            ESP_LOGW("SENSOR", "Lỗi đọc dữ liệu lần %d", error_count);

            // Chỉ khi lỗi 3 lần liên tiếp mới khởi tạo lại
            if (error_count >= 3) {
                ESP_LOGE("SENSOR", "Mất kết nối cảm biến! Đang thử init lại...");
                aht10_init();
                error_count = 0; // Reset để thử chu kỳ mới
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

QueueHandle_t sensor_task_start(void)
{
    sensor_queue = xQueueCreate(20, sizeof(sensor_data_t));
    xTaskCreate(sensor_reading_task, "sensor", 4096, NULL, 5, NULL);
    return sensor_queue;
}
