/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef VSC8479_PORT_COMPONENT_H
#define VSC8479_PORT_COMPONENT_H

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
	uint32_t mStatusCache;

} vsc8479_state_t;
 
ComponentPtr vsc8479_get_new_component(DagCardPtr card, uint32_t index);

#endif

