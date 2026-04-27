/**
 * @file    diwin_comm.h
 * @brief   迪文（DGUS）显示屏通信驱动
 *
 * 协议格式（T5L/T5S DGUS II）：
 *   帧头(0x5A 0xA5) + 长度 + 指令码 + VP地址(2B) + 数据
 *
 * 接口：USART2 + DMA 双向
 * - 接收：解析 VP 地址对应操作，推送事件到全局事件队列
 * - 发送：主动刷新电机状态、转速、故障信息到屏幕寄存器
 */
#ifndef DIWIN_COMM_H
#define DIWIN_COMM_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * DGUS 协议常量
 * --------------------------------------------------------------------- */
#define DGUS_FRAME_HEADER_0  0x5AU
#define DGUS_FRAME_HEADER_1  0xA5U
#define DGUS_CMD_WRITE       0x82U
#define DGUS_CMD_READ        0x83U

/* -----------------------------------------------------------------------
 * VP 地址定义（根据 HMI 工程配置，仅示例）
 * --------------------------------------------------------------------- */
#define VP_BTN_START         0x1000U  /**< 屏幕"启动"按钮 VP      */
#define VP_BTN_STOP          0x1001U  /**< 屏幕"停止"按钮 VP      */
#define VP_SPEED_SET         0x1010U  /**< 转速设定值 VP           */
#define VP_MOTOR_STATUS      0x2000U  /**< 电机状态显示 VP（只读） */
#define VP_MOTOR_SPEED_DISP  0x2001U  /**< 转速显示 VP（只读）     */
#define VP_FAULT_CODE        0x2010U  /**< 故障码显示 VP（只读）   */

/* -----------------------------------------------------------------------
 * 接收缓冲区大小
 * --------------------------------------------------------------------- */
#define DGUS_RX_BUF_SIZE  64U
#define DGUS_TX_BUF_SIZE  32U

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化迪文屏通信（配置 USART2 DMA 接收）
 */
void DiwinComm_Init(void);

/**
 * @brief  周期性通信处理（由 Task_DiwinComm 每 10ms 调用）
 *         - 解析已接收的 DMA 数据帧
 *         - 发送主动上报帧
 */
void DiwinComm_Process(void);

/**
 * @brief  向屏幕写入一个 16 位字（主动上报）
 * @param  vp    VP 寄存器地址
 * @param  value 16 位数据
 */
void DiwinComm_WriteVP(uint16_t vp, uint16_t value);

/**
 * @brief  刷新电机状态显示（状态码 + 转速）
 * @param  status    系统状态码（0=IDLE, 1=RUNNING, 2=FAULT...）
 * @param  speed_rpm 当前转速 rpm
 */
void DiwinComm_UpdateMotorStatus(uint16_t status, uint16_t speed_rpm);

/**
 * @brief  刷新故障码显示
 * @param  fault_code  0=无故障，非零=故障码
 */
void DiwinComm_UpdateFaultCode(uint16_t fault_code);

/**
 * @brief  USART2 DMA 接收完成回调（在 HAL 回调中调用）
 */
void DiwinComm_RxCpltCallback(void);

/**
 * @brief  USART2 DMA 发送完成回调（在 HAL_UART_TxCpltCallback 中调用）
 */
void DiwinComm_TxCpltCallback(void);

#endif /* DIWIN_COMM_H */
