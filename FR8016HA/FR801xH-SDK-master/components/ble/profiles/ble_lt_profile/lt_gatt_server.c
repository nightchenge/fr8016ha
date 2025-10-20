#include "lt_gatt_server.h"
#include <stdio.h>
#include <string.h>
#include "co_printf.h"
#include "gap_api.h"
#include "gatt_api.h"
#include "gatt_sig_uuid.h"
#include "driver_uart.h"
#include "user_task.h"



#if 1
// Simple GATT Profile Service UUID: 0xFFF0
const uint8_t lt_svc_uuid[] = UUID16_ARR(LT_SVC_UUID);

/******************************* Characteristic 1 defination *******************************/
// Characteristic 1 UUID: 0xFFF1
#define LT_CHAR1_VALUE_LEN 100
//uint8_t lt_char1_value[LT_CHAR1_VALUE_LEN] = {0};
// Characteristic 1 User Description
#define LT_CHAR1_DESC_LEN 30
const uint8_t lt_char1_desc[LT_CHAR1_DESC_LEN] = "Characteristic 1";
const uint8_t lt_char2_desc[LT_CHAR1_DESC_LEN] = "Characteristic 2";


uint8_t lt_svc_id = 0;
uint8_t ltntf_char1_enable = 0;
static gatt_service_t ltsimple_profile_svc;

const gatt_attribute_t ltsimple_profile_att_table[LT_IDX_NB] =
{
	[LT_IDX_SERVICE]={{ UUID_SIZE_2, UUID16_ARR(GATT_PRIMARY_SERVICE_UUID) },     /* UUID */
                                                    GATT_PROP_READ,                                             /* Permissions */
                                                    UUID_SIZE_2,                                                /* Max size of the value */     /* Service UUID size in service declaration */
                                                    (uint8_t*)lt_svc_uuid,                                      /* Value of the attribute */    /* Service UUID value in service declaration */
                                                },
   
    [LT_IDX_CHAR1_DECLARATION]={{ UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },           					/* UUID */
                                                    GATT_PROP_READ,                                             /* Permissions */
                                                    0,                                                          /* Max size of the value */
                                                    NULL,                                                       /* Value of the attribute */
							   },
	
	[LT_IDX_CHAR1_VALUE]={{ UUID_SIZE_2, UUID16_ARR(LT_CHAR1_UUID) },                 										/* UUID */
                                                    GATT_PROP_READ | GATT_PROP_NOTI|GATT_PROP_WRITE,            /* Permissions */
                                                    LT_CHAR1_VALUE_LEN,                                         /* Max size of the value */
                                                    NULL,                                                       /* Value of the attribute */ 
                         },
	[LT_IDX_CHAR1_CFG]={ { UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID) },     										/* UUID */
                                                    GATT_PROP_READ | GATT_PROP_WRITE|GATT_PROP_NOTI,                           /* Permissions */
                                                    100,                                           				/* Max size of the value */
                                                    NULL,                                                       /* Value of the attribute */
						},
	
    [LT_IDX_CHAR1_USER_DESCRIPTION]={ { UUID_SIZE_2, UUID16_ARR(GATT_CHAR_USER_DESC_UUID) },      				/* UUID */
                                                    GATT_PROP_READ,                                             /* Permissions */
                                                    LT_CHAR1_DESC_LEN,                                          /* Max size of the value */
                                                    (uint8_t *)lt_char1_desc,                                   /* Value of the attribute */
                                     },
    [LT_IDX_CHAR2_DECLARATION]={{ UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID) },           					/* UUID */
                                                    GATT_PROP_READ,                                             /* Permissions */
                                                    0,                                                          /* Max size of the value */
                                                    NULL,                                                       /* Value of the attribute */
							   },

	[LT_IDX_CHAR2_VALUE]={ { UUID_SIZE_2, UUID16_ARR(LT_CHAR2_UUID) },     										/* UUID */
                                                    GATT_PROP_READ | GATT_PROP_NOTI|GATT_PROP_WRITE,                           /* Permissions */
                                                    LT_CHAR1_VALUE_LEN,                                           				/* Max size of the value */
                                                    NULL,                                                       /* Value of the attribute */
						},
	[LT_IDX_CHAR2_CFG]={ { UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID) },     										/* UUID */
                                                    GATT_PROP_READ | GATT_PROP_WRITE|GATT_PROP_NOTI,                           /* Permissions */
                                                    100,                                           				/* Max size of the value */
                                                    NULL,                                                       /* Value of the attribute */
						},
	[LT_IDX_CHAR2_USER_DESCRIPTION]={ { UUID_SIZE_2, UUID16_ARR(GATT_CHAR_USER_DESC_UUID) },      				/* UUID */
                                                    GATT_PROP_READ,                                             /* Permissions */
                                                    LT_CHAR1_DESC_LEN,                                          /* Max size of the value */
                                                    (uint8_t *)lt_char2_desc,                                   /* Value of the attribute */
                                     },
   
};

static void show_reg(uint8_t *data,uint32_t len,uint8_t dbg_on)
{
    uint32_t i=0;
    if(len == 0 || (dbg_on==0)) return;
    for(; i<len; i++)
    {
        co_printf("0x%02X,",data[i]);
    }
    co_printf("\r\n");
}
static void ltntf_data(uint8_t con_idx,uint8_t att_idx,uint8_t *data,uint16_t len)
{
		gatt_ntf_t ntf_att;
		ntf_att.att_idx = att_idx;
		ntf_att.conidx = con_idx;
		ntf_att.svc_id = lt_svc_id;
		ntf_att.data_len = len;
		ntf_att.p_data = data;
		gatt_notification(ntf_att);
}

