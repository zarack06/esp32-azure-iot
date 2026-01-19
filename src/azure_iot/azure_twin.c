#include "azure_twin.h"
#include "device_config.h"
#include "schedule_config.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h" 
#include "azure_iot.h"
#include <string.h>
#include <stdio.h>
#include <app_version.h>
#include "azure_iot/ota/ota_manager.h"


#define TAG "AZURE_TWIN"

/* Azure IoT Hub twin topics */
#define TWIN_DESIRED_PREFIX "$iothub/twin/PATCH/properties/desired"
#define TWIN_RES_PREFIX     "$iothub/twin/res"
static uint16_t request_id = 100;
/* =========================================================
 * Internal helpers
 * ========================================================= */

static void handle_twin_desired(const char *payload, int len);
static void handle_twin_full(const char *payload, int len);
static void handle_twin_desired_json(const cJSON *root) ;
/* =========================================================
 * Public API
 * ========================================================= */

void azure_twin_on_mqtt_message(
    const char *topic, int topic_len,
    const char *payload, int payload_len)
{
    if (strncmp(topic,
                TWIN_DESIRED_PREFIX,
                strlen(TWIN_DESIRED_PREFIX)) == 0) {
        ESP_LOGI(TAG, "Twin desired update");
        handle_twin_desired(payload, payload_len);
        return;
    }

    if (strncmp(topic,
                TWIN_RES_PREFIX,
                strlen(TWIN_RES_PREFIX)) == 0) {
        ESP_LOGI(TAG, "Twin full response");
        handle_twin_full(payload, payload_len);
        return;
    }
}
static void handle_twin_desired(const char *payload, int len)
{   
    ESP_LOGI(TAG, "Handle twin desired: %.*s", len, payload);
    cJSON *root = cJSON_ParseWithLength(payload, len);
    if (!root) return;

    handle_twin_desired_json(root);

    cJSON_Delete(root);
}

/* =========================================================
 * Twin handlers
 * ========================================================= */

static void handle_twin_full(const char *payload, int len)
{
    /* Full twin trả về dạng:
     * {
     *   "properties": {
     *     "desired": { ... },
     *     "reported": { ... }
     *   }
     * }
     */ 
    ESP_LOGI(TAG, "Handle twin full: %.*s", len, payload);
    cJSON *root = cJSON_ParseWithLength(payload, len);
    if (!root) return;

    cJSON *desired = cJSON_GetObjectItem(root, "desired");
    ESP_LOGI(TAG, "Twin desired part: %s",
             desired ? cJSON_PrintUnformatted(desired) : "null");
    if (desired) {
        handle_twin_desired_json(desired);
    }

    cJSON_Delete(root);
}

static void ota_handle_from_twin(const cJSON *ota)
{
    const char *ver = cJSON_GetStringValue(
        cJSON_GetObjectItem(ota, "version")
    );
    const char *url = cJSON_GetStringValue(
        cJSON_GetObjectItem(ota, "url")
    );
    const char *sha = cJSON_GetStringValue(
        cJSON_GetObjectItem(ota, "sha256")
    );

    if (!ver || !url) return;

    if (!version_is_newer(ver, APP_VERSION)) {
    ESP_LOGI(TAG, "OTA skipped (current=%s, desired=%s)",
             APP_VERSION, ver);
    return;
   }
    ota_request_t req = {0};
    strncpy(req.version, ver, sizeof(req.version) - 1);
    strncpy(req.url, url, sizeof(req.url) - 1);

    if (sha)
        strncpy(req.sha256, sha, sizeof(req.sha256) - 1);

    ota_manager_request(&req);
}

static void handle_twin_desired_json(const cJSON *root)
{   
    const cJSON *ota = cJSON_GetObjectItem(root, "ota");
    if (cJSON_IsObject(ota)) {
        ota_handle_from_twin(ota);   // 👈 OTA   ở đây
    }
    /* ===== VERSION CHECK (GLOBAL) ===== */
    cJSON *ver = cJSON_GetObjectItem(root, "$version");
    uint32_t twin_version = cJSON_IsNumber(ver) ? ver->valueint : 0;  

    /* ================= CONFIG ================= */
    const cJSON *cfg_obj = cJSON_GetObjectItem(root, "config");
    if (cfg_obj) {
        device_config_t cfg = config_get();

        if (twin_version > cfg.version) {
            cfg.version = twin_version;

            cJSON *item;
            if ((item = cJSON_GetObjectItem(cfg_obj, "sampling_interval")))
                cfg.sampling_interval = item->valueint;

            if ((item = cJSON_GetObjectItem(cfg_obj, "temp_max")))
                cfg.temp_max = (float)item->valuedouble;

            if ((item = cJSON_GetObjectItem(cfg_obj, "auto_mode")))
                cfg.auto_mode = cJSON_IsTrue(item);

            if ((item = cJSON_GetObjectItem(cfg_obj, "pump")))
                cfg.pump = cJSON_IsTrue(item);
            ESP_LOGI(TAG, "Pump set to %s from Twin", cfg); //test
            config_set(&cfg);
        }
    }

    /* ================= SCHEDULE ================= */
    const cJSON *sch_obj = cJSON_GetObjectItem(root, "schedule");
    if (sch_obj && twin_version > 0) {
        device_schedule_t sch;
        if (schedule_parse_from_json_schedule(sch_obj, twin_version, &sch)) {
            schedule_set(&sch);
        }
    }

    /* ================= REPORT ================= */
    azure_twin_send_reported(); 
}


