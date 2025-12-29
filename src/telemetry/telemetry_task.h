#pragma once
#include "freertos/queue.h"
//  #include "sensor_task.h"

void telemetry_task_start(QueueHandle_t sensor_queue);
