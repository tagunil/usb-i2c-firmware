#include <stdnoreturn.h>

#include <FreeRTOS.h>
#include <task.h>

noreturn void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;

    for (;;) {
    }
}

#ifdef DEBUG
noreturn void vAssertCalled(const char *file, int line)
{
    (void)file;
    (void)line;

    taskDISABLE_INTERRUPTS();

    for (;;) {
    }
}
#endif

void vApplicationGetIdleTaskMemory(StaticTask_t **idle_task_data,
                                   StackType_t **idle_task_stack,
                                   uint32_t *idle_task_stack_size)
{
    static StaticTask_t task_data;
    static StackType_t task_stack[configMINIMAL_STACK_SIZE];

    *idle_task_data = &task_data;
    *idle_task_stack = task_stack;
    *idle_task_stack_size = sizeof(task_stack) / sizeof(StackType_t);
}

#ifdef configUSE_TIMERS
void vApplicationGetTimerTaskMemory(StaticTask_t **timer_task_data,
                                    StackType_t **timer_task_stack,
                                    uint32_t *timer_task_stack_size)
{
    static StaticTask_t task_data;
    static StackType_t task_stack[configTIMER_TASK_STACK_DEPTH];

    *timer_task_data = &task_data;
    *timer_task_stack = task_stack;
    *timer_task_stack_size = sizeof(task_stack) / sizeof(StackType_t);
}
#endif
