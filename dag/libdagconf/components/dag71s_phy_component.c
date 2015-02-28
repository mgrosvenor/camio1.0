/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/components/dag71s_phy_component.h"
#include "../include/modules/generic_read_write.h"
#include "../include/attribute_factory.h"

/* dag api headers */
#include "dag_platform.h"
#include "dagreg.h"
#include "dagutil.h"

typedef struct
{
    uint32_t mPhyBase;
} phy_state_t;

typedef enum
{
    kAMCCReset = 0x174,
    kAMCCConfig = 0x0
} amcc_registers_t;


static void dag71s_phy_dispose(ComponentPtr component);
static void dag71s_phy_reset(ComponentPtr component);
static void dag71s_phy_default(ComponentPtr component);
static int dag71s_phy_post_initialize(ComponentPtr component);
static dag_err_t dag71s_phy_update_register_base(ComponentPtr component);

static AttributePtr get_new_fcl(void);
static void fcl_dispose(AttributePtr attribute);
static void fcl_post_initialize(AttributePtr attribute);
static void* fcl_get_value(AttributePtr attribute);
static void fcl_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr get_new_eql(void);
static void eql_dispose(AttributePtr attribute);
static void eql_post_initialize(AttributePtr attribute);
static void* eql_get_value(AttributePtr attribute);
static void eql_set_value(AttributePtr attribute, void* value, int length);


ComponentPtr
dag71s_get_new_phy_component(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentPhy, card); 
    phy_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag71s_phy_dispose);
        component_set_post_initialize_routine(result, dag71s_phy_post_initialize);
        component_set_reset_routine(result, dag71s_phy_reset);
        component_set_default_routine(result, dag71s_phy_default);
        component_set_update_register_base_routine(result, dag71s_phy_update_register_base);
        component_set_name(result, "phy");
		component_set_description(result, "The component that represents the AMCC transceiver.");
        state = (phy_state_t*)malloc(sizeof(phy_state_t));
        component_set_private_state(result, state);
    }
    return result;
}

dag_err_t
dag71s_phy_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = component_get_card(component);
        phy_state_t* state = component_get_private_state(component);
        state->mPhyBase = card_get_register_address(card, DAG_REG_AMCC, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static void
dag71s_phy_dispose(ComponentPtr component)
{
}

static void
dag71s_phy_reset(ComponentPtr component)
{
}

static void
dag71s_phy_default(ComponentPtr component)
{
}

static int
dag71s_phy_post_initialize(ComponentPtr component)
{
    AttributePtr eql;
    AttributePtr fcl;
    phy_state_t* state = NULL;
    DagCardPtr card;

    state = component_get_private_state(component);
    card = component_get_card(component);
    state->mPhyBase = card_get_register_address(card, DAG_REG_AMCC, 0);

    eql = get_new_eql();
	fcl = get_new_fcl();

    component_add_attribute(component, eql);
    component_add_attribute(component, fcl);

    return 1;
}

static AttributePtr
get_new_fcl(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFacilityLoopback); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, fcl_dispose);
        attribute_set_post_initialize_routine(result, fcl_post_initialize);
        attribute_set_getvalue_routine(result, fcl_get_value);
        attribute_set_setvalue_routine(result, fcl_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "fcl");
        attribute_set_description(result, "facility loopback");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
fcl_dispose(AttributePtr attribute)
{
}

static void*
fcl_get_value(AttributePtr attribute)
{
    ComponentPtr phy;
    uint32_t register_value;
    static uint8_t value;
	//int index;
//	DagCardPtr card = NULL;
//	daginf_t* dag_card_info;

    if (1 == valid_attribute(attribute))
    {
//        card = attribute_get_card(attribute);
		//dag_card_info = card_get_info(card);
        phy = attribute_get_component(attribute);
		/**if(dag_card_info->brd_rev==1){	//this is AMCC1221 framer   
			//This will read LLEB (line loop back enable bit from register 0x0)
			//The bit is bit is active low meaning when equal to zero will
			//put the device in FCL mode
			//The new chip has four bits at location 0x8,0xa,0xc,0xe
			register_value = dag71s_phy_read(phy, 0x08);	
			value = !((register_value & BIT6) == BIT6);
			for(index=1;index<4;index++){
				register_value = dag71s_phy_read(phy, 0x08+(index*2));	
				value &= !((register_value & BIT6) == BIT6);
			}
			
			return (void*)&value;
		}
		else{	//this is AMCC1213 framer **/  
			
			register_value = dag71s_phy_read(phy, kAMCCConfig);	//kAMCCConfig = 0x0 (AMCC register)
			value = !((register_value & BIT5) == BIT5);
			return (void*)&value;
		//}
	}
    return NULL;
}

static void
fcl_post_initialize(AttributePtr attribute)
{
}

static void
fcl_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr phy;
    uint32_t register_value;
