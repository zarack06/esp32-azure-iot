# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS "main.cpp"
                        "I2c/aht10.c"
                        "fonts/font5x7.c"
                        "Oled/oled.c"
                        "I2c/Display.c"
                        "system_init.c"
                        "Wifi/wifi_manager.c"
                        "azure_iot\azure_service.c"
                        "text/oled_text.c"  # Thêm file .c chứa hàm oled_put_string
                    INCLUDE_DIRS "."
                                 "I2c" 
                                 "Oled"
                                 "azure_iot"
                                 "Wifi"
                                 "fonts"
                                 "text"
                    REQUIRES freertos) # Đảm bảo có freertos ở đây 
