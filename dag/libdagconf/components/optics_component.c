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
#include "../include/components/optics_component.h"

#include "dagutil.h"

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
    uint32_t mOpticsBase;
    uint32_t mPortNumber;
} optics_state_t;

/* detect attribute. */
static AttributePtr optics_get_new_detect_attribute();
static void optics_detect_dispose(AttributePtr attribute);
static void* optics_detect_get_value(AttributePtr attribute);
static void optics_detect_post_initialize(AttributePtr attribute);


/* optics  */
static void optics_dispose(ComponentPtr component);
static void optics_reset(ComponentPtr component);
static void optics_default(ComponentPtr component);
static int optics_post_initialize(ComponentPtr component);
static dag_err_t optics_update_register_base(ComponentPtr component);

/* signal */
static AttributePtr optics_get_new_signal_attribute();
static void optics_signal_dispose(AttributePtr attribute);
static void* optics_signal_get_value(AttributePtr attribute);
static void optics_signal_post_initialize(AttributePtr attribute);

/* sfppwr */
static AttributePtr optics_get_new_sfppwr_attribute();
static void optics_sfppwr_dispose(AttributePtr attribute);
static void* optics_sfppwr_get_value(AttributePtr attribute);
static void optics_sfppwr_post_initialize(AttributePtr attribute);
static void optics_sfppwr_set_value(AttributePtr attribute, void* value, int length);

/* laser */
static AttributePtr optics_get_new_laser_attribute();
static void optics_laser_dispose(AttributePtr attribute);
static void* optics_laser_get_value(AttributePtr attribute);
static void optics_laser_post_initialize(AttributePtr attribute);
static void optics_laser_set_value(AttributePtr attribute, void* value, int length);

#define BUFFER_SIZE 1024

