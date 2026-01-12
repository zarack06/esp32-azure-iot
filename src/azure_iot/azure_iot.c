#include "azure_iot.h"
#include "azure_service.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "az_iot_hub_client.h"
#include "config.h"
#include "azure_iot_internal.h"
#include "helper_time.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "mbedtls/sha256.h"
#include <time.h>
#define TAG "AZURE_IOT"   
static az_iot_hub_client client;
static esp_mqtt_client_handle_t mqtt_handle; 
static bool is_mqtt_connected = false;
static char az_username[128] = {0};
static char az_sas_token[512]; // Tăng kích thước buffer cho an toàn
extern const uint8_t azure_ca_pem_start[] asm("_binary_azure_ca_pem_start");
extern const uint8_t azure_ca_pem_end[]   asm("_binary_azure_ca_pem_end"); 

#define SAS_VALID_SECONDS      (10 * 60)      // 1 giờ  test  10'
#define SAS_RENEW_MARGIN       (2 * 60)      // renew trước 10 phút  test
static esp_mqtt_client_config_t mqtt_cfg = { 
    .broker.verification.certificate = (const char *)azure_ca_pem_start,
    .credentials.client_id = AZ_DEVICE_ID, 
    .broker.address.uri = "mqtts://" AZ_HOST_NAME, 
    .broker.verification.certificate = (const char *)azure_ca_pem_start

};
#define EVT_MQTT_IDLE      BIT0
#define EVT_MQTT_BUSY      BIT1
#define EVT_SAS_RENEWING   BIT2
static EventGroupHandle_t evt;
static int64_t sas_expiry_time = 0;
static SemaphoreHandle_t sas_mutex;

// Hàm bổ trợ để ký HMAC-SHA256 (Azure yêu cầu bước này)
#include "mbedtls/base64.h"
#include "mbedtls/md.h"

static az_result sign_signature_base64(
    az_span device_key_base64,
    az_span signature,
    az_span base64_hmac_buf,
    az_span *out_base64_hmac)
{
    /* 1. Decode device key (Base64 -> Raw) dùng mbedTLS */
    uint8_t decoded_key_buf[64]; // Tăng kích thước buffer cho an toàn
    size_t decoded_key_len = 0;

    int ret = mbedtls_base64_decode(
        decoded_key_buf, sizeof(decoded_key_buf), &decoded_key_len,
        (const unsigned char*)az_span_ptr(device_key_base64), 
        az_span_size(device_key_base64)
    );
    
    if (ret != 0) {
        ESP_LOGE("AZ_SIGN", "Base64 decode device key failed: -0x%04x", -ret);
        return AZ_ERROR_ARG;
    }

    /* 2. Tính HMAC-SHA256 dùng mbedTLS */
    uint8_t hmac_buf[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, decoded_key_buf, decoded_key_len);
    mbedtls_md_hmac_update(&ctx, az_span_ptr(signature), az_span_size(signature));
    mbedtls_md_hmac_finish(&ctx, hmac_buf);
    mbedtls_md_free(&ctx);

    /* 3. Encode HMAC ngược lại Base64 dùng mbedTLS */
    size_t out_len = 0;
    ret = mbedtls_base64_encode(
        az_span_ptr(base64_hmac_buf), az_span_size(base64_hmac_buf), &out_len,
        hmac_buf, sizeof(hmac_buf)
    );

    if (ret != 0) {
        ESP_LOGE("AZ_SIGN", "Base64 encode HMAC failed: -0x%04x", -ret);
        return AZ_ERROR_ARG;
    }

    // Cập nhật kết quả đầu ra
    *out_base64_hmac = az_span_create(az_span_ptr(base64_hmac_buf), (int32_t)out_len);

    return AZ_OK;
}


