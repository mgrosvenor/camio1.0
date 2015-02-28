/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* 
Component interface to the MiniMac (http://192.168.64.242/phpwiki/MiniMAC%20version%201)
Any card that uses a mini mac can use this card. The mini mac is a soft (i.e. written in VHDL) ethernet framer.
*/

#include "../include/cards/common_dagx_constants.h"
#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/attribute_factory.h"
#include "../include/components/mini_mac_statistics_component.h"
#include "../include/modules/generic_read_write.h"
#include "../include/cards/dag3_constants.h"
#include "../include/components/sr_gpp.h"
#include "../include/components/gpp_component.h"
#include "../include/util/enum_string_table.h"

/* dag api headers */
#include "dag_platform.h"
#include "dagreg.h"
#include "dagutil.h"

typedef struct
{
    uint32_t mMiniMacBase;
    uint32_t mIndex;
} mini_mac_state_t;

enum
{
    k45CFPhyAddress = 0x80,
    kPhyAddress = 0xac
};

#define BUFFER_SIZE 1024

static int valid_sfp_module(AttributePtr attribute);
static void mini_mac_dispose(ComponentPtr component);
static void mini_mac_reset(ComponentPtr component);
static void mini_mac_default(ComponentPtr component);
static int mini_mac_post_initialize(ComponentPtr component);
static dag_err_t mini_mac_update_register_base(ComponentPtr component);

/* LineRate */
static AttributePtr mini_mac_get_new_linerate(void);
static void* mini_mac_linerate_get_value(AttributePtr attribute);
static void mini_mac_linerate_set_value(AttributePtr attribute, void* value, int length);
static void line_rate_to_string_routine(AttributePtr attribute);
static void line_rate_from_string_routine(AttributePtr attribute, const char* string);

/* Nic */
static AttributePtr mini_mac_get_new_nic(void);
static void* mini_mac_nic_get_value(AttributePtr attribute);
static void mini_mac_nic_set_value(AttributePtr attribute, void* value, int length);

/* Disable cooper link  */
static AttributePtr mini_mac_get_new_disablecopperlink(void);
static void* mini_mac_disablecopperlink_get_value(AttributePtr attribute);
static void mini_mac_disablecopperlink_set_value(AttributePtr attribute, void* value, int length);


/*Auto negotiation complete */
static AttributePtr mini_mac_get_new_auto_negotiation_complete_attribute(void);
static void* mini_mac_auto_negotiation_complete_get_value(AttributePtr attribute);

static uint32_t smbus_io_get_value(AttributePtr attribute, uint32_t device_register);
static void smbus_io_set_value(AttributePtr attribute, uint32_t device_register, uint32_t value);

/* function to set the mode to SGMII (for copper modules) or GBIC ( 1000Mbps only ) */
static void set_SGMII_or_GBIC(ComponentPtr comp);

ComponentPtr
mini_mac_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPort, card); 
    mini_mac_state_t* state = NULL;
    
    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        component_set_dispose_routine(result, mini_mac_dispose);
        component_set_post_initialize_routine(result, mini_mac_post_initialize);
        component_set_reset_routine(result, mini_mac_reset);
        component_set_default_routine(result, mini_mac_default);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        component_set_update_register_base_routine(result, mini_mac_update_register_base);
        component_set_description(result, "The MiniMAC soft framer");
        state = (mini_mac_state_t*)malloc(sizeof(mini_mac_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    return result;
}

static void
mini_mac_dispose(ComponentPtr component)
{
}

static void
mini_mac_reset(ComponentPtr component)
{
    uint32_t reg_val = 0;
    DagCardPtr card = component_get_card(component);
    mini_mac_state_t* state = component_get_private_state(component);
    reg_val = card_read_iom(card, state->mMiniMacBase);
    reg_val |= BIT0;
    card_write_iom(card, state->mMiniMacBase, reg_val);
}

static void
mini_mac_default(ComponentPtr component)
{
    AttributePtr any_attribute = NULL;
    uint8_t bool_val = 0;
    void* temp = NULL;
    ComponentPtr sfp_component = NULL;
    ComponentPtr root_component = NULL;
    mini_mac_state_t* state = NULL;
    line_rate_t lr = kLineRateEthernet1000;
    crc_t crc = kCrc32;
    uint32_t version = 0;
    DagCardPtr card = component_get_card(component);

    /* reset the mini-mac */
    mini_mac_reset(component);

    /*turn off eql*/
    bool_val = 0;
    state = component_get_private_state(component);
    version = card_get_register_version(card, DAG_REG_MINIMAC, state->mIndex);
    any_attribute = component_get_attribute(component, kBooleanAttributeEquipmentLoopback);
    attribute_set_value(any_attribute, (void*)&bool_val, 1);

    /* Note the SFP SMBUS will not work unless 
    TX is enabled in this case this means Laser enabled
    If the Laser is disbled the SMBUS is dead because the Marvell reset is hooked to the SFP laser 
    SMBUS can be accessed after 5ms after out of reset 
    */
    any_attribute = 0;
    /* Disable and then Enable Laser */ 
    if ( version < 2 )
    {
		/* Laser is in mini_mac component */
    any_attribute = component_get_attribute(component, kBooleanAttributeLaser);
    }
    else
    {
        /* Laser is in sfp/xfp  component */
        root_component = card_get_root_component(card);
        if (NULL != root_component)
            sfp_component = component_get_subcomponent(root_component, kComponentOptics, state->mIndex); 
        if ( NULL != sfp_component)
            any_attribute = component_get_attribute(sfp_component, kBooleanAttributeLaser);
    }
	if ( any_attribute)
	{
		bool_val = 0;
    attribute_set_value(any_attribute, (void*)&bool_val, 1);
        dagutil_microsleep(100000);
    
        bool_val = 1;
        attribute_set_value(any_attribute, (void*)&bool_val, 1);
        dagutil_microsleep(200000);
    }
    
     /* set to SGMII or GBIC */
     set_SGMII_or_GBIC  (component);

    /* set line rate to 1000 */
    any_attribute = component_get_attribute(component, kUint32AttributeLineRate);
    attribute_set_value(any_attribute, (void*)&lr, 1);

   /* enable autonegotiation */
    any_attribute = component_get_attribute(component, kBooleanAttributeNic);
    bool_val = 1;
    attribute_set_value(any_attribute, (void*)&bool_val, 1);

    /* set crc to 32-bits */
    any_attribute = component_get_attribute(component, kUint32AttributeCrcSelect);
    attribute_set_value(any_attribute, (void*)&crc, 1);

    /* enable CRC appending */
    bool_val = 1;
    any_attribute = component_get_attribute(component, kBooleanAttributeTxCrc);
    attribute_set_value(any_attribute, (void*)&bool_val, 1);
    
    any_attribute = component_get_attribute(component, kBooleanAttributeSFPDetect);
    temp = attribute_get_value(any_attribute);
    if (temp)
    {
        bool_val = *(uint8_t*)temp;
        if (bool_val)
        {
            /* turn on rocket io power. */
            any_attribute = component_get_attribute(component, kBooleanAttributeRocketIOPower);
            attribute_set_value(any_attribute, (void*)&bool_val, 1);

// Only DAG 4.5 Rev A Specific and in the ICD but not used 
//assumption is that customers do not have Rev A or they are using old SW and FW 	
//            any_attribute = component_get_attribute(component, kBooleanAttributeSfpPwr);
//            attribute_set_value(any_attribute, (void*)&bool_val, 1);
        }
    }

    /* activate the port */
    bool_val = 1;
    any_attribute = component_get_attribute(component, kBooleanAttributeActive);
    if (any_attribute)
    {
        attribute_set_value(any_attribute, (void*)&bool_val, 1); 
    }

    mini_mac_reset(component);
}

static dag_err_t
mini_mac_update_register_base(ComponentPtr component)
{
    return kDagErrNone;    
}

static int
mini_mac_post_initialize(ComponentPtr component)
{
    AttributePtr attr = NULL;
    mini_mac_state_t* state = component_get_private_state(component);
    uint32_t mask = 0;
    DagCardPtr card = component_get_card(component);
    GenericReadWritePtr grw = NULL;
    uintptr_t address = 0;
    uintptr_t gpp_base = 0;
    uint32_t version = 0;
    uint32_t register_value;
    int reg_version;

    state->mMiniMacBase = card_get_register_address(card, DAG_REG_MINIMAC, state->mIndex);
    
    address = ((uintptr_t)card_get_iom_address(card)) + state->mMiniMacBase;
    
    attr = mini_mac_get_new_nic();
    component_add_attribute(component, attr);

    attr = mini_mac_get_new_disablecopperlink();
    component_add_attribute(component, attr);

    version = card_get_register_version(card, DAG_REG_MINIMAC, state->mIndex);
    if (version < 2) 
    {
        mask = BIT1;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeRocketIOPower, grw, &mask, 1);
        component_add_attribute(component, attr);

// Only DAG 4.5 Rev A Specific and in the ICD but not used 
//assumption is that customers do not have Rev A or they are using old SW and FW 	
//        mask = BIT2;
//        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
//        attr = attribute_factory_make_attribute(kBooleanAttributeSfpPwr, grw, &mask, 1);
 //       component_add_attribute(component, attr);

        mask = BIT3;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeLaser, grw, &mask, 1);
        component_add_attribute(component, attr);

        mask = BIT4;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeEquipmentLoopback, grw, &mask, 1);
        component_add_attribute(component, attr);
    
        /* for version 2 onwards,these are implemented in SFP/XFP module*/
        mask = BIT18;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeLossOfSignal, grw, &mask, 1);
        component_add_attribute(component, attr);

        mask = BIT16;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeSFPDetect, grw, &mask, 1);
        component_add_attribute(component, attr);

    }

    mask = BIT9;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeTxCrc, grw, &mask, 1);
    component_add_attribute(component, attr);

    mask = BIT10;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeCrcSelect, grw, &mask, 1);
    component_add_attribute(component, attr);

    mask = BIT11;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributePMinCheck, grw, &mask, 1);
    component_add_attribute(component, attr);

    mask = BIT22;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLink, grw, &mask, 1);
    component_add_attribute(component, attr);

    mask = BIT21;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributePeerLink, grw, &mask, 1);
    component_add_attribute(component, attr);

    mask = BIT20;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRemoteFault, grw, &mask, 1);
    component_add_attribute(component, attr);

    mask = BIT19;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFrame, grw, &mask, 1);
    component_add_attribute(component, attr);

    /* add the active and drop count components to this mini_mac because it is a port component type */
    gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
    if (gpp_base > 0)
    {
        address = ((uintptr_t)card_get_iom_address(card)) + gpp_base + (uintptr_t)((state->mIndex + 1)*SP_OFFSET) + (uintptr_t)SP_CONFIG;

        mask = BIT12;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        grw_set_on_operation(grw, kGRWClearBit);
        attr = attribute_factory_make_attribute(kBooleanAttributeActive, grw, &mask, 1);
        attribute_set_getvalue_routine(attr, gpp_active_get_value);
        attribute_set_setvalue_routine(attr, gpp_active_set_value);
        component_add_attribute(component, attr);

        mask = 0xffffffff;
        address = ((uintptr_t)card_get_iom_address(card)) + gpp_base + (uintptr_t)((state->mIndex + 1)*SP_OFFSET) + (uintptr_t)SP_DROP;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kUint32AttributeDropCount, grw, &mask, 1);
        component_add_attribute(component, attr);
    }
    gpp_base = card_get_register_address(card, DAG_REG_SRGPP, 0);
    if (gpp_base > 0)
    {
        mask = 0xffffffff;
        address = (uintptr_t)card_get_iom_address(card) + gpp_base + (uintptr_t)(state->mIndex * kSRGPPStride + kSRGPPCounter);

        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kUint32AttributeDropCount, grw, &mask, 1);
        component_add_attribute(component, attr);

        /* Have to check the version of the SR-GPP to see if it supports stream pausing (enable/disable)
         * and varlen */
        if (card_get_register_version(card, DAG_REG_SRGPP, 0) == 1)
        {
            mask = BIT27;
            address = (uintptr_t)card_get_iom_address(card) + gpp_base + (uintptr_t)(state->mIndex * kSRGPPStride);
            grw = grw_init(card, address, grw_iom_read, grw_iom_write);
            grw_set_on_operation(grw, kGRWClearBit);
            attr = attribute_factory_make_attribute(kBooleanAttributeActive, grw, &mask, 1);
            component_add_attribute(component, attr);
        }
    }

    reg_version = card_get_register_version(card,DAG_REG_ROM, 0); 
    if(reg_version == -1)
	dagutil_error("not able to find the register \n");	
    /* Add Ethernet MAC Address */
    address = 8 + (state->mIndex*8); /* First port */
    
    grw = grw_init(card, address, grw_rom_read, NULL);
    attr = attribute_factory_make_attribute(kStringAttributeEthernetMACAddress, grw, &mask, 1);
    component_add_attribute(component, attr);    

    attr = mini_mac_get_new_linerate();
    component_add_attribute(component,attr);
     /* Add auto neg complete for all SFPs , but valid only for copper SFPs with contains a marvel chip*/
    attr = mini_mac_get_new_auto_negotiation_complete_attribute();
    component_add_attribute(component,attr);
    ////CHECK ME: for MIMIMAC VERSION 1 is it applicable
    if ( version < 2)
    {
        register_value = card_read_iom(card, state->mMiniMacBase);
        if (BIT31 & register_value) 
        {
            /* statistics module is present. */
            ComponentPtr mini_mac_stats = mini_mac_statistics_get_new_component(card, state->mIndex);
            component_add_subcomponent(component, mini_mac_stats);
        }
    }

    return 1;
}


