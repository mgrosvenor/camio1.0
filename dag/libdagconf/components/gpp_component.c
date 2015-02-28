/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
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
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/gpp_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"

/* DAG API files */
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
    uint32_t mGppBase;
    uint32_t mIndex;
} gpp_state_t;

void* gpp_interfacecount_get_value(AttributePtr attribute);
/* Attribute array - Does not contain all the attributes */
Attribute_t gpp_attr[] = 
{
    {
        /* Name */                 "interface_count",
        /* Attribute Code */       kUint32AttributeInterfaceCount,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of interfaces in the card",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    6,
        /* Register Address */     DAG_REG_GPP,
        /* Offset */               SP_STATUS,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 (BIT6 | BIT7),
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             gpp_interfacecount_get_value, 
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }, 
};
/* Number of elements in array */
#define NB_ELEM (sizeof(gpp_attr) / sizeof(Attribute_t))

/* gpp component. */
static void gpp_dispose(ComponentPtr component);
static void gpp_reset(ComponentPtr component);
static void gpp_default(ComponentPtr component);
static int gpp_post_initialize(ComponentPtr component);
static dag_err_t gpp_update_register_base(ComponentPtr component);

/* varlen attribute. */
static AttributePtr gpp_get_new_varlen_attribute(void);
static void gpp_varlen_dispose(AttributePtr attribute);
static void* gpp_varlen_get_value(AttributePtr attribute);
static void gpp_varlen_set_value(AttributePtr attribute, void* value, int length);
static void gpp_varlen_post_initialize(AttributePtr attribute);

static AttributePtr gpp_get_new_align64_attribute(void);
static void gpp_align64_dispose(AttributePtr attribute);
static void* gpp_align64_get_value(AttributePtr attribute);
static void gpp_align64_set_value(AttributePtr attribute, void* value, int length);
static void gpp_align64_post_initialize(AttributePtr attribute);
static void gpp_create_drop_count_attributes( ComponentPtr component, int count );
/*karthik.sharma*/
static void gpp_create_port_enable_attributes(ComponentPtr component, int count );

ComponentPtr
gpp_get_new_gpp(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentGpp, card); 
    char buffer[BUFFER_SIZE];
    gpp_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, gpp_dispose);
        component_set_post_initialize_routine(result, gpp_post_initialize);
        component_set_reset_routine(result, gpp_reset);
        component_set_default_routine(result, gpp_default);
        component_set_update_register_base_routine(result, gpp_update_register_base);
        sprintf(buffer, "gpp%u", index);
        component_set_name(result, buffer);
        state = (gpp_state_t*)malloc(sizeof(gpp_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
gpp_dispose(ComponentPtr component)
{
}

static int
gpp_post_initialize(ComponentPtr component)
{
    AttributePtr varlen;
    AttributePtr snaplen;
    AttributePtr align64;
    AttributePtr interface_count = NULL;
    gpp_state_t* state = NULL;
    DagCardPtr card = NULL;
    uint32_t mask;
    uintptr_t address;
    GenericReadWritePtr grw = NULL;
    uint32_t *ptr_count = NULL;

    state = component_get_private_state(component);
    card = component_get_card(component);
    /* The setting of the base address for this component (and all others) has to happen in
     * the post_initialize function because the card's register address information
     * is needed. This information is set after the card is initialized (see dag_config_init)
     */
    state->mGppBase = card_get_register_address(card, DAG_REG_GPP, state->mIndex);

    varlen = gpp_get_new_varlen_attribute();
    mask = 0xffff;
    card = component_get_card(component);
    address = (uintptr_t)card_get_iom_address(card) + state->mGppBase + GPP_SNAPLEN;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    snaplen  = attribute_factory_make_attribute(kUint32AttributeSnaplength, grw, &mask, 1);
    
    align64 = gpp_get_new_align64_attribute();

    component_add_attribute(component, varlen);
    component_add_attribute(component, snaplen);
    component_add_attribute(component, align64);

    /* create and add attributes defined in gpp_attr */
    read_attr_array(component, gpp_attr, NB_ELEM, state->mIndex);
    
    /* Add drop count attributes here-depending upon the interfce count*/
    interface_count = component_get_attribute(component, kUint32AttributeInterfaceCount);
    if ( NULL != (ptr_count = (uint32_t*)gpp_interfacecount_get_value(interface_count) ) && *ptr_count )
    {
        gpp_create_drop_count_attributes( component, *ptr_count );
        
    }
    /*Add Port Enable attributes here - depending on the port count*/
    if(NULL != (ptr_count = (uint32_t*)gpp_interfacecount_get_value(interface_count)) && *ptr_count)
    {
        gpp_create_port_enable_attributes(component,*ptr_count);
    }

    return 1;
}

static dag_err_t
gpp_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        gpp_state_t* state = NULL;
        DagCardPtr card = NULL;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mGppBase = card_get_register_address(card, DAG_REG_GPP, state->mIndex);

        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static void
gpp_reset(ComponentPtr component)
{
    DagCardPtr card = NULL;
    uint32_t register_val = 0;
    card = component_get_card(component);
    register_val = card_read_iom(card, 0x88);
    register_val |= BIT31;
    card_write_iom(card, 0x88, register_val);
    
}


static void
gpp_default(ComponentPtr component)
{
	DagCardPtr card = NULL;
    uint32_t register_val = 0;
	gpp_state_t* state = NULL;

 	if (1 == valid_component(component))
    { 
		card = component_get_card(component);
		state = (gpp_state_t*)component_get_private_state(component);
		register_val = card_read_iom(card, state->mGppBase);        
		register_val &= ~BIT12;
		card_write_iom(card, state->mGppBase, register_val);
	}
}

static AttributePtr
gpp_get_new_varlen_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeVarlen); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, gpp_varlen_dispose);
        attribute_set_post_initialize_routine(result, gpp_varlen_post_initialize);
        attribute_set_getvalue_routine(result, gpp_varlen_get_value);
        attribute_set_setvalue_routine(result, gpp_varlen_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "varlen");
        attribute_set_description(result, "Enable or disable variable length capture");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
