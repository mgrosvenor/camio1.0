/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

/* Public API headers. */
#include "dag_attribute_codes.h"
#include "dag_config.h"

/* Internal project headers. */
#include "attribute_types.h"
#include "card_types.h"
#include "component_types.h"
#include "modules/generic_read_write.h"
#include "modules/latch.h"



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	/* Factory routine. */
	AttributePtr attribute_init(dag_attribute_code_t attribute_code);

	/* Attribute methods. */
	void attribute_dispose(AttributePtr attribute);
	void attribute_post_initialize(AttributePtr attribute);
	
	DagCardPtr attribute_get_card(AttributePtr attribute);
	ComponentPtr attribute_get_component(AttributePtr attribute);
	void attribute_set_component_and_card(AttributePtr attribute, ComponentPtr component, DagCardPtr card);
	void* attribute_get_private_state(AttributePtr attribute);
	void attribute_set_private_state(AttributePtr attribute, void* state);
	
    /* Attribute member getters */
	void* attribute_get_value(AttributePtr attribute);
	void attribute_set_value(AttributePtr attribute, void* value, int length);
	dag_attribute_code_t attribute_get_attribute_code(AttributePtr attribute);
	void attribute_set_default(AttributePtr attribute);
	void* attribute_get_default_value(AttributePtr attribute);
	void attribute_set_default_value(AttributePtr attribute, void* value);
	const char* attribute_get_name(AttributePtr attribute);
    void attribute_set_name(AttributePtr attribute, const char* name);
	const char* attribute_get_description(AttributePtr attribute);
	const char* attribute_get_type_to_string(AttributePtr attribute);
	const char* attribute_get_valuetype_to_string(AttributePtr attribute);
    void attribute_set_description(AttributePtr attribute, const char* description);
	int attribute_get_writable(AttributePtr attribute);
	dag_attr_t attribute_get_valuetype(AttributePtr attribute);
	void attribute_set_valuetype(AttributePtr attribute, dag_attr_t val);
    void attribute_set_config_status(AttributePtr attribute, dag_attr_config_status_t val);
    dag_attr_config_status_t attribute_get_config_status(AttributePtr attribute);
    int attribute_get_value_array_count(AttributePtr attribute);
    void* attribute_get_value_array(AttributePtr attribute);
    GenericReadWritePtr attribute_get_generic_read_write_object(AttributePtr attribute);
    LatchPtr attribute_get_latch_object(AttributePtr attribute);
        uint32_t* attribute_get_masks(AttributePtr attribute);

    /* attribute member setter functions */
    void attribute_set_dispose_routine(AttributePtr attribute, AttributeDisposeRoutine routine);
	void attribute_set_post_initialize_routine(AttributePtr attribute, AttributePostInitializeRoutine routine);
	void attribute_set_getvalue_routine(AttributePtr attribute, AttributeGetValueRoutine routine);
	void attribute_set_setvalue_routine(AttributePtr attribute, AttributeSetValueRoutine routine);
    void attribute_set_value_array_count(AttributePtr attribute, int count);
    void attribute_set_generic_read_write_object(AttributePtr attribute, GenericReadWritePtr rw);
    void attribute_set_value_array(AttributePtr attribute, void* array, int len);
    void attribute_set_to_string_routine(AttributePtr attribute, AttributeToStringRoutine routine);
    void attribute_set_from_string_routine(AttributePtr attribute, AttributeFromStringRoutine routine);
    void attribute_set_from_string(AttributePtr attribute, const char* value);
    void attribute_set_masks(AttributePtr attribute, const uint32_t* masks, int len);
    void attribute_set_latch_object(AttributePtr attribute, LatchPtr latch);
    
    /*converting an attribute's value to and from a string*/
    const char* attribute_to_string(AttributePtr attribute);
    void attribute_set_to_string(AttributePtr attribute, const char* value);
    const char* attribute_get_to_string(AttributePtr attribute);
    void attribute_boolean_to_string(AttributePtr attribute);
    void attribute_boolean_to_int_string(AttributePtr attribute);
    void attribute_uint32_to_string(AttributePtr attribute);
    void attribute_uint32_to_hex_string(AttributePtr attribute);
    void attribute_int32_to_string(AttributePtr attribute);
    void attribute_uint64_to_string(AttributePtr attribute);
    void attribute_float_to_string(AttributePtr attribute);
    void attribute_boolean_from_string(AttributePtr attribute, const char* string);
    void attribute_uint32_from_string(AttributePtr attribute, const char* string);
    void attribute_uint32_from_hex_string(AttributePtr attribute, const char* string);
    void attribute_int32_from_string(AttributePtr attribute, const char * string);
    void attribute_uint64_from_string(AttributePtr attribute, const char* string);
    void attribute_float_from_string(AttributePtr attribute, const char* string);

    /*Functions that use the GenericReadWritePtr to get their values*/
    void attribute_boolean_set_value(AttributePtr attribute, void* value, int len);
    void attribute_boolean_set_value_with_clear(AttributePtr attribute, void* value, int len);
    void attribute_boolean_set_reset_tcp_flow(AttributePtr attribute, void* value, int len);
    void* attribute_boolean_get_value(AttributePtr attribute);
    void attribute_int32_set_value(AttributePtr attribute, void* value, int len);
    void* attribute_int32_get_value(AttributePtr attribute);
    void attribute_uint32_set_value(AttributePtr attribute, void* value, int len);
    void* attribute_uint32_get_value(AttributePtr attribute);
    void attribute_uint64_set_value(AttributePtr attribute, void* value, int len);
    void* attribute_uint64_get_value(AttributePtr attribute);
    void attribute_uint64_set_value_reverse(AttributePtr attribute, void* value, int len);
    void* attribute_uint64_get_value_reverse(AttributePtr attribute);
    void attribute_rocket_io_set_value(AttributePtr, void* value, int len);
    void attribute_refresh_cache_set_value(AttributePtr, void* value, int len);
    void* attribute_refresh_cache_get_value(AttributePtr);
    void* attribute_cache_get_value(AttributePtr);    
    void* attribute_steer_get_value(AttributePtr);    
    void attribute_steer_set_value(AttributePtr, void* value, int len);
    void* attribute_mac_get_value(AttributePtr);    
    void attribute_snaplen_set_value(AttributePtr, void* value, int len);
	
	/* Verification routine.  Returns 1 if the AttributePtr is valid, 0 otherwise. */
	int valid_attribute(AttributePtr attribute);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ATTRIBUTE_H */

