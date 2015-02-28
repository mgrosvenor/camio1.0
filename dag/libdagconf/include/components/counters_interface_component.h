/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: counters_interface_component.h,v 1.1 2006/12/05 05:25:20 patricia Exp $
 */

#ifndef COUNTERS_INTERFACE_COMPONENT_H
#define COUNTERS_INTERFACE_COMPONENT_H

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
	uint32_t mStatusCache;

} counters_interface_state_t;
 

ComponentPtr
counters_interface_get_new_component(DagCardPtr card, uint32_t index);

#endif
