/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef COMPONENT_H
#define COMPONENT_H

/* Public API headers. */
#include "dag_attribute_codes.h"
#include "dag_component_codes.h"

/* Internal project headers. */
#include "attribute_types.h"
#include "card_types.h"
#include "component_types.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	/* Factory routine. */
	ComponentPtr component_init(dag_component_code_t component_code, DagCardPtr parent_card);
	
	/* Component methods. */
	void component_dispose(ComponentPtr component);
	void component_post_initialize(ComponentPtr component);
	void component_reset(ComponentPtr component);
	void component_default(ComponentPtr component);
	ComponentPtr component_get_parent(ComponentPtr component);
	dag_component_code_t component_get_component_code(ComponentPtr component);
	DagCardPtr component_get_card(ComponentPtr component);
	void* component_get_private_state(ComponentPtr component);
	void component_set_private_state(ComponentPtr component, void* state);
    const char* component_get_name(ComponentPtr component);
    const char* component_get_description(ComponentPtr component);
    void component_set_parent(ComponentPtr component, ComponentPtr mParent);

	
	/* Subcomponent access. */
	ComponentPtr component_get_subcomponent(ComponentPtr component, dag_component_code_t component_code, int index);
	int component_get_subcomponent_count(ComponentPtr component);
	ComponentPtr component_get_indexed_subcomponent(ComponentPtr component, int index);
	void component_add_subcomponent(ComponentPtr component, ComponentPtr subcomponent);
    int component_get_subcomponent_count_of_type(ComponentPtr component, dag_component_code_t code);
    ComponentPtr component_get_indexed_subcomponent_of_type(ComponentPtr component, dag_component_code_t code, int index);
	
	/* Attribute access. */
	AttributePtr component_get_attribute(ComponentPtr component, dag_attribute_code_t attribute_code);
	void component_add_attribute(ComponentPtr component, AttributePtr attribute);
    void component_delete_attribute(ComponentPtr component, dag_attribute_code_t code);
	int component_get_attribute_count(ComponentPtr component);
	AttributePtr component_get_indexed_attribute(ComponentPtr component, int index);
    void component_dispose_attribute(ComponentPtr component, AttributePtr attribute);
    int component_has_attributes(ComponentPtr component);
	
	void component_get_status_header(ComponentPtr component, char* buffer, int buflen);
	void component_get_status_data(ComponentPtr component, char* buffer, int buflen);
	
	void component_set_dispose_routine(ComponentPtr component, ComponentDisposeRoutine routine);
	void component_set_post_initialize_routine(ComponentPtr component, ComponentPostInitializeRoutine routine);
	void component_set_reset_routine(ComponentPtr component, ComponentResetRoutine routine);
	void component_set_default_routine(ComponentPtr component, ComponentDefaultRoutine routine);
    void component_set_name(ComponentPtr component, const char* name);
    void component_set_description(ComponentPtr component, const char* name);
    void component_set_update_register_base_routine(ComponentPtr component, ComponentUpdateRegisterBaseRoutine routine);
    dag_err_t component_update_register_base(ComponentPtr component);
    AttributePtr component_indexed_get_attribute_of_type(ComponentPtr component, dag_attribute_code_t attribute_code, int index);
    int component_get_attribute_count_of_type(ComponentPtr component, dag_attribute_code_t code);
	
	/* Verification routine.  Returns 1 if the ComponentPtr is valid, 0 otherwise. */
	int valid_component(ComponentPtr component);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPONENT_H */
