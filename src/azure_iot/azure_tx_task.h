#pragma once

#include <stdbool.h>

typedef struct {
    char topic[128];    // rỗng => telemetry
    char payload[512];
} azure_msg_t;

void azure_tx_task_start(void);

bool azure_tx_send_telemetry(const char *payload);
bool azure_tx_send_topic(const char *topic, const char *payload);
