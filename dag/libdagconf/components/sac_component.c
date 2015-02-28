/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* 
   This module is for communicating with the Single Attribute Counter Module
*/

#include "dagutil.h"
#include "dag_platform.h"
#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/attribute_factory.h"

typedef enum
{
    SAC_CONFIG = 0x00,
    SAC_IP_COUNTER_CONTROL = 0x04,
    SAC_TCP_FLOW_COUNTER_CONTROL = 0x08,
    SAC_TCP_FLOW_UNIQUE_COUNT_REGISTER = 0x0C
} sac_config_t;
    
typedef struct
{
    uint32_t mSacBase;
    int mIndex;
} sac_state_t;

static void sac_dispose(ComponentPtr component);
static void sac_reset(ComponentPtr component);
static void sac_default(ComponentPtr component);
static int sac_post_initialize(ComponentPtr component);

/* Attributes */


/* TcpFlow Counter */
static AttributePtr get_new_tcpflows(void);
static void* tcpflows_get_value(AttributePtr attribute);

/* IP Address Counter */
static AttributePtr get_new_ipadd(void);
static void* ipadd_get_value(AttributePtr attribute);


ComponentPtr
sac_get_new_component(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentSingleAttributeCounter, card); 
    
    if (NULL != result)
    {
        sac_state_t* state = (sac_state_t*)malloc(sizeof(sac_state_t));
        
        component_set_dispose_routine(result, sac_dispose);
        component_set_post_initialize_routine(result, sac_post_initialize);
        component_set_reset_routine(result, sac_reset);
        component_set_default_routine(result, sac_default);
        component_set_name(result, "sac");
        component_set_description(result, "Single Attribute Counter Module.");
        component_set_private_state(result, state);
        return result;
    }
    return result;
}

static void
sac_dispose(ComponentPtr component)
{
}

static void
sac_reset(ComponentPtr component)
{
}

static void
sac_default(ComponentPtr component)
{
}

static int
sac_post_initialize(ComponentPtr component)
{
    AttributePtr attr = NULL;
    DagCardPtr card = component_get_card(component);
    uintptr_t addr;
    uint32_t bit_mask;
    GenericReadWritePtr grw = NULL;
    sac_state_t* state = component_get_private_state(component);
    state->mSacBase = card_get_register_address(card, DAG_REG_STATS, 0);

    attr = get_new_tcpflows();
    component_add_attribute(component, attr);

    attr = get_new_ipadd();
    component_add_attribute(component, attr);

    addr = ((uintptr_t)card_get_iom_address(card)) + state->mSacBase;
    bit_mask = BIT8;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnableCounterModules, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT9;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeTCAMInit, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    addr = ((uintptr_t)card_get_iom_address(card)) + state->mSacBase + SAC_IP_COUNTER_CONTROL;
    bit_mask = BIT1;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeGenerateIPCounterErf, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT0;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeResetIPCounter, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT4;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeReadyEnableIPCounterModule, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT2;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnableIPAddressCounter, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    addr = ((uintptr_t)card_get_iom_address(card)) + state->mSacBase + SAC_TCP_FLOW_COUNTER_CONTROL;
    bit_mask = BIT1;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeGenerateTCPFlowCounterErf, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT4;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeReadyEnableTCPFlowModule, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT0;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeResetTCPFlowCounter, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT2;
    grw = grw_init(card, addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnableTCPFlowCounter, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    return 1;
}

static AttributePtr
get_new_tcpflows(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTcpFlowCounter);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, tcpflows_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "tcpflows");
        attribute_set_description(result, "TCP Flow Counter");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
tcpflows_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    sac_state_t* state = component_get_private_state(comp);
    uint32_t register_value;
    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, state->mSacBase + SAC_TCP_FLOW_UNIQUE_COUNT_REGISTER) & 0x7FFFFF;
        attribute_set_value_array(attribute, &register_value, sizeof(register_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_ipadd(void)
{
    AttributePtr result = attribute_init(kUint32AttributeIpAddressCounter);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, ipadd_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "ipadd");
        attribute_set_description(result, "IP Address Counter");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
ipadd_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    sac_state_t* state = component_get_private_state(comp);
    uint32_t register_value;
    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, state->mSacBase + SAC_IP_COUNTER_CONTROL) >> 16;
        register_value &= 0xFFFF;
        attribute_set_value_array(attribute, &register_value, sizeof(register_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

