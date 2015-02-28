/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *
 * This component is used by DAG 5.0S (OC-48) and DAG8.0SX (OC-192) as well.
 *
 * $Id: counter_component.c,v 1.8 2008/04/07 23:34:15 vladimir Exp $
 */

#include "dagutil.h"

#include "../include/attribute.h"
#include "../include/util/set.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/counter_component.h"
#include "../include/components/counters_interface_component.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"


/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: counter_component.c,v 1.8 2008/04/07 23:34:15 vladimir Exp $";
static const char* const kRevisionString = "$Revision: 1.8 $";

static void counter_dispose(ComponentPtr component);
static void counter_reset(ComponentPtr component);
static void counter_default(ComponentPtr component);
static int counter_post_initialize(ComponentPtr component);
static dag_err_t counter_update_register_base(ComponentPtr component);
static void* counter_descr_attribute_get_value(AttributePtr attribute);
static void* counter_value_get_value(AttributePtr attribute);
	
#define OFFSET_FIELD 0x04
/**
 * Counter's Attribute definition array 
 */
Attribute_t counter_attr[]=
{

    {
        /* Name */                 "counter_ID",
        /* Attribute Code */       kUint32AttributeCounterID,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Counter type ID",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    16,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFF0000,
        /* Default Value */        1,
        /* SetValue */             attribute_uint32_set_value,
		/* GetValue */             counter_descr_attribute_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "sub_function",
        /* Attribute Code */       kUint32AttributeSubFunction,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Counter Sub-function",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    5,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT5 | BIT6 | BIT7 | BIT8 | BIT9 | BIT10 | BIT11 | BIT12,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             counter_descr_attribute_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "sub_function_type",
        /* Attribute Code */       kUint32AttributeSubFunctionType,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Counter Sub-function type",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    5,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT13 | BIT14 | BIT15,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             counter_descr_attribute_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	
    {
        /* Name */                 "counter_value_type",
        /* Attribute Code */       kBooleanAttributeValueType,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Counter value type (address or value)",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    4,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0,
        /* Default Value */        0x10,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             counter_descr_attribute_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	
	{
        /* Name */                 "counter_latch_clear",
        /* Attribute Code */       kBooleanAttributeLatchClear,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Counter Latch & Clear",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    3,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x8,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             counter_descr_attribute_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	
    {
        /* Name */                 "size",
        /* Attribute Code */       kUint32AttributeCounterSize,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Counter size",
        /* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x7,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             counter_descr_attribute_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	
	{
        /* Name */                 "access_type",
        /* Attribute Code */       kBooleanAttributeAccess,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Access type (Direct -0- or Indirect -1-)",
        /* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x7,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             counter_descr_attribute_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	
	{
        /* Name */                 "counter_value",
        /* Attribute Code */       kUint64AttributeValue,
        /* Attribute Type */       kAttributeUint64,
        /* Description */          "Value of counter",
        /* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x7,
        /* Default Value */        0,
        /* SetValue */             attribute_uint64_set_value,
        /* GetValue */             counter_value_get_value,
        /* SetToString */          attribute_uint64_to_string,
        /* SetFromString */        attribute_uint64_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};

/* Number of elements in array */
#define NB_ELEM (sizeof(counter_attr) / sizeof(Attribute_t))

/* counter component. */
ComponentPtr
counter_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentCounter, card); 
    counter_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, counter_dispose);
        component_set_post_initialize_routine(result, counter_post_initialize);
        component_set_reset_routine(result, counter_reset);
        component_set_default_routine(result, counter_default);
        component_set_update_register_base_routine(result, counter_update_register_base);
        component_set_name(result, "counter");
        state = (counter_state_t*)malloc(sizeof(counter_state_t));
        state->mIndex = index; 
		//state->mBase = card_get_register_address(card, DAG_REG_COUNT_INTERF, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
counter_dispose(ComponentPtr component)
{
}

static dag_err_t
counter_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        counter_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        //state->mBase = card_get_register_address(card, DAG_REG_COUNT_INTERF, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
counter_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        DagCardPtr card = NULL;
		counter_state_t* counter_state = NULL;		
		
		/* Get card reference */
		card = component_get_card(component);
        
		/* Get counter state structure */
		counter_state = component_get_private_state(component);
        
		/* Add attribute of counter */ 
        read_attr_array(component, counter_attr, NB_ELEM, counter_state->mIndex);
	return 1;
     }
    return kDagErrInvalidParameter;
  
}

static void
counter_reset(ComponentPtr component)
{   
    uint8_t val = 0;
    if (1 == valid_component(component))
    {
		AttributePtr attribute = component_get_attribute(component, kBooleanAttributeReset);
		attribute_set_value(attribute, (void*)&val, 1);
		counter_default(component);
    }	
}


