/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
*/

#include "../include/attribute.h"
#include "../include/component.h"

#include "../include/components/pattern_match_component.h"

#define BUFFER_SIZE 1024

static void pattern_match_dispose(ComponentPtr component);
static void pattern_match_reset(ComponentPtr component);
static void pattern_match_default(ComponentPtr component);
static int pattern_match_post_initialize(ComponentPtr component);

/* Match Value attribute */
static AttributePtr get_new_match_value_attribute(void);
static void* match_value_attribute_get_value(AttributePtr attribute);
static void match_value_attribute_set_value(AttributePtr attribute, void* value, int length);

/* Match Mask attribute */
static AttributePtr get_new_match_mask_attribute(void);
static void* match_mask_attribute_get_value(AttributePtr attribute);
static void match_mask_attribute_set_value(AttributePtr attribute, void* value, int length);

typedef struct pattern_match_state
{
	uint32_t mIndex;
	uint32_t mBase;
} pattern_match_state_t;

typedef enum
{
	kMatchValue		= 0x00,
	kMatchMask		= 0x04
} pattern_match_register_offset_t;


ComponentPtr
pattern_match_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPatternMatch, card);
    char buffer[BUFFER_SIZE];

    if (NULL != result)
    {
        pattern_match_state_t* state = (pattern_match_state_t*)malloc(sizeof(pattern_match_state_t));
        component_set_dispose_routine(result, pattern_match_dispose);
        component_set_reset_routine(result, pattern_match_reset);
        component_set_default_routine(result, pattern_match_default);
        component_set_post_initialize_routine(result, pattern_match_post_initialize);
        sprintf(buffer, "pattern_match%d", index);
        component_set_name(result, buffer);
        component_set_description(result, "The Pattern Matching Module.");
        state->mIndex = index;
	state->mBase = card_get_register_address(card, DAG_REG_PATTERN_MATCH, state->mIndex);
        component_set_private_state(result, state);
    }

    return result;
}

static void
pattern_match_dispose(ComponentPtr component)
{
	// Unused
}

static void
pattern_match_reset(ComponentPtr component)
{
	// Unused
}

static void
pattern_match_default(ComponentPtr component)
{
	AttributePtr attr;
	uint32_t value = 0;

	if (1 == valid_component(component))
	{
		value = 0x0;
		attr = component_get_attribute(component, kUint32AttributeMatchValue);
		attribute_uint32_set_value(attr, &value, 1);

		value = 0x3FF;
		attr = component_get_attribute(component, kUint32AttributeMatchMask);
		attribute_uint32_set_value(attr, &value, 1);
	}
}

static int
pattern_match_post_initialize(ComponentPtr component)
{
	AttributePtr value;
	AttributePtr mask;

	if (1 == valid_component(component))
	{
		/* Create new attributes. */
		value = get_new_match_value_attribute();
		mask = get_new_match_mask_attribute();
		
		/* Add attributes to the connection. */
		component_add_attribute(component, value);
		component_add_attribute(component, mask);
	
		return 1;
	}
	return kDagErrInvalidParameter;
}


static AttributePtr
get_new_match_value_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMatchValue);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, match_value_attribute_get_value);
        attribute_set_setvalue_routine(result, match_value_attribute_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_hex_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_hex_string);
        attribute_set_name(result, "match_value");
        attribute_set_description(result, "Pattern matching value (32-bits).");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
match_value_attribute_get_value(AttributePtr attribute)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = attribute_get_card(attribute);
	pattern_match_state_t *state = (pattern_match_state_t *)component_get_private_state(comp);
	uint32_t value = 0;

	if (1 == valid_attribute(attribute))
	{
		value = card_read_iom(card, state->mBase + kMatchValue);
	
		attribute_set_value_array(attribute, &value, sizeof(value));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

static void
match_value_attribute_set_value(AttributePtr attribute, void* value, int length)
{
	DagCardPtr card = attribute_get_card(attribute);
	ComponentPtr comp = attribute_get_component(attribute);
	pattern_match_state_t *state = (pattern_match_state_t *)component_get_private_state(comp);
	
	if (1 == valid_attribute(attribute))
	{
		card_write_iom(card, state->mBase + kMatchValue, *(uint32_t *)value);
	}
}

static AttributePtr
get_new_match_mask_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMatchMask);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, match_mask_attribute_get_value);
        attribute_set_setvalue_routine(result, match_mask_attribute_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_hex_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_hex_string);
        attribute_set_name(result, "match_mask");
        attribute_set_description(result, "Pattern matching mask (32-bits).");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
match_mask_attribute_get_value(AttributePtr attribute)
{
	ComponentPtr comp = attribute_get_component(attribute);
	DagCardPtr card = attribute_get_card(attribute);
	pattern_match_state_t *state = (pattern_match_state_t *)component_get_private_state(comp);
	uint32_t value = 0;

	if (1 == valid_attribute(attribute))
	{
		value = card_read_iom(card, state->mBase + kMatchMask);
	
		attribute_set_value_array(attribute, &value, sizeof(value));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

static void
match_mask_attribute_set_value(AttributePtr attribute, void* value, int length)
{
	DagCardPtr card = attribute_get_card(attribute);
	ComponentPtr comp = attribute_get_component(attribute);
	pattern_match_state_t *state = (pattern_match_state_t *)component_get_private_state(comp);
	
	if (1 == valid_attribute(attribute))
	{
		card_write_iom(card, state->mBase + kMatchMask, *(uint32_t *)value);
	}
}


