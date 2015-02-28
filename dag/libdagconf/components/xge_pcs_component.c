/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *
 * This component is used by DAG 5.2 SXA - probably others will follow.
 *
 * $Id: xge_pcs_component.c,v 1.9 2008/10/24 00:50:02 vladimir Exp $
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
#include "../include/components/xge_pcs_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"


/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: xge_pcs_component.c,v 1.9 2008/10/24 00:50:02 vladimir Exp $";
static const char* const kRevisionString = "$Revision: 1.9 $";

static void xge_pcs_dispose(ComponentPtr component);
static void xge_pcs_reset(ComponentPtr component);
static void xge_pcs_default(ComponentPtr component);
static int xge_pcs_post_initialize(ComponentPtr component);
static dag_err_t xge_pcs_update_register_base(ComponentPtr component);

typedef enum
{
    // SONET Deframer/Framer
    kControl            = 0x00,
    kStatus             = 0x04,
    kBitErrorRate 	= 0x08,
    kBlockError		= 0x0c,
 
} xge_register_offset_t;

/**
 * XGE Physical Coding Sublayer Attribute definition array 
 */
Attribute_t xge_pcs_attr[]=
{


    {
        /* Name */                 "reset",
        /* Attribute Code */       kBooleanAttributeReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Reset",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    31,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT31,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "hi_ber",
        /* Attribute Code */       kBooleanAttributeHiBER,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "High BER",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    8,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kBitErrorRate,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT8,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "ber_counter",
        /* Attribute Code */       kUint32AttributeBERCounter,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Bit error rate counter",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kBitErrorRate,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x003F,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    
   {
        /* Name */                 "lock",
        /* Attribute Code */       kBooleanAttributeLock,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Signal Lock if 1",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    8,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kBlockError,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT8,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "block_err_count",
        /* Attribute Code */       kUint32AttributeErrorBlockCounter,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Block Error counter",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kBlockError,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x00FF,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "tx_fault",
        /* Attribute Code */       kBooleanAttributeTxFault,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Transmit signal fault",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kStatus,
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

    {
        /* Name */                 "rx_fault",
        /* Attribute Code */       kBooleanAttributeRxFault,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Receive signal fault",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    1,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT1,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
  
    {
        /* Name */                 "tx_fifo_overflow",
        /* Attribute Code */       kBooleanAttributeTxFIFOOverflow,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Transmit FIFO overflow",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kStatus,
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

    {
        /* Name */                 "rx_fifo_overflow",
        /* Attribute Code */       kBooleanAttributeRxFIFOOverflow,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Receive FIFO overflow",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    3,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kStatus,
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
        /* Name */                 "link",
        /* Attribute Code */       kBooleanAttributeLink,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Rx Link status",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    4,
        /* Register Address */     DAG_REG_XGE_PCS,
        /* Offset */               kStatus,
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


};

/* Number of elements in array */
#define NB_ELEM (sizeof(xge_pcs_attr) / sizeof(Attribute_t))

ComponentPtr
xge_pcs_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPCS, card); 
    xge_pcs_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, xge_pcs_dispose);
        component_set_post_initialize_routine(result, xge_pcs_post_initialize);
        component_set_reset_routine(result, xge_pcs_reset);
        component_set_default_routine(result, xge_pcs_default);
        component_set_update_register_base_routine(result, xge_pcs_update_register_base);
        component_set_name(result, "xge_pcs");
        state = (xge_pcs_state_t*)malloc(sizeof(xge_pcs_state_t));
        state->mIndex = index;
	state->mBase = card_get_register_address(card, DAG_REG_XGE_PCS, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
xge_pcs_dispose(ComponentPtr component)
{
}

static dag_err_t
xge_pcs_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        xge_pcs_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_XGE_PCS, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
xge_pcs_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        xge_pcs_state_t* state = NULL;
//	AttributePtr attribute = NULL;
//    	GenericReadWritePtr grw = NULL;  

        state = component_get_private_state(component);
        /* Add attribute of xge_pcs */ 
        read_attr_array(component, xge_pcs_attr, NB_ELEM, state->mIndex);

	return 1;
    }
    return kDagErrInvalidParameter;
}

static void
xge_pcs_reset(ComponentPtr component)
{   
    AttributePtr attribute = NULL;
    void * val;
    
    if (1 == valid_component(component))
    {
        /* there is no reset bit available */
        xge_pcs_default(component);
	//reset PCS 
#if 0 
	printf("PCS - RESET \n");
#endif 	
	attribute = component_get_attribute(component,kBooleanAttributeReset);
	//sets the reset for about 10ms may be 1 ms should be enough 
	val = (void *)1;
        attribute_set_value(attribute, &val, 1);
	dagutil_microsleep(10000);
	//clear the PCS Reset 
	val = (void *)0;
        attribute_set_value(attribute, &val, 1);
    }	
}


static void
xge_pcs_default(ComponentPtr component)
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

