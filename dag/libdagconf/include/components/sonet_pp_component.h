/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 * this the sonet component definition which handles 
 * the DAG 5.0s, 5.2sxa, 8.1sxa 
 * it is combined dfreamer and demapper4 and POS or ATM extrator 
 * the ATM and chanal;ized is not implemeted in the fir,mware or software
 * POS and concatenated done 
 * $Id: sonet_pp_component.h,v 1.1 2007/05/02 23:50:41 vladimir Exp $
 */

#ifndef SONET_PP_COMPONENT_H
#define SONET_PP_COMPONENT_H
 
typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
	uint32_t mStatusCache;
	uint32_t mSonetStatusCache;
	uint32_t mPosStatusCache;

    uint32_t mB1ErrorCounterCache;
    uint32_t mB2ErrorCounterCache;
    uint32_t mB3ErrorCounterCache;
    uint32_t mREICounterCache;
   
} sonet_pp_state_t;

ComponentPtr
sonet_pp_get_new_component(DagCardPtr card, uint32_t index);

#endif
