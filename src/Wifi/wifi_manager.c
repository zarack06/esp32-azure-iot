#include "wifi_manager.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "..\include\config.h"

#include "esp_sntp.h"
#include <time.h>
static const char *TAG = "WIFI";
static bool wifi_started = false;

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_started = false;
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_started = true;
        ESP_LOGI(TAG, "WiFi connected, got IP");
    }
}
static void obtain_time(void)
{
    ESP_LOGI("SNTP", "Initializing SNTP");

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
    {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry == retry_count)
    {
        ESP_LOGE("SNTP", "Failed to get time");
    }
    else
    {
        ESP_LOGI("SNTP", "Time synced: %s", asctime(&timeinfo));
    }
    ESP_LOGI("TIME", "Unix time: %ld", now);
}
void wifi_init_sta(void)
{
    if (wifi_started) return;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    obtain_time();
    wifi_started = true;
    
    ESP_LOGI(TAG, "WiFi started");
}
bool wifi_wait_connected(unsigned long timeout_ms)
{
    unsigned long start = xTaskGetTickCount();

    while (!wifi_manager_is_connected())
    {
        if ((xTaskGetTickCount() - start) > pdMS_TO_TICKS(timeout_ms))
            return false;

        vTaskDelay(pdMS_TO_TICKS(500));
    }
    return true;
}

bool wifi_manager_is_connected(void)
{
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}