static bool azure_build_sas(void)
{
    az_result res;
    ESP_LOGI(TAG, "azure_build_sas!");
    // 0. Khởi tạo client nếu chưa có (Bắt buộc để SDK tính toán signature) 
    static bool client_inited = false;
    if (!client_inited) {
        az_iot_hub_client_options options = az_iot_hub_client_options_default();
        res = az_iot_hub_client_init(
            &client, 
            az_span_create_from_str(AZ_HOST_NAME), 
            az_span_create_from_str(AZ_DEVICE_ID), 
            &options
        );
        if (az_result_failed(res)) {
            ESP_LOGE(TAG, "Failed to init azure client");
            return false;
        }
        client_inited = true;
    }

    /* 1. Build MQTT Username (Dùng để kết nối MQTT) */
    res = az_iot_hub_client_get_user_name(
        &client,
        az_username,
        sizeof(az_username),
        NULL
    );
    if (az_result_failed(res)) {
        ESP_LOGE(TAG, "Get username failed");
        return false;
    }

    /* 2. Kiểm tra thời gian hệ thống (Dùng SNTP để đồng bộ trước đó) */
    int64_t now = (int64_t)time(NULL); // Sử dụng hàm chuẩn của ESP-IDF 2026
    if (now < 1700000000) { // Check nếu thời gian chưa đồng bộ (Epoch 2023+)
        ESP_LOGE(TAG, "System time not synced yet!");
        return false;
    }

    uint32_t expiry = (uint32_t)(now + SAS_VALID_SECONDS);
    xSemaphoreTake(sas_mutex, portMAX_DELAY);
    sas_expiry_time = expiry;
    xSemaphoreGive(sas_mutex); 
    /* 3. Tạo chuỗi ký (Signature base) */
    uint8_t signature_buf[256];
    az_span signature_span = AZ_SPAN_FROM_BUFFER(signature_buf);

    res = az_iot_hub_client_sas_get_signature(
        &client,
        expiry,
        signature_span,
        &signature_span // Lưu signature thực tế vào đây
    );
    if (az_result_failed(res)) {
        ESP_LOGE(TAG, "Get SAS signature failed");
        return false;
    }

    /* 4. Ký HMAC-SHA256 bằng Device Key */
    uint8_t base64_hmac_buf[128];
    az_span base64_hmac = AZ_SPAN_FROM_BUFFER(base64_hmac_buf);

    // Gọi hàm bổ trợ, truyền vào AZ_DEVICE_KEY từ #define
    res = sign_signature_base64(
        az_span_create_from_str(AZ_DEVICE_KEY),
        signature_span,
        base64_hmac,
        &base64_hmac
    );
    if (az_result_failed(res)) {
        ESP_LOGE(TAG, "Sign signature failed");
        return false;
    }

    /* 5. Tổng hợp thành Token hoàn chỉnh */
    size_t sas_len = 0;
    res = az_iot_hub_client_sas_get_password(
        &client,
        expiry,
        base64_hmac,
        AZ_SPAN_EMPTY,  
        az_sas_token,
        sizeof(az_sas_token),
        &sas_len
    );

    if (az_result_failed(res)) {
        ESP_LOGE(TAG, "Build SAS token failed");
        return false;
    }

    az_sas_token[sas_len] = '\0';
    ESP_LOGI(TAG, "New SAS Token generated successfully!");
    
    return true;
}


