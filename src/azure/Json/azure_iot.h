#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Kết nối Azure IoT Hub bằng connection string
 */
esp_err_t azure_iot_connect();

/**
 * @brief Gửi telemetry (JSON) gồm nhiệt độ & độ ẩm
 */
esp_err_t azure_iot_send_telemetry(float temperature, float humidity);

/**
 * @brief Ngắt kết nối Azure IoT Hub
 */
esp_err_t azure_iot_disconnect(void);

/**
 * @brief Kiểm tra trạng thái kết nối
 */
bool azure_iot_is_connected(void);


#ifdef __cplusplus
}
#endif
