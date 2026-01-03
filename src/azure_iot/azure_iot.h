#pragma once

#ifndef AZURE_IOT_H
#define AZURE_IOT_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif  

void azure_iot_init(void);
void azure_iot_start(void); 
bool azure_iot_is_connected(void);  
#ifdef __cplusplus
}
#endif
#endif /* AZURE_IOT_H */