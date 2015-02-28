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
#include "../include/components/dag71s_e1t1_component.h"
#include "../include/util/enum_string_table.h"

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
    uint32_t mBase;
    uint32_t mIndex;
} e1t1_state_t;

/* e1t1 component. */
static void dag71s_e1t1_dispose(ComponentPtr component);
static void dag71s_e1t1_reset(ComponentPtr component);
static void dag71s_e1t1_default(ComponentPtr component);
static int dag71s_e1t1_post_initialize(ComponentPtr component);
static dag_err_t dag71s_e1t1_update_register_base(ComponentPtr component);

static AttributePtr dag71s_get_new_e1t1_line_type_attribute(void);
static void dag71s_e1t1_line_type_dispose(AttributePtr attribute);
static void dag71s_e1t1_line_type_set_value(AttributePtr attribute, void* value, int length);
static void* dag71s_e1t1_line_type_get_value(AttributePtr attribute);
static void dag71s_e1t1_line_type_to_string(AttributePtr attribute);
static void dag71s_e1t1_line_type_from_string(AttributePtr attribute, const char* string);

static AttributePtr dag71s_get_new_e1t1_stream_number(void);
static void dag71s_e1t1_stream_number_dispose(AttributePtr attribute);
static void* dag71s_e1t1_stream_number_get_value(AttributePtr attribute);
static void dag71s_e1t1_stream_number_post_initialize(AttributePtr attribute);
static void dag71s_e1t1_stream_number_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag71s_get_new_e1t1_generate_alarm_indication(void);
static void dag71s_e1t1_generate_alarm_indication_dispose(AttributePtr attribute);
static void* dag71s_e1t1_generate_alarm_indication_get_value(AttributePtr attribute);
static void dag71s_e1t1_generate_alarm_indication_post_initialize(AttributePtr attribute);
static void dag71s_e1t1_generate_alarm_indication_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag71s_get_new_e1t1_link_synchronized(void);
static void dag71s_e1t1_link_synchronized_dispose(AttributePtr attribute);
static void* dag71s_e1t1_link_synchronized_get_value(AttributePtr attribute);
static void dag71s_e1t1_link_synchronized_post_initialize(AttributePtr attribute);

static AttributePtr dag71s_get_new_e1t1_link_ais(void);
static void dag71s_e1t1_link_ais_dispose(AttributePtr attribute);
static void* dag71s_e1t1_link_ais_get_value(AttributePtr attribute);
static void dag71s_e1t1_link_ais_post_initialize(AttributePtr attribute);

static AttributePtr dag71s_get_new_e1t1_link_synchronized_up(void);
static void dag71s_e1t1_link_synchronized_up_dispose(AttributePtr attribute);
static void* dag71s_e1t1_link_synchronized_up_get_value(AttributePtr attribute);
static void dag71s_e1t1_link_synchronized_up_post_initialize(AttributePtr attribute);

static AttributePtr dag71s_get_new_e1t1_link_synchronized_down(void);
static void dag71s_e1t1_link_synchronized_down_dispose(AttributePtr attribute);
static void* dag71s_e1t1_link_synchronized_down_get_value(AttributePtr attribute);
static void dag71s_e1t1_link_synchronized_down_post_initialize(AttributePtr attribute);

static AttributePtr dag71s_get_new_e1t1_link_framing_error(void);
static void dag71s_e1t1_link_framing_error_dispose(AttributePtr attribute);
static void* dag71s_e1t1_link_framing_error_get_value(AttributePtr attribute);
static void dag71s_e1t1_link_framing_error_post_initialize(AttributePtr attribute);

static AttributePtr dag71s_get_new_e1t1_link_crc_error(void);
static void dag71s_e1t1_link_crc_error_dispose(AttributePtr attribute);
static void* dag71s_e1t1_link_crc_error_get_value(AttributePtr attribute);
static void dag71s_e1t1_link_crc_error_post_initialize(AttributePtr attribute);

#define BUFFER_SIZE 1024
#define MAX_READ_RETRIES 10

ComponentPtr
dag71s_get_new_e1t1(DagCardPtr card, int i)
{
    ComponentPtr result = component_init(kComponentE1T1, card); 
    e1t1_state_t* state = NULL;
    char buffer[BUFFER_SIZE];
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag71s_e1t1_dispose);
        component_set_post_initialize_routine(result, dag71s_e1t1_post_initialize);
        component_set_reset_routine(result, dag71s_e1t1_reset);
        component_set_default_routine(result, dag71s_e1t1_default);
        component_set_update_register_base_routine(result, dag71s_e1t1_update_register_base);
        sprintf(buffer, "e1t1%d", i);
        component_set_name(result, buffer);
        state = (e1t1_state_t*)malloc(sizeof(e1t1_state_t));
        state->mIndex = i;;
        component_set_private_state(result, state);
    }
    
    return result;

}

