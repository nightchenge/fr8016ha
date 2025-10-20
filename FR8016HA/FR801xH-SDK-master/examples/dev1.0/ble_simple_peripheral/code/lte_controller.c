/*
 * @Descripttion:
 * @version:
 * @Author: zhouhao
 * @Date: 2025-09-17 10:30:41
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-15 17:05:11
 */
#include "lte_controller.h"
#include "config.h"
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

static void parse_imei_from_response(const uint8_t *data, uint16_t length);

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

    g_lte_state = LTE_STATE_OFF;
    co_printf("LTE Controller Initialized.\r\n");
}

void lte_controller_power_on(void)
{
    if (g_lte_state == LTE_STATE_OFF)
    {
        co_printf("LTE Powering On...\r\n");
        gpio_set_pin_value(LTE_POWER_EN_PORT, LTE_POWER_EN_PIN_BIT, 1);
        co_delay_100us(500 * 10); // 供电后稳定500ms

        gpio_set_pin_value(LTE_RESET_PORT, LTE_RESET_PIN_BIT, 1);
        gpio_set_pin_value(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, 1);

        g_timer_ms = 0;
        g_lte_state = LTE_STATE_POWERING_ON;
    }
}

void lte_controller_power_off(void)
{
    co_printf("LTE Powering Off...\r\n");
    gpio_set_pin_value(LTE_RESET_PORT, LTE_RESET_PIN_BIT, 0);
    gpio_set_pin_value(LTE_PWRKEY_PORT, LTE_PWRKEY_PIN_BIT, 0);
    gpio_set_pin_value(LTE_POWER_EN_PORT, LTE_POWER_EN_PIN_BIT, 0);
    g_lte_state = LTE_STATE_OFF;
}

void lte_controller_periodic_task(void)
{
    g_timer_ms += SYSTEM_TICK_MS;
    //   co_printf("LTE Controller Periodic Task, State: %d, Timer: %dms\r\n", g_lte_state, g_timer_ms);
    // --- State Machine ---
    switch (g_lte_state)
    {
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
        if (g_timer_ms % 1000 == 0)
        {
            co_printf("Sending IMEI request...\r\n");
            const char *cmd = "{\"act\":\"q\",\"para\":\"module\"}";
            lt_uart0_write((uint8_t *)cmd, strlen(cmd));
        }
        // 超时处理
        if (g_timer_ms >= LTE_CMD_TIMEOUT_MS)
        {
            co_printf("ERROR: Get IMEI timeout.\r\n");
            g_lte_state = LTE_STATE_ERROR;
        }
        break;

    case LTE_STATE_READY:
        // 在此状态下，处理UART接收到的透传数据
        lt_handle_uart_process();
        break;

    default:
        break;
    }

    // UART 接收处理 (只在等待IMEI时解析)
    if (g_lte_state == LTE_STATE_WAITING_FOR_IMEI && uart0_recvsta_get())
    {
        co_printf("Received data: %s\r\n", lt_uart0_recbuf_get());
        parse_imei_from_response(lt_uart0_recbuf_get(), lt_uart0_reclen_get());
        uart0_recvsta_clear();
    }
}

static void parse_imei_from_response(const uint8_t *data, uint16_t length)
{
    char *ptr = strstr((const char *)data, "\"IMEI\":\"");
    if (ptr != NULL)
    {
        ptr += 8; // Move pointer to the start of the IMEI value
        int i = 0;
        while (*ptr != '"' && *ptr != '\0' && i < 15)
        {
            g_imei_buffer[i++] = *ptr++;
        }
        g_imei_buffer[i] = '\0';

        if (strlen(g_imei_buffer) == 15)
        {
            co_printf("LTE IMEI Acquired: %s\r\n", g_imei_buffer);
            g_lte_state = LTE_STATE_READY;
            // 通知BLE模块使用新IMEI开始广播
            ble_manager_start_advertising(g_imei_buffer);
        }
    }
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