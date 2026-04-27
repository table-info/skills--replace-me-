/**
 * @file    sys_state.h
 * @brief   系统主状态机
 *
 * 状态转换：
 *   SYSTEM_INIT → SYSTEM_IDLE → SYSTEM_RUNNING → SYSTEM_STOPPING → SYSTEM_IDLE
 *                                     ↓ (任意状态收到故障)
 *                               SYSTEM_FAULT
 */
#ifndef SYS_STATE_H
#define SYS_STATE_H

#include <stdint.h>
#include "event_queue.h"

/* -----------------------------------------------------------------------
 * 系统状态枚举
 * --------------------------------------------------------------------- */
typedef enum {
    SYSTEM_INIT     = 0,  /**< 上电初始化 / 自检                     */
    SYSTEM_IDLE,          /**< 空闲：主轴电机运行，等待启动信号       */
    SYSTEM_RUNNING,       /**< 运行：刨削电机已启动                   */
    SYSTEM_STOPPING,      /**< 停止中：刨削电机受控减速               */
    SYSTEM_FAULT,         /**< 故障：语音报警 + 显示报错 + 停机       */
} SystemState_t;

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化系统状态机（进入 SYSTEM_INIT）
 */
void SysState_Init(void);

/**
 * @brief  获取当前系统状态
 */
SystemState_t SysState_Get(void);

/**
 * @brief  处理一个事件，驱动状态机转换
 * @param  evt  从事件队列弹出的事件
 */
void SysState_HandleEvent(const Event_t *evt);

/**
 * @brief  周期性状态机轮询（由 Task_EventDispatch 调用）
 *         用于处理超时、状态入口动作等
 */
void SysState_Update(void);

#endif /* SYS_STATE_H */
