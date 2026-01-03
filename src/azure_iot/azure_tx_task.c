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

static QueueHandle_t azure_tx_queue = NULL;

/* =========================
 * Azure TX Task
 * ========================= */
static void azure_tx_task(void *pv)
{
    azure_msg_t msg;

    ESP_LOGI(TAG, "Azure TX task started");

    while (1)
    {
        if (xQueueReceive(azure_tx_queue, &msg, portMAX_DELAY) == pdPASS)
        {
            if (!azure_iot_is_connected())
            {
                ESP_LOGW(TAG, "Azure not connected, drop message");
                set_last_error("Azure dis, drop mes");
                continue;
            }

            //có topic -> publish theo topic
             
            if (strlen(msg.topic) > 0)
            {
                azure_iot_publish_topic(
                    msg.topic,
                    msg.payload
                );

                ESP_LOGI(TAG, "Published to topic: %s", msg.topic);
            }//không -> coi là telemetry thường 
            else
            {
                azure_iot_publish_payload(msg.payload);
                ESP_LOGI(TAG, "Telemetry sent");
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
        8192,
        NULL,
        5,
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
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);

    return xQueueSend(azure_tx_queue, &msg, 0) == pdPASS;
}

/**
 * @brief Gửi message custom topic 
 */
bool azure_tx_send_topic(const char *topic, const char *payload)
{
    if (!azure_tx_queue || !topic || !payload) return false;

    azure_msg_t msg = {0};
    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    strncpy(msg.payload, payload, sizeof(msg.payload) - 1);

    return xQueueSend(azure_tx_queue, &msg, 0) == pdPASS;
}
