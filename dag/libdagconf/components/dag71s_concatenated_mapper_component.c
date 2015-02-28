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
#include "../include/util/logger.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/dag71s_concatenated_mapper_component.h"

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

/* Bit definitions for register 0 */
#define MAP_PAYLOAD_SCRAMBLING	BIT0	/* RW 0=disable 1=enable */
#define MAP_FCS_INSERTION	BIT1	/* RW 0=disable 1=enable */
#define MAP_FCS_SELECT		BIT2	/* RW 0=FCS16   1=FCS32 */
#define MAP_POS_ATM_SELECT	BIT4	/* RW 0=ATM     1=POS */

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} concatenated_atm_pos_mapper_state_t;

enum
{
    kTimeSlot = 0x8,
    kConnectionConfig = 0x4
};

#define BUFFER_SIZE 1024

/* demapper component. */
static void dag71s_concatenated_atm_pos_mapper_dispose(ComponentPtr component);
static void dag71s_concatenated_atm_pos_mapper_reset(ComponentPtr component);
static void dag71s_concatenated_atm_pos_mapper_default(ComponentPtr component);
static int dag71s_concatenated_atm_pos_mapper_post_initialize(ComponentPtr component);
static dag_err_t dag71s_concatenated_atm_pos_mapper_update_register_base(ComponentPtr component);

static AttributePtr get_new_payload_scrambling(void);
static void payload_scrambling_dispose(AttributePtr attribute);
static void payload_scrambling_post_initialize(AttributePtr attribute);
static void payload_scrambling_set_value(AttributePtr attribute, void* value, int length);
static void* payload_scrambling_get_value(AttributePtr attribute);

static AttributePtr get_new_crc_select(void);
static void crc_select_dispose(AttributePtr attribute);
static void crc_select_post_initialize(AttributePtr attribute);
static void crc_select_set_value(AttributePtr attribute, void* value, int length);
static void* crc_select_get_value(AttributePtr attribute);
static void crc_select_to_string_routine(AttributePtr attribute);
static void crc_select_from_string_routine(AttributePtr attribute, const char* string);

static AttributePtr get_new_network_mode(void);
static void network_mode_dispose(AttributePtr attribute);
static void network_mode_post_initialize(AttributePtr attribute);
static void network_mode_set_value(AttributePtr attribute, void* value, int length);
static void* network_mode_get_value(AttributePtr attribute);
static void network_mode_to_string_routine(AttributePtr attribute);
static void network_mode_from_string_routine(AttributePtr attribute, const char* string);

static void* sonet_type_get_value(AttributePtr attribute);
static void sonet_type_post_initialize(AttributePtr attribute);
static AttributePtr get_new_sonet_type_attribute(void);
static void sonet_type_dispose(AttributePtr attribute);
static void sonet_type_to_string_routine(AttributePtr attribute);


ComponentPtr
concatenated_atm_pos_mapper_get_new_component(DagCardPtr card, uint32_t i)
{
    ComponentPtr result = component_init(kComponentMapper, card); 
    concatenated_atm_pos_mapper_state_t* state = NULL;
    char buffer[BUFFER_SIZE];
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag71s_concatenated_atm_pos_mapper_dispose);
        component_set_post_initialize_routine(result, dag71s_concatenated_atm_pos_mapper_post_initialize);
        component_set_reset_routine(result, dag71s_concatenated_atm_pos_mapper_reset);
        component_set_default_routine(result, dag71s_concatenated_atm_pos_mapper_default);
        component_set_update_register_base_routine(result, dag71s_concatenated_atm_pos_mapper_update_register_base);
        sprintf(buffer, "concatenated_atm_pos_mapper%d", i);
        component_set_name(result, buffer);
        component_set_description(result, "A concatenated ATM/POS Mapper");
        state = (concatenated_atm_pos_mapper_state_t*)malloc(sizeof(concatenated_atm_pos_mapper_state_t));
        state->mIndex = i;
        component_set_private_state(result, state);
    }
    return result;
}

static dag_err_t
dag71s_concatenated_atm_pos_mapper_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card;
        concatenated_atm_pos_mapper_state_t* state;

        card = component_get_card(component);
        NULL_RETURN_WV(card, kDagErrGeneral);
        state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_E1T1_HDLC_MAP, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrGeneral;
}

