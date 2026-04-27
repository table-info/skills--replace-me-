/**
 * @file    motor_blade.c
 * @brief   刨削电机 A/B 互斥控制实现
 *
 * 每台电机维护独立状态机，互斥锁由软件标志位保证。
 * 启动/停止使用 PWM 斜坡控制，避免电流冲击。
 */
#include "motor_blade.h"
#include "../../HAL/stm32f4xx_hal.h"
#include "../../Core/event_queue.h"

/* -----------------------------------------------------------------------
 * 单台电机控制块
 * --------------------------------------------------------------------- */
typedef struct {
    BladeMotorState_t state;
    uint16_t          duty;          /* 当前占空比 ‰ */
    uint16_t          target_duty;   /* 目标占空比 ‰ */
    uint16_t          ramp_timer;    /* 斜坡计时器（ms） */
    TIM_HandleTypeDef *htim;
    GPIO_TypeDef      *en_port;
    uint16_t           en_pin;
} BladeMotorCtrl_t;

/* -----------------------------------------------------------------------
 * 模块私有状态
 * --------------------------------------------------------------------- */
static BladeMotorCtrl_t s_motors[2];
static BladeMotorId_t   s_active = BLADE_MOTOR_NONE;

/* 斜坡步长（每 1ms 增减的占空比 ‰） */
#define RAMP_STEP_UP    5U
#define RAMP_STEP_DOWN  10U

/* 启动超时（ms），超时则报故障 */
#define START_TIMEOUT_MS  3000U

/* 过流计数阈值 */
#define BLADE_FAULT_THRESH  50U
static uint8_t s_fault_cnt[2] = {0U, 0U};

/* -----------------------------------------------------------------------
 * 私有：写占空比到对应 TIM
 * --------------------------------------------------------------------- */
