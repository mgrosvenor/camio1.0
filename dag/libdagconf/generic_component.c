/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#if defined (_WIN32)
#define UNUSED
#endif

/* File header. */
#include "include/component.h"

/* Public API headers. */

/* Internal project headers. */
#include "include/attribute.h"
#include "include/util/set.h"
#include "include/util/utility.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined (__linux__)
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


/* Chosen randomly. */
#define COMPONENT_MAGIC 0xb6059c55


/* Software implementation of a DAG card. */
struct CardComponent
{
    /* Standard protection. */
    uint32_t mMagicNumber;
    uint32_t mChecksum;

    /* Members. */
    DagCardPtr mCard;
    ComponentPtr mParent;
    SetPtr mSubcomponents;
    SetPtr mAttributes;
    dag_component_code_t mComponentCode;
    void* mPrivateState;
    char* mName;
    char* mDescription;
    
    /* Methods. */
    ComponentDisposeRoutine fDispose;
    ComponentPostInitializeRoutine fPostInitialize;
    ComponentResetRoutine fReset;
    ComponentDefaultRoutine fDefault;
	ComponentUpdateRegisterBaseRoutine fUpdateRegisterBase;
};


/* Internal routines. */
static uint32_t compute_component_checksum(ComponentPtr component);
/* Default method implementations. */
static void generic_component_dispose(ComponentPtr component);
static int generic_component_post_initialize(ComponentPtr component);
static void generic_component_reset(ComponentPtr component);
static void generic_component_default(ComponentPtr component);


/* Implementation of internal routines. */
static uint32_t
compute_component_checksum(ComponentPtr component)
{
    /* Don't check validity here because the component may not be fully constructed yet. */
    if (component)
        return compute_checksum(&component->mCard, sizeof(*component) - 2 * sizeof(uint32_t));
    return 0;
}

static void
generic_component_dispose(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        /* Do nothing - default implementation. */
    }
}

static int
generic_component_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        return 1;
    }
    
    return 0;
}


static void
generic_component_reset(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        /* Call reset() on all subcomponents.
         * If the reset() calls must be ordered in a particular way, then this routine should be overridden.
         */
        int count = set_get_count(component->mSubcomponents);
        int index;
        
        /* Reset subcomponents. */
        for (index = 0; index < count; index++)
        {
            ComponentPtr subcomp = (ComponentPtr) set_retrieve(component->mSubcomponents, index);
        
            component_reset(subcomp);
        }
    }
}


static void
generic_component_default(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        /* Call default() on all subcomponents and attributes.
         * If the default() calls must be ordered in a particular way, then this routine should be overridden.
         */
        int count = set_get_count(component->mSubcomponents);
        int index;
        
        /* Reset subcomponents. */
        for (index = 0; index < count; index++)
        {
            ComponentPtr subcomp = (ComponentPtr) set_retrieve(component->mSubcomponents, index);
        
            component_default(subcomp);
        }
        
        /* Reset attributes. */
        count = set_get_count(component->mAttributes);
        for (index = 0; index < count; index++)
        {
            AttributePtr attr = (AttributePtr) set_retrieve(component->mAttributes, index);
        
            attribute_set_default(attr);
        }
    }
}


/* Factory routine. */
ComponentPtr
component_init(dag_component_code_t component_code, DagCardPtr card)
{
    ComponentPtr result;
    
    assert(kFirstComponentCode <= component_code);
    assert(component_code <= kLastComponentCode);
    
    result = (ComponentPtr) malloc(sizeof(*result));
    if (NULL != result)
    {
        memset(result, 0, sizeof(*result));
        
        result->mCard = card;
        result->mComponentCode = component_code;
        result->fDispose = generic_component_dispose;
        result->fPostInitialize = generic_component_post_initialize;
        result->fReset = generic_component_reset;
        result->fDefault = generic_component_default;
		result->fUpdateRegisterBase = NULL;
        result->mName = NULL;
        
        result->mSubcomponents = set_init();
        result->mAttributes = set_init();

        /* Compute checksum over the object. */
        result->mChecksum = compute_component_checksum(result);
        result->mMagicNumber = COMPONENT_MAGIC;
        
        (void) valid_component(result);
        if (component_code == kComponentRoot)
        {
            component_set_name(result, "root");
            component_set_description(result, "The root component");
        }
        else
        {
            component_set_name(result, "<unnamed>");
            component_set_description(result, "<undescribed>");
        }
    }
    
    return result;
}

