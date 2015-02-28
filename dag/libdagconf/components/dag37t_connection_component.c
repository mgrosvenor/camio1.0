/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag37t_connection_component.c,v 1.8 2008/04/07 23:34:15 vladimir Exp $
 */

/* *
 * This is the connection component for the DAG 3.7T
*/

/* File Header */

#include "dagutil.h"
#include "dag_platform.h"

#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/channel_config/dag37t_chan_cfg.h"

#include "../include/components/dag37t_connection_component.h"

#define BUFFER_SIZE 1024

static void dag37t_connection_dispose(ComponentPtr component);
static void dag37t_connection_reset(ComponentPtr component);
static void dag37t_connection_default(ComponentPtr component);
static int dag37t_connection_post_initialize(ComponentPtr component);


/* Active Attribute */
static AttributePtr get_new_active_attribute(void);
static void* active_attribute_get_value(AttributePtr attribute);

/* direction attribute. */
static AttributePtr get_new_direction_attribute(void);
static void* direction_attribute_get_value(AttributePtr attribute);

/* port number attribute */
static AttributePtr get_new_port_number_attribute(void);
static void* port_number_get_value(AttributePtr attribute);


ComponentPtr
dag37t_get_new_connection(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentConnection, card); 
    char buffer[BUFFER_SIZE];
    dag37t_connection_state_t* state;
    
    if (NULL != result)
    {
        state = (dag37t_connection_state_t*)malloc(sizeof(dag37t_connection_state_t));
        state->mIndex = index;  // this becomes the connection number
        
        component_set_dispose_routine(result, dag37t_connection_dispose);
        component_set_post_initialize_routine(result, dag37t_connection_post_initialize);
        component_set_reset_routine(result, dag37t_connection_reset);
        component_set_default_routine(result, dag37t_connection_default);
        sprintf(buffer, "connection%d", index);
        component_set_name(result, buffer);
        component_set_description(result, "Connection Component");
        component_set_private_state(result, state);
        return result;
    }
    return result;
}

static void
dag37t_connection_dispose(ComponentPtr component)
{
}

static void
dag37t_connection_reset(ComponentPtr component)
{
}

static void
dag37t_connection_default(ComponentPtr component)
{

    DagCardPtr card = NULL;
    dag37t_connection_state_t * connection_state = NULL;
    card = component_get_card(component);
    connection_state = component_get_private_state(component);
    
}

static int
dag37t_connection_post_initialize(ComponentPtr component)
{
    AttributePtr active;
    AttributePtr direction;
    AttributePtr port;


    /* Create new attributes. */
    active = get_new_active_attribute();
    direction = get_new_direction_attribute();
    port = get_new_port_number_attribute();


    /* Add attributes to the port. */
    component_add_attribute(component, active);
    component_add_attribute(component, direction);
    component_add_attribute(component, port);
    
    return 1;
}

static AttributePtr
get_new_active_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, active_attribute_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "active");
        attribute_set_description(result, "connection active");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}
static void*
active_attribute_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37t_connection_state_t * state = (dag37t_connection_state_t *)component_get_private_state(comp);
    static uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        int dagfd = card_get_fd(card);
        value = is_channel_active(dagfd, state->mIndex);

        return (void*)&value;
    }
    return NULL;
}




static AttributePtr
get_new_direction_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDirection);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, direction_attribute_get_value);
        attribute_set_name(result, "direction");
        attribute_set_description(result, "direction of connection (receive or transmit)");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}



static void*
direction_attribute_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    dag37t_connection_state_t* state = NULL;
    ComponentPtr comp = attribute_get_component(attribute);
    static uint32_t value = 0;
    int dagfd;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        state = (dag37t_connection_state_t *)component_get_private_state(comp);     
        
        dagfd = card_get_fd(card);
        value = get_37t_channel_direction(dagfd, state->mIndex);

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}


static AttributePtr
get_new_port_number_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributePortNumber);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, port_number_get_value);
        attribute_set_name(result, "port_number");
        attribute_set_description(result, "port that connection number is configured on");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
port_number_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    dag37t_connection_state_t* state = NULL;
    ComponentPtr comp = attribute_get_component(attribute);
    uint32_t value = 0;
    int dagfd;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))

    {
        state = (dag37t_connection_state_t *)component_get_private_state(comp);     
        dagfd = card_get_fd(card);
        value = get_37t_channel_port_num(dagfd, state->mIndex);

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}




