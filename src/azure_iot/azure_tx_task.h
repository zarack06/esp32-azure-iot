#pragma once

#include <stdbool.h>

typedef enum {
    AZURE_MSG_TELEMETRY,
    AZURE_MSG_TOPIC
} azure_msg_type_t;

typedef struct {
    azure_msg_type_t type;
    char topic[128];
    char payload[256];
} azure_msg_t;

void azure_tx_task_start(void);

bool azure_tx_send_telemetry(const char *payload);
bool azure_tx_send_topic(const char *topic, const char *payload);
