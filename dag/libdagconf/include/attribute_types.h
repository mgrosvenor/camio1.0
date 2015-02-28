/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef ATTRIBUTE_TYPES_H
#define ATTRIBUTE_TYPES_H

#if defined (__linux__)
#include <inttypes.h>
#endif

/* Forward declarations. */
typedef struct AttributeObject AttributeObject;
typedef AttributeObject* AttributePtr;


/* Method signatures. */
typedef void (*AttributeDisposeRoutine)(AttributePtr self);
typedef void (*AttributePostInitializeRoutine)(AttributePtr self);
typedef void* (*AttributeGetValueRoutine)(AttributePtr self);
typedef void (*AttributeSetValueRoutine)(AttributePtr self, void* value, int length);
typedef void (*AttributeToStringRoutine)(AttributePtr self);
typedef void (*AttributeFromStringRoutine)(AttributePtr self, const char* string);
typedef const char* (*AttributeGetNameRoutine)(AttributePtr self);
typedef const char* (*AttributeGetDescriptionRoutine)(AttributePtr self);

#endif /* ATTRIBUTE_TYPES_H */

