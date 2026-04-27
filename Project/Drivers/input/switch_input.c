/**
 * @file    switch_input.c
 * @brief   手柄按键与脚踏开关中断输入驱动实现
 *
 * 中断 ISR 仅记录粗滤状态，消抖由 SwitchInput_DebounceUpdate() 完成；
 * 消抖稳定后推送事件到全局队列。
 */
#include "switch_input.h"
#include "../../HAL/stm32f4xx_hal.h"
#include "../../Core/event_queue.h"

/* -----------------------------------------------------------------------
 * 消抖状态机
 * --------------------------------------------------------------------- */
typedef struct {
    bool     raw;           /**< 中断触发的粗滤电平 */
    bool     stable;        /**< 消抖后稳定电平 */
    uint16_t cnt;           /**< 连续保持相同电平的计数（ms） */
    bool     prev_stable;   /**< 上一次稳定电平（用于检测边沿） */
} DebounceCtx_t;

static DebounceCtx_t s_btn;   /**< 手柄按键 */
static DebounceCtx_t s_foot;  /**< 脚踏开关 */

/* -----------------------------------------------------------------------
 * 私有：消抖处理（每 1ms 调用一次）
 * 返回：1=有边沿产生，0=无变化
 * --------------------------------------------------------------------- */
static void debounce_update(DebounceCtx_t *ctx,
                            EventType_t press_evt,
                            EventType_t release_evt)
{
    if (ctx->raw == ctx->stable) {
        ctx->cnt = 0U;
        return;
    }

    ctx->cnt++;
    if (ctx->cnt >= SWITCH_DEBOUNCE_MS) {
        ctx->prev_stable = ctx->stable;
        ctx->stable      = ctx->raw;
        ctx->cnt         = 0U;

        /* 推送事件 */
        if (ctx->stable && !ctx->prev_stable) {
            EVT_PUSH(press_evt, 0U);
        } else if (!ctx->stable && ctx->prev_stable) {
            EVT_PUSH(release_evt, 0U);
        }
    }
}

/* -----------------------------------------------------------------------
 * 公共 API
 * --------------------------------------------------------------------- */
void SwitchInput_Init(void)
{
    s_btn.raw         = false;
    s_btn.stable      = false;
    s_btn.prev_stable = false;
    s_btn.cnt         = 0U;

    s_foot.raw         = false;
    s_foot.stable      = false;
    s_foot.prev_stable = false;
    s_foot.cnt         = 0U;

    /* 实际项目中在此处配置 GPIO 和 NVIC（CubeMX 生成代码已完成） */
}

void SwitchInput_HandleBtnIRQ(bool rising)
{
    s_btn.raw = rising;
}

void SwitchInput_HandleFootIRQ(bool rising)
{
    s_foot.raw = rising;
}

void SwitchInput_DebounceUpdate(void)
{
    debounce_update(&s_btn,  EVT_HANDLE_BTN_PRESS, EVT_HANDLE_BTN_RELEASE);
    debounce_update(&s_foot, EVT_FOOT_SW_PRESS,    EVT_FOOT_SW_RELEASE);
}

bool SwitchInput_IsBtnPressed(void)
{
    return s_btn.stable;
}

bool SwitchInput_IsFootPressed(void)
{
    return s_foot.stable;
}