static void
counter_default(ComponentPtr component)
{

    //set all the attribute default values
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

static void*
counter_descr_attribute_get_value(AttributePtr attribute)
{
	if (1 == valid_attribute(attribute))
	{	
		DagCardPtr card = NULL;
		counter_state_t* counter_state = NULL;
		counters_interface_state_t* csi_state = NULL;
		ComponentPtr c_block = NULL, c_counter = NULL;
		uint32_t *masks, mask = 0;
		uint32_t reg_count_desc = 0;
		int version;
		
		uint32_t* ind_addr_field =  NULL;
		uint32_t* ind_data_reg =  NULL;

		/* Get card reference */
		card = attribute_get_card(attribute);

		/* Get the counter component */
		c_counter = attribute_get_component(attribute);
        counter_state = component_get_private_state(c_counter);
		
		/* Get CSI block component */
		c_block = component_get_parent(c_counter);
		csi_state = component_get_private_state(c_block);

		/* Get the version of the component */
		version = card_get_register_version(card, DAG_REG_COUNT_INTERF, csi_state->mIndex);
		
		/* Address of the card, DRB, Indirect addressing field and Indirect data register */
		ind_addr_field = (uint32_t*)((uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_COUNT_INTERF, csi_state->mIndex) + INDIRECT_ADDR_OFFSET);
		ind_data_reg = (uint32_t*)((uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_COUNT_INTERF, csi_state->mIndex) + INDIRECT_DATA_OFFSET);

		masks = attribute_get_masks(attribute);
		mask = masks[0];
		
		switch(version)
		{
			/* Direct register */
			case 0:
			{
				reg_count_desc =  *(counter_state->mDescrAddress);
				break;
			}	
			case 1:
			{
										
				/* To retrieve the data from the counter description field
				 * write description base address in the Indirect addressing field */
				*ind_addr_field = counter_state->mDescrOffset;
				/* read data in the indirect data register */
				reg_count_desc = (uint32_t)*(ind_data_reg);
				break;
			}
				
		}
		reg_count_desc &= mask;
		while (!(mask & 1))
		{
           mask >>= 1;
           reg_count_desc >>= 1;
		}	
		attribute_set_value_array(attribute, &reg_count_desc, sizeof(reg_count_desc));
        return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

static void*
counter_value_get_value(AttributePtr attribute)
{
	if (1 == valid_attribute(attribute))
	{	
		DagCardPtr card = NULL;
		counter_state_t* counter_state = NULL;
		counters_interface_state_t* csi_state = NULL;
		ComponentPtr c_block = NULL, c_counter = NULL;
		uint32_t reg_count_desc = 0;
		int version;
		
		uint32_t* ind_addr_field =  NULL;
		uint32_t* ind_data_reg =  NULL;
		AttributePtr attr = NULL;
		dag_counter_value_t counter;

		uint32_t nb_counters = 0;
		uint64_t value = 0, temp = 0;

		/* Get card reference */
		card = attribute_get_card(attribute);

		/* Get the counter component */
		c_counter = attribute_get_component(attribute);
        counter_state = component_get_private_state(c_counter);
		
		/* Get CSI block component */
		c_block = component_get_parent(c_counter);
		csi_state = component_get_private_state(c_block);

		/* Get the version of the csi component */
		version = card_get_register_version(card, DAG_REG_COUNT_INTERF, csi_state->mIndex);
		
		/* get the number of counters */
		attr = component_get_attribute(c_block, kUint32AttributeNbCounters);
		nb_counters = *((uint32_t*)attribute_get_value(attr));

		/* Address of the Indirect addressing field and Indirect data register */
		ind_addr_field = (uint32_t*)((uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_COUNT_INTERF, csi_state->mIndex) + INDIRECT_ADDR_OFFSET);
		ind_data_reg = (uint32_t*)((uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_COUNT_INTERF, csi_state->mIndex) + INDIRECT_DATA_OFFSET);
	
		switch(version)
		{
			/* Direct register */
			case 0:
			{
				reg_count_desc =  *(counter_state->mDescrAddress);
				
				/* Read size (32 or 64 bits) info */
				counter.size = (uint8_t)(reg_count_desc & 0x07);
			
				/* Check data size 32 or 64 bits */
				if (counter.size == 1)
				{
					temp = (uint64_t)*(counter_state->mValueAddress);
					value = (uint64_t)*((counter_state->mValueAddress)+1);
				}
				else
				{
					value = (uint64_t)*(counter_state->mValueAddress);
					temp = 0;
				}
				break;
			}
			case 1:
			{
				/* To retrieve the data from the counter description field
				 * write description base address in the Indirect addressing field */
				*ind_addr_field = counter_state->mDescrOffset;
				/* read data in the indirect data register */
				reg_count_desc = (uint32_t)*(ind_data_reg);
					
				/* Read counter ID, size (32 or 64 bits) and Latch and clear info */
				counter.size = (uint8_t)(reg_count_desc & 0x07);
				
				/* To retrieve the value of the counters 
				 * write value base address in the indirect addressing field */
				*ind_addr_field = counter_state->mValueOffset;
											
				/* Check data size 32 or 64 bits */
				if (counter.size == 1)
				{
					temp = *((uint32_t*)(ind_data_reg));
					*ind_addr_field = counter_state->mValueOffset + 1;
					value = *((uint32_t*)(ind_data_reg));
				}
				else
				{
					value = *((uint32_t*)(ind_data_reg));
					temp = 0;
				}	
				break;
			}
				
		}
		value |= (temp << 32);
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

