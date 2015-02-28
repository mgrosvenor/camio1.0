/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
*/

#include "../include/component.h"
#include "../include/attribute.h"
#include "../include/util/enum_string_table.h"

#include "../include/components/sonet_connection_component.h"
#include "../include/components/sonet_channel_mgmt_component.h"

#define BUFFER_SIZE 1024

static void sonet_connection_dispose(ComponentPtr component);
static void sonet_connection_reset(ComponentPtr component);
static void sonet_connection_default(ComponentPtr component);
static int sonet_connection_post_initialize(ComponentPtr component);

/* Active attribute */
static AttributePtr get_new_active_attribute(void);
static void* active_attribute_get_value(AttributePtr attribute);
static void active_attribute_set_value(AttributePtr attribute, void* value, int length);

/* Size attribute */
static AttributePtr get_new_size_attribute(void);
static void* size_attribute_get_value(AttributePtr attribute);
static void size_attribute_set_value(AttributePtr attribute, void* value, int length);
static void size_attribute_to_string_routine(AttributePtr attribute);
static void size_attribute_from_string_routine(AttributePtr attribute, const char* string);

/* Snaplength attribute */
static AttributePtr get_new_snaplength_attribute(void);
static void* snaplength_attribute_get_value(AttributePtr attribute);
static void snaplength_attribute_set_value(AttributePtr attribute, void* value, int length);

/* Select attribute */
static AttributePtr get_new_select_attribute(void);
static void* select_attribute_get_value(AttributePtr attribute);
static void select_attribute_set_value(AttributePtr attribute, void* value, int length);

/* VC-ID attribute */
static AttributePtr get_new_vc_id_attribute(void);
static void* vc_id_attribute_get_value(AttributePtr attribute);
static void vc_id_attribute_set_value(AttributePtr attribute, void* value, int length);

ComponentPtr
sonet_get_new_connection(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentConnection, card); 
    char buffer[BUFFER_SIZE];
    sonet_connection_state_t* state;
    
    if (NULL != result)
    {
        state = (sonet_connection_state_t*)malloc(sizeof(sonet_connection_state_t));
        state->mIndex = index;  // this becomes the connection number

        component_set_dispose_routine(result, sonet_connection_dispose);
        component_set_post_initialize_routine(result, sonet_connection_post_initialize);
        component_set_reset_routine(result, sonet_connection_reset);
        component_set_default_routine(result, sonet_connection_default);
        sprintf(buffer, "sonet_connection%d", index);
        component_set_name(result, buffer);
        component_set_description(result, "SONET Connection Component");
        component_set_private_state(result, state);
        return result;
    }
    return result;
}

static void
sonet_connection_dispose(ComponentPtr component)
{
	// Not implmented
}

static void
sonet_connection_reset(ComponentPtr component)
{
	// Not implmented
}

static void
sonet_connection_default(ComponentPtr component)
{
        sonet_connection_state_t* state = (sonet_connection_state_t*)malloc(sizeof(sonet_connection_state_t));

	if (1 == valid_component(component))
	{
		AttributePtr attr;
		uint8_t value_bool = 0;
		uint32_t value = 0;

		value_bool = 1;	
		attr = component_get_attribute(component, kBooleanAttributeActive);
		attribute_boolean_set_value(attr, &value_bool, 1);

		value = 0;
		attr = component_get_attribute(component, kUint32AttributeVCSize);
		attribute_uint32_set_value(attr, &value, 1);

		value = 128;
		attr = component_get_attribute(component, kUint32AttributeSnaplength);
		attribute_uint32_set_value(attr, &value, 1);

		value_bool = 0;
		attr = component_get_attribute(component, kBooleanAttributeConnectionSelect);
		attribute_boolean_set_value(attr, &value_bool, 1);

		value = state->mIndex;
		attr = component_get_attribute(component, kUint32AttributeVCIndex);
		attribute_uint32_set_value(attr, &value, 1);
	}	
}

