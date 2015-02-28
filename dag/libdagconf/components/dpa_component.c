/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dpa_component.c,v 1.7 2006/10/24 05:09:53 lipi Exp $
 */

#include "dagutil.h"

#include "../include/attribute.h"
#include "../include/util/set.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"

#define THSD 1000 

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: dpa_component.c,v 1.7 2006/10/24 05:09:53 lipi Exp $";
static const char* const kRevisionString = "$Revision: 1.7 $";
static void dpa_dispose(ComponentPtr component);
static void dpa_reset(ComponentPtr component);
static void dpa_default(ComponentPtr component);
static int dpa_post_initialize(ComponentPtr component);
static dag_err_t dpa_update_register_base(ComponentPtr component);


/**
 * DPA (Dynamic Phase Alignment) Attribute definition array 
 */
Attribute_t dpa_attr[]=
{
    {
        /* Name */                 "dpa_fault",
        /* Attribute Code */       kBooleanAttributeDPAFault,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Dynamic Phase Alignment Fault",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    8,
        /* Register Address */     DAG_REG_DPA,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT8,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "dpa_start",
        /* Attribute Code */       kBooleanAttributeDPAStart,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Dynamic Phase Alignment Start",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    10,
        /* Register Address */     DAG_REG_DPA,
        /* Offset */               0,
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

    {
        /* Name */                 "dpa_done",
        /* Attribute Code */       kBooleanAttributeDPADone,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Asserted when DPA has finished",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    11,
        /* Register Address */     DAG_REG_DPA,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT11,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "dpa_dcm_error",
        /* Attribute Code */       kBooleanAttributeDPADcmError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "DCM error during alignment",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    12,
        /* Register Address */     DAG_REG_DPA,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT12,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "dpa_dcm_locked",
        /* Attribute Code */       kBooleanAttributeDPADcmLock,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Dynamic Phase Alignment Locked",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    13,
        /* Register Address */     DAG_REG_DPA,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT13,
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
#define NB_ELEM (sizeof(dpa_attr) / sizeof(Attribute_t))

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} dpa_state_t;

/* terf component. */
ComponentPtr
dpa_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentDPA, card); 
    dpa_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dpa_dispose);
        component_set_post_initialize_routine(result, dpa_post_initialize);
        component_set_reset_routine(result, dpa_reset);
        component_set_default_routine(result, dpa_default);
        component_set_update_register_base_routine(result, dpa_update_register_base);
        component_set_name(result, "dpa");
        state = (dpa_state_t*)malloc(sizeof(dpa_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
dpa_dispose(ComponentPtr component)
{
}

static dag_err_t
dpa_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        dpa_state_t* state = NULL;
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
dpa_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        dpa_state_t* state = NULL;
	
        state = component_get_private_state(component);
        /* Add attribute of dpa */ 
        read_attr_array(component, dpa_attr, NB_ELEM, state->mIndex);
        
	return 1;
    }
    return kDagErrInvalidParameter;
}

static void
dpa_reset(ComponentPtr component)
{   
    uint8_t val = 0;
    int  i=0, j=0;
    int done = 0;
    int fault = 1, error = 1;
    DagCardPtr card = NULL;
    ComponentPtr root = NULL, amcc = NULL;
    AttributePtr a_eql = NULL, a_dcm_lock = NULL;
    int lock = 0;
    
    if (1 == valid_component(component))
    {
	/* Get the card */
        card = component_get_card(component);
	/* Get the root component */
	root = card_get_root_component(card);
    
	/* Put the card ports in loopback mode */
	val = 1;
	amcc = component_get_subcomponent(root, kComponentPort, 0);
	a_eql = component_get_attribute(amcc, kBooleanAttributeEquipmentLoopback);
	attribute_set_value(a_eql, (void*)&val, 1);

        amcc = component_get_subcomponent(root, kComponentPort, 1);
        a_eql = component_get_attribute(amcc, kBooleanAttributeEquipmentLoopback);
        attribute_set_value(a_eql, (void*)&val, 1);
	    
	/* loop for phy reset */
	a_dcm_lock = component_get_attribute(component, kBooleanAttributeDPADcmLock);

	lock = *(int *)attribute_get_value(a_dcm_lock);
	while ((lock == 0) && j < 5)
	{
		/* reset Phy */
		component_reset(amcc);
		dagutil_microsleep(THSD);
		lock = *(int *)attribute_get_value(a_dcm_lock);
		j++;
	}
	if (lock)
	{
		AttributePtr a_dpa_start = component_get_attribute(component, kBooleanAttributeDPAStart);
		AttributePtr a_dpa_done = component_get_attribute(component, kBooleanAttributeDPADone);
		AttributePtr a_dpa_fault = component_get_attribute(component, kBooleanAttributeDPAFault);
		AttributePtr a_dcm_error = component_get_attribute(component, kBooleanAttributeDPADcmError);
		
		/* While the DPA failed and less than 6 attempts */
		while((fault || error) && i < 6)
		{
			/* start DPA */
			val = 1;
			attribute_set_value(a_dpa_start, (void*)&val, 1);
			dagutil_microsleep(THSD);
			done = *(int *)attribute_get_value(a_dpa_done);
			/*if DPA has finished */
			if(done)
			{
				/* Check dpa result */
				fault = *(int *)attribute_get_value(a_dpa_fault);
				error = *(int *)attribute_get_value(a_dcm_error);
			}
			i++;
		}
		/* if DPA failed */
		if (fault || error)
			dagutil_panic("## Error: DPA failed. Card may not function correctly - Please contact <support@endace.com> for assistance.\n");
		else
			dagutil_verbose("*** Dynamic Phase Alignment succeed ***\n");
	}
	else
		dagutil_warning("DCM not lock after reseting\n");

	/* set up the card ports to no loopback */
	val = 0;
	amcc = component_get_subcomponent(root, kComponentPort, 0);
	a_eql = component_get_attribute(amcc, kBooleanAttributeEquipmentLoopback); 
	attribute_set_value(a_eql, (void*)&val, 1);
	
	amcc = component_get_subcomponent(root, kComponentPort, 1);
	a_eql = component_get_attribute(amcc, kBooleanAttributeEquipmentLoopback); 
	attribute_set_value(a_eql, (void*)&val, 1);
	
    }
}


static void
dpa_default(ComponentPtr component)
{

    //set all the attribute default values
    if (1 == valid_component(component))
    {
        AttributePtr a_done = NULL;
		int done = 0;		
        a_done = component_get_attribute(component, kBooleanAttributeDPADone);
        done = *(int*)attribute_get_value(a_done);
		
		if (done == 0)
			/* start dpa */
			dpa_reset(component);		

    }	
}


