#pragma once

#include "freertos/FreeRTOS.h"   
#include "freertos/queue.h"

// #include "work_schedule.h"
#include "sensor_data/sensor_data.h" 

#ifdef __cplusplus
extern "C" {
#endif

QueueHandle_t sensor_task_start(void);

#ifdef __cplusplus
}
#endif
