
#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

char *generate_sas_token(
    const char *host,
    const char *device_id,
    const char *device_key,
    int expiry_minutes);


#ifdef __cplusplus
}
#endif 