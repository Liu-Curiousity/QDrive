#include "sys_public.h"
#include "task_public.h"

void App_DebugTask(void *argument) {
    for (;;) {
        delay(1000);
    }
}
