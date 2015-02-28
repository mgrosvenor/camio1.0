/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
*/


/* Public API headers. */

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/components/pbm_component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/xgmii_component.h"
#include "../include/components/xgmii_statistics_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"

#include "../../../include/dag_platform.h"
#include "../../../include/dagutil.h"

#define BUFFER_SIZE 1024

typedef struct
{
    int mIndex;
} xgmii_state_t;


/* port component. */
static void xgmii_dispose(ComponentPtr component);
static void xgmii_reset(ComponentPtr component);
static void xgmii_default(ComponentPtr component);
static int xgmii_post_initialize(ComponentPtr component);

static void network_mode_to_string_routine(AttributePtr attribute);
static void network_mode_from_string_routine(AttributePtr attribute, const char* string);
static void* network_mode_get_value(AttributePtr attribute);
static void network_mode_set_value(AttributePtr attribute, void* value, int length);

typedef enum
{
    kConfig        = 0x00,
    kMac0          = 0x04,
    kMac1          = 0x08,
}  xgmii_register_offset_t;

/**
 * XGMII's attribute definition array 
 */
Attribute_t xgmii_attr[]=
{

    {
        /* Name */                 "tx_crc",
        /* Attribute Code */       kBooleanAttributeTxCrc,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Enable or disable CRC appending onto transmitted frames.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    17,
        /* Register Address */     DAG_REG_XGMII,
        /* Offset */               kConfig,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT17,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "crcstrip",
        /* Attribute Code */       kBooleanAttributeCrcStrip,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Enable or disable CRC stripping from received frames.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    10,
        /* Register Address */     DAG_REG_XGMII,
        /* Offset */               kConfig,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT10,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};

Attribute_t xgmii_network_mode_dummy_attribute[]= 
{
    {
        /* Name */                 "network_mode",			// A dummy attribute that always returns Ethernet mode
        /* Attribute Code */       kUint32AttributeNetworkMode,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "POS, ATM, Raw or Ethernet mode if supported",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,			// Don't care
        /* Register Address */     DAG_REG_XGMII,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,	// Don't care
        /* Write */                grw_iom_write,	// Don't care
        /* Mask */                 0,			// Don't care
        /* Default Value */        kNetworkModeEth,
        /* SetValue */             network_mode_set_value,
        /* GetValue */             network_mode_get_value,
        /* SetToString */          network_mode_to_string_routine,
        /* SetFromString */        network_mode_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};
/* Number of elements in array */
#define NB_ELEM (sizeof(xgmii_attr) / sizeof(Attribute_t))

ComponentPtr
xgmii_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentXGMII, card);

    if (NULL != result)
    {
        xgmii_state_t* state = (xgmii_state_t*)malloc(sizeof(xgmii_state_t));
        component_set_dispose_routine(result, xgmii_dispose);
        component_set_reset_routine(result, xgmii_reset);
        component_set_default_routine(result, xgmii_default);
        component_set_post_initialize_routine(result, xgmii_post_initialize);
        component_set_name(result, "xgmii");
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;
}

static void
xgmii_dispose(ComponentPtr component)
{
}

static void
xgmii_reset(ComponentPtr component)
{
}

static void
xgmii_default(ComponentPtr component)
{
    AttributePtr attribute;
    void* val;

    //sets the default values 
    if (1 == valid_component(component))
    {
		attribute = component_get_attribute(component, kBooleanAttributeTxCrc); 
		val = (void *)1;
		attribute_set_value(attribute, &val, 1);

		attribute = component_get_attribute(component, kBooleanAttributeCrcStrip); 
		val = (void *)0;
		attribute_set_value(attribute, &val, 1);

/*	int count = component_get_attribute_count(component);
        int index;
        
        for (index = 0; index < count; index++)
        {
           AttributePtr attribute = component_get_indexed_attribute(component, index); 
           void* val = attribute_get_default_value(attribute);
           attribute_set_value(attribute, &val, 1);
        }
	*/
    }	

}

static int
xgmii_post_initialize(ComponentPtr component)
{
	uintptr_t address = 0;
	uint32_t reg_val = 0;
	DagCardPtr card = NULL;
	xgmii_state_t* state = component_get_private_state(component);
	AttributePtr attr;
	GenericReadWritePtr grw = NULL;
        uint32_t mask = 0;
	int reg_version;

	assert(state);
	card = component_get_card(component);
	address = card_get_register_address(card, DAG_REG_XGMII, state->mIndex);
	reg_val = card_read_iom(card, address);
	if ((reg_val & BIT0) == BIT0)
	{
		/* we have a stats module */
		dagutil_verbose_level(4, "Loading xgmii statistics component\n");
		component_add_subcomponent(component, xgmii_statistics_get_new_component(card, state->mIndex));
	}

	read_attr_array(component, xgmii_attr, NB_ELEM, state->mIndex);

	/* reverse logic for the tx_crc bit */
	attr = component_get_attribute(component, kBooleanAttributeTxCrc);
	grw = attribute_get_generic_read_write_object(attr);
	assert(grw);
	grw_set_on_operation(grw, kGRWClearBit);

	/* reverse logic for the crcstrip bit */
	attr = component_get_attribute(component, kBooleanAttributeCrcStrip);
	grw = attribute_get_generic_read_write_object(attr);
	assert(grw);
	grw_set_on_operation(grw, kGRWClearBit);

	/* Add Ethernet MAC Address */
	reg_version = card_get_register_version(card,DAG_REG_ROM, 0); 
    	if(reg_version == -1)
	         dagutil_error("not able to find the register \n");	
    	/* Add Ethernet MAC Address */
    	address = 8 + (state->mIndex*8); /* First port */
    	

	address = 8 + (state->mIndex*8); /* First port */
	grw = grw_init(card, address, grw_rom_read, NULL);
	attr = attribute_factory_make_attribute(kStringAttributeEthernetMACAddress, grw, &mask, 1);
	component_add_attribute(component, attr);

    /* Add the dummy 'network mode' attribute here */
    /* check for any 'sonet pp' component.*/
    if ( card_get_register_count(card, DAG_REG_SONET_PP) <= 0 )
    {
         /*Work-around for wan supported images. Add dummy network mode only if no 'sonet pp' exist*/
        read_attr_array(component, xgmii_network_mode_dummy_attribute, 1/*only one attrib*/, state->mIndex);
    }
	return 1;
}

static void
network_mode_to_string_routine(AttributePtr attribute)
{
    network_mode_t nm = *(network_mode_t*)attribute_get_value(attribute);
    if (nm != kNetworkModeInvalid && 1 == valid_attribute(attribute))
    {
        const char* temp = NULL;
        temp = network_mode_to_string(nm);
        attribute_set_to_string(attribute, temp);
    }
}

static void
network_mode_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute) && string != NULL)
    {
        network_mode_t nm = string_to_network_mode(string);
        network_mode_set_value(attribute, (void*)&nm, sizeof(nm));
    }
}

static void*
network_mode_get_value(AttributePtr attribute)
{
   network_mode_t mode = kNetworkModeEth;

   if (1 == valid_attribute(attribute))
   {
	attribute_set_value_array(attribute, &mode, sizeof(mode));
	return (void *)attribute_get_value_array(attribute);
   }
   return NULL;
}


static void
network_mode_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
	// Do nothing
    }
}


