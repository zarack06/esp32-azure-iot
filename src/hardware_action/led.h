#pragma once
#include <stdbool.h>
void hardware_led_set(int status, const char* place, int time);
void gpios_all_init(void);
void led2_toggle(void);
void led2_set(const char* place, int time);
void hardware_init_timers(void);
bool led2_get_state(void);