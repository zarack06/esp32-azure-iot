#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "mqtt_client.h"
#include "esp_tls.h"

#include "..\include\config.h"
static char *url_encode(const char *str)
{
    const char *hex = "0123456789ABCDEF";
    char *enc = malloc(strlen(str) * 3 + 1);
    char *p = enc;

    while (*str)
    {
        if (('a' <= *str && *str <= 'z') ||
            ('A' <= *str && *str <= 'Z') ||
            ('0' <= *str && *str <= '9') ||
            (*str == '-' || *str == '_' || *str == '.' || *str == '~'))
        {
            *p++ = *str;
        }
        else
        {
            *p++ = '%';
            *p++ = hex[(*str >> 4) & 0xF];
            *p++ = hex[*str & 0xF];
        }
        str++;
    }
    *p = '\0';
    return enc;
}
char *generate_sas_token(
    const char *host,
    const char *device_id,
    const char *device_key,
    int expiry_minutes)
{
    /* 1. expiry time */
    time_t now;
    time(&now);
    long expiry = now + expiry_minutes * 60;

    /* 2. resource URI */
    char resource[256];
    snprintf(resource, sizeof(resource),
             "%s/devices/%s", host, device_id);

    char expiry_str[16];
    snprintf(expiry_str, sizeof(expiry_str), "%ld", expiry);

    /* 3. string to sign */
    char string_to_sign[512];
    snprintf(string_to_sign, sizeof(string_to_sign),
             "%s\n%s", resource, expiry_str);

    /* 4. decode device key */
    uint8_t key_bin[64];
    size_t key_len;

    mbedtls_base64_decode(
        key_bin, sizeof(key_bin),
        &key_len,
        (const unsigned char *)device_key,
        strlen(device_key));

    /* 5. HMAC-SHA256 */
    uint8_t hmac[32];
    const mbedtls_md_info_t *md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    mbedtls_md_hmac(
        md_info,
        key_bin, key_len,
        (const unsigned char *)string_to_sign,
        strlen(string_to_sign),
        hmac);

    /* 6. base64 encode signature */
    unsigned char sig_base64[64];
    size_t sig_len;

    mbedtls_base64_encode(
        sig_base64, sizeof(sig_base64),
        &sig_len,
        hmac, sizeof(hmac));

    sig_base64[sig_len] = '\0';

    /* 7. URL encode */
    char *enc_sig = url_encode((char *)sig_base64);
    char *enc_res = url_encode(resource);

    /* 8. build final SAS */
    char *sas = malloc(512);
    snprintf(sas, 512,
             "SharedAccessSignature sr=%s&sig=%s&se=%s",
             enc_res, enc_sig, expiry_str);

    free(enc_sig);
    free(enc_res);

    return sas;
}
// Example usage    

// esp_mqtt_client_config_t mqtt_cfg = {
//     .broker.address.uri =
//         "mqtts://Esp32HnTemp.azure-devices.net:8883",

//     .credentials.username =
//         "Esp32HnTemp.azure-devices.net/ESP32_WaterTemp_01/?api-version=2021-04-12",

//     .credentials.authentication.password = sas_token,

//     .network.crt_bundle_attach = esp_crt_bundle_attach,
// };
