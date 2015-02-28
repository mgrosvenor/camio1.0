/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef CREATE_ATTRIBUTE_H
#define CREATE_ATTRIBUTE_H


/* File header. */

/* Public API headers. */

/* Internal project headers. */
#include "card_types.h"
#include "util/utility.h"
#include "modules/latch.h"
#include "modules/generic_read_write.h"
#include "attribute_factory.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined (__linux__)
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dagutil.h"

/* Software implementation of an attribute. */
typedef struct
{
    char* mName;
    dag_attribute_code_t mAttributeCode;
    dag_attr_t mAttributeType;
    char* mDescription;
    dag_attr_config_status_t mConfigStatus;
    uint32_t mIndex;                        /* Index of the mask in the register */
    dag_reg_module_t mReg;                  /* Register Address */  
    uint32_t mOffset;                       /* Offset */
    /* Used to read/write values to the card (register). Hides the details of reading/writing and allows for multiple ways to do this. */
    uint32_t mLength;
    GenericReadRoutine fRead;
    GenericWriteRoutine fWrite;
    uint32_t mMask;                         /* mask to apply on value returned from reading/writing*/
    uint32_t mDefaultVal;                   /* Default value */
    /* Methods. */
    AttributeSetValueRoutine fSetValue;
    AttributeGetValueRoutine fGetValue;
    AttributeToStringRoutine fToString;      /* return a string representation of the attribute's value. */
    AttributeFromStringRoutine fFromString;  /* Set the attribute value using a string representation.
                                                Useful for enumerated types when the user wants to set the value via pretty printing. */
    AttributeDisposeRoutine fDispose;
    AttributePostInitializeRoutine fPostInitialize;   
}Attribute_t;


AttributePtr
create_attribute(uint32_t index_tab, ComponentPtr component, Attribute_t tab[], uint32_t num_module);

void
read_attr_array(ComponentPtr component, Attribute_t array[], int nb_elem, uint32_t num_mod);

#endif /* CREATE_ATTRIBUTE_H */
