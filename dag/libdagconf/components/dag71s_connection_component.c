/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag71s_connection_component.c,v 1.10 2007/05/02 23:50:41 vladimir Exp $
 */

/* *
 * This is the connection component for the DAG 7.1S
*/

/* File Header */

#include "dagutil.h"
#include "dag_platform.h"

#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/dag_component_codes.h"
#include "../include/components/dag71s_connection_component.h"
#include "../include/components/dag71s_e1t1_component.h"

#define BUFFER_SIZE 1024

static void dag71s_connection_dispose(ComponentPtr component);
static void dag71s_connection_reset(ComponentPtr component);
static void dag71s_connection_default(ComponentPtr component);
static int dag71s_connection_post_initialize(ComponentPtr component);


/* Active Attribute */
static AttributePtr get_new_connection_active_attribute(void);
static void* connection_active_attribute_get_value(AttributePtr attribute);

/* hec correction attribute. */
static AttributePtr get_new_hec_correction_attribute(void);
static void* hec_correction_attribute_get_value(AttributePtr attribute);

/* scramble attribute. */
static AttributePtr get_new_scramble_attribute(void);
static void* scramble_attribute_get_value(AttributePtr attribute);

/* idle cell attribute. */
static AttributePtr get_new_idle_cell_attribute(void);
static void* idle_cell_attribute_get_value(AttributePtr attribute);

/* vc number attribute. */
static AttributePtr get_new_vc_number_attribute(void);
static void* vc_number_attribute_get_value(AttributePtr attribute);

/* tu number attribute. */
static AttributePtr get_new_tu_number_attribute(void);
static void* tu_number_attribute_get_value(AttributePtr attribute);

/* tug2 number attribute. */
static AttributePtr get_new_tug2_number_attribute(void);
static void* tug2_number_attribute_get_value(AttributePtr attribute);

/* tug3 number attribute. */
static AttributePtr get_new_tug3_number_attribute(void);
static void* tug3_number_attribute_get_value(AttributePtr attribute);

/* port number attribute */
static AttributePtr get_new_port_number_attribute(void);
static void* port_number_get_value(AttributePtr attribute);

/* payload Attribute */
static AttributePtr get_new_payload_type_attribute(void);
static void* payload_attribute_get_value(AttributePtr attribute);

/* connection type Attribute */
static AttributePtr get_new_connection_type_attribute(void);
static void* connection_type_attribute_get_value(AttributePtr attribute);

/* timeslot pattern Attribute */
static AttributePtr get_new_timeslot_pattern_attribute(void);
static void* timeslot_pattern_attribute_get_value(AttributePtr attribute);


int GetE1T1Type(AttributePtr attribute, int index);


ComponentPtr
dag71s_get_new_connection(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentConnection, card); 
    char buffer[BUFFER_SIZE];
    dag71s_connection_state_t* state;
    
    if (NULL != result)
    {
        state = (dag71s_connection_state_t*)malloc(sizeof(dag71s_connection_state_t));
        state->mIndex = index;  // this becomes the connection number
        state->mIom = card_get_iom_address(card);

        component_set_dispose_routine(result, dag71s_connection_dispose);
        component_set_post_initialize_routine(result, dag71s_connection_post_initialize);
        component_set_reset_routine(result, dag71s_connection_reset);
        component_set_default_routine(result, dag71s_connection_default);
        sprintf(buffer, "connection%d", index);
        component_set_name(result, buffer);
        component_set_description(result, "Connection Component");
        component_set_private_state(result, state);
        return result;
    }
    return result;
}

static void
dag71s_connection_dispose(ComponentPtr component)
{
}

static void
dag71s_connection_reset(ComponentPtr component)
{
}

static void
dag71s_connection_default(ComponentPtr component)
{
   
}

