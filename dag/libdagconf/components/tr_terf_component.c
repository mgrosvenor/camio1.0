/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: tr_terf_component.c,v 1.4 2008/02/05 01:42:45 vladimir Exp $
 */
//This is not used any more please use terf_component.c which combiines all of the terf versions 

#include "dagutil.h"

#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/tr_terf_component.h"
#include "../include/attribute_factory.h"

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: tr_terf_component.c,v 1.4 2008/02/05 01:42:45 vladimir Exp $";
static const char* const kRevisionString = "$Revision: 1.4 $";

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} tr_terf_state_t;

enum
{
    CONTROL_REGISTER                = 0x00,
    ABSOLUTE_MODE_OFFSET_REGISTER   = 0x04,
    CONFIGURABLE_LIMIT_REGISTER     = 0x08
};

/* terf component. */
static void tr_terf_dispose(ComponentPtr component);
static void tr_terf_reset(ComponentPtr component);
static void tr_terf_default(ComponentPtr component);
static int tr_terf_post_initialize(ComponentPtr component);
static dag_err_t tr_terf_update_register_base(ComponentPtr component);

ComponentPtr
terf_get_new_tr_terf(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentTrTerf, card); 
    tr_terf_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, tr_terf_dispose);
        component_set_post_initialize_routine(result, tr_terf_post_initialize);
        component_set_reset_routine(result, tr_terf_reset);
        component_set_default_routine(result, tr_terf_default);
        component_set_update_register_base_routine(result, tr_terf_update_register_base);
        component_set_name(result, "tr_terf");
        state = (tr_terf_state_t*)malloc(sizeof(tr_terf_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
tr_terf_dispose(ComponentPtr component)
{
}

static dag_err_t
tr_terf_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        tr_terf_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_TERF64, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
tr_terf_post_initialize(ComponentPtr component)
{
    AttributePtr attr = NULL;
    tr_terf_state_t* state = NULL;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t bool_attr_mask = 0;
    uint32_t version = 0;
    uintptr_t address = 0;

    state = component_get_private_state(component);
    card = component_get_card(component);

    state->mBase = card_get_register_address(card, DAG_REG_TERF64, 0);
    
    /* Get the version of the component */
    version = card_get_register_version(card, DAG_REG_TERF64, 0);
  
    /* create a subcomponent TR_TERF */
    address = ((uintptr_t)card_get_iom_address(card)) + state->mBase;
    
    /* Attribute for time released function */
    /* Add the RX-Error A attribute */
    bool_attr_mask = BIT2;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRXErrorA, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    
  
    /* Add the RX-Error B attribute */
    bool_attr_mask = BIT3;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRXErrorB, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    
    
    /* Add the RX-Error C attribute */
    bool_attr_mask = BIT4;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRXErrorC, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    

    /* Add the RX-Error D attribute */
    bool_attr_mask = BIT5;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRXErrorD, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    
 
    /* Add the Time mode attribute */
    bool_attr_mask = BIT6|BIT7;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeTimeMode, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    
    
    /* Add the Scale Range attribute */
    bool_attr_mask = BIT9|BIT10|BIT11|BIT12|BIT13|BIT14;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeScaleRange, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);
    
    /* Add the Shift Direction attribute */
    bool_attr_mask = BIT15;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeShiftDirection, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    
   
    return 1;
}

static void
tr_terf_reset(ComponentPtr component)
{
}


static void
tr_terf_default(ComponentPtr component)
{
}