static void
dag71s_e1t1_dispose(ComponentPtr component)
{
}

static void
dag71s_e1t1_reset(ComponentPtr component)
{
}

static void
dag71s_e1t1_default(ComponentPtr component)
{
}

static dag_err_t
dag71s_e1t1_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = component_get_card(component);
        e1t1_state_t* state = component_get_private_state(component);
        NULL_RETURN_WV(card, kDagErrInvalidCardRef);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_SONIC_3, state->mIndex);              
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static int
dag71s_e1t1_post_initialize(ComponentPtr component)
{
    AttributePtr line_type;
    AttributePtr stream_number;
    AttributePtr generate_ais;
    AttributePtr link_sync;
    AttributePtr link_sync_up;
    AttributePtr link_sync_down;
    AttributePtr ais;
    AttributePtr crc;
    AttributePtr framing;
    e1t1_state_t* state;
    DagCardPtr card;

    state				= component_get_private_state(component);
    card				= component_get_card(component);
    state->mBase		= card_get_register_address(card, DAG_REG_SONIC_3, state->mIndex);
    line_type			= dag71s_get_new_e1t1_line_type_attribute();
    stream_number		= dag71s_get_new_e1t1_stream_number();
    generate_ais		= dag71s_get_new_e1t1_generate_alarm_indication();
    link_sync			= dag71s_get_new_e1t1_link_synchronized();
    ais					= dag71s_get_new_e1t1_link_ais();
    link_sync_up		= dag71s_get_new_e1t1_link_synchronized_up();
    link_sync_down		= dag71s_get_new_e1t1_link_synchronized_down();
    crc					= dag71s_get_new_e1t1_link_crc_error();
    framing				= dag71s_get_new_e1t1_link_framing_error();

    component_add_attribute(component, line_type);
    component_add_attribute(component, stream_number);
    component_add_attribute(component, generate_ais);
    component_add_attribute(component, link_sync);
    component_add_attribute(component, ais);
    component_add_attribute(component, link_sync_up);
    component_add_attribute(component, link_sync_down);
    component_add_attribute(component, framing);
    component_add_attribute(component, crc);

    return 1;
}

static AttributePtr
dag71s_get_new_e1t1_line_type_attribute(void)
{ 
    AttributePtr result = attribute_init(kUint32AttributeLineType); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_line_type_dispose);
        attribute_set_setvalue_routine(result, dag71s_e1t1_line_type_set_value);
        attribute_set_getvalue_routine(result, dag71s_e1t1_line_type_get_value);
        attribute_set_name(result, "e1t1_line_type");
        attribute_set_description(result, "The E1/T1 line type.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_from_string_routine(result, dag71s_e1t1_line_type_from_string);
        attribute_set_to_string_routine(result, dag71s_e1t1_line_type_to_string);
    }
    
    return result;
}

static void
dag71s_e1t1_line_type_to_string(AttributePtr attribute)
{
    const char* temp = NULL;
    line_type_t line_type = *(line_type_t*)dag71s_e1t1_line_type_get_value(attribute);
    temp = line_type_to_string(line_type);
    attribute_set_to_string(attribute, temp);
}

static void
dag71s_e1t1_line_type_from_string(AttributePtr attribute, const char* string)
{
    line_type_t line_type = string_to_line_type(string);
    if (line_type != kLineTypeInvalid)
        dag71s_e1t1_line_type_set_value(attribute, (void*)&line_type, sizeof(line_type));
}

static void
dag71s_e1t1_line_type_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_line_type_get_value(AttributePtr attribute)
{
    ComponentPtr e1t1;
    DagCardPtr card;
    uint32_t register_value;
    static line_type_t return_value = kLineTypeInvalid;
    e1t1_state_t* state = NULL;
    uint32_t mask = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        e1t1 = attribute_get_component(attribute);
        state = component_get_private_state(e1t1);
        register_value = card_read_iom(card, state->mBase + kE1T1Config);
        mask = BIT0 | BIT1 | BIT2;
        switch(register_value & mask)
        {
            case 0:
                return_value = kLineTypeNoPayload;
                break;
            case 1:
                return_value = kLineTypeT1esf;
                break;
            case 2:
                return_value = kLineTypeT1sf;
                break;
            case 3:
                return_value = kLineTypeT1unframed;
                break;
            case 4:
                return_value = kLineTypeE1;
                break;
            case 5:
                return_value = kLineTypeE1crc;
                break;
            case 6:
                return_value = kLineTypeE1unframed;
                break;
            default:
                return_value = kLineTypeInvalid;
                break;
        }
    }
    return (void*)&return_value;
}


