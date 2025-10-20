#include "lt_main.h"
#include "user_task.h"
#include "power_manager.h"
#include "os_timer.h"
#include "co_printf.h"
#include "ble_manager.h"
#include <stddef.h>

#include "config.h"
#include "driver_uart.h"
#include "driver_gpio.h"
#include "driver_pmu.h"
#include "lte_controller.h"
#include "led_indicator.h"

// 引用来自proj_main.c的全局唤醒标志
extern volatile bool g_wakeup_event;
extern volatile char g_wakeup_reason;

static os_timer_t g_system_tick_timer;
static void gpio_init_on_boot(void);
static void system_tick_callback(void *arg);
static void handle_wakeup_event(void);
static void api_led_on_green(void);
static void api_led_off(void);
// 应用唯一入口

// 新增函数，用于安全地停止主定时器
void main_app_stop_timers(void)
{
    os_timer_stop(&g_system_tick_timer);
    co_printf("[LT_MAIN] System tick timer stopped for sleep.\r\n");
}

// 新增函数，用于从睡眠唤醒后重启主定时器
void main_app_restart_timers(void)
{
    os_timer_start(&g_system_tick_timer, SYSTEM_TICK_MS, true);
    co_printf("[LT_MAIN] System tick timer restarted from wakeup.\r\n");
}

void main_app_init(void)
{
    co_printf("[LT_MAIN] Main App Initializing...\r\n");

    // 1. 初始化开机检测所需的基础硬件
    gpio_init_on_boot();
    // led_indicator_init();
    power_manager_init();
    // 2. 开机时进行关键的电池电量检查
    power_manager_check_on_boot();
    // 3. === 恢复并实现“长按开机”逻辑 ===
    co_printf("Please press and hold the power button to boot...\r\n");
    int hold_count = 0;
    // 循环检测1秒 (20次 x 50毫秒)

    for (int i = 0; i < 20; i++)
    {
        // GPIO_PA1, 低电平为按下
        if (gpio_get_pin_value(POWER_KEY_PORT, POWER_KEY_PIN_BIT) == 0)
        {
            hold_count++;

            // 复现原始代码中的LED跳动反馈
            if (hold_count == 7)
                api_led_off();
            else if (hold_count == 8)
                api_led_on_green();
            else if (hold_count == 9)
                api_led_off();
            else if (hold_count >= 10)
                api_led_on_green();
            co_delay_100us(50 * 10); // 50ms delay
        }
        else
        {
            // 如果在达到长按阈值前松手，则停止开机
            if (hold_count > 0 && hold_count < 10)
            {
                co_printf("Button released too early. Aborting boot.\r\n");
                hold_count = 0; // 重置计数，准备休眠
                break;
            }
        }
        co_delay_100us(50 * 10); // 延时50ms
    }
    while (gpio_get_pin_value(POWER_KEY_PORT, POWER_KEY_PIN_BIT) == 0)
    {
        co_delay_100us(50 * 10);
    }

    pmu_set_pin_pull(GPIO_PORT_A, (1 << GPIO_BIT_1), true);
    button_init(GPIO_PA1);
    co_printf("WAKEUPSET####\r\n");
    pmu_port_wakeup_func_set(GPIO_PA1);
    // 4. 根据长按检测结果，决定是继续启动还是进入睡眠
    if (hold_count >= 10)
    { // 按下时间足够长 (>= 500ms)

        //    system_sleep_enable();

        co_printf("[LT_MAIN] Long press detected. Booting system.\r\n");
        api_led_on_green(); // 保持绿灯常亮
        pmu_port_wakeup_func_clear(GPIO_PA1);
        // 初始化所有软件模块
        user_modules_init();

        // 启动4G模块
        lte_controller_power_on();

        // 启动系统主循环定时器
        os_timer_init(&g_system_tick_timer, system_tick_callback, NULL);
        os_timer_start(&g_system_tick_timer, SYSTEM_TICK_MS, true);

        co_printf("[LT_MAIN] Main App Running.\r\n");
    }
    else
    { // 按下时间不够长或未按下
        co_printf("[LT_MAIN] No long press detected. Going to sleep.\r\n");
        power_manager_shutdown();
    }
}

