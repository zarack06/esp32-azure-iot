#pragma once
#include <stdbool.h>
#include <stdint.h>  
#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H 
#ifdef __cplusplus
extern "C" {
#endif  
 
typedef struct {
    uint32_t version;
    uint16_t sampling_interval;
    float temp_max;
    bool auto_mode;
    bool pump;
    bool heater;
} device_config_t;

void config_init(void);
device_config_t config_get(void);
void config_set(device_config_t *cfg);


#ifdef __cplusplus
}
#endif
#endif /* DEVICE_CONFIG_H */