/* detect */
AttributePtr
optics_get_new_detect_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeDetect); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, optics_detect_dispose);
        attribute_set_post_initialize_routine(result, optics_detect_post_initialize);
        attribute_set_getvalue_routine(result, optics_detect_get_value);
        attribute_set_name(result, "detect");
        attribute_set_description(result, "Indicates whether the optics module is present.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
optics_detect_dispose(AttributePtr attribute)
{

}

static void*
optics_detect_get_value(AttributePtr attribute)
{
    ComponentPtr component;
    DagCardPtr card;
    uint8_t return_value = 0;
    uint32_t register_value = 0;
    uint32_t mask = 0;
    if (1 == valid_attribute(attribute))
    {
        optics_state_t* state = NULL;
        component = attribute_get_component(attribute);
        state = (optics_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
		register_value = card_read_iom(card, state->mOpticsBase);
        mask = BIT0 << state->mPortNumber*4;
        return_value = (register_value & mask) == mask;
        attribute_set_value_array(attribute, &return_value, sizeof(return_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
optics_detect_post_initialize(AttributePtr attribute)
{

}

ComponentPtr
optics_get_new_optics(DagCardPtr card, int port_number)
{
    ComponentPtr result = component_init(kComponentOptics, card); 
    optics_state_t* state = NULL;
    char buffer[BUFFER_SIZE];
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, optics_dispose);
        component_set_post_initialize_routine(result, optics_post_initialize);
        component_set_reset_routine(result, optics_reset);
        component_set_default_routine(result, optics_default);
        component_set_update_register_base_routine(result, optics_update_register_base);
        sprintf(buffer, "optics%d", port_number);
        component_set_name(result, buffer);
        state = (optics_state_t*)malloc(sizeof(optics_state_t));
        memset(state, 0, sizeof(optics_state_t));
        state->mPortNumber = port_number;
        component_set_private_state(result, state);
        
    }
    
    return result;
}


static void
optics_dispose(ComponentPtr component)
{
}

static dag_err_t
optics_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        optics_state_t* state = (optics_state_t*)component_get_private_state(component);
        state->mOpticsBase = 0x300;
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
optics_post_initialize(ComponentPtr component)
{
    AttributePtr laser;
    AttributePtr sfppwr;
    AttributePtr detect;
    AttributePtr signal;
    optics_state_t* state = (optics_state_t*)component_get_private_state(component);

    state->mOpticsBase = 0x300;
    laser = optics_get_new_laser_attribute();
    sfppwr = optics_get_new_sfppwr_attribute();
    detect = optics_get_new_detect_attribute();
    signal = optics_get_new_signal_attribute();

    component_add_attribute(component, laser);
    component_add_attribute(component, sfppwr);
    component_add_attribute(component, detect);
    component_add_attribute(component, signal);

    /* Return 1 if standard post_initialize() should continue, 0 if not.
    * "Standard" post_initialize() calls post_initialize() recursively on subcomponents.
    */
    return 1;
}

static void
optics_reset(ComponentPtr component)
{
}


static void
optics_default(ComponentPtr component)
{
}

/* buffer size */
AttributePtr
optics_get_new_signal_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeSignal); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, optics_signal_dispose);
        attribute_set_post_initialize_routine(result, optics_signal_post_initialize);
        attribute_set_getvalue_routine(result, optics_signal_get_value);
        attribute_set_name(result, "signal");
        attribute_set_description(result, "Indicates whether the optics module detects an input signal.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
optics_signal_dispose(AttributePtr attribute)
{

}

static void*
optics_signal_get_value(AttributePtr attribute)
{
    uint8_t return_value = 0;
    uint32_t register_value = 0;
    uint32_t mask = 0;
    optics_state_t* state = NULL;
    ComponentPtr component = NULL;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (optics_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
        mask = (BIT1 << (state->mPortNumber*4));
        register_value = card_read_iom(card, state->mOpticsBase);
        return_value = (register_value & mask) == mask;
        attribute_set_value_array(attribute, &return_value, sizeof(return_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
optics_signal_post_initialize(AttributePtr attribute)
{

}

/* laser attribute*/
AttributePtr
optics_get_new_laser_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLaser); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, optics_laser_dispose);
        attribute_set_post_initialize_routine(result, optics_laser_post_initialize);
        attribute_set_getvalue_routine(result, optics_laser_get_value);
        attribute_set_setvalue_routine(result, optics_laser_set_value);
        attribute_set_name(result, "laser");
        attribute_set_description(result, "Configures the transmit laser to be on or off.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
optics_laser_dispose(AttributePtr attribute)
{

}

static void*
optics_laser_get_value(AttributePtr attribute)
{
    uint8_t return_value = 0;
    optics_state_t* state = NULL;
    ComponentPtr component = NULL;
    uint32_t register_value = 0;
    uint32_t mask = 0;
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (optics_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
        mask = BIT3 << (state->mPortNumber*4);
		register_value = card_read_iom(card, state->mOpticsBase);
        return_value = (register_value & mask) == mask;
        attribute_set_value_array(attribute, &return_value, sizeof(return_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
optics_laser_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr component = NULL;
    optics_state_t* state = NULL;
    uint32_t val = 0;
    DagCardPtr card = NULL;

    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = component_get_card(component);
        if (*(uint8_t*)value == 0)
        {
            /* turn off laser */
            val = card_read_iom(card, state->mOpticsBase);
            switch (state->mPortNumber)
            {
                case 0:
                {
                    val &= ~BIT3;
                    break;
                }

                case 1:
                {
                    val &= ~BIT7;
                    break;
                }
                
                case 2:
                {
                    val &= ~BIT11;
                    break;
                }

                case 3:
                {
                    val &= ~BIT15;
                    break;
                }
            }
            card_write_iom(card, state->mOpticsBase, val);
        }
        else
        {
            val = card_read_iom(card, state->mOpticsBase);
            switch (state->mPortNumber)
            {
                case 0:
                {
                    val |= BIT3;
                    break;
                }

                case 1:
                {
                    val |= BIT7;
                    break;
                }
                
                case 2:
                {
                    val |= BIT11;
                    break;
                }

                case 3:
                {
                    val |= BIT15;
                    break;
                }
            }
            card_write_iom(card, state->mOpticsBase, val);
        }
    }
}

static void
optics_laser_post_initialize(AttributePtr attribute)
{

}

/* sfppwr count */
AttributePtr
optics_get_new_sfppwr_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeSfpPwr); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, optics_sfppwr_dispose);
        attribute_set_post_initialize_routine(result, optics_sfppwr_post_initialize);
        attribute_set_getvalue_routine(result, optics_sfppwr_get_value);
        attribute_set_setvalue_routine(result, optics_sfppwr_set_value);
        attribute_set_name(result, "sfppwr");
        attribute_set_description(result, "Enable or disable optics transmit power.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
optics_sfppwr_dispose(AttributePtr attribute)
{

}

static void*
optics_sfppwr_get_value(AttributePtr attribute)
{
    uint8_t return_value = 0;
    uint32_t register_value = 0;
    uint32_t mask = 0;
    optics_state_t* state = NULL;
    ComponentPtr component = NULL;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (optics_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
        mask = BIT2 << (state->mPortNumber*4);
		register_value = card_read_iom(card, state->mOpticsBase);
        return_value = (register_value & mask) == mask;
         attribute_set_value_array(attribute, &return_value, sizeof(return_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
optics_sfppwr_set_value(AttributePtr attribute, void* value, int length)
{
	ComponentPtr component = NULL;
    optics_state_t* state = NULL;
    uint32_t register_value = 0;
    DagCardPtr card = NULL;
  
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = component_get_card(component);
        if (*(uint8_t*)value == 0)
        {
            register_value = card_read_iom(card, state->mOpticsBase);
            switch (state->mPortNumber)
            {
                case 0:
                {
                    register_value &= ~BIT2;
                    break;
                }
                case 1:
                {
                    register_value &= ~BIT6;
                    break;
                }
                case 2:
                {
                    register_value &= ~BIT10;
                    break;
                }
                case 3:
                {
                    register_value &= ~BIT14;
                    break;
                }
            }
            card_write_iom(card, state->mOpticsBase, register_value);
        }
        else
        {
            register_value= card_read_iom(card, state->mOpticsBase);
            switch (state->mPortNumber)
            {
                case 0:
                {
                    register_value |= BIT2;
                    break;
                }
                case 1:
                {
                    register_value |= BIT6;
                    break;
                }
                case 2:
                {
                    register_value |= BIT10;
                    break;
                }
                case 3:
                {
                    register_value |= BIT14;
                    break;
                }
            }
            card_write_iom(card, state->mOpticsBase, register_value);
        }
    }
}


static void
optics_sfppwr_post_initialize(AttributePtr attribute)
{

}

uint32_t
optics_get_port_number(ComponentPtr component)
{
    optics_state_t* state = NULL;
    state = (optics_state_t*)component_get_private_state(component);
    return state->mPortNumber;
}

void
optics_set_port_number(ComponentPtr component, uint32_t val)
{
    optics_state_t* state = NULL;
    state = (optics_state_t*)component_get_private_state(component);
    state->mPortNumber = val;
}

