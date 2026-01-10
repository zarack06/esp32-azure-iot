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
#define TAG "AZURE_IOT"

static az_iot_hub_client client;
static esp_mqtt_client_handle_t mqtt_handle;
static bool is_mqtt_connected = false;
static char az_username[128] = {0};
static char az_sas_token[512]; // Tăng kích thước buffer cho an toàn
extern const uint8_t azure_ca_pem_start[] asm("_binary_azure_ca_pem_start");
extern const uint8_t azure_ca_pem_end[]   asm("_binary_azure_ca_pem_end");
static volatile bool is_sas_renewing = false;

#define SAS_VALID_SECONDS      (60 * 60)      // 1 giờ
#define SAS_RENEW_MARGIN       (10 * 60)      // renew trước 10 phút

static int64_t sas_expiry_time = 0;
static SemaphoreHandle_t sas_mutex;

// Hàm bổ trợ để ký HMAC-SHA256 (Azure yêu cầu bước này)
static az_result sign_signature(az_span signature, az_span device_key, az_span base64_hmac_sha256_signature) {
    uint8_t decoded_key[64];
    size_t decoded_len;
    
    // Decode key từ Base64 config
    if (mbedtls_base64_decode(decoded_key, sizeof(decoded_key), &decoded_len, 
                             az_span_ptr(device_key), az_span_size(device_key)) != 0) {
        return AZ_ERROR_ARG;
    }

    uint8_t hmac_res[32];
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info) {
        ESP_LOGE(TAG, "AZ_ERROR_UNEXPECTED_CHAR =");
            update_sys_error( SYS_ERROR_MQTT, false, "AZ_ERROR_UNEXPECTED_CHAR");
        return AZ_ERROR_UNEXPECTED_CHAR;
    }

    mbedtls_md_hmac_starts(&ctx, decoded_key, decoded_len);
    mbedtls_md_hmac_update(&ctx, az_span_ptr(signature), az_span_size(signature));
    mbedtls_md_hmac_finish(&ctx, hmac_res);
    mbedtls_md_free(&ctx);

    // Encode kết quả HMAC sang Base64
    size_t out_len; 
    int ret = mbedtls_base64_encode(
    az_span_ptr(base64_hmac_sha256_signature),
    az_span_size(base64_hmac_sha256_signature),
    &out_len,
    hmac_res,
    sizeof(hmac_res));
if (ret == 0) {
        // CHÈN KÝ TỰ KẾT THÚC CHUỖI VÀO ĐÚNG VỊ TRÍ
        az_span_ptr(base64_hmac_sha256_signature)[out_len] = '\0';
    };
    memset(decoded_key, 0, sizeof(decoded_key)); 
    return (ret == 0) ? AZ_OK : AZ_ERROR_NOT_ENOUGH_SPACE; 
}

static bool azure_build_sas(void)
{
    az_result res;
    // 1. Lấy Username nếu chưa có
    if (az_username[0] == 0) {
        res = az_iot_hub_client_get_user_name(&client, az_username, sizeof(az_username), NULL); 
        if (az_result_failed(res)) return false;
    }

    // 2. Tính thời gian hết hạn
    uint32_t expiry = (uint32_t)helper_time_get_unix() + SAS_VALID_SECONDS;
    sas_expiry_time = (int64_t)expiry; 
    // 3. Lấy Signature string
    uint8_t signature_buf[256];
    az_span signature_span = az_span_create(signature_buf, sizeof(signature_buf));
    res = az_iot_hub_client_sas_get_signature(&client, expiry, signature_span, &signature_span);
    if (az_result_failed(res)) return false;

    // 4. Ký HMAC-SHA256 cho Signature đó
    uint8_t base64_hmac_buf[128];
    memset(base64_hmac_buf, 0, sizeof(base64_hmac_buf));
    az_span base64_hmac_span = az_span_create(base64_hmac_buf, sizeof(base64_hmac_buf));   //sizeof(base64_hmac_buf) chưa up

    if (az_result_failed(sign_signature(signature_span, az_span_create_from_str(AZ_DEVICE_KEY), base64_hmac_span))) {
        return false;
    }
    az_span hmac_res_span = az_span_create_from_str((char*)base64_hmac_buf);
    // 5. Lấy Password cuối cùng (ĐÂY LÀ CHỖ BẠN BỊ LỖI BIÊN DỊCH)
    // Cấu trúc đúng: client, expiry, hmac_span, key_name_span, output_buf, output_size, out_len
    size_t sas_len = 0;
    res = az_iot_hub_client_sas_get_password(
        &client,
        expiry,
        hmac_res_span,
        AZ_SPAN_EMPTY, // key_name thường để trống khi dùng Device Key trực tiếp
        az_sas_token,
        sizeof(az_sas_token),
        &sas_len);
        if (az_result_succeeded(res) && sas_len < sizeof(az_sas_token)) {
            az_sas_token[sas_len] = '\0';
        } 
    return az_result_succeeded(res);
}

