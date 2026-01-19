#pragma once

#ifndef  SCHEDULE_CONFIG_H
#define  SCHEDULE_CONFIG_H 
#define MAX_SCHEDULE_TASKS 8
#include <stdint.h>
#include <stdbool.h>
#include "cJSON.h"
#ifdef __cplusplus
extern "C" {
#endif   
typedef struct {
    uint8_t id;
    uint8_t hour;
    uint8_t minute;
    uint16_t duration_min;
} schedule_task_t;

typedef struct {
    uint32_t version;
    bool enabled;
    uint8_t task_count;
    schedule_task_t tasks[MAX_SCHEDULE_TASKS];
} device_schedule_t;


void schedule_init(void);
bool schedule_parse_from_json_schedule(
    const cJSON *json,
    uint32_t twin_version,
    device_schedule_t *out);
void schedule_set(const device_schedule_t *sch);
device_schedule_t schedule_get(void);
#ifdef __cplusplus
}
#endif
#endif /* SCHEDULE_CONFIG_H */