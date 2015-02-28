/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef VSC8476_PORT_COMPONENT_H
#define VSC8476_PORT_COMPONENT_H
enum
{
    CONTROL_1                   = 0x0000,
    STATUS_1                    = 0x0001,
    CONTROL_2                   = 0x0007,
    STATUS_2                    = 0x0008,
    PCS_STATUS_10GBASE_R_1      = 0x0020,
    PCS_STATUS_10GBASE_R_2      = 0x0021,
    VS_CONTROL_PMA              = 0x8000,
    VS_CONTROL_PCS              = 0x8005
};

enum
{
    PMA_DEVICE  = 1,
    PCS_DEVICE  = 3
};
ComponentPtr vsc8476_get_new_port(DagCardPtr card, uint32_t index);

#endif

