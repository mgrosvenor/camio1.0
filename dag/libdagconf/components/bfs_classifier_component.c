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
#include "../include/components/bfs_component.h"
/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: ";
static const char* const kRevisionString = "$Revision: 1.11.2.3 $";

typedef enum
{
		/* Control register*/
    	kControlRegister = 0x00,
    	/* Status register */
    	kStatusRegister  = 0x04,
    	/* color memory address*/
		kSramMissValueRegister = 0x1c
} ipf_register_offset_t;

/* function declarations */
static void bfs_dispose(ComponentPtr component);
static void bfs_reset(ComponentPtr component);
static void bfs_default(ComponentPtr component);
static int bfs_post_initialize(ComponentPtr component);
static dag_err_t bfs_update_register_base(ComponentPtr component);
#if 0
static int get_interface_count(ComponentPtr component);
#endif
#if 0
static AttributePtr ruleset_use_for_iface_zero_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr ruleset_use_for_iface_1_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr ruleset_use_for_iface_2_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr ruleset_use_for_iface_3_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
#endif
static AttributePtr activate_bank_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);


static void link_type_to_string(AttributePtr attribute);

void bank_activate_set_value(AttributePtr attribute,void *value,int len);
void* bank_activate_get_value(AttributePtr attribute);

Attribute_t bfs_attr[] = 
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
        /* Name */                 "link_type",
        /* Attribute Code */       kBooleanAttributeIPFLinkType,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates whether the link type is ethernet = 1 or PoS/PPP = 0",
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
        /* Name */                 "sram_miss_value",
        /* Attribute Code */       kUint32AttributeSRamMissValue,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Value that replaces the associated SRAM value if a search misses.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_BFSCAM,
        /* Offset */               kSramMissValueRegister,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xffffffff, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	{
        /* Name */                 "bfs_active_bank",
        /* Attribute Code */       kBooleanAttributeBfsActiveBank,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates the active BFS Bank.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_BFSCAM,
        /* Offset */               0x00,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT28, 
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }
};
/* Number of elements in array */
#define NB_ELEM (sizeof(bfs_attr) / sizeof(Attribute_t))
/* Number of elements in array */
Attribute_t bfs_status_attr[] =
{
	{
        /* Name */                 "vlan_support",
        /* Attribute Code */       kBooleanAttributeVLANFiltering,
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
        /* Name */                 "mpls_support",
        /* Attribute Code */       kBooleanAttributeMPLSFiltering,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Shows MPLS is supported or not",
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
        /* Name */                 "ipfv6_skipping",
        /* Attribute Code */       kBooleanAttributeIPFV6Support,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Shows IPFv6 supported or not.",
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
        /* Name */                 "rule_width",
        /* Attribute Code */       kUint32AttributeBFSRuleWidth,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "width of the rules",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_PPF,
        /* Offset */               kStatusRegister,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xffff0000, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};
/* Number of elements in status attribute array */
#define NB_STATUS_ELEM (sizeof(bfs_status_attr) / sizeof(Attribute_t))

ComponentPtr
bfs_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentIPF, card); 
    bfs_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, bfs_dispose);
        component_set_post_initialize_routine(result, bfs_post_initialize);
        component_set_reset_routine(result, bfs_reset);
        component_set_default_routine(result, bfs_default);
        component_set_update_register_base_routine(result, bfs_update_register_base);
        component_set_name(result, "BFS");
	component_set_description(result,"BFS Packet Filtering component");
        state = (bfs_state_t*)malloc(sizeof(bfs_state_t));
        state->mIndex = index;
        state->mBase = card_get_register_address(card, DAG_REG_STREAM_FTR, state->mIndex);
        component_set_private_state(result, state);
    }
    return result;
}
static dag_err_t
bfs_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        bfs_state_t* state = NULL;
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
bfs_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        bfs_state_t* state = NULL;
        uint32_t version = 0;
        DagCardPtr card;
	GenericReadWritePtr grw = NULL;
		
		uint32_t mask = 0;
		AttributePtr attr = NULL;
		uintptr_t address = 0;
	
        card = component_get_card(component);
        state = component_get_private_state(component);
        read_attr_array(component, bfs_attr, NB_ELEM, state->mIndex);
        version = card_get_register_version(card, DAG_REG_PPF, state->mIndex);
        
		if ( version >= 3 )
            		read_attr_array(component, bfs_status_attr, NB_STATUS_ELEM, state->mIndex);

		#if 0
		address = ((uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_PPF, state->mIndex));
    	mask = BIT16;
		grw = grw_init(card, address, grw_iom_read, grw_iom_write);
		attr = ruleset_use_for_iface_zero_attribute(grw,&mask,1);	
	
		component_add_attribute(component,attr);
	
		/*Add ruleset selection for interface.*/
		iface_count = get_interface_count(component);

		if(iface_count > 1)
		{
			mask = BIT17;
			attr = ruleset_use_for_iface_1_attribute(grw,&mask, len);	
			component_add_attribute(component,attr);
		}
		if(iface_count > 2)
		{
			mask = BIT18;
			attr = ruleset_use_for_iface_2_attribute(grw,&mask, len);	
			component_add_attribute(component,attr);
		}
		if(iface_count > 3)
		{
			mask = BIT19;
			attr = ruleset_use_for_iface_3_attribute(grw,&mask,len);	
			component_add_attribute(component,attr);
		}
		#endif	
	
		address = ((uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_BFSCAM, state->mIndex));
    	mask = BIT31|BIT30;
		grw = grw_init(card, address, grw_iom_read, grw_iom_write);
		attr = activate_bank_attribute(grw,&mask,1);	
		component_add_attribute(component,attr);	
		return 1;
    }
    return kDagErrInvalidParameter;
}

