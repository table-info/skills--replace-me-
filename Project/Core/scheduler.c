/**
 * @file    scheduler.c
 * @brief   时间片调度器实现
 *
 * SysTick 每 1ms 调用 Scheduler_Tick()，递减所有任务计数器。
 * 主循环调用 Scheduler_Run()，执行计数器归零的任务并重装计数。
 */
#include "scheduler.h"
#include <string.h>

/* 任务表 */
static Task_t    s_tasks[SCHEDULER_MAX_TASKS];
static uint8_t   s_task_count = 0U;

void Scheduler_Init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    s_task_count = 0U;
}

int Scheduler_AddTask(void (*handler)(void), uint16_t period, const char *name)
{
    if (s_task_count >= SCHEDULER_MAX_TASKS || handler == NULL) {
        return -1;
    }
    int id = (int)s_task_count;
    s_tasks[id].handler = handler;
    s_tasks[id].period  = period;
    s_tasks[id].counter = period;
    s_tasks[id].enabled = true;
    s_tasks[id].name    = name;
    s_task_count++;
    return id;
}

void Scheduler_SetTaskEnabled(int task_id, bool enable)
{
    if (task_id >= 0 && (uint8_t)task_id < s_task_count) {
        s_tasks[task_id].enabled = enable;
        if (enable) {
            /* 重置计数，避免立即触发 */
            s_tasks[task_id].counter = s_tasks[task_id].period;
        }
    }
}

/* 在 SysTick_Handler 中调用，每 1ms 一次 */
void Scheduler_Tick(void)
{
    for (uint8_t i = 0U; i < s_task_count; i++) {
        if (!s_tasks[i].enabled) {
            continue;
        }
        if (s_tasks[i].counter > 0U) {
            s_tasks[i].counter--;
        }
    }
}

/* 在主循环 while(1) 中调用 */
void Scheduler_Run(void)
{
    for (uint8_t i = 0U; i < s_task_count; i++) {
        if (!s_tasks[i].enabled) {
            continue;
        }
        if (s_tasks[i].counter == 0U) {
            s_tasks[i].counter = s_tasks[i].period; /* 重装 */
            s_tasks[i].handler();                   /* 执行任务 */
        }
    }
}
