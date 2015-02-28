/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef MINI_MAC_STATISTICS_H
#define MINI_MAC_STATISTICS_H


typedef struct
{
    uint32_t mMiniMacBase;
    uint32_t mIndex;
    uint32_t mExtCache;
} mini_mac_statistics_state_t;


ComponentPtr mini_mac_statistics_get_new_component(DagCardPtr card, uint32_t index);

#endif


