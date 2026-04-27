/**
 * @file    switch_input.h
 * @brief   手柄按键与脚踏开关中断输入驱动
 *
 * 两路输入均通过 EXTI 中断捕获上升/下降沿，
 * 中断 ISR 仅推送事件到全局事件队列，不做业务逻辑。
 * 包含软件消抖（20ms）。
 *
 * 硬件：
 *   手柄按键 → EXTI（例如 PC6）
 *   脚踏开关 → EXTI（例如 PC7）
 */
#ifndef SWITCH_INPUT_H
#define SWITCH_INPUT_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * 消抖时间（ms）
 * --------------------------------------------------------------------- */
#define SWITCH_DEBOUNCE_MS  20U

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化输入开关（GPIO EXTI 配置）
 */
void SwitchInput_Init(void);

/**
 * @brief  手柄按键 EXTI 中断回调（在 HAL_GPIO_EXTI_Callback 中调用）
 * @param  rising  true=上升沿（按下），false=下降沿（释放）
 */
void SwitchInput_HandleBtnIRQ(bool rising);

/**
 * @brief  脚踏开关 EXTI 中断回调（在 HAL_GPIO_EXTI_Callback 中调用）
 * @param  rising  true=踩下，false=抬起
 */
void SwitchInput_HandleFootIRQ(bool rising);

/**
 * @brief  消抖定时器轮询（由 Task_MotorCtrl 或专用 1ms 任务调用）
 */
void SwitchInput_DebounceUpdate(void);

/**
 * @brief  查询手柄按键当前稳定电平（消抖后）
 * @return true=按下，false=释放
 */
bool SwitchInput_IsBtnPressed(void);

/**
 * @brief  查询脚踏开关当前稳定电平（消抖后）
 * @return true=踩下，false=抬起
 */
bool SwitchInput_IsFootPressed(void);

#endif /* SWITCH_INPUT_H */
