// azure_iot_internal.h
#ifndef AZURE_IOT_INTERNAL_H
#define AZURE_IOT_INTERNAL_H 
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif  

void azure_iot_publish_payload(const char *payload);
void azure_iot_publish_topic(const char *topic, const char *payload);
 
#ifdef __cplusplus
}
#endif
#endif /* AZURE_IOT_INTERNAL_H */ 