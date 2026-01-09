#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_manager.h"
#include "esp_log.h"
#include "config.h"
#include "aht10.h"
#define TAG "AHT10"   
#include "helper_time.h" // Chứa hàm set_last_error 

static i2c_master_dev_handle_t aht10_dev_handle = NULL;
extern i2c_master_bus_handle_t global_i2c_bus_handle; // Lấy từ i2c_manager.c 

esp_err_t aht10_init(void)
{    
    // 1. Kiểm tra Bus tổng đã sẵn sàng chưa
    if (global_i2c_bus_handle == NULL) {
         update_sys_error(SYS_ERROR_AHT10, true, "I2C Bus Not Init");
        return ESP_ERR_INVALID_STATE;
    }  
    // 2. Nếu chưa đăng ký AHT10 vào Bus thì đăng ký (chỉ làm 1 lần)
    if (aht10_dev_handle == NULL) {
        i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT10_I2C_ADDRESS,               // Địa chỉ I2C của AHT10
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,               // Tốc độ bus (Hz)  
    };
        
        esp_err_t err = i2c_master_bus_add_device(global_i2c_bus_handle, &dev_cfg, &aht10_dev_handle);
        if (err != ESP_OK) {
            update_sys_error(SYS_ERROR_AHT10, true, "AHT10 Dev Fail init");
            return err;
        } 
    }

    // 3. Chờ cảm biến ổn định (AHT10 cần thời gian sau khi cấp nguồn 50~70)
    vTaskDelay(pdMS_TO_TICKS(100));

    // 4. Gửi lệnh khởi tạo 0xE1
    uint8_t cmd[3] = {0xE1, 0x08, 0x00};
    
    // Lưu ý: Driver mới dùng timeout trực tiếp bằng ms (int) 
    esp_err_t err = i2c_master_transmit(aht10_dev_handle, cmd, sizeof(cmd), 100);

    if (err != ESP_OK) {
        // Trả về lỗi chi tiết dựa trên mã lỗi hệ thống
        if (err == ESP_ERR_TIMEOUT) {
             update_sys_error(SYS_ERROR_AHT10, true, "AHT10 Timeout init");
             ESP_LOGI(TAG, "AHT10 Timeout");
        } else if (err == ESP_ERR_NOT_FOUND) {
            update_sys_error(SYS_ERROR_AHT10, true, "AHT10 Not Found init"); // Không thấy địa chỉ 0x38 trên bus
            ESP_LOGI(TAG, "AHT10 Not Found ");
        } else {
            char buf[32];
            snprintf(buf, sizeof(buf), "AHT10 Err: 0x%x", err);
            update_sys_error(SYS_ERROR_AHT10, true, buf);
             ESP_LOGI(TAG, "AHT10 err %s ", buf);
        }
        return err;
    }

    // 5. Nếu mọi thứ OK, xóa thông báo lỗi trên LED
    ESP_LOGI(TAG, "AHT10 initialized successfully"); 
    return ESP_OK;
} 

esp_err_t aht10_read(float *temperature, float *humidity)
{
    if (!temperature || !humidity) return ESP_ERR_INVALID_ARG;
    
    // Kiểm tra xem handle tạo ở hàm init chưa
    if (aht10_dev_handle == NULL) {
        update_sys_error(SYS_ERROR_AHT10, true, "AHT10 Not Init");
        return ESP_ERR_INVALID_STATE;
    } 
    uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t data[6];

    /* 1. Kích hoạt đo (Trigger measurement) */
    // Driver : i2c_master_transmit(handle, buffer, size, timeout_ms)
    esp_err_t err = i2c_master_transmit(aht10_dev_handle, measure_cmd, 3, 100);
    if (err != ESP_OK) {
        update_sys_error(SYS_ERROR_AHT10, true, "AHT10 Trigger Fail");
        return err;
    }
    /* 2. Chờ cảm biến xử lý dữ liệu */
    vTaskDelay(pdMS_TO_TICKS(100)); 

    /* 3. Đọc dữ liệu (Read data) */
    // i2c_master_receive(handle, buffer, size, timeout_ms)
    err = i2c_master_receive(aht10_dev_handle, data, 6, 100);
    
    if (err != ESP_OK) {
        update_sys_error(SYS_ERROR_AHT10, true, "AHT10 Read Fail");
        return err;
    } 
    /* 4. Kiểm tra trạng thái dữ liệu (Tùy chọn nhưng nên có) */
    // Bit 7 của data[0] là Busy indication (0: Ready, 1: Busy)
    if (data[0] == 0x00 && data[1] == 0x00) {
        update_sys_error(SYS_ERROR_AHT10, true, "AHT10 Data Empty");
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* 5. Tính toán giá trị */
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) | 
                            ((uint32_t)data[2] << 4) | 
                            ((data[3] & 0xF0) >> 4);

    uint32_t raw_temperature = ((uint32_t)(data[3] & 0x0F) << 16) | 
                               ((uint32_t)data[4] << 8) | 
                               data[5];

    *humidity = ((float)raw_humidity / 1048576.0f) * 100.0f;
    *temperature = ((float)raw_temperature / 1048576.0f) * 200.0f - 50.0f;

    // Nếu đọc thành công, hiện trạng thái OK
    update_sys_error(SYS_ERROR_AHT10, false, NULL);
    
    return ESP_OK;
}