#if 0
static
AttributePtr ruleset_use_for_iface_zero_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = attribute_init(kBooleanAttributeIPFRulesetInterface0); 
	if (NULL != result)
	{
		attribute_set_generic_read_write_object(result,grw);
		attribute_set_name(result, "use_ruleset_for_interface_0");
		attribute_set_getvalue_routine(result, attribute_boolean_get_value);
		attribute_set_setvalue_routine(result, attribute_boolean_set_value);
		attribute_set_to_string_routine(result,attribute_boolean_to_string);
		attribute_set_from_string_routine(result,attribute_boolean_from_string);
		attribute_set_description(result, "Set the rule set for interface 0 - can be used for hot swapping.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_masks(result,bit_masks,len);
		attribute_set_valuetype(result, kAttributeBoolean);
	}
	return result;
}
static
AttributePtr ruleset_use_for_iface_1_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = attribute_init(kBooleanAttributeIPFRulesetInterface1); 
	if (NULL != result)
	{
		attribute_set_generic_read_write_object(result,grw);
		attribute_set_name(result, "use_ruleset_for_interface_1");
		attribute_set_getvalue_routine(result, attribute_boolean_get_value);
		attribute_set_setvalue_routine(result, attribute_boolean_set_value);
		attribute_set_to_string_routine(result,attribute_boolean_to_string);
		attribute_set_from_string_routine(result,attribute_boolean_from_string);
		attribute_set_description(result, "Set the rule set for interface 1 - can be used for hot swapping.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_masks(result,bit_masks,len);
		attribute_set_valuetype(result, kAttributeBoolean);
	}
	return result;
}
static
AttributePtr ruleset_use_for_iface_2_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = attribute_init(kBooleanAttributeIPFRulesetInterface2); 
	if (NULL != result)
	{
		attribute_set_generic_read_write_object(result,grw);
		attribute_set_name(result, "use_ruleset_for_interface_2");
		attribute_set_getvalue_routine(result, attribute_boolean_get_value);
		attribute_set_setvalue_routine(result, attribute_boolean_set_value);
		attribute_set_to_string_routine(result,attribute_boolean_to_string);
		attribute_set_from_string_routine(result,attribute_boolean_from_string);
		attribute_set_description(result, "Set the rule set for interface 2 - can be used for hot swapping.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_masks(result,bit_masks,len);
		attribute_set_valuetype(result, kAttributeBoolean);
	}
	return result;
}
static
AttributePtr ruleset_use_for_iface_3_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = attribute_init(kBooleanAttributeIPFRulesetInterface3); 
	if (NULL != result)
	{
		attribute_set_generic_read_write_object(result,grw);
		attribute_set_name(result, "use_ruleset_for_interface_3");
		attribute_set_getvalue_routine(result, attribute_boolean_get_value);
		attribute_set_setvalue_routine(result, attribute_boolean_set_value);
		attribute_set_to_string_routine(result,attribute_boolean_to_string);
		attribute_set_from_string_routine(result,attribute_boolean_from_string);
		attribute_set_description(result, "Set the rule set for interface 3 - can be used for hot swapping.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_masks(result,bit_masks,len);
		attribute_set_valuetype(result, kAttributeBoolean);
	}
	return result;
}

