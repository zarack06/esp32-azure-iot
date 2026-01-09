#include "azure_tx_task.h"
#include "azure_iot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include "azure_iot_internal.h"
#include "helper_time.h"
#define TAG "AZURE_TX"
#define AZURE_TX_TASK_STACK_SIZE 8192
#define AZURE_TX_TASK_PRIORITY   5
static bool azure_tx_send(const azure_msg_t *msg);

static QueueHandle_t azure_tx_queue = NULL;

/* =========================
 * Azure TX Task
 * ========================= */
static void azure_tx_task(void *pv)
{
    azure_msg_t msg; 
    static bool last_azure_state = true;
    while (1)
    {
        if (xQueueReceive(azure_tx_queue, &msg, portMAX_DELAY) == pdPASS)
        {
            bool connected = azure_iot_is_connected();
            if (!connected)
            {
                if (last_azure_state) {
                     update_sys_error(SYS_ERROR_AZURE, true, "Azure disconnected");
                }
                last_azure_state = false;
                continue;
            }
            last_azure_state = true;

            switch (msg.type) {
            case AZURE_MSG_TOPIC:
                azure_iot_publish_topic(msg.topic, msg.payload);
                break;

            case AZURE_MSG_TELEMETRY:
                azure_iot_publish_payload(msg.payload);
                break;

            default:
                ESP_LOGW(TAG, "Unknown msg type");
                break;
            }
        } 
    }
} 

/**
 * @brief Khởi tạo Azure TX task
 */
void azure_tx_task_start(void)
{
    if (azure_tx_queue != NULL)
    {
        ESP_LOGW(TAG, "Azure TX already started"); 
        return;
    }

    azure_tx_queue = xQueueCreate(10, sizeof(azure_msg_t));
    if (!azure_tx_queue)
    {
        ESP_LOGE(TAG, "Failed to create azure_tx_queue"); 
        return;
    }

    xTaskCreate(
        azure_tx_task,
        "azure_tx",
        AZURE_TX_TASK_STACK_SIZE,
        NULL,
        AZURE_TX_TASK_PRIORITY,
        NULL
    );
}

/**
 * @brief Gửi telemetry  
 */
bool azure_tx_send_telemetry(const char *payload)
{
    if (!azure_tx_queue || !payload) return false;

    azure_msg_t msg = {0};
    msg.type = AZURE_MSG_TELEMETRY;
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);

    return azure_tx_send(&msg);
}
/**
 * @brief Gửi message custom topic 
 */
bool azure_tx_send_topic(const char *topic, const char *payload)
{
    if (!azure_tx_queue || !topic || !payload) return false;
    
    azure_msg_t msg = {0};
    msg.type = AZURE_MSG_TOPIC; 
    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
    return azure_tx_send(&msg);
}
static bool azure_tx_send(const azure_msg_t *msg)
{
    if (!azure_tx_queue || !msg) return false; 
    if (xQueueSend(azure_tx_queue, msg, 0) != pdPASS) {
        update_sys_error(SYS_ERROR_TX_QUEUE, true, "Azure TX queue full"); 
        ESP_LOGW(TAG, "azure_tx_queue,  msg  =%s", msg->payload);
        return false;
    }
    update_sys_error(SYS_ERROR_TX_QUEUE, false,  "");
    return true;
}