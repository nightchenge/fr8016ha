/*
 * @Descripttion:
 * @version:
 * @Author: zhouhao
 * @Date: 2025-09-17 10:26:46
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-24 13:49:26
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "driver_iomux.h"
/*
 * =================================================================================
 * SYSTEM CONFIGURATION
 * =================================================================================
 */

// --- Battery Voltage Thresholds (in mV) ---
#define BATTERY_VOLTAGE_CRITICAL_SHUTDOWN 1465 // 低于此电压强制关机
#define BATTERY_VOLTAGE_LOW_RED 1550           // 低于此电压亮红灯
#define BATTERY_VOLTAGE_MEDIUM_YELLOW 1620     // 低于此电压亮黄灯
#define BATTERY_VOLTAGE_HIGH_GREEN 1630        // 高于此电压亮绿灯
#define BATTERY_VOLTAGE_HYSTERESIS 10          // 电压迟滞，防止LED在临界值附近闪烁
#define BATTERY_VOLTAGE_FULLY_CHARGED 2400     // 充电输入电压判断为充满的阈值

// --- GPIO Pin Definitions ---
// Power Key (Button)
#define POWER_KEY_PORT GPIO_PORT_A
#define POWER_KEY_PIN_BIT GPIO_BIT_1
#define POWER_KEY_GPIO GPIO_PA1

// LTE Module Control Pins
#define LTE_PWRKEY_PORT GPIO_PORT_A
#define LTE_PWRKEY_PIN_BIT GPIO_BIT_4

#define LTE_RESET_PORT GPIO_PORT_A
#define LTE_RESET_PIN_BIT GPIO_BIT_0

#define LTE_POWER_EN_PORT GPIO_PORT_A
#define LTE_POWER_EN_PIN_BIT GPIO_BIT_7

#define LTE_WAKEUP_MCU_PORT GPIO_PORT_A
#define LTE_WAKEUP_MCU_PIN_BIT GPIO_BIT_6
#define LTE_WAKEUP_MCU_GPIO GPIO_PA6

// --- Timing Constants (in ms) ---
#define LED_BLINK_HALF_PERIOD_MS 500 // LED闪烁半周期 (500ms), 即亮500ms, 灭500ms
#define SYSTEM_TICK_MS 100           // 系统主任务循环周期
#define ADC_REFRESH_INTERVAL_MS 2000 // ADC采样周期
#define LTE_POWER_ON_PULSE_MS 2100   // LTE模块开机脉冲时长
#define LTE_POWER_ON_STABLE_MS 2500  // LTE模块开机后稳定时间
#define LTE_CMD_TIMEOUT_MS 2000      // 获取IMEI的超时时间

// --- UART Configuration ---
#define UART0_BAUDRATE BAUD_RATE_115200 // To LTE Module
#define UART1_BAUDRATE BAUD_RATE_115200 // Debug/Other

// --- BLE Configuration ---
#define BLE_ADVERTISING_INTERVAL_MIN 300
#define BLE_ADVERTISING_INTERVAL_MAX 300
#define BLE_DEVICE_NAME_PREFIX "LT-HHT-"
#define BLE_DEVICE_NAME_PREFIX_ZR "ZR-ZHRHT-"
#define SW_VERSION "2.00.07"
#endif // _CONFIG_H_