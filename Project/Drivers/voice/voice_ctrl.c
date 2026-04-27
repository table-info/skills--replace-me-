/**
 * @file    voice_ctrl.c
 * @brief   语音芯片控制驱动实现（JQ8400 UART 协议）
 *
 * JQ8400 命令格式：$S<CMD><PARAM>*（可选校验和）
 * 此处使用简化的二进制协议：7E FF 06 <CMD> 00 <P1> <P2> EF
 */
#include "voice_ctrl.h"
#include "../../HAL/stm32f4xx_hal.h"
#include <string.h>

/* -----------------------------------------------------------------------
 * JQ8400 / DY-SV5W 串行命令帧（通用格式）
 * --------------------------------------------------------------------- */
#define VOICE_FRAME_HEAD  0x7EU
#define VOICE_FRAME_END   0xEFU
#define VOICE_CMD_PLAY    0x03U   /* 播放指定曲目 */
#define VOICE_CMD_STOP    0x16U   /* 停止播放 */
#define VOICE_CMD_VOLUME  0x06U   /* 设置音量(0-30) */
#define VOICE_VOLUME_DEF  20U     /* 默认音量 */

/* 每个条目估计播报时长（ms），粗略超时判断 */
#define VOICE_PLAY_TIMEOUT_MS  5000U

/* -----------------------------------------------------------------------
 * 播报队列
 * --------------------------------------------------------------------- */
typedef struct {
    VoiceId_t items[VOICE_QUEUE_SIZE];
    uint8_t   head;
    uint8_t   tail;
} VoiceQueue_t;

static VoiceQueue_t s_queue;
static bool         s_busy        = false;
static uint32_t     s_play_start  = 0U;   /* 开始播报的系统 tick */

/* -----------------------------------------------------------------------
 * 私有：发送命令帧到语音芯片
 * --------------------------------------------------------------------- */
static void send_cmd(uint8_t cmd, uint8_t param1, uint8_t param2)
{
    static uint8_t frame[8];
    frame[0] = VOICE_FRAME_HEAD;
    frame[1] = 0xFFU;
    frame[2] = 0x06U;
    frame[3] = cmd;
    frame[4] = 0x00U;
    frame[5] = param1;
    frame[6] = param2;
    frame[7] = VOICE_FRAME_END;
    HAL_UART_Transmit_DMA(&huart6, frame, 8U);
}

/* -----------------------------------------------------------------------
 * 私有：队列操作
 * --------------------------------------------------------------------- */
static bool queue_empty(void)
{
    return s_queue.head == s_queue.tail;
}

static bool queue_push(VoiceId_t id)
{
    uint8_t next = (uint8_t)((s_queue.head + 1U) & (VOICE_QUEUE_SIZE - 1U));
    if (next == s_queue.tail) {
        return false; /* 队列满 */
    }
    s_queue.items[s_queue.head] = id;
    s_queue.head = next;
    return true;
}

static bool queue_pop(VoiceId_t *id)
{
    if (queue_empty()) {
        return false;
    }
    *id = s_queue.items[s_queue.tail];
    s_queue.tail = (uint8_t)((s_queue.tail + 1U) & (VOICE_QUEUE_SIZE - 1U));
    return true;
}

/* -----------------------------------------------------------------------
 * 私有：触发播放一个条目
 * --------------------------------------------------------------------- */
static void play_next(void)
{
    VoiceId_t id;
    if (!queue_pop(&id)) {
        return;
    }
    send_cmd(VOICE_CMD_PLAY, 0x00U, (uint8_t)id);
    s_busy       = true;
    s_play_start = HAL_GetTick();
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void VoiceCtrl_Init(void)
{
    memset(&s_queue, 0, sizeof(s_queue));
    s_busy = false;
    /* 设置默认音量 */
    send_cmd(VOICE_CMD_VOLUME, 0x00U, VOICE_VOLUME_DEF);
}

void VoiceCtrl_Play(VoiceId_t id)
{
    queue_push(id);
}

void VoiceCtrl_PlayUrgent(VoiceId_t id)
{
    /* 清空队列，立即播放 */
    memset(&s_queue, 0, sizeof(s_queue));
    send_cmd(VOICE_CMD_STOP, 0U, 0U);
    send_cmd(VOICE_CMD_PLAY, 0x00U, (uint8_t)id);
    s_busy       = true;
    s_play_start = HAL_GetTick();
}

void VoiceCtrl_Stop(void)
{
    memset(&s_queue, 0, sizeof(s_queue));
    send_cmd(VOICE_CMD_STOP, 0U, 0U);
    s_busy = false;
}

bool VoiceCtrl_IsBusy(void)
{
    return s_busy;
}

void VoiceCtrl_Update(void)
{
    if (!s_busy) {
        /* 不在播放，检查队列 */
        if (!queue_empty()) {
            play_next();
        }
        return;
    }

    /* 优先读取 BUSY 引脚（低电平=播放中，高电平=空闲） */
    uint32_t busy_pin = HAL_GPIO_ReadPin(GPIOB, VOICE_BUSY_PIN);
    if (busy_pin == GPIO_PIN_SET) {
        /* 引脚表明播放完成 */
        s_busy = false;
        return;
    }

    /* 兜底超时检测 */
    if ((HAL_GetTick() - s_play_start) >= VOICE_PLAY_TIMEOUT_MS) {
        s_busy = false;
    }
}
