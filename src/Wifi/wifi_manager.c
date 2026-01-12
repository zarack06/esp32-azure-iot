#include "wifi_manager.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "config.h"
#include "helper_time.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h" // Thư viện SNTP mới cho v5.x
#include <time.h>
#include <string.h>

#define WIFI_CONNECTED_BIT BIT0
static const char *TAG = "WIFI_MGR";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_count = 0;
static int s_current_wifi_index = 0; // 0: WiFi_1, 1: WiFi_2
#define MAX_RETRY 5 

/**
 * @brief Hàm cập nhật cấu hình WiFi linh hoạt
 * Tách riêng để dễ dàng gọi lại khi cần đổi mạng
 */
static void wifi_apply_current_config(void) {
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.rssi = -127, // Cho phép kết nối ngay cả khi sóng yếu
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
        },
    };

    if (s_current_wifi_index == 0) {
        strlcpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
        strlcpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));
    } else {
        strlcpy((char*)wifi_config.sta.ssid, WIFI_SSID2, sizeof(wifi_config.sta.ssid));
        strlcpy((char*)wifi_config.sta.password, WIFI_PASS2, sizeof(wifi_config.sta.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGW(TAG, "Đang thử kết nối SSID: %s", (char*)wifi_config.sta.ssid);
    esp_wifi_connect();
}

/**
 * @brief Xử lý sự kiện WiFi và IP
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_count < MAX_RETRY) {
            s_retry_count++;
            esp_wifi_connect();
            ESP_LOGI(TAG, "Thử kết nối lại lần %d...", s_retry_count);
            // Cập nhật lỗi hệ thống nếu có helper function
            update_sys_error(SYS_ERROR_WIFI, true, "Reconnect...");
        } else {
            // Khi vượt quá MAX_RETRY, thực hiện đảo mạng
            s_retry_count = 0;
            s_current_wifi_index = (s_current_wifi_index == 0) ? 1 : 0;
            ESP_LOGW(TAG, "Mất kết nối quá lâu. Chuyển sang WiFi dự phòng...");
            update_sys_error(SYS_ERROR_WIFI, true, "Reconnect...");
            wifi_apply_current_config();
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Đã lấy được IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_count = 0; 
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        update_sys_error(SYS_ERROR_WIFI, false, NULL);
    }
}

void wifi_init(void) { 

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    // Đặt Hostname chuyên nghiệp để quản lý trên Router
    esp_netif_set_hostname(sta_netif, "ESP32_Pro_HELLO");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // Đăng ký nhận sự kiện
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Khởi tạo WiFi đầu tiên
    wifi_apply_current_config();

    ESP_ERROR_CHECK(esp_wifi_start());

    // Tắt chế độ tiết kiệm điện để tăng độ ổn định cho các giao thức MQTT/Azure/Cloud
    esp_wifi_set_ps(WIFI_PS_NONE); 
}

/**
 * @brief Callback khi thời gian được đồng bộ
 */
void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI("TIME", "Đã đồng bộ thời gian từ NTP Server!");
}

void time_sync_init(void) {
    const int max_retry = 10;
    
    while (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "WiFi chưa kết nối, đợi SNTP..."); 
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Khởi tạo SNTP...");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.start = false; 
    config.server_from_dhcp = false; 
    config.sync_cb = time_sync_notification_cb; 
    
    esp_netif_sntp_init(&config);
    
    // Server dự phòng
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_setservername(2, "vn.pool.ntp.org");
    
    sntp_set_sync_interval(3600000); 
    esp_netif_sntp_start();

    // Chờ đồng bộ
    int retry = 0;
    while (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(5000)) != ESP_OK && ++retry < max_retry) {
        ESP_LOGI(TAG, "Đang đợi NTP lần %d...", retry);
    } 

    // LẤY THỜI GIAN THỰC TẾ ĐỂ KIỂM TRA
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    if (timeinfo.tm_year < (2024 - 1900)) { 
        ESP_LOGE(TAG, "Đồng bộ thời gian THẤT BẠI!");
        update_sys_error(SYS_ERROR_SNTP, true, "NTP Sync Failed");
    } else {
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "Thời gian hiện tại: %s", strftime_buf);
        update_sys_error(SYS_ERROR_SNTP, false, NULL);
    }
}

bool wifi_manager_is_connected(void) {
    return (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT) != 0;
}
