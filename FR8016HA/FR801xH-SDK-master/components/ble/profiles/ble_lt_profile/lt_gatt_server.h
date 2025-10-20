#ifndef LT_GATT_SERVER_H
#define LT_GATT_SERVER_H

/*
 * INCLUDES 
 */
#include <stdio.h>
#include <string.h>
#include "gap_api.h"
#include "gatt_api.h"
#include "gatt_sig_uuid.h"


enum
{
    LT_IDX_SERVICE,

    LT_IDX_CHAR1_DECLARATION,
    LT_IDX_CHAR1_VALUE,
	LT_IDX_CHAR1_CFG,
    LT_IDX_CHAR1_USER_DESCRIPTION,

    LT_IDX_CHAR2_DECLARATION,
    LT_IDX_CHAR2_VALUE,
    LT_IDX_CHAR2_CFG,
    LT_IDX_CHAR2_USER_DESCRIPTION,

    LT_IDX_CHAR3_DECLARATION,
    LT_IDX_CHAR3_VALUE,
    LT_IDX_CHAR3_USER_DESCRIPTION,
    
    LT_IDX_NB,
};

// Simple GATT Profile Service UUID
#define LT_SVC_UUID              0xFFF0

#define LT_CHAR1_UUID            0XFFF1
#define LT_CHAR2_UUID         0XFFF2   
#define LT_CHAR3_UUID            0xFFF3
//#define SP_CHAR4_UUID            0xFFF4
//#define SP_CHAR5_UUID            0xFFF5

int notify_ble_to_dev(unsigned char *data,int  size);




#endif
