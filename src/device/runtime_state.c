#include "runtime_state.h"
#include <string.h>

static runtime_state_t g_runtime;

void runtime_state_init(void)
{
    memset(&g_runtime, 0, sizeof(g_runtime));
    strcpy(g_runtime.last_error, "System OK");
}

runtime_state_t* runtime_state_get(void)
{
    return &g_runtime;
}

void runtime_state_set_error(const char *msg)
{
    strncpy(g_runtime.last_error, msg, sizeof(g_runtime.last_error) - 1);
}
