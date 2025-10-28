/*
 * @Descripttion:
 * @version:
 * @Author: zhouhao
 * @Date: 2025-09-17 10:30:13
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-23 13:42:37
 */
#ifndef LTE_CONTROLLER_H
#define LTE_CONTROLLER_H
#include <stdint.h>
#include <stdbool.h>

// LTE模块状态机
typedef enum
{
    LTE_STATE_OFF,
    LTE_STATE_POWERING_ON,
    LTE_STATE_WAITING_FOR_IMEI,
    LTE_STATE_READY, // 初始化完成
    LTE_STATE_ERROR,
} lte_state_t;

/**
 * @brief 初始化LTE控制器
 */
void lte_controller_init(void);

/**
 * @brief 启动LTE模块开机序列
 */
void lte_controller_power_on(void);

/**
 * @brief 启动LTE模块关机序列
 */
void lte_controller_power_off(void);

/**
 * @brief 获取当前LTE模块状态
 * @return lte_state_t
 */
lte_state_t lte_controller_get_state(void);

/**
 * @brief 设置当前LTE模块状态
 * @param state 要设置的状态
 */
void lte_controller_set_state(lte_state_t state);
/**
 * @brief 获取解析到的IMEI字符串
 * @return const char* IMEI字符串指针,如果未获取到则为NULL
 */
const char *lte_controller_get_imei(void);

/**
 * @brief 通过LTE模块发送数据 (UART透传)
 * @param data 数据指针
 * @param length 数据长度
 */
void lte_controller_send_data(uint8_t *data, uint16_t length);

/**
 * @brief LTE控制器周期性任务，驱动状态机
 */
void lte_controller_periodic_task(void);

#endif // LTE_CONTROLLER_H