static int
sonet_connection_post_initialize(ComponentPtr component)
{
    AttributePtr active;
    AttributePtr size;
    AttributePtr slen;
    AttributePtr select;
    AttributePtr vc_id;

    /* Create new attributes. */
    active = get_new_active_attribute();
    size = get_new_size_attribute();
    slen = get_new_snaplength_attribute();
    select = get_new_select_attribute();
    vc_id = get_new_vc_id_attribute();

    /* Add attributes to the connection. */
    component_add_attribute(component, active);
    component_add_attribute(component, size);
    component_add_attribute(component, slen);
    component_add_attribute(component, select);
    component_add_attribute(component, vc_id);

    return 1;
}

static AttributePtr
get_new_active_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, active_attribute_get_value);
        attribute_set_setvalue_routine(result, active_attribute_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "active");
        attribute_set_description(result, "SONET connection active");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
active_attribute_get_value(AttributePtr attribute)
{
    uint8_t value = 1;

    if (1 == valid_attribute(attribute))
    {
	// Always return 1 
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}


static void
active_attribute_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
	// Do nothing. Should not be attempting to do this.
    }
}

static AttributePtr
get_new_size_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeVCSize);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, size_attribute_get_value);
        attribute_set_setvalue_routine(result, size_attribute_set_value);
        attribute_set_to_string_routine(result, size_attribute_to_string_routine);
        attribute_set_from_string_routine(result, size_attribute_from_string_routine);
        attribute_set_name(result, "size");
        attribute_set_description(result, "SONET connection channel size.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
size_attribute_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    ComponentPtr parent = NULL;
    ComponentPtr root = NULL;
    ComponentPtr sonet_pp = NULL;
    sonet_connection_state_t * state = (sonet_connection_state_t *)component_get_private_state(comp);
    sonet_channel_mgmt_state_t *parent_state;
    uint32_t value = 0;
    uint32_t count;
    uint32_t sonet_pp_index = 0;
    AttributePtr mode_select = NULL;
    GenericReadWritePtr mode_select_grw = NULL;
    uint32_t mode_select_value = 0;
    uint32_t *masks;
    line_rate_t rate = kLineRateInvalid;

    if (1 == valid_attribute(attribute))
    {
        // Read information from the parent component's state table
	parent = component_get_parent(comp);
	parent_state = (sonet_channel_mgmt_state_t *)component_get_private_state(parent);

	// Search for the VC-ID that matches the connection index and retrieve the channel size information
	for (count = 0; count < parent_state->timeslots; count++)
	{
#if 0
		printf("timeslot: %d, vc-id: %d, size: %d\n",
			parent_state->timeslot_info[count].timeslot_id,
			parent_state->timeslot_info[count].vc_id,
			parent_state->timeslot_info[count].size);
#endif

		if (parent_state->timeslot_info[count].vc_id == state->mIndex)
		{
			value = parent_state->timeslot_info[count].size;
			break;
		}
	}

	// Retrieve the mode attribute from the SONET PP component to check if current mode is SONET/SDH
	root = component_get_parent(parent);
	sonet_pp_index = parent_state->mIndex;	// Use the same index as the parent SONET channel management component
	sonet_pp = component_get_subcomponent(root, kComponentSonetPP, sonet_pp_index);
	mode_select = component_get_attribute(sonet_pp, kBooleanAttributeSonetMode);
	mode_select_grw = attribute_get_generic_read_write_object(mode_select);
	mode_select_value = grw_read(mode_select_grw);
	masks = attribute_get_masks(mode_select);
	mode_select_value = (mode_select_value & (*masks)) >> 8;

	if (mode_select_value)
		switch(value)
		{
			case 0x00:
				rate = kLineRateOC1c;
				break;
			case 0x01:
				rate = kLineRateOC3c;
				break;
			case 0x02:
				rate = kLineRateOC12c;
				break;
			case 0x03:
				rate = kLineRateOC48c;
				break;
			case 0x04:
				rate = kLineRateOC192c;
				break;
		}
	else
		switch(value)
		{
			case 0x00:
				rate = kLineRateSTM0;
				break;
			case 0x01:
				rate = kLineRateSTM1;
				break;
			case 0x02:
				rate = kLineRateSTM4;
				break;
			case 0x03:
				rate = kLineRateSTM16;
				break;
			case 0x04:
				rate = kLineRateSTM64;
				break;
		}

        attribute_set_value_array(attribute, &rate, sizeof(rate));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
size_attribute_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        // Do nothing. Should not be attempting to do this.
    }
}