static int
dag71s_connection_post_initialize(ComponentPtr component)
{
    AttributePtr active;
    AttributePtr hec_correction;
    AttributePtr scramble;
    AttributePtr idle_cell;
    AttributePtr vc_number;
    AttributePtr tu_number;
    AttributePtr tug2_number;
    AttributePtr tug3_number;
    AttributePtr port;
    AttributePtr payload_type;
    AttributePtr connection_type;
    AttributePtr timeslot_pattern;
    ComponentPtr parent_component;
    parent_state_t* parent_state;
    dag71s_connection_state_t* state;
    char* name;


    // Get the register address for the mapper/demapper
    parent_component = component_get_parent(component);
    parent_state = (parent_state_t*)component_get_private_state(parent_component);
    state = (dag71s_connection_state_t *)component_get_private_state(component);
    state->mBase = parent_state->mBase;

    name = (char*)component_get_name(parent_component);


    if(strncmp(name,"atm_hdlc_mapper", 12))
        state->mComponentType = kComponentDemapper;
    else
        state->mComponentType = kComponentMapper;



    /* Create new attributes. */
    active = get_new_connection_active_attribute();
    scramble = get_new_scramble_attribute();
    vc_number = get_new_vc_number_attribute();
    tu_number = get_new_tu_number_attribute();
    tug2_number = get_new_tug2_number_attribute();
    tug3_number = get_new_tug3_number_attribute();
    port = get_new_port_number_attribute();
    payload_type = get_new_payload_type_attribute();
    connection_type = get_new_connection_type_attribute();
    timeslot_pattern = get_new_timeslot_pattern_attribute();

    /* Add attributes to the port. */
    component_add_attribute(component, active);
    component_add_attribute(component, scramble);
    component_add_attribute(component, vc_number);
    component_add_attribute(component, tu_number);
    component_add_attribute(component, tug2_number);
    component_add_attribute(component, tug3_number);
    component_add_attribute(component, port);
    component_add_attribute(component, payload_type);
    component_add_attribute(component, connection_type);
    component_add_attribute(component, timeslot_pattern);

    if(state->mComponentType == kComponentMapper)
        return 1;

    //add the demapper attributes
    hec_correction = get_new_hec_correction_attribute();
    idle_cell = get_new_idle_cell_attribute();

    component_add_attribute(component, idle_cell);
    component_add_attribute(component, hec_correction);


    return 1;
}

uint32_t
get_information_register_value(AttributePtr attribute, uint32_t *info)
{
    ComponentPtr comp = attribute_get_component(attribute);
    dag71s_connection_state_t * state = (dag71s_connection_state_t *)component_get_private_state(comp);
    static uint32_t value;
    uint32_t *reg1_val;
    uint32_t *reg2_val;

    if (1 == valid_attribute(attribute))
    {


       	/* Get pointers to the two registers of interest */
    	reg1_val = (uint32_t*)(state->mIom + state->mBase);
	    reg2_val = (uint32_t*)(state->mIom + (state->mBase + 4));

        /* set the connection and port number to read */
        value  = *reg1_val;
        value &= ~0x07FF0000;
        value |= ((uint32_t)state->mIndex << 16);
        *reg1_val = value;

        /* wait for the ready indicator */
        do {
            dagutil_microsleep(1000);	
        } while ( (*reg1_val & 0x10) == 0 );

        /* read back */
        *info = *reg2_val;
        
        return true;
    }
    return false;
}





static AttributePtr
get_new_connection_active_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, connection_active_attribute_get_value);
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
connection_active_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value;
 
    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if((value & 0x07) != 0)
            value = true;
        else
            value = false;
        
        return (void*)&value;
    }
    return NULL;
}


static AttributePtr
get_new_hec_correction_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeHECCorrection);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, hec_correction_attribute_get_value);
        attribute_set_name(result, "HEC_correction");
        attribute_set_description(result, "Enable or disable HEC correction on this connection");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}



