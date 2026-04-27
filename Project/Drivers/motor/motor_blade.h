/**
 * @file    motor_blade.h
 * @brief   刨削无刷直流电机 A/B 驱动（互斥运行）
 *
 * 同型号两台电机，同一时刻只允许运行一台。
 * 通过 RFID 卡识别或手动指令选择当前激活电机。
 *
 * 每台电机使用独立状态机：
 *   IDLE → STARTING → RUNNING → STOPPING → IDLE
 *
 * 硬件：TIM2（电机A）、TIM3（电机B），各 3 路 PWM + 使能引脚
 */
#ifndef MOTOR_BLADE_H
#define MOTOR_BLADE_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * 电机 ID
 * --------------------------------------------------------------------- */
typedef enum {
    BLADE_MOTOR_A = 0,
    BLADE_MOTOR_B = 1,
    BLADE_MOTOR_NONE = 0xFF,
} BladeMotorId_t;

/* -----------------------------------------------------------------------
 * 电机状态机状态
 * --------------------------------------------------------------------- */
typedef enum {
    BLADE_STATE_IDLE     = 0,
    BLADE_STATE_STARTING,
    BLADE_STATE_RUNNING,
    BLADE_STATE_STOPPING,
} BladeMotorState_t;

/* -----------------------------------------------------------------------
 * 占空比范围（‰）
 * --------------------------------------------------------------------- */
#define BLADE_DUTY_MIN   100U
#define BLADE_DUTY_MAX  1000U
#define BLADE_DUTY_DEF   700U

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化刨削电机模块（两台电机均处于 IDLE）
 */
void MotorBlade_Init(void);

/**
 * @brief  选择激活电机（切换前必须确保另一台已完全停止）
 * @param  id  BLADE_MOTOR_A 或 BLADE_MOTOR_B
 * @return true=切换成功，false=另一台仍在运行，切换被拒绝
 */
bool MotorBlade_Select(BladeMotorId_t id);

/**
 * @brief  获取当前激活电机 ID
 */
BladeMotorId_t MotorBlade_GetActive(void);

/**
 * @brief  启动当前激活电机
 */
void MotorBlade_Start(void);

/**
 * @brief  停止当前激活电机（受控减速）
 */
void MotorBlade_Stop(void);

/**
 * @brief  设置当前激活电机目标占空比
 * @param  duty_permil  [BLADE_DUTY_MIN, BLADE_DUTY_MAX]
 */
void MotorBlade_SetDuty(uint16_t duty_permil);

/**
 * @brief  获取指定电机当前状态机状态
 */
BladeMotorState_t MotorBlade_GetState(BladeMotorId_t id);

/**
 * @brief  电机控制轮询（由 Task_MotorCtrl 每 1ms 调用）
 *         驱动启动/停止斜坡、故障检测
 */
void MotorBlade_Update(void);

/**
 * @brief  判断指定电机是否已完全停止（IDLE）
 */
bool MotorBlade_IsIdle(BladeMotorId_t id);

#endif /* MOTOR_BLADE_H */
