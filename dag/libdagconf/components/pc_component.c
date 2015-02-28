/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: pc_component.c,v 1.2 2006/04/13 00:35:41 koryn Exp $
 */

/* 
   This module is for with the packet capture module on the copro.
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
    PC_CONFIG = 0x00,
    PC_THRESHOLD = 0x04,
    FLOW_THRESHOLD = 0x8,
    HASH_TABLE_CONFIG = 0xc,
    FLOW_CAP_CONFIG = 0x10
} pc_register_offsets_t;
    
typedef struct
{
    uint32_t mPCBase;
    int mIndex;
} pc_state_t;

static void pc_dispose(ComponentPtr component);
static void pc_reset(ComponentPtr component);
static void pc_default(ComponentPtr component);
static int pc_post_initialize(ComponentPtr component);

ComponentPtr
pc_get_new_component(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentPacketCapture, card); 
    
    if (NULL != result)
    {
        pc_state_t* state = (pc_state_t*)malloc(sizeof(pc_state_t));
        
        component_set_dispose_routine(result, pc_dispose);
        component_set_post_initialize_routine(result, pc_post_initialize);
        component_set_reset_routine(result, pc_reset);
        component_set_default_routine(result, pc_default);
        component_set_name(result, "pc");
        component_set_description(result, "Packet Capture Module.");
        component_set_private_state(result, state);
        return result;
    }
    return result;
}

static void
pc_dispose(ComponentPtr component)
{
}

static void
pc_reset(ComponentPtr component)
{
}

static void
pc_default(ComponentPtr component)
{
}

static int
pc_post_initialize(ComponentPtr component)
{
    AttributePtr attr = NULL;
    DagCardPtr card = component_get_card(component);
    uintptr_t base_addr;
    uint32_t bit_mask;
    GenericReadWritePtr grw = NULL;
    pc_state_t* state = component_get_private_state(component);
    state->mPCBase = card_get_register_address(card, DAG_REG_RANDOM_CAPTURE, 0);

    base_addr = ((uintptr_t)card_get_iom_address(card)) + state->mPCBase;
    bit_mask = BIT8;
    grw = grw_init(card, base_addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnablePacketCaptureModules, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT12;
    grw = grw_init(card, base_addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnableProbabilitySampler, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT13;
    grw = grw_init(card, base_addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnableProbabilityHashTable, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT14;
    grw = grw_init(card, base_addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnableHostHashTable, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT15;
    grw = grw_init(card, base_addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEnableHostFlowTable, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = 0x3ffff;
    grw = grw_init(card, base_addr + HASH_TABLE_CONFIG, NULL, grw_set_capture_hash);
    attr = attribute_factory_make_attribute(kUint32AttributeSetCaptureHash, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = 0x3ffff;
    grw = grw_init(card, base_addr + HASH_TABLE_CONFIG, NULL, grw_unset_capture_hash);
    attr = attribute_factory_make_attribute(kUint32AttributeUnsetCaptureHash, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT24;
    grw = grw_init(card, base_addr + HASH_TABLE_CONFIG, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeResetHashTable, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT0;
    grw = grw_init(card, base_addr + FLOW_CAP_CONFIG, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeResetProbabilityHashTable, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT28;
    grw = grw_init(card, base_addr + HASH_TABLE_CONFIG, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeHashTableReady, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = 0xffffffff;
    grw = grw_init(card, base_addr + PC_THRESHOLD, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributePacketCaptureThreshold, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = 0xffffffff;
    grw = grw_init(card, base_addr + FLOW_THRESHOLD, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeFlowCaptureThreshold, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    bit_mask = BIT9;
    grw = grw_init(card, base_addr, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeTCAMInit, grw, &bit_mask, 1);
    component_add_attribute(component, attr);

    return 1;
}


