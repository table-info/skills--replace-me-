/**
 * @file    event_queue.c
 * @brief   环形事件队列实现（无锁单生产者-单消费者）
 */
#include "event_queue.h"
#include <string.h>

/* 全局队列单例 */
static EventQueue_t s_global_queue;

/* -----------------------------------------------------------------------
 * 私有辅助
 * --------------------------------------------------------------------- */
static inline uint32_t next_idx(uint32_t idx)
{
    return (idx + 1U) & (EVENT_QUEUE_SIZE - 1U);
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void EventQueue_Init(EventQueue_t *q)
{
    memset(q, 0, sizeof(EventQueue_t));
}

bool EventQueue_IsFull(const EventQueue_t *q)
{
    return next_idx(q->head) == q->tail;
}

bool EventQueue_IsEmpty(const EventQueue_t *q)
{
    return q->head == q->tail;
}

bool EventQueue_Push(EventQueue_t *q, const Event_t *evt)
{
    if (EventQueue_IsFull(q)) {
        /* 队列满：丢弃事件，保护生产者不阻塞 */
        return false;
    }
    q->buf[q->head] = *evt;
    /* 写屏障：确保数据写入在 head 更新之前对消费者可见
     * 在 Cortex-M4 上编译器内存屏障即可（不需要 DMB，因为单核） */
    __asm volatile("" ::: "memory");
    q->head = next_idx(q->head);
    return true;
}

bool EventQueue_Pop(EventQueue_t *q, Event_t *evt)
{
    if (EventQueue_IsEmpty(q)) {
        return false;
    }
    *evt = q->buf[q->tail];
    __asm volatile("" ::: "memory");
    q->tail = next_idx(q->tail);
    return true;
}

EventQueue_t *EventQueue_GetGlobal(void)
{
    return &s_global_queue;
}