uint32_t
mini_mac_get_index(ComponentPtr mini_mac)
{
    mini_mac_state_t* state = NULL;
    state = component_get_private_state(mini_mac);
    return state->mIndex;
}

static AttributePtr
mini_mac_get_new_nic(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeNic);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, mini_mac_nic_get_value);
        attribute_set_setvalue_routine(result, mini_mac_nic_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "auto_neg");
        attribute_set_description(result, "Enable or disable Ethernet auto-negotiation mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static AttributePtr
mini_mac_get_new_disablecopperlink(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeDisableCopperLink);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, mini_mac_disablecopperlink_get_value);
        attribute_set_setvalue_routine(result, mini_mac_disablecopperlink_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "disablecopperlink");
        attribute_set_description(result, "Disables Ethernet copper link forced - uses power down of Marvell chip");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}







static void*
mini_mac_disablecopperlink_get_value(AttributePtr attribute)
{
    uint32_t register_value;
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
  
	// Check if is a copper or Optics module 
	 if(valid_sfp_module(attribute) == 1) {
         /* Reads the autonegotiation for the copper side of the SFP in copper SFPs only */
            register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
            if( register_value & BIT11)
                value = 1;
            else
                value = 0;
 	} else {
	// Optics module is not supporting it so it is always enabled link 
		value = 0;
	}; 

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
mini_mac_disablecopperlink_set_value(AttributePtr attribute, void* value, int length)
{
    uint32_t register_value = 0;
    uint8_t valid_sfp = 0;

    if (1 == valid_attribute(attribute))
    {
        /* Check to see if valid sfp module first */
        if(valid_sfp_module(attribute) == 1)
        {
            valid_sfp = 1;

	    /* Sets the SFP Marvel chip in to low power forcing the copper link to go off */
            register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
            if(*(uint8_t*)value == 0)
                register_value &= ~BIT11;
            else
                register_value |= BIT11;
            /* Have to write a byte at a time */
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>> 8);
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));

        } else {
		// for optics module we do nothing 
	} 

    }
    dagutil_nanosleep(1000);
	
}











