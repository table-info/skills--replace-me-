/**
 * @file    rfid_rc522.h
 * @brief   13.56 MHz RFID 读卡模块驱动（MFRC522 / PN532 SPI 接口）
 *
 * 功能：
 *  - 100ms 轮询检测卡片
 *  - 读取 4 字节 UID
 *  - 校验白名单，推送 EVT_RFID_CARD_READ 或 EVT_RFID_CARD_INVALID
 *  - 支持通过卡号选择刨削电机 A 或 B
 *
 * 硬件：SPI1 + NSS(PB8)
 */
#ifndef RFID_RC522_H
#define RFID_RC522_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * UID 最大长度（MIFARE Classic = 4 字节）
 * --------------------------------------------------------------------- */
#define RFID_UID_MAX_LEN  10U

/* -----------------------------------------------------------------------
 * RFID 卡信息结构体
 * --------------------------------------------------------------------- */
typedef struct {
    uint8_t  uid[RFID_UID_MAX_LEN];
    uint8_t  uid_len;
    uint32_t uid32;    /**< UID 低 4 字节压缩为 uint32_t，用于事件 param */
} RFID_Card_t;

/* -----------------------------------------------------------------------
 * 白名单最大条目数
 * --------------------------------------------------------------------- */
#define RFID_WHITELIST_MAX  16U

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化 RFID 模块（SPI1 + RC522 复位）
 */
void RFID_Init(void);

/**
 * @brief  轮询读卡（由 Task_RFID 每 100ms 调用）
 *         检测到卡片后校验白名单并推送事件
 */
void RFID_Poll(void);

/**
 * @brief  向白名单添加一条记录
 * @param  uid32  卡 UID 低 32 位
 * @return true=添加成功，false=白名单已满
 */
bool RFID_Whitelist_Add(uint32_t uid32);

/**
 * @brief  检查 UID 是否在白名单内
 * @param  uid32  卡 UID 低 32 位
 * @return true=授权，false=未授权
 */
bool RFID_Whitelist_Check(uint32_t uid32);

/**
 * @brief  读取最近一次成功读到的卡信息
 */
const RFID_Card_t *RFID_GetLastCard(void);

#endif /* RFID_RC522_H */
