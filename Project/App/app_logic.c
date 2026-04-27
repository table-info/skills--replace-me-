/**
 * @file    app_logic.c
 * @brief   应用层业务逻辑实现
 *
 * 将所有模块粘合在一起：
 *  - 初始化所有驱动与调度器
 *  - Task_EventDispatch：轮询事件队列 → 系统状态机处理
 *  - Task_MotorCtrl：电机 PID / 消抖 / 换相
 *  - Task_SysMon：喂狗 + 刷新屏幕
 */
#include "app_logic.h"
#include "../Core/event_queue.h"
#include "../Core/sys_state.h"
#include "../Core/scheduler.h"
#include "../Drivers/motor/motor_main.h"
#include "../Drivers/motor/motor_blade.h"
#include "../Drivers/diwin/diwin_comm.h"
#include "../Drivers/rfid/rfid_rc522.h"
#include "../Drivers/voice/voice_ctrl.h"
#include "../Drivers/input/switch_input.h"
#include "../HAL/stm32f4xx_hal.h"

/* -----------------------------------------------------------------------
 * 任务处理函数前向声明
 * --------------------------------------------------------------------- */
static void Task_MotorCtrl(void);
static void Task_EventDispatch(void);
static void Task_DiwinComm(void);
static void Task_RFID(void);
static void Task_VoiceCtrl(void);
static void Task_SysMon(void);

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void AppLogic_Init(void)
{
    /* 初始化全局事件队列 */
    EventQueue_Init(EventQueue_GetGlobal());

    /* 初始化各驱动模块 */
    SwitchInput_Init();
    MotorMain_Init();
    MotorBlade_Init();
    DiwinComm_Init();
    RFID_Init();
    VoiceCtrl_Init();

    /* 初始化系统状态机 */
    SysState_Init();

    /* 初始化调度器并注册任务
     * 任务按执行周期从短到长排列（调度器按注册顺序轮询） */
    Scheduler_Init();
    Scheduler_AddTask(Task_MotorCtrl,    1U,   "MotorCtrl");
    Scheduler_AddTask(Task_EventDispatch, 2U,  "EventDispatch");
    Scheduler_AddTask(Task_DiwinComm,    10U,  "DiwinComm");
    Scheduler_AddTask(Task_VoiceCtrl,    50U,  "VoiceCtrl");
    Scheduler_AddTask(Task_RFID,         100U, "RFID");
    Scheduler_AddTask(Task_SysMon,       500U, "SysMon");

    /* 启动主轴电机（上电即运行） */
    MotorMain_Start();

    /* 播报上电提示音 */
    VoiceCtrl_Play(VOICE_ID_BOOT);
}

/* -----------------------------------------------------------------------
 * 任务：电机控制（1ms）
 * 优先级最高，驱动电机状态机 + 故障检测 + 消抖
 * --------------------------------------------------------------------- */
static void Task_MotorCtrl(void)
{
    MotorMain_FaultCheck();
    MotorBlade_Update();
    SwitchInput_DebounceUpdate();
}

/* -----------------------------------------------------------------------
 * 任务：事件分发（2ms）
 * 轮询事件队列，将所有待处理事件交给系统状态机
 * --------------------------------------------------------------------- */
static void Task_EventDispatch(void)
{
    Event_t evt;
    EventQueue_t *q = EventQueue_GetGlobal();

    while (EventQueue_Pop(q, &evt)) {
        SysState_HandleEvent(&evt);
    }

    /* 状态机周期轮询（处理超时/入口动作） */
    SysState_Update();
}

/* -----------------------------------------------------------------------
 * 任务：迪文屏通信（10ms）
 * --------------------------------------------------------------------- */
static void Task_DiwinComm(void)
{
    DiwinComm_Process();
}

/* -----------------------------------------------------------------------
 * 任务：RFID 轮询（100ms）
 * --------------------------------------------------------------------- */
static void Task_RFID(void)
{
    RFID_Poll();
}

/* -----------------------------------------------------------------------
 * 任务：语音播报管理（50ms）
 * --------------------------------------------------------------------- */
static void Task_VoiceCtrl(void)
{
    VoiceCtrl_Update();
}

/* -----------------------------------------------------------------------
 * 任务：系统监控（500ms）
 * --------------------------------------------------------------------- */
static void Task_SysMon(void)
{
    /* 喂独立看门狗 */
    HAL_IWDG_Refresh(&hiwdg);

    /* 刷新屏幕状态显示 */
    SystemState_t st = SysState_Get();
    DiwinComm_UpdateMotorStatus((uint16_t)st,
                                MotorMain_GetDuty()); /* 用占空比代替转速显示 */
}

/* -----------------------------------------------------------------------
 * 对外暴露的任务入口（main.c 也可直接调用）
 * --------------------------------------------------------------------- */
void AppLogic_EventDispatch(void)
{
    Task_EventDispatch();
}

void AppLogic_MotorCtrlTask(void)
{
    Task_MotorCtrl();
}

void AppLogic_SysMonTask(void)
{
    Task_SysMon();
}
