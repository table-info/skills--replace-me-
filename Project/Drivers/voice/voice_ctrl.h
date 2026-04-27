/**
 * @file    voice_ctrl.h
 * @brief   语音芯片控制驱动（JQ8400 / DY-SV5W 等 UART 串行协议）
 *
 * 功能：
 *  - 维护播报队列，避免打断正在播放内容
 *  - 关键事件触发对应语音条目
 *  - 通过 VOICE_BUSY_PIN 或固定时长估算播报完成
 *
 * 硬件：USART6（9600bps，8N1）
 */
#ifndef VOICE_CTRL_H
#define VOICE_CTRL_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * 语音条目 ID（与语音芯片内 Flash 中的音频编号对应）
 * --------------------------------------------------------------------- */
typedef enum {
    VOICE_ID_BOOT        = 1,   /**< "系统启动"          */
    VOICE_ID_READY       = 2,   /**< "设备就绪"          */
    VOICE_ID_START       = 3,   /**< "启动刨削"          */
    VOICE_ID_STOP        = 4,   /**< "停止刨削"          */
    VOICE_ID_FAULT       = 5,   /**< "设备故障，请检查"  */
    VOICE_ID_CARD_OK     = 6,   /**< "刷卡成功"          */
    VOICE_ID_CARD_FAIL   = 7,   /**< "无效卡片"          */
    VOICE_ID_MOTOR_A     = 8,   /**< "切换至电机A"       */
    VOICE_ID_MOTOR_B     = 9,   /**< "切换至电机B"       */
    VOICE_ID_SPEED_UP    = 10,  /**< "转速已提高"        */
    VOICE_ID_SPEED_DOWN  = 11,  /**< "转速已降低"        */
} VoiceId_t;

/* -----------------------------------------------------------------------
 * 播报队列深度
 * --------------------------------------------------------------------- */
#define VOICE_QUEUE_SIZE  8U

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化语音模块（USART6）
 */
void VoiceCtrl_Init(void);

/**
 * @brief  请求播报语音条目（入播报队列）
 * @param  id  语音条目 ID
 */
void VoiceCtrl_Play(VoiceId_t id);

/**
 * @brief  立即播报（抢占当前播报，用于故障报警）
 * @param  id  语音条目 ID
 */
void VoiceCtrl_PlayUrgent(VoiceId_t id);

/**
 * @brief  停止当前播报并清空队列
 */
void VoiceCtrl_Stop(void);

/**
 * @brief  语音控制轮询（由 Task_VoiceCtrl 每 50ms 调用）
 *         检测播报完成，驱动队列
 */
void VoiceCtrl_Update(void);

/**
 * @brief  判断语音芯片当前是否正在播报
 */
bool VoiceCtrl_IsBusy(void);

#endif /* VOICE_CTRL_H */
