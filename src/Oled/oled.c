// oled.c
 
// #include <stdio.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
// #include "esp_log.h"
#include "config.h" 

#include "oled.h" 
#include "driver/i2c_master.h"
#include "i2c_manager.h" // Nơi chứa global_i2c_bus_handle

static i2c_master_dev_handle_t oled_dev_handle = NULL;

// Hàm đăng ký OLED vào Bus (Gọi cái này thay vì i2c_driver_install)
static void oled_register_device() {
    if (oled_dev_handle != NULL) return; // Đã đăng ký rồi

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3C, // Địa chỉ OLED
        .scl_speed_hz = 400000, // OLED chạy nhanh 400kHz cho mượt
    };
    i2c_master_bus_add_device(global_i2c_bus_handle, &dev_cfg, &oled_dev_handle);
}

static void oled_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd}; // 0x00 là Control Byte cho lệnh
    i2c_master_transmit(oled_dev_handle, buf, 2, 100);
}

void oled_data(uint8_t *data, size_t len)
{
    // Tạo một buffer tĩnh, chỉ cấp phát 1 lần duy nhất khi khởi động
    static uint8_t oled_buffer[129]; 

    if (len > 128) len = 128; // Bảo vệ chống tràn mảng

    oled_buffer[0] = 0x40; // Control byte
    memcpy(&oled_buffer[1], data, len);
    
    i2c_master_transmit(oled_dev_handle, oled_buffer, len + 1, 100);
}
/* ================= OLED init ================= */
void oled_init()
{
    // 1. Phải đăng ký thiết bị trước khi gửi lệnh
    oled_register_device();

    vTaskDelay(pdMS_TO_TICKS(100));

    oled_cmd(0xAE); // display off
    oled_cmd(0x20); oled_cmd(0x00); // horizontal addressing
    oled_cmd(0xB0);
    oled_cmd(0xC8);
    oled_cmd(0x00);
    oled_cmd(0x10);
    oled_cmd(0x40);
    oled_cmd(0x81); oled_cmd(0x7F);
    oled_cmd(0xA1);
    oled_cmd(0xA6);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xA4);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xD9); oled_cmd(0xF1);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14);
    oled_cmd(0xAF); // display ON
    
    oled_clear(); // Xóa màn hình ngay sau khi bật
} 
/* ================= OLED text ================= */

void oled_clear()
{
    uint8_t zero[128];
    memset(zero, 0, 128);

    for (int page = 0; page < 8; page++) {
        oled_cmd(0xB0 + page);
        oled_cmd(0x00);
        oled_cmd(0x10);
        oled_data(zero, 128);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}

void oled_set_cursor(uint8_t page, uint8_t col)
{
    oled_cmd(0xB0 + page);
    oled_cmd(0x00 + (col & 0x0F));
    oled_cmd(0x10 + (col >> 4));
} 
 

void oled_draw_bitmap(uint8_t page,
                      uint8_t col,
                      const uint8_t *bitmap,
                      uint8_t width,
                      uint8_t height)
{
    uint8_t pages = (height + 7) / 8;

    for (uint8_t p = 0; p < pages; p++) {
        oled_set_cursor(page + p, col);
        oled_data((uint8_t*)(bitmap + p * width), width);
    }
} 