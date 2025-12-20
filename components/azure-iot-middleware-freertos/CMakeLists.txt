# Tự động tìm tất cả các file .c trong thư viện
file(GLOB_RECURSE SOURCES 
    "libraries/azure-sdk-for-c/sdk/src/*.c"
    "source/*.c"
)

# Loại bỏ các file test để tránh lỗi trùng lặp
list(FILTER SOURCES EXCLUDE REGEX "test")

idf_component_register(
    SRCS "${SOURCES}"
    INCLUDE_DIRS 
        "libraries/azure-sdk-for-c/sdk/inc"
        "libraries/azure-sdk-for-c/sdk/inc/azure/iot"
        "libraries/freeRTOS-addons/include"
        "source/include"
        "source/interface"  # Thêm mục này vì thấy trong hình bạn có thư mục interface
        "ports/common/include"
    REQUIRES esp-tls esp_timer nvs_flash esp_event esp_netif mqtt
)