#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool pump_running;
    bool pump_auto_mode;
    bool heater_on;

    uint32_t pump_started_at;
    uint32_t pump_duration_sec;
    uint32_t pump_remaining_sec;

    char last_error[64];
} runtime_state_t;

/* API */
void runtime_state_init(void);
runtime_state_t* runtime_state_get(void);
void runtime_state_set_error(const char *msg);