static void
dag71s_e1t1_line_type_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr e1t1;
    DagCardPtr card;
    uint32_t register_value;
    line_type_t e1t1_line_type = *(line_type_t*)value;
    e1t1_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        e1t1 = attribute_get_component(attribute);
        state = component_get_private_state(e1t1);
        register_value = card_read_iom(card, state->mBase + kE1T1Config);
        register_value = 0;
        //register_value &= ~(BIT0 | BIT1 | BIT2);
        switch (e1t1_line_type)
        {
            case kLineTypeInvalid:
            case kLineTypeNoPayload:
            break;

            case kLineTypeT1esf:
            case kLineTypeT1:
            register_value |= (BIT31|BIT0);
            break;

            case kLineTypeT1sf:
            register_value |= (BIT31|BIT1);
            break;

            case kLineTypeT1unframed:
            register_value |= (BIT31 | BIT1 | BIT0);
            break;

            case kLineTypeE1:
            register_value |= (BIT31|BIT2);
            break;

            case kLineTypeE1crc:
            register_value |= (BIT31 | BIT2 | BIT0);
            break;

            case kLineTypeE1unframed:
            register_value |= (BIT31 | BIT2 | BIT1 );
            break;

        }
        card_write_iom(card, state->mBase + kE1T1Config, register_value);
        //set global configuration - use this configuration for all lines
        register_value  = card_read_iom(card, state->mBase + kE1T1Status);
        //resync to data stream - have to write 1 then 0 then 1 then 0 to the register cos that's what the register map document says
        register_value = card_read_iom(card, state->mBase + kE1T1Config);
        register_value |= BIT4;
        card_write_iom(card, state->mBase + kE1T1Config, register_value);
        register_value &= ~BIT4;
        card_write_iom(card, state->mBase + kE1T1Config, register_value);
        register_value |= BIT4;
        card_write_iom(card, state->mBase + kE1T1Config, register_value);
        register_value &= ~BIT4;
        card_write_iom(card, state->mBase + kE1T1Config, register_value);
        //set the load data bit
        register_value |= BIT31;
        card_write_iom(card, state->mBase + kE1T1Config, register_value);
    }
}

static AttributePtr
dag71s_get_new_e1t1_stream_number(void)
{ 
    AttributePtr result = attribute_init(kUint32AttributeE1T1StreamNumber); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_stream_number_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_stream_number_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_stream_number_get_value);
        attribute_set_setvalue_routine(result, dag71s_e1t1_stream_number_set_value);
        attribute_set_name(result, "e1t1_stream_number");
        attribute_set_description(result, "received synchronization status message");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
    }
    
    return result;
}

static void
dag71s_e1t1_stream_number_dispose(AttributePtr attribute)
{
}

static void
dag71s_e1t1_stream_number_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    e1t1_state_t* e1t1_state = NULL;
    uint32_t val = *(uint32_t*)value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1Status);
        register_value &= ~0x1ff0;
        register_value |= val << 4;
        card_write_iom(card, e1t1_state->mBase + kE1T1Status, register_value);
    }
}

static void*
dag71s_e1t1_stream_number_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    static uint32_t e1t1_stream_number;
    e1t1_state_t* e1t1_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1Status);
        e1t1_stream_number = (register_value & 0x1ff0) >> 4;
        return (void*)&e1t1_stream_number;
    }
    return NULL;
}

static void
dag71s_e1t1_stream_number_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag71s_get_new_e1t1_generate_alarm_indication(void)
{ 
    AttributePtr result = attribute_init(kBooleanAttributeE1T1GenerateAlarmIndication); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_generate_alarm_indication_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_generate_alarm_indication_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_generate_alarm_indication_get_value);
        attribute_set_setvalue_routine(result, dag71s_e1t1_generate_alarm_indication_set_value);
        attribute_set_name(result, "e1t1_generate_alarm_indication");
        attribute_set_description(result, "Tells the E1/T1 framer to generate alarm signal indication.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
dag71s_e1t1_generate_alarm_indication_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_generate_alarm_indication_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    static uint8_t e1t1_generate_alarm_indication;
    e1t1_state_t* e1t1_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1Config);
        e1t1_generate_alarm_indication = ((BIT5 & register_value) == BIT5);
        return (void*)&e1t1_generate_alarm_indication;
    }
    return NULL;
}

static void
dag71s_e1t1_generate_alarm_indication_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    uint8_t val = *(uint8_t*)value;
    e1t1_state_t* e1t1_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1Config);
        register_value &= ~BIT5;
        register_value |= val << 5;
        card_write_iom(card, e1t1_state->mBase + kE1T1Config, register_value);
    }
}

