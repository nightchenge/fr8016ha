/*
 * @Descripttion:
 * @version:
 * @Author: houfangting
 * @Date: 2025-10-17 10:30:41
 * @LastEditors: houfangting
 * @LastEditTime: 2025-10-17 16:05:11
 */
#ifndef _UART_HANDLE_H
#define _UART_HANDLE_H
#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions

// 封装uart函数
void lt_uart0_write(uint8_t *data, int size);
uint8_t lt_uart0_reclen_get();
uint8_t *lt_uart0_recbuf_get();

void lt_uart1_write(uint8_t *data, int size);
uint8_t lt_uart1_reclen_get();
uint8_t *lt_uart1_recbuf_get();

void lt_uart0_recvsta_clear();
void lt_uart1_recvsta_clear();



void lt_handle_uart_process();
void lt_uart_init(void);


#endif