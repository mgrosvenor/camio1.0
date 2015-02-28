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
#include "../include/components/dag71s_concatenated_demapper_component.h"
#include "../include/util/enum_string_table.h"
#include "../include/util/logger.h"

/* DAG API files we rely on being installed in a sys include path somewhere */
#include <dagutil.h>

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

static void dag71s_concatenated_atm_pos_demapper_dispose(ComponentPtr component);
static void dag71s_concatenated_atm_pos_demapper_reset(ComponentPtr component);
static void dag71s_concatenated_atm_pos_demapper_default(ComponentPtr component);
static int dag71s_concatenated_atm_pos_demapper_post_initialize(ComponentPtr component);
static dag_err_t dag71s_concatenated_atm_pos_demapper_update_register_base(ComponentPtr component);

static void payload_scrambling_dispose(AttributePtr attribute);
static void* payload_scrambling_get_value(AttributePtr attribute);
static void payload_scrambling_post_initialize(AttributePtr attribute);
static void payload_scrambling_set_value(AttributePtr attribute, void* value, int length);
static AttributePtr get_new_payload_scrambling_attribute(void);

static void* pass_idle_get_value(AttributePtr attribute);
static void pass_idle_post_initialize(AttributePtr attribute);
static void pass_idle_set_value(AttributePtr attribute, void* value, int length);
static AttributePtr get_new_pass_idle_attribute(void);
static void pass_idle_dispose(AttributePtr attribute);

static void* crc_select_get_value(AttributePtr attribute);
static void crc_select_post_initialize(AttributePtr attribute);
static void crc_select_set_value(AttributePtr attribute, void* value, int length);
static AttributePtr get_new_crc_select_attribute(void);
static void crc_select_dispose(AttributePtr attribute);
static void crc_select_from_string(AttributePtr attribute, const char* string);
static void crc_select_to_string(AttributePtr attribute);

static void* pos_atm_select_get_value(AttributePtr attribute);
static void pos_atm_select_post_initialize(AttributePtr attribute);
static void pos_atm_select_set_value(AttributePtr attribute, void* value, int length);
static AttributePtr get_new_pos_atm_select_attribute(void);
static void pos_atm_select_dispose(AttributePtr attribute);
static void pos_atm_select_from_string(AttributePtr attribute, const char* string);
static void pos_atm_select_to_string(AttributePtr attribute);

static void* loss_of_cell_delineation_get_value(AttributePtr attribute);
static void loss_of_cell_delineation_post_initialize(AttributePtr attribute);
static AttributePtr get_new_loss_of_cell_delineation_attribute(void);
static void loss_of_cell_delineation_dispose(AttributePtr attribute);

static void* idle_count_get_value(AttributePtr attribute);
static void idle_count_post_initialize(AttributePtr attribute);
static AttributePtr get_new_idle_count_attribute(void);
static void idle_count_dispose(AttributePtr attribute);

static void* hec_count_get_value(AttributePtr attribute);
static void hec_count_post_initialize(AttributePtr attribute);
static AttributePtr get_new_hec_count_attribute(void);
static void hec_count_dispose(AttributePtr attribute);

static void* sonet_type_get_value(AttributePtr attribute);
static void sonet_type_post_initialize(AttributePtr attribute);
static AttributePtr get_new_sonet_type_attribute(void);
static void sonet_type_dispose(AttributePtr attribute);
static void sonet_type_to_string_routine(AttributePtr attribute);

/* Bit definitions for register 0 */
#define DEMAP_PAYLOAD_SCRAMBLING	BIT0	/* RW 0=disable 1=enable */
#define DEMAP_IDLE_CAPTURE		BIT1	/* RW 0=disable 1=enable */
#define DEMAP_FCS_CHECKING		BIT2	/* RW 0=disable 1=enable */
#define DEMAP_FCS_SELECT		BIT3	/* RW 0=FCS16   1=FCS32 */
#define DEMAP_POS_ATM_SELECT		BIT4	/* RW 0=ATM     1=POS */
#define DEMAP_LCD_INDICATOR		BIT8	/* RO 0=Locked  1=Lost Cell Delineation */
#define DEMAP_HEC_CORRECTION		BIT12	/* RW 0=disable 1=enable */	/* XXX not implemented! */
#define DEMAP_LATCH_COUNTERS		BIT31	/* RW 0=no effect 1=latch counters */

typedef struct
{
    int mIndex;
    int mBase;
} dag71s_concatenated_atm_pos_demapper_state_t;

#define BUFFER_SIZE 1024