static void*
hec_correction_attribute_get_value(AttributePtr attribute)
{
    uint32_t value;
    static uint8_t result = false;
 
    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if(value & 0x200 )
            result = true;
        
        return (void*)&result;
    }
    return NULL;
}

static AttributePtr
get_new_scramble_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeScramble);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, scramble_attribute_get_value);
        attribute_set_name(result, "scramble");
        attribute_set_description(result, "Scrambling enabled or disabled");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
scramble_attribute_get_value(AttributePtr attribute)
{
    uint32_t value;
    static uint8_t result = false;

 
    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if(value & 0x100 )
            result = true; 
        
        return (void*)&result;
    }
    return NULL;
}

static AttributePtr
get_new_idle_cell_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeIdleCellMode);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, idle_cell_attribute_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "idle_cell_mode");
        attribute_set_description(result, "idle cell mode");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}
static void*
idle_cell_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value;
 
    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if(value & 0x400)
            value = true;
        else
            value = false;
        
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_vc_number_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeVCNumber);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, vc_number_attribute_get_value);
        attribute_set_name(result, "VC_Number");
        attribute_set_description(result, "retrieve VC Number for connection");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}



static void*
vc_number_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value = 0;
    uint32_t port;
    ComponentPtr comp = attribute_get_component(attribute);
    dag71s_connection_state_t * state = (dag71s_connection_state_t *)component_get_private_state(comp);

    if(get_information_register_value(attribute, &value))
    {

        /* check the mode (dont return a proper value if the connection is not active) */
        if((value & 0x07) != 0 )
        {
            port = (state->mIndex >> 9); 

            /*work out how many bits are required  by finding out if this is tug-2 or tug-3 */
            if(GetE1T1Type(attribute, port) == 2)
                value = (state->mIndex >> 5) & 0x0F;
            else
                value = (state->mIndex >> 5) & 0x03;

        }
        else
			value = 0;
        
        return (void*)&value;
    }
	value = 0;
    return (void*)&value;
}

static AttributePtr
get_new_tu_number_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTUNumber);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, tu_number_attribute_get_value);
        attribute_set_name(result, "TU_Number");
        attribute_set_description(result, "retrieve TU Number for connection");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}



static void*
tu_number_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value = 0;
    ComponentPtr comp = attribute_get_component(attribute);
    dag71s_connection_state_t * state = (dag71s_connection_state_t *)component_get_private_state(comp);

    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if((value & 0x07) != 0 )
        {
            value = (state->mIndex & 0x03); 

        }
        else
			value = 0;
        
        return (void*)&value;
    }
	value = 0;
    return (void*)&value;
}

static AttributePtr
get_new_tug2_number_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTUG2Number);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, tug2_number_attribute_get_value);
        attribute_set_name(result, "TUG2_Number");
        attribute_set_description(result, "retrieve TUG2 Number for connection");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}



static void*
tug2_number_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value = 0;
    ComponentPtr comp = attribute_get_component(attribute);
    dag71s_connection_state_t * state = (dag71s_connection_state_t *)component_get_private_state(comp);

    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if((value & 0x07) != 0 )
        {
            value = (state->mIndex >> 2) & 0x07; 

        }
        else
			value = 0;
        
        return (void*)&value;
    }
	value = 0;
    return (void*)&value;
}

static AttributePtr
get_new_tug3_number_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTUG3Number);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, tug3_number_attribute_get_value);
        attribute_set_name(result, "TUG3_Number");
        attribute_set_description(result, "retrieve TUG3 Number for connection");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}



