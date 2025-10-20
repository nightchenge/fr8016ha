/*
 * @Descripttion: 
 * @version: 
 * @Author: zhouhao
 * @Date: 2025-09-17 10:30:56
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-09-18 10:31:25
 */
#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <stdint.h>

/**
 * @brief 初始化BLE管理器
 */
void ble_manager_init(void);

/**
 * @brief 根据IMEI启动BLE广播
 * @param imei 从LTE模块获取到的IMEI字符串
 */
void ble_manager_start_advertising(const char *imei);

/**
 * @brief 停止BLE广播
 */
void ble_manager_stop_advertising(void);

/**
 * @brief 断开所有当前的BLE连接
 */
void ble_manager_disconnect_all(void); // <-- 新增函数

/**
 * @brief 向连接的设备发送数据
 * @param data 数据指针
 * @param length 数据长度
 */
void ble_manager_send_notification(const uint8_t *data, uint16_t length);

#endif // BLE_MANAGER_H