ComponentPtr
concatenated_atm_pos_demapper_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentDemapper, card); 
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    char buffer[BUFFER_SIZE];

    if (NULL != result)
    {
        component_set_dispose_routine(result, dag71s_concatenated_atm_pos_demapper_dispose);
        component_set_post_initialize_routine(result, dag71s_concatenated_atm_pos_demapper_post_initialize);
        component_set_reset_routine(result, dag71s_concatenated_atm_pos_demapper_reset);
        component_set_default_routine(result, dag71s_concatenated_atm_pos_demapper_default);
        component_set_update_register_base_routine(result, dag71s_concatenated_atm_pos_demapper_update_register_base);
        sprintf(buffer, "concatenated_atm_pos%d", index);
        component_set_name(result, buffer);
        state = (dag71s_concatenated_atm_pos_demapper_state_t*)malloc(sizeof(dag71s_concatenated_atm_pos_demapper_state_t));        
        memset(state, 0, sizeof(dag71s_concatenated_atm_pos_demapper_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;

}

static int
dag71s_concatenated_atm_pos_demapper_post_initialize(ComponentPtr component)
{
    AttributePtr payload_scrambling;
    AttributePtr pass_idle;
    AttributePtr crc_select;
    AttributePtr atm_pos_select;
    AttributePtr loss_of_cell_delineation;
    AttributePtr hec_count;
    AttributePtr idle_cell_count;
    AttributePtr sonet_type;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    DagCardPtr card;

    state = component_get_private_state(component);
    assert(state != NULL);
    card = component_get_card(component);
    assert(card != NULL);
    state->mBase = card_get_register_address(card, DAG_REG_E1T1_HDLC_DEMAP, state->mIndex);    
    
    payload_scrambling = get_new_payload_scrambling_attribute();
    pass_idle = get_new_pass_idle_attribute();
    crc_select = get_new_crc_select_attribute();
    atm_pos_select = get_new_pos_atm_select_attribute();
    loss_of_cell_delineation = get_new_loss_of_cell_delineation_attribute();
    idle_cell_count = get_new_idle_count_attribute();
    hec_count = get_new_hec_count_attribute();
    sonet_type = get_new_sonet_type_attribute();

    component_add_attribute(component, payload_scrambling);
    component_add_attribute(component, pass_idle);
    component_add_attribute(component, atm_pos_select);
    component_add_attribute(component, crc_select);
    component_add_attribute(component, loss_of_cell_delineation);
    component_add_attribute(component, idle_cell_count);
    component_add_attribute(component, hec_count);
    component_add_attribute(component, sonet_type);

    return 1;
}

static void
dag71s_concatenated_atm_pos_demapper_dispose(ComponentPtr component)
{
}

static void
dag71s_concatenated_atm_pos_demapper_reset(ComponentPtr component)
{
}

static void
dag71s_concatenated_atm_pos_demapper_default(ComponentPtr component)
{
}

static dag_err_t
dag71s_concatenated_atm_pos_demapper_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card;
        dag71s_concatenated_atm_pos_demapper_state_t* state;

        card = component_get_card(component);
        NULL_RETURN_WV(card, kDagErrInvalidCardRef);
        state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_E1T1_HDLC_DEMAP, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static AttributePtr
get_new_payload_scrambling_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributePayloadScramble); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, payload_scrambling_dispose);
        attribute_set_setvalue_routine(result, payload_scrambling_set_value);
        attribute_set_getvalue_routine(result, payload_scrambling_get_value);
        attribute_set_post_initialize_routine(result, payload_scrambling_post_initialize);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "payload_scrambling");
        attribute_set_description(result, "Enable/disable the ATM cell or POS frame scrambling.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
payload_scrambling_dispose(AttributePtr attribute)
{
}

static void*
payload_scrambling_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    static uint8_t return_value = 0;
    uint32_t register_value = 0;
    
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        assert(state != NULL);
        register_value = card_read_iom(card, state->mBase);
        return_value = (register_value & DEMAP_PAYLOAD_SCRAMBLING) == DEMAP_PAYLOAD_SCRAMBLING;
    }
    return (void*)&return_value;
}

static void
payload_scrambling_post_initialize(AttributePtr attribute)
{
}

static void
payload_scrambling_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    uint32_t register_value = 0;
    
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        assert(state != NULL);
        register_value = card_read_iom(card, state->mBase);
        if (*(uint8_t*)value == 0)
        {
            register_value &= ~DEMAP_PAYLOAD_SCRAMBLING;
        }
        else
        {
            register_value |= DEMAP_PAYLOAD_SCRAMBLING;
        }
        card_write_iom(card, state->mBase, register_value);
    }
}

