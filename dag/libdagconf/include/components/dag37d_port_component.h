/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef DAG37D_PORT_COMPONENT_H
#define DAG37D_PORT_COMPONENT_H

#include "../card_types.h"
#include "../component_types.h"

ComponentPtr dag37d_get_new_port(DagCardPtr card, int index);
uint32_t dag37d_iom_read(DagCardPtr card, uint32_t base, uint32_t address);
void dag37d_iom_write(DagCardPtr card, uint32_t base,uint32_t address, uint32_t val);

#endif

