#include "power_manager.h"
#include "config.h"
#include "driver_adc.h"
#include "driver_pmu.h"
#include "driver_system.h"
#include "sys_utils.h"
#include "os_timer.h"
#include "co_printf.h"
#include "driver_gpio.h"
#include "led_indicator.h" // 需要调用关机动画

static battery_state_t g_current_battery_state = BATTERY_STATE_HIGH;
static os_timer_t g_adc_timer;

static void update_battery_state(void);
static void adc_timer_callback(void *arg);
static void ltsys_adc_init(void);
static void get_average_vbat_vols(uint16_t *vbat_vol4, uint16_t *vbat_vol5, uint16_t *vbat_vol6);

/*
 * =================================================================================
 * Public Functions
 * =================================================================================
 */

void power_manager_init(void)
{
    ltsys_adc_init();
    update_battery_state();
    os_timer_init(&g_adc_timer, adc_timer_callback, NULL);
    os_timer_start(&g_adc_timer, ADC_REFRESH_INTERVAL_MS, true);
    co_printf("Power Manager Initialized.\r\n");
}

void power_manager_check_on_boot(void)
{
    update_battery_state();
    if (g_current_battery_state == BATTERY_STATE_CRITICAL)
    {
        co_printf("Battery critical on boot. Shutting down.\r\n");
        power_manager_shutdown();
    }
}

battery_state_t power_manager_get_state(void)
{
    return g_current_battery_state;
}

void power_manager_prepare_for_sleep(void)
{
    // 新增: 停止应用层定时器
    main_app_stop_timers();
    co_printf("\r\n Preparing for sleep...\r\n");
    os_timer_stop(&g_adc_timer);
    adc_reset();

    //    co_printf("Preparing for sleep end...\r\n");
}

void power_manager_handle_wakeup(void)
{
    co_printf("Handling wakeup...\r\n");
    ltsys_adc_init();
    os_timer_start(&g_adc_timer, ADC_REFRESH_INTERVAL_MS, true);
}
extern volatile char g_wakeup_reason;
void power_manager_request_sleep(void)
{
    co_printf("Sleep requested.\r\n");
    power_manager_prepare_for_sleep();
    g_wakeup_reason = 'U';
    system_sleep_enable();
}

void power_manager_shutdown(void)
{
    co_printf("System shutting down!\r\n");
    // 执行关机前的LED动画
    led_indicator_show_shutdown();
    adc_disable();
    system_power_off(1);
}

/*
 * =================================================================================
 * Private (Static) Functions
 * =================================================================================
 */

static void adc_timer_callback(void *arg)
{
    update_battery_state();
}

static void update_battery_state(void)
{
    uint16_t vbat_vol4, vbat_vol5, vbat_vol6;
    get_average_vbat_vols(&vbat_vol4, &vbat_vol5, &vbat_vol6);
    co_printf("val_pd4:%d,val_pd5:%d,val_pd6:%d.\r\n", vbat_vol4, vbat_vol5, vbat_vol6);

    // --- 恢复您源文件中的电压校准逻辑 ---
    uint16_t adjusted_vbat_vol4 = vbat_vol4;
    uint16_t adjusted_vbat_vol5 = vbat_vol5;

    if (vbat_vol4 < 1000 && vbat_vol5 > 1000)
    { // typec 连接
        adjusted_vbat_vol5 = vbat_vol5 + 400;
        adjusted_vbat_vol4 = 0;
    }
    else if (vbat_vol4 > 1000 && vbat_vol5 > 1000)
    {
        adjusted_vbat_vol4 += 400;
        adjusted_vbat_vol5 += 400;
    }
    else
    {
        adjusted_vbat_vol4 = 0;
        adjusted_vbat_vol5 = 0;
    }

    // --- 恢复您源文件中的状态判断逻辑 ---
    if (adjusted_vbat_vol4 == 0 && adjusted_vbat_vol5 != 0)
    {
        g_current_battery_state = BATTERY_STATE_CHARGING;
    }
    else if (adjusted_vbat_vol4 >= BATTERY_VOLTAGE_FULLY_CHARGED && adjusted_vbat_vol5 >= BATTERY_VOLTAGE_FULLY_CHARGED)
    {
        g_current_battery_state = BATTERY_STATE_FULLY_CHARGED;
    }
    else
    {
        if (vbat_vol6 < BATTERY_VOLTAGE_CRITICAL_SHUTDOWN)
        {
            g_current_battery_state = BATTERY_STATE_CRITICAL;
        }
        else if (vbat_vol6 < BATTERY_VOLTAGE_LOW_RED)
        {
            g_current_battery_state = BATTERY_STATE_LOW;
        }
        else if (vbat_vol6 < BATTERY_VOLTAGE_MEDIUM_YELLOW)
        {
            g_current_battery_state = BATTERY_STATE_MEDIUM;
        }
        else
        {
            g_current_battery_state = BATTERY_STATE_HIGH;
        }
    }

    // 如果检测到电量过低，立即关机
    if (g_current_battery_state == BATTERY_STATE_CRITICAL)
    {
        co_printf("v111al_pd4:%d,val_pd5:%d,val_pd6:%d.\r\n", vbat_vol4, vbat_vol5, vbat_vol6);
        // power_manager_shutdown();
    }
}