static int
component_has_subcomponents(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        return component_get_subcomponent_count(component) > 0;
    }
    return 0;
}

static void
component_dispose_subcomponent(ComponentPtr component, ComponentPtr comp)
{
	if (1 == valid_component(component))
	{
        component_dispose(comp);
        set_delete(component->mSubcomponents, comp);
	}
}

/* Virtual method implementations. */
void
component_dispose(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        AttributePtr attribute = NULL;
		/* dispose of the subcomponents */
        while (component_has_subcomponents(component) == 1)
        {
            ComponentPtr comp;
            comp = component_get_indexed_subcomponent(component, 0);
            component_dispose_subcomponent(component, comp);
        }
        while (component_has_attributes(component) == 1)
        {
            attribute = component_get_indexed_attribute(component, 0);
            component_dispose_attribute(component, attribute);
        }
        /* Allow the specific implementation to clean up. */
        component->fDispose(component);
        /* Free resources. */
        set_dispose(component->mSubcomponents);
        set_dispose(component->mAttributes);
        
        free(component->mName);
        component->mName = NULL;
        free(component->mDescription);
        component->mDescription = NULL;
        free(component->mPrivateState);
        component->mPrivateState = NULL;


        memset(component, 0, sizeof(*component));
        free(component);
    }
}


void
component_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        int result = component->fPostInitialize(component);
        
        if (1 == result)
        {
            /* Call post_initialize() on all subcomponents and direct attributes.
             * If the reset() calls must be ordered in a particular way, then this routine should be overridden.
             */
            int count;
            int index;
            
            /* Post-initialize attributes. */
            count = set_get_count(component->mAttributes);
            for (index = 0; index < count; index++)
            {
                AttributePtr attribute = (AttributePtr) set_retrieve(component->mAttributes, index);
            
                attribute_post_initialize(attribute);
            }
            
            /* Post-initialize subcomponents. */
            count = set_get_count(component->mSubcomponents);
            for (index = 0; index < count; index++)
            {
                ComponentPtr subcomp = (ComponentPtr) set_retrieve(component->mSubcomponents, index);
            
                component_post_initialize(subcomp);
            }
        }
    }
}


void
component_reset(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        component->fReset(component);
    }
}


void
component_default(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        component->fDefault(component);
    }
}



/* Non-virtual method implementations. */
ComponentPtr
component_get_parent(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        return component->mParent;
    }
    
    return NULL;
}

/* Non-virtual method implementations. */
void
component_set_parent(ComponentPtr component, ComponentPtr mParent)
{
    if (1 == valid_component(component))
    {
         component->mParent = mParent;
    }
    component->mChecksum = compute_component_checksum(component);
    
    return;
}


dag_component_code_t
component_get_component_code(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        return component->mComponentCode;
    }
    
    return kComponentInvalid;
}


DagCardPtr
component_get_card(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        return component->mCard;
    }
    
    return NULL;
}


void*
component_get_private_state(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        return component->mPrivateState;
    }
    
    return NULL;
}


void
component_set_private_state(ComponentPtr component, void* state)
{
    if (1 == valid_component(component))
    {
        assert(NULL == component->mPrivateState); /* Possible memory leak if not disposed. */
        
        component->mPrivateState = state;
        component->mChecksum = compute_component_checksum(component);
        
        /* Make sure nothing has broken. */
        (void) valid_component(component);
    }
}


