
#include <stdbool.h>

#define MAX_TASKS 5
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int id;
    int start_min;
    int duration_min;
}   schedule_task_t;

typedef struct {
    bool enabled;
    char timezone[32];
    int task_count;
    schedule_task_t tasks[MAX_TASKS];
    int config_version;
} work_schedule_t;

#ifdef __cplusplus
}
#endif
