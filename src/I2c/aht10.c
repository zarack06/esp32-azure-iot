#include "aht10.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "config.h"
#define TAG "AHT10" 

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    }; 
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    vTaskDelay(pdMS_TO_TICKS(500));
    if (err != ESP_OK) return err;

    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}
 

esp_err_t aht10_init(void)
{
    static bool i2c_initialized = false;

    if (!i2c_initialized)
    {
        esp_err_t err = i2c_master_init();
        vTaskDelay(pdMS_TO_TICKS(500));
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "I2C init failed: %s", esp_err_to_name(err));
            return err;
        }
        i2c_initialized = true;
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // AHT10 wake-up time

    uint8_t cmd[3] = {0xE1, 0x08, 0x00};
    esp_err_t err = i2c_master_write_to_device(
        I2C_MASTER_NUM,
        AHT10_I2C_ADDRESS,
        cmd,
        sizeof(cmd),
        pdMS_TO_TICKS(100)
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AHT10 init cmd failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "AHT10 initialized successfully");
    return ESP_OK;
}


esp_err_t aht10_read(float *temperature, float *humidity)
{
    if (!temperature || !humidity)
        return ESP_ERR_INVALID_ARG;

    uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t data[6];

    /* Trigger measurement */
    esp_err_t err = i2c_master_write_to_device(
        I2C_MASTER_NUM,
        AHT10_I2C_ADDRESS,
        measure_cmd,
        sizeof(measure_cmd),
        pdMS_TO_TICKS(100)
    );
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "AHT10 errrrrrrrrrrrrr:");
        return err;
    }  

    vTaskDelay(pdMS_TO_TICKS(500));

    /* Read data */
    err = i2c_master_read_from_device(
        I2C_MASTER_NUM,
        AHT10_I2C_ADDRESS,
        data,
        6,
        pdMS_TO_TICKS(100)
    );
    if (err != ESP_OK) return err;

    uint32_t raw_humidity =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        ((data[3] & 0xF0) >> 4);

    uint32_t raw_temperature =
        ((uint32_t)(data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        data[5];

    *humidity = ((float)raw_humidity / 1048576.0f) * 100.0f;
    *temperature = ((float)raw_temperature / 1048576.0f) * 200.0f - 50.0f;  
    return ESP_OK;
}