static void*
mini_mac_nic_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    mini_mac_state_t * state = (mini_mac_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
/*
 * Read out of the autonegotionation from the FPGA 
 * This autonegotiation is not the real value for copper it is only for the SGMII interface
 * Now the FPGA in copper uses always auto negotiation 
 * So now it reads the satstus out of the SFP module 
 */
  
	// Check if is a copper or Optics module 
	 if(valid_sfp_module(attribute) == 1) {
         /* Reads the autonegotiation for the copper side of the SFP in copper SFPs only */
            register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
            if( register_value & BIT12)
                value = 1;
            else
                value = 0;
 	} else {
	// Optics module read from the FPGA  
        register_value = card_read_iom(card, state->mMiniMacBase);
        if(register_value & BIT5)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
	}; 

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
mini_mac_nic_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    mini_mac_state_t * state = (mini_mac_state_t *)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint8_t valid_sfp = 0;

    if (1 == valid_attribute(attribute))
    {
        /* Check to see if valid sfp module first */
        if(valid_sfp_module(attribute) == 1)
        {
            valid_sfp = 1;
            /* If so configure module via smbus and then configure mini mac */

                        
            /* advertise all the speeeds */
            /* enable 1000Base-T Advertisment */
            register_value = smbus_io_get_value(attribute, MV88E1111_1KTCR);
            register_value |= BIT11|BIT10|BIT9;
            
            /* Have to write a byte at a time */
            smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff00)>>8);
            smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff));
            /*enable 10Base-T  and 100Base-T Advertisment */
            register_value = BIT6 | BIT8|BIT0;
            smbus_io_set_value(attribute, MV88E1111_ANAR, (register_value&0xff00)>>8);
            smbus_io_set_value(attribute, MV88E1111_ANAR, (register_value&0xff));

	    /* enable disable PHY autoneg global control register bit 12 */
            register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
            if(*(uint8_t*)value == 0)
                register_value &= ~BIT12;
            else
                register_value |= BIT12;
            /* Have to write a byte at a time */
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>> 8);
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));
            /* We want to talk to page 1  */
            register_value = smbus_io_get_value(attribute, MV88E1111_EXAR);
            register_value |= BIT0;
            smbus_io_set_value(attribute, MV88E1111_EXAR, (register_value&0xff00)>>8);
            smbus_io_set_value(attribute, MV88E1111_EXAR, (register_value&0xff));

            /* set no nic on reg 0 page 1 */
            /* If so configure module via smbus and then configure mini mac */
            register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
			//SGMII or connection with the FPGA aways in autonegotiation mode
			// Not to be confused with the copper side 
            //if(*(uint8_t*)value == 0)
            //    register_value &= ~BIT12;
            //else
                register_value |= BIT12;
            /* Have to write a byte at a time */
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>> 8);
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));        

            /* Default back to page 0  */
            register_value = smbus_io_get_value(attribute, MV88E1111_EXAR);
            register_value &= ~BIT0;
            smbus_io_set_value(attribute, MV88E1111_EXAR, (register_value&0xff00)>>8);
            smbus_io_set_value(attribute, MV88E1111_EXAR, (register_value&0xff));        
        }

        register_value = card_read_iom(card, state->mMiniMacBase);
		//Leaving the FPGA setting to the SGMII always in auto neg mode 
		//note: not to be confused with autonegotiation on the line copper side 
		// Reason we are using in both (autoneg and no autoneg SGMII autonegotiation to extract 
		// the link status , fixes problems with some SFPs which do not report Loss of fignal 
        

	if(valid_sfp == 1)
	{
		//Copper  module olways autoneg in sgmii
		register_value |= BIT5;
	} else 
	{
	//Optics module old way 
         	if(*(uint8_t*)value == 0)
        	    register_value &= ~BIT5;
        	else
        	    register_value |= BIT5;
	}

        card_write_iom(card, state->mMiniMacBase, register_value);

        if(valid_sfp == 1)
        {
            /* reset the sfp module */
            register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
            register_value |= BIT15;
            /* Have to write a byte at a time */
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00) >> 8);
            smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));
            /* Reset the minimac */
 //           mini_mac_reset(comp);
        }
    }
    dagutil_nanosleep(1000);
    mini_mac_reset(comp);

	
}


