/*
 * @Descripttion:
 * @version:
 * @Author: zhouhao
 * @Date: 2025-09-17 10:29:36
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-16 13:33:32
 */
#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "power_manager.h"

void led_indicator_init(void);
void led_indicator_update(battery_state_t state);
void led_indicator_blink_task(void);
void led_indicator_show_shutdown(void); // <-- 新增关机动画函数

void led_indicator_show_sleep(void); // <-- 新增睡眠动画函数
#endif                               // LED_INDICATOR_H