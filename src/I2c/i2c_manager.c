
#include "driver/i2c_master.h"
#include"i2c_manager.h" 
#include "config.h"
#include "esp_log.h"
#include "helper_time.h" // Chứa hàm set_last_error 
#define TAG "I2C_MANAGER" 


i2c_master_bus_handle_t global_i2c_bus_handle = NULL; 
esp_err_t i2c_manager_init(void)
{
    if (global_i2c_bus_handle != NULL) return ESP_OK; // Đã khởi tạo rồi 
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t err = i2c_new_master_bus(&bus_config, &global_i2c_bus_handle);
    if (err != ESP_OK) {
        set_last_error("Khoi tao I2C fail");
        ESP_LOGE(TAG, "Khoi tao I2C Bus that bai!");
        return err;
    }
    return ESP_OK;
}
  