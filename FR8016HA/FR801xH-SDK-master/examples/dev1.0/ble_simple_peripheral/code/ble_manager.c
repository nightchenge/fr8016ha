/*
 * @Descripttion:
 * @version:
 * @Author: zhouhao
 * @Date: 2025-09-17 10:31:28
 * @LastEditors: zhouhao
 * @LastEditTime: 2025-09-18 10:31:48
 */
#include "ble_manager.h"
#include "config.h"
#include "gap_api.h"
#include "gatt_api.h"
#include "lt_gatt_server.h"
#include "ota_service.h"
#include "co_printf.h"
#include "string.h"
#include "lte_controller.h" // 需要获取LTE状态

// 广播包和服务数据
static uint8_t adv_data[] = {
    0x03,
    GAP_ADVTYPE_16BIT_MORE,
    0xFF,
    0xFE,
};

// 扫描响应包，动态填充
static uint8_t scan_rsp_data[32] = {0}; // 增加一点余量

static void ble_gap_evt_cb(gap_event_t *p_event);

void ble_manager_init(void)
{
    uint8_t local_name[] = "lootom hht";
    gap_set_dev_name(local_name, sizeof(local_name));

    gap_security_param_t param = {
        .mitm = false,
        .ble_secure_conn = false,
        .io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
        .pair_init_mode = GAP_PAIRING_MODE_WAIT_FOR_REQ,
        .bond_auth = true,
        .password = 0,
    };
    gap_security_param_init(&param);
    gap_set_cb_func(ble_gap_evt_cb);
    gap_set_mtu(247);

    // 添加GATT服务
    lt_gatt_add_service();
    ota_gatt_add_service();
    co_printf("BLE Manager Initialized.\r\n");
}

void ble_manager_start_advertising(const char *imei)
{
    if (imei == NULL || strlen(imei) == 0)
    {
        co_printf("Cannot start advertising: IMEI is null or empty.\r\n");
        return;
    }

    gap_adv_param_t adv_param;
    adv_param.adv_mode = GAP_ADV_MODE_UNDIRECT;
    adv_param.adv_addr_type = GAP_ADDR_TYPE_PUBLIC;
    adv_param.adv_chnl_map = GAP_ADV_CHAN_ALL;
    adv_param.adv_filt_policy = GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    adv_param.adv_intv_min = BLE_ADVERTISING_INTERVAL_MIN;
    adv_param.adv_intv_max = BLE_ADVERTISING_INTERVAL_MAX;
    gap_set_advertising_param(&adv_param);

    // 构造扫描响应包
    uint8_t prefix[] = BLE_DEVICE_NAME_PREFIX;
    uint8_t prefix_len = sizeof(prefix) - 1;
    uint8_t imei_len = strlen(imei);

    scan_rsp_data[0] = 1 + prefix_len + imei_len; // name length
    scan_rsp_data[1] = GAP_ADVTYPE_LOCAL_NAME_COMPLETE;
    memcpy(&scan_rsp_data[2], prefix, prefix_len);
    memcpy(&scan_rsp_data[2 + prefix_len], imei, imei_len);

    uint8_t power_level_pos = 2 + prefix_len + imei_len;
    scan_rsp_data[power_level_pos] = 0x02; // length of tx power
    scan_rsp_data[power_level_pos + 1] = GAP_ADVTYPE_POWER_LEVEL;
    scan_rsp_data[power_level_pos + 2] = 0x00; // 0dBm

    uint8_t total_len = power_level_pos + 3;

    gap_set_advertising_data(adv_data, sizeof(adv_data));
    gap_set_advertising_rsp_data(scan_rsp_data, total_len);

    gap_start_advertising(0);
    co_printf("BLE Advertising Started with name: %s%s\r\n", prefix, imei);
}

void ble_manager_stop_advertising(void)
{
    gap_stop_advertising();
    co_printf("BLE Advertising Stopped.\r\n");
}

// 新增函数实现
void ble_manager_disconnect_all(void)
{
    uint8_t conn_num = gap_get_connect_num();
    co_printf("Disconnecting %d BLE links...\r\n", conn_num);
    for (uint8_t i = 0; i < conn_num; i++)
    {
        // 通常外设只有一个连接，conidx为0
        gap_disconnect_req(i);
    }
}

void ble_manager_send_notification(const uint8_t *data, uint16_t length)
{
    // 调用GATT服务中的函数发送通知
    notify_ble_to_dev((uint8_t *)data, length);
}

static void ble_gap_evt_cb(gap_event_t *p_event)
{
    switch (p_event->type)
    {
    case GAP_EVT_ADV_END:
        co_printf("adv_end, status: 0x%02x\r\n", p_event->param.adv_end.status);
        break;
    case GAP_EVT_ALL_SVC_ADDED:
        co_printf("All BLE services added.\r\n");
        // 服务添加完成后，由外部逻辑决定何时开始广播
        break;
    case GAP_EVT_SLAVE_CONNECT:
        co_printf("slave[%d] connected. link_num: %d\r\n", p_event->param.slave_connect.conidx, gap_get_connect_num());
        gatt_mtu_exchange_req(p_event->param.slave_connect.conidx);
        break;
    case GAP_EVT_DISCONNECT:
        co_printf("Link[%d] disconnected, reason: 0x%02X\r\n", p_event->param.disconnect.conidx, p_event->param.disconnect.reason);
        // 断开连接后重新开始广播
        if (lte_controller_get_state() == LTE_STATE_READY)
        {
            ble_manager_start_advertising(lte_controller_get_imei());
        }
        break;
    case GAP_EVT_MTU:
        co_printf("mtu update, conidx=%d, mtu=%d\r\n", p_event->param.mtu.conidx, p_event->param.mtu.value);
        break;
    default:
        break;
    }
}