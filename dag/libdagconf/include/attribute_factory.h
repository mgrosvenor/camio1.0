/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef ATTRIBUTE_FACTORY_H
#define ATTRIBUTE_FACTORY_H

#include "attribute_types.h"
#include "card_types.h"
#include "component.h"
#include "card.h"
#include "attribute.h"
#include "modules/generic_read_write.h"

AttributePtr attribute_factory_make_attribute(dag_attribute_code_t code, GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);

#endif