static void
dag71s_e1t1_generate_alarm_indication_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag71s_get_new_e1t1_link_synchronized(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1LinkSynchronized); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_link_synchronized_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_link_synchronized_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_link_synchronized_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "e1t1_link_synchronized");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag71s_e1t1_link_synchronized_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_link_synchronized_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    e1t1_state_t* e1t1_state = NULL;
    static uint8_t value;
	int num_retries = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
		while (((register_value & BIT31) != BIT31) && (num_retries < MAX_READ_RETRIES))
		{
			register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
			num_retries++;
		}
		if (num_retries == MAX_READ_RETRIES)
			return NULL;
        value = (register_value & BIT0) == BIT0;
        return &value;
    }
    return NULL;
}

static void
dag71s_e1t1_link_synchronized_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag71s_get_new_e1t1_link_ais(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1LinkAIS); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_link_ais_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_link_ais_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_link_ais_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "e1t1_link_ais");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag71s_e1t1_link_ais_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_link_ais_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    e1t1_state_t* e1t1_state = NULL;
    static uint8_t value;
	int num_retries = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
		while (((register_value & BIT31) != BIT31) && (num_retries < MAX_READ_RETRIES))
		{
			register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
			num_retries++;
		}
		if (num_retries == MAX_READ_RETRIES)
			return NULL;
        value = ((register_value & BIT1) == BIT1);
        return &value;
    }
    return NULL;
}

static void
dag71s_e1t1_link_ais_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag71s_get_new_e1t1_link_synchronized_up(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1LinkSynchronizedUp); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_link_synchronized_up_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_link_synchronized_up_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_link_synchronized_up_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "e1t1_link_synchronized_up");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag71s_e1t1_link_synchronized_up_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_link_synchronized_up_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    e1t1_state_t* e1t1_state = NULL;
    static uint8_t value;
	int num_retries = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
		while (((register_value & BIT31) != BIT31) && (num_retries < MAX_READ_RETRIES))
		{
			register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
			num_retries++;
		}
		if (num_retries == MAX_READ_RETRIES)
			return NULL;
        value = ((register_value & BIT2) == BIT2);
        return &value;
    }
    return NULL;
}

static void
dag71s_e1t1_link_synchronized_up_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag71s_get_new_e1t1_link_synchronized_down(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1LinkSynchronizedDown); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_link_synchronized_down_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_link_synchronized_down_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_link_synchronized_down_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "e1t1_link_synchronized_down");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag71s_e1t1_link_synchronized_down_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_link_synchronized_down_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    e1t1_state_t* e1t1_state = NULL;
    static uint8_t value;
	int num_retries = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
	while (((register_value & BIT31) != BIT31) && (num_retries < MAX_READ_RETRIES))
		{
			register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
			num_retries++;
		}
		if (num_retries == MAX_READ_RETRIES)
			return NULL;
        value = ((register_value & BIT3) == BIT3);
        return &value;
    }
    return NULL;
}

static void
dag71s_e1t1_link_synchronized_down_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag71s_get_new_e1t1_link_framing_error(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1LinkFramingError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_link_framing_error_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_link_framing_error_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_link_framing_error_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "e1t1_link_framing_error");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag71s_e1t1_link_framing_error_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_link_framing_error_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    e1t1_state_t* e1t1_state = NULL;
    static uint8_t value;
	int num_retries = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
		while (((register_value & BIT31) != BIT31) && (num_retries < MAX_READ_RETRIES))
		{
			register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
			num_retries++;
		}
		if (num_retries == MAX_READ_RETRIES)
			return NULL;
        value = ((register_value & BIT4) == BIT4);
        return &value;
    }
    return NULL;
}

static void
dag71s_e1t1_link_framing_error_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag71s_get_new_e1t1_link_crc_error(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1LinkCRCError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_e1t1_link_crc_error_dispose);
        attribute_set_post_initialize_routine(result, dag71s_e1t1_link_crc_error_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_e1t1_link_crc_error_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "e1t1_link_crc_error");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag71s_e1t1_link_crc_error_dispose(AttributePtr attribute)
{
}

static void*
dag71s_e1t1_link_crc_error_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    e1t1_state_t* e1t1_state = NULL;
    static uint8_t value;
	int num_retries = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        e1t1_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
		while (((register_value & BIT31) != BIT31) && (num_retries < MAX_READ_RETRIES))
		{
			register_value = card_read_iom(card, e1t1_state->mBase + kE1T1LinkStatus);
			num_retries++;
		}
		if (num_retries == MAX_READ_RETRIES)
			return NULL;
        value = ((register_value & BIT5) == BIT5);
        return &value;
    }
    return NULL;
}

static void
dag71s_e1t1_link_crc_error_post_initialize(AttributePtr attribute)
{
}

