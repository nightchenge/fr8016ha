/*
 * @Descripttion:
 * @version:
 * @Author: houfangting
 * @Date: 2025-10-17 10:30:41
 * @LastEditors: houfangting
 * @LastEditTime: 2025-10-17 16:05:11
 */
#include "lte_controller.h"
#include "config.h"
#include "driver_gpio.h"
#include "driver_uart.h"
#include "sys_utils.h"
#include "co_printf.h"
#include "string.h"
#include "ble_manager.h" // 需要通知BLE


// 封装uart接口
// uart0 发送数据
void lt_uart0_write(uint8_t *data, int size)
{
    uart0_write(data, size, NULL);
}

uint8_t lt_uart0_reclen_get()
{
    uint8_t rxlen = uart0_reclen_get();

	return rxlen;
}

// uart0 接收数据缓存区获取
uint8_t *lt_uart0_recbuf_get()
{
    uint8_t *recv_buf = uart0_recbuf_get();
    
    return recv_buf;
}

// uart1 发送数据
void lt_uart1_write(uint8_t *data, int size)
{
    uart1_write(data, size, NULL);
}

uint8_t lt_uart1_reclen_get()
{
    uint8_t rxlen = uart1_reclen_get();
    
	return rxlen;
}
// uart1 接收数据缓存区获取
uint8_t *lt_uart1_recbuf_get()
{
    uint8_t *recv_buf = uart1_recbuf_get();
    
    return recv_buf;
}

void lt_uart0_recvsta_clear()
{
    uart0_recvsta_clear();
}

void lt_uart1_recvsta_clear()
{
    uart1_recvsta_clear();
}

void lt_uart_init(void)
{
    // 初始化UART0 for LTE
    system_set_port_mux(GPIO_PORT_C, GPIO_BIT_6, PORTC6_FUNC_UART0_RXD);
    system_set_port_mux(GPIO_PORT_C, GPIO_BIT_7, PORTC7_FUNC_UART0_TXD);
    uart_init(UART0, UART0_BAUDRATE);
    NVIC_EnableIRQ(UART0_IRQn);

    // 初始化UART0接收定时器
    uart0_recv_timer_init();
    // 初始化UART1接收定时器
    uart1_recv_timer_init();
}

// UART接收串口处理
extern volatile bool g_shutdown_request_from_uart;
int lt_handle_uart0_recv(uint8_t *data, int size)
{
    if (size != 0)
    {
        if (data[0] == 0xA9 && data[1] == 0xA9 && data[2] == 0x01)
        {

            if (data[3] == 0x01)
            {
                int value = 0;
                memcpy(&value, &data[4], 4);
                co_printf("4g adc %d \n", value);
            }
        }
        if (data[0] == 0xA4 && data[1] == 0xA4 && data[2] == 0x01)
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
        if (data[0] == 0xA5 && data[1] == 0xA5 && data[2] == 0xff && data[3] == 0xff)
        {
            // 关闭系统
            co_printf("Received sleep command from LTE module.\r\n");
            g_shutdown_request_from_uart = true;
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

void lt_handle_uart_process()
{
    if (uart0_recvsta_get())
    {
        co_printf("Received uart0 data: %d\r\n", uart0_recvsta_get());
        lt_handle_uart0_recv(uart0_recbuf_get(), uart0_reclen_get());
        uart0_recvsta_clear();
    }

    if (uart1_recflag_get())
    {
        co_printf("Received uart1 data: %d\r\n", uart1_recflag_get());
        lt_handle_uart1_recv(uart1_recbuf_get(), uart1_reclen_get());
        uart1_recvsta_clear();
    }
}
