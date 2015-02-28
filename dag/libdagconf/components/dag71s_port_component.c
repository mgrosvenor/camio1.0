/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/components/dag71s_port_component.h"
#include "../include/components/dag71s_phy_component.h"
#include "../include/util/enum_string_table.h"
#include "../include/cards/common_dagx_constants.h"

#include "dagutil.h"

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

#define BUFFER_SIZE 1024

typedef struct
{
    uint32_t mIndex;
} port_state_t;

/* port component. */
static void dag71s_port_dispose(ComponentPtr component);
static void dag71s_port_reset(ComponentPtr component);
static void dag71s_port_default(ComponentPtr component);
static int dag71s_port_post_initialize(ComponentPtr component);
static dag_err_t dag71s_port_update_register_base(ComponentPtr component);

/* master_slave attribute */
static AttributePtr get_new_master_slave_attribute(void);
static void* master_slave_get_value(AttributePtr attribute);
static void master_slave_set_value(AttributePtr attribute, void* value, int len);
static void master_slave_dispose(AttributePtr attribute);
static void master_slave_post_initialize(AttributePtr attribute);

/* lock attribute */
static AttributePtr get_new_lock_attribute(void);
static void lock_dispose(AttributePtr attribute);
static void lock_post_initialize(AttributePtr attribute);
static void* lock_get_value(AttributePtr attribute);

/* core attribute */
static AttributePtr get_new_core_attribute(void);
static void core_dispose(AttributePtr attribute);
static void core_post_initialize(AttributePtr attribute);
static void* core_get_value(AttributePtr attribute);

/* fifo attribute */
static AttributePtr get_new_fifo_attribute(void);
static void fifo_dispose(AttributePtr attribute);
static void fifo_post_initialize(AttributePtr attribute);
static void* fifo_get_value(AttributePtr attribute);

/* line_rate attribute. */
static AttributePtr get_new_line_rate_attribute(void);
static void line_rate_dispose(AttributePtr attribute);
static void* line_rate_get_value(AttributePtr attribute);
static void line_rate_set_value(AttributePtr attribute, void* value, int length);
static void line_rate_post_initialize(AttributePtr attribute);
static void line_rate_to_string_routine(AttributePtr attribute);
static void line_rate_from_string_routine(AttributePtr attribute, const char* string);

static AttributePtr get_new_active_attribute(void);
static void active_dispose(AttributePtr attribute);
static void* active_get_value(AttributePtr attribute);
static void active_set_value(AttributePtr attribute, void* value, int length);
static void active_post_initialize(AttributePtr attribute);


/* port component. */
ComponentPtr
dag71s_get_new_port(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentPort, card); 
    char buffer[BUFFER_SIZE];
    port_state_t* port_state;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag71s_port_dispose);
        component_set_post_initialize_routine(result, dag71s_port_post_initialize);
        component_set_reset_routine(result, dag71s_port_reset);
        component_set_default_routine(result, dag71s_port_default);
        component_set_update_register_base_routine(result, dag71s_port_update_register_base);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);        
        port_state = (port_state_t*)malloc(sizeof(port_state_t));
        port_state->mIndex = index;
        component_set_private_state(result, port_state);
    }
    
    return result;
}

static dag_err_t
dag71s_port_update_register_base(ComponentPtr component)
{
    /* Writes and reads to this component take place via the phy component. So as long as that component's register base is ok
    then this component should work*/
    return kDagErrNone;
}

static void
dag71s_port_dispose(ComponentPtr component)
{
}

static void
dag71s_port_reset(ComponentPtr component)
{
}

static void
dag71s_port_default(ComponentPtr component)
{
}

static int
dag71s_port_post_initialize(ComponentPtr component)
{
    port_state_t* state;
    AttributePtr line_rate;
    AttributePtr lock;
    AttributePtr core;
    AttributePtr fifo;
    AttributePtr ms;
    AttributePtr active = 0;
    DagCardPtr card = NULL;

    card = component_get_card(component);
    state = component_get_private_state(component);

    line_rate = get_new_line_rate_attribute();
    lock = get_new_lock_attribute();
    core = get_new_core_attribute();
    fifo = get_new_fifo_attribute();
    ms = get_new_master_slave_attribute();
    if (card_get_register_address(card, DAG_REG_GPP, 0))
        active = get_new_active_attribute();

    component_add_attribute(component, line_rate);
    component_add_attribute(component, lock);
    component_add_attribute(component, core);
    component_add_attribute(component, fifo);
    component_add_attribute(component, ms);
    if (card_get_register_address(card, DAG_REG_GPP, 0))
        component_add_attribute(component, active);

    return 1;
}

