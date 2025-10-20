/**
 * Copyright (c) 2019, Freqchip
 * All rights reserved.
 */
#include <stdio.h>
#include <string.h>

#include "co_printf.h"
#include "jump_table.h"
#include "driver_plf.h"
#include "driver_system.h"
#include "driver_pmu.h"
#include "driver_uart.h"
#include "driver_efuse.h"
#include "driver_flash.h"
#include "flash_usage_config.h"
#include "gap_api.h" 

// 包含重构后的模块头文件
#include "lt_main.h"
#include "power_manager.h"

// 全局标志位，由ISR设置，由主循环处理
volatile bool g_wakeup_event = false;
volatile char g_wakeup_reason = 'U'; // U=Unknown, B=Button, L=LTE4

/*
 * =================================================================================
 * SDK JUMP TABLE & HOOKS (必需的底层接口)
 * =================================================================================
 */

const struct jump_table_version_t _jump_table_version __attribute__((section("jump_table_3"))) =
{
    .firmware_version = 0x00000001,
};

const struct jump_table_image_t _jump_table_image __attribute__((section("jump_table_1"))) =
{
    .image_type = IMAGE_TYPE_APP,
    .image_size = 0x20000,
};

void user_custom_parameters(void)
{
    struct chip_unique_id_t id_data;
    efuse_get_chip_unique_id(&id_data);
    
    id_data.unique_id[5] |= 0xc0; 
    memcpy(__jump_table.addr.addr, id_data.unique_id, 6);
    
    __jump_table.system_clk = SYSTEM_SYS_CLK_48M;
    jump_table_set_static_keys_store_offset(JUMP_TABLE_STATIC_KEY_OFFSET);
    ble_set_addr_type(BLE_ADDR_TYPE_PUBLIC); 
    retry_handshake();
}

void user_entry_before_ble_init(void)
{
    pmu_set_sys_power_mode(PMU_SYS_POW_BUCK);
#ifdef FLASH_PROTECT
    flash_protect_enable(1);
#endif
    // 仅初始化调试串口
	system_set_port_pull(GPIO_PA2, true);
	system_set_port_mux(GPIO_PORT_A, GPIO_BIT_2, PORTA2_FUNC_UART1_RXD);
	system_set_port_mux(GPIO_PORT_A, GPIO_BIT_3, PORTA3_FUNC_UART1_TXD);

	uart_init(UART1, BAUD_RATE_115200);
	NVIC_EnableIRQ(UART1_IRQn);
}

void user_entry_after_ble_init(void)
{
    co_printf("\r\n[PROJ_MAIN] Handing control to Main App#############...\r\n");
    system_sleep_disable();
    main_app_init(); 
    //    pmu_port_wakeup_func_clear(GPIO_PA1);
}

/*
 * =================================================================================
 * ISR & SLEEP/WAKEUP (安全的底层处理)
 * =================================================================================
 */
__attribute__((section("ram_code"))) void pmu_gpio_isr_ram(void)
{
    co_printf("\r\npmu_gpios_isr_ram strat\r\n");

    uint32_t gpio_value = ool_read32(PMU_REG_GPIOA_V);
    //	button_toggle_detected(gpio_value);
    ool_write32(PMU_REG_PORTA_LAST, gpio_value);
   button_toggle_detected(gpio_value);
    if ((gpio_value & (1 << 1))) { // PA1 button
        g_wakeup_reason = 'B'; 
    } else if ((gpio_value & (1 << 6))) { // PA6 from LTE
        g_wakeup_reason = 'L';
    } else {
        g_wakeup_reason = 'O';
    } 
    uart_putc_noint_no_wait(UART1, 'H'); 
    co_printf("pmu_gpios_isr_ram wakeup reason1: %c\r\n", g_wakeup_reason);

    if(g_wakeup_reason == 'B')
    {
        g_wakeup_reason = 'U';
        co_printf("WAKEUPCLEAR####\r\n");
        pmu_port_wakeup_func_clear(GPIO_PA1);
        main_app_restart_timers();
        system_sleep_disable();
    }
    g_wakeup_event = true; 
    co_printf("pmu_gpios_isr_ram wakeup reason2: %c\r\n", g_wakeup_reason);
}


__attribute__((section("ram_code"))) void user_entry_before_sleep_imp(void)
{
    static bool already_running_b = false;
	if (already_running_b)
    {
		return;
    }
	already_running_b = true;
    co_printf("######sss##\n");
    led_indicator_show_sleep();
    led_indicator_init();
    
  //   g_wakeup_reason = 'U';
 //   power_manager_prepare_for_sleep();
    already_running_b = false;
}

__attribute__((section("ram_code"))) void user_entry_after_sleep_imp(void)
{
    static bool already_running_a = false;
	if (already_running_a)
    {
		return;
    }
	already_running_a = true;
	system_set_port_pull(GPIO_PA2, true);
	system_set_port_mux(GPIO_PORT_A, GPIO_BIT_2, PORTA2_FUNC_UART1_RXD);
	system_set_port_mux(GPIO_PORT_A, GPIO_BIT_3, PORTA3_FUNC_UART1_TXD);

	uart_init(UART1, BAUD_RATE_115200);
	NVIC_EnableIRQ(UART1_IRQn);



    uart_putc_noint_no_wait(UART1, 'W');
    uart_putc_noint_no_wait(UART1, g_wakeup_reason);
    NVIC_EnableIRQ(PMU_IRQn);
    int n = 0;
    co_delay_100us(10 * 100);
    while(g_wakeup_reason == 'U')
    {
        n++;
    co_delay_100us(10 * 10);
	if (n > 100)
	{
			break;
	}
      //  system_sleep_enable();
    }
    n =0;
   	if (g_wakeup_reason == 'U')
	{
         uart_putc_noint_no_wait(UART1, 'K');
        // /1/     pmu_port_wakeup_func_set(GPIO_PA1);
		 system_sleep_enable();
	}
	// else
	// {
	// 	g_wakeup_reason = 'U';
    //      main_app_restart_timers();
    //      system_sleep_disable();
	// } 

    already_running_a = false;
}