/**
 * @file    rfid_rc522.c
 * @brief   MFRC522 RFID 驱动实现（SPI1 接口）
 *
 * 关键寄存器操作通过 SPI 读写，100ms 轮询检测卡片，
 * 读到卡后校验白名单并推送事件。
 */
#include "rfid_rc522.h"
#include "../../HAL/stm32f4xx_hal.h"
#include "../../Core/event_queue.h"
#include <string.h>

/* -----------------------------------------------------------------------
 * RC522 寄存器地址（部分常用）
 * --------------------------------------------------------------------- */
#define RC522_REG_COMMAND      0x01U
#define RC522_REG_COM_IEN      0x02U
#define RC522_REG_COM_IRQ      0x04U
#define RC522_REG_ERROR        0x06U
#define RC522_REG_FIFO_DATA    0x09U
#define RC522_REG_FIFO_LEVEL   0x0AU
#define RC522_REG_CONTROL      0x0CU
#define RC522_REG_BIT_FRAMING  0x0DU
#define RC522_REG_MODE         0x11U
#define RC522_REG_TX_CONTROL   0x14U
#define RC522_REG_T_MODE       0x2AU
#define RC522_REG_T_PRESCALER  0x2BU
#define RC522_REG_T_RELOAD_H   0x2CU
#define RC522_REG_T_RELOAD_L   0x2DU

#define RC522_CMD_IDLE         0x00U
#define RC522_CMD_TRANSCEIVE   0x0CU
#define RC522_CMD_SOFT_RESET   0x0FU

#define PICC_REQIDL            0x26U  /* REQA（休眠中的卡） */
#define PICC_ANTICOLL          0x93U  /* 防碰撞 */

/* -----------------------------------------------------------------------
 * 私有状态
 * --------------------------------------------------------------------- */
static RFID_Card_t   s_last_card;
static uint32_t      s_whitelist[RFID_WHITELIST_MAX];
static uint8_t       s_whitelist_count = 0U;

/* -----------------------------------------------------------------------
 * SPI 读写辅助
 * --------------------------------------------------------------------- */
static void cs_low(void)
{
    HAL_GPIO_WritePin(GPIOB, RFID_CS_PIN, GPIO_PIN_RESET);
}

static void cs_high(void)
{
    HAL_GPIO_WritePin(GPIOB, RFID_CS_PIN, GPIO_PIN_SET);
}

static void rc522_write(uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = { (uint8_t)((reg << 1U) & 0x7EU), value };
    uint8_t rx[2];
    cs_low();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2U, 10U);
    cs_high();
}

static uint8_t rc522_read(uint8_t reg)
{
    uint8_t tx[2] = { (uint8_t)(((reg << 1U) & 0x7EU) | 0x80U), 0x00U };
    uint8_t rx[2] = {0U};
    cs_low();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2U, 10U);
    cs_high();
    return rx[1];
}

static void rc522_set_bit(uint8_t reg, uint8_t mask)
{
    rc522_write(reg, rc522_read(reg) | mask);
}

/* -----------------------------------------------------------------------
 * 私有：RC522 天线使能
 * --------------------------------------------------------------------- */
static void rc522_antenna_on(void)
{
    uint8_t val = rc522_read(RC522_REG_TX_CONTROL);
    if ((val & 0x03U) != 0x03U) {
        rc522_set_bit(RC522_REG_TX_CONTROL, 0x03U);
    }
}

/* -----------------------------------------------------------------------
 * 私有：发送 REQA，检测是否有卡响应
 * 返回 true=有卡，false=无卡
 * --------------------------------------------------------------------- */