ComponentPtr
component_get_subcomponent(ComponentPtr component, dag_component_code_t component_code, int index)
{
	DagCardPtr card = component_get_card(component);
    assert(kFirstComponentCode <= component_code);
    assert(component_code <= kLastComponentCode);

    if (1 == valid_component(component))
    {
        int count = set_get_count(component->mSubcomponents);
        int i;
        int component_count = 0;
        
        for (i = 0; i < count; i++)
        {
            ComponentPtr subcomp = (ComponentPtr) set_retrieve(component->mSubcomponents, i);
            
            if (component_code == component_get_component_code(subcomp))
            {
                if (component_count == index)
                {
					card_set_last_error(card, kDagErrNone);
                    return subcomp;
                }
                else
                {
                    component_count++;
                }
            }
        }
		card_set_last_error(card, kDagErrNoSuchComponent);
		return NULL;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return NULL;
}

int
component_get_attribute_count_of_type(ComponentPtr component, dag_attribute_code_t code)
{
    DagCardPtr card = component_get_card(component);
    assert(kFirstAttributeCode <= code);
    assert(code <= kLastAttributeCode);

    if (1 == valid_component(component))
    {
        int i;
        int total = set_get_count(component->mAttributes);
        int count = 0;

        for (i = 0; i < total; i++)
        {
            AttributePtr any_attribute = (AttributePtr) set_retrieve(component->mAttributes, i);
            if (attribute_get_attribute_code(any_attribute) == code)
            {
                count++;
            }
        }
        return count;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return 0;
}

AttributePtr
component_indexed_get_attribute_of_type(ComponentPtr component, dag_attribute_code_t attribute_code, int index)
{
    DagCardPtr card = component_get_card(component);
    assert(kFirstAttributeCode <= attribute_code);
    assert(attribute_code <= kLastAttributeCode);

    if (1 == valid_component(component))
    {
        int count = set_get_count(component->mAttributes);
        int current_count = 0;
	int i;
        
        for (i = 0; i < count; i ++)
        {
            AttributePtr attribute = (AttributePtr) set_retrieve(component->mAttributes, i);
            
            if (attribute_code == attribute_get_attribute_code(attribute))
            {
                if ( index == current_count)
                    return attribute;
		current_count ++;
            }
        }
    }

	card_set_last_error(card, kDagErrInvalidComponent);
    return NULL;
}

void
component_add_subcomponent(ComponentPtr component, ComponentPtr subcomponent)
{
	DagCardPtr card = component_get_card(component);
    assert(NULL != subcomponent);
    
    if (1 == valid_component(component))
    {
        set_add(component->mSubcomponents, (void*) subcomponent);
	component_set_parent(subcomponent, component);
	return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}


int
component_get_subcomponent_count(ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        return set_get_count(component->mSubcomponents);
    }
	card_set_last_error(card, kDagErrInvalidComponent);

    return 0;
}

int
component_get_subcomponent_count_of_type(ComponentPtr component, dag_component_code_t code)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        int i;
        int total = set_get_count(component->mSubcomponents);
        int count = 0;
        ComponentPtr any_component;

        for (i = 0; i < total; i++)
        {
            any_component = set_retrieve(component->mSubcomponents, i);
            if (component_get_component_code(any_component) == code)
            {
                count++;
            }
        }
        return count;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return 0;
}

ComponentPtr
component_get_indexed_subcomponent_of_type(ComponentPtr component, dag_component_code_t code, int index)
{
	DagCardPtr card = component_get_card(component);
    assert(index < component_get_subcomponent_count_of_type(component, code));
    if (1 == valid_component(component))
    {
        int i;
        int total = set_get_count(component->mSubcomponents);
        int count = 0;
        ComponentPtr any_component;

        for (i = 0; i < total; i++)
        {
            any_component = set_retrieve(component->mSubcomponents, i);
            if (component_get_component_code(any_component) == code)
            {
                if (count == index)
                {
                    return any_component;
                }
                count++;
            }
        }
		card_set_last_error(card, kDagErrNoSuchComponent);
        return NULL;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return NULL;
}

ComponentPtr
component_get_indexed_subcomponent(ComponentPtr component, int index)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        int count __attribute__((unused)) = set_get_count(component->mSubcomponents);
        
        assert(index < count);
        
        return (ComponentPtr) set_retrieve(component->mSubcomponents, index);
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return NULL;
}


AttributePtr
component_get_attribute(ComponentPtr component, dag_attribute_code_t attribute_code)
{
	DagCardPtr card = component_get_card(component);
    assert(kFirstAttributeCode <= attribute_code);
    assert(attribute_code <= kLastAttributeCode);

    if (1 == valid_component(component))
    {
        int count = set_get_count(component->mAttributes);
        int index;
        
        for (index = 0; index < count; index++)
        {
            AttributePtr attribute = (AttributePtr) set_retrieve(component->mAttributes, index);
            
            if (attribute_code == attribute_get_attribute_code(attribute))
            {
                return attribute;
            }
        }
    }

	card_set_last_error(card, kDagErrInvalidComponent);
    return NULL;
}

void
component_delete_attribute(ComponentPtr component, dag_attribute_code_t code)
{
	DagCardPtr card = component_get_card(component);
    assert(kFirstAttributeCode <= code);
    assert(code <= kLastAttributeCode);

    if (1 == valid_component(component))
    {
        AttributePtr attribute;
        attribute = component_get_attribute(component, code);
        set_delete(component->mAttributes, attribute);
        return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}

void
component_add_attribute(ComponentPtr component, AttributePtr attribute)
{
	DagCardPtr card = component_get_card(component);
    assert(NULL != attribute);
    
    if (1 == valid_component(component))
    {
        set_add(component->mAttributes, (void*) attribute);
        
        assert(NULL != component->mCard);
        attribute_set_component_and_card(attribute, component, component->mCard);
        return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}


int
component_get_attribute_count(ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        return set_get_count(component->mAttributes);
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return 0;
}


AttributePtr
component_get_indexed_attribute(ComponentPtr component, int index)
{
	DagCardPtr card = component_get_card(component);
    AttributePtr attribute = NULL;

    if (1 == valid_component(component))
    {
        attribute = set_retrieve(component->mAttributes, index);
        return attribute;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return NULL;
}


void
component_get_status_header(ComponentPtr component, char* buffer, int buflen)
{
    assert(NULL != buffer);
    assert(0 != buflen);

    if (1 == valid_component(component))
    {
        /* FIXME: unimplemented. */
    }
}


void
component_get_status_data(ComponentPtr component, char* buffer, int buflen)
{
    assert(NULL != buffer);
    assert(0 != buflen);
    
    if (1 == valid_component(component))
    {
        /* FIXME: unimplemented. */
    }
}


void
component_set_dispose_routine(ComponentPtr component, ComponentDisposeRoutine routine)
{
	DagCardPtr card = component_get_card(component);
    assert(NULL != routine);
    
    if (1 == valid_component(component))
    {
        component->fDispose = routine;
        component->mChecksum = compute_component_checksum(component);
        
        /* Make sure nothing has broken. */
        (void) valid_component(component);
		return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}


void
component_set_post_initialize_routine(ComponentPtr component, ComponentPostInitializeRoutine routine)
{
	DagCardPtr card = component_get_card(component);
    assert(NULL != routine);
    
    if (1 == valid_component(component))
    {
        component->fPostInitialize = routine;
        component->mChecksum = compute_component_checksum(component);
        
        /* Make sure nothing has broken. */
        (void) valid_component(component);
		return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}


void
component_set_reset_routine(ComponentPtr component, ComponentResetRoutine routine)
{
	DagCardPtr card = component_get_card(component);
    assert(NULL != routine);
    
    if (1 == valid_component(component))
    {
        component->fReset = routine;
        component->mChecksum = compute_component_checksum(component);
        
        /* Make sure nothing has broken. */
        (void) valid_component(component);
		return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}


void
component_set_default_routine(ComponentPtr component, ComponentDefaultRoutine routine)
{
	DagCardPtr card = component_get_card(component);
    assert(NULL != routine);
    
    if (1 == valid_component(component))
    {
        component->fDefault = routine;
        component->mChecksum = compute_component_checksum(component);
        
        /* Make sure nothing has broken. */
        (void) valid_component(component);
		return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}


int
valid_component(ComponentPtr component)
{
    uint32_t checksum;
    
    assert(NULL != component);
    assert(COMPONENT_MAGIC == component->mMagicNumber);
    
    checksum = compute_component_checksum(component);
    assert(checksum == component->mChecksum);
    
    assert(NULL != component->mSubcomponents);
    assert(NULL != component->mAttributes);
    assert(NULL != component->fDispose);
    assert(NULL != component->fPostInitialize);
    assert(NULL != component->fReset);
    assert(NULL != component->fDefault);

    if ((NULL != component) && (COMPONENT_MAGIC == component->mMagicNumber) && (checksum == component->mChecksum))
    {
        return 1;
    }
    return 0;
}

const char*
component_get_description(ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        return component->mDescription;
    }
	card_set_last_error(card, kDagErrInvalidComponent);

    return NULL;
}

const char*
component_get_name(ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        return component->mName;
    }
	card_set_last_error(card, kDagErrInvalidComponent);

    return NULL;
}

void
component_set_name(ComponentPtr component, const char* value)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        if (component->mName)
            free(component->mName);

        component->mName = (char*)malloc(strlen(value) + 1);
        strcpy(component->mName, value);

        component->mChecksum = compute_component_checksum(component);
		return;
    }
	card_set_last_error(card, kDagErrNoSuchComponent);
}

void
component_set_update_register_base_routine(ComponentPtr component, ComponentUpdateRegisterBaseRoutine routine)
{
    if (1 == valid_component(component))
    {
        component->fUpdateRegisterBase = routine;
        component->mChecksum = compute_component_checksum(component);
    }
}

void
component_set_description(ComponentPtr component, const char* value)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        if (component->mDescription)
            free(component->mDescription);

        component->mDescription = (char*)malloc(strlen(value) + 1);
        strcpy(component->mDescription, value);

        component->mChecksum = compute_component_checksum(component);
		return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}

void
component_dispose_attribute(ComponentPtr component, AttributePtr attribute)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        /* delete from the set */
        set_delete(component->mAttributes, attribute);
        attribute_dispose(attribute);
		return;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
}

int
component_has_attributes(ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
    if (1 == valid_component(component))
    {
        return component_get_attribute_count(component) > 0;
    }
	card_set_last_error(card, kDagErrInvalidComponent);
    return 0;
}

dag_err_t
component_update_register_base(ComponentPtr component)
{
    dag_err_t error = kDagErrNone;
    if (1 == valid_component(component))
    {
        if (component->fUpdateRegisterBase != NULL)
        {
            error = component->fUpdateRegisterBase(component);
            if (error != kDagErrNone)
                return error;
        }
        
        /* do the subcomponents */
        if (component->mSubcomponents != NULL)
        {
            int count = set_get_count(component->mSubcomponents);
            int index;
            
            for (index = 0; index < count; index++)
            {
                ComponentPtr subcomp = (ComponentPtr) set_retrieve(component->mSubcomponents, index);
                error = component_update_register_base(subcomp);
                if (error != kDagErrNone)
                    return error;
            }
        }
    }
    else
    {
        error = kDagErrInvalidParameter;
    }
    return error;
}

