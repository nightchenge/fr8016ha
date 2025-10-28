/**
 * Copyright (c) 2019, Freqchip
 *
 * All rights reserved.
 *
 *
 */

/*
 * INCLUDES
 */
#include <stdint.h>
#include <stddef.h>  // 添加此行以引入 NULL 定义
#include "driver_plf.h"
#include "driver_uart.h"
#include "driver_gpio.h"
#include "os_timer.h"
#include "lt_main.h"
#include "user_task.h"

// // 在 driver_uart.c 顶部（或一个共享头文件中）声明外部标志
// extern volatile bool g_shutdown_request_from_uart;

#define UART_CLK 14745600
#define UART_FIFO_TRIGGER (FCR_RX_TRIGGER_00 | FCR_TX_TRIGGER_10)

typedef struct uart0recvflag
{
	uint8_t rxbuf[512];
	uint8_t rxlen;
	uint8_t uart0recvflag;
} S_UART0REC;

typedef struct uart1recvflag
{
	uint8_t rxbuf[512];
	uint8_t rxlen;
	uint8_t uart1recvflag;
} S_UART1REC;

os_timer_t uart1_recv_timer; // recive data timer
os_timer_t uart0_recv_timer; // recive data timer

S_UART0REC s_uart0rec = {0};
S_UART1REC s_uart1rec = {0};

// ******************** ADDED ********************
// Pointers for frame reception callbacks
static uart_frame_rx_callback_t g_uart0_frame_cb = NULL;
static uart_frame_rx_callback_t g_uart1_frame_cb = NULL;

/**
 * @brief Register a callback function to be called when UART0 frame reception times out.
 * @param cb  The function pointer to call.
 */
void uart0_register_frame_callback(uart_frame_rx_callback_t cb)
{
	g_uart0_frame_cb = cb;
}

/**
 * @brief Register a callback function to be called when UART1 frame reception times out.
 * @param cb  The function pointer to call.
 */
void uart1_register_frame_callback(uart_frame_rx_callback_t cb)
{
	g_uart1_frame_cb = cb;
}
// ***********************************************


int uart0_recvsta_get()
{
	return s_uart0rec.uart0recvflag;
}

int uart0_recvsta_clear()
{
	s_uart0rec.uart0recvflag = 0;
	s_uart0rec.rxlen = 0;
	// os_timer_stop(&uart0_recv_timer);

	return 0;
}

int uart1_recvsta_clear()
{
	s_uart1rec.uart1recvflag = 0;
	s_uart1rec.rxlen = 0;
	// os_timer_stop(&uart1_recv_timer);

	return 0;
}

uint8_t *uart0_recbuf_get()
{
	return &s_uart0rec.rxbuf[0];
}

uint8_t uart0_reclen_get()
{
	return s_uart0rec.rxlen;
}

uint8_t *uart1_recbuf_get()
{
	return &s_uart1rec.rxbuf[0];
}

uint8_t uart1_reclen_get()
{
	return s_uart1rec.rxlen;
}

uint8_t uart1_recflag_get()
{
	return s_uart1rec.uart1recvflag;
}

// ******************** MODIFIED ********************
void uart0_recv_func(void *arg)
{
   // co_printf("uart0_recv_func.\r\n");

    s_uart0rec.uart0recvflag = 1;

    // 检查回调函数和有效数据长度
    if (g_uart0_frame_cb)
    {
        uint8_t *buffer = uart0_recbuf_get();
        uint32_t len = uart0_reclen_get();
        
        // 仅当数据长度大于0时复制并回调
        if (len > 0)
        {
            static uint8_t copy_buf[512];  // 静态缓冲区避免堆分配
            memcpy(copy_buf, buffer, len); // 复制原始数据
            g_uart0_frame_cb(copy_buf, len); // 传递复制后的缓冲区
			uart0_recvsta_clear(); // 清除接收状态标志
        }
    }
    return;
}
// ******************** MODIFIED ********************
void uart1_recv_func(void *arg)
{
   // co_printf("recv data over now send to phone.\r\n");

    s_uart1rec.uart1recvflag = 1;

    // 检查回调函数和有效数据长度
    if (g_uart1_frame_cb)
    {
        uint8_t *buffer = uart1_recbuf_get();
        uint32_t len = uart1_reclen_get();
        
        // 仅当数据长度大于0时复制并回调
        if (len > 0)
        {
            static uint8_t copy_buf[512];  // 静态缓冲区避免堆分配
            memcpy(copy_buf, buffer, len); // 复制原始数据
            g_uart1_frame_cb(copy_buf, len); // 传递复制后的缓冲区
			uart1_recvsta_clear(); // 清除接收状态标志
        }
    }
    return;
}

void uart1_timer_stop()
{
	os_timer_stop(&uart1_recv_timer);
}

void uart0_timer_stop()
{
	os_timer_stop(&uart0_recv_timer);
}

