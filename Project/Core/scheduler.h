/**
 * @file    scheduler.h
 * @brief   时间片调度器
 *
 * SysTick 每 1 ms 中断一次，驱动任务计数器递减；
 * 主循环轮询 Scheduler_Run()，当某任务计数器归零时执行其 handler。
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * 任务描述符
 * --------------------------------------------------------------------- */
typedef struct {
    void     (*handler)(void); /**< 任务处理函数         */
    uint16_t  period;          /**< 调度周期（ms）        */
    uint16_t  counter;         /**< 剩余计数（ms）        */
    bool      enabled;         /**< 是否使能              */
    const char *name;          /**< 调试用任务名称        */
} Task_t;

/* -----------------------------------------------------------------------
 * 最大任务数
 * --------------------------------------------------------------------- */
#define SCHEDULER_MAX_TASKS  8U

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化调度器（清空任务表）
 */
void Scheduler_Init(void);

/**
 * @brief  注册任务
 * @param  handler  任务函数指针
 * @param  period   调度周期（ms），填 0 则每 tick 都执行
 * @param  name     调试名称
 * @return 任务 ID（>= 0），失败返回 -1
 */
int  Scheduler_AddTask(void (*handler)(void), uint16_t period, const char *name);

/**
 * @brief  使能 / 禁用任务
 * @param  task_id  由 Scheduler_AddTask 返回的 ID
 * @param  enable   true=使能
 */
void Scheduler_SetTaskEnabled(int task_id, bool enable);

/**
 * @brief  在 SysTick 中断中调用（每 1 ms 一次），递减所有任务计数器
 */
void Scheduler_Tick(void);

/**
 * @brief  在主循环中调用，执行到期任务
 */
void Scheduler_Run(void);

#endif /* SCHEDULER_H */
