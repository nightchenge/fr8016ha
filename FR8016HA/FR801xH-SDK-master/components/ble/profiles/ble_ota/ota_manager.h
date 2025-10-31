/*
 * @Descripttion: 
 * @version: 
 * @Author: zhouhao
 * @Date: 2025-10-28 13:59:57
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-28 14:42:07
 */
/*
 * ota_manager.h
 * * OTA 路由层，用于管理 BLE 和 UART 的 OTA 请求。
 */
#ifndef _OTA_MANAGER_H
#define _OTA_MANAGER_H

#include <stdint.h>

/**
 * @brief 定义一个特殊的 "Connection Index" (连接索引) 来代表 UART 通道
 * * BLE 升级时，conidx 通常是 0 或 1。
 * 我们使用 0xFF (255) 来唯一标识 UART 通道。
 */
#define UART_OTA_CONIDX   (0xFF)

/**
 * @brief 初始化 OTA 管理器
 * * 此函数将配置 UART0 并注册中断，使其准备好接收 OTA 命令。
 */
void ota_manager_init(void);
void ota_manager_process_packet(uint8_t *p_data, uint16_t len);
#endif // _OTA_MANAGER_H