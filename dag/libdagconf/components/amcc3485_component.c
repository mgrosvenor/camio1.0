/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: amcc3485_component.c,v 1.34 2008/07/03 05:18:39 jomi Exp $
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
#include "../include/components/gpp_component.h"
#include "../include/components/sr_gpp.h"
#include "../include/components/dag71s_port_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"

#define OFFSET 0x1c

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: amcc3485_component.c,v 1.34 2008/07/03 05:18:39 jomi Exp $";
static const char* const kRevisionString = "$Revision: 1.34 $";
static void amcc3485_dispose(ComponentPtr component);
static void amcc3485_reset(ComponentPtr component);
static void amcc3485_default(ComponentPtr component);
static int amcc3485_post_initialize(ComponentPtr component);
static dag_err_t amcc3485_update_register_base(ComponentPtr component);

static void* amcc_master_slave_get_value(AttributePtr attribute);
static void amcc_master_slave_set_value(AttributePtr attribute, void* value, int length);
static void amcc_master_slave_to_string_routine(AttributePtr attribute);
static void amcc_master_slave_from_string_routine(AttributePtr attribute, const char* string);

#if 0 /* Moved to sonet_pp_component.c along with "network mode" attribute (bit 19) */
static void amcc_network_mode_to_string_routine(AttributePtr attribute);
static void amcc_network_mode_from_string_routine(AttributePtr attribute, const char* string);
static void* amcc_network_mode_get_value(AttributePtr attribute);
static void amcc_network_mode_set_value(AttributePtr attribute, void* value, int length);
#endif


/**
 * AMCC-3485 Attribute definition array 
 */
