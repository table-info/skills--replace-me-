/**
 * @file    motor_main.c
 * @brief   主轴无刷直流电机驱动实现
 *
 * 使用 TIM1 高级定时器 3 路互补 PWM 驱动三相无刷电机。
 * 开环 PWM 占空比调速，支持软件过流/堵转保护。
 */
#include "motor_main.h"
#include "../../HAL/stm32f4xx_hal.h"
#include "../../Core/event_queue.h"

/* -----------------------------------------------------------------------
 * 模块私有状态
 * --------------------------------------------------------------------- */
static bool     s_running    = false;
static uint16_t s_duty       = MOTOR_MAIN_DUTY_DEF;  /* 当前占空比 ‰ */

/* 过流计数（连续 N 次采样超阈值则报故障） */
#define FAULT_COUNT_THRESH  50U
static uint8_t  s_fault_cnt  = 0U;

/* -----------------------------------------------------------------------
 * 私有：将占空比写入 TIM1 CCR 寄存器
 * 此处使用 HAL 宏替代，实际项目中直接操作寄存器效率更高
 * --------------------------------------------------------------------- */
static void apply_duty(uint16_t duty)
{
    /* TIM1 ARR 假设为 1000（1kHz PWM）
     * CCR = duty（‰ → ARR 对应值）
     * 实际根据 CubeMX 配置的 ARR 值调整 */
    (void)duty;  /* 实际应写 htim1.Instance->CCR1 = duty; 等 */
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void MotorMain_Init(void)
{
    s_running  = false;
    s_duty     = MOTOR_MAIN_DUTY_DEF;
    s_fault_cnt = 0U;

    /* 配置 TIM1 三路 PWM 输出（CubeMX 已完成基础配置） */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

    /* 使能引脚拉低（禁止输出） */
    HAL_GPIO_WritePin(GPIOA, MOTOR_MAIN_EN_PIN, GPIO_PIN_RESET);
}

void MotorMain_Start(void)
{
    if (s_running) {
        return;
    }
    s_fault_cnt = 0U;
    apply_duty(s_duty);
    HAL_GPIO_WritePin(GPIOA, MOTOR_MAIN_EN_PIN, GPIO_PIN_SET);
    s_running = true;
}

void MotorMain_Stop(void)
{
    HAL_GPIO_WritePin(GPIOA, MOTOR_MAIN_EN_PIN, GPIO_PIN_RESET);
    apply_duty(0U);
    s_running = false;
}

void MotorMain_SetDuty(uint16_t duty_permil)
{
    if (duty_permil < MOTOR_MAIN_DUTY_MIN) {
        duty_permil = MOTOR_MAIN_DUTY_MIN;
    }
    if (duty_permil > MOTOR_MAIN_DUTY_MAX) {
        duty_permil = MOTOR_MAIN_DUTY_MAX;
    }
    s_duty = duty_permil;
    if (s_running) {
        apply_duty(s_duty);
    }
}

uint16_t MotorMain_GetDuty(void)
{
    return s_duty;
}

bool MotorMain_IsRunning(void)
{
    return s_running;
}

void MotorMain_FaultCheck(void)
{
    if (!s_running) {
        s_fault_cnt = 0U;
        return;
    }

    /* 读取过流检测 GPIO（高电平表示过流）
     * 实际项目中读取 ADC 采样值或比较器输出 */
    uint32_t oc = HAL_GPIO_ReadPin(GPIOB, 0x0010U); /* 示例引脚 */
    if (oc == GPIO_PIN_SET) {
        s_fault_cnt++;
    } else {
        if (s_fault_cnt > 0U) {
            s_fault_cnt--;
        }
    }

    if (s_fault_cnt >= FAULT_COUNT_THRESH) {
        /* 立即停机并发出故障事件 */
        MotorMain_Stop();
        EVT_PUSH(EVT_MOTOR_FAULT, 0x0001U); /* 0x0001 = 主轴过流 */
        s_fault_cnt = 0U;
    }
}
