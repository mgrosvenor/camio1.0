/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * This file includes the declaration  of GTP phy component.
  *
* $Id: gtp_phy_component.h,v 1.1 2008/03/12 00:29:24 jomi Exp $
 */

#ifndef GTP_PHY_COMPONENT_H
#define GTP_PHY_COMPONENT_H

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
}gtp_phy_state_t;

ComponentPtr
gtp_phy_get_new_component(DagCardPtr card, uint32_t index);

#endif // GTP_PHY_COMPONENT_H