gpp_varlen_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
gpp_varlen_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void*
gpp_varlen_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint8_t val = 0;
        uint32_t reg_val = 0;
        gpp_state_t* state = NULL;
        ComponentPtr gpp;

        card = attribute_get_card(attribute);
        gpp = attribute_get_component(attribute);
        state = (gpp_state_t*)component_get_private_state(gpp);
        reg_val = card_read_iom(card, state->mGppBase);
        val = (reg_val & BIT0) == BIT0;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    
    return NULL;
}


static void
gpp_varlen_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr gpp;
        gpp_state_t* state = NULL;
        uint32_t val;

        card = attribute_get_card(attribute);
        gpp = attribute_get_component(attribute);
        state = (gpp_state_t*)component_get_private_state(gpp);
        val = card_read_iom(card, state->mGppBase);
        if (0 == *(uint8_t*)value)
        {
            val &= ~BIT0;
        }
        else
        {
            val |= BIT0;
        }
        card_write_iom(card, state->mGppBase, val);
        /* Perform an L2 reset */
        component_reset(gpp);
    }
}

int
gpp_record_type_set_value(ComponentPtr gpp, gpp_record_type_t record_type)
{
    DagCardPtr card = component_get_card(gpp);
    gpp_state_t* state = (gpp_state_t*)component_get_private_state(gpp);
    uint32_t control = card_read_iom(card, state->mGppBase + GPP_CONFIG);

    if (1 == valid_component(gpp))
    {
        /* Clear Record Type and Record Type Source */
        control &= ~0xff0006;

        control |= 0x04;
        control |= ((record_type & 0xff) << 16);

        card_write_iom(card, state->mGppBase + GPP_CONFIG, control);
        /* verify */
        control = card_read_iom(card, state->mGppBase + GPP_CONFIG);
        if ( !(control & 0x04) )
            return 0; /* older gpp, fail silently */
        if ( ((control >> 16) & 0xff) != record_type )
            return 1;
    }
    return 0;
}

static AttributePtr
gpp_get_new_align64_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAlign64); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, gpp_align64_dispose);
        attribute_set_post_initialize_routine(result, gpp_align64_post_initialize);
        attribute_set_getvalue_routine(result, gpp_align64_get_value);
        attribute_set_setvalue_routine(result, gpp_align64_set_value);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "align64");
        attribute_set_description(result, "Align/pad the received ERF record to the next 64-bit boundary.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
gpp_align64_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
gpp_align64_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
gpp_align64_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr gpp = attribute_get_component(attribute);
        gpp_state_t* state = (gpp_state_t*)component_get_private_state(gpp);
        uint32_t val = 0;
        val = card_read_iom(card, state->mGppBase + GPP_CONFIG);
        val = (val & BIT3) >> 3;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    
    return NULL;
}

static void
gpp_align64_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr gpp = attribute_get_component(attribute);
        gpp_state_t* state = (gpp_state_t*)component_get_private_state(gpp);
        uint32_t val = card_read_iom(card, state->mGppBase + GPP_CONFIG);
        if (0 == *(uint8_t*)value)
        {
            val &= ~BIT3;
        }
        else
        {
            val |= BIT3;
        }
        card_write_iom(card, state->mGppBase + GPP_CONFIG, val);
        /* Perform an L2 reset */
        component_reset(gpp);        
    }
}

