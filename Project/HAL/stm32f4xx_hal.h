/**
 * @file    stm32f4xx_hal.h
 * @brief   STM32F4xx HAL 抽象层桩头文件
 *
 * 在实际项目中请替换为 STM32CubeMX 生成的 HAL 库头文件。
 * 此文件仅作为编译占位与接口说明。
 */
#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H

#include <stdint.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * 基础类型别名
 * --------------------------------------------------------------------- */
typedef uint8_t  HAL_StatusTypeDef;   /* 0=OK, 1=ERROR, 2=BUSY, 3=TIMEOUT */

#define HAL_OK      ((HAL_StatusTypeDef)0x00U)
#define HAL_ERROR   ((HAL_StatusTypeDef)0x01U)
#define HAL_BUSY    ((HAL_StatusTypeDef)0x02U)
#define HAL_TIMEOUT ((HAL_StatusTypeDef)0x03U)

/* -----------------------------------------------------------------------
 * GPIO
 * --------------------------------------------------------------------- */
#define GPIO_PIN_SET   1U
#define GPIO_PIN_RESET 0U

typedef struct { uint32_t dummy; } GPIO_TypeDef;

void     HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint32_t PinState);
uint32_t HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

/* -----------------------------------------------------------------------
 * TIM (PWM / 编码器)
 * --------------------------------------------------------------------- */
typedef struct { uint32_t dummy; } TIM_HandleTypeDef;

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

#define TIM_CHANNEL_1  0x00000000U
#define TIM_CHANNEL_2  0x00000004U
#define TIM_CHANNEL_3  0x00000008U

/* -----------------------------------------------------------------------
 * UART (DMA)
 * --------------------------------------------------------------------- */
typedef struct { uint32_t dummy; } UART_HandleTypeDef;

HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *huart,
                                         const uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef *huart,
                                        uint8_t *pData, uint16_t Size);

/* -----------------------------------------------------------------------
 * SPI
 * --------------------------------------------------------------------- */
typedef struct { uint32_t dummy; } SPI_HandleTypeDef;

HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi,
                                           const uint8_t *pTxData,
                                           uint8_t *pRxData,
                                           uint16_t Size, uint32_t Timeout);

/* -----------------------------------------------------------------------
 * IWDG
 * --------------------------------------------------------------------- */
typedef struct { uint32_t dummy; } IWDG_HandleTypeDef;

HAL_StatusTypeDef HAL_IWDG_Refresh(IWDG_HandleTypeDef *hiwdg);

/* -----------------------------------------------------------------------
 * 系统时间
 * --------------------------------------------------------------------- */
uint32_t HAL_GetTick(void);   /* 返回自启动以来的毫秒数 */
void     HAL_Delay(uint32_t Delay);

/* -----------------------------------------------------------------------
 * 外部句柄声明（由 main.c / CubeMX 生成文件定义）
 * --------------------------------------------------------------------- */
extern TIM_HandleTypeDef  htim1;   /* 主轴电机 */
extern TIM_HandleTypeDef  htim2;   /* 刨削电机A */
extern TIM_HandleTypeDef  htim3;   /* 刨削电机B */
extern UART_HandleTypeDef huart2;  /* 迪文屏 */
extern UART_HandleTypeDef huart3;  /* RFID (UART模式) */
extern UART_HandleTypeDef huart6;  /* 语音芯片 */
extern SPI_HandleTypeDef  hspi1;   /* RFID (SPI模式) */
extern IWDG_HandleTypeDef hiwdg;   /* 独立看门狗 */

/* -----------------------------------------------------------------------
 * GPIO 端口/引脚宏（示例，根据实际原理图调整）
 * --------------------------------------------------------------------- */
extern GPIO_TypeDef *GPIOA;
extern GPIO_TypeDef *GPIOB;
extern GPIO_TypeDef *GPIOC;

#define MOTOR_MAIN_EN_PIN    0x0001U  /* PA0: 主轴使能 */
#define MOTOR_A_EN_PIN       0x0002U  /* PA1: 刨削A使能 */
#define MOTOR_B_EN_PIN       0x0004U  /* PA2: 刨削B使能 */
#define RFID_CS_PIN          0x0100U  /* PB8: SPI片选 */
#define VOICE_BUSY_PIN       0x0200U  /* PB9: 语音忙信号 */

#endif /* STM32F4XX_HAL_H */
