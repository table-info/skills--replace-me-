/**
 * @file    event_queue.h
 * @brief   环形事件队列（无锁单生产者-单消费者）
 *
 * 设计原则：
 *  - 中断/驱动层调用 EventQueue_Push() 写入事件
 *  - 主循环（Task_EventDispatch）调用 EventQueue_Pop() 消费事件
 *  - 写指针只由生产者更新，读指针只由消费者更新，无需关中断即可安全操作
 */
#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * 事件类型枚举
 * --------------------------------------------------------------------- */
typedef enum {
    EVT_NONE = 0,

    /* 输入事件 */
    EVT_HANDLE_BTN_PRESS,    /**< 手柄按键按下   */
    EVT_HANDLE_BTN_RELEASE,  /**< 手柄按键释放   */
    EVT_FOOT_SW_PRESS,       /**< 脚踏开关触发   */
    EVT_FOOT_SW_RELEASE,     /**< 脚踏开关释放   */

    /* 迪文屏指令事件 */
    EVT_DIWIN_START,         /**< 屏幕发出启动指令  */
    EVT_DIWIN_STOP,          /**< 屏幕发出停止指令  */
    EVT_DIWIN_SPEED,         /**< 屏幕调速，param=目标占空比(0-1000‰) */

    /* RFID 事件 */
    EVT_RFID_CARD_READ,      /**< 读到有效卡，param=卡号低32位 */
    EVT_RFID_CARD_INVALID,   /**< 读到无效/未授权卡            */

    /* 电机事件 */
    EVT_MOTOR_FAULT,         /**< 电机故障，param=故障码       */
    EVT_MOTOR_STARTED,       /**< 刨削电机已达运行转速          */
    EVT_MOTOR_STOPPED,       /**< 刨削电机已完全停止            */

    /* 语音事件 */
    EVT_VOICE_PLAY,          /**< 请求播报，param=语音条目ID    */
    EVT_VOICE_DONE,          /**< 播报完成                      */

    EVT_MAX
} EventType_t;

/* -----------------------------------------------------------------------
 * 事件结构体
 * --------------------------------------------------------------------- */
typedef struct {
    EventType_t type;   /**< 事件类型 */
    uint32_t    param;  /**< 附带参数（速度、卡号、故障码等） */
} Event_t;

/* -----------------------------------------------------------------------
 * 队列容量（必须为 2 的幂，便于掩码运算）
 * --------------------------------------------------------------------- */
#define EVENT_QUEUE_SIZE  32U

/* -----------------------------------------------------------------------
 * 队列结构体（可多实例）
 * --------------------------------------------------------------------- */
typedef struct {
    Event_t  buf[EVENT_QUEUE_SIZE];
    volatile uint32_t head;   /**< 写指针（生产者维护） */
    volatile uint32_t tail;   /**< 读指针（消费者维护） */
} EventQueue_t;

/* -----------------------------------------------------------------------
 * API
 * --------------------------------------------------------------------- */

/**
 * @brief  初始化事件队列
 * @param  q  队列指针
 */
void EventQueue_Init(EventQueue_t *q);

/**
 * @brief  向队列压入事件（可在中断中调用）
 * @param  q     队列指针
 * @param  evt   要压入的事件
 * @return true=成功，false=队列满丢弃
 */
bool EventQueue_Push(EventQueue_t *q, const Event_t *evt);

/**
 * @brief  从队列弹出事件（主循环调用）
 * @param  q     队列指针
 * @param  evt   输出事件
 * @return true=成功，false=队列空
 */
bool EventQueue_Pop(EventQueue_t *q, Event_t *evt);

/**
 * @brief  判断队列是否为空
 */
bool EventQueue_IsEmpty(const EventQueue_t *q);

/**
 * @brief  判断队列是否已满
 */
bool EventQueue_IsFull(const EventQueue_t *q);

/**
 * @brief  获取全局事件队列单例（由 event_queue.c 维护）
 */
EventQueue_t *EventQueue_GetGlobal(void);

/**
 * @brief  便捷宏：向全局队列压入事件
 */
#define EVT_PUSH(type_, param_)  \
    do {                         \
        Event_t _e = { (type_), (uint32_t)(param_) }; \
        EventQueue_Push(EventQueue_GetGlobal(), &_e);  \
    } while (0)

#endif /* EVENT_QUEUE_H */