void my_uart_callback(void* arg, uint8_t data) 
{
	co_printf("send data to 4g cat1 over.\r\n");
	//ltntf_data(0,LT_IDX_CHAR1_VALUE,"ok",strlen("ok"));
    // 在这里实现回调函数的逻辑
    // 可以使用传递进来的参数 arg 和 data 进行处理
}

//extern int lowersleep;
extern int pa1wakeflag;
int notify_ble_to_dev(uint8_t *data,int  size)
{
	if(size !=0)
	{
	if (data[0] == 0xA9 && data[1] == 0xA9 && data[2] == 0x01)
	{
		// lowersleep=1;
#if 1
		if (data[3] == 0x01)
		{
			int value =0;
			memcpy(&value,&data[4],4);
			co_printf("4g adc %d \n", value);
		}
#endif
	}
		if(data[0]==0xA4 && data[1]==0xA4 && data[2]==0x01)
		{
			//lowersleep=1;	
			#if 1
			if(data[3] == 0x01)
			{
			// 	pa1wakeflag=0;
			// 	//led_send_event(GRELED_OFF_COMMAND);
			// 	pmu_set_led1_value(0);
			// 	pmu_set_led2_value(0);
			// 	gap_stop_conn();
			// 	lt_blestop();
			// 	gap_stop_advertising();
			// 	adc_disable();
			// 	lt_main_timer_stop();
			// 	adc_timer_stop();
			// 	//delay_ms(500);
			// 	system_sleep_enable();
			// ...
			co_printf("Received sleep command from LTE module.\r\n");
			// 1. 停止BLE功能
			ble_manager_stop_advertising();
			ble_manager_disconnect_all(); // 断开所有BLE连接

			// 2. 发出系统睡眠请求
			power_manager_request_sleep();
			
			// ...
			 }
			#endif
		}
		else
		{
			ltntf_data(0,LT_IDX_CHAR1_VALUE,data,size);
		}
	}
	return 0;
}



static uint16_t lt_gatt_msg_handler(gatt_msg_t *p_msg)
{
	co_printf("lootom log:p_msg->msg_evt:%d.\r\n",p_msg->msg_evt);
	switch(p_msg->msg_evt)
	{
		case GATTC_MSG_READ_REQ:
		{
			co_printf("p_msg->att_idx:%d\r\n",p_msg->att_idx);
			if (p_msg->att_idx == LT_IDX_CHAR1_VALUE)
			{

			}
		}
		break;
		case GATTC_MSG_WRITE_REQ:
		{
			co_printf("p_msg->att_idx:%d\r\n",p_msg->att_idx);
			if (p_msg->att_idx == LT_IDX_CHAR1_VALUE)
			{
				//co_printf("char1_recv:");
				show_reg(p_msg->param.msg.p_msg_data,p_msg->param.msg.msg_len,1);
				//uart_write()
				uart0_write(p_msg->param.msg.p_msg_data, p_msg->param.msg.msg_len, my_uart_callback); 
			}
			else if (p_msg->att_idx == LT_IDX_CHAR2_VALUE)
			{
				co_printf("char2_recv:");
				show_reg(p_msg->param.msg.p_msg_data,p_msg->param.msg.msg_len,1);
			}
			else if(p_msg->att_idx==LT_IDX_CHAR1_CFG)
			{
				co_printf("char1_ntf_enable:");
				show_reg(p_msg->param.msg.p_msg_data,p_msg->param.msg.msg_len,1);
				if(p_msg->param.msg.p_msg_data[0] & 0x1)
					ltntf_char1_enable = 1;
				if(ltntf_char1_enable)
					ltntf_data(p_msg->conn_idx,LT_IDX_CHAR1_VALUE,(uint8_t *)"char1_ntf_data can notify app",strlen("char2_ntf_data can notify app"));
			}
			else if(p_msg->att_idx==LT_IDX_CHAR2_CFG)
			{
				co_printf("char2_ntf_enable:");
				show_reg(p_msg->param.msg.p_msg_data,p_msg->param.msg.msg_len,1);
				if(p_msg->param.msg.p_msg_data[0] & 0x1)
					ltntf_char1_enable = 1;
				if(ltntf_char1_enable)
					ltntf_data(p_msg->conn_idx,LT_IDX_CHAR2_VALUE,(uint8_t *)"char2_ntf_data can notify app",strlen("char2_ntf_data can notify app"));
			}
		}
		break;
		case GATTC_MSG_LINK_CREATE:
			co_printf("link_created\r\n");
		break;
		case GATTC_MSG_LINK_LOST:
			co_printf("link_lost\r\n");
			//ntf_char1_enable = 0;
		break;    
		default:
		break;
	}
	return p_msg->param.msg.msg_len;
}

void lt_gatt_add_service(void)
{
	ltsimple_profile_svc.p_att_tb = ltsimple_profile_att_table;
	ltsimple_profile_svc.att_nb = LT_IDX_NB;
	ltsimple_profile_svc.gatt_msg_handler = lt_gatt_msg_handler;
	
	lt_svc_id = gatt_add_service(&ltsimple_profile_svc);
}



#endif


