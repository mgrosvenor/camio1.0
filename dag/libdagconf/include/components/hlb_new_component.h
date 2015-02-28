/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: hlb_new_component.h,v 1.4.2.1 2009/07/01 23:47:50 karthik Exp $
 */

#ifndef HLB_NEW_COMPONENT_H
#define HLB_NEW_COMPONENT_H

#define DAG_STREAM_MAX_SUPPORTED 32

typedef struct
{
	uint32_t mIndex;
	uint32_t *mHatBase;
} hat_state_t;

ComponentPtr
hat_get_new_component(DagCardPtr card, uint32_t index);

#endif
