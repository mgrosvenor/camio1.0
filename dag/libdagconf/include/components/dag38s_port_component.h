/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef DAG38S_PORT_COMPONENT_H
#define DAG38S_PORT_COMPONENT_H

ComponentPtr dag38s_get_new_port(DagCardPtr card, int index);
void dag38s_looptimer0_set_value(ComponentPtr port, uint8_t value);
void dag38s_looptimer1_set_value(ComponentPtr port, uint8_t value);
ComponentPtr sonic_v1_get_new_component(DagCardPtr card, uint32_t index);

#endif