static void
dag71s_concatenated_atm_pos_mapper_dispose(ComponentPtr component)
{
}

static void
dag71s_concatenated_atm_pos_mapper_reset(ComponentPtr component)
{
}

static void
dag71s_concatenated_atm_pos_mapper_default(ComponentPtr component)
{
}

static int
dag71s_concatenated_atm_pos_mapper_post_initialize(ComponentPtr component)
{
    AttributePtr payload_scrambling;
    AttributePtr crc_select;
    AttributePtr network_mode;
    AttributePtr sonet_type;
    concatenated_atm_pos_mapper_state_t* state = NULL;
    DagCardPtr card;

    state = component_get_private_state(component);
    card = component_get_card(component);
    state->mBase = card_get_register_address(card, DAG_REG_E1T1_HDLC_MAP, state->mIndex);
    assert(state->mBase != 0);
    payload_scrambling = get_new_payload_scrambling();
    crc_select = get_new_crc_select();
    network_mode = get_new_network_mode();
    sonet_type = get_new_sonet_type_attribute();

    component_add_attribute(component, payload_scrambling);
    component_add_attribute(component, crc_select);
    component_add_attribute(component, network_mode);
    component_add_attribute(component, sonet_type);

    return 1;
}

static AttributePtr
get_new_payload_scrambling(void)
{
    AttributePtr result = attribute_init(kBooleanAttributePayloadScramble); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, payload_scrambling_dispose);
        attribute_set_post_initialize_routine(result, payload_scrambling_post_initialize);
        attribute_set_getvalue_routine(result, payload_scrambling_get_value);
        attribute_set_setvalue_routine(result, payload_scrambling_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "payload_scrambling");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
payload_scrambling_dispose(AttributePtr attribute)
{
}

static void*
payload_scrambling_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr mapper;
    uint32_t register_value;
    static uint8_t value;
    concatenated_atm_pos_mapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        mapper= attribute_get_component(attribute);
        state = component_get_private_state(mapper);
        register_value = card_read_iom(card, state->mBase);
        value = (register_value & MAP_PAYLOAD_SCRAMBLING) == MAP_PAYLOAD_SCRAMBLING;
        return (void*)&value;
    }
    return NULL;
}

static void
payload_scrambling_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr mapper;
    uint32_t register_value;
    concatenated_atm_pos_mapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        mapper = attribute_get_component(attribute);
        state = component_get_private_state(mapper);
        register_value = card_read_iom(card, state->mBase);
        if (*(uint8_t*)value == 1)
        {
            register_value |= MAP_PAYLOAD_SCRAMBLING;
        }
        else
        {
            register_value &= ~MAP_PAYLOAD_SCRAMBLING;
        }
        card_write_iom(card, state->mBase, register_value);
    }
}


static void
payload_scrambling_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
get_new_crc_select(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCrcSelect); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, crc_select_dispose);
        attribute_set_post_initialize_routine(result, crc_select_post_initialize);
        attribute_set_getvalue_routine(result, crc_select_get_value);
        attribute_set_setvalue_routine(result, crc_select_set_value);
        attribute_set_to_string_routine(result, crc_select_to_string_routine);
        attribute_set_from_string_routine(result, crc_select_from_string_routine);
        attribute_set_name(result, "crc_select");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
crc_select_dispose(AttributePtr attribute)
{
}

static void*
crc_select_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr mapper;
    uint32_t register_value;
    static crc_t value;
    concatenated_atm_pos_mapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        mapper = attribute_get_component(attribute);
        state = component_get_private_state(mapper);
        register_value = card_read_iom(card, state->mBase);
        if ((register_value & MAP_FCS_INSERTION) == 0)
        {
            value = kCrcOff;
        }
        else if ((register_value & MAP_FCS_SELECT) == 0)
        {
            value = kCrc16;
        }
        else if ((register_value & MAP_FCS_SELECT) == MAP_FCS_SELECT)
        {
            value = kCrc32;
        }

        return (void*)&value;
    }
    return NULL;
}

