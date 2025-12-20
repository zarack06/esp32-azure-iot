#include "azure_iot.h"

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h> 

#include "esp_log.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "..\include\config.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "esp_crt_bundle.h" 
// extern const uint8_t _binary_azure_ca_pem_start[];
// extern const uint8_t _binary_azure_ca_pem_end[];
extern const uint8_t _src_certs_azure_ca_pem_start[];
extern const uint8_t _src_certs_azure_ca_pem_end[];

static const char *TAG = "AZURE_IOT";

static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool azure_connected = false;

static char hub_host[128];
static char device_id[64];
static char shared_access_key[128];

static char mqtt_username[256];
static char mqtt_password[768];
static char mqtt_uri[256];

static void parse_connection_string(const char *conn)
{
    // HostName=xxx;DeviceId=yyy;SharedAccessKey=zzz
    sscanf(conn,
           "HostName=%127[^;];DeviceId=%63[^;];SharedAccessKey=%127s",
           hub_host, device_id, shared_access_key);
}
 

static void url_encode(const char *src, char *dst, size_t dst_len)
{
    const char *hex = "0123456789ABCDEF";
    size_t pos = 0;

    while (*src && pos + 3 < dst_len) {
        if (isalnum((unsigned char)*src) || *src == '-' || *src == '_' ||
            *src == '.' || *src == '~') {
            dst[pos++] = *src;
        } else {
            dst[pos++] = '%';
            dst[pos++] = hex[*src >> 4];
            dst[pos++] = hex[*src & 15];
        }
        src++;
    }
    dst[pos] = '\0';
}
static void generate_sas_token(void)
{
    char resource[256];
    char resource_enc[256];
    char string_to_sign[512];
    unsigned char hmac[32]; 

    time_t now;
    time(&now);
    int expiry = now + 3600; // token sống 1 giờ

    snprintf(resource, sizeof(resource),
             "%s/devices/%s", hub_host, device_id);

    url_encode(resource, resource_enc, sizeof(resource_enc));

    snprintf(string_to_sign, sizeof(string_to_sign),
             "%s\n%d", resource_enc, expiry);

    // Decode key
    unsigned char key_bin[64];
    size_t key_len;
    mbedtls_base64_decode(key_bin, sizeof(key_bin),
                          &key_len,
                          (const unsigned char *)shared_access_key,
                          strlen(shared_access_key));

    // HMAC-SHA256
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx,
                     mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                     1);

    mbedtls_md_hmac_starts(&ctx, key_bin, key_len);
    mbedtls_md_hmac_update(&ctx,
                           (const unsigned char *)string_to_sign,
                           strlen(string_to_sign));
    mbedtls_md_hmac_finish(&ctx, hmac);
    mbedtls_md_free(&ctx);

    // Base64 encode signature
    unsigned char sig_b64[128];
    size_t sig_len;
    mbedtls_base64_encode(sig_b64, sizeof(sig_b64),
                          &sig_len, hmac, 32);

    char sig_enc[256];
    url_encode((char *)sig_b64, sig_enc, sizeof(sig_enc));

    snprintf(mqtt_password, sizeof(mqtt_password),
             "SharedAccessSignature sr=%s&sig=%s&se=%d",
             resource_enc, sig_enc, expiry);
}
static void mqtt_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    if (event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Connected to Azure IoT Hub");
        azure_connected = true;
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        ESP_LOGW(TAG, "Disconnected from Azure IoT Hub");
        azure_connected = false;
    }
}
esp_err_t azure_iot_connect(void)
{
    parse_connection_string(AZURE_IOT_HUB_CONNECTION_STRING);
    generate_sas_token();

    snprintf(mqtt_uri, sizeof(mqtt_uri),
             "mqtts://%s:8883", hub_host);

    snprintf(mqtt_username, sizeof(mqtt_username),
             "%s/%s/?api-version=2021-04-12",
             hub_host, device_id);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = mqtt_uri,
        .credentials.username = mqtt_username,
        .credentials.authentication.password = mqtt_password,
        .broker.verification.certificate =
            (const char *)_src_certs_azure_ca_pem_start,
    };

    mqtt_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(
        mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL); 
    return esp_mqtt_client_start(mqtt_client);
}
esp_err_t azure_iot_send_telemetry(float temperature, float humidity)
{
    if (!azure_connected) return ESP_FAIL;

    char topic[128];
    snprintf(topic, sizeof(topic),
             "devices/%s/messages/events/", device_id);

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"temperature\":%.2f,\"humidity\":%.2f}",
             temperature, humidity);

    return esp_mqtt_client_publish(
        mqtt_client, topic, payload, 0, 1, 0);
}
esp_err_t azure_iot_disconnect(void)
{
    if (!mqtt_client) return ESP_OK;

    esp_mqtt_client_stop(mqtt_client);
    esp_mqtt_client_destroy(mqtt_client);

    mqtt_client = NULL;
    azure_connected = false;
    return ESP_OK;
}
bool azure_iot_is_connected(void)
{
    return azure_connected;
}
