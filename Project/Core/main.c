/**
 * @file    main.c
 * @brief   系统入口：初始化 + 主循环
 *
 * 硬件初始化（时钟、GPIO、TIM、UART、SPI、IWDG）由 STM32CubeMX
 * 生成的 MX_XXX_Init() 函数完成（在此以注释形式占位）。
 *
 * 主循环仅调用 Scheduler_Run()，所有业务逻辑由调度器分发到各任务函数。
 *
 * SysTick_Handler 每 1ms 调用 Scheduler_Tick()，驱动时间片计数。
 *
 * 外部中断回调（HAL_GPIO_EXTI_Callback）向事件队列推送输入事件。
 * UART 接收完成回调通知迪文屏驱动处理新帧。
 */
#include "../Core/scheduler.h"
#include "../Core/event_queue.h"
#include "../App/app_logic.h"
#include "../Drivers/input/switch_input.h"
#include "../Drivers/diwin/diwin_comm.h"
#include "../HAL/stm32f4xx_hal.h"

/* -----------------------------------------------------------------------
 * HAL 句柄定义（实际由 CubeMX 生成，此处占位）
 * --------------------------------------------------------------------- */
TIM_HandleTypeDef  htim1;
TIM_HandleTypeDef  htim2;
TIM_HandleTypeDef  htim3;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;
SPI_HandleTypeDef  hspi1;
IWDG_HandleTypeDef hiwdg;

GPIO_TypeDef *GPIOA;
GPIO_TypeDef *GPIOB;
GPIO_TypeDef *GPIOC;

/* -----------------------------------------------------------------------
 * 硬件外设初始化（CubeMX 生成，此处声明占位）
 * --------------------------------------------------------------------- */
static void SystemClock_Config(void)  { /* CubeMX 生成 */ }
static void MX_GPIO_Init(void)        { /* CubeMX 生成 */ }
static void MX_TIM1_Init(void)        { /* CubeMX 生成 */ }
static void MX_TIM2_Init(void)        { /* CubeMX 生成 */ }
static void MX_TIM3_Init(void)        { /* CubeMX 生成 */ }
static void MX_USART2_UART_Init(void) { /* CubeMX 生成 */ }
static void MX_USART3_UART_Init(void) { /* CubeMX 生成 */ }
static void MX_USART6_UART_Init(void) { /* CubeMX 生成 */ }
static void MX_SPI1_Init(void)        { /* CubeMX 生成 */ }
static void MX_IWDG_Init(void)        { /* CubeMX 生成 */ }

/* -----------------------------------------------------------------------
 * 主函数
 * --------------------------------------------------------------------- */
int main(void)
{
    /* 1. HAL 库初始化（配置 SysTick 为 1ms） */
    /* HAL_Init(); */

    /* 2. 系统时钟配置（168 MHz） */
    SystemClock_Config();

    /* 3. 外设初始化 */
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_USART2_UART_Init();
    MX_USART3_UART_Init();
    MX_USART6_UART_Init();
    MX_SPI1_Init();
    MX_IWDG_Init();

    /* 4. 应用层初始化（调度器 + 所有驱动模块 + 状态机） */
    AppLogic_Init();

    /* 5. 主循环 */
    while (1) {
        Scheduler_Run();
    }
}

/* -----------------------------------------------------------------------
 * SysTick 中断（每 1ms）
 * 注意：HAL_IncTick() 也在此处调用（或在 HAL_Init 注册的弱函数中）
 * --------------------------------------------------------------------- */
void SysTick_Handler(void)
{
    /* HAL_IncTick(); */   /* 维护 HAL_GetTick() */
    Scheduler_Tick();
}

/* -----------------------------------------------------------------------
 * GPIO 外部中断回调
 * HAL 库在对应 EXTI_IRQHandler 中调用此弱函数
 * --------------------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* 手柄按键引脚（PC6） */
    if (GPIO_Pin == 0x0040U) {
        bool rising = (HAL_GPIO_ReadPin(GPIOC, GPIO_Pin) == GPIO_PIN_SET);
        SwitchInput_HandleBtnIRQ(rising);
    }
    /* 脚踏开关引脚（PC7） */
    else if (GPIO_Pin == 0x0080U) {
        bool rising = (HAL_GPIO_ReadPin(GPIOC, GPIO_Pin) == GPIO_PIN_SET);
        SwitchInput_HandleFootIRQ(rising);
    }
}

/* -----------------------------------------------------------------------
 * UART DMA 接收完成回调
 * --------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) {
        DiwinComm_RxCpltCallback();
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) {
        DiwinComm_TxCpltCallback();
    }
}
