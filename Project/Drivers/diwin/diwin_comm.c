/**
 * @file    diwin_comm.c
 * @brief   迪文（DGUS II）显示屏通信驱动实现
 *
 * 接收解析 VP 数据帧，推送对应事件；
 * 周期性主动上报电机状态、转速、故障码到屏幕 VP 寄存器。
 */
#include "diwin_comm.h"
#include "../../HAL/stm32f4xx_hal.h"
#include "../../Core/event_queue.h"
#include <string.h>

/* -----------------------------------------------------------------------
 * 私有缓冲区与状态
 * --------------------------------------------------------------------- */
static uint8_t s_rx_buf[DGUS_RX_BUF_SIZE];
static uint8_t s_tx_buf[DGUS_TX_BUF_SIZE];
static bool    s_rx_ready  = false;  /* DMA 接收完成标志 */
static bool    s_tx_busy   = false;  /* DMA 发送进行中标志，防止缓冲区覆盖 */

/* -----------------------------------------------------------------------
 * 私有：构建写 VP 帧
 *   格式：5A A5 <len> 82 <VP_H> <VP_L> <DATA_H> <DATA_L>
 * --------------------------------------------------------------------- */
static uint8_t build_write_frame(uint8_t *buf, uint16_t vp, uint16_t value)
{
    buf[0] = DGUS_FRAME_HEADER_0;
    buf[1] = DGUS_FRAME_HEADER_1;
    buf[2] = 0x05U;           /* 长度：CMD(1) + VP(2) + DATA(2) */
    buf[3] = DGUS_CMD_WRITE;
    buf[4] = (uint8_t)(vp >> 8U);
    buf[5] = (uint8_t)(vp & 0xFFU);
    buf[6] = (uint8_t)(value >> 8U);
    buf[7] = (uint8_t)(value & 0xFFU);
    return 8U;
}

/* -----------------------------------------------------------------------
 * 私有：解析一帧接收数据，推送对应事件
 * 帧格式：5A A5 <len> 83 <VP_H> <VP_L> <word_count> <DATA...>
 * --------------------------------------------------------------------- */
static void parse_rx_frame(const uint8_t *buf, uint8_t len)
{
    if (len < 7U) return;
    if (buf[0] != DGUS_FRAME_HEADER_0 || buf[1] != DGUS_FRAME_HEADER_1) return;
    if (buf[3] != DGUS_CMD_READ) return;  /* 只处理屏幕主动上报（读通知） */

    uint16_t vp    = ((uint16_t)buf[4] << 8U) | buf[5];
    uint16_t value = ((uint16_t)buf[7] << 8U) | buf[8];

    switch (vp) {
    case VP_BTN_START:
        if (value != 0U) {
            EVT_PUSH(EVT_DIWIN_START, 0U);
        }
        break;
    case VP_BTN_STOP:
        if (value != 0U) {
            EVT_PUSH(EVT_DIWIN_STOP, 0U);
        }
        break;
    case VP_SPEED_SET:
        EVT_PUSH(EVT_DIWIN_SPEED, (uint32_t)value);
        break;
    default:
        break;
    }
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void DiwinComm_Init(void)
{
    memset(s_rx_buf, 0, sizeof(s_rx_buf));
    s_rx_ready = false;
    /* 启动 DMA 接收（循环模式） */
    HAL_UART_Receive_DMA(&huart2, s_rx_buf, DGUS_RX_BUF_SIZE);
}

void DiwinComm_Process(void)
{
    if (s_rx_ready) {
        s_rx_ready = false;
        parse_rx_frame(s_rx_buf, DGUS_RX_BUF_SIZE);
        /* 重启 DMA 接收 */
        HAL_UART_Receive_DMA(&huart2, s_rx_buf, DGUS_RX_BUF_SIZE);
    }
}

void DiwinComm_WriteVP(uint16_t vp, uint16_t value)
{
    /* Skip if a previous DMA transfer is still in progress */
    if (s_tx_busy) {
        return;
    }
    uint8_t len = build_write_frame(s_tx_buf, vp, value);
    s_tx_busy = true;
    HAL_UART_Transmit_DMA(&huart2, s_tx_buf, len);
}

void DiwinComm_UpdateMotorStatus(uint16_t status, uint16_t speed_rpm)
{
    DiwinComm_WriteVP(VP_MOTOR_STATUS, status);
    DiwinComm_WriteVP(VP_MOTOR_SPEED_DISP, speed_rpm);
}

void DiwinComm_UpdateFaultCode(uint16_t fault_code)
{
    DiwinComm_WriteVP(VP_FAULT_CODE, fault_code);
}

void DiwinComm_RxCpltCallback(void)
{
    s_rx_ready = true;
}

/**
 * @brief  USART2 DMA 发送完成回调（在 HAL_UART_TxCpltCallback 中调用）
 *         清除忙标志，允许下一次 WriteVP
 */
void DiwinComm_TxCpltCallback(void)
{
    s_tx_busy = false;
}
