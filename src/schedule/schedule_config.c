#include "schedule_config.h" 
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h> 

#define TAG "SCHEDULE_CFG"
#define NVS_NS_SCHEDULE "schedule"

/* ===== Internal storage ===== */
static device_schedule_t g_schedule;

/* ===== Default schedule ===== */
static void schedule_load_default(void)
{
    memset(&g_schedule, 0, sizeof(g_schedule));

    g_schedule.version = 0;
    g_schedule.enabled = true;
    g_schedule.task_count = 2;

    g_schedule.tasks[0].id = 1;
    g_schedule.tasks[0].hour = 8;
    g_schedule.tasks[0].minute = 0;
    g_schedule.tasks[0].duration_min = 5;
    g_schedule.tasks[1].id = 2;
    g_schedule.tasks[1].hour = 17;
    g_schedule.tasks[1].minute = 0;
    g_schedule.tasks[1].duration_min = 5;
}

/* =========================================================
 * Init / Load
 * ========================================================= */

void schedule_init(void)
{
    nvs_handle_t nvs;
    size_t size = sizeof(g_schedule);

    if (nvs_open(NVS_NS_SCHEDULE, NVS_READONLY, &nvs) == ESP_OK) {
        if (nvs_get_blob(nvs, "sch", &g_schedule, &size) != ESP_OK) {
            ESP_LOGW(TAG, "No schedule in NVS, loading default");
            schedule_load_default();
        }
        nvs_close(nvs);
    } else {
        ESP_LOGW(TAG, "Cannot open NVS, loading default schedule");
        schedule_load_default();
    }
}

/* =========================================================
 * Getter / Setter
 * ========================================================= */

device_schedule_t schedule_get(void)
{
    return g_schedule;
}

void schedule_set(const device_schedule_t *sch)
{
    if (!sch) return;

    /* Không đổi → bỏ qua */
    if (memcmp(&g_schedule, sch, sizeof(device_schedule_t)) == 0) {
        return;
    }

    g_schedule = *sch;

    nvs_handle_t nvs;
    if (nvs_open(NVS_NS_SCHEDULE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_blob(nvs, "sch", sch, sizeof(*sch));
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "Schedule saved (v=%lu, tasks=%d)",
                 sch->version, sch->task_count);
    } else {
        ESP_LOGE(TAG, "Failed to open NVS for schedule write");
    }
}

/* =========================================================
 * JSON helpers
 * ========================================================= */

static bool parse_time_hhmm(const char *str, uint8_t *h, uint8_t *m)
{
    if (!str || strlen(str) != 5 || str[2] != ':')
        return false;

    *h = (str[0] - '0') * 10 + (str[1] - '0');
    *m = (str[3] - '0') * 10 + (str[4] - '0');

    if (*h > 23 || *m > 59)
        return false;

    return true;
}

/* =========================================================
 * Parse Twin Desired → Schedule
 * ========================================================= */

bool schedule_parse_from_json_schedule(
    const cJSON *json,
    uint32_t twin_version,
    device_schedule_t *out)
{
    if (!json || !out)
        return false;

    device_schedule_t cur = schedule_get();

    /* Version check */
    if (twin_version <= cur.version) {
        ESP_LOGI(TAG, "Schedule version old (%lu <= %lu), skip",
                 twin_version, cur.version);
        return false;
    }

    device_schedule_t sch;
    memset(&sch, 0, sizeof(sch));

    sch.version = twin_version;

    /* enabled */
    const cJSON *enabled = cJSON_GetObjectItem(json, "enabled");
    sch.enabled = enabled ? cJSON_IsTrue(enabled) : true;

    /* tasks */
    const cJSON *tasks = cJSON_GetObjectItem(json, "tasks");
    if (tasks && cJSON_IsArray(tasks)) {

        int count = cJSON_GetArraySize(tasks);
        if (count > MAX_SCHEDULE_TASKS)
            count = MAX_SCHEDULE_TASKS;

        for (int i = 0; i < count; i++) {
            const cJSON *t = cJSON_GetArrayItem(tasks, i);
            if (!cJSON_IsObject(t))
                continue;

            schedule_task_t *task = &sch.tasks[sch.task_count];

            const cJSON *id = cJSON_GetObjectItem(t, "id");
            const cJSON *start = cJSON_GetObjectItem(t, "start");
            const cJSON *dur = cJSON_GetObjectItem(t, "duration_min");

            if (!cJSON_IsNumber(id) ||
                !cJSON_IsString(start) ||
                !cJSON_IsNumber(dur))
                continue;

            uint8_t h, m;
            if (!parse_time_hhmm(start->valuestring, &h, &m))
                continue;

            task->id = (uint8_t)id->valueint;
            task->hour = h;
            task->minute = m;
            task->duration_min = (uint16_t)dur->valueint;

            sch.task_count++;
        }
    }

    *out = sch;

    ESP_LOGI(TAG,
        "Parsed schedule v=%lu enabled=%d tasks=%d",
        sch.version, sch.enabled, sch.task_count);

    return true;
}