/* =========================================================
 * Reported properties
 * ========================================================= */
void azure_twin_send_reported(void)
{
    device_config_t cfg = config_get();
    device_schedule_t sch = schedule_get();

    /* ================= ROOT ================= */
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    /* ================= CONFIG ================= */  // test report config cần điều chỉnh ở đây
    cJSON *cfg_obj = cJSON_AddObjectToObject(root, "config");
    cJSON_AddNumberToObject(cfg_obj, "sampling_interval", cfg.sampling_interval);
    cJSON_AddNumberToObject(cfg_obj, "temp_max", cfg.temp_max);
    cJSON_AddBoolToObject(cfg_obj, "auto_mode", cfg.auto_mode);
    cJSON_AddBoolToObject(cfg_obj, "pump", cfg.pump);
    cJSON_AddBoolToObject(cfg_obj, "heater", cfg.heater);

    /* ================= SCHEDULE ================= */
    cJSON *sch_obj = cJSON_AddObjectToObject(root, "schedule");
    cJSON_AddBoolToObject(sch_obj, "enabled", sch.enabled); 
    cJSON_AddNumberToObject(sch_obj, "version", sch.version);

    cJSON *tasks = cJSON_AddArrayToObject(sch_obj, "tasks");
    for (int i = 0; i < sch.task_count && i < MAX_SCHEDULE_TASKS; i++) {
        cJSON *t = cJSON_CreateObject();
        cJSON_AddNumberToObject(t, "id", sch.tasks[i].id);
        char start_str[9]; // "hh:mm" cần 5 ký tự + '\0' nếu ép kiểu
        snprintf(start_str, sizeof(start_str), "%02d:%02d", sch.tasks[i].hour, sch.tasks[i].minute);
        cJSON_AddStringToObject(t, "start", start_str);
        cJSON_AddNumberToObject(t, "duration_min", sch.tasks[i].duration_min);
        cJSON_AddItemToArray(tasks, t);
    }

    /* ================= META ================= */
    cJSON *meta = cJSON_AddObjectToObject(root, "meta");
    cJSON_AddNumberToObject(meta, "config_version", cfg.version);
    cJSON_AddNumberToObject(meta, "schedule_version", sch.version);

    /* ================= SERIALIZE ================= */
    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root); // Delete sớm để giải phóng RAM cho payload string

    if (!payload) return;

    /* ================= MQTT PUBLISH (Sửa Topic) ================= */ 
    char topic[128];
    snprintf(topic, sizeof(topic), "$iothub/twin/PATCH/properties/reported/?$rid=%d", request_id++);

    int msg_id = esp_mqtt_client_publish(
        get_mqtt(),
        topic,      // Sử dụng topic có $rid
        payload,
        0,          // Độ dài tự tính toán từ payload (nếu payload là null-terminated)
        1,          // QoS 1 là bắt buộc cho Twin để đảm bảo không mất dữ liệu
        0
    );

    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish Twin report");
    } else {
        ESP_LOGI(TAG, "Twin reported sent [RID:%d]: %s", request_id-1, payload);
    }

    /* ================= CLEANUP ================= */
    free(payload); // Dùng free() chuẩn của hệ thống cho chuỗi cJSON_Print
}

bool schedule_parse_from_json(
    const cJSON *sch_obj,
    uint32_t version,
    device_schedule_t *out)
{
    if (!cJSON_IsObject(sch_obj)) return false;

    out->version = version;

    cJSON *enabled = cJSON_GetObjectItem(sch_obj, "enabled");
    out->enabled = cJSON_IsTrue(enabled);

    cJSON *tasks = cJSON_GetObjectItem(sch_obj, "tasks");
    if (!cJSON_IsArray(tasks)) return false;

    int count = cJSON_GetArraySize(tasks);
    if (count > MAX_SCHEDULE_TASKS)
        count = MAX_SCHEDULE_TASKS;

    out->task_count = count;

    for (int i = 0; i < count; i++) {
        cJSON *t = cJSON_GetArrayItem(tasks, i);
        if (!cJSON_IsObject(t)) continue;

        schedule_task_t *dst = &out->tasks[i];

        cJSON *id = cJSON_GetObjectItem(t, "id");
        if (cJSON_IsNumber(id)) dst->id = id->valueint;
        dst->duration_min =
            cJSON_GetObjectItem(t, "duration_min")->valueint;

        /* parse "HH:MM" */
        const char *start =
            cJSON_GetObjectItem(t, "start")->valuestring;

        int h = 0, m = 0;
        if (sscanf(start, "%d:%d", &h, &m) == 2) {
            dst->hour = h;
            dst->minute = m;
        }
    }

    return true;
}
