 
#include <stdbool.h> 
#define WIFI_MANAGER_H
#define WIFI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

void wifi_init_sta(void);
bool wifi_manager_is_connected(void);
bool wifi_wait_connected(unsigned long timeout_ms); 
#ifdef __cplusplus
}
#endif