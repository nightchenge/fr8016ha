/*
 * @Descripttion:
 * @version:
 * @Author: zhouhao
 * @Date: 2025-09-17 10:30:41
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-23 13:06:28
 */
#include "lte_controller.h"
#include "config.h"
#include "os_timer.h"
#include "driver_gpio.h"
// #include "driver_uart.h"
#include "sys_utils.h"
#include "co_printf.h"
#include "string.h"
#include "ble_manager.h" // 需要通知BLE
#include "uart_handle.h"

static lte_state_t g_lte_state = LTE_STATE_OFF;
static char g_imei_buffer[16]; // 15 digits + null terminator
static uint32_t g_timer_ms = 0;
static os_timer_t g_lte_imei_timer;

static void parse_imei_from_response(const uint8_t *data, uint16_t length);

static void lte_imei_timer_callback(void *arg);

void lte_controller_init(void)
{
    // 初始化控制LTE模块的GPIO
    // Power Enable
    system_set_port_mux(LTE_POWER_EN_PORT, LTE_POWER_EN_PIN_BIT, PORTA7_FUNC_A7);
    system_set_port_pull(GPIO_PA7, true);
    gpio_set_dir(LTE_POWER_EN_PORT, LTE_POWER_EN_PIN_BIT, GPIO_DIR_OUT);
    gpio_set_pin_value(LTE_POWER_EN_PORT, LTE_POWER_EN_PIN_BIT, 0); // 默认关闭

    // PWRKEY
    system_set_port_mux(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, PORTA4_FUNC_A4);
    system_set_port_pull(GPIO_PA4, true);
    gpio_set_dir(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, GPIO_DIR_OUT);
    gpio_set_pin_value(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, 0);

    // RESET
    system_set_port_mux(LTE_RESET_PORT, LTE_RESET_PIN_BIT, PORTA0_FUNC_A0);
    system_set_port_pull(GPIO_PA0, true);
    gpio_set_dir(LTE_RESET_PORT, LTE_RESET_PIN_BIT, GPIO_DIR_OUT);
    gpio_set_pin_value(LTE_RESET_PORT, LTE_RESET_PIN_BIT, 0);

    memset(g_imei_buffer, 0, sizeof(g_imei_buffer));

    os_timer_init(&g_lte_imei_timer, lte_imei_timer_callback, NULL);

    g_lte_state = LTE_STATE_OFF;
    co_printf("LTE Controller Initialized.\r\n");
}

void lte_controller_power_on(void)
{
    // if (g_lte_state == LTE_STATE_OFF)
    // {
    co_printf("LTE Powering On...\r\n");
    gpio_set_pin_value(LTE_POWER_EN_PORT, LTE_POWER_EN_PIN_BIT, 1);
    co_delay_100us(500 * 10); // 供电后稳定500ms

    gpio_set_pin_value(LTE_RESET_PORT, LTE_RESET_PIN_BIT, 1);
    gpio_set_pin_value(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, 1);
    // }
}

void lte_controller_power_off(void)
{
    co_printf("LTE Powering Off...\r\n");
    gpio_set_pin_value(LTE_RESET_PORT, LTE_RESET_PIN_BIT, 0);
    gpio_set_pin_value(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, 0);
    gpio_set_pin_value(LTE_POWER_EN_PORT, LTE_POWER_EN_PIN_BIT, 0);
    g_lte_state = LTE_STATE_OFF;
}
// ... lte_controller.c 的其余部分 ...
void lte_controller_periodic_task(void)
{
    g_timer_ms += SYSTEM_TICK_MS;
    // --- State Machine ---
    switch (g_lte_state)
    {
    case LTE_STATE_OFF:
        lte_controller_power_on();
        g_timer_ms = 0;
        g_lte_state = LTE_STATE_POWERING_ON;
        // g_timer_ms = 0;
        break;
    case LTE_STATE_POWERING_ON:
        if (g_timer_ms >= LTE_POWER_ON_STABLE_MS)
        {
            gpio_set_pin_value(LTE_RESET_PORT, LTE_RESET_PIN_BIT, 0);
            gpio_set_pin_value(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, 0);
            g_timer_ms = 0;
            g_lte_state = LTE_STATE_WAITING_FOR_IMEI;
            co_printf("LTE powered on. Waiting for IMEI.\r\n");
        }
        break;

    case LTE_STATE_WAITING_FOR_IMEI:
        // 每秒发送一次获取IMEI的指令
        co_printf("Sending IMEI request...\r\n");
        os_timer_start(&g_lte_imei_timer, LTE_CMD_TIMEOUT_MS, true); // 启动获取imei的定时器
        g_lte_state = LTE_STATE_READY;
        break;
    case LTE_STATE_READY:

        break;

    default:
        break;
    }
}
static void lte_imei_timer_callback(void *arg)
{
    static uint32_t imei_timer_ms = 0;

    lt_imei_request();
    char imei_buffer[16];
    imei_timer_ms += LTE_CMD_TIMEOUT_MS;
    if (lt_imei_acquired(imei_buffer))
    {
        if (strlen(imei_buffer) == 15)
        {
            memcpy(g_imei_buffer, imei_buffer, 15);
            g_imei_buffer[15] = '\0';
            co_printf("LTE IMEI Acquired: %s\r\n", g_imei_buffer);
            // 通知BLE模块使用新IMEI开始广播
            ble_manager_start_advertising(g_imei_buffer);

            g_lte_state = LTE_STATE_READY;
            imei_timer_ms = 0;
            os_timer_stop(&g_lte_imei_timer);
            return;
        }
    }
    if (imei_timer_ms > LTE_CMD_TIMEOUT_MS * 10)
    {
        imei_timer_ms = 0;
        os_timer_stop(&g_lte_imei_timer);
        g_lte_state = LTE_STATE_OFF;
    }
    co_printf("LTE IMEI timeout.\r\n");
}

void lte_controller_set_state(lte_state_t state)
{
    g_lte_state = state;
}

lte_state_t lte_controller_get_state(void)
{
    return g_lte_state;
}

const char *lte_controller_get_imei(void)
{
    if (g_lte_state >= LTE_STATE_READY)
    {
        return g_imei_buffer;
    }
    return NULL;
}

void lte_controller_send_data(uint8_t *data, uint16_t length)
{
    if (g_lte_state == LTE_STATE_READY)
    {
        lt_uart0_write(data, length);
    }
}