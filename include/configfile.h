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
"Your AZURE_CA_PEM\n" \
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