static void
size_attribute_to_string_routine(AttributePtr attribute)
{
    void* temp = attribute_get_value(attribute);
    const char* string = NULL;
    line_rate_t lr;
    if (temp)
    {
        lr = *(line_rate_t*)temp;
        string = line_rate_to_string(lr);
        if (string)
            attribute_set_to_string(attribute, string);
    }
}

static void
size_attribute_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        // Do nothing. Should not be attempting to do this.
    }
}

static AttributePtr
get_new_snaplength_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeSnaplength);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, snaplength_attribute_get_value);
        attribute_set_setvalue_routine(result, snaplength_attribute_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "snaplength");
        attribute_set_description(result, "SONET connection snaplength.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
snaplength_attribute_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    ComponentPtr parent = NULL;
    AttributePtr parent_attr = NULL;
    uint32_t value = 0;

    if (1 == valid_attribute(attribute))
    {
        // Read from the parent component
	parent = component_get_parent(comp);
	parent_attr = component_get_attribute(parent, kUint32AttributeSPESnaplength);
	value = *(uint32_t *)attribute_uint32_get_value(parent_attr);

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
snaplength_attribute_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        // Do nothing. May change in the future.
	// This should be set from the parent component (i.e. SONET channel management component) not the connection component.
    }
}

static AttributePtr
get_new_select_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeConnectionSelect);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, select_attribute_get_value);
        attribute_set_setvalue_routine(result, select_attribute_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "select");
        attribute_set_description(result, "SONET connection select.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
select_attribute_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    ComponentPtr parent = NULL;
    AttributePtr parent_attr = NULL;
    sonet_connection_state_t * state = (sonet_connection_state_t *)component_get_private_state(comp);
    uint32_t parent_value;
    uint8_t value = 0;

    if (1 == valid_attribute(attribute))
    {
        // Read vc-id attribute from parent component. If vc-id field is the same as the index return 1
	parent = component_get_parent(comp);
	parent_attr = component_get_attribute(parent, kUint32AttributeVCIDSelect);
	parent_value = *(uint32_t *)attribute_uint32_get_value(parent_attr);

	if (parent_value == state->mIndex)
		value = 1;
	else
		value = 0;

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
select_attribute_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    ComponentPtr parent = NULL;
    AttributePtr parent_attr = NULL;
    sonet_connection_state_t * state = (sonet_connection_state_t *)component_get_private_state(comp);
    uint32_t vc_id_value = state->mIndex;

    if (1 == valid_attribute(attribute))
    {
        // Retrieve vc-id attribute from parent component and set it to the index of this connection. Note this can also be set directly from the parent component.
	parent = component_get_parent(comp);
	parent_attr = component_get_attribute(parent, kUint32AttributeVCIDSelect);

	if (*(uint8_t *)value == 1)
		attribute_uint32_set_value(parent_attr, &vc_id_value, sizeof(value));
    }
}

static AttributePtr
get_new_vc_id_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeVCIndex);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, vc_id_attribute_get_value);
        attribute_set_setvalue_routine(result, vc_id_attribute_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "vc_id");
        attribute_set_description(result, "SONET connection channel identification number.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
vc_id_attribute_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    sonet_connection_state_t * state = (sonet_connection_state_t *)component_get_private_state(comp);
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
	// The vc-id of the connection component is just it's index
        value = state->mIndex;

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
vc_id_attribute_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        // Do nothing. Should not be attempting to do this.
    }
}


