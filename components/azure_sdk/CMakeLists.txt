# Tìm tất cả các file .c trong thư mục này
file(GLOB SOURCES "*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."  # Vì bạn để .h cùng cấp với .c nên dùng dấu chấm
)