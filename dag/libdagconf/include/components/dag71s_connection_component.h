/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag71s_connection_component.h,v 1.4 2007/02/19 22:58:34 cassandra Exp $
 */

#ifndef DAG71S_CONNECTION_COMPONENT_H
#define DAG71S_CONNECTION_COMPONENT_H


typedef struct
{
    uint32_t mIndex;
    uintptr_t mBase;
    dag_component_code_t mComponentType;
    volatile uint8_t* mIom;

}dag71s_connection_state_t;

typedef struct
{
    uint32_t mBase;
   	ComponentPtr mConnection[2048];
} parent_state_t;

ComponentPtr
dag71s_get_new_connection(DagCardPtr card, int index);

#endif

