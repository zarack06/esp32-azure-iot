#pragma once

int time_to_minutes(const char *hhmm);
void oled_update_err(void *pv);
void set_last_error(const char* msg);