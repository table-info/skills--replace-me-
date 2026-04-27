/**
 * @file    motor_main.h
 * @brief   主轴无刷直流电机驱动（常运行）
 *
 * 主轴电机上电后即自动启动，始终保持运转。
 * 支持 PWM 占空比调速（开环）和故障检测（过流/堵转）。
 * 故障时发出 EVT_MOTOR_FAULT 事件。
 *
 * 硬件：TIM1（高级定时器），3 路互补 PWM + 使能引脚
 */
#ifndef MOTOR_MAIN_H
#define MOTOR_MAIN_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * PWM 占空比范围（‰，千分比）
 * --------------------------------------------------------------------- */
#define MOTOR_MAIN_DUTY_MIN   100U   /**< 最低 10% */
#define MOTOR_MAIN_DUTY_MAX  1000U   /**< 最高 100% */
#define MOTOR_MAIN_DUTY_DEF   600U   /**< 默认 60% */

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化主轴电机（配置 TIM1 PWM，使能引脚）
 */
void MotorMain_Init(void);

/**
 * @brief  启动主轴电机
 */
void MotorMain_Start(void);

/**
 * @brief  停止主轴电机（紧急停机）
 */
void MotorMain_Stop(void);

/**
 * @brief  设置目标转速（PWM 占空比调节）
 * @param  duty_permil  占空比千分比 [100, 1000]
 */
void MotorMain_SetDuty(uint16_t duty_permil);

/**
 * @brief  获取当前占空比设定值
 */
uint16_t MotorMain_GetDuty(void);

/**
 * @brief  故障检测轮询（由 Task_MotorCtrl 每 1ms 调用）
 *         检测过流/堵转，触发 EVT_MOTOR_FAULT
 */
void MotorMain_FaultCheck(void);

/**
 * @brief  判断主轴电机是否正在运行
 */
bool MotorMain_IsRunning(void);

#endif /* MOTOR_MAIN_H */