static void ltsys_adc_init(void)
{
    struct adc_cfg_t cfg;

    system_set_port_mux(GPIO_PORT_D, GPIO_BIT_4, PORTD4_FUNC_ADC0);
    system_set_port_mux(GPIO_PORT_D, GPIO_BIT_5, PORTD5_FUNC_ADC1);
    system_set_port_mux(GPIO_PORT_D, GPIO_BIT_6, PORTD6_FUNC_ADC2);

    memset((void *)&cfg, 0, sizeof(cfg));
    cfg.src = ADC_TRANS_SOURCE_PAD;
    cfg.ref_sel = ADC_REFERENCE_AVDD;
    cfg.channels = 0x07;
    cfg.route.pad_to_sample = 1;
    // cfg.route.pad_to_div=1;
    cfg.clk_sel = ADC_SAMPLE_CLK_24M_DIV13;
    cfg.clk_div = 0x3f;

    adc_init(&cfg);
    adc_enable(NULL, NULL, 0);
    pmu_set_aldo_voltage(PMU_ALDO_MODE_NORMAL, PMU_ALDO_VOL_3_5);
    adc_set_ref_voltage(3300, 3300);
}
static void get_average_vbat_vols(uint16_t *vbat_vol4, uint16_t *vbat_vol5, uint16_t *vbat_vol6)
{
    const int samples = 10;
    uint16_t results_vol4[samples];
    uint16_t results_vol5[samples];
    uint16_t results_vol6[samples];
    uint16_t result[3] = {0};
    uint16_t ref_vol = 0;

    // 进行 10 次采样
    for (int i = 0; i < samples; i++)
    {
        adc_get_result(ADC_TRANS_SOURCE_PAD, 0x01, &result[0]);
        adc_get_result(ADC_TRANS_SOURCE_PAD, 0x02, &result[1]);
        adc_get_result(ADC_TRANS_SOURCE_PAD, 0x04, &result[2]);
        ref_vol = adc_get_ref_voltage(ADC_REFERENCE_AVDD);
        results_vol4[i] = result[0] * ref_vol / 1024;
        results_vol5[i] = result[1] * ref_vol / 1024;
        results_vol6[i] = result[2] * ref_vol / 1024;
    }

    // 处理 vbat_vol4
    uint16_t max_val_vol4 = results_vol4[0];
    uint16_t min_val_vol4 = results_vol4[0];
    uint32_t sum_vol4 = 0;

    for (int i = 0; i < samples; i++)
    {
        if (results_vol4[i] > max_val_vol4)
        {
            max_val_vol4 = results_vol4[i];
        }
        if (results_vol4[i] < min_val_vol4)
        {
            min_val_vol4 = results_vol4[i];
        }
        sum_vol4 += results_vol4[i];
    }

    // 去掉 vbat_vol4 的最大值和最小值
    sum_vol4 = sum_vol4 - max_val_vol4 - min_val_vol4;

    // 计算 vbat_vol4 的平均值
    *vbat_vol4 = (uint16_t)(sum_vol4 / (samples - 2));

    // 处理 vbat_vol5
    uint16_t max_val_vol5 = results_vol5[0];
    uint16_t min_val_vol5 = results_vol5[0];
    uint32_t sum_vol5 = 0;

    for (int i = 0; i < samples; i++)
    {
        if (results_vol5[i] > max_val_vol5)
        {
            max_val_vol5 = results_vol5[i];
        }
        if (results_vol5[i] < min_val_vol5)
        {
            min_val_vol5 = results_vol5[i];
        }
        sum_vol5 += results_vol5[i];
    }

    // 去掉 vbat_vol5 的最大值和最小值
    sum_vol5 = sum_vol5 - max_val_vol5 - min_val_vol5;

    // 计算 vbat_vol5 的平均值
    *vbat_vol5 = (uint16_t)(sum_vol5 / (samples - 2));

    // 处理 vbat_vol6
    uint16_t max_val_vol6 = results_vol6[0];
    uint16_t min_val_vol6 = results_vol6[0];
    uint32_t sum_vol6 = 0;

    for (int i = 0; i < samples; i++)
    {
        if (results_vol6[i] > max_val_vol6)
        {
            max_val_vol6 = results_vol6[i];
        }
        if (results_vol6[i] < min_val_vol6)
        {
            min_val_vol6 = results_vol6[i];
        }
        sum_vol6 += results_vol6[i];
    }

    // 去掉 vbat_vol6 的最大值和最小值
    sum_vol6 = sum_vol6 - max_val_vol6 - min_val_vol6;

    // 计算 vbat_vol6 的平均值
    *vbat_vol6 = (uint16_t)(sum_vol6 / (samples - 2));
}