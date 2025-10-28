#ifndef _USER_TASK_H
#define _USER_TASK_H

#include <stdint.h>

enum user_event_t
{
    USER_EVT_AT_COMMAND,
    USER_EVT_BUTTON,
    USER_EVT_UART0_RX,
    USER_EVT_UART1_RX,
};

enum user_power_t
{
    REDLED_ON_COMMAND,
    REDLED_OFF_COMMAND,
    GRELED_ON_COMMAND,
    GRELED_OFF_COMMAND,
    GRELED_BLINK_COMMAND,
    YELLED_ON_COMMAND,
    YELLED_OFF_COMMAND,
    LED_OFF_COMMAND,
};

enum user_audio_event_t
{
    USER_EVT_MIC, // Mic
    DECODER_EVENT_PREPARE,
    DECODER_EVENT_NEXT_FRAME,
    DECODER_EVENT_STOP,
    DECODER_EVENT_WAIT_END_TO,
};
extern uint16_t g_user_task_id;
/**
 * @brief 初始化所有用户任务和模块
 */
void user_modules_init(void);

/**
 * @brief 系统主循环/周期性任务
 * 该函数应被周期性调用，以驱动所有模块的状态机和逻辑
 */
void system_periodic_task(void);


#endif // _USER_TASK_Hs