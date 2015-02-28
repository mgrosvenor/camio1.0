/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef SONET_CONNECTION_COMPONENT_H
#define SONET_CONNECTION_COMPONENT_H

typedef struct
{
    uint32_t mIndex;
}sonet_connection_state_t;

ComponentPtr sonet_get_new_connection(DagCardPtr card, uint32_t index);

#endif
