/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* File header. */
#include "dag_component.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdlib.h>

#include "dag_config.h"
#include "include/card.h"
#include "include/card_types.h"
#include "include/component.h"
#include "include/attribute.h"
#include "string.h"


/* Public API routines. */
dag_component_t
dag_component_get_parent(dag_component_t ref)
{
    ComponentPtr component = (ComponentPtr)ref;
    assert(NULL != ref);
    if(1 ==  valid_component(component))
    {
        return (dag_component_t) component_get_parent(component);
    }
    return NULL;
}
            
int
dag_component_get_subcomponent_count(dag_component_t ref)
{
    ComponentPtr component = (ComponentPtr)ref;
	assert(NULL != ref);
    if (1 == valid_component(component))
    {
        return component_get_subcomponent_count(component);
    }
	return 0; 
}

dag_component_t
dag_component_get_indexed_subcomponent(dag_component_t ref, int comp_index)
{
    ComponentPtr component = (ComponentPtr)ref;
	assert(NULL != ref);
    if (1 == valid_component(component))
    {
        return (dag_component_t)component_get_indexed_subcomponent(component, comp_index);
    }
	return NULL; 
}


dag_component_t
dag_component_get_named_subcomponent(dag_component_t ref, const char* name)
{
    int i = 0;
    int component_count = 0;
    ComponentPtr component = 0;
    ComponentPtr subcomponent = 0;
	assert(NULL != ref);
	assert(NULL != name);

    component = (ComponentPtr)ref;
    if (1 == valid_component(component))
    {
        component_count = component_get_subcomponent_count(component);   
        for (i = 0; i < component_count; i++)
        {
            subcomponent = component_get_indexed_subcomponent(component, i);
            if (0 == strcmp(name, component_get_name(subcomponent)))
            {
                return (dag_component_t)subcomponent;
            }
        }
    }

	return NULL; 
}


dag_component_t
dag_component_get_subcomponent(dag_component_t ref, dag_component_code_t component_code, int comp_index)
{
    ComponentPtr component = (ComponentPtr)ref;
    ComponentPtr subcomponent;
	DagCardPtr card = component_get_card(component);
	assert(NULL != ref);
	assert(kFirstComponentCode <= component_code);
	assert(component_code <= kLastComponentCode);
    
    if (valid_component(component))
    {
        subcomponent = component_get_subcomponent(component, component_code, comp_index);
        return (dag_component_t)subcomponent;
    }
	card_set_last_error(card, kDagErrNoSuchComponent);
	return NULL; 
}


int
dag_component_get_config_line_count(dag_component_t ref)
{
	assert(NULL != ref);

	return 0; 
}


dag_err_t
dag_component_get_indexed_config_line(dag_component_t ref, int line_num, char* buffer, int buflen)
{
	assert(NULL != ref);
	assert(NULL != buffer);
	assert(0 != buflen);

	return kDagErrUnimplemented; /* FIXME: unimplemented routine. */
}


attr_uuid_t
dag_component_get_indexed_status_attribute_uuid(dag_component_t ref, int attr_index)
{
    ComponentPtr component = (ComponentPtr)ref;
    AttributePtr attribute;
    int i = 0;
    int attr_count = 0;
    int status_index = 0;
    assert(NULL != ref);

    if (1 == valid_component(component))
    {
        attr_count = component_get_attribute_count(component);
        for (i = 0 ; i < attr_count; i++)
        {
            attribute = component_get_indexed_attribute(component, i);
            if (attribute_get_config_status(attribute) == kDagAttrStatus)
            {
                if (attr_index == status_index)
                {
                    return (attr_uuid_t)attribute;
                }
                status_index++;
            }
        }
    }
    return kNullAttributeUuid;
}

attr_uuid_t
dag_component_get_indexed_config_attribute_uuid(dag_component_t ref, int attr_index)
{
    ComponentPtr component = (ComponentPtr)ref;
    AttributePtr attribute;
    int i = 0;
    int attr_count = 0;
    int config_index = 0;
    assert(NULL != ref);

    if (1 == valid_component(component))
    {
        attr_count = component_get_attribute_count(component);
        for (i = 0 ; i < attr_count; i++)
        {
            attribute = component_get_indexed_attribute(component, i);
            if (attribute_get_config_status(attribute) == kDagAttrConfig)
            {
                if (attr_index == config_index)
                {
                    return (attr_uuid_t)attribute;
                }
                config_index++;
            }
        }
    }
    return kNullAttributeUuid;
}

attr_uuid_t
dag_component_get_indexed_attribute_uuid(dag_component_t ref, int attr_index)
{
    ComponentPtr component = (ComponentPtr)ref;
	assert(NULL != ref);
    if (1 == valid_component(component))
    {
        return (attr_uuid_t)component_get_indexed_attribute(component, attr_index);
    }

	return kNullAttributeUuid; 
}


attr_uuid_t
dag_component_get_named_config_attribute_uuid(dag_component_t ref, const char* name)
{
	ComponentPtr component = (ComponentPtr)ref;
    AttributePtr attribute;
    int i = 0;
    int attribute_count = 0;
    assert(NULL != ref);
	assert(NULL != name);
    
    attribute_count = component_get_attribute_count(component);
    for (i = 0; i < attribute_count; i++)
    {
        attribute = component_get_indexed_attribute(component, i);
        if ((0 == strcmp(name, attribute_get_name(attribute))))
        {
            return (attr_uuid_t)attribute;
        }
    }
    
	return kNullAttributeUuid; 
}


