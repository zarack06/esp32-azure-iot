#include "driver/i2c_master.h"  
#include "Display.h"
#include "config.h"
#include "i2c_manager.h" // File chứa global_i2c_bus_handle

static i2c_master_dev_handle_t oled_dev_handle;

esp_err_t i2c_master_display_init(void)
{
    // 1. Cấu hình "định danh" cho OLED trên Bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_ADDR,           // Địa chỉ mặc định của OLED SSD1306
        .scl_speed_hz = 400000,           // OLED thường chạy mượt ở 400kHz (Fast Mode)
    };

    // 2. Đăng ký OLED vào bus chung đã khởi tạo ở i2c_master_init
    esp_err_t err = i2c_master_bus_add_device(global_i2c_bus_handle, &dev_cfg, &oled_dev_handle);
    if (err != ESP_OK) {
        return err;
    }

    // 3. Các lệnh khởi tạo OLED sau đó dùng oled_dev_handle
    // Ví dụ: gửi lệnh Reset, bật hiển thị...
    return ESP_OK;
}