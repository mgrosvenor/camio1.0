/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

/* 
Component interface to the MUX on the DAG3.7T cards
*/


#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/attribute_factory.h"
#include "../include/components/dag37t_mux_component.h"
#include "../include/modules/generic_read_write.h"
#include "../include/util/enum_string_table.h"
#include "../include/cards/dag3_constants.h"

/* dag api headers */
#include "dag_platform.h"
#include "dagreg.h"
#include "dagutil.h"



#define ERFMUX_REG_ADDR         0x400

typedef struct
{
	uint32_t mErfMuxBase;
} d37t_erfmux_state_t;



static void steering_mode_to_string(AttributePtr attribute);

static void dag37t_mux_dispose(ComponentPtr component);
static void dag37t_mux_reset(ComponentPtr component);
static void dag37t_mux_default(ComponentPtr component);
static int dag37t_mux_post_initialize(ComponentPtr component);
static dag_err_t dag37t_mux_update_register_base(ComponentPtr component);

static AttributePtr get_new_line_steering_mode(void);
static void* line_steering_mode_get_value(AttributePtr attribute);
static void line_steering_mode_set_value(AttributePtr attribute, void* value, int length);
static void line_steering_mode_from_string(AttributePtr attribute, const char* string);

static AttributePtr get_new_host_steering_mode(void);
static void* host_steering_mode_get_value(AttributePtr attribute);
static void host_steering_mode_set_value(AttributePtr attribute, void* value, int length);
static void host_steering_mode_from_string(AttributePtr attribute, const char* string);

static AttributePtr get_new_xscale_steering_mode(void);
static void* xscale_steering_mode_get_value(AttributePtr attribute);
static void xscale_steering_mode_set_value(AttributePtr attribute, void* value, int length);
static void xscale_steering_mode_from_string(AttributePtr attribute, const char* string);


ComponentPtr
dag37t_mux_get_new_component(DagCardPtr card)
{
	ComponentPtr result = component_init(kComponentErfMux, card); 
	d37t_erfmux_state_t* state = NULL;
	
	if (NULL != result)
	{
		component_set_dispose_routine(result, dag37t_mux_dispose);
		component_set_post_initialize_routine(result, dag37t_mux_post_initialize);
		component_set_reset_routine(result, dag37t_mux_reset);
		component_set_default_routine(result, dag37t_mux_default);
		component_set_name(result, "erfmux");
		component_set_update_register_base_routine(result, dag37t_mux_update_register_base);
		component_set_description(result, "The Erf MUX on D3.7T cards");
		state = (d37t_erfmux_state_t*)malloc(sizeof(d37t_erfmux_state_t));
		component_set_private_state(result, state);
	}
	return result;
}


static void
dag37t_mux_dispose(ComponentPtr component)
{
}

static void
dag37t_mux_reset(ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
	d37t_erfmux_state_t* state = component_get_private_state(component);
	card_write_iom(card, state->mErfMuxBase + MUX_RX_HOST, 0x00000001);
	card_write_iom(card, state->mErfMuxBase + MUX_RX_PHY, 0x00000000);
	card_write_iom(card, state->mErfMuxBase + MUX_RX_IOP, 0x00000002);
	
}

static void
dag37t_mux_default(ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
	d37t_erfmux_state_t* state = component_get_private_state(component);
	card_write_iom(card, state->mErfMuxBase + MUX_RX_HOST, 0x00000001);
	card_write_iom(card, state->mErfMuxBase + MUX_RX_PHY, 0x00000000);
	card_write_iom(card, state->mErfMuxBase + MUX_RX_IOP, 0x00000002);
}

static dag_err_t
dag37t_mux_update_register_base(ComponentPtr component)
{
	return kDagErrNone;    
}

