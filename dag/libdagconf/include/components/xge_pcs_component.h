/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: xge_pcs_component.h,v 1.2 2007/03/29 03:42:54 patricia Exp $
 */

#ifndef XGE_PCS_COMPONENT_H
#define XGE_PCS_COMPONENT_H
 
typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
    uint32_t mStatusCache;
   
} xge_pcs_state_t;

ComponentPtr
xge_pcs_get_new_component(DagCardPtr card, uint32_t index);

#endif
