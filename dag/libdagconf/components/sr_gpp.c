/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: sr_gpp.c,v 1.21 2008/11/20 07:35:49 jomi Exp $
 */

/* 
The size reduced gpp.
*/

/* Endace headers. */
#include "dagutil.h"
#include "dag_platform.h"

/* Configuration API headers. */
#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/attribute_factory.h"
#include "../include/components/mini_mac_statistics_component.h"
#include "../include/modules/generic_read_write.h"
#include "../include/create_attribute.h"
#include "../include/components/sr_gpp.h"

static void sr_gpp_dispose(ComponentPtr component);
static void sr_gpp_reset(ComponentPtr component);
static void sr_gpp_default(ComponentPtr component);
static int sr_gpp_post_initialize(ComponentPtr component);

static AttributePtr sr_gpp_get_snaplen_attribute(void);
static void* sr_gpp_get_snaplen_value(AttributePtr attribute);
static void sr_gpp_set_snaplen_value(AttributePtr attribute, void* value, int len);

static void sr_gpp_set_varlen_value(AttributePtr attribute, void* value, int len);
void sr_gpp_create_drop_count_attributes( ComponentPtr component,int count );
void sr_gpp_create_port_enable_attributes(ComponentPtr component, int count );

/* functions for dummy align64 attribute */
static void sr_gpp_dummy_align64_set_value(AttributePtr attribute, void* value, int len);
static void* sr_gpp_dummy_align64_get_value(AttributePtr attribute);


Attribute_t sr_gpp_attr[] = 
{
    {
        /* Name */                 "interface_count",
        /* Attribute Code */       kUint32AttributeInterfaceCount,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of interfaces in the card.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    28,
        /* Register Address */     DAG_REG_SRGPP,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 (BIT28 | BIT29 | BIT30),
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value, 
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }, 
    {
        /* Name */                 "align64",
        /* Attribute Code */       kBooleanAttributeAlign64,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Align/pad the received ERF record to the next 64-bit boundary.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,/* dont care  - dummy attribute*/
        /* Register Address */     DAG_REG_SRGPP,
        /* Offset */               0,/*dont care - dummy attribute */
        /* Size/length */          1,
        /* Read */                 NULL,/* No read/write - dummy attribute */
        /* Write */                NULL,/* No read/write - dummy attribute */
        /* Mask */                 0,/*dont care - dummy attribute */
        /* Default Value */        1,
        /* SetValue */             sr_gpp_dummy_align64_set_value,
        /* GetValue */             sr_gpp_dummy_align64_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }, 

};

#define NB_ELEM (sizeof(sr_gpp_attr) / sizeof(Attribute_t))

ComponentPtr
sr_gpp_get_new_component(DagCardPtr card, uint32_t index)
{
	ComponentPtr result = component_init(kComponentSRGPP, card); 
	
	if (NULL != result)
	{
		component_set_dispose_routine(result, sr_gpp_dispose);
		component_set_post_initialize_routine(result, sr_gpp_post_initialize);
		component_set_reset_routine(result, sr_gpp_reset);
		component_set_default_routine(result, sr_gpp_default);
		component_set_name(result, "gpp");
		component_set_description(result, "The size reduced gpp.");
		return result;
	}
	return result;
}

static void
sr_gpp_dispose(ComponentPtr component)
{
}

static void
sr_gpp_reset(ComponentPtr component)
{
}

static void
sr_gpp_default(ComponentPtr component)
{
	AttributePtr slen = NULL;
	uint32_t slen_val = 10240;

	if (1 == valid_component(component))
	{
		slen = component_get_attribute(component, kUint32AttributeSnaplength);
		sr_gpp_set_snaplen_value(slen, &slen_val, 1);
	}

}

static int
sr_gpp_post_initialize(ComponentPtr component)
{
	DagCardPtr card;
	AttributePtr attr;
    uint32_t *ptr_count = NULL;
    AttributePtr interface_count = NULL;

	card = component_get_card(component);

	attr = sr_gpp_get_snaplen_attribute();
	component_add_attribute(component, attr);

    if (card_get_register_version(card, DAG_REG_SRGPP, 0) == 1)
    {
        GenericReadWritePtr grw;
        uint32_t mask;
        uintptr_t address;

        mask = BIT26;
        address = (uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_SRGPP, 0);
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeVarlen, grw, &mask, 1);
        attribute_set_setvalue_routine(attr, sr_gpp_set_varlen_value);
        component_add_attribute(component, attr);
    }
    /* create and add attributes defined in sr_gpp_attr */
    read_attr_array(component, sr_gpp_attr, NB_ELEM, /*state->mIndex*/ 0);

    /* Add drop count attributes here-depending upon the interfce count*/
    interface_count = component_get_attribute(component, kUint32AttributeInterfaceCount);
    if ( NULL != (ptr_count = (uint32_t*)attribute_uint32_get_value(interface_count) ) && *ptr_count )
    {
        sr_gpp_create_drop_count_attributes( component, *ptr_count );
    }
    if ( NULL != (ptr_count = (uint32_t*)attribute_uint32_get_value(interface_count) ) && *ptr_count )
    {
        sr_gpp_create_port_enable_attributes( component, *ptr_count );
    }
	return 1;
}

