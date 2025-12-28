#pragma once


#ifndef AZURE_SERVICE_H
#define AZURE_SERVICE_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif 

void azure_service_init(void);

/* Azure IoT gọi vào */
void azure_service_on_mqtt_message(
    const char *topic, int topic_len,
    const char *payload, int payload_len);

/* App đăng ký */
void azure_service_register_led_callback(void (*cb)(int));

#ifdef __cplusplus
}
#endif
#endif /* AZURE_SERVICE_H */