static AttributePtr
mini_mac_get_new_linerate(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineRate);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, mini_mac_linerate_get_value);
        attribute_set_setvalue_routine(result, mini_mac_linerate_set_value);
        attribute_set_to_string_routine(result, line_rate_to_string_routine);
        attribute_set_from_string_routine(result, line_rate_from_string_routine);
        attribute_set_name(result, "line_rate");
        attribute_set_description(result, "Set the line rate to a specific speed");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
mini_mac_linerate_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    mini_mac_state_t * state = (mini_mac_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    line_rate_t value = kLineRateInvalid;

    if (1 == valid_attribute(attribute))
    {
        if(valid_sfp_module(attribute))
        {
            register_value = smbus_io_get_value(attribute, MV88E1111_LINK_AN);
            if((register_value & (BIT15 | BIT14)) == BIT15)
            {
                value = kLineRateEthernet1000;
            }
            else if((register_value & (BIT15 | BIT14)) == BIT14 )
            {
                value = kLineRateEthernet100;
            }
            else if((register_value & (BIT15 | BIT14)) == 0x00 )
            {
                value = kLineRateEthernet10;
            }
            attribute_set_value_array(attribute, &value, sizeof(value));
            return (void *)attribute_get_value_array(attribute);
        }
        else
        {
            register_value = card_read_iom(card, state->mMiniMacBase);
            if(register_value & BIT8)
            {
                value = kLineRateEthernet1000;
            }
            else if(register_value & BIT7)
            {
                value = kLineRateEthernet100;
            }
            else
            {
                value = kLineRateEthernet10;
            }
            attribute_set_value_array(attribute, &value, sizeof(value));
            return (void *)attribute_get_value_array(attribute);
        }
    }
    return NULL;
}

