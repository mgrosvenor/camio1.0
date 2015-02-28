/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: counter_component.h,v 1.1 2007/02/26 05:33:08 patricia Exp $
 */

#ifndef COUNTER_COMPONENT_H
#define COUNTER_COMPONENT_H

typedef struct
{
    uint32_t mIndex;
	uint32_t mValueOffset;
	uint32_t mDescrOffset;
	uint32_t* mValueAddress;
	uint32_t* mDescrAddress;

} counter_state_t;
 

ComponentPtr
counter_get_new_component(DagCardPtr card, uint32_t index);

#endif
