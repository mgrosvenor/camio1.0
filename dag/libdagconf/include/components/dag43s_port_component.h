/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef DAG43S_PORT_COMPONENT_H
#define DAG43S_PORT_COMPONENT_H

ComponentPtr dag43s_get_new_port(DagCardPtr card, int index);
void dag43s_looptimer0_set_value(ComponentPtr port, uint8_t value);
void dag43s_looptimer1_set_value(ComponentPtr port, uint8_t value);

#endif