static void azure_sas_renew_task(void *arg)
{
    const int64_t MIN_VALID_TIMESTAMP = 1700000000; // Mốc thời gian hợp lệ (năm 2023) 
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); // Check mỗi 30s 
        int64_t now = (int64_t)time(NULL); 
        int64_t expiry;
        xSemaphoreTake(sas_mutex, portMAX_DELAY);
        expiry = sas_expiry_time;
        xSemaphoreGive(sas_mutex);
        if (expiry == 0) continue;
        if (now < MIN_VALID_TIMESTAMP) continue;

        // Nếu còn hơn 10 phút thì chưa cần làm gì
        if ((expiry - now) > SAS_RENEW_MARGIN) continue;

        ESP_LOGW(TAG, "SAS expiring soon, starting renewal process...");
 
            if (mqtt_handle != NULL) {
                 ESP_LOGI(TAG, "Đợi MQTT idle..."); 
                xEventGroupWaitBits(evt, EVT_MQTT_IDLE, pdFALSE, pdTRUE, portMAX_DELAY);
                // Đánh dấu renew
                xEventGroupClearBits(evt, EVT_MQTT_IDLE);
                xEventGroupSetBits(evt, EVT_MQTT_BUSY | EVT_SAS_RENEWING);
                // 1. Gọi hàm tạo của bạn để cập nhật mảng az_sas_token
                if (azure_build_sas()) {
                    
                    ESP_LOGI(TAG, "New SAS generated successfully");
                    // 2. Cập nhật cấu hình mới 
                    mqtt_cfg.credentials.username = az_username;  
                    mqtt_cfg.credentials.authentication.password = az_sas_token;  
                    esp_mqtt_set_config(mqtt_handle, &mqtt_cfg); 
                    esp_mqtt_client_reconnect(mqtt_handle);
                    
                    ESP_LOGI(TAG, "MQTT reconnected with new SAS Token");
                } else {
                    ESP_LOGE(TAG, "Critical: Could not generate new SAS Token");
                } 
                xEventGroupClearBits(evt,EVT_SAS_RENEWING); 
            } 
        // Sau khi renew xong, nghỉ 1 phút để tránh lặp lại logic ngay lập tức
        vTaskDelay(pdMS_TO_TICKS(60000)); 
    }
}


/* ================= MQTT EVENT ================= */

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED: 
        if (xEventGroupGetBits(evt) & EVT_SAS_RENEWING) { 
            ESP_LOGI(TAG, "MQTT reconnected after SAS renew"); 
        } else {
            ESP_LOGI(TAG, "MQTT connected");
            is_mqtt_connected = true; 
            xEventGroupClearBits(evt, EVT_MQTT_BUSY);
            xEventGroupSetBits(evt, EVT_MQTT_IDLE);
            esp_mqtt_client_subscribe(mqtt_handle, "$iothub/methods/POST/#", 1);
            update_sys_error( SYS_ERROR_MQTT, false, "MQTT Connected"); 
        }
        break;

    case MQTT_EVENT_DATA:
        azure_service_on_mqtt_message(
            event->topic, event->topic_len,
            event->data, event->data_len);
        break;

    case MQTT_EVENT_DISCONNECTED:
        is_mqtt_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected"); 
        if (!(xEventGroupGetBits(evt) & EVT_SAS_RENEWING)) { 
            update_sys_error(SYS_ERROR_MQTT, true, "MQTT Lost!");
        }

        break;

    default:
        break;
    }
}

/* ================= INIT ================= */

void azure_iot_init(void)
{   
    evt = xEventGroupCreate();
    assert(evt);
    xEventGroupSetBits(evt, EVT_MQTT_IDLE);
    xEventGroupClearBits(evt, EVT_MQTT_BUSY);
    xEventGroupClearBits(evt, EVT_SAS_RENEWING); 
    az_iot_hub_client_options options =
        az_iot_hub_client_options_default();
    // 1. Cập nhật API Version lên bản mới nhất ổn định                     
    options.user_agent = AZ_SPAN_EMPTY;
    az_result res = az_iot_hub_client_init(
        &client,
        az_span_create_from_str(AZ_HOST_NAME),
        az_span_create_from_str(AZ_DEVICE_ID),
        &options);

    if (az_result_failed(res)) {
        ESP_LOGE(TAG, "Azu client init failed");
        update_sys_error(SYS_ERROR_AZURE_CLIENT, true, "Azu client failed");
    }
     sas_mutex = xSemaphoreCreateMutex();
    assert(sas_mutex);
    ESP_LOGE(TAG, "Azu client inited");
    xTaskCreatePinnedToCore(  
        azure_sas_renew_task,
        "azure_sas_renew",
        4096,
        NULL,
        3,
        NULL,
        0); 
    update_sys_error(SYS_ERROR_AZURE_CLIENT, false, "");
}

