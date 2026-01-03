#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include "esp_err.h"
#include "driver/i2c_master.h"

// Khối này giúp C++ hiểu được các hàm viết bằng C
#ifdef __cplusplus
extern "C" {
#endif

// Khai báo extern để các file khác có thể thấy và sử dụng
extern i2c_master_bus_handle_t global_i2c_bus_handle;

// Hàm khởi tạo Bus
esp_err_t i2c_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif