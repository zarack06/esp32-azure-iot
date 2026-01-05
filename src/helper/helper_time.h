#pragma once
void error_log_init(void);
int time_to_minutes(const char *hhmm);
void oled_update_err(void *pv);
void set_last_error(const char* msg);