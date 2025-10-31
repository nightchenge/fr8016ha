/*
 * @Descripttion: 
 * @version: 
 * @Author: zhouhao
 * @Date: 2025-10-28 14:03:30
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-10-28 15:03:59
 */
/*
 * ota_manager.c
 * * 实现了 OTA 响应路由和 UART0 协议解析。
 */
#include <string.h>
#include "ota_manager.h"
#include "ota.h"                //
#include "ota_service.h"        //
#include "driver_uart.h"        //
#include "driver_iomux.h"       //
#include "os_mem.h"             //
#include "sys_utils.h"          //

// 定义 UART 协议
#define UART_PACKET_SOF 0xAA // 帧头 (Start of Frame)
#define UART_RX_BUF_SIZE 620 // 必须大于 OTAS_MAX_DATA_SIZE (600)

// UART 接收状态
typedef enum {
    UART_STATE_WAIT_SOF,
    UART_STATE_WAIT_LEN_H,
    UART_STATE_WAIT_LEN_L,
    UART_STATE_WAIT_PAYLOAD,
    UART_STATE_WAIT_CRC,
} uart_rx_state_t;

// UART 接收状态机变量
static uart_rx_state_t g_uart_state = UART_STATE_WAIT_SOF;
static uint8_t  g_rx_buf[UART_RX_BUF_SIZE];
static uint16_t g_payload_len = 0;
static uint16_t g_rx_idx = 0;
static uint8_t  g_checksum = 0;


/*
 * =================================================================================
 * 关键函数 1: OTA 响应路由
 * * ota.c 会调用这个函数 来发送响应。
 * 我们在这里判断响应应该发给 BLE 还是 UART。
 * =================================================================================
 */
void ota_gatt_report_notify(uint8_t conidx, uint8_t *p_data, uint16_t len)
{
    if (conidx == UART_OTA_CONIDX)
    {
        // 1. 目的地是 UART
        
        // 2. 将响应数据封装成 UART 协议包 (格式: [SOF][LenH][LenL][Payload][CRC])
        uint16_t packet_len = len + 5; // 1(SOF) + 2(Len) + Payload(len) + 1(CRC)
        uint8_t *tx_buf = (uint8_t *)os_malloc(packet_len);
        if (tx_buf == NULL) {
            co_printf("OTA UART TX: Malloc failed\r\n");
            return;
        }

        tx_buf[0] = UART_PACKET_SOF;   // 帧头
        tx_buf[1] = (len >> 8) & 0xFF; // 长度高位
        tx_buf[2] = len & 0xFF;        // 长度低位
        memcpy(&tx_buf[3], p_data, len);

        // 3. 计算一个简单的异或校验和
        uint8_t crc = 0;
        for(uint16_t i = 0; i < (packet_len - 1); i++) {
            crc ^= tx_buf[i];
        }
        tx_buf[packet_len - 1] = crc;

        // 4. 通过 UART0 发送
        uart_write(UART0_BASE, tx_buf, packet_len, NULL);
        uart_finish_transfers(UART0_BASE); // 确保发送完成

        os_free(tx_buf);
    }
    else
    _ {
        // 目的地是 BLE，调用我们改名后的 BLE 发送函数
        // (见步骤 3)
        ota_service_send_ble_notify(conidx, p_data, len);
    }
}


/*
 * =================================================================================
 * 关键函数 2: UART 数据包处理
 * =================================================================================
 */
void ota_manager_process_packet(uint8_t *p_data, uint16_t len)
{
    // [!!] OTA 互斥锁检查 [!!]
    // 检查 ota.c 中的 OTA 状态是否空闲
    if (app_get_ota_state() != 0)
    {
        co_printf("OTA Manager: Busy! BLE OTA may be in progress.\r\n");
        // (可选) 你可以在这里回复一个 "Busy" 的 UART 包
        return;
    }

    // OTA 空闲，调用 ota.c 的核心入口函数
    // 注意：我们传入了特殊的 UART_OTA_CONIDX
    app_otas_recv_data(UART_OTA_CONIDX, p_data, len);
}