void azure_iot_start(void)
{ 
    // 1. Tạo SAS Token (Password) - 
    if (mqtt_handle) return; 
    if (!azure_build_sas()) return; 
    // 3. Lấy Client ID
    static char mqtt_client_id[128]; 
    az_result res = az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id), NULL);
    if (az_result_failed(res)) {
        ESP_LOGE(TAG, "Failed to get client id");
        return;
    } 
 
    mqtt_cfg.credentials.client_id = mqtt_client_id;
    mqtt_cfg.credentials.username = az_username; // az_username Username thường cố định: Host/Device/?api-version...
    mqtt_cfg.credentials.authentication.password = az_sas_token;// az_sas_token; // DÙNG TOKEN VỪA TẠO 

    mqtt_handle = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(
        mqtt_handle, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);  // đăng ký nhận lệnh điều khiển
    esp_mqtt_client_start(mqtt_handle);
}
 
void azure_iot_publish_payload(const char *payload) {
    static az_result last_err = AZ_OK;
    char topic[256];
    if (!is_mqtt_connected || payload == NULL) return;
    // "$.ct=application%2Fjson&$.ce=utf-8" là từ khóa để Azure tự giải mã Base64
    const char* property_bag = "$.ct=application%2Fjson&$.ce=utf-8";
    // Tự lấy topic nội bộ 
    az_result res = az_iot_hub_client_telemetry_get_publish_topic(
        &client, 
        NULL, 
        topic, 
        sizeof(topic), 
        NULL);

    if (!az_result_succeeded(res)) {
        if (last_err != res) {
            ESP_LOGW(TAG,"Telemetry topic error changed: 0x%08x",res);
            last_err = res;
        }
        return;
    }
    last_err = AZ_OK; 
    // 2. Tính toán độ dài hiện tại của topic
    size_t current_len = strlen(topic);

    // 3. Sử dụng snprintf để nối chuỗi an toàn
    // Ta ghi đè từ vị trí topic + current_len
    int written = snprintf(topic + current_len, 
                            sizeof(topic) - current_len, 
                            "%s", 
                            property_bag);

    // 4. Kiểm tra xem việc nối chuỗi có bị cắt cụt (truncated) không
    if (written >= (sizeof(topic) - current_len)) {
        ESP_LOGE("AZURE", "Topic buffer quá nhỏ để chứa Metadata!");
         update_sys_error(SYS_ERROR_BUFFER, true, "buffer Full Metadata");
        return; 
    }
    // 5. Publish
    esp_mqtt_client_publish(mqtt_handle, topic, payload, 0, 1, 0);  
}
 
void azure_iot_publish_topic(const char *topic, const char *payload)
{

    if (!is_mqtt_connected) return;  
    xEventGroupWaitBits(
        evt,
        EVT_MQTT_IDLE,
        pdFALSE,
        pdTRUE,
        pdMS_TO_TICKS(1000)   );
    xEventGroupClearBits(evt, EVT_MQTT_IDLE);
    xEventGroupSetBits(evt, EVT_MQTT_BUSY);
    esp_mqtt_client_publish(
        mqtt_handle,
        topic,
        payload,
        0,
        1,
        0);   
    xEventGroupClearBits(evt, EVT_MQTT_BUSY);
    xEventGroupSetBits(evt, EVT_MQTT_IDLE);
    is_mqtt_connected = true; 
}

bool azure_iot_is_connected(void)
{
    return is_mqtt_connected;
} 


//  EventBits_t bits = xEventGroupGetBits(evt);
// if (bits & EVT_SAS_RENEWING) {
//     // đang renew SAS
// }