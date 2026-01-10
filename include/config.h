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
#define AZ_PASSWORD "<<PUT_YOUR_CONNECTION_STRING_HERE>>" // Thay
 
// Publish 1 message every 5 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 5000 

// WiFi credentials
#define WIFI_SSID "<PUT_YOUR_CONNECTION_STRING_HERE>" //Culun  
#define WIFI_PASS "<PUT_YOUR_CONNECTION_STRING_HERE>"  
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


//=================== led gipo
#define LED_BUILTIN    GPIO_NUM_2
#define SWITCH_GPIO    GPIO_NUM_16
#ifdef __cplusplus
}
#endif 
#endif // CONFIG_H