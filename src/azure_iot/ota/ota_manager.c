#include "ota_manager.h"
#include "ota_config.h"

#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_err.h"
#include "azure_iot_internal.h"
#include <string.h>
#include <stdio.h>
#include <app_version.h>
extern const char azure_ca_pem_start[] asm("_binary_azure_ca_pem_start"); 
static const char *TAG = "OTA_MGR";

static ota_state_t s_state = OTA_STATE_IDLE;
static char s_last_error[64] = "none";

/* ================= UTIL ================= */

static void set_error(const char *err)
{
    strncpy(s_last_error, err, sizeof(s_last_error) - 1);
}

static void bytes_to_hex(const uint8_t *src, size_t len, char *dst)
{
    for (size_t i = 0; i < len; i++) {
        sprintf(dst + (i * 2), "%02x", src[i]);
    }
    dst[len * 2] = 0;
}

#if OTA_ENABLE_SHA256
static bool verify_sha256(const char *expected_hex)
{
    uint8_t sha256_bin[32];
    char sha256_hex[65];

    const esp_partition_t *part =
        esp_ota_get_next_update_partition(NULL);

    if (!part) {
        ESP_LOGE(TAG, "No OTA partition");
        return false;
    }

    esp_err_t err = esp_partition_get_sha256(part, sha256_bin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SHA256 read failed");
        return false;
    }

    bytes_to_hex(sha256_bin, 32, sha256_hex);

    ESP_LOGI(TAG, "SHA256 actual  : %s", sha256_hex);
    ESP_LOGI(TAG, "SHA256 expected: %s", expected_hex);

    return (strcasecmp(sha256_hex, expected_hex) == 0);
}
#endif

/* ================= API ================= */

void ota_manager_init(void)
{
    s_state = OTA_STATE_IDLE;
    strcpy(s_last_error, "none");
}

ota_state_t ota_manager_get_state(void)
{
    return s_state;
}

const char *ota_manager_get_last_error(void)
{
    return s_last_error;
}

bool ota_manager_request(const ota_request_t *req)
{
    if (!req || strlen(req->url) == 0) {
        set_error("invalid_request");
        return false;
    }

    ESP_LOGI(TAG, "OTA start");
    ESP_LOGI(TAG, "URL     : %s", req->url);
    ESP_LOGI(TAG, "Version : %s", req->version);

    s_state = OTA_STATE_DOWNLOADING;

    esp_http_client_config_t http_cfg = {
        .url = req->url,
        .timeout_ms = 20000,
        .keep_alive_enable = true,
        .cert_pem = azure_ca_pem_start, // nếu HTTPS private
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_https_ota_handle_t ota_handle = NULL;

    esp_err_t err = esp_https_ota_begin(&ota_cfg, &ota_handle);
    if (err != ESP_OK) {
        set_error("ota_begin_failed");
        s_state = OTA_STATE_FAILED;
        return false;
    }

    while (true) {
        err = esp_https_ota_perform(ota_handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            continue;
        }
        break;
    }

    err = esp_https_ota_finish(ota_handle);
    if (err != ESP_OK) {
        set_error("ota_download_failed");
        esp_https_ota_abort(ota_handle);
        s_state = OTA_STATE_FAILED;
        return false;
    }
    
    #if OTA_ENABLE_SHA256
    s_state = OTA_STATE_VERIFYING;

    if (strlen(req->sha256) > 0) {
        if (!verify_sha256(req->sha256)) {
            set_error("sha256_mismatch");
            s_state = OTA_STATE_FAILED;
            return false;
        }
    }
    #endif
    if (err != ESP_OK) {
        set_error("ota_finish_failed");
        s_state = OTA_STATE_FAILED;
        return false;
    } 

    ESP_LOGI(TAG, "OTA success → reboot");
    s_state = OTA_STATE_SUCCESS;
    esp_restart();
    return true;
}

void ota_manager_confirm_if_needed(void)
{
    const esp_partition_t *running =
        esp_ota_get_running_partition();
    esp_ota_img_states_t state;

    if (esp_ota_get_state_partition(running, &state) == ESP_OK) {
        if (state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Confirming new firmware");
            esp_ota_mark_app_valid_cancel_rollback();
        }
    }
}
void ota_report_version(void)  // report firmware version lên Azure Twin test
{
    char payload[256];

    snprintf(payload, sizeof(payload),
        "{"
        "\"firmware\":{"
            "\"version\":\"%s\","
            "\"build\":\"%s\""
        "}"
        "}",
        APP_VERSION,
        APP_BUILD
    );

    azure_iot_publish_payload(payload); // test can chinh lại sau
}
static bool parse_version(const char *v, int *a, int *b, int *c)
{
    if (!v) return false;
    return sscanf(v, "%d.%d.%d", a, b, c) == 3;
}

bool version_is_newer(const char *new_v, const char *cur_v)
{
    int n1, n2, n3;
    int c1, c2, c3;

    if (!parse_version(new_v, &n1, &n2, &n3)) return false;
    if (!parse_version(cur_v, &c1, &c2, &c3)) return false;

    if (n1 != c1) return n1 > c1;
    if (n2 != c2) return n2 > c2;
    return n3 > c3;
}