// 系统主循环（由定时器驱动）
static void system_tick_callback(void *arg)
{
    // 检查是否有唤醒事件需要处理
    if (g_wakeup_event)
    {
        handle_wakeup_event();
    }
    system_periodic_task();
}

// 唤醒事件处理器
static void handle_wakeup_event(void)
{
    g_wakeup_event = false; // 立即清除标志，防止重复处理
    co_printf("[LT_MAIN] Wakeup event detected! Reason: %c\r\n", g_wakeup_reason);
    g_wakeup_reason = 'U';
    system_sleep_disable();

    power_manager_handle_wakeup();

    gpio_init_on_boot();

    system_set_port_mux(GPIO_PORT_C, GPIO_BIT_6, PORTC6_FUNC_UART0_RXD);
    system_set_port_mux(GPIO_PORT_C, GPIO_BIT_7, PORTC7_FUNC_UART0_TXD);
    uart_init(UART0, UART0_BAUDRATE);
    NVIC_EnableIRQ(UART0_IRQn);

    if (lte_controller_get_state() == LTE_STATE_READY)
    {
        ble_manager_start_advertising(lte_controller_get_imei());
    }
}

// 开机和唤醒时都需要执行的GPIO初始化
static void gpio_init_on_boot(void)
{

    pmu_set_pin_to_CPU(GPIO_PORT_A, (1 << GPIO_BIT_7));
    system_set_port_pull(GPIO_PA7, true);
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_7, PORTA7_FUNC_A7);
    gpio_set_dir(GPIO_PORT_A, GPIO_BIT_7, GPIO_DIR_OUT);
    gpio_set_pin_value(GPIO_PORT_A, GPIO_BIT_7, 1);

    pmu_set_pin_to_CPU(GPIO_PORT_A, (1 << GPIO_BIT_4));
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_4, PORTA4_FUNC_A4); // PA4
    system_set_port_pull(GPIO_PA4, true);
    gpio_set_dir(GPIO_PORT_A, GPIO_BIT_4, GPIO_DIR_OUT);
    gpio_set_pin_value(GPIO_PORT_A, GPIO_BIT_4, 0);

    pmu_set_pin_to_CPU(GPIO_PORT_A, (1 << GPIO_BIT_5));
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_5, PORTA5_FUNC_A5);
    system_set_port_pull(GPIO_PA5, true);
    gpio_set_dir(GPIO_PORT_A, GPIO_BIT_5, GPIO_DIR_OUT);
    gpio_set_pin_value(GPIO_PORT_A, GPIO_BIT_5, 1);

    pmu_set_pin_to_CPU(GPIO_PORT_A, (1 << GPIO_BIT_0));
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_0, PORTA0_FUNC_A0);
    system_set_port_pull(GPIO_PA0, true);
    gpio_set_dir(GPIO_PORT_A, GPIO_BIT_0, GPIO_DIR_OUT);
    gpio_set_pin_value(GPIO_PORT_A, GPIO_BIT_0, 0);
    pmu_set_led1_as_pwm();

    pmu_set_pin_to_CPU(GPIO_PORT_A, (1 << GPIO_BIT_1));
    system_set_port_mux(GPIO_PORT_A, GPIO_BIT_1, PORTA1_FUNC_A1);
    gpio_set_dir(GPIO_PORT_A, GPIO_BIT_1, GPIO_DIR_IN);
    system_set_port_pull(GPIO_PA1, false);

    // For LEDs
    pmu_set_led1_as_pwm();
    pmu_set_led2_as_pwm();
}

// LED控制辅助函数
static void api_led_on_green(void)
{
    pmu_set_led1_value(0);
    pmu_set_led2_value(1);
}
static void api_led_off(void)
{
    pmu_set_led1_value(0);
    pmu_set_led2_value(0);
}