static AttributePtr
get_new_crc_select_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCrcSelect); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, crc_select_dispose);
        attribute_set_setvalue_routine(result, crc_select_set_value);
        attribute_set_getvalue_routine(result, crc_select_get_value);
        attribute_set_post_initialize_routine(result, crc_select_post_initialize);
        attribute_set_to_string_routine(result, crc_select_to_string);
        attribute_set_from_string_routine(result, crc_select_from_string);
        attribute_set_name(result, "crc_select");
        attribute_set_description(result, "Select crc16 or crc32 turn off crc");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
crc_select_dispose(AttributePtr attribute)
{
}

static void
crc_select_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        crc_t value = *(crc_t*)crc_select_get_value(attribute);
        const char* temp = crc_to_string(value); 
        if (temp)
            attribute_set_to_string(attribute, temp);
    }
}

static void
crc_select_from_string(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        crc_t value = string_to_crc(string);
        crc_select_set_value(attribute, (void*)&value, sizeof(line_rate_t));
    }
}

static void*
crc_select_get_value(AttributePtr attribute)
{
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    static crc_t return_value;
    uint32_t register_value;
    DagCardPtr card;

    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = attribute_get_card(attribute);
        register_value = card_read_iom(card, state->mBase);
        if ((register_value & DEMAP_FCS_CHECKING) == 0)
        {
            return_value = kCrcOff;
        }
        else if ((register_value & DEMAP_FCS_SELECT) == 0)
        {
            return_value = kCrc16;
        }
        else if ((register_value & DEMAP_FCS_SELECT) == DEMAP_FCS_SELECT)
        {
            return_value = kCrc32;
        }
    }
    return (void*)&return_value;
}

static void
crc_select_post_initialize(AttributePtr attribute)
{
}

static void
crc_select_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    uint32_t register_value;
    DagCardPtr card;

    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = attribute_get_card(attribute);
        register_value = card_read_iom(card, state->mBase);
        card = attribute_get_card(attribute);
        if (kCrcOff == *(crc_t*)value)
        {
            register_value &= ~DEMAP_FCS_CHECKING;
        }
        else if (kCrc16 == *(crc_t*)value)
        {
            register_value &= ~DEMAP_FCS_SELECT;
            register_value |= DEMAP_FCS_CHECKING;
        }
        else if (kCrc32 == *(crc_t*)value)
        {
            register_value |= DEMAP_FCS_CHECKING;
            register_value |= DEMAP_FCS_SELECT;
        }
        card_write_iom(card, state->mBase, register_value);
    }
}

static AttributePtr
get_new_pos_atm_select_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeNetworkMode); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pos_atm_select_dispose);
        attribute_set_setvalue_routine(result, pos_atm_select_set_value);
        attribute_set_getvalue_routine(result, pos_atm_select_get_value);
        attribute_set_post_initialize_routine(result, pos_atm_select_post_initialize);
        attribute_set_to_string_routine(result, pos_atm_select_to_string);
        attribute_set_from_string_routine(result, pos_atm_select_from_string);
        attribute_set_name(result, "network_mode");
        attribute_set_description(result, "Select ATM or POS mode");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
pos_atm_select_dispose(AttributePtr attribute)
{
}

static void*
pos_atm_select_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    static network_mode_t return_value = kNetworkModeATM;
    uint32_t register_value = 0;
    
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = attribute_get_card(attribute);
        assert(state != NULL);
        register_value = card_read_iom(card, state->mBase);
        if ((register_value & DEMAP_POS_ATM_SELECT) == DEMAP_POS_ATM_SELECT)
        {
            return_value = kNetworkModePoS;
        }
        else
        {
            register_value = kNetworkModeATM;
        }
    }
    return (void*)&return_value;
}

static void
pos_atm_select_post_initialize(AttributePtr attribute)
{
}

static void
pos_atm_select_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;
    uint32_t register_value = 0;
    
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = attribute_get_card(attribute);
        assert(state != NULL);
        register_value = card_read_iom(card, state->mBase);
        if (kNetworkModeATM == *(network_mode_t*)value)
        {
            register_value &= ~DEMAP_POS_ATM_SELECT;           
        }
        else if (kNetworkModePoS == *(network_mode_t*)value)
        {
            register_value |= DEMAP_POS_ATM_SELECT;
        }
        card_write_iom(card, state->mBase, register_value);
    }
}


static void
pos_atm_select_from_string(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        network_mode_t mode = string_to_network_mode(string);
        pos_atm_select_set_value(attribute, (void*)&mode, sizeof(mode));
    }
}

static void
pos_atm_select_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        network_mode_t mode = *(network_mode_t*)pos_atm_select_get_value(attribute);
        const char* temp = network_mode_to_string(mode);
        if (temp)
            attribute_set_to_string(attribute, temp);
    }
}