#endif


static void bfs_dispose(ComponentPtr component)
{

}
static void bfs_reset(ComponentPtr component)
{

}
static void bfs_default(ComponentPtr component)
{


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
#if 0
int get_interface_count(ComponentPtr component)
{
    ComponentPtr root = NULL;
    ComponentPtr gpp_comp = NULL;
    ComponentPtr port_comp = NULL;
    ComponentPtr phy_comp = NULL;
    
    uint32_t gpp_count = 0;
	uint32_t iface_count = 0;
	AttributePtr attr = kNullAttributeUuid;
    int i = 0;

    
	root = component_get_parent(component);
    if ( NULL == root)
    {
        dagutil_error("root component is NULL\n");
        return 0;
    }
    gpp_count = component_get_subcomponent_count_of_type(root, kComponentGpp);
    for ( i = 0; i < gpp_count; i++)
    {
        gpp_comp = component_get_subcomponent(root, kComponentGpp, i);
        if(gpp_comp != NULL)
	   {
            attr = component_get_attribute(gpp_comp,kUint32AttributeInterfaceCount);
            if(attr)
            iface_count += *(uint32_t*)attribute_get_value(attr);
	   }
    }
	if ( 0 == iface_count)
	{
		port_comp = component_get_subcomponent(root, kComponentPort, 0);
		if (port_comp != NULL)
			iface_count = component_get_subcomponent_count_of_type(root, kComponentPort);
		else
		{
			phy_comp = component_get_subcomponent(root, kComponentPhy, 0);
			if (phy_comp != NULL)
				iface_count = component_get_subcomponent_count_of_type(root, kComponentPhy);
		}
	}
    return iface_count;
}
#endif
static
AttributePtr activate_bank_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = attribute_init(kBooleanAttributeActivateBank); 
	if (NULL != result)
	{
		attribute_set_generic_read_write_object(result,grw);
		attribute_set_name(result, "activate_bank");
		attribute_set_getvalue_routine(result, bank_activate_get_value);
		attribute_set_setvalue_routine(result, bank_activate_set_value);
		attribute_set_to_string_routine(result,attribute_boolean_to_string);
		attribute_set_from_string_routine(result,attribute_boolean_from_string);
		attribute_set_description(result, "Activates the BANK to be used for searching.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_masks(result,bit_masks,len);
		attribute_set_valuetype(result, kAttributeBoolean);
	}
	return result;
}
void bank_activate_set_value(AttributePtr attribute,void *value,int len)
{
	if(1 == valid_attribute(attribute))
	{
		uint32_t val = 0;
		bool bool_value;
			
		bool_value = *(bool*)value;
		if(bool_value)
		{	 /*Activate Bank 1*/
			val = 0x3;	
		}
		else /*Activate Bank 0*/
		{
			val = 0x1;
		}
		attribute_uint32_set_value(attribute,(void*)&val,len);
		val = 0;
		attribute_uint32_set_value(attribute,(void*)&val,len);
	}
	return;
}
void* bank_activate_get_value(AttributePtr attribute)
{
	if(1 == valid_attribute(attribute))
	{
		ComponentPtr component;
		AttributePtr attr;
		bool bool_value;
		
		component = attribute_get_component(attribute);
		attr = component_get_attribute(component,kBooleanAttributeBfsActiveBank);
		bool_value = *(uint8_t*)attribute_get_value(attr);
		/*This is the actual BANK STATUS BIT.This is what we are going to report.*/
		attribute_set_value_array(attribute,&bool_value,sizeof(bool_value));
		return (void*)attribute_get_value_array(attribute);	
	}
	return NULL;


}
