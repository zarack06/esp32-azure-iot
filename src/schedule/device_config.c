#include "device_config.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#define NVS_NS "devcfg"
static device_config_t g_cfg;
#define TAG "DEVICE_CONFIG"
#define CONFIG_DEFAULT_VERSION  1
#define CONFIG_DEFAULT_SAMPLING 30
#define CONFIG_DEFAULT_TEMP_MAX 32.0f

static void config_load_default(void)
{
    g_cfg.version = CONFIG_DEFAULT_VERSION;
    g_cfg.sampling_interval = CONFIG_DEFAULT_SAMPLING;
    g_cfg.temp_max = CONFIG_DEFAULT_TEMP_MAX;
    g_cfg.auto_mode = true;
    g_cfg.pump = false;
    g_cfg.heater = false;
}

void config_init(void)
{
    nvs_handle_t nvs;
    size_t size = sizeof(g_cfg);

    if (nvs_open(NVS_NS, NVS_READONLY, &nvs) == ESP_OK) {
        if (nvs_get_blob(nvs, "cfg", &g_cfg, &size) != ESP_OK) {
            config_load_default();
        }
        nvs_close(nvs);
    } else {
        config_load_default();
    }
}


device_config_t config_get(void)
{
    return g_cfg;
}


void config_set(device_config_t *cfg   )
{    
    if (memcmp(&g_cfg, cfg, sizeof(device_config_t)) == 0) {
        return; // không đổi
    }
    g_cfg = *cfg;

    nvs_handle_t nvs;
    if (nvs_open(NVS_NS, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_blob(nvs, "cfg", cfg, sizeof(*cfg));
        nvs_commit(nvs);
        nvs_close(nvs);
    } else {
        // Lỗi mở NVS để ghi
        ESP_LOGE(TAG, "Failed to open NVS for writing");
    }
}