static void
crc_select_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr mapper;
    uint32_t register_value;
    concatenated_atm_pos_mapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        mapper = attribute_get_component(attribute);
        state = component_get_private_state(mapper);
        register_value = card_read_iom(card, state->mBase);
        
        if (kCrcOff == *(crc_t*)value)
        {
            register_value &= ~MAP_FCS_INSERTION;
        }
        else if (kCrc16 == *(crc_t*)value)
        {
            register_value &= ~MAP_FCS_SELECT;
            register_value |= MAP_FCS_INSERTION;
        }
        else if (kCrc32 == *(crc_t*)value)
        {
            register_value |= MAP_FCS_INSERTION;
            register_value |= MAP_FCS_SELECT;
        }
        card_write_iom(card, state->mBase, register_value);    
    }
}


static void
crc_select_post_initialize(AttributePtr attribute)
{
}

static void
crc_select_to_string_routine(AttributePtr attribute)
{
    crc_t value = *(crc_t*)crc_select_get_value(attribute);
    attribute_set_to_string(attribute, crc_to_string(value));
}

static void
crc_select_from_string_routine(AttributePtr attribute, const char* string)
{
    crc_t value = string_to_crc(string);
    crc_select_set_value(attribute, (void*)&value, sizeof(crc_t));
}

static AttributePtr
get_new_network_mode(void)
{
    AttributePtr result = attribute_init(kUint32AttributeNetworkMode); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, network_mode_dispose);
        attribute_set_post_initialize_routine(result, network_mode_post_initialize);
        attribute_set_getvalue_routine(result, network_mode_get_value);
        attribute_set_setvalue_routine(result, network_mode_set_value);
        attribute_set_to_string_routine(result, network_mode_to_string_routine);
        attribute_set_from_string_routine(result, network_mode_from_string_routine);
        attribute_set_name(result, "network_mode");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
network_mode_dispose(AttributePtr attribute)
{
}

static void*
network_mode_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr mapper;
    uint32_t register_value;
    static network_mode_t value;
    concatenated_atm_pos_mapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        mapper = attribute_get_component(attribute);
        state = component_get_private_state(mapper);
        register_value = card_read_iom(card, state->mBase);
        value = ((register_value & MAP_POS_ATM_SELECT) == MAP_POS_ATM_SELECT) ? kNetworkModePoS:kNetworkModeATM;
        return (void*)&value;
    }
    return NULL;
}

static void
network_mode_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr mapper;
    uint32_t register_value;
    concatenated_atm_pos_mapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        mapper = attribute_get_component(attribute);
        state = component_get_private_state(mapper);
        register_value = card_read_iom(card, state->mBase);
        if (*(network_mode_t*)value == kNetworkModePoS)
        {
            register_value |= MAP_POS_ATM_SELECT;
        }
        else
        {
            register_value &= ~MAP_POS_ATM_SELECT;
        }
        card_write_iom(card, state->mBase, register_value);
    }
}


static void
network_mode_post_initialize(AttributePtr attribute)
{
}

static void
network_mode_to_string_routine(AttributePtr attribute)
{
    network_mode_t mode = *(network_mode_t*)network_mode_get_value(attribute);
    attribute_set_to_string(attribute, network_mode_to_string(mode));
}

static void
network_mode_from_string_routine(AttributePtr attribute, const char* string)
{
    network_mode_t mode = *(network_mode_t*)string_to_network_mode(string);
    network_mode_set_value(attribute, (void*)&mode, mode);
}

static AttributePtr
get_new_sonet_type_attribute(void)
{ 
    AttributePtr result = attribute_init(kUint32AttributeSonetType); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, sonet_type_get_value);
        attribute_set_dispose_routine(result, sonet_type_dispose);
        attribute_set_post_initialize_routine(result, sonet_type_post_initialize);
        attribute_set_to_string_routine(result, sonet_type_to_string_routine);
        attribute_set_name(result, "sonet_type");
        attribute_set_description(result, "Is this component designed for concatenated or channelized SONET traffic.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void*
sonet_type_get_value(AttributePtr attribute)
{
    static uint32_t return_value = kSonetTypeConcatenated;

    if (1 == valid_attribute(attribute))
    {
        return (void*)&return_value;
    }
    return NULL;
}

static void
sonet_type_post_initialize(AttributePtr attribute)
{
}

static void
sonet_type_dispose(AttributePtr attribute)
{
}

static void
sonet_type_to_string_routine(AttributePtr attribute)
{
    sonet_type_t st = *(sonet_type_t*)attribute_get_value(attribute);
    if (st != kSonetTypeInvalid)
    {
      const char* temp;
      temp = sonet_type_to_string(st);
      attribute_set_to_string(attribute, temp);
    }
}

