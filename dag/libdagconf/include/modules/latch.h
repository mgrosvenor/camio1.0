/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef LATCH_H
#define LATCH_H

#include "dag_platform.h"
#include "../card.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* 
Defines a latch object for use by attributes (counters) that need to latch
something before reading it.
*/


/* Forward declarations. */
typedef struct LatchObject LatchObject;
typedef LatchObject* LatchPtr;

LatchPtr latch_init(DagCardPtr card, uint32_t address, uint32_t mask);
void latch_dispose(LatchPtr latch);
void latch_set(LatchPtr latch);
void latch_clear(LatchPtr latch);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ATTRIBUTE_H */
