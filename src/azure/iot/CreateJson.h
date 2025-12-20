#include "cJSON.h"
// #include "azure_iot_hub_client.h"

// Hàm tạo chuỗi JSON và gửi
// void send_telemetry_data(AzureIoTHubClient_t *xAzureIoTHubClient) {
//     // 1. Tạo đối tượng JSON
//     cJSON *root = cJSON_CreateObject();
//     cJSON_AddNumberToObject(root, "temperature", 28.5);
//     cJSON_AddNumberToObject(root, "humidity", 65.0);
//     cJSON_AddStringToObject(root, "status", "normal");

//     // 2. Chuyển sang dạng chuỗi text
//     char *json_string = cJSON_PrintUnformatted(root);

//     // 3. Gửi lên Azure IoT Hub
//     AzureIoTHubClient_SendTelemetry(xAzureIoTHubClient,
//                                     (uint8_t *)json_string,
//                                     strlen(json_string),
//                                     NULL,
//                                     eAzureIoTHubMessageQoS1,
//                                     NULL);

//     printf("Đã gửi JSON: %s\n", json_string);

//     // 4. Giải phóng bộ nhớ
//     cJSON_free(json_string);
//     cJSON_Delete(root);
// }