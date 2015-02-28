/*
 * Copyright (c) 2008 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * This file includes the implementation of GTP phy component.
 * 
 *
* $Id: gtp_phy_component.c,v 1.2 2008/05/09 05:54:27 jomi Exp $
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
#include "../include/components/gtp_phy_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"


typedef enum
{
    /* Status and control Registers*/
    kStatusControl             = 0x00,
    
} gtp_phy_register_offset_t;

/* function declarations */
static void gtp_phy_dispose(ComponentPtr component);
static void gtp_phy_reset(ComponentPtr component);
static void gtp_phy_default(ComponentPtr component);
static int gtp_phy_post_initialize(ComponentPtr component);
static dag_err_t gtp_phy_update_register_base(ComponentPtr component);


Attribute_t gtp_phy_attr[] = 
{
    {
        /* Name */                 "gtp_reset",
        /* Attribute Code */       kBooleanAttributeGTPReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Reset the GTP module.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
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
        /* Name */                 "gtp_rx_reset",
        /* Attribute Code */       kBooleanAttributeGTPRxReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Reset the RX data path of the GTP module",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
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
        /* Name */                 "gtp_tx_reset",
        /* Attribute Code */       kBooleanAttributeGTPTxReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Reset the TX data path of the GTP module",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
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
        /* Name */                 "gtp_pma_reset",
        /* Attribute Code */       kBooleanAttributeGTPPMAReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Reset the TX data path of the GTP module",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
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
        /* Name */                 "fcl",
        /* Attribute Code */       kBooleanAttributeFacilityLoopback,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Setting of FCL loopback mode",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
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
    {
        /* Name */                 "eql",
        /* Attribute Code */       kBooleanAttributeEquipmentLoopback,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Setting of EQL loopback mode",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT5,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "pll_lock",
        /* Attribute Code */       kBooleanAttributePLLDetect,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether PLL in the GTP locked or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT16,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "lock",
        /* Attribute Code */       kBooleanAttributeLock,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether locked to Comma sequence ",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
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
        /* Name */                 "los",
        /* Attribute Code */       kBooleanAttributeLossOfSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether GTP has detected signal or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
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
    {
        /* Name */                 "rx_fifo_overflow",
        /* Attribute Code */       kBooleanAttributeRxFIFOOverflow,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Receive FIFO overflow",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT19,
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
        /* Description */          "Receive FIFO overflow",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT20,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
     {
        /* Name */                 "tx_fifo_error",
        /* Attribute Code */       kBooleanAttributeFIFOError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Receive FIFO overflow",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT21,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "reset_done",
        /* Attribute Code */       kBooleanAttributeGTPResetDone,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether reset  has finished",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GTP_PHY,
        /* Offset */               kStatusControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT22,
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
#define NB_ELEM (sizeof(gtp_phy_attr) / sizeof(Attribute_t))

ComponentPtr
gtp_phy_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPhy, card); 
    gtp_phy_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, gtp_phy_dispose);
        component_set_post_initialize_routine(result, gtp_phy_post_initialize);
        component_set_reset_routine(result, gtp_phy_reset);
        component_set_default_routine(result, gtp_phy_default);
        component_set_update_register_base_routine(result, gtp_phy_update_register_base);
        component_set_name(result, "gtp_phy");
        state = (gtp_phy_state_t*)malloc(sizeof(gtp_phy_state_t));
        state->mIndex = index;
        state->mBase = card_get_register_address(card, DAG_REG_GTP_PHY, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
gtp_phy_dispose(ComponentPtr component)
{
}

static dag_err_t
gtp_phy_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        gtp_phy_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_GTP_PHY, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
gtp_phy_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        gtp_phy_state_t* state = NULL;
        state = component_get_private_state(component);
        read_attr_array(component, gtp_phy_attr, NB_ELEM, state->mIndex);
        return 1;
    }
    return kDagErrInvalidParameter;
}

static void
gtp_phy_reset(ComponentPtr component)
{   
    if (1 == valid_component(component))
    {
        /* there is no reset bit available */
        gtp_phy_default(component);
    }    
}


static void
gtp_phy_default(ComponentPtr component)
{
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