__attribute__((section("ram_code"))) void uart0_isr_ram(void)
{
	uint8_t int_id;
	uint8_t c;
	volatile struct uart_reg_t *const uart0_reg = (volatile struct uart_reg_t *)UART0;

	int_id = uart0_reg->u3.iir.int_id;
//	co_printf("uart0_isr_ram.\r\n");
	if (int_id == 0x04 || int_id == 0x0c) /* Receiver data available or Character time-out indication */
	{
		while (uart0_reg->lsr & 0x01)
		{
			c = uart0_reg->u1.data;
			// uart_putc_noint(UART1,c);
			s_uart0rec.rxbuf[s_uart0rec.rxlen] = c;
			os_timer_start(&uart0_recv_timer, 50, false); // 50ms定时器，超时判断为一帧数据
			s_uart0rec.rxlen++;
			if (s_uart0rec.rxlen > 512)
				s_uart0rec.rxlen = 0;
		}
	}
	else if (int_id == 0x06)
	{
		volatile uint32_t line_status = uart0_reg->lsr;
	}
}

__attribute__((section("ram_code"))) void uart1_isr_ram(void)
{
	uint8_t int_id;
	uint8_t c;
	volatile struct uart_reg_t *const uart1_reg = (volatile struct uart_reg_t *)UART1;
//	co_printf("uart1_isr_ram.\r\n");
	int_id = uart1_reg->u3.iir.int_id;
	if (int_id == 0x04 || int_id == 0x0c) /* Receiver data available or Character time-out indication */
	{
		while (uart1_reg->lsr & 0x01)
		{
			c = uart1_reg->u1.data;
			// uart_putc_noint(UART1,'K');
			s_uart1rec.rxbuf[s_uart1rec.rxlen] = c;
			// co_printf("rxbuf=%02x.\r\n",c);
			os_timer_start(&uart1_recv_timer, 50, false); // 50ms定时器，超时判断为一帧数据
			s_uart1rec.rxlen++;
			if (s_uart1rec.rxlen > 512)
				s_uart1rec.rxlen = 0;
		}
	}
	else if (int_id == 0x06)
	{
		volatile uint32_t line_status = uart1_reg->lsr;
	}

	// s_uart1rec.uart1recvflag=1;
}

int uart0_recv_timer_init()
{
	os_timer_init(&uart0_recv_timer, uart0_recv_func, 0);
	return 0;
}

int uart1_recv_timer_init()
{
	os_timer_init(&uart1_recv_timer, uart1_recv_func, 0);
	return 0;
}

void uart_init1(uint32_t uart_addr, uart_param_t param)
{
	int uart_baud_divisor;
	volatile struct uart_reg_t *const uart_reg = (volatile struct uart_reg_t *)uart_addr;

	/* wait for tx fifo is empty */
	while (!(uart_reg->lsr & 0x40))
		;

	volatile uint32_t misc_data;
	REG_PL_WR(&(uart_reg->u2.ier), 0);
	REG_PL_WR(&(uart_reg->u3.fcr), 0x06);
	REG_PL_WR(&(uart_reg->lcr), 0);
	misc_data = REG_PL_RD(&(uart_reg->lsr));
	misc_data = REG_PL_RD(&(uart_reg->mcr));

	uart_baud_divisor = UART_CLK / (16 * param.baud_rate);

	while (!(uart_reg->lsr & 0x40))
		;

	/* baud rate */
	uart_reg->lcr.divisor_access = 1;
	uart_reg->u1.dll.data = uart_baud_divisor & 0xff;
	uart_reg->u2.dlm.data = (uart_baud_divisor >> 8) & 0xff;
	uart_reg->lcr.divisor_access = 0;

	/*word len*/
	uart_reg->lcr.word_len = param.data_bit_num - 5;
	if (param.pari == 0)
	{
		uart_reg->lcr.parity_enable = 0;
		uart_reg->lcr.even_enable = 0;
	}
	else
	{
		uart_reg->lcr.parity_enable = 1;
		if (param.pari == 2)
			uart_reg->lcr.even_enable = 1;
		else
			uart_reg->lcr.even_enable = 0;
	}
	uart_reg->lcr.sp = 0;

	if (param.stop_bit == 1)
		uart_reg->lcr.stop = 0;
	else
		uart_reg->lcr.stop = 1;

	/*fifo*/
	uart_reg->u3.fcr.data = UART_FIFO_TRIGGER | FCR_FIFO_ENABLE;

	/*auto flow control*/
	uart_reg->mcr.afe = 0;

	// flush rx fifo.
	volatile uint8_t data;
	while (uart_reg->lsr & 0x01) // UART_RX_FIFO_AVAILABLE
	{
		data = uart_reg->u1.data;
	}

	/*enable recv and line status interrupt*/
	uart_reg->u2.ier.erdi = 1;
	uart_reg->u2.ier.erlsi = 1;
}