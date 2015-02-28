/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: cat_component.h,v 1.3 2007/09/24 04:12:48 karthik Exp $
 */

#ifndef CAT_COMPONENT_H
#define CAT_COMPONENT_H

typedef struct
{
    uint32_t mIndex;
    uint32_t mCatBase;
} cat_state_t;


ComponentPtr
cat_get_new_component(DagCardPtr card, uint32_t index);

#endif