static AttributePtr
get_new_pass_idle_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeIdleCellMode); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pass_idle_dispose);
        attribute_set_setvalue_routine(result, pass_idle_set_value);
        attribute_set_getvalue_routine(result, pass_idle_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_post_initialize_routine(result, pass_idle_post_initialize);
        attribute_set_name(result, "pass_idle");
        attribute_set_description(result, "Pass idle cells.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
pass_idle_dispose(AttributePtr attribute)
{
}

static void*
pass_idle_get_value(AttributePtr attribute)
{
    ComponentPtr component;
    DagCardPtr card;
    static uint8_t return_value;
    uint32_t register_value;
    dag71s_concatenated_atm_pos_demapper_state_t* state;
    
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = attribute_get_card(attribute);
        register_value = card_read_iom(card, state->mBase);
        return_value = (register_value & DEMAP_IDLE_CAPTURE) == DEMAP_IDLE_CAPTURE;
    }
    return (void*)&return_value;
}

static void
pass_idle_post_initialize(AttributePtr attribute)
{
}

static void
pass_idle_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr component;
    DagCardPtr card;
    uint32_t register_value;
    dag71s_concatenated_atm_pos_demapper_state_t* state;
    
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = attribute_get_card(attribute);
        register_value = card_read_iom(card, state->mBase);
        if (*(uint8_t*)value == 0)
        {
            register_value &= ~DEMAP_IDLE_CAPTURE;
        }
        else
        {
            register_value |= DEMAP_IDLE_CAPTURE;
        }        
        card_write_iom(card, state->mBase, register_value);
    }
}

static void*
loss_of_cell_delineation_get_value(AttributePtr attribute)
{
    ComponentPtr component;
    DagCardPtr card;
    static uint8_t return_value;
    uint32_t register_value;
    dag71s_concatenated_atm_pos_demapper_state_t* state;
    
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = attribute_get_card(attribute);
        register_value = card_read_iom(card, state->mBase);
        return_value = (register_value & DEMAP_LCD_INDICATOR) == DEMAP_LCD_INDICATOR;
    }
    return (void*)&return_value;
}

static void
loss_of_cell_delineation_post_initialize(AttributePtr attribute)
{
}


static AttributePtr
get_new_loss_of_cell_delineation_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLossOfCellDelineation); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, loss_of_cell_delineation_get_value);
        attribute_set_dispose_routine(result, loss_of_cell_delineation_dispose);
        attribute_set_post_initialize_routine(result, loss_of_cell_delineation_post_initialize);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "loss_of_cell_delineation");
        attribute_set_description(result, "Is the demapper experiencing loss of cell delineation?");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
loss_of_cell_delineation_dispose(AttributePtr attribute)
{

}

static void*
idle_count_get_value(AttributePtr attribute)
{
    static uint32_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        register_value = card_read_iom(card, state->mBase);
        register_value |= DEMAP_LATCH_COUNTERS;
        /* latch the stats counter */
        register_value = card_read_iom(card, state->mBase + 0x04);
        return_value = register_value & 0x3fffffff;
    }
    return (void*)&return_value;
}

static void
idle_count_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
get_new_idle_count_attribute(void)
{ 
    AttributePtr result = attribute_init(kUint32AttributeIdleCellCount); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, idle_count_get_value);
        attribute_set_dispose_routine(result, idle_count_dispose);
        attribute_set_post_initialize_routine(result, idle_count_post_initialize);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "idle_count");
        attribute_set_description(result, "Count of the number of idle cells.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;

}

static void
idle_count_dispose(AttributePtr attribute)
{
}

static AttributePtr
get_new_hec_count_attribute(void)
{ 
    AttributePtr result = attribute_init(kUint32AttributeHECCount); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, hec_count_get_value);
        attribute_set_dispose_routine(result, hec_count_dispose);
        attribute_set_post_initialize_routine(result, hec_count_post_initialize);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "hec_count");
        attribute_set_description(result, "Count of the number of idle cells.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void*
hec_count_get_value(AttributePtr attribute)
{
    static uint32_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag71s_concatenated_atm_pos_demapper_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        assert(state != NULL);
        register_value = card_read_iom(card, state->mBase);
        register_value |= DEMAP_LATCH_COUNTERS;
        /* latch the stats counter */
        card_write_iom(card, state->mBase, register_value);
        register_value = card_read_iom(card, state->mBase + 0x08);
        return_value = register_value & 0x3fffffff;
    }
    return (void*)&return_value;
}

static void
hec_count_post_initialize(AttributePtr attribute)
{
}

static void
hec_count_dispose(AttributePtr attribute)
{
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

