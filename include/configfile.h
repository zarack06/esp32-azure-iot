#ifndef CONFIG_H
#define CONFIG_H 
 
#ifdef __cplusplus
extern "C" {
#endif 
// Azure IoT Hub 
#define IOTHUB_HOST      "<PUT_YOUR_IOTHUB_HOST>" 
/* Tách từ Connection String của bạn */
#define AZ_HOST_NAME "<PUT_YOUR_HOST_NAME_HERE>"
#define AZ_DEVICE_ID "<PUT_YOUR_DEVICE_ID_HERE>"
#define AZ_DEVICE_KEY "<PUT_YOURKEY?"
#define AZ_PASSWORD "<PUT_YOUR_AZ_PASSWORD_STRING_HERE>" // Thay
// Chứng chỉ DigiCert Global Root G2 (Azure sử dụng cert này)
#define AZ_USERNAME "Esp32.2" // Thay bằng Username bạn đã tạo

#define AZURE_CA_PEM \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n" \
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n" \
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n" \
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n" \
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n" \
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n" \
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n" \
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n" \
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n" \
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n" \
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n" \
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n" \
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n" \
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n" \
"MrY=\n" \
"-----END CERTIFICATE-----\n" 
// Enter Your Wifi and Password
// #define IOT_CONFIG_WIFI_SSID "Maya"
// #define IOT_CONFIG_WIFI_PASSWORD "nguyenanhtien@199"

// #define WIFI_SSID "Culun"
// #define WIFI_PASS "61a04592"

// Publish 1 message every 5 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 5000 



// WiFi credentials
// #define WIFI_SSID "Culun"  
// #define WIFI_PASS "61a04592"   
#define WIFI_SSID "Maya" //Culun  
#define WIFI_PASS "nguyenanhtien@199"  
// WiFi security type
// #define WIFI_SECURITY   WIFI_AUTH_WPA2_PSK
/* I2C config */
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_SDA_IO   GPIO_NUM_21
#define I2C_MASTER_SCL_IO   GPIO_NUM_22
#define I2C_MASTER_FREQ_HZ  100000
#define OLED_ADDR           0x3C

/* AHT10 I2C address */
#define AHT10_I2C_ADDRESS 0x38


//===================


#ifdef __cplusplus
}
#endif 
#endif // CONFIG_H