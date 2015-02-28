/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * This file contains the implementation of dsm component. All the dsm functionalities are added in libdagdsm.
 * $Id: dsm_component.c,v 1.5.16.1 2009/09/08 00:34:45 jomi.gregory Exp $
 */


/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../../../include/dag_attribute_codes.h"
#include "../include/create_attribute.h"
#include "../include/components/dsm_component.h"

#include "dagutil.h"
#include "dagapi.h"
/* DSM headers */
#include "dagdsm.h"
#include "dagdsm/dsm_types.h"

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
    uintptr_t mBase;
    uint32_t mVersion;
	uint32_t mIndex;
} dsm_state_t;

/* steering component. */
static int dsm_post_initialize(ComponentPtr component);

/* steering value */

static void* dsm_bypass_get_value(AttributePtr attribute);
static void dsm_bypass_set_value(AttributePtr attribute, void* value, int len);
static void dsm_default(ComponentPtr component);
static void dsm_reset(ComponentPtr component);

Attribute_t dsm_attr[] = 
{
    {
        /* Name */                 "dsm_bypass",
        /* Attribute Code */       kBooleanAttributeDSMBypass,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "To enable/disable DSM functionality",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_HMM,
        /* Offset */               0,/*Dont care */
        /* Size/length */          1,/*Dont care */
        /* Read */                 grw_iom_read, /*Dont care */
        /* Write */                grw_iom_write,/*Dont care */
        /* Mask */                 BIT0, /*Dont care */
        /* Default Value */        1,
        /* SetValue */             dsm_bypass_set_value,
        /* GetValue */             dsm_bypass_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};


/* Number of elements in array */
#define NB_ELEM (sizeof(dsm_attr) / sizeof(Attribute_t))

static void
dsm_bypass_set_value(AttributePtr attribute, void* val, int len)
{
    DagCardPtr card;
    ComponentPtr component = NULL;
    uint8_t value = *(uint8_t*)val;
    dsm_state_t* state = NULL;
    
    if (1 == valid_attribute(attribute))
    {
        volatile uintptr_t  base_reg;
        uint32_t           csr_value;

        card = attribute_get_card(attribute);
        if ( NULL == card )
            return ;
        component = attribute_get_component( attribute);
        if ( NULL == component )
            return;
        state = component_get_private_state(component);
        if ( NULL == state ) 
            return ;
        /* get a pointer to the base register */
        base_reg =state->mBase;
        
        /* Enable or disable the DSM module */
        csr_value = dag_readl ((uintptr_t)card_get_iom_address(card) + base_reg + DSM_REG_CSR_OFFSET);
//         dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
        
        
        if ( value == 0 )
            csr_value &= ~BIT31;
        else
            csr_value |=  BIT31;
        
        dag_writel (csr_value, ((uintptr_t)card_get_iom_address(card) + base_reg + DSM_REG_CSR_OFFSET));
//         dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
        
    }
}

static void*
dsm_bypass_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    uint8_t val = 0;
    ComponentPtr component = NULL;
    dsm_state_t* state = NULL;

  if (1 == valid_attribute(attribute))
    {
        volatile uintptr_t  base_reg;
        uint32_t           csr_value;
        
        card = attribute_get_card(attribute);
        if ( NULL == card )
            return NULL;
        component = attribute_get_component( attribute);
        if ( NULL == component )
            return NULL;

        state = component_get_private_state(component);
        if ( NULL == state ) 
            return NULL ;
        /* get a pointer to the base register */
        base_reg =state->mBase;
        
        /* Enable or disable the DSM module */
        csr_value = dag_readl ((uintptr_t)card_get_iom_address(card) + base_reg + DSM_REG_CSR_OFFSET);
      //  dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
        
        
        if ( csr_value & BIT31)
            val = 1;
        else
            val = 0;
        
    }
    attribute_set_value_array(attribute, &val, sizeof(val));
    return (void *)attribute_get_value_array(attribute);
}

ComponentPtr
dsm_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentDSM, card); 
    
    if (NULL != result)
    {
		dsm_state_t* state = NULL;
        component_set_post_initialize_routine(result, dsm_post_initialize);
        component_set_name(result, "dsm");
		component_set_description(result, "DSM module");
    	state = (dsm_state_t*)malloc(sizeof(dsm_state_t));
		state->mIndex = index;
        state->mBase = card_get_register_address(card, DAG_REG_HMM, state->mIndex);
    	component_set_private_state(result, state);
        component_set_default_routine(result, dsm_default);
        component_set_reset_routine(result, dsm_reset);
    }
    
    return result;
}


static int
dsm_post_initialize(ComponentPtr component)
{
    dsm_state_t* state = NULL;
    DagCardPtr card = NULL;
    
    card = component_get_card(component);
	state = component_get_private_state(component);
    state->mBase = card_get_register_address(card, DAG_REG_HMM, 0);
    state->mVersion = card_get_register_version(card, DAG_REG_HMM, 0);

    read_attr_array(component, dsm_attr, NB_ELEM, state->mIndex);

    return 1;
}

static void
dsm_default(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        int count = component_get_attribute_count(component);
        int index;
            
        for (index = 0; index < count; index++)
        {
            AttributePtr attribute = component_get_indexed_attribute(component, index); 
            void* val = attribute_get_default_value(attribute);
            attribute_set_value(attribute, &val, 1);
        }
}    
}

static void
dsm_reset(ComponentPtr component)
{   
    if (1 == valid_component(component))
    {
        /* assuming same as default */
        dsm_default(component);
    }    
}