static AttributePtr
get_new_lock_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLock); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, lock_dispose);
        attribute_set_post_initialize_routine(result, lock_post_initialize);
        attribute_set_getvalue_routine(result, lock_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "lock");
        attribute_set_description(result, "Indicates whether the core has a lock on the datastream.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
lock_dispose(AttributePtr attribute)
{
}

static void*
lock_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr phy = NULL;
    ComponentPtr port = NULL;
    ComponentPtr root = NULL;
    uint32_t register_value;
    static uint8_t value;
    port_state_t* state;
	//daginf_t* dag_card_info;	

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
		//dag_card_info = card_get_info(card);
        root = card_get_root_component(card);
        phy = component_get_subcomponent(root, kComponentPhy, 0);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
		/**if(dag_card_info->brd_rev==1){
			register_value = dag71s_phy_read(phy, 0x15);
			switch(state->mIndex){
				case 0: value = (register_value & BIT4) == BIT4; break;
				case 1: value = (register_value & BIT5) == BIT5; break;
				case 2: value = (register_value & BIT6) == BIT6; break;
				case 3: value = (register_value & BIT7) == BIT7; break;
			
				default: value =0;
			}
		
			return (void*)&value;
		}

		else{	//the framer is AMCC1213 **/
			register_value = dag71s_phy_read(phy, 0x8 + (state->mIndex * 2));
			value = (register_value & BIT7) == BIT7;
			return (void*)&value;
		//}
    }
    return NULL;
}

static void
lock_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
get_new_line_rate_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineRate); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, line_rate_dispose);
        attribute_set_post_initialize_routine(result, line_rate_post_initialize);
        attribute_set_getvalue_routine(result, line_rate_get_value);
        attribute_set_setvalue_routine(result, line_rate_set_value);
        attribute_set_to_string_routine(result, line_rate_to_string_routine);
        attribute_set_from_string_routine(result, line_rate_from_string_routine);
        attribute_set_name(result, "line_rate");
        attribute_set_description(result, "Set the line rate to a specific speed.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
line_rate_dispose(AttributePtr attribute)
{
}

static void*
line_rate_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port = NULL;
    ComponentPtr root_component = NULL;
    ComponentPtr phy = NULL;
    port_state_t* port_state = NULL;
    uint32_t register_value;
    static line_rate_t line_rate;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        root_component = card_get_root_component(card);
        phy = component_get_subcomponent(root_component, kComponentPhy, 0);
        port = attribute_get_component(attribute);
        port_state = component_get_private_state(port);
        register_value = dag71s_phy_read(phy, 0x8 + (port_state->mIndex * 2));
        if ((register_value & BIT2) == BIT2)
        {
            line_rate = kLineRateOC12c;
        }
        else
        {
            line_rate = kLineRateOC3c;
        }
        return (void*)&line_rate;
    }
    return NULL;
}

static void
line_rate_post_initialize(AttributePtr attribute)
{
}

#if 0
static bool
check_sfp_line_rate(ComponentPtr port, line_rate_t lr)
{
    GenericReadWritePtr grw;
    uint32_t address = 0;
    uint32_t value = 0;
    DagCardPtr card = component_get_card(port);
    port_state_t* state = component_get_private_state(port);

    /*address is packed to contain the index of the port, address of the device and the register
     * on the device */
    address = 0xa0 << 8;
    address |= 12;
    address |=  state->mIndex << 16;
    grw = grw_init(card, address, grw_raw_smbus_read, grw_raw_smbus_write);
    value = grw_read(grw) * 100 /*convert to Mbps - see SFP module datasheet*/;
    if (grw_get_last_error(grw) != kGRWNoError)
    {
        dagutil_verbose_level(1, "SFP module read error!\n");
        return false;
    }
    dagutil_verbose_level(2, "SFP module supports %d Mbps\n", value);
    return true;
}
#endif

static void
line_rate_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port = NULL;
    ComponentPtr root_component = NULL;
    ComponentPtr phy = NULL;
    port_state_t* port_state = NULL;
    uint32_t register_value;
    line_rate_t line_rate = *(line_rate_t*)value;;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        root_component = card_get_root_component(card);
        phy = component_get_subcomponent(root_component, kComponentPhy, 0);
        port = attribute_get_component(attribute);
        port_state = component_get_private_state(port);
        register_value = dag71s_phy_read(phy, 0x8 + (port_state->mIndex * 2));
        if (line_rate == kLineRateOC12c)
        {
            register_value |= BIT2;
        }
        else
        {
            register_value &= ~BIT2;
        }
        /*
        if (check_sfp_line_rate(port, line_rate))
        */
        {
            //dagutil_verbose_level(2, "SFP module supports requested line rate.\n");
            dag71s_phy_write(phy, 0x8 + (port_state->mIndex * 2), register_value);
        }
        /*
        else
        */
        {
            //dagutil_verbose_level(2, "Not setting line rate - SFP module does not support this line rate.\n");
        }
    }
}

