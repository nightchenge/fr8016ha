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

// 初始化所有模块
void user_modules_init(void) {

    led_indicator_init();
    lt_uart_init(); 
    lte_controller_init();
    ble_manager_init();

//     pmu_set_pin_to_CPU(GPIO_PORT_A, (1 << GPIO_BIT_1));
//     system_set_port_mux(GPIO_PORT_A, GPIO_BIT_1, PORTA1_FUNC_A1);
//     gpio_set_dir(GPIO_PORT_A, GPIO_BIT_1, GPIO_DIR_IN);
//     system_set_port_pull(GPIO_PA1, false);

//     pmu_set_pin_pull(GPIO_PORT_A, (1 << GPIO_BIT_1), true);
//     co_printf("%s--%d\r\n", __func__, __LINE__);
//    // pmu_port_wakeup_func_set(GPIO_PA1);

//     co_printf("%s--%d\r\n", __func__, __LINE__);
    g_user_task_id = os_task_create(user_task_func);
   // button_init(POWER_KEY_GPIO);

    co_printf("All user modules initialized.\r\n");
}

// 主系统周期任务
void system_periodic_task(void) {
    // 检查是否有来自UART的关机请求
    if (g_shutdown_request_from_uart) {
        g_shutdown_request_from_uart = false; // 清除标志
        co_printf("Shutdown requested from UART ISR. Shutting down now.\r\n");
        power_manager_shutdown(); // 在安全的任务上下文中执行关机
        return; // 关机后不再执行其他逻辑
    }
    // 驱动各个模块的周期性逻辑
    lte_controller_periodic_task();
    // 更新LED状态
    battery_state_t current_battery_state = power_manager_get_state();
    led_indicator_update(current_battery_state);
    // 处理LED闪烁
    led_indicator_blink_task();
}

// 按键事件处理任务
static int user_task_func(os_event_t *param) {
    if (param->event_id != USER_EVT_BUTTON) {
        return EVT_CONSUMED;
    }

    struct button_msg_t *button_msg = (struct button_msg_t *)param->param;
    if (button_msg->button_index != POWER_KEY_GPIO) {
        return EVT_CONSUMED;
    }

    switch (button_msg->button_type) {
        case BUTTON_SHORT_PRESSED:
            co_printf("Button short pressed.\r\n");
            // 定义短按功能...
            break;

        case BUTTON_LONG_PRESSED:
            co_printf("Button long pressed.\r\n");
            if (lte_controller_get_state() == LTE_STATE_OFF) {
                lte_controller_power_on();
            }
            break;

        case BUTTON_LONG_LONG_PRESSED:
            co_printf("Button very long pressed.\r\n");
            if (lte_controller_get_state() != LTE_STATE_OFF) {
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