static void
mini_mac_linerate_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    mini_mac_state_t * state = (mini_mac_state_t *)component_get_private_state(comp);
    uint32_t register_value = 0;
    line_rate_t line_rate = *(line_rate_t*)value;

	//printf ("Warning! %s: Entering %s function\n", __FILE__, __FUNCTION__);


    if (1 == valid_attribute(attribute))
    {
         /* Check to see if valid sfp module first */
        if(valid_sfp_module(attribute) == 0)
        {
		//printf ("Warning! %s: %s - Invalid sfp module!\n", __FILE__, __FUNCTION__);

            return;
        }
        /* Set interface to SGMII */
        //register_value = BIT15|BIT7|BIT2;
        register_value = BIT15|BIT2;
        smbus_io_set_value(attribute, MV88E1111_EPSSR, (register_value&0xff00)>>8);
        smbus_io_set_value(attribute, MV88E1111_EPSSR, (register_value&0xff));
         
        /* reset the phy */
        register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
        register_value |= BIT15;
        /* Have to write a byte at a time */
        smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>>8);
        smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));
        /* If so configure module via smbus and then configure mini mac */

	//printf ("Warning! %s: %s - Line rate: %s\n", __FILE__, __FUNCTION__, line_rate);

        switch(line_rate)
        {
            case kLineRateEthernet1000:

		//printf ("Warning! %s: %s - Setting line rate to 1000\n", __FILE__, __FUNCTION__);
                /* enable 1000Base-T Advertisment */
                register_value = smbus_io_get_value(attribute, MV88E1111_1KTCR);
                register_value |= BIT11|BIT10|BIT9;

        /* Have to write a byte at a time */
                smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff));

                /* set up base control register */
                register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
                register_value &= ~(BIT13|BIT8|BIT6);
                register_value |= BIT8|BIT6;
        /* Have to write a byte at a time */
                smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));
                break;

            case kLineRateEthernet100:
		//printf ("Warning! %s: %s -  Setting line rate to 100\n", __FILE__, __FUNCTION__);

                /* disable 1000m advertisment*/
                register_value = BIT11|BIT10;
                smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff));

                /* Set up base control register */
                register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
                register_value &= ~(BIT13|BIT8|BIT6);
                register_value |= BIT13|BIT8;
                smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));

                /*enable 100Base-T Advertisment */
                register_value = BIT8|BIT0;
                smbus_io_set_value(attribute, MV88E1111_ANAR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_ANAR, (register_value&0xff));
                break;

            case kLineRateEthernet10:
		//printf ("Warning! %s: %s - Setting line rate to 10\n", __FILE__, __FUNCTION__);

                /* setup base control register */
                register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
                register_value &= ~(BIT13|BIT8|BIT6);
                register_value |= BIT8;
                smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));

                /* enable 10Base-T advertisment */
                register_value = BIT6|BIT0;
                smbus_io_set_value(attribute, MV88E1111_ANAR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_ANAR, (register_value&0xff));
                /* diable 1000Base-T advertisment */
                register_value = BIT11|BIT10;
                smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff00)>>8);
                smbus_io_set_value(attribute, MV88E1111_1KTCR, (register_value&0xff));
                break;
            default:
                break;
        }
        register_value = card_read_iom(card, state->mMiniMacBase);
        register_value &=~(BIT8|BIT7);
        switch(line_rate)
        {
            case kLineRateEthernet1000:
                register_value |= BIT8;
                break;

            case kLineRateEthernet100:
                register_value |= BIT7;
                break;

            default:
                break;
        }
        register_value |= BIT6 | BIT0;
        //register_value |= BIT6;
        card_write_iom(card, state->mMiniMacBase, register_value);
        /* reset the phy */
        register_value = smbus_io_get_value(attribute, MV88E1111_BMCR);
        register_value |= BIT15;
        /* Have to write a byte at a time */
        smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff00)>>8);
        smbus_io_set_value(attribute, MV88E1111_BMCR, (register_value&0xff));

    }
}

