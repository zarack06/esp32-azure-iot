#include <stdio.h>
#include "helper_time.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h" // Bắt buộc có cái này trước task.h
#include "freertos/task.h"     // Để dùng vTaskDelay và pdMS_TO_TICKS
#include "oled.h"            
#include <string.h>            // Để dùng strlen, memcpy
#include "oled_text.h"
int time_to_minutes(const char *hhmm)
{
    int h, m;
    if (sscanf(hhmm, "%d:%d", &h, &m) != 2) return -1;
    return h * 60 + m;
}

static char last_error_msg[63] = "System OK"; // Lưu lỗi cuối cùng

// Hàm để các file khác "ghi" lỗi vào
void set_last_error(const char* msg) {
    if (msg == NULL) return;
    snprintf(last_error_msg, sizeof(last_error_msg), "%s", msg);
    
    // Log ra để dễ debug qua cổng Serial
   // ESP_LOGW("STATUS", "New Error Captured: %s", last_error_msg);
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
    char line_buffer[22]; // 21 ký tự + 1 ký tự kết thúc '\0'
    int msg_len = strlen(msg);
    int chars_processed = 0;
    uint8_t current_page = start_page;

    // Duyệt qua toàn bộ chuỗi lỗi
    while (chars_processed < msg_len && current_page < 3) // OLED thường có 8 page (0-7)
    {
        // Tính toán xem dòng này sẽ in bao nhiêu ký tự
        int remaining_chars = msg_len - chars_processed;
        int chars_to_print = (remaining_chars > 21) ? 21 : remaining_chars;

        // Copy một đoạn 21 ký tự vào bộ đệm tạm
        memcpy(line_buffer, msg + chars_processed, chars_to_print); 

        // Gọi hàm  để in dòng 
        oled_put_string_5x7(current_page, 0, line_buffer);

        // Cập nhật vị trí đã xử lý và nhảy sang page tiếp theo
        chars_processed += chars_to_print;
        current_page++; 
    }
}
void oled_update_err(void *pv) {
    
    while (1) {
            oled_put_string_5x7(0, 0, "Last Error Log:"); // Tiêu đề ở hàng 0
            oled_display_error_multiline(1, get_last_error());
    
            vTaskDelay(pdMS_TO_TICKS(5000));
            set_last_error("System OK");
    }
}