void
line_rate_to_string_routine(AttributePtr attribute)
{
    line_rate_t value = *(line_rate_t*)line_rate_get_value(attribute);
    const char* temp = line_rate_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}

void
line_rate_from_string_routine(AttributePtr attribute, const char* string)
{
    line_rate_t value = string_to_line_rate(string);
    if (kLineRateInvalid != value)
        line_rate_set_value(attribute, (void*)&value, sizeof(line_rate_t));
}

static AttributePtr
get_new_core_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeCore);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, core_dispose);
        attribute_set_post_initialize_routine(result, core_post_initialize);
        attribute_set_getvalue_routine(result, core_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "core");
        attribute_set_description(result, "Indeicates whether the SONIC core is active.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
         
    }
    return result;
}

static void
core_dispose(AttributePtr attribute)
{
}

static void
core_post_initialize(AttributePtr attribute)
{
}

static void*
core_get_value(AttributePtr attribute)
{
    ComponentPtr phy = NULL;
    ComponentPtr port = NULL;
    ComponentPtr root = NULL;
    DagCardPtr card = NULL;
    uint32_t register_value;
    static uint8_t value;
    port_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        root = card_get_root_component(card);
        phy = component_get_subcomponent(root, kComponentPhy, 0);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        register_value = dag71s_phy_read(phy, 0x8 + (state->mIndex * 2));
        value = !((register_value & BIT0) == BIT0);
        return (void*)&value;
    }
    return NULL;
}


static AttributePtr
get_new_fifo_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFIFOError);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, fifo_dispose);
        attribute_set_post_initialize_routine(result, fifo_post_initialize);
        attribute_set_getvalue_routine(result, fifo_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "fifo");
        attribute_set_description(result, "Indicates whether a FIFO error has occurred.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
         
    }
    return result;
}

static void
fifo_dispose(AttributePtr attribute)
{
}

static void
fifo_post_initialize(AttributePtr attribute)
{
}

static void*
fifo_get_value(AttributePtr attribute)
{
    ComponentPtr phy = NULL;
    ComponentPtr port = NULL;
    uint32_t register_value;
    static uint8_t value;
    port_state_t* state;
    ComponentPtr root = NULL;
    DagCardPtr card = NULL;
	//daginf_t* dag_card_info;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        root = card_get_root_component(card);
		//dag_card_info = card_get_info(card);
        port = attribute_get_component(attribute);
        phy = component_get_subcomponent(root, kComponentPhy, 0);
        state = component_get_private_state(port);
		/**if(dag_card_info->brd_rev==1){	//This is AMCC1221 framer

			register_value = dag71s_phy_read(phy, 0x17);
			switch(state->mIndex){
				case 0: value = (register_value & BIT4) == BIT4;
				case 1: value = (register_value & BIT5) == BIT5;
				case 2: value = (register_value & BIT6) == BIT6;
				case 3: value = (register_value & BIT7) == BIT7;
			
				default: value =0;
			}
			return (void*)&value;
		}
		//else{		//This is AMCC1213 framer**/
			register_value = dag71s_phy_read(phy, 0x8 + (state->mIndex * 2));
			value = (register_value & BIT5) == BIT5;
			return (void*)&value;
		//}
    }
    return NULL;
}


static AttributePtr
get_new_master_slave_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMasterSlave);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, master_slave_dispose);
        attribute_set_post_initialize_routine(result, master_slave_post_initialize);
        attribute_set_getvalue_routine(result, master_slave_get_value);
        attribute_set_setvalue_routine(result, master_slave_set_value);
        attribute_set_to_string_routine(result, master_slave_to_string_routine);
        attribute_set_from_string_routine(result, master_slave_from_string_routine);
        attribute_set_name(result, "master_or_slave");
        attribute_set_description(result, "set master or slave mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
         
    }
    return result;
}

static void
master_slave_dispose(AttributePtr attribute)
{
}

static void
master_slave_post_initialize(AttributePtr attribute)
{
}