static AttributePtr
sr_gpp_get_snaplen_attribute(void)
{
	AttributePtr result = attribute_init(kUint32AttributeSnaplength); 
	
	if (NULL != result)
	{
		attribute_set_name(result, "snap_length");
		attribute_set_description(result, "Get/set the snaplength. Accepts any value, but the value will be rounded to a multiple of the ERF record alignment.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_getvalue_routine(result, sr_gpp_get_snaplen_value);
		attribute_set_setvalue_routine(result, sr_gpp_set_snaplen_value);
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
		attribute_set_from_string_routine(result, attribute_uint32_from_string);
	}
	
	return result;

}

static void*
sr_gpp_get_snaplen_value(AttributePtr attribute)
{
	static uint32_t value = 0;
	if (1 == valid_attribute(attribute))
	{
		DagCardPtr card;
		uintptr_t address;
		card = attribute_get_card(attribute);
		address = card_get_register_address(card, DAG_REG_SRGPP, 0); 
		value = card_read_iom(card, address);
		value &= 0xffff;
	}
	return (void*)&value;
}

static void
sr_gpp_set_snaplen_value(AttributePtr attribute, void* value, int len)
{
	if (1 == valid_attribute(attribute))
	{
		DagCardPtr card;
		uintptr_t address;
		uint32_t reg_val = 0;
		uint32_t stream_count = 0;
		int i = 0;

		assert(value);
		card = attribute_get_card(attribute);
		address = card_get_register_address(card, DAG_REG_SRGPP, 0); 
		stream_count = (card_read_iom(card, address) & 0x70000000) >> 28;
		for (i = 0; i < stream_count; i++)
		{
			reg_val = card_read_iom(card, address + (i*8));
			reg_val &= ~0xffff;
			reg_val |= (*(uint32_t*)value & 0xffff);
			card_write_iom(card, address + (i*8), reg_val);
		}
	}
}

static void
sr_gpp_set_varlen_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card;
        uintptr_t address;
        uintptr_t base_address;
        uint32_t stream_count = 0;
        int i = 0;
        GenericReadWritePtr grw;

        assert(value);
        card = attribute_get_card(attribute);
        address = card_get_register_address(card, DAG_REG_SRGPP, 0); 
        stream_count = (card_read_iom(card, address) & 0x70000000) >> 28;
        grw = attribute_get_generic_read_write_object(attribute);
        base_address = grw_get_address(grw);
        address = base_address;
        for (i = 0; i < stream_count; i++)
        {
            attribute_boolean_set_value(attribute, value, len);
            address += kSRGPPStride;
            grw_set_address(grw, address);
        }
        /* set it back otherwise the next we have to start from the base address again */
        grw_set_address(grw, base_address);
    }
}

void 
sr_gpp_create_drop_count_attributes( ComponentPtr component,int count )
{
    AttributePtr result = NULL;
    char name[32] ;
    uintptr_t offset = 0;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t mask = 0xffffffff;
    int length = 1;
    uint32_t num_module = 0;
    /*sr_gpp_state_t *state = NULL;*/
    uint32_t index = 0;

    for ( index = 0; index < count; index++)
    {
    
        /*state = component_get_private_state(component); 
        num_module = state->mIndex;*/
        sprintf(name,"drop_count%d",index);
        
        card = component_get_card(component);
        offset =   index * kSRGPPStride + kSRGPPCounter;
        base_reg = card_get_register_address(card, DAG_REG_SRGPP, num_module);
        address = ((uintptr_t)card_get_iom_address(card) + base_reg  + offset);
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        result = attribute_init(kUint32AttributeDropCount);
        grw_set_attribute(grw, result);
        attribute_set_generic_read_write_object(result, grw);

        attribute_set_name(result, name);
        attribute_set_description(result, "A count of the packets dropped on a port");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_getvalue_routine(result, attribute_uint32_get_value);
        attribute_set_setvalue_routine(result, attribute_uint32_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_masks( result, &mask, length );
   
        component_add_attribute(component,result);
    }
}

void 
sr_gpp_create_port_enable_attributes(ComponentPtr component,int count)
{
    AttributePtr result = NULL;
    char name[256];
    uintptr_t offset = 0;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t mask = BIT27;
    int length = 1;
    uint32_t num_module = 0;
    //sr_gpp_state_t* state = NULL;
    uint32_t index = 0;

    for(index = 0;index < count;index++)
    {
        /*state = component_get_private_state(component);
        num_module = state->mIndex;*/
        sprintf(name,"Enable/Disable Port %d",index);

        card = component_get_card(component);
        offset = (index) * kSRGPPStride;
        base_reg = card_get_register_address(card,DAG_REG_SRGPP,num_module);
        address = ((uintptr_t)card_get_iom_address(card) + base_reg + offset);
        grw = grw_init(card,address,grw_iom_read,grw_iom_write); 
        grw_set_on_operation(grw,kGRWClearBit);
        result = attribute_factory_make_attribute(kBooleanAttributeActive,grw,&mask,length);
      	sprintf(name,"active%d",index);
        attribute_set_name(result, name);
      	sprintf(name,"Enable/Disable Port%d",index);
        attribute_set_description(result, name);
	
        component_add_attribute(component,result);
    }
}
void sr_gpp_dummy_align64_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        dagutil_verbose_level(3, "Setting %s to 1\n", attribute_get_name(attribute));
    }
}

void* sr_gpp_dummy_align64_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        uint8_t value = 1;
        attribute_set_value_array(attribute, &value, sizeof(value));
        dagutil_verbose_level(3, "%s is %d\n", attribute_get_name(attribute), ((uint8_t*)attribute_get_value_array( attribute ))[0]);
        return ( attribute_get_value_array(attribute) );
    }
    return NULL;
}