static void*
tug3_number_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value = 0;
    uint32_t port;
    ComponentPtr comp = attribute_get_component(attribute);
    dag71s_connection_state_t * state = (dag71s_connection_state_t *)component_get_private_state(comp);

    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if((value & 0x07) != 0 )
        {
            port = (state->mIndex >> 9); 

            /*work out how many bits are required  by finding out if this is tug-2 or tug-3 */
            if(GetE1T1Type(attribute, port) == 2)
                value = 0;
            else
                value = (state->mIndex >> 7) & 0x03;

        }
        else
			value = 0;
        
        return (void*)&value;
    }
	value = 0;
    return (void*)&value;
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
    static uint32_t value = 0;
    ComponentPtr comp = attribute_get_component(attribute);
    dag71s_connection_state_t * state = (dag71s_connection_state_t *)component_get_private_state(comp);

 
    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if(value & 0x07 )
        {
            value = (state->mIndex >> 9); 
        }
        else
			value = 0;
        
        return (void*)&value;
    }
	value = 0;
    return (void*)&value;
}



static AttributePtr
get_new_payload_type_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributePayloadType);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, payload_attribute_get_value);
        attribute_set_name(result, "payload_type");
        attribute_set_description(result, "payload type of this connection (ATM/HDLC/RAW)");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
payload_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value;

 
    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if((value & 0x07) != 0 )
        {
            value = ((value & 0x70) >> 4); 
        }
        else
            value = 0;
        
        return (void*)&value;
    }
	value = 0;
    return (void*)&value;
}


static AttributePtr
get_new_connection_type_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeConnectionType);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, connection_type_attribute_get_value);
        attribute_set_name(result, "connection_type");
        attribute_set_description(result, "Connection type of this connection");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
connection_type_attribute_get_value(AttributePtr attribute)
{
    static uint32_t value;

 
    if(get_information_register_value(attribute, &value))
    {

        /* check the mode */
        if((value & 0x07) == 0 )
            value = 0;
        else
            value &= 0x07;

        return (void*)&value;
    }
	value = 0;
    return (void*)&value;
}


static AttributePtr
get_new_timeslot_pattern_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTimeslotPattern);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, timeslot_pattern_attribute_get_value);
        attribute_set_name(result, "timeslot_pattern");
        attribute_set_description(result, "timeslot pattern of this connection");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
timeslot_pattern_attribute_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    dag71s_connection_state_t * state = (dag71s_connection_state_t *)component_get_private_state(comp);
    static uint32_t value;
    uint32_t *reg1_val;
    uint32_t *reg2_val;

    if (1 == valid_attribute(attribute))
    {


       	/* Get pointers to the two registers of interest */
    	reg1_val = (uint32_t*)(state->mIom + state->mBase);
	    reg2_val = (uint32_t*)(state->mIom + (state->mBase + 8));

        /* set the connection and port number to read */
        value  = *reg1_val;
        value &= ~0x07FF0000;
        value |= ((uint32_t)state->mIndex << 16);
        *reg1_val = value;

        /* wait for the ready indicator */
        do {
            dagutil_microsleep(1000);	
        } while ( (*reg1_val & 0x10) == 0 );

        /* read back */
        value = *reg2_val;
        
        return (void*)&value;
    }

    return NULL;
}


int GetE1T1Type(AttributePtr attribute, int index)
{

    int result;
    uint32_t register_value;
    uint32_t mask;
    DagCardPtr card;
    uintptr_t E1T1Base;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        E1T1Base = card_get_register_address(card, DAG_REG_SONIC_3, index);

        /*work out how many bits are required  by finding out if this is tug2 or tug3 */
        register_value = card_read_iom(card, E1T1Base + kE1T1Config);
        mask = BIT0 | BIT1 | BIT2;
        switch(register_value & mask)
        {
        case 1:         //kLineTypeT1esf
        case 2:         //kLineTypeT1sf;
        case 3:         //kLineTypeT1unframed;
            result = 2;
            break;

        case 0:         //kLineTypeNoPayload
        case 4:         //kLineTypeE1
        case 5:         //kLineTypeE1crc;
        case 6:         //kLineTypeE1unframed;
        default:        //kLineTypeInvalid
            result = 3;
            break;
        }

        return result;
    }
    return 0;
}