attr_uuid_t
dag_component_get_config_attribute_uuid(dag_component_t ref, dag_attribute_code_t attribute_code)
{
    AttributePtr attribute = component_get_attribute((ComponentPtr)ref, attribute_code);
	assert(NULL != ref);
	assert(kFirstAttributeCode <= attribute_code);
	assert(attribute_code <= kLastAttributeCode);
    if (valid_attribute(attribute) == 1 && attribute_get_config_status(attribute) == kDagAttrConfig)
    {
        return (attr_uuid_t)attribute; 
    }
    return kNullAttributeUuid;
}

attr_uuid_t
dag_component_get_attribute_uuid(dag_component_t ref, dag_attribute_code_t attribute_code)
{
    AttributePtr
        attribute = component_get_attribute((ComponentPtr)ref, attribute_code);
	DagCardPtr card = component_get_card((ComponentPtr)ref);
	assert(NULL != ref);
	assert(kFirstAttributeCode <= attribute_code);
	assert(attribute_code <= kLastAttributeCode);
    if (valid_attribute(attribute) == 1)
    {
        return (attr_uuid_t)attribute; 
    }
	card_set_last_error(card, kDagErrNoSuchAttribute);
    return kNullAttributeUuid;
}
dag_err_t
dag_component_get_status_header_line(dag_component_t ref, char* buffer, int buflen)
{
	assert(NULL != ref);
	assert(NULL != buffer);
	assert(0 != buflen);

	return kDagErrUnimplemented; /* FIXME: unimplemented routine. */
}


dag_err_t
dag_component_get_status_data_line(dag_component_t ref, char* buffer, int buflen)
{
	assert(NULL != ref);
	assert(NULL != buffer);
	assert(0 != buflen);

	return kDagErrUnimplemented; /* FIXME: unimplemented routine. */
}


int
dag_component_get_status_token_count(dag_component_t ref)
{
	assert(NULL != ref);

	return 0; /* FIXME: unimplemented routine. */
}


attr_uuid_t
dag_component_get_named_status_attribute_uuid(dag_component_t ref, const char* name)
{
	assert(NULL != ref);
	assert(NULL != name);

	return kNullAttributeUuid; /* FIXME: unimplemented routine. */
}

int
dag_component_get_attribute_count(dag_component_t ref)
{
    ComponentPtr component = (ComponentPtr)ref;

    if (1 == valid_component(component))
    {
        return component_get_attribute_count(component);
    }
    return 0;
}

int
dag_component_get_config_attribute_count(dag_component_t ref)
{
    ComponentPtr component = (ComponentPtr)ref;
    AttributePtr attribute;
    int total_count= 0;
    int config_count = 0;
    int i = 0;

    if (1 == valid_component(component))
    {
        total_count = component_get_attribute_count(component);
        for (i = 0; i < total_count; i++)
        {
            attribute = component_get_indexed_attribute(component, i);
            if (attribute_get_config_status(attribute) == kDagAttrConfig)
            {
                config_count++;
            }
        }
    }
    return config_count;
}

int
dag_component_get_status_attribute_count(dag_component_t ref)
{
    ComponentPtr component = (ComponentPtr)ref;
    AttributePtr attribute;
    int total_count= 0;
    int status_count = 0;
    int i = 0;

    if (1 == valid_component(component))
    {
        total_count = component_get_attribute_count(component);
        for (i = 0; i < total_count; i++)
        {
            attribute = component_get_indexed_attribute(component, i);
            if (attribute_get_config_status(attribute) == kDagAttrStatus)
            {
                status_count++;
            }
        }
    }
    return status_count;
}

int
dag_component_get_subcomponent_count_of_type(dag_component_t ref, dag_component_code_t code)
{
    ComponentPtr component = (ComponentPtr)ref;
	DagCardPtr card = component_get_card(component);
    
    if (1 == valid_component(component))
    {
        return component_get_subcomponent_count_of_type(component, code);
    }
	card_set_last_error(card, kDagErrNoSuchComponent);
    return 0;
}


int
dag_component_get_attribute_count_of_type(dag_component_t ref, dag_attribute_code_t attribute_code)
{
    ComponentPtr component = (ComponentPtr)ref;
    AttributePtr attribute;
    int total_count= 0;
    int type_count = 0;
    int i = 0;

    if (1 == valid_component(component))
    {
        total_count = component_get_attribute_count(component);
        for (i = 0; i < total_count; i++)
        {
            attribute = component_get_indexed_attribute(component, i);
            if (attribute_get_attribute_code(attribute) == attribute_code)
            {
                type_count++;
            }
        }
    }
    return type_count;
}


attr_uuid_t
dag_component_get_indexed_attribute_uuid_of_type(dag_component_t ref, dag_attribute_code_t attribute_code, int index)
{
    ComponentPtr component = (ComponentPtr)ref;
    AttributePtr attribute;
    int i = 0;
    int attr_count = 0;
    int type_count = 0;
    assert(NULL != ref);

    if (1 == valid_component(component))
    {
        attr_count = component_get_attribute_count(component);
        for (i = 0 ; i < attr_count; i++)
        {
            attribute = component_get_indexed_attribute(component, i);
            if (attribute_get_attribute_code(attribute) == attribute_code)
            {
                if (index == type_count)
                {
                    return (attr_uuid_t)attribute;
                }
                type_count++;
            }
        }
    }
    return kNullAttributeUuid;
}
/*Added to get the card reference from the component reference.*/
dag_card_ref_t
dag_component_get_card_ref(dag_component_t ref)
{
        DagCardPtr card = component_get_card((ComponentPtr)ref);
	assert(NULL != ref);
	
	if (valid_card(card) == 1)
    	{
        	return (dag_card_ref_t)card; 
    	}
	card_set_last_error(card, kDagErrInvalidCardRef);
    return kNullAttributeUuid;
}
