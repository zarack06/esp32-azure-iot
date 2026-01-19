#include <stdio.h>
#include "helper_time.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"     // Để dùng vTaskDelay và pdMS_TO_TICKS
#include "oled.h"            
#include "freertos/semphr.h" // Để dùng Semaphore
#include <string.h>            // Để dùng strlen, memcpy
#include "oled_text.h" 
#include <time.h>
#include <app_version.h>
#define TAG "HELPER_TIME" 

static uint32_t current_errors = 0; // Biến lưu tổng hợp các lỗi đang tồn tại

static SemaphoreHandle_t error_mutex;
static char last_error_msg[64] = "System OK";

int time_to_minutes(const char *hhmm)
{
    int h, m;
    if (sscanf(hhmm, "%d:%d", &h, &m) != 2) return -1;
    return h * 60 + m;
} 
int64_t helper_time_get_unix(void)
{
    time_t now;
    time(&now);
    return (int64_t)now;
}
void error_log_init(void) {
    if (error_mutex == NULL) {
        error_mutex = xSemaphoreCreateMutex();
    }
}
// void set_last_error(const char *msg)
// {   ESP_LOGI(TAG, "set_last_error=: %s", msg);
//     if (!msg || !error_mutex) return;

//     if (xSemaphoreTake(error_mutex, pdMS_TO_TICKS(50))) {
//         snprintf(last_error_msg, sizeof(last_error_msg), "%s", msg);
//         xSemaphoreGive(error_mutex);
//     }
// }
void update_sys_error(sys_error_bit_t error_bit, bool is_error, const char *msg) {
    if (!error_mutex) return;

    if (xSemaphoreTake(error_mutex, pdMS_TO_TICKS(100))) {
        if (is_error) {
            // BẬT bit lỗi: Dùng phép toán OR (|)
            current_errors |= error_bit;
            // Cập nhật dòng chữ hiển thị lỗi mới nhất
            if (msg) snprintf(last_error_msg, sizeof(last_error_msg), "%s", msg);
        } else {
            // TẮT bit lỗi: Dùng phép toán AND NOT (& ~)
            current_errors &= ~error_bit;
            
            // Nếu sau khi tắt bit này mà bằng 0, nghĩa là sạch bóng lỗi
            if (current_errors == 0) {
                snprintf(last_error_msg, sizeof(last_error_msg), "%s", "System OK");
            } else {
                // Tùy chọn: Bạn có thể cập nhật msg báo là "Thành phần X đã OK"
            }
        }
        xSemaphoreGive(error_mutex);
    }
}

// Hàm để Task OLED "lấy" lỗi ra hiển thị
const char* get_last_error(void) {
    static char buf[64];
    if (error_mutex == NULL) {
        error_mutex = xSemaphoreCreateMutex();
    }
    ESP_LOGI(TAG, "last_error_msg=%s", last_error_msg);
    if (xSemaphoreTake(error_mutex, pdMS_TO_TICKS(50))) {
        strncpy(buf, last_error_msg, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        xSemaphoreGive(error_mutex);
    }
    return buf;
} 
/**
 * @brief Hiển thị lỗi dài lên OLED (tự động xuống dòng)
 * @param start_page: Hàng bắt đầu in (ví dụ: hàng 4)
 * @param msg: Chuỗi lỗi cần in
 */ 
static void oled_display_error_multiline(uint8_t start_page, const char *msg) {
   static char line_buffer[22]; // OLED thường có 21 ký tự/dòng
    int msg_len = strlen(msg);
    int offset = 0;
    uint8_t page = start_page; 
     ESP_LOGW(TAG,"msg = %s",msg);
    // 2. In lỗi, nhưng khống chế không cho vượt quá dòng 2
    while (offset < msg_len && page < 3) {
        // Lấy tối đa 21 ký tự
        int len = (msg_len - offset > 21) ? 21 : msg_len - offset;
        
        memcpy(line_buffer, msg + offset, len);
        line_buffer[len] = '\0';

        oled_put_string_5x7(page, 0, line_buffer); 
        
        offset += len;
        page++;
    }
}

void oled_update_err_task(void *pv) {
    while (1) {
        static char Line_version[22];
         snprintf(Line_version, sizeof(Line_version), "ver: %s", APP_VERSION);
        oled_put_string_5x7(0, 0, Line_version); 
        oled_display_error_multiline(1, get_last_error());
        
        // có thể in thêm mã Hex của current_errors để debug
        // char debug[20]; snprintf(debug, 20, "Hex: 0x%02X", current_errors);
        // oled_put_string_5x7(7, 0, debug); 
        char quick_stat[22] = "E:"; // E là Error
        if (current_errors == 0) {
            strcpy(quick_stat, ".        "); // Không lỗi gì
        } else {
            if (current_errors & SYS_ERROR_WIFI)  strcat(quick_stat, " WiFi");
            if (current_errors & SYS_ERROR_AHT10) strcat(quick_stat, " Snsr");
            if (current_errors & SYS_ERROR_AZURE) strcat(quick_stat, " Azur");
            if (current_errors & SYS_ERROR_I2C)   strcat(quick_stat, " I2C");
            if (current_errors & SYS_ERROR_OLED)  strcat(quick_stat, " OLED");
            if (current_errors & SYS_ERROR_NVS)   strcat(quick_stat, " NVS");
            if (current_errors & SYS_ERROR_SNTP)  strcat(quick_stat, " SNTP");
            if (current_errors & SYS_ERROR_MQTT)  strcat(quick_stat, " MQTT");
            if (current_errors & SYS_ERROR_BUFFER)  strcat(quick_stat, " BUF");
            // Thêm các khoảng trắng để xóa dấu vết cũ
        }
        oled_put_string_5x7(7, 0, quick_stat);  
        vTaskDelay(pdMS_TO_TICKS(5000)); // Cứ 5 giây cập nhật một lần 
        // Định nghĩa nhóm lỗi liên quan đến kết nối mạng


        // #define NETWORK_ERRORS (SYS_ERROR_WIFI | SYS_ERROR_AZURE | SYS_ERROR_MQTT)

        // // Kiểm tra nhanh trong 1 nốt nhạc
        // if (current_errors & NETWORK_ERRORS) {
        //     // Chỉ cần 1 trong 3 thằng trên lỗi là nhảy vào đây ngay
        //     ESP_LOGE(TAG, "Hệ thống đang mất kết nối mạng!");
        // }
    }
}