static void
line_rate_to_string_routine(AttributePtr attribute)
{
    void* temp = attribute_get_value(attribute);
    const char* string = NULL;
    line_rate_t lr;
    if (temp)
    {
        lr = *(line_rate_t*)temp;
        string = line_rate_to_string(lr);
        if (string)
            attribute_set_to_string(attribute, string);
    }
}

static void
line_rate_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        line_rate_t mode = string_to_line_rate(string);
        mini_mac_linerate_set_value(attribute, (void*)&mode, sizeof(mode));
    }
}


static int
valid_sfp_module(AttributePtr attribute)
{
	//printf ("Warning! %s: Entering %s function\n", __FILE__, __FUNCTION__);

    if(1 == valid_attribute(attribute))
    {
	//printf ("Warning! %s: %s - Valid attribute detected\n", __FILE__, __FUNCTION__);
	
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr mini_mac = attribute_get_component(attribute);
        int index = mini_mac_get_index(mini_mac);
        uint32_t phy_oui = 0;
        mini_mac_state_t* state = component_get_private_state(mini_mac);
        uint32_t version;
        
        /*address has to be packed to contain the the minimac index, device address and the device register*/
        if (card_get_card_type(card) == kDag452cf)
        {
            address = k45CFPhyAddress << 8;
        }
        else
        {
            address = kPhyAddress << 8;
        }
        address |= MV88E1111_PHYID1;
        address |= (index << 16);

	//printf ("Warning! %s: %s - address: 0x%x\n", __FILE__, __FUNCTION__, address);

        version = card_get_register_version(card, DAG_REG_MINIMAC, state->mIndex);
        if (version < 2)
        {
            sfp_grw = grw_init(card, address, grw_mini_mac_raw_smbus_read, grw_mini_mac_raw_smbus_write);
        }
        else if ( card_get_register_count(card, DAG_REG_XFP) > 0 )
        {
		//sfp_grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
		//sfp_grw = grw_init(card, address, grw_drb_optics_smbus_read, grw_drb_optics_smbus_write);
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        }
        else /* CHECK ME */
        {
            sfp_grw = grw_init(card, address, grw_drb_optics_smbus_read, grw_drb_optics_smbus_write);
        }

        assert(sfp_grw);
        phy_oui = grw_read(sfp_grw) << 8;
        phy_oui |= grw_read(sfp_grw);
        
        grw_dispose(sfp_grw);
	//printf ("Warning! %s: %s - phy_oui: 0x%x\n", __FILE__, __FUNCTION__, phy_oui);
        if(phy_oui == 0x0141)
            return 1;
    }
    return 0;
}
static void
smbus_io_set_value(AttributePtr attribute, uint32_t device_register, uint32_t value)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t address = 0;
        ComponentPtr mini_mac = attribute_get_component(attribute);
        int index = mini_mac_get_index(mini_mac);
        mini_mac_state_t* state = component_get_private_state(mini_mac);
        uint32_t version;

        /*address has to be packed to contain the the minimac index, device address and the device register*/
        if (card_get_card_type(card) == kDag452cf)
        {
            address = k45CFPhyAddress << 8;
        }
        else
        {
            address = kPhyAddress << 8;
        }
        address |= device_register;
        address |= (index << 16);

        version = card_get_register_version(card, DAG_REG_MINIMAC, state->mIndex);
        if (version < 2)
        {
            sfp_grw = grw_init(card, address, grw_mini_mac_raw_smbus_read, grw_mini_mac_raw_smbus_write);
        }
        else if ( card_get_register_count(card, DAG_REG_XFP) > 0 )
        {
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        }
        else
        {
            sfp_grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
        }

        assert(sfp_grw);
        grw_write(sfp_grw, value);
        free(sfp_grw);
        usleep(1000);
    }
}