static bool rc522_detect_card(void)
{
    rc522_write(RC522_REG_BIT_FRAMING, 0x07U);
    rc522_write(RC522_REG_COMMAND, RC522_CMD_IDLE);
    rc522_write(RC522_REG_FIFO_DATA, PICC_REQIDL);
    rc522_write(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    rc522_set_bit(RC522_REG_BIT_FRAMING, 0x80U); /* StartSend */

    /* 等待中断（简单轮询，超时 25ms） */
    for (uint16_t i = 0U; i < 2500U; i++) {
        uint8_t irq = rc522_read(RC522_REG_COM_IRQ);
        if (irq & 0x30U) break;   /* RxIRq 或 IdleIRq */
        if (irq & 0x01U) return false; /* TimerIRq = 超时 */
    }

    uint8_t error = rc522_read(RC522_REG_ERROR);
    if (error & 0x1BU) return false;  /* 错误标志 */

    uint8_t fifo_level = rc522_read(RC522_REG_FIFO_LEVEL);
    return (fifo_level >= 2U);
}

/* -----------------------------------------------------------------------
 * 私有：防碰撞，读取 UID
 * 返回 true=成功，false=失败
 * --------------------------------------------------------------------- */
static bool rc522_read_uid(RFID_Card_t *card)
{
    uint8_t buf[5] = { PICC_ANTICOLL, 0x20U, 0U, 0U, 0U };

    rc522_write(RC522_REG_BIT_FRAMING, 0x00U);
    rc522_write(RC522_REG_COMMAND, RC522_CMD_IDLE);

    /* 写 FIFO */
    for (uint8_t i = 0U; i < 2U; i++) {
        rc522_write(RC522_REG_FIFO_DATA, buf[i]);
    }
    rc522_write(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    rc522_set_bit(RC522_REG_BIT_FRAMING, 0x80U);

    for (uint16_t i = 0U; i < 2500U; i++) {
        uint8_t irq = rc522_read(RC522_REG_COM_IRQ);
        if (irq & 0x30U) break;
        if (irq & 0x01U) return false;
    }

    uint8_t err = rc522_read(RC522_REG_ERROR);
    if (err & 0x1BU) return false;

    uint8_t level = rc522_read(RC522_REG_FIFO_LEVEL);
    if (level < 5U) return false;

    uint8_t check = 0U;
    for (uint8_t i = 0U; i < 4U; i++) {
        card->uid[i] = rc522_read(RC522_REG_FIFO_DATA);
        check ^= card->uid[i];
    }
    /* BCC 校验 */
    uint8_t bcc = rc522_read(RC522_REG_FIFO_DATA);
    if (check != bcc) return false;

    card->uid_len = 4U;
    card->uid32   = ((uint32_t)card->uid[0])
                  | ((uint32_t)card->uid[1] << 8U)
                  | ((uint32_t)card->uid[2] << 16U)
                  | ((uint32_t)card->uid[3] << 24U);
    return true;
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void RFID_Init(void)
{
    memset(&s_last_card, 0, sizeof(s_last_card));
    s_whitelist_count = 0U;

    cs_high();
    HAL_Delay(10U);

    /* 软复位 */
    rc522_write(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    HAL_Delay(10U);

    /* 配置定时器（25ms 超时） */
    rc522_write(RC522_REG_T_MODE,      0x80U);
    rc522_write(RC522_REG_T_PRESCALER, 0xA9U);
    rc522_write(RC522_REG_T_RELOAD_H,  0x03U);
    rc522_write(RC522_REG_T_RELOAD_L,  0xE8U);
    rc522_write(RC522_REG_MODE,        0x3DU);

    rc522_antenna_on();
}

void RFID_Poll(void)
{
    if (!rc522_detect_card()) {
        return;
    }

    RFID_Card_t card;
    memset(&card, 0, sizeof(card));

    if (!rc522_read_uid(&card)) {
        return;
    }

    s_last_card = card;

    if (RFID_Whitelist_Check(card.uid32)) {
        EVT_PUSH(EVT_RFID_CARD_READ, card.uid32);
    } else {
        EVT_PUSH(EVT_RFID_CARD_INVALID, card.uid32);
    }
}

bool RFID_Whitelist_Add(uint32_t uid32)
{
    if (s_whitelist_count >= RFID_WHITELIST_MAX) {
        return false;
    }
    s_whitelist[s_whitelist_count++] = uid32;
    return true;
}

bool RFID_Whitelist_Check(uint32_t uid32)
{
    for (uint8_t i = 0U; i < s_whitelist_count; i++) {
        if (s_whitelist[i] == uid32) {
            return true;
        }
    }
    return false;
}

const RFID_Card_t *RFID_GetLastCard(void)
{
    return &s_last_card;
}