static void write_duty(BladeMotorId_t id, uint16_t duty)
{
    (void)id;
    (void)duty;
    /* 实际：s_motors[id].htim->Instance->CCR1 = duty; 等 */
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void MotorBlade_Init(void)
{
    /* 电机 A：TIM2 */
    s_motors[BLADE_MOTOR_A].state       = BLADE_STATE_IDLE;
    s_motors[BLADE_MOTOR_A].duty        = 0U;
    s_motors[BLADE_MOTOR_A].target_duty = BLADE_DUTY_DEF;
    s_motors[BLADE_MOTOR_A].ramp_timer  = 0U;
    s_motors[BLADE_MOTOR_A].htim        = &htim2;
    s_motors[BLADE_MOTOR_A].en_port     = GPIOA;
    s_motors[BLADE_MOTOR_A].en_pin      = MOTOR_A_EN_PIN;

    /* 电机 B：TIM3 */
    s_motors[BLADE_MOTOR_B].state       = BLADE_STATE_IDLE;
    s_motors[BLADE_MOTOR_B].duty        = 0U;
    s_motors[BLADE_MOTOR_B].target_duty = BLADE_DUTY_DEF;
    s_motors[BLADE_MOTOR_B].ramp_timer  = 0U;
    s_motors[BLADE_MOTOR_B].htim        = &htim3;
    s_motors[BLADE_MOTOR_B].en_port     = GPIOA;
    s_motors[BLADE_MOTOR_B].en_pin      = MOTOR_B_EN_PIN;

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

    s_active = BLADE_MOTOR_NONE;
}

bool MotorBlade_Select(BladeMotorId_t id)
{
    if (id >= 2U) {
        return false;
    }
    /* 互斥保护：另一台必须完全停止 */
    BladeMotorId_t other = (id == BLADE_MOTOR_A) ? BLADE_MOTOR_B : BLADE_MOTOR_A;
    if (s_motors[other].state != BLADE_STATE_IDLE) {
        return false;
    }
    s_active = id;
    return true;
}

BladeMotorId_t MotorBlade_GetActive(void)
{
    return s_active;
}

void MotorBlade_Start(void)
{
    if (s_active >= 2U) {
        return; /* 未选择电机 */
    }
    BladeMotorCtrl_t *m = &s_motors[s_active];
    if (m->state != BLADE_STATE_IDLE) {
        return;
    }
    m->duty       = 0U;
    m->ramp_timer = 0U;
    s_fault_cnt[s_active] = 0U;
    HAL_GPIO_WritePin(m->en_port, m->en_pin, GPIO_PIN_SET);
    m->state = BLADE_STATE_STARTING;
}

void MotorBlade_Stop(void)
{
    if (s_active >= 2U) {
        return;
    }
    BladeMotorCtrl_t *m = &s_motors[s_active];
    if (m->state == BLADE_STATE_IDLE) {
        return;
    }
    m->ramp_timer = 0U;
    m->state = BLADE_STATE_STOPPING;
}

void MotorBlade_SetDuty(uint16_t duty_permil)
{
    if (s_active >= 2U) {
        return;
    }
    if (duty_permil < BLADE_DUTY_MIN) duty_permil = BLADE_DUTY_MIN;
    if (duty_permil > BLADE_DUTY_MAX) duty_permil = BLADE_DUTY_MAX;
    s_motors[s_active].target_duty = duty_permil;
}

BladeMotorState_t MotorBlade_GetState(BladeMotorId_t id)
{
    if (id >= 2U) {
        return BLADE_STATE_IDLE;
    }
    return s_motors[id].state;
}

bool MotorBlade_IsIdle(BladeMotorId_t id)
{
    if (id >= 2U) {
        return true;
    }
    return s_motors[id].state == BLADE_STATE_IDLE;
}

/* -----------------------------------------------------------------------
 * 每 1ms 调用的控制轮询
 * --------------------------------------------------------------------- */
void MotorBlade_Update(void)
{
    for (uint8_t i = 0U; i < 2U; i++) {
        BladeMotorCtrl_t *m = &s_motors[i];

        switch (m->state) {
        /* ------------------------------------------------------------ */
        case BLADE_STATE_STARTING:
            /* PWM 斜坡升速 */
            if (m->duty < m->target_duty) {
                m->duty += RAMP_STEP_UP;
                if (m->duty > m->target_duty) {
                    m->duty = m->target_duty;
                }
                write_duty((BladeMotorId_t)i, m->duty);
            } else {
                /* 到达目标转速 */
                m->state = BLADE_STATE_RUNNING;
                EVT_PUSH(EVT_MOTOR_STARTED, (uint32_t)i);
            }
            /* 启动超时检测 */
            m->ramp_timer++;
            if (m->ramp_timer >= START_TIMEOUT_MS) {
                m->state = BLADE_STATE_IDLE;
                HAL_GPIO_WritePin(m->en_port, m->en_pin, GPIO_PIN_RESET);
                write_duty((BladeMotorId_t)i, 0U);
                EVT_PUSH(EVT_MOTOR_FAULT, 0x0010U | (uint32_t)i); /* 启动超时 */
            }
            break;

        /* ------------------------------------------------------------ */
        case BLADE_STATE_RUNNING:
            /* 过流检测（示例：读取比较器 GPIO） */
            {
                uint16_t oc_pin = (i == (uint8_t)BLADE_MOTOR_A) ? 0x0020U : 0x0040U;
                uint32_t oc = HAL_GPIO_ReadPin(GPIOB, oc_pin);
                if (oc == GPIO_PIN_SET) {
                    s_fault_cnt[i]++;
                } else {
                    if (s_fault_cnt[i] > 0U) s_fault_cnt[i]--;
                }
                if (s_fault_cnt[i] >= BLADE_FAULT_THRESH) {
                    m->state = BLADE_STATE_IDLE;
                    HAL_GPIO_WritePin(m->en_port, m->en_pin, GPIO_PIN_RESET);
                    write_duty((BladeMotorId_t)i, 0U);
                    s_fault_cnt[i] = 0U;
                    EVT_PUSH(EVT_MOTOR_FAULT, 0x0020U | (uint32_t)i); /* 过流 */
                }
            }
            break;

        /* ------------------------------------------------------------ */
        case BLADE_STATE_STOPPING:
            /* PWM 斜坡降速 */
            if (m->duty > RAMP_STEP_DOWN) {
                m->duty -= RAMP_STEP_DOWN;
                write_duty((BladeMotorId_t)i, m->duty);
            } else {
                m->duty = 0U;
                write_duty((BladeMotorId_t)i, 0U);
                HAL_GPIO_WritePin(m->en_port, m->en_pin, GPIO_PIN_RESET);
                m->state = BLADE_STATE_IDLE;
                EVT_PUSH(EVT_MOTOR_STOPPED, (uint32_t)i);
            }
            break;

        /* ------------------------------------------------------------ */
        case BLADE_STATE_IDLE:
        default:
            break;
        }
    }
}
