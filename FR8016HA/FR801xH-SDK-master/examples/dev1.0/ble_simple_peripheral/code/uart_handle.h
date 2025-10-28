/*
 * @Descripttion:
 * @version:
 * @Author: houfangting
 * @Date: 2025-10-17 10:30:41
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-22 10:50:22
 */
#ifndef _UART_HANDLE_H
#define _UART_HANDLE_H
#include <stdbool.h>          // standard boolean definitions
#include <stdint.h>           // standard integer functions

// 封装uart函数
void lt_uart0_write(uint8_t *data, int size);

void lt_uart1_write(uint8_t *data, int size);

void lt_handle_uart_process(uint32_t event_id, uint8_t *data, int size);
void lt_uart_init(void);
bool lt_imei_acquired(char *imei_buffer);
void lt_imei_request(void);
#endif