Attribute_t amcc3485_attr[]=
{
    {
        /* Name */                 "lock",
        /* Attribute Code */       kBooleanAttributeLock,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Lock on its reference clock",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
//TODO: To Add Bit 1, offest 0 Bist Error Status active high 
//TODO: To Add Bit 1, offset 0 status of TX clock lock  active high 
    {
//DONE: change it to LOS 
// to do change it may be to LOS and duplicated with SFP SDetect (LOS negaated ) active high????
//TODO: check for the other components it is duplicated in the chain of firmware components and to the end user to be shown only one 
        /* Name */                 "los",
        /* Attribute Code */       kBooleanAttributeLossOfSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether the there is LOS ",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    3,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT3,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                  "phy_bist_enable",
        /* Attribute Code */       kBooleanAttributePhyBistEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Enable Built-In Self Test (BIST)",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    4,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT4,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
// Reversed poliarity or active low in the component post init function 
    {
        /* Name */                 "eql",
        /* Attribute Code */       kBooleanAttributeEquipmentLoopback,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Enable or disable equipment loopback",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    6,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT6,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
//Reversed Poliarity doen in the component post init function  
    {
        /* Name */                 "fcl",
        /* Attribute Code */       kBooleanAttributeFacilityLoopback,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Enable or disable facility loopback",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    7,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT7,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
//TODO: May be to add possibility to set the bit 8 option but not accesable to the user and by default should be always 1
//Thsi is the sfp clock out tio be interanl reference or locked to the TX clock
    
//This signal is not used 
//TODO: Remove it and leave it just as setup 1 or maybe leave it for firmawre usage 
    {
        /* Name */                 "phy_tx_clock_off",
        /* Attribute Code */       kBooleanAttributePhyTxClockOff,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Clock Off Tx",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    9,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT9,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
//TODO: Remove it and setup to 1 by default 
    {
        /* Name */                 "phy_kill_rxclock",
        /* Attribute Code */       kBooleanAttributePhyKillRxClock,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Deassert to start receiving",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    10,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT10,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "reset",
        /* Attribute Code */       kBooleanAttributeReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Reset the component",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    11,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT11,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "phy_rate",
        /* Attribute Code */       kUint32AttributePhyRate,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Rate selection",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    12,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT12|BIT13|BIT14,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "phy_ref_select",
        /* Attribute Code */       kUint32AttributePhyRefSelect,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Reference selection",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    15,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT15|BIT16,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "master_or_slave",
        /* Attribute Code */       kUint32AttributeMasterSlave,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Set master or slave mode",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    17,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT17,
        /* Default Value */        0,
        /* SetValue */             amcc_master_slave_set_value,
        /* GetValue */             amcc_master_slave_get_value,
        /* SetToString */          amcc_master_slave_to_string_routine,
        /* SetFromString */        amcc_master_slave_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
//Reversed poliarity in the componenet post init function 

    {
        /* Name */                 "active",
        /* Attribute Code */       kBooleanAttributeActive,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Use to enable or disable this link/port.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    12,
        /* Register Address */     DAG_REG_GPP,
        /* Offset */               SP_CONFIG,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT12,
        /* Default Value */        1,
        /* SetValue */             gpp_active_set_value,
        /* GetValue */             gpp_active_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "config",
        /* Attribute Code */       kUint32AttributeConfig,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Main clock configuration",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               OFFSET,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0|BIT1,
        /* Default Value */        1,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "discard_data",
        /* Attribute Code */       kBooleanAttributeDiscardData,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Discard data policy",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               OFFSET,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT2,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
#if 0
    {
        /* Name */                 "laser",
        /* Attribute Code */       kBooleanAttributeLaser,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Configure the transmit laser to be on or off.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    18,
        /* Register Address */     DAG_REG_AMCC3485,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT18,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
#endif
    /* negated, see amcc3485_post_initialize */
    {
        /* Name */                 "tx_lock_error",
        /* Attribute Code */       kBooleanAttributeTransmitLockError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Transmit lock error",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_AMCC3485, /* another module!!! */
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT2,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};

/* Number of elements in array */
#define NB_ELEM (sizeof(amcc3485_attr) / sizeof(Attribute_t))

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} amcc3485_state_t;

/* terf component. */
ComponentPtr
amcc3485_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPhy, card); 
    amcc3485_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, amcc3485_dispose);
        component_set_post_initialize_routine(result, amcc3485_post_initialize);
        component_set_reset_routine(result, amcc3485_reset);
        component_set_default_routine(result, amcc3485_default);
        component_set_update_register_base_routine(result, amcc3485_update_register_base);
        component_set_name(result, "amcc3485");
        state = (amcc3485_state_t*)malloc(sizeof(amcc3485_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
amcc3485_dispose(ComponentPtr component)
{
}

static dag_err_t
amcc3485_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        amcc3485_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_AMCC3485, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
amcc3485_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        amcc3485_state_t* state = NULL;
        GenericReadWritePtr grw;
        AttributePtr attr;
	uintptr_t address;
	DagCardPtr card;
    	uint32_t gpp_base = 0;
	uint32_t mask = 0;
	
        state = component_get_private_state(component);
        /* Add attribute of amcc3485 */ 
        read_attr_array(component, amcc3485_attr, NB_ELEM, state->mIndex);
        
        /* negated bits */
	/* Transmit Lock error is active low */
        attr = component_get_attribute(component, kBooleanAttributeTransmitLockError);
        grw = attribute_get_generic_read_write_object(attr);
        grw_set_on_operation(grw, kGRWClearBit);
	
	/* FCL is active low on AMCC3485 */
        attr = component_get_attribute(component, kBooleanAttributeFacilityLoopback);
	grw = attribute_get_generic_read_write_object(attr);
	grw_set_on_operation(grw, kGRWClearBit);
	
	/* EQL is active low on AMCC3485 */
        attr = component_get_attribute(component, kBooleanAttributeEquipmentLoopback);
	grw = attribute_get_generic_read_write_object(attr);
	grw_set_on_operation(grw, kGRWClearBit);

	/* Activate is active low on AMCC3485 */
        attr = component_get_attribute(component, kBooleanAttributeActive);
	card = component_get_card (component);
	
	/* The next two 'if' statements determine whether GPP or size-reduced GPP is used despite what was set in the attribute array. This is for 5.0SG2 */
	gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
	if (gpp_base > 0) 
	{
		address = ((uintptr_t)card_get_iom_address(card)) + gpp_base + (uintptr_t)((state->mIndex + 1)*SP_OFFSET) + (uintptr_t)SP_CONFIG;
		//printf ("Warning! %s: %s - address (GPP): 0x%x\n", __FILE__, __FUNCTION__, address);
	
		mask = BIT12;
		grw = grw_init(card, address, grw_iom_read, grw_iom_write);
		grw_set_on_operation(grw, kGRWClearBit);
		attribute_set_getvalue_routine(attr, gpp_active_get_value);
		attribute_set_setvalue_routine(attr, gpp_active_set_value);

		attribute_set_generic_read_write_object(attr, grw);
		attribute_set_masks(attr, &mask, 1);
	}

 	gpp_base = card_get_register_address(card, DAG_REG_SRGPP, 0);
    	if (gpp_base > 0)
	{
		if (card_get_register_version(card, DAG_REG_SRGPP, 0) == 1)
		{
			mask = BIT27;
			address = (uintptr_t)card_get_iom_address(card) + gpp_base + (uintptr_t)(state->mIndex * kSRGPPStride);
			//printf ("Warning! %s: %s - address (SRGPP): 0x%x\n", __FILE__, __FUNCTION__, address);
			
			grw = grw_init(card, address, grw_iom_read, grw_iom_write);
			grw_set_on_operation(grw, kGRWClearBit);

			attribute_set_getvalue_routine(attr, attribute_boolean_get_value);
			attribute_set_setvalue_routine(attr, attribute_boolean_set_value);

			//printf ("Warning! %s: %s - Setting SRGPP attributes\n", __FILE__, __FUNCTION__);
			attribute_set_generic_read_write_object(attr, grw);
			attribute_set_masks(attr, &mask, 1);

		}
	}
#if 0
	/* Laser is active low on AMCC3485 */
        attr = component_get_attribute(component, kBooleanAttributeLaser);
	grw = attribute_get_generic_read_write_object(attr);
	grw_set_on_operation(grw, kGRWClearBit);
#endif
	return 1;
    }
    return kDagErrInvalidParameter;
}

static void
amcc3485_reset(ComponentPtr component)
{   
    uint8_t val = 0;
    DagCardPtr card = NULL;
    bool dpa = false;
    ComponentPtr root = NULL, dpa_comp = NULL;
    
    if (1 == valid_component(component))
    {
	AttributePtr attribute = component_get_attribute(component, kBooleanAttributeReset);
	attribute_set_value(attribute, (void*)&val, 1);
	
	/* Get the card */
        card = component_get_card(component);
	/* Get the root component */
	root = card_get_root_component(card);
	/* get dpa component if exists */
	dpa_comp = component_get_subcomponent(root, kComponentDPA, 0);
	
	if(dpa_comp != NULL)
		dpa = true;
    else
    {
        /*if dpa is not there, it sets the the error to kDagErrNoSuchComponent.
          Making it to kDagErrNone, for further config and status APIs to work properly*/
        card_set_last_error(card, kDagErrNone);
    }
	if(dpa)
	{
		amcc3485_default(component);
		/* change rate to OC48 */
		attribute = component_get_attribute(component, kUint32AttributePhyRate);
		val = 3;
		attribute_set_value(attribute, (void*)&val, 1);	
	}
	else
		amcc3485_default(component);
    }	
}


static void
amcc3485_default(ComponentPtr component)
{
    uint8_t val = 0;
    uint8_t* ptr;
    DagCardPtr card = NULL;
    bool dpa = false;
    ComponentPtr root = NULL, dpa_comp = NULL;
    AttributePtr attribute = NULL;

    //set all the attribute default values
    if (1 == valid_component(component))
    {
	int count = component_get_attribute_count(component);
        int index;
  
	/* Get the card */
        card = component_get_card(component);
	/* Get the root component */
	root = card_get_root_component(card);
	/* get dpa component if exists */
	dpa_comp = component_get_subcomponent(root, kComponentDPA, 0);
	
	if(dpa_comp != NULL)
		dpa = true;
    else
    {
        /*if dpa is not there, it sets the the error to kDagErrNoSuchComponent.
          Making it to kDagErrNone, for further config and status APIs to work properly*/
        card_set_last_error(card, kDagErrNone);
    }
    for (index = 0; index < count; index++)
    {
       attribute = component_get_indexed_attribute(component, index); 
	   ptr = attribute_get_default_value(attribute);
	   attribute_set_value(attribute, &ptr, 1);
	}
	/* if card dag5.0s oc48 */
	if(dpa)
	{
		/* set the rate to oc48 */
		attribute = component_get_attribute(component, kUint32AttributePhyRate);
		val = 3;
		attribute_set_value(attribute, (void*)&val, 1);	
	}
	
    }	
}

static void*
amcc_master_slave_get_value(AttributePtr attribute)
{
    uint32_t register_value;
    uint32_t value = 0;
    GenericReadWritePtr master_slave_grw = NULL;
    uint32_t *masks;

    if (1 == valid_attribute(attribute))
    {
        master_slave_grw = attribute_get_generic_read_write_object(attribute);
        assert(master_slave_grw);
        register_value = grw_read(master_slave_grw);
        masks = attribute_get_masks(attribute);
        register_value &= masks[0];

        if (register_value) {
            value = kSlave;
        } else {
            value = kMaster;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
     }
    return NULL;
}

static void
amcc_master_slave_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card =NULL;
    uint32_t register_value = 0;
    GenericReadWritePtr master_slave_grw = NULL;    
    master_slave_t master_slave = *(master_slave_t*)value;
    uint32_t *masks;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        master_slave_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(master_slave_grw);
        masks = attribute_get_masks(attribute);
        switch(master_slave)
        {
          case kMasterSlaveInvalid:
               /* nothing */
               break;
          case kMaster:
               register_value &= ~masks[0]; /* clear */
               break;
          case kSlave:
               register_value |= masks[0]; /* set */
               break;
        default:
            assert(0); /* should never get here */
        }
        grw_write(master_slave_grw, register_value);
    }
}

static void
amcc_master_slave_to_string_routine(AttributePtr attribute)
{
    master_slave_t value = *(master_slave_t*)amcc_master_slave_get_value(attribute);
    const char* temp = master_slave_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}

static void
amcc_master_slave_from_string_routine(AttributePtr attribute, const char* string)
{
    master_slave_t value = string_to_master_slave(string);
    if (kLineRateInvalid != value)
        amcc_master_slave_set_value(attribute, (void*)&value, sizeof(master_slave_t));
}


#if 0 /* Moved to sonet_pp_component.c along with "network mode" attribute (bit 19) */
static void
amcc_network_mode_to_string_routine(AttributePtr attribute)
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
amcc_network_mode_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute) && string != NULL)
    {
        network_mode_t nm = string_to_network_mode(string);
        amcc_network_mode_set_value(attribute, (void*)&nm, sizeof(nm));
    }
}

static void*
amcc_network_mode_get_value(AttributePtr attribute)
{
   static network_mode_t mode = kNetworkModeInvalid;
   uint32_t register_value = 0;
   GenericReadWritePtr network_grw = NULL;
   uint32_t *masks;
   
   if (1 == valid_attribute(attribute))
   {
	   network_grw = attribute_get_generic_read_write_object(attribute);
	   register_value = grw_read(network_grw);
       masks = attribute_get_masks(attribute);
       register_value &= masks[0];
	   if (register_value) 
		   mode = kNetworkModeATM;
	   else 
		   mode = kNetworkModePoS;
   }
   return (void*)&mode;
	   
}


static void
amcc_network_mode_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card =NULL;
    uint32_t register_value = 0;
    GenericReadWritePtr network_grw = NULL;    
    network_mode_t mode = *(network_mode_t*)value;
    uint32_t *masks;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        network_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(network_grw);
        masks = attribute_get_masks(attribute);
        switch(mode)
        {
           case kNetworkModeInvalid:
               /* nothing */
               break;
          case kNetworkModePoS:
               register_value &= ~(masks[0]); /* clear */
               break;
          case kNetworkModeATM:
               register_value |= masks[0]; /* set */
               break;
           case kNetworkModeRAW:
               /* nothing */
               break;
           case kNetworkModeEth:
               /* nothing */
               break;
        }
        grw_write(network_grw, register_value);
    }
}
#endif
