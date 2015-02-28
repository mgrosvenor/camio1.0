/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: oc48_deframer_component.h,v 1.3 2006/11/03 02:48:38 lipi Exp $
 */

#ifndef OC48_DEFRAMER_COMPONENT_H
#define OC48_DEFRAMER_COMPONENT_H
 
typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
	uint32_t mStatusCache;

    uint32_t mB1ErrorCounterCache;
    uint32_t mB2ErrorCounterCache;
    uint32_t mB3ErrorCounterCache;
    
    uint32_t mRXPacketCounterCache;
    uint32_t mRXByteCounterCache;
    uint32_t mTXPacketCounterCache;
    uint32_t mTXByteCounterCache;
    
    uint32_t mRXLongCounterCache;
    uint32_t mRXShortCounterCache;
    uint32_t mRXAbortCounterCache;
    uint32_t mRXFCSErrorCounterCache;
} oc48_deframer_state_t;

ComponentPtr
oc48_deframer_get_new_component(DagCardPtr card, uint32_t index);

#endif
