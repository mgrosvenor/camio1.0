/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef LM63_H
#define LM63_H

#define SAFETY_ITERATIONS 5000
#define SAFETY_NANOSECONDS 100

ComponentPtr lm63_get_new_component(DagCardPtr card);

#endif

