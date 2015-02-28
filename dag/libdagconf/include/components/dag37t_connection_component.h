/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag37t_connection_component.h,v 1.3 2007/02/19 22:58:34 cassandra Exp $
 */

#ifndef DAG37T_CONNECTION_COMPONENT_H
#define DAG37T_CONNECTION_COMPONENT_H


typedef struct
{
    uint32_t mIndex;
}dag37t_connection_state_t;


ComponentPtr
dag37t_get_new_connection(DagCardPtr card, int index);

#endif

