/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef PORT_COMPONENT_H
#define PORT_COMPONENT_H

/* port component. */
ComponentPtr dag71s_get_new_port(DagCardPtr card, int index);
void master_slave_from_string_routine(AttributePtr attribute, const char* string);
void master_slave_to_string_routine(AttributePtr attribute);

#endif

