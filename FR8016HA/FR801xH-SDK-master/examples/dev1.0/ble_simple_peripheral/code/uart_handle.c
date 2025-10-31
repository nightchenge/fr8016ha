/*
 * @Descripttion:
 * @version:
 * @Author: houfangting
 * @Date: 2025-10-17 10:30:41
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-28 20:04:42
 */
#include "lte_controller.h"
#include "config.h"
#include "driver_gpio.h"
#include "driver_uart.h"
#include "sys_utils.h"
#include "co_printf.h"
#include "string.h"
#include "ble_manager.h" // 需要通知BLE

// ******************** ADDED ********************
#include "os_task.h"   // For os_task_send_event
#include "user_task.h" // For g_user_task_id and USER_EVT_UART_RX
// ***********************************************

// ******************** ADDED ********************
/**
 * @brief Callback function called from driver's timer context when a frame is received.
 * This function sends an event to the main user_task.
 * (This function is called from OS_TIMER context)
 */

// ******************** MODIFIED ********************
// UART0专用回调

static void ota_uart0_frame_rx_cb(uint8_t *buffer, uint32_t len)
{
    if (g_user_task_id)
    {
        os_event_t evt;
        evt.event_id = USER_OTA_COMMAND; // 新增专用事件ID
        evt.param = buffer;
        evt.param_len = len;
        os_msg_post(g_user_task_id, &evt);
    }
}

static void app_uart0_frame_rx_cb(uint8_t *buffer, uint32_t len)
{
    if (g_user_task_id)
    {
        os_event_t evt;
        evt.event_id = USER_EVT_UART0_RX; // 新增专用事件ID
        evt.param = buffer;
        evt.param_len = len;
        os_msg_post(g_user_task_id, &evt);
    }
}

// UART1专用回调
static void app_uart1_frame_rx_cb(uint8_t *buffer, uint32_t len)
{
    if (g_user_task_id)
    {
        os_event_t evt;
        evt.event_id = USER_EVT_UART1_RX; // 新增专用事件ID
        evt.param = buffer;
        evt.param_len = len;
        os_msg_post(g_user_task_id, &evt);
    }
}

// 封装uart接口
// uart0 发送数据
void lt_uart0_write(uint8_t *data, int size)
{
    uart0_write(data, size, NULL);
}
// uart1 发送数据
void lt_uart1_write(uint8_t *data, int size)
{
    uart1_write(data, size, NULL);
}

// ******************** MODIFIED ********************
void lt_uart_init(void)
{
    // 初始化UART0 for LTE
    system_set_port_mux(GPIO_PORT_C, GPIO_BIT_6, PORTC6_FUNC_UART0_RXD);
    system_set_port_mux(GPIO_PORT_C, GPIO_BIT_7, PORTC7_FUNC_UART0_TXD);
    co_printf("lt_uart_init UART0_BAUDRATE: %d\r\n", UART0_BAUDRATE);
    uart_init(UART0, UART0_BAUDRATE);
    NVIC_EnableIRQ(UART0_IRQn);

    uart0_register_frame_callback(app_uart0_frame_rx_cb);
    uart1_register_frame_callback(app_uart1_frame_rx_cb);

    // 初始化UART0接收定时器
    uart0_recv_timer_init();
    // 初始化UART1接收定时器
    uart1_recv_timer_init();
}

// UART接收串口处理
extern volatile bool g_shutdown_request_from_uart;
static bool g_imei_acquired = false;
static char g_imei_buffer[16] = {0}; // 15 digits + null terminator

// 发送获取imei的接口
void lt_imei_request(void)
{
    const char *cmd = "{\"act\":\"q\",\"para\":\"module\"}";
    lt_uart0_write((uint8_t *)cmd, strlen(cmd));
}

bool lt_imei_acquired(char *imei_buffer)
{
    if (g_imei_acquired)
    {
        memcpy(imei_buffer, g_imei_buffer, sizeof(g_imei_buffer));
    }
    return g_imei_acquired;
}

int parse_imei_from_response(const uint8_t *data, uint16_t length)
{
    char imei_buffer[16]; // 15 digits + null terminator

    char *ptr = strstr((const char *)data, "\"IMEI\":\"");
    if (ptr != NULL)
    {
        ptr += 8; // Move pointer to the start of the IMEI value
        int i = 0;
        while (*ptr != '"' && *ptr != '\0' && i < 15)
        {
            imei_buffer[i++] = *ptr++;
        }
        imei_buffer[i] = '\0';

        if (strlen(imei_buffer) == 15)
        {
            memcpy(g_imei_buffer, imei_buffer, sizeof(imei_buffer));
            return 0;
        }
    }
    return -1;
}
int lt_handle_uart0_recv(uint8_t *data, int size)
{
    if (size != 0)
    {
        if (!g_imei_acquired)
        {
            if (parse_imei_from_response(data, size) == 0)
            {
                g_imei_acquired = true;
                return 0;
            }
        }

        if (data[0] == 0xA9 && data[1] == 0xA9 && data[2] == 0x01)
        {
            if (data[3] == 0x01)
            {
                int value = 0;
                memcpy(&value, &data[4], 4);
                co_printf("4g adc %d \n", value);
            }
        }
        else if (data[0] == 0xA4 && data[1] == 0xA4 && data[2] == 0x01)
        {
            // 低功耗模式
            if (data[3] == 0x01)
            {

                co_printf("Received sleep command from LTE module.\r\n");
                // 1. 停止BLE功能
                ble_manager_stop_advertising();
                ble_manager_disconnect_all(); // 断开所有BLE连接

                // 2. 发出系统睡眠请求
                power_manager_request_sleep();
            }
        }
        else if (data[0] == 0xA5 && data[1] == 0xA5 && data[2] == 0xff && data[3] == 0xff)
        {
            // 关闭系统
            co_printf("Received sleep command from LTE module.\r\n");
            g_shutdown_request_from_uart = true;
        }
        else if (data[0] == 0xAA && data[1] == 0xAA && data[2] == 0xff && data[3] == 0xff)
        {
            uart0_register_frame_callback(ota_uart0_frame_rx_cb);
            // ota 升级开始
            co_printf("ota upgrade start.\r\n");

           // g_shutdown_request_from_uart = true;
        }
        else
        {
            co_printf("ble_manager_send_notification\r\n");
            ble_manager_send_notification(data, size);
        }
    }

    return 0;
}

int lt_handle_uart1_recv(uint8_t *data, int size)
{
    if (size != 0)
    {

        if (data[0] == 0x49) // 检查协议头
        {
            // 检查远程关机指令
            if (data[5] == 0x03 && data[8] == 0x00)
            {
                co_printf("[check_fun] Remote shutdown command received.\r\n");
                co_delay_100us(1500 * 10);
                power_manager_shutdown(); // <-- 使用新的、正确的接口
            }

            // 将所有有效指令转发给4G模块
            uart0_write(data, size, NULL);
        }
    }

    return 0;
}
// lt_handle_uart_process 函数本身保持不变
// 它现在由事件触发，而不是定时器轮询

void lt_handle_uart_process(uint32_t event_id, uint8_t *data, int size)
{
    // 检查事件ID是否是UART0接收事件
    if (event_id == USER_EVT_UART0_RX)
    {
        //  co_printf("Received uart0 data: %d\r\n", size);
        lt_handle_uart0_recv(data, size);
    }
    // 检查事件ID是否是UART1接收事件
    if (event_id == USER_EVT_UART1_RX)
    {
        // co_printf("Received uart1 data: %d\r\n", size);
        lt_handle_uart1_recv(data, size);
    }
}