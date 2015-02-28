/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: card_info_component.h,v 1.1 2007/04/16 01:07:02 trisha Exp $
 */

#ifndef CARD_INFO_COMPONENT_H
#define CARD_INFO_COMPONENT_H
 
typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
   
} card_info_state_t;

ComponentPtr
card_info_get_new_component(DagCardPtr card, uint32_t index);

#endif
