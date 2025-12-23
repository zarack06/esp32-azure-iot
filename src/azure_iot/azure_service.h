#pragma once


#ifndef AZURE_SERVICE_H
#define AZURE_SERVICE_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif  
void setup_azure(void);
void start_azure_mqtt(void);
void send_telemetry(float temperature, float humidity);
bool is_mqtt_connected_func(void);
void azure_service_register_led_callback(void (*cb)(int));
#ifdef __cplusplus
}
#endif
#endif /* AZURE_SERVICE_H */