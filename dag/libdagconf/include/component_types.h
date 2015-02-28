/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef COMPONENT_TYPES_H
#define COMPONENT_TYPES_H

#include "dag_config.h"
#include "./card_types.h"
/* Forward declarations. */
typedef struct CardComponent CardComponent;
typedef CardComponent* ComponentPtr;


/* Method signatures. */
typedef void (*ComponentDisposeRoutine)(ComponentPtr component);
typedef int (*ComponentPostInitializeRoutine)(ComponentPtr component); /* Return 1 to continue with recursive post-initialize, 0 if not. */
typedef void (*ComponentResetRoutine)(ComponentPtr component);
typedef void (*ComponentDefaultRoutine)(ComponentPtr component);
typedef dag_err_t (*ComponentUpdateRegisterBaseRoutine)(ComponentPtr component);
typedef ComponentPtr (*ComponentCreatorRoutine)(DagCardPtr card, uint32_t index);

#endif /* COMPONENT_TYPES_H */
