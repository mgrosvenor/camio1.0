/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/util/enum_string_table.h"
#include "../../../include/dag_attribute_codes.h"

#include "dagutil.h"
#include "dagapi.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif


typedef struct
{
    uintptr_t mBase;
    uint32_t mVersion;
	uint32_t mIndex;
} steering_ipf_state_t;

/* steering component. */
static int steering_ipf_post_initialize(ComponentPtr component);
static void steering_default(ComponentPtr component);

/* steering value */
static AttributePtr steer_value_get_new_attribute(void);
static void steer_value_dispose(AttributePtr attribute);
static void* steer_value_get_value(AttributePtr attribute);
static void steer_value_set_value(AttributePtr attribute, void* value, int len);
static void steer_value_post_initialize(AttributePtr attribute);
static void steer_value_to_string(AttributePtr attribute);
static void steer_value_from_string(AttributePtr attribute, const char* string);

/* steer value */
AttributePtr
steer_value_get_new_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeSteer); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, steer_value_dispose);
        attribute_set_post_initialize_routine(result, steer_value_post_initialize);
        attribute_set_getvalue_routine(result, steer_value_get_value);
        attribute_set_setvalue_routine(result, steer_value_set_value);
        attribute_set_name(result, "steer_value");
        attribute_set_description(result, "The algorithm to use to steer the incoming packet.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, steer_value_to_string);
        attribute_set_from_string_routine(result, steer_value_from_string);
    }
    
    return result;
}

static void
steer_value_dispose(AttributePtr attribute)
{

}

static void
steer_value_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        const char* temp = NULL;
        steer_t steer = *(steer_t*)steer_value_get_value(attribute);
        temp = steer_to_string(steer);
        attribute_set_to_string(attribute, temp);
    }
}

static void
steer_value_from_string(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        steer_t steer = string_to_steer(string);
        steer_value_set_value(attribute, (void*)&steer, sizeof(steer_t));
    }
}

static void
steer_value_set_value(AttributePtr attribute, void* val, int len)
{
    DagCardPtr card;
    ComponentPtr component = NULL;
    steer_t value = *(steer_t*)val;
    steering_ipf_state_t* state = NULL;
    uint32_t regval = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        regval = card_read_iom(card, state->mBase);
        if (value == kSteerStream0)
        {
            regval &= ~(BIT12 | BIT13);
        }
        else if (value == kSteerColour)
        {
            regval &= ~(BIT12 | BIT13);
            regval |= BIT12;
        }
        card_write_iom(card, state->mBase, regval);
    }
}

static void*
steer_value_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    steer_t val = 0;
    ComponentPtr component;
    steering_ipf_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        val = card_read_iom(card, state->mBase) & (BIT12 | BIT13);
        if ((val & (BIT12 | BIT13)) == 0)
        {
            val = kSteerStream0;
        }
        else if ((val & (BIT12 | BIT13)) == BIT12)
        {
            val = kSteerColour;
        }
    }
    attribute_set_value_array(attribute, &val, sizeof(val));
    return (void *)attribute_get_value_array(attribute);
}

static void
steer_value_post_initialize(AttributePtr attribute)
{

}

ComponentPtr
steering_ipf_get_new_steering_ipf(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentSteering, card); 
    
    if (NULL != result)
    {
		steering_ipf_state_t* state = NULL;
        component_set_post_initialize_routine(result, steering_ipf_post_initialize);
        component_set_name(result, "steering_ipf");
		component_set_description(result, "Use to set how packets are steered.");
    	state = (steering_ipf_state_t*)malloc(sizeof(steering_ipf_state_t));
		state->mIndex = index;
    	component_set_private_state(result, state);
        component_set_default_routine(result, steering_default);
    }
    
    return result;
}


static int
steering_ipf_post_initialize(ComponentPtr component)
{
    AttributePtr attr = NULL;
    steering_ipf_state_t* state = NULL;
    DagCardPtr card = NULL;
    
    card = component_get_card(component);
	state = component_get_private_state(component);
    state->mBase = card_get_register_address(card, DAG_REG_STEERING, 0);
    state->mVersion = card_get_register_version(card, DAG_REG_STEERING, 0);
    attr = steer_value_get_new_attribute();
    component_add_attribute(component, attr);
    return 1;
}


static void
steering_default(ComponentPtr component)
{
    AttributePtr steer_attribute = NULL;
    steer_t      steer_default_value = kSteerStream0;

    if (1 == valid_component(component))
    {
        steer_attribute = component_get_attribute(component, kUint32AttributeSteer);
        if ( NULL == steer_attribute) 
        {
            return;
        }
        attribute_set_value(steer_attribute, (void*)&steer_default_value, sizeof(steer_t));
    }
}
