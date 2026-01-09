
#pragma once

#ifndef HELPER_TIME_H
#define HELPER_TIME_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus 
typedef enum {
    SYS_ERROR_WIFI    = (1 << 0), 
    SYS_ERROR_AHT10   = (1 << 1), 
    SYS_ERROR_AZURE   = (1 << 2), 
    SYS_ERROR_I2C     = (1 << 3),  
    SYS_ERROR_OLED    = (1 << 4), 
    SYS_ERROR_NVS     = (1 << 5), 
    SYS_ERROR_SNTP    = (1 << 6), 
    SYS_ERROR_MQTT    = (1 << 7),
    SYS_ERROR_BUFFER  = (1 << 8),
    SYS_ERROR_AZURE_CLIENT = (1 << 9),
    SYS_ERROR_TX_QUEUE = (1 << 10)
} sys_error_bit_t;
int64_t helper_time_get_unix(void);
void error_log_init(void);
int time_to_minutes(const char *hhmm);
void oled_update_err_task(void *pv);
void update_sys_error(sys_error_bit_t error_bit, bool is_error, const char *msg);
#ifdef __cplusplus
}
#endif
#endif  /* HELPER_TIME_H */