static int
dag37t_mux_post_initialize(ComponentPtr component)
{
	AttributePtr attr = NULL;
	d37t_erfmux_state_t* state = component_get_private_state(component);

	/* this is hard-coded to 0x400 for now, possibly in the future there will be a enumuration entry */
	state->mErfMuxBase = ERFMUX_REG_ADDR;

	/* line steering mode */
	attr = get_new_line_steering_mode();
	component_add_attribute(component, attr);

	/* host steering mode */
	attr = get_new_host_steering_mode();
	component_add_attribute(component, attr);
	
	/* XSCALE steering mode */
	attr = get_new_xscale_steering_mode();
	component_add_attribute(component, attr);

	
	return 1;
}


static AttributePtr
get_new_line_steering_mode(void)
{
	AttributePtr result = attribute_init(kUint32AttributeLineSteeringMode);

	if (NULL != result)
	{
		attribute_set_getvalue_routine(result, line_steering_mode_get_value);
		attribute_set_setvalue_routine(result, line_steering_mode_set_value);
		attribute_set_to_string_routine(result, steering_mode_to_string);
		attribute_set_from_string_routine(result, line_steering_mode_from_string);
		attribute_set_name(result, "line_steering_mode");
		attribute_set_description(result, "Steering direction for packets received from the line");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_valuetype(result, kAttributeUint32);
	}

	return result;
}
static void*
line_steering_mode_get_value(AttributePtr attribute)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = component_get_card(comp);
	d37t_erfmux_state_t * state = component_get_private_state(comp);
	uint32_t register_value;
	static uint32_t value;

	if (1 == valid_attribute(attribute))
	{
		register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_PHY);
		switch (register_value & 0x3)
		{
			case 0x0:   value = kSteerHost;     break;
			case 0x1:   value = kSteerLine;     break;
			case 0x2:   value = kSteerIXP;      break;
			default:    value = kSteerInvalid;  break;
		}
		return (void*)&value; 
	}
	return NULL;
}
static void
line_steering_mode_set_value(AttributePtr attribute, void* value, int length)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = component_get_card(comp);
	d37t_erfmux_state_t * state = component_get_private_state(comp);
	uint32_t register_value = 0;

	if (1 == valid_attribute(attribute))
	{
		switch(*(erfmux_steer_t*)value)
		{
			case kSteerLine:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_PHY);
				register_value = 0x00000001;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_PHY, register_value);
				break;

			case kSteerHost:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_PHY);
				register_value = 0x00000000;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_PHY, register_value);
				break;

			case kSteerIXP:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_PHY);
				register_value = 0x00000002;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_PHY, register_value);
				break;

			default:
				break;
		}
	}
}

static void
line_steering_mode_from_string(AttributePtr attribute, const char* string)
{
    erfmux_steer_t value = string_to_erfmux_steer(string);
    if (kSteerInvalid != value)
        line_steering_mode_set_value(attribute, (void*)&value, sizeof(erfmux_steer_t));
}


static AttributePtr
get_new_host_steering_mode(void)
{
	AttributePtr result = attribute_init(kUint32AttributeHostSteeringMode);

	if (NULL != result)
	{
		attribute_set_getvalue_routine(result, host_steering_mode_get_value);
		attribute_set_setvalue_routine(result, host_steering_mode_set_value);
		attribute_set_to_string_routine(result, steering_mode_to_string);
		attribute_set_from_string_routine(result, host_steering_mode_from_string);
		attribute_set_name(result, "host_steering_mode");
		attribute_set_description(result, "Steering direction for packets received from the host");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_valuetype(result, kAttributeUint32);
	}

	return result;
}
static void*
host_steering_mode_get_value(AttributePtr attribute)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = component_get_card(comp);
	d37t_erfmux_state_t * state = component_get_private_state(comp);
	uint32_t register_value;
	static uint32_t value;

	if (1 == valid_attribute(attribute))
	{
		register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_HOST);
		switch ( register_value & 0x3 )
		{
			case 0x0:   value = kSteerHost;     break;
			case 0x1:   value = kSteerLine;     break;
			case 0x2:   value = kSteerIXP;      break;
			default:    value = kSteerInvalid;  break;
		}
		return (void*)&value; 
	}
	return NULL;
}

