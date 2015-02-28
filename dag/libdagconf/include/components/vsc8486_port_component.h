/*
 * * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * * All rights reserved.
 * *
 * * This source code is proprietary to Endace Technology Limited and no part
 * * of it may be redistributed, published or disclosed except as outlined in
 * * the written contract supplied with this product.
 * *
 * */

#ifndef VSC8486_PORT_COMPONENT_H
#define VSC8486_PORT_COMPONENT_H
enum
{
    VSC8486_CONTROL_1                   = 0x0000,
    VSC8486_STATUS_1                    = 0x0001,
    VSC8486_PMA_DEV_PKG_1               = 0x0005, 
    VSC8486_CONTROL_2                   = 0x0007,
    VSC8486_STATUS_2                    = 0x0008,
    VSC8486_PCS_STATUS_10GBASE_R_1      = 0x0020,
    VSC8486_PCS_STATUS_10GBASE_R_2      = 0x0021,
    VSC8486_VS_CONTROL_PMA              = 0x8000,
    VSC8486_VS_CONTROL_PCS              = 0x8005,
    VSC8486_WIS_STATUS_3                = 0x0021,
    VSC8486_PMA_DEV_STATUS1             = 0xE606,
    VSC8486_DEV_CTRL_3                  = 0xe605,  
    VSC8486_PMA_LOS_STAT                = 0xe702,
    VSC8486_PMA_STAT_4                  = 0xE600,
    VSC8486_WIS_TX_C2_H1                = 0xE615,
    VSC8486_WIS_MODE_CONTROL            = 0xEC40,
    VSC8486_DEV_CONTROL_5               = 0xE602,
    VSC8486_SYNC_ETHERNET_CLK_CTRL      = 0xE604,
    VSC8486_WIS_INTERRUPT_STATUS        = 0xEF03
};
typedef struct
{
	uint32_t mIndex;
}vsc8486_state_t;


ComponentPtr 
vsc8486_get_new_port(DagCardPtr card, uint32_t index);

#endif

