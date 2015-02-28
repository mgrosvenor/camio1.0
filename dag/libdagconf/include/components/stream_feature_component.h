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
* $Id: stream_feature_component.h,v 1.2 2008/08/05 02:20:52 jomi Exp $
 */

#ifndef STREAM_FEATURE_COMPONENT_H
#define STREAM_FEATURE_COMPONENT_H

typedef struct
{
    uintptr_t mBase;
    uint32_t mIndex;
}stream_feature_state_t;

ComponentPtr
stream_feature_get_new_component(DagCardPtr card, uint32_t index);

#endif // STREAM_FEATURE_COMPONENT_H