static void*
master_slave_get_value(AttributePtr attribute)
{
    ComponentPtr phy = NULL;
    ComponentPtr port = NULL;
    ComponentPtr root = NULL;
    DagCardPtr card = NULL;
    uint32_t register_value;
    static master_slave_t value;
    port_state_t* state;
	//daginf_t* dag_card_info;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
		//dag_card_info = card_get_info(card);
        root = card_get_root_component(card);
        phy = component_get_subcomponent(root, kComponentPhy, 0);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
		//if(dag_card_info->brd_rev==1){/**This is AMCC1221 framer**/
			//register_value = dag71s_phy_read(phy, 0x08+(state->mIndex*2));
			//value = (register_value & BIT7) ? kSlave: kMaster;
		//}

		//else{/**This is AMCC1213 framer**/
			register_value = dag71s_phy_read(phy, 0x1F);
			value = (register_value & (1 << state->mIndex)) ? kSlave: kMaster;
		//}
        return (void*)&value;
    }
    return NULL;
}

static void
master_slave_set_value(AttributePtr attribute, void* value, int len)
{
    ComponentPtr phy = NULL;
    ComponentPtr port = NULL;
    uint32_t register_value;
    port_state_t* state;
    DagCardPtr card = NULL;
    ComponentPtr root = NULL;
	//daginf_t* dag_card_info;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
		//dag_card_info = card_get_info(card);
        root = card_get_root_component(card);
        phy = component_get_subcomponent(root, kComponentPhy, 0);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
		/* if(dag_card_info->brd_rev==1){ //This is AMCC1221 framer
			register_value = dag71s_phy_read(phy, 0x08+(state->mIndex*2));
		
			if (*(uint8_t*)value == kSlave)
				register_value |=	BIT7;
			else
				register_value &= ~BIT7;

			dag71s_phy_write(phy, 0x08+(state->mIndex*2), register_value);
	    
		}
		//else{	//This is AMCC1213 framer */
			register_value = dag71s_phy_read(phy, 0x1F);
			if (*(uint8_t*)value == kSlave)
				register_value |= (1 << state->mIndex);
			else
				register_value &= ~(1 << state->mIndex);
			dag71s_phy_write(phy, 0x1F, register_value);
		//}//else
    }//valid attribute
}

void
master_slave_to_string_routine(AttributePtr attribute)
{
    master_slave_t value = *(master_slave_t*)master_slave_get_value(attribute);
    const char* temp = master_slave_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}

void
master_slave_from_string_routine(AttributePtr attribute, const char* string)
{
    master_slave_t value = string_to_master_slave(string);
    if (kLineRateInvalid != value)
        master_slave_set_value(attribute, (void*)&value, sizeof(master_slave_t));
}

/* Active status - GPP Enable / Disable */
static AttributePtr
get_new_active_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, active_dispose);
        attribute_set_post_initialize_routine(result, active_post_initialize);
        attribute_set_getvalue_routine(result, active_get_value);
        attribute_set_setvalue_routine(result, active_set_value);
        attribute_set_name(result, "active");
        attribute_set_description(result, "Enable or disable the port.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}


static void
active_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
active_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void*
active_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr component = NULL;
        static uint32_t val = 0;
        port_state_t* state = NULL;
        uint32_t gpp_base = 0;

        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(component);
        gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
        
        val = card_read_iom(card, gpp_base + GPP_CONFIG);
        if( val & BIT4 )
        {
            val = card_read_iom(card, gpp_base + (state->mIndex+1)*SP_OFFSET + SP_CONFIG);
             
            /* The bit is set if the stream is inactive. However this function retrievs the active value,
            not the inactive value. Therefore we negate the result. */
            val = !((val & BIT12) >> 12);
            return (void*)&val;
        }
    }
    return NULL;
}


static void
active_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr component = NULL;
        int val = 0;
        uint32_t gpp_base = 0;
        port_state_t* state = NULL;
        
        component = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(component);

        card = attribute_get_card(attribute);
        
        gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
        val = card_read_iom(card, gpp_base + GPP_CONFIG);
        if( val & BIT4 )
        {
            val = card_read_iom(card, gpp_base + (state->mIndex+1)*SP_OFFSET + SP_CONFIG);
            /* BIT12 contains active-low ("disable"), so invert the user
            request to set "enable" */
            if (0 == *(uint8_t*)value)
            {
                val |= BIT12;
            }
            else
            {
                val &= ~BIT12;
            }
            card_write_iom(card, gpp_base + (state->mIndex+1)*SP_OFFSET + SP_CONFIG, val);
        }
    }
}