/*
 * =================================================================================
 * 关键函数 3: UART 字节接收状态机
 * =================================================================================
 */
void ota_manager_uart_rx_byte(uint8_t byte)
{
    switch (g_uart_state)
    {
        case UART_STATE_WAIT_SOF:
            if (byte == UART_PACKET_SOF) {
                g_checksum = byte; // 校验和从 SOF 开始计算
                g_rx_idx = 0;
                g_payload_len = 0;
                g_uart_state = UART_STATE_WAIT_LEN_H;
            }
            break;

        case UART_STATE_WAIT_LEN_H:
            g_payload_len = (byte << 8);
            g_checksum ^= byte;
            g_uart_state = UART_STATE_WAIT_LEN_L;
            break;

        case UART_STATE_WAIT_LEN_L:
            g_payload_len |= byte;
            g_checksum ^= byte;
            if (g_payload_len > 0 && g_payload_len <= UART_RX_BUF_SIZE) {
                g_rx_idx = 0;
                g_uart_state = UART_STATE_WAIT_PAYLOAD;
            } else {
                co_printf("OTA UART: Invalid len %d\r\n", g_payload_len);
                g_uart_state = UART_STATE_WAIT_SOF; // 长度错误，复位
            }
            break;

        case UART_STATE_WAIT_PAYLOAD:
            g_rx_buf[g_rx_idx++] = byte;
            g_checksum ^= byte;
            if (g_rx_idx == g_payload_len) {
                g_uart_state = UART_STATE_WAIT_CRC;
            }
            break;

        case UART_STATE_WAIT_CRC:
            if (g_checksum == byte) {
                // 校验成功! 处理数据包
                ota_manager_process_packet(g_rx_buf, g_payload_len);
            } else {
                co_printf("OTA UART: CRC Error! (calc: %02X, recv: %02X)\r\n", g_checksum, byte);
            }
            g_uart_state = UART_STATE_WAIT_SOF; // 无论成功失败，都复位
            break;
            
        default:
            g_uart_state = UART_STATE_WAIT_SOF;
            break;
    }
}


/*
 * =================================================================================
 * 关键函数 4: UART0 初始化和中断处理
 * =================================================================================
 */

// UART0 接收中断回调函数
static void ota_uart0_rx_handler(void)
{
    uint8_t byte;
    // 循环读取 FIFO 中的所有字节
    while(uart_read_data_wait(UART0_BASE, &byte)) {
        ota_manager_uart_rx_byte(byte);
    }
}

// ota_manager 初始化函数
void ota_manager_init(void)
{
    // // [!!] 请根据你的硬件原理图修改 UART0 的 IO 引脚 [!!]
    // // 示例使用 P04 (TXD) 和 P05 (RXD)
    // system_set_port_mux(GPIO_PORT_0, GPIO_BIT_4, PORT_MUX_FUNC_UART0_TXD);
    // system_set_port_mux(GPIO_PORT_0, GPIO_BIT_5, PORT_MUX_FUNC_UART0_RXD);

    // // 配置 UART0
    // struct uart_conf conf;
    // conf.baudrate = UART_BAUDRATE_115200;
    // conf.data_bits = UART_DATABITS_8;
    // conf.stop_bits = UART_STOPBITS_1;
    // conf.parity = UART_PARITY_NONE;
    // conf.flow_control = UART_FLOWCONTROL_NONE;
    // conf.rx_limit = 0; // 尽快触发中断
    // conf.dma_en = false;
    // conf.auto_flow_control = false;

    // // 初始化 UART0
    // uart_init(UART0_BASE, &conf);
    
    // 注册 UART0 接收回调
    // uart_callback_register(UART0_BASE, ota_uart0_rx_handler);

    // g_uart_state = UART_STATE_WAIT_SOF;
    // co_printf("UART0 OTA Manager Initialized.\r\n");
}