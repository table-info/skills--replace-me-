# 嵌入式刨削器控制系统

基于 **STM32F407** 的刨削器控制固件，采用**时间片 + 事件驱动**架构。

---

## 硬件资源

| 外设 | 接口/资源 | 说明 |
|------|-----------|------|
| 主轴电机（常运行）| TIM1 互补 PWM + GPIOA | 无刷 BLDC，3 路 PWM |
| 刨削电机 A | TIM2 PWM + GPIOA | 同型号，互斥运行 |
| 刨削电机 B | TIM3 PWM + GPIOA | 同型号，互斥运行 |
| 手柄按键 | EXTI(PC6) | 双沿触发 |
| 脚踏开关 | EXTI(PC7) | 双沿触发 |
| 迪文显示屏 | USART2 + DMA | DGUS II 协议 |
| RFID（13.56 MHz）| SPI1 + GPIOB(CS) | MFRC522 |
| 语音芯片 | USART6 | JQ8400/DY-SV5W |
| 看门狗 | IWDG | 系统可靠性 |

---

## 软件架构

```
┌──────────────────────────────────────────┐
│              应用层 (App Layer)            │
│  app_logic.c — 事件回调 / 业务协调        │
├──────────────────────────────────────────┤
│           事件调度层 (Event Layer)         │
│  event_queue.c — 环形队列（无锁）         │
│  sys_state.c  — 系统主状态机              │
├──────────────────────────────────────────┤
│           时间片调度层 (Scheduler)         │
│  scheduler.c  — SysTick 驱动任务表        │
├──────────────────────────────────────────┤
│           驱动层 (Driver Layer)            │
│  motor_main.c / motor_blade.c             │
│  diwin_comm.c / rfid_rc522.c              │
│  voice_ctrl.c / switch_input.c            │
└──────────────────────────────────────────┘
```

### 调度任务表

| 任务 | 周期 | 说明 |
|------|------|------|
| `Task_MotorCtrl` | 1 ms | 电机斜坡/故障检测/输入消抖 |
| `Task_EventDispatch` | 2 ms | 事件队列轮询 → 状态机 |
| `Task_DiwinComm` | 10 ms | 迪文屏收发 |
| `Task_VoiceCtrl` | 50 ms | 语音播报管理 |
| `Task_RFID` | 100 ms | RFID 轮询读卡 |
| `Task_SysMon` | 500 ms | 喂狗 + 刷新屏幕 |

### 系统状态机

```
SYSTEM_INIT → SYSTEM_IDLE ⇄ SYSTEM_RUNNING → SYSTEM_STOPPING → SYSTEM_IDLE
                    ↓ (任意状态 + EVT_MOTOR_FAULT)
               SYSTEM_FAULT
```

---

## 目录结构

```
Project/
├── Core/
│   ├── main.c            系统入口、中断回调
│   ├── scheduler.c/h     时间片调度器
│   ├── event_queue.c/h   环形事件队列
│   └── sys_state.c/h     系统主状态机
├── Drivers/
│   ├── motor/
│   │   ├── motor_main.c/h   主轴电机
│   │   └── motor_blade.c/h  刨削电机 A/B 互斥控制
│   ├── diwin/
│   │   └── diwin_comm.c/h   迪文屏 DGUS 协议
│   ├── rfid/
│   │   └── rfid_rc522.c/h   MFRC522 驱动
│   ├── voice/
│   │   └── voice_ctrl.c/h   语音芯片控制
│   └── input/
│       └── switch_input.c/h 手柄/脚踏消抖中断
├── HAL/
│   └── stm32f4xx_hal.h      HAL 桩头文件（替换为 CubeMX 生成库）
└── App/
    └── app_logic.c/h        应用业务逻辑（模块粘合层）
```

---

## 移植说明

1. 使用 **STM32CubeMX** 生成 HAL 初始化代码，替换 `HAL/stm32f4xx_hal.h` 中的桩函数。
2. 在 CubeMX 中配置：
   - SysTick = 1 ms（默认）
   - TIM1 高级定时器：互补 PWM，频率建议 20 kHz
   - TIM2/TIM3：单路 PWM，同频率
   - USART2 DMA 双向（迪文屏）
   - USART6 TX DMA（语音芯片）
   - SPI1 主模式（RFID）
   - EXTI PC6/PC7 双沿触发
   - IWDG 超时 ≥ 1 s
3. 根据实际原理图修改 `HAL/stm32f4xx_hal.h` 中的引脚宏定义。
4. 在 `rfid_rc522.c` 中调用 `RFID_Whitelist_Add()` 预置授权卡号。

---

## 关键设计原则

- **中断只做最少的事**：ISR 仅推送事件，不做业务逻辑。
- **电机互斥保护**：状态机 + 软件标志位双重保护。
- **故障最高优先级**：`EVT_MOTOR_FAULT` 直接跳转 `SYSTEM_FAULT`。
- **无锁队列**：单生产者（中断）/ 单消费者（主循环），无需关中断。
- **看门狗由最低优先级任务喂**：确保主循环健康才喂狗。