static uint32_t
smbus_io_get_value(AttributePtr attribute, uint32_t device_register)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t address = 0;
        ComponentPtr mini_mac = attribute_get_component(attribute);
        int index = mini_mac_get_index(mini_mac);
        static uint32_t reg_val = 0;
        mini_mac_state_t* state = component_get_private_state(mini_mac);
        uint32_t version;
        
        /*address has to be packed to contain the the minimac index, device address and the device register*/
        if (card_get_card_type(card) == kDag452cf)
        {
            address = k45CFPhyAddress << 8;
        }
        else
        {
            address = kPhyAddress << 8;
        }
        address |= device_register;
        address |= (index << 16);

        version = card_get_register_version(card, DAG_REG_MINIMAC, state->mIndex);
        if (version < 2)
        {
            sfp_grw = grw_init(card, address, grw_mini_mac_raw_smbus_read, grw_mini_mac_raw_smbus_write);
        }
        else if ( card_get_register_count(card, DAG_REG_XFP) > 0 )
        {
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        }
        else
        {
            sfp_grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
        }

        assert(sfp_grw);
        reg_val = grw_read(sfp_grw) << 8;
        reg_val |= grw_read(sfp_grw);
        free(sfp_grw);
        return reg_val;
    }
    return 0;
}

void set_SGMII_or_GBIC( ComponentPtr comp)
{ 
    AttributePtr any_attribute;
    DagCardPtr card = component_get_card(comp);
    mini_mac_state_t * state = (mini_mac_state_t *)component_get_private_state(comp);
    uint32_t register_value = 0;
    /* take any attribute of the mini mac component - Just to pass it to valid_sfp_module() */
    any_attribute = component_get_attribute(comp, kUint32AttributeLineRate);

    register_value = card_read_iom(card, state->mMiniMacBase);  
    if(valid_sfp_module(any_attribute) == 1)
     {
        /* set the mode to SGMII*/
        register_value |= (BIT6);
     }
     else
    {
        /* set mode to GBIC  */
        register_value &= ~(BIT6);
    }
    card_write_iom(card, state->mMiniMacBase, register_value);
    return;

}

AttributePtr
mini_mac_get_new_auto_negotiation_complete_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAutoNegotiationComplete); 
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, mini_mac_auto_negotiation_complete_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "auto_neg_complete");
        attribute_set_description(result, "Is Auto netotiation complete or not");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void*
mini_mac_auto_negotiation_complete_get_value(AttributePtr attribute)
{
    static uint8_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    mini_mac_state_t *state = NULL;

    if (1 == valid_attribute(attribute))
    {
        if (0 == valid_sfp_module(attribute) )
        {
            /* For normal SFPs ( other than copper with marvel) assume that attribute value is 1 */
            return_value = 1;
        }
        else
        {
            card = attribute_get_card(attribute);
            component = attribute_get_component(attribute);
            state = component_get_private_state(component);
            register_value = smbus_io_get_value(attribute, MV88E1111_LINK_AN);
            return_value = (register_value & BIT11) == BIT11;
        }
        attribute_set_value_array(attribute, &return_value, sizeof(return_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}


