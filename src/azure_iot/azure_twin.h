#pragma once

#include <stdint.h>

/**
 * @brief Entry point để xử lý tất cả MQTT message từ Azure
 *        (được gọi trong MQTT_EVENT_DATA)
 */
void azure_twin_on_mqtt_message(
    const char *topic, int topic_len,
    const char *payload, int payload_len);

/**
 * @brief Gửi reported properties lên Azure
 */
void azure_twin_send_reported(void);
