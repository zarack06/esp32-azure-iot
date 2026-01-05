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
/*************  ✨ Windsurf Command ⭐  *************/
/**
 * @brief Đọc dữ liệu cảm biến AHT10 và gửi đến queue sensor_data
 * 
 * Task này đọc dữ liệu cảm biến AHT10 và gửi đến queue sensor_data
 * khi có thay đổi giá trị lớn hơn 0.5C hoặc sau 1 phút
 * 
 * @param pvParameters Không sử dụng
 * 
 * @return Không có giá trị trả về
 */
/*******  8de3a87d-7032-4864-a315-365e47271716  *******/
static void sensor_reading_task(void *pvParameters)
{
    sensor_data_t data;
    char buf[32];
    int error_count = 0;
    
    // Thêm biến lưu thời điểm gửi cuối cùng (tính bằng Tick)
    TickType_t last_send_time = xTaskGetTickCount();
    const TickType_t timeout_1min = pdMS_TO_TICKS(60000); // 60 giây

    while (1) {  
        if (aht10_read(&data.temperature, &data.humidity) == ESP_OK) {
            error_count = 0; // Reset lỗi khi đọc thành công

            // Cập nhật OLED
            snprintf(buf, sizeof(buf), "T: %.2f C       ", data.temperature);
            oled_put_string_5x7(5, 0, buf);
            snprintf(buf, sizeof(buf), "Hum: %.2f %%        ", data.humidity);
            oled_put_string_5x7(6, 0, buf);

            // KIỂM TRA ĐIỀU KIỆN GỬI DỮ LIỆU
            bool threshold_exceeded = (last_read_temp != -999.0 && fabs(data.temperature - last_read_temp) > 0.5);
            bool timeout_reached = (xTaskGetTickCount() - last_send_time >= timeout_1min);

            if (threshold_exceeded || timeout_reached) {
                if (xQueueSend(sensor_queue, &data, 0) == pdPASS) {
                    if (threshold_exceeded) {
                        ESP_LOGI("SENSOR", "Bien dong manh (>0.5C)! Gui Queue");
                    } else {
                        ESP_LOGI("SENSOR", "Het 1 phut! Gui thong so dinh ky");
                    }
                    
                    // Cập nhật lại mốc thời gian và giá trị cuối sau khi gửi thành công
                    last_send_time = xTaskGetTickCount();
                    last_read_temp = data.temperature;
                }
            }
        } else {
            error_count++;
            ESP_LOGW("SENSOR", "Lỗi đọc dữ liệu lần %d", error_count);

            if (error_count >= 3) {
                ESP_LOGE("SENSOR", "Mất kết nối cảm biến! Đang thử init lại...");
                aht10_init();
                error_count = 0; 
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Chu kỳ quét 5 giây
    }
}

QueueHandle_t sensor_task_start(void)
{
    sensor_queue = xQueueCreate(20, sizeof(sensor_data_t));
    xTaskCreate(sensor_reading_task, "sensor", 4096, NULL, 5, NULL);
    return sensor_queue;
}
