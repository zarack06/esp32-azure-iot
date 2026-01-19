#pragma once
#include <stdbool.h>
#include "ota_config.h"

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_VERIFYING,
    OTA_STATE_SUCCESS,
    OTA_STATE_FAILED,
    OTA_STATE_ROLLBACK
} ota_state_t;

typedef struct {
    char url[OTA_MAX_URL_LEN];
    char version[OTA_MAX_VER_LEN];
    char sha256[OTA_SHA256_LEN + 1]; // hex string
} ota_request_t;

// gọi khi boot
void ota_manager_init(void);
void ota_report_version(void);
// gọi khi nhận Azure Twin
bool ota_manager_request(const ota_request_t *req);

// confirm OTA sau khi app chạy ổn
void ota_manager_confirm_if_needed(void);

// để report lên Twin
ota_state_t ota_manager_get_state(void);
const char *ota_manager_get_last_error(void);
bool version_is_newer(const char *new_v, const char *cur_v);