static void
host_steering_mode_set_value(AttributePtr attribute, void* value, int length)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = component_get_card(comp);
	d37t_erfmux_state_t * state = component_get_private_state(comp);
	uint32_t register_value = 0;

	if (1 == valid_attribute(attribute))
	{
		switch(*(erfmux_steer_t*)value)
		{
			case kSteerLine:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_HOST);
				register_value = 0x00000001;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_HOST, register_value);
				break;

			case kSteerHost:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_HOST);
				register_value = 0x00000000;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_HOST, register_value);
				break;

			case kSteerIXP:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_HOST);
				register_value = 0x00000002;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_HOST, register_value);
				break;

			default:
				break;
		}
	}
}


static void
steering_mode_to_string(AttributePtr attribute)
{
    erfmux_steer_t value = *(erfmux_steer_t*)attribute_get_value(attribute);
    const char* temp = erfmux_steer_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}


static void
host_steering_mode_from_string(AttributePtr attribute, const char* string)
{
    erfmux_steer_t value = string_to_erfmux_steer(string);
    if (kSteerInvalid != value)
        host_steering_mode_set_value(attribute, (void*)&value, sizeof(erfmux_steer_t));
}


static AttributePtr
get_new_xscale_steering_mode(void)
{
	AttributePtr result = attribute_init(kUint32AttributeIXPSteeringMode);
	
	if (NULL != result)
	{
		attribute_set_getvalue_routine(result, xscale_steering_mode_get_value);
		attribute_set_setvalue_routine(result, xscale_steering_mode_set_value);
		attribute_set_to_string_routine(result, steering_mode_to_string);
		attribute_set_from_string_routine(result, xscale_steering_mode_from_string);
		attribute_set_name(result, "xscale_steering_mode");
		attribute_set_description(result, "Steering direction for packets received from the xscale");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_valuetype(result, kAttributeUint32);
	}
	
	return result;
}
static void*
xscale_steering_mode_get_value(AttributePtr attribute)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = component_get_card(comp);
	d37t_erfmux_state_t * state = component_get_private_state(comp);
	uint32_t register_value;
	static uint32_t value;

	if (1 == valid_attribute(attribute))
	{
		register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_IOP);
		if (register_value & BIT7 )
			value = kSteerDirectionBit;
		switch ( register_value & 0x3 )
		{
			case 0x0:   value = kSteerHost;     break;
			case 0x1:   value = kSteerLine;     break;
			case 0x2:   value = kSteerIXP;      break;
			default:    value = kSteerInvalid;  break;
		}
		return (void*)&value; 
	}
	return NULL;
}
static void
xscale_steering_mode_set_value(AttributePtr attribute, void* value, int length)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = component_get_card(comp);
	d37t_erfmux_state_t * state = component_get_private_state(comp);
	uint32_t register_value = 0;

	if (1 == valid_attribute(attribute))
	{
		switch(*(erfmux_steer_t*)value)
		{
			case kSteerLine:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_IOP);
				register_value = 0x00000001;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_IOP, register_value);
				break;

			case kSteerHost:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_IOP);
				register_value = 0x00000000;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_IOP, register_value);
				break;

			case kSteerIXP:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_IOP);
				register_value = 0x00000002;
				card_write_iom(card, state->mErfMuxBase + MUX_RX_IOP, register_value);
				break;
			case kSteerDirectionBit:
				register_value = card_read_iom(card, state->mErfMuxBase + MUX_RX_IOP);
                                register_value = 0x00000080;
                                card_write_iom(card, state->mErfMuxBase + MUX_RX_IOP, register_value);
                                break;

			
			default:
				break;
		}
	}
}

static void
xscale_steering_mode_from_string(AttributePtr attribute, const char* string)
{
    erfmux_steer_t value = string_to_erfmux_steer(string);
    if (kSteerInvalid != value)
        xscale_steering_mode_set_value(attribute, (void*)&value, sizeof(erfmux_steer_t));
}
