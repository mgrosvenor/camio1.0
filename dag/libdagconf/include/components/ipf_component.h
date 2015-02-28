/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * This file includes the declaration  of stream feature component.
 * At the moment supports only per-stream SNAP_LEN attribute.
 *
* $Id: 
 */

#ifndef IPF_COMPONENT_H
#define IPF_COMPONENT_H

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
}ipf_state_t;

ComponentPtr
ipf_get_new_component(DagCardPtr card, uint32_t index);

#endif // IPF_COMPONENT_H
