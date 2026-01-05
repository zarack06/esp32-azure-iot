#include <stdio.h>
#include "helper_time.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"     // Để dùng vTaskDelay và pdMS_TO_TICKS
#include "oled.h"            
#include "freertos/semphr.h" // Để dùng Semaphore
#include <string.h>            // Để dùng strlen, memcpy
#include "oled_text.h"
static SemaphoreHandle_t error_mutex;
static char last_error_msg[64] = "System OK";

int time_to_minutes(const char *hhmm)
{
    int h, m;
    if (sscanf(hhmm, "%d:%d", &h, &m) != 2) return -1;
    return h * 60 + m;
} 
void error_log_init(void)
{
    error_mutex = xSemaphoreCreateMutex();
}
void set_last_error(const char *msg)
{
    if (!msg || !error_mutex) return;

    if (xSemaphoreTake(error_mutex, pdMS_TO_TICKS(50))) {
        snprintf(last_error_msg, sizeof(last_error_msg), "%s", msg);
        xSemaphoreGive(error_mutex);
    }
}


// Hàm để Task OLED "lấy" lỗi ra hiển thị
const char* get_last_error(void) {
    return last_error_msg;
}
/**
 * @brief Hiển thị lỗi dài lên OLED (tự động xuống dòng)
 * @param start_page: Hàng bắt đầu in (ví dụ: hàng 4)
 * @param msg: Chuỗi lỗi cần in
 */ 
static void oled_display_error_multiline(uint8_t start_page, const char *msg)
{
    char line_buffer[22];
    int msg_len = strlen(msg);
    int offset = 0;
    uint8_t page = start_page;

    while (offset < msg_len && page < 7) {
        int len = (msg_len - offset > 21) ? 21 : msg_len - offset;

        memcpy(line_buffer, msg + offset, len);
        // line_buffer[len] = '\0';   // Thêm ký tự kết thúc chuỗi

        oled_put_string_5x7(page, 0, line_buffer);

        offset += len;
        page++;
    }
}

void oled_update_err(void *pv) {
    
    while (1) {
            oled_put_string_5x7(0, 0, "Last Error Log:"); // Tiêu đề ở hàng 0
            oled_display_error_multiline(1, get_last_error());
    
            vTaskDelay(pdMS_TO_TICKS(5000)); // Cứ 5 giây cập nhật một lần
            // set_last_error("System OK");
    }
}