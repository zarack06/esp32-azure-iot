# Xóa bỏ dòng FILE(GLOB_RECURSE...) để tránh xung đột

idf_component_register(
    SRCS 
        "main.cpp"
        "I2c/aht10.c"
        "fonts/font5x7.c"
        "Oled/oled.c"
        "I2c/Display.c"
        "Wifi/system_init.c"
        "schedule/device_config.c"
        "Wifi/wifi_manager.c"
        "azure_iot/azure_service.c"
        "schedule/schedule_config.c"
        "I2c/telemetry/telemetry_task.c"
        "azure_iot/azure_iot.c"
        "hardware_action/led.c"
        "sensor/sensor_task.c"
        "I2c/i2c_manager.c" 
        "helper/helper_time.c"
        "azure_iot/azure_tx_task.c"
        "azure_iot/ota/ota_manager.c"
        "azure_iot/azure_twin.c"
        "text/oled_text.c"
    INCLUDE_DIRS 
        "." "I2c" "sensor" "Oled" "helper" 
        "schedule" "azure_iot" "Wifi" "fonts" "text" "azure_iot/ota"
    EMBED_TXTFILES "azure_ca.pem"
    REQUIRES 
        freertos
)