static void azure_sas_renew_task(void *arg)
{
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); // check mỗi 30s
        if (!is_mqtt_connected) {
            ESP_LOGW(TAG, "MQTT not connected, skip SAS renew"); 
            continue;
        }

        int64_t now = helper_time_get_unix();
            ESP_LOGW(TAG, "now = %ld , sas_expiry_time = %ld ", now, sas_expiry_time);
        if (sas_expiry_time == 0) continue;
        if (now < 1700000000) { // ~2023
            ESP_LOGE(TAG, "System time not synced");
            update_sys_error( SYS_ERROR_MQTT, false, "Not syn time");
        continue;
}
        if ((sas_expiry_time - now) > SAS_RENEW_MARGIN)
            continue;

        ESP_LOGW(TAG, "SAS expiring, renewing...");

        if (xSemaphoreTake(sas_mutex, pdMS_TO_TICKS(1000))) {
            if (!mqtt_handle) {    xSemaphoreGive(sas_mutex);    continue; }
            esp_mqtt_client_stop(mqtt_handle);

            if (azure_build_sas()) {
                ESP_LOGI(TAG, "SAS renewed");
            } else {
                ESP_LOGE(TAG, "SAS renew failed");
            }

            esp_mqtt_client_start(mqtt_handle);

            xSemaphoreGive(sas_mutex);
        }
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
        ESP_LOGI(TAG, "MQTT connected");
        is_mqtt_connected = true;
        esp_mqtt_client_subscribe(mqtt_handle,
            "$iothub/methods/POST/#", 1);
        update_sys_error( SYS_ERROR_MQTT, false, "MQTT Connected"); 
        
        break;

    case MQTT_EVENT_DATA:
        azure_service_on_mqtt_message(
            event->topic, event->topic_len,
            event->data, event->data_len);
        break;

    case MQTT_EVENT_DISCONNECTED:
        is_mqtt_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        update_sys_error(SYS_ERROR_MQTT, true, "MQTT Lost!");
        break;

    default:
        break;
    }
}

/* ================= INIT ================= */

void azure_iot_init(void)
{
    az_iot_hub_client_options options =
        az_iot_hub_client_options_default();

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
    // // 2. Tự động tạo Username từ Client SDK
    // char mqtt_username[128];
    // az_result res = az_iot_hub_client_get_user_name(
    //     &client, 
    //     mqtt_username, 
    //     sizeof(mqtt_username), 
    //     NULL);

    // if (az_result_failed(res)) {
    //     ESP_LOGE(TAG, "Failed to get MQTT username");
    //     return;
    // }
    // 3. Lấy Client ID
    char mqtt_client_id[128]; 
    az_result res = az_iot_hub_client_get_client_id(&client, mqtt_client_id, sizeof(mqtt_client_id), NULL);
    if (az_result_failed(res)) {
        ESP_LOGE("AZURE", "Failed to get client id");
        return;
    }
    esp_mqtt_client_config_t cfg = {}; 

    cfg.broker.address.uri = "mqtts://" AZ_HOST_NAME;
    cfg.credentials.client_id = mqtt_client_id;
    cfg.credentials.username = az_username; // Username thường cố định: Host/Device/?api-version...
    cfg.credentials.authentication.password = az_sas_token; // DÙNG TOKEN VỪA TẠO
    cfg.broker.verification.certificate = (const char *)azure_ca_pem_start;


    mqtt_handle = esp_mqtt_client_init(&cfg);
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
    
    esp_mqtt_client_publish(
        mqtt_handle,
        topic,
        payload,
        0,
        1,
        0);
}

bool azure_iot_is_connected(void)
{
    return is_mqtt_connected;
} 


 