void
gpp_active_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
	DagCardPtr card = NULL;
        int val = 0;
        AttributePtr interface_count = NULL;
        uint32_t *ptr_count = NULL;
        uintptr_t gpp_base = 0;
        ComponentPtr gpp_component = NULL;
	gpp_state_t *state = NULL;

        card = attribute_get_card(attribute);
        gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
        val = card_read_iom(card, gpp_base + GPP_CONFIG);
        /* per port  config registers exist? */
        if( val & BIT4 )
        {
            val |= BIT5; /* enable per-stream config registers */
            card_write_iom(card, gpp_base + GPP_CONFIG, val);
            attribute_boolean_set_value(attribute, value, length);
        }
        else
        {
	    card = attribute_get_card(attribute);
            gpp_component = attribute_get_component(attribute);
            state = component_get_private_state(gpp_component);
            gpp_base = state->mGppBase;
            val = card_read_iom(card, gpp_base + GPP_CONFIG);

            /* Get the number of ports and check if it is 1*/
            interface_count = component_get_attribute(gpp_component, kUint32AttributeInterfaceCount);
            if ( NULL != (ptr_count = (uint32_t*)gpp_interfacecount_get_value(interface_count) ) && (*ptr_count == 1) )
            {
                /* only one port - so use the Global Control register ( common to all ports ) write 0/1 on bit12 for  enable/disable */
                val = (*(uint8_t*)value)? (val & ~BIT12): (val | BIT12);
                card_write_iom(card,gpp_base + GPP_CONFIG, val);
            }
        }
    }
}

void*
gpp_active_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
	 DagCardPtr card = NULL;
        int val = 0;
        uintptr_t gpp_base = 0;
        uint8_t bool_val = 1;
        AttributePtr interface_count = NULL;
        uint32_t *ptr_count = NULL;
        ComponentPtr gpp_component = NULL;
	gpp_state_t *state = NULL;

        card = attribute_get_card(attribute);	
        gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
        val = card_read_iom(card, gpp_base + GPP_CONFIG);
        /* per port  config registers exist? */
        if( val & BIT4 )
        {
            return attribute_boolean_get_value(attribute);
        }
        else
        {
            gpp_component = attribute_get_component(attribute);
            state = component_get_private_state(gpp_component);
            gpp_base = state->mGppBase;
            val = card_read_iom(card, gpp_base + GPP_CONFIG);

            interface_count = component_get_attribute(gpp_component, kUint32AttributeInterfaceCount);
            /* get the interface count and check whether it is 1 */
            if ( NULL != (ptr_count = (uint32_t*)gpp_interfacecount_get_value(interface_count) ) && (*ptr_count == 1) )
            {
                /* only one port - so use the Global Control register ( common to all ports ) */
                if (val & BIT12)
                {
                    bool_val = 0;
                }
                attribute_set_value_array(attribute, &bool_val, sizeof(bool_val));
                return attribute_get_value_array(attribute);
            }
	}
    }

    return NULL;
}

void 
gpp_create_drop_count_attributes( ComponentPtr component,int count )
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
    gpp_state_t *state = NULL;
    uint32_t index = 0;

    for ( index = 0; index < count; index++)
    {
    
        state = component_get_private_state(component); 
        num_module = state->mIndex;
        sprintf(name,"drop_count%d",index);
        
        card = component_get_card(component);
        offset =  (index + 1 ) * SP_OFFSET + SP_DROP; 
        base_reg = card_get_register_address(card, DAG_REG_GPP, num_module);
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
gpp_create_port_enable_attributes(ComponentPtr component,int count)
{
    AttributePtr result = NULL;
    char name[256];
    uintptr_t offset = 0;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t mask = BIT12;
    int length = 1;
    uint32_t num_module = 0;
    gpp_state_t* state = NULL;
    uint32_t index = 0;

    for(index = 0;index < count;index++)
    {
        state = component_get_private_state(component);
        num_module = state->mIndex;
  
        card = component_get_card(component);
        offset = (index + 1) * SP_OFFSET + (uintptr_t)SP_CONFIG;
        base_reg = card_get_register_address(card,DAG_REG_GPP,num_module);
	address = ((uintptr_t)card_get_iom_address(card) + base_reg + offset);
        grw = grw_init(card,address,grw_iom_read,grw_iom_write); 
        grw_set_on_operation(grw,kGRWClearBit);
        result = attribute_factory_make_attribute(kBooleanAttributeActive,grw,&mask,length);
	
      	sprintf(name,"active%d",index);
        attribute_set_name(result, name);
      	sprintf(name,"Enable/Disable Port%d",index);
        attribute_set_description(result, name);

	attribute_set_getvalue_routine(result,gpp_active_get_value);
        attribute_set_setvalue_routine(result,gpp_active_set_value);
        
	component_add_attribute(component,result);
    }


}
void* 
gpp_interfacecount_get_value(AttributePtr attribute)
{
    uint32_t *ptr_value  = NULL;
    ptr_value = (uint32_t*) attribute_uint32_get_value(attribute);
    if ( NULL == ptr_value )
    {
         return ptr_value;
    }
    *ptr_value = *ptr_value + 1;
    return ptr_value;
}
