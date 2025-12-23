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
#define WIFI_CONNECTED_BIT BIT0
static const char *TAG = "WIFI";
static bool wifi_started = false;
static EventGroupHandle_t s_wifi_event_group;

// Hàm xử lý sự kiện Wi-Fi
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Đang thử kết nối lại Wi-Fi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Khởi tạo Wi-Fi
void wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);
    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi đã kết nối thành công!");
}
// Đồng bộ thời gian qua SNTP (Azure bắt buộc phải có thời gian đúng)
void time_sync_init(void) {
    ESP_LOGI(TAG, "Đang lấy thời gian từ Internet...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    time_t now = 0;
    struct tm timeinfo = {};
    while (timeinfo.tm_year < (2024 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    ESP_LOGI(TAG, "Thời gian hiện tại: %s", asctime(&timeinfo));
}
bool wifi_manager_is_connected(void)
{
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}
