#pragma once


#ifndef AHT10_H
#define AHT10_H

#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @brief Initialize AHT10 sensor
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
esp_err_t aht10_init(void);

/**
 * @brief Read temperature and humidity from AHT10
 *
 * @param temperature Pointer to temperature (°C)
 * @param humidity Pointer to humidity (%RH)
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
esp_err_t aht10_read(float *temperature, float *humidity);
#ifdef __cplusplus
}
#endif
#endif /* AHT10_H */