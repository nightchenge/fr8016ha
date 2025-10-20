#include "led_indicator.h"
#include "config.h"
#include "driver_pmu.h"
#include "sys_utils.h"

// 定义LED显示状态，用于实现防抖状态机
typedef enum {
    LED_DISPLAY_STATE_UNINITIALIZED,
    LED_DISPLAY_STATE_GREEN,
    LED_DISPLAY_STATE_YELLOW,
    LED_DISPLAY_STATE_RED,
} led_display_state_t;

// 将闪烁周期(毫秒)转换为系统心跳的次数
#define BLINK_INTERVAL_TICKS (LED_BLINK_HALF_PERIOD_MS / SYSTEM_TICK_MS)

static bool is_blinking = false;
static bool blink_on = false;
static led_display_state_t g_current_led_display_state = LED_DISPLAY_STATE_UNINITIALIZED;
static uint32_t blink_counter = 0; // 新增：闪烁计数器

void led_indicator_init(void) {
    pmu_set_led1_as_pwm();
    pmu_set_led2_as_pwm();
    pmu_set_led1_value(0); // Green ON
    pmu_set_led2_value(0);
    g_current_led_display_state = LED_DISPLAY_STATE_UNINITIALIZED;
}

void led_indicator_update(battery_state_t state) {
    co_printf("LED Indicator Update: %d\r\n", state);
    if (state == BATTERY_STATE_CHARGING) {
        if (!is_blinking) { // 仅在状态初次变为“充电中”时重置
            is_blinking = true;
            blink_counter = 0;
            blink_on = false; // 确保从“灭”的状态开始闪
        }
        g_current_led_display_state = LED_DISPLAY_STATE_UNINITIALIZED;
        return;
    } 
    else if (state == BATTERY_STATE_FULLY_CHARGED) {
        is_blinking = false;
        pmu_set_led1_value(0); // Green ON
        pmu_set_led2_value(1);
        g_current_led_display_state = LED_DISPLAY_STATE_UNINITIALIZED;
        return; 
    }

    is_blinking = false; // 如果不是充电状态，则确保停止闪烁
    
    switch (state) {
        case BATTERY_STATE_HIGH:
            if (g_current_led_display_state != LED_DISPLAY_STATE_GREEN) {
                pmu_set_led1_value(0); // Green ON
                pmu_set_led2_value(1);
                g_current_led_display_state = LED_DISPLAY_STATE_GREEN;
            }
            break;
        case BATTERY_STATE_MEDIUM:
            if (g_current_led_display_state != LED_DISPLAY_STATE_YELLOW) {
                pmu_set_led1_value(1); // Yellow ON
                pmu_set_led2_value(1);
                g_current_led_display_state = LED_DISPLAY_STATE_YELLOW;
            }
            break;
        case BATTERY_STATE_LOW:
        case BATTERY_STATE_CRITICAL:
            if (g_current_led_display_state != LED_DISPLAY_STATE_RED) {
                pmu_set_led1_value(1); // Red ON
                pmu_set_led2_value(0);
                g_current_led_display_state = LED_DISPLAY_STATE_RED;
            }
            break;
        default:
            pmu_set_led1_value(0);
            pmu_set_led2_value(0);
            break;
    }
}

void led_indicator_blink_task(void) {
    if (!is_blinking) {
        return;
    }

    blink_counter++;
    if (blink_counter >= BLINK_INTERVAL_TICKS) {
        blink_counter = 0; // 重置计数器
        
        if (blink_on) {
            pmu_set_led1_value(0);
            pmu_set_led2_value(0); // Turn off
            blink_on = false;
        } else {
            pmu_set_led1_value(0);

            pmu_set_led2_value(1); // Turn on Green
            blink_on = true;
        }
    }
}
// --- 恢复您源文件中的关机动画 ---
void led_indicator_show_shutdown(void) {
    pmu_set_led2_value(0);
    pmu_set_led1_value(1);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(0);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(1);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(0);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(1);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(0);
    pmu_set_led2_value(0);
}

void led_indicator_show_sleep(void)
{
    pmu_set_led2_value(1);
    pmu_set_led1_value(1);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(0);
    pmu_set_led2_value(0);
    co_delay_100us(200 * 10);
    pmu_set_led2_value(1);
    pmu_set_led1_value(1);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(0);
    pmu_set_led2_value(0);
    co_delay_100us(200 * 10);
    pmu_set_led2_value(1);
    pmu_set_led1_value(1);
    co_delay_100us(200 * 10);
    pmu_set_led1_value(0);
    pmu_set_led2_value(0);
}