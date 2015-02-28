/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * This file includes the implementation of IPF  component.
 *
* $Id: ipf_component.c,v 1.5 2008/05/09 05:54:27 jomi Exp $
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
#include "../include/components/ipf_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: ";
static const char* const kRevisionString = "$Revision: 1.5 $";

typedef enum
{
    /* Control register*/
    kControlRegister             = 0x00,
    /* Status register */
    kStatusRegister = 0x04,
    /* color memory address */
    kColourMemoryAddress = 0x08,
    /*Colour Value*/
    kColourValue = 0x0C,
} ipf_register_offset_t;

/* function declarations */
static void ipf_dispose(ComponentPtr component);
static void ipf_reset(ComponentPtr component);
static void ipf_default(ComponentPtr component);
static int ipf_post_initialize(ComponentPtr component);
static dag_err_t ipf_update_register_base(ComponentPtr component);

void link_type_to_string(AttributePtr attribute);
static void rule_width_to_string(AttributePtr attribute);

Attribute_t ipf_attr[] = 
{
    {
        /* Name */                 "ipf_enable",
        /* Attribute Code */       kBooleanAttributeIPFEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "To enable/disable IPF functionality",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
        /* Name */                 "drop_enable",
        /* Attribute Code */       kBooleanAttributeIPFDropEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether the IPF will drop packets that are supposed to go to none of the streams",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
#if 0
    {
        /* Name */                 "hash_enable",
        /* Attribute Code */       kBooleanAttributeIPFHashEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether the IPF will embed a hash value in the color",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
#endif 
    {
        /* Name */                 "shift_colour",
        /* Attribute Code */       kBooleanAttributeIPFShiftColour,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether the higher or lower 2 bits of the colour is dropped when hash mode is enabled",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
        /* Name */                 "link_type",
        /* Attribute Code */       kBooleanAttributeIPFLinkType,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether the link type is ethernet or PoS/PPP",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT4, 
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          link_type_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }, 
    {
        /* Name */                 "sel_lctr",
        /* Attribute Code */       kBooleanAttributeIPFSelLctr,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether the colour is embedded with in the loss counter field",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
        /* Name */                 "use_rx_error",
        /* Attribute Code */       kBooleanAttributeIPFUseRXError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether RX error is used to show the pass/drop status",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
        /* Name */                 "sel_rulset_interface_0",
        /* Attribute Code */       kBooleanAttributeIPFRulesetInterface0,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Set the rule to interface 0 - can be used for hot swapping ",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
        /* Name */                 "sel_rulset_interface_1",
        /* Attribute Code */       kBooleanAttributeIPFRulesetInterface1,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Set the rule to interface 1 - can be used for hot swapping ",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kControlRegister,
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
    
    };
/* Number of elements in array */
#define NB_ELEM (sizeof(ipf_attr) / sizeof(Attribute_t))

Attribute_t ipf_status_attr[] =
{
    {
        /* Name */                 "ipv4_support",
        /* Attribute Code */       kBooleanAttributeIPFV4Support,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Shows IPFV4 is supported or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
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
        /* Name */                 "ipv6_support",
        /* Attribute Code */       kBooleanAttributeIPFV6Support,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Shows IPFV6 is supported or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
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
        /* Name */                 "vlan_skipping",
        /* Attribute Code */       kBooleanAttributeVLANSkipping,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "VLAN skipping supported or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
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
        /* Name */                 "vlan_filtering",
        /* Attribute Code */       kBooleanAttributeVLANFiltering,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "VLAN filtering supported or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
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
        /* Name */                 "vlan_tags",
        /* Attribute Code */       kBooleanAttributeVLANTags,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "VLAN tags",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
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
        /* Name */                 "mpls_skipping",
        /* Attribute Code */       kBooleanAttributeMPLSSkipping,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "MPLS skipping supported or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
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
        /* Name */                 "mpls_filtering",
        /* Attribute Code */       kBooleanAttributeMPLSFiltering,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "MPLS filtering supported or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
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
    {
        /* Name */                 "rule_width",
        /* Attribute Code */       kBooleanAttributeIPFRuleWidth,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "width of the rules",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT7, 
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          rule_width_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

};

/* Number of elements in status attribute array */
#define NB_STATUS_ELEM (sizeof(ipf_status_attr) / sizeof(Attribute_t))

ComponentPtr
ipf_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentIPF, card); 
    ipf_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, ipf_dispose);
        component_set_post_initialize_routine(result, ipf_post_initialize);
        component_set_reset_routine(result, ipf_reset);
        component_set_default_routine(result, ipf_default);
        component_set_update_register_base_routine(result, ipf_update_register_base);
        component_set_name(result, "ipf");
	    component_set_description(result,"Packet Filtering component");
        state = (ipf_state_t*)malloc(sizeof(ipf_state_t));
        state->mIndex = index;
        state->mBase = card_get_register_address(card, DAG_REG_STREAM_FTR, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
ipf_dispose(ComponentPtr component)
{
}

static dag_err_t
ipf_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        ipf_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_PPF, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
ipf_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        ipf_state_t* state = NULL;
        uint32_t version = 0;
        DagCardPtr card;

        card = component_get_card(component);
        state = component_get_private_state(component);
        read_attr_array(component, ipf_attr, NB_ELEM, state->mIndex);
        version = card_get_register_version(card, DAG_REG_PPF, state->mIndex);
        if ( version >= 3 )
            read_attr_array(component, ipf_status_attr, NB_STATUS_ELEM, state->mIndex);

        return 1;
    }
    return kDagErrInvalidParameter;
}

static void
ipf_reset(ComponentPtr component)
{   
    if (1 == valid_component(component))
    {
        /* there is no reset bit available */
        ipf_default(component);
    }    
}


static void
ipf_default(ComponentPtr component)
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

void 
link_type_to_string(AttributePtr attribute)
{
    uint8_t value; 
    uint8_t *value_ptr = (uint8_t*)attribute_get_value(attribute); 
    char buffer[32];

    if (1 == valid_attribute(attribute))
    {
        assert(attribute_get_valuetype(attribute) == kAttributeBoolean);
        if( value_ptr == NULL )
        {
            strcpy(buffer,"error");
            attribute_set_to_string(attribute, buffer); 
            return;
        }
        value = *value_ptr;
        if (value == 1)
        {
            strcpy(buffer, "ethernet");
        }
        else if (value == 0)
        {
            strcpy(buffer, "PoS or PPP");
        }
        attribute_set_to_string(attribute, buffer); 
    }
}

void 
rule_width_to_string(AttributePtr attribute)
{
    uint8_t value; 
    uint8_t *value_ptr = (uint8_t*)attribute_get_value(attribute); 
    char buffer[32];

    if (1 == valid_attribute(attribute))
    {
        assert(attribute_get_valuetype(attribute) == kAttributeBoolean);
        if( value_ptr == NULL )
        {
            strcpy(buffer,"error");
            attribute_set_to_string(attribute, buffer); 
            return;
        }
        value = *value_ptr;
        if (value == 1)
        {
            strcpy(buffer, "144/576 bits");
        }
        else if (value == 0)
        {
            strcpy(buffer, "Only 144bit");
        }
        attribute_set_to_string(attribute, buffer); 
    }
}
