#pragma once

#ifndef AZURE_IOT_H
#define AZURE_IOT_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif  

void azure_iot_init(void);
void azure_iot_start(void);
void azure_iot_send_telemetry(float temp, float hum);
bool azure_iot_is_connected(void);
void azure_iot_publish(const char *topic, const char *payload);
void azure_iot_send_telemetry_raw(const char *payload);
#ifdef __cplusplus
}
#endif
#endif /* AZURE_IOT_H */