//	int index;
	//DagCardPtr card = NULL;
	//daginf_t* dag_card_info;

    if (1 == valid_attribute(attribute))
    {
        phy = attribute_get_component(attribute);
		//card = attribute_get_card(attribute);
		//dag_card_info = card_get_info(card);    
		/**if(dag_card_info->brd_rev==1){//this is an AMCC1221 framer
			//need to set all four bits
			for(index=0;index<4;index++){
				register_value = dag71s_phy_read(phy, 0x08+(index*2));	
				if (*(uint8_t*)value == 1)
					register_value &= ~BIT6;
				else if (*(uint8_t*)value == 0)
					register_value |= BIT6;
				dag71s_phy_write(phy,0x08+(index*2), register_value);
			}//for loop
		}//if
		else{//this is an AMCC1213 framer**/
			register_value = dag71s_phy_read(phy, kAMCCConfig);
			if (*(uint8_t*)value == 1)
			{
				register_value &= ~BIT5;
			}
			else if (*(uint8_t*)value == 0)
        {
				register_value |= BIT5;
			}
	        
			dag71s_phy_write(phy, kAMCCConfig, register_value);
	//	}//else
}
}


static AttributePtr
get_new_eql(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeEquipmentLoopback); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, eql_dispose);
        attribute_set_post_initialize_routine(result, eql_post_initialize);
        attribute_set_getvalue_routine(result, eql_get_value);
        attribute_set_setvalue_routine(result, eql_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "eql");
        attribute_set_description(result, "Enable or disable equipment loopback");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
eql_dispose(AttributePtr attribute)
{
}

static void*
eql_get_value(AttributePtr attribute)
{
    ComponentPtr phy;
    uint32_t register_value;
    static uint8_t value;
	//int index;
	//DagCardPtr card = NULL;
	//daginf_t* dag_card_info;

    if (1 == valid_attribute(attribute))
    {
       // card = attribute_get_card(attribute);
	//	dag_card_info = card_get_info(card);	
        phy = attribute_get_component(attribute);
		/**if(dag_card_info->brd_rev==1){	//this is AMCC1221 framer   
			
			register_value = dag71s_phy_read(phy, 0x08);	
			value = !((register_value & BIT5) == BIT5);
			
			for(index=1;index<4;index++){
				register_value = dag71s_phy_read(phy, 0x08+(index*2));	
				value &= !((register_value & BIT5) == BIT5);
			}
			return (void*)&value;
		}//if
		else{	//this is AMCC1213 framer **/  
			register_value = dag71s_phy_read(phy, kAMCCConfig);
			value = !((register_value & BIT4) == BIT4);
			return (void*)&value;
		//}//else
	}//valid attribute
    return NULL;
}

static void
eql_post_initialize(AttributePtr attribute)
{
}

static void
eql_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr phy;
    uint32_t register_value;
	//int index;
	//DagCardPtr card = NULL;
	//daginf_t* dag_card_info;

    if (1 == valid_attribute(attribute))
    {
        phy = attribute_get_component(attribute);
       // card = attribute_get_card(attribute);
		//dag_card_info = card_get_info(card);
		 /**if(dag_card_info->brd_rev==1){//this is an AMCC1221 framer
			printf("board rev is 1");
			 //need to set all four bits
			for(index=0;index<4;index++){
				register_value = dag71s_phy_read(phy, 0x08+(index*2));	
				if (*(uint8_t*)value == 1)
					register_value &= ~BIT5;
				else if (*(uint8_t*)value == 0)
					register_value |= BIT5;
				dag71s_phy_write(phy,0x08+(index*2), register_value);
			}//for loop
		}//if
		else{//this is an AMCC1213 framer**/
			register_value = dag71s_phy_read(phy, kAMCCConfig);
			if (*(uint8_t*)value == 1)
				register_value &= ~BIT4;
			else if (*(uint8_t*)value == 0)
				register_value |= BIT4;
			dag71s_phy_write(phy, kAMCCConfig,register_value);
		//}//else        
        
    }
}


void
dag71s_phy_write(ComponentPtr component, int phy_reg, uint32_t val)
{
    int phy_val;
    phy_state_t* state = component_get_private_state(component);
    DagCardPtr card = component_get_card(component);
    
    if (component_get_component_code(component) == kComponentPhy)
    {
        phy_val = 0;
        phy_val |= ((phy_reg & 0x01F) << 16);
        phy_val |= (val & 0x0FFFF);
        //    if (phy_reg == 0x08)
        //        printf("phy_WR: 0x%08x\n", phy_val);
        card_write_iom(card, state->mPhyBase, phy_val);
        usleep(1000); 
    }
}

int
dag71s_phy_read(ComponentPtr component, int phy_reg)
{
    int wr_val, rd_val;
    phy_state_t* state = component_get_private_state(component);
    DagCardPtr card = component_get_card(component);

    if (component_get_component_code(component) == kComponentPhy)
    {
        wr_val = 0;
        wr_val |= BIT31;
        wr_val |= ((phy_reg & 0x01F) << 16);
        card_write_iom(card, state->mPhyBase, wr_val);
        usleep(1000);
        rd_val = card_read_iom(card, state->mPhyBase);
        //    if (phy_reg == 0x0c)
        //        printf("phy_RD: 0x%08x\n", rd_val);
        return (rd_val & 0x0000FFFF);
    }
    return 0;
}
