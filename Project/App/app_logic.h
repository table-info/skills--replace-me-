/**
 * @file    app_logic.h
 * @brief   应用层业务逻辑与事件回调
 *
 * 负责：
 *  - 注册事件处理回调
 *  - 将事件转发给系统状态机
 *  - 协调 RFID → 电机选择、迪文屏刷新等跨模块逻辑
 */
#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include "../Core/event_queue.h"

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化应用层（在所有驱动初始化完成后调用）
 */
void AppLogic_Init(void);

/**
 * @brief  事件分发处理（由 Task_EventDispatch 每 2ms 调用）
 *         轮询事件队列并调用对应处理函数
 */
void AppLogic_EventDispatch(void);

/**
 * @brief  电机控制任务（由 Task_MotorCtrl 每 1ms 调用）
 */
void AppLogic_MotorCtrlTask(void);

/**
 * @brief  系统监控任务（由 Task_SysMon 每 500ms 调用）
 *         喂狗 + 刷新屏幕状态
 */
void AppLogic_SysMonTask(void);

#endif /* APP_LOGIC_H */
