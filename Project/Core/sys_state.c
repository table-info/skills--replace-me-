/**
 * @file    sys_state.c
 * @brief   系统主状态机实现
 *
 * 状态转换图：
 *
 *   SYSTEM_INIT
 *       │ (初始化完成)
 *       ▼
 *   SYSTEM_IDLE ◄──────────────────────────────────────────┐
 *       │ EVT_HANDLE_BTN_PRESS / EVT_FOOT_SW_PRESS         │
 *       │ EVT_DIWIN_START                                  │
 *       ▼                                                  │
 *   SYSTEM_RUNNING                                         │
 *       │ EVT_HANDLE_BTN_PRESS / EVT_FOOT_SW_PRESS         │
 *       │ EVT_DIWIN_STOP                                   │
 *       ▼                                                  │
 *   SYSTEM_STOPPING ──(EVT_MOTOR_STOPPED)──────────────────┘
 *
 *   任意状态 + EVT_MOTOR_FAULT → SYSTEM_FAULT
 */
#include "sys_state.h"
#include "event_queue.h"
#include <stddef.h>
#include "../Drivers/motor/motor_main.h"
#include "../Drivers/motor/motor_blade.h"
#include "../Drivers/diwin/diwin_comm.h"
#include "../Drivers/voice/voice_ctrl.h"

static SystemState_t s_state = SYSTEM_INIT;
static uint16_t      s_fault_code = 0U;

/* -----------------------------------------------------------------------
 * 私有：进入各状态的入口动作
 * --------------------------------------------------------------------- */
static void enter_idle(void)
{
    s_state = SYSTEM_IDLE;
    DiwinComm_UpdateMotorStatus(0U, 0U);
    DiwinComm_UpdateFaultCode(0U);
    VoiceCtrl_Play(VOICE_ID_READY);
}

static void enter_running(void)
{
    s_state = SYSTEM_RUNNING;
    MotorBlade_Start();
    DiwinComm_UpdateMotorStatus(1U, 0U);
    VoiceCtrl_Play(VOICE_ID_START);
}

static void enter_stopping(void)
{
    s_state = SYSTEM_STOPPING;
    MotorBlade_Stop();
    DiwinComm_UpdateMotorStatus(2U, 0U);
    VoiceCtrl_Play(VOICE_ID_STOP);
}

static void enter_fault(uint16_t fault_code)
{
    s_state      = SYSTEM_FAULT;
    s_fault_code = fault_code;
    /* 立即停止刨削电机 */
    MotorBlade_Stop();
    DiwinComm_UpdateMotorStatus(0xFFU, 0U);
    DiwinComm_UpdateFaultCode(fault_code);
    VoiceCtrl_PlayUrgent(VOICE_ID_FAULT);
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void SysState_Init(void)
{
    s_state      = SYSTEM_INIT;
    s_fault_code = 0U;
}

SystemState_t SysState_Get(void)
{
    return s_state;
}

void SysState_HandleEvent(const Event_t *evt)
{
    if (evt == NULL) {
        return;
    }

    /* 故障事件：任意状态均最高优先级处理 */
    if (evt->type == EVT_MOTOR_FAULT) {
        enter_fault((uint16_t)evt->param);
        return;
    }

    switch (s_state) {
    /* ---------------------------------------------------------------- */
    case SYSTEM_INIT:
        /* 初始化阶段忽略外部事件，由 SysState_Update 驱动初始化完成跳转 */
        break;

    /* ---------------------------------------------------------------- */
    case SYSTEM_IDLE:
        switch (evt->type) {
        case EVT_HANDLE_BTN_PRESS:
        case EVT_FOOT_SW_PRESS:
        case EVT_DIWIN_START:
            enter_running();
            break;
        case EVT_DIWIN_SPEED:
            MotorMain_SetDuty((uint16_t)evt->param);
            break;
        case EVT_RFID_CARD_READ:
            /* 根据卡号选择电机 A 或 B（示例：奇数→A，偶数→B） */
            {
                BladeMotorId_t target = (evt->param & 0x01U)
                                        ? BLADE_MOTOR_A : BLADE_MOTOR_B;
                if (MotorBlade_Select(target)) {
                    VoiceCtrl_Play((target == BLADE_MOTOR_A)
                                   ? VOICE_ID_MOTOR_A : VOICE_ID_MOTOR_B);
                } else {
                    /* 另一台电机仍在运行，无法切换 */
                    VoiceCtrl_Play(VOICE_ID_CARD_FAIL);
                }
            }
            break;
        case EVT_RFID_CARD_INVALID:
            VoiceCtrl_Play(VOICE_ID_CARD_FAIL);
            DiwinComm_UpdateFaultCode(0x0001U);
            break;
        default:
            break;
        }
        break;

    /* ---------------------------------------------------------------- */
    case SYSTEM_RUNNING:
        switch (evt->type) {
        case EVT_HANDLE_BTN_PRESS:
        case EVT_FOOT_SW_PRESS:
        case EVT_DIWIN_STOP:
            enter_stopping();
            break;
        case EVT_DIWIN_SPEED:
            MotorBlade_SetDuty((uint16_t)evt->param);
            MotorMain_SetDuty((uint16_t)evt->param);
            break;
        default:
            break;
        }
        break;

    /* ---------------------------------------------------------------- */
    case SYSTEM_STOPPING:
        /* 等待刨削电机完全停止 */
        if (evt->type == EVT_MOTOR_STOPPED) {
            enter_idle();
        }
        break;

    /* ---------------------------------------------------------------- */
    case SYSTEM_FAULT:
        /* 故障状态只接受人工复位（此处预留，实际可通过屏幕按键复位） */
        if (evt->type == EVT_DIWIN_START) {
            /* 简单复位：回到 IDLE */
            s_fault_code = 0U;
            enter_idle();
        }
        break;

    default:
        break;
    }
}

void SysState_Update(void)
{
    switch (s_state) {
    case SYSTEM_INIT:
        /* 初始化完成后自动进入 IDLE */
        enter_idle();
        break;

    case SYSTEM_STOPPING:
        /* 若刨削电机已停止但事件还未到队列，直接检查 */
        if (MotorBlade_IsIdle(MotorBlade_GetActive())) {
            enter_idle();
        }
        break;

    default:
        break;
    }
}
