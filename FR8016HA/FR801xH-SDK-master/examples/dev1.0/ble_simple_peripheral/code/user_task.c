#include "user_task.h"
#include "os_task.h"
#include "co_printf.h"
#include "button.h"
#include "config.h"

// 引入新模块的头文件
#include "power_manager.h"
#include "led_indicator.h"
#include "lte_controller.h"
#include "ble_manager.h"
// ... (includes) ...

// 添加一个全局的、易失的（volatile）关机请求标志
volatile bool g_shutdown_request_from_uart = false;

// ...

static int user_task_func(os_event_t *param);
uint16_t g_user_task_id;

// ******************** MODIFIED ********************
// 初始化所有模块
void user_modules_init(void)
{

    g_user_task_id = os_task_create(user_task_func);

    led_indicator_init();
    lt_uart_init();
    lte_controller_init();
    ble_manager_init();
}

// 主系统周期任务
void system_periodic_task(void)
{
    // 检查是否有来自UART的关机请求
    if (g_shutdown_request_from_uart)
    {
        g_shutdown_request_from_uart = false; // 清除标志
        co_printf("Shutdown requested from UART ISR. Shutting down now.\r\n");
        power_manager_shutdown(); // 在安全的任务上下文中执行关机
        return;                   // 关机后不再执行其他逻辑
    }
    // 驱动各个模块的周期性逻辑
    lte_controller_periodic_task(); // (轮询的UART处理应从此函数中移除)
    // 更新LED状态
    battery_state_t current_battery_state = power_manager_get_state();
    led_indicator_update(current_battery_state);
    // 处理LED闪烁
    led_indicator_blink_task();
}

// ******************** MODIFIED ********************
// 按键和UART事件处理任务
static int user_task_func(os_event_t *param)
{

    // 处理UART接收事件
    if (param->event_id == USER_EVT_UART0_RX || param->event_id == USER_EVT_UART1_RX)
    {
        lt_handle_uart_process(param->event_id, param->param, param->param_len);

        return EVT_CONSUMED;
    }

    if (param->event_id != USER_EVT_BUTTON)
    {
        return EVT_CONSUMED;
    }

    struct button_msg_t *button_msg = (struct button_msg_t *)param->param;

    if (button_msg->button_index != POWER_KEY_GPIO)
    {
        return EVT_CONSUMED;
    }

    switch (button_msg->button_type)
    {
    case BUTTON_SHORT_PRESSED:
        co_printf("Button short pressed.\r\n");
        // 定义短按功能...
        break;

    case BUTTON_LONG_PRESSED:
        co_printf("Button long pressed.\r\n");
        if (lte_controller_get_state() == LTE_STATE_READY)
        {
            lte_controller_set_state(LTE_STATE_WAITING_FOR_IMEI);
        }

        break;

    case BUTTON_LONG_LONG_PRESSED:
        co_printf("Button very long pressed.\r\n");
        if (lte_controller_get_state() != LTE_STATE_OFF)
        {
            lte_controller_power_off();
            // 也可以在这里触发关机或睡眠
            // power_manager_request_sleep();
        }
        break;

    default:
        break;
    }

    return EVT_CONSUMED;
}