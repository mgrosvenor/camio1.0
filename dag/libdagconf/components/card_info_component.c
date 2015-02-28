/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *
 * This component is used by all DAG cards.
 *
 * $Id: card_info_component.c,v 1.23 2008/08/21 07:40:10 jomi Exp $
 */

#include "dagutil.h"
#include "dagapi.h"

#include "../include/attribute.h"
#include "../include/util/set.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/card_info_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"


#define BUFFER_SIZE 1024
/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: card_info_component.c,v 1.23 2008/08/21 07:40:10 jomi Exp $";
static const char* const kRevisionString = "$Revision: 1.23 $";

static void card_info_dispose(ComponentPtr component);
static void card_info_reset(ComponentPtr component);
static void card_info_default(ComponentPtr component);
static int card_info_post_initialize(ComponentPtr component);
static dag_err_t card_info_update_register_base(ComponentPtr component);

static void* user_fw_get_value(AttributePtr attribute);
static void user_fw_set_value(AttributePtr attribute, void * value, int length);
/* commenting out because not used now- replaced by a common function
   fw_image_name_to_string_routine */
/*static void user_fw_to_string_routine(AttributePtr attribute);*/
static void user_fw_from_string_routine(AttributePtr attribute, const char * string);

static void* factory_fw_get_value(AttributePtr attribute);
static void factory_fw_set_value(AttributePtr attribute, void * value, int length);
/* commenting out because not used now- replaced by a common function
   fw_image_name_to_string_routine */
/*static void factory_fw_to_string_routine(AttributePtr attribute);*/
static void factory_fw_from_string_routine(AttributePtr attribute, const char * string);

/* Active Firmware */
static void* active_fw_get_value(AttributePtr attribute);
//Commented out due not used 
//static void active_fw_set_value(AttributePtr attribute, void * value, int length);
static void active_fw_to_string_routine(AttributePtr attribute);
static void active_fw_from_string_routine(AttributePtr attribute, const char * string);

static void* serial_id_get_value(AttributePtr attribute);
/* CoPro */
static void* copro_type_get_value(AttributePtr attribute);
static void copro_type_set_value(AttributePtr attribute, void * value, int length);
static void copro_type_to_string_routine(AttributePtr attribute);
static void copro_type_from_string_routine(AttributePtr attribute, const char * string);

/* PCI Info */
static void* pci_info_get_value(AttributePtr attribute);
static void pci_info_set_value(AttributePtr attribute, void * value, int length);
static void pci_info_to_string_routine(AttributePtr attribute);
static void pci_info_from_string_routine(AttributePtr attribute, const char * string);

/* Board Revision Info */
static void* board_rev_get_value(AttributePtr attribute);
static void board_rev_set_value(AttributePtr attribute, void * value, int length);
/* PCI device code */
static void* pci_device_code_get_value(AttributePtr attribute);

static void factory_copro_fw_set_value(AttributePtr attribute, void* value, int length);
static void* factory_copro_fw_get_value(AttributePtr attribute);
static void user_copro_fw_set_value(AttributePtr attribute, void* value, int length);
static void* user_copro_fw_get_value(AttributePtr attribute);
/* common to_string_routine for firmware image names - for user and firmware images */
static void fw_image_name_to_string_routine(AttributePtr attribute);

/**
 * Sonet PP 's Attribute definition array 
 * this component is fo the DAG 8.1sx, 5.0s, 5.2sxa cards and is combine deframer demapper
 * And ATM or POS extractor at the moment it does support only concatenated POS no
 * not implemented ATM and channlized 
 */
Attribute_t card_info_attr[]=
{


    {
        /* Name */                 "user_fw",
        /* Attribute Code */       kStringAttributeUserFirmware,
        /* Attribute Type */       kAttributeString,
        /* Description */          "User firmware",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             user_fw_set_value,
        /* GetValue */             user_fw_get_value,
        /* SetToString */          fw_image_name_to_string_routine,
        /* SetFromString */        user_fw_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "factory_fw",
        /* Attribute Code */       kStringAttributeFactoryFirmware,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Factory firmware",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             factory_fw_set_value,
        /* GetValue */             factory_fw_get_value,
        /* SetToString */          fw_image_name_to_string_routine,
        /* SetFromString */        factory_fw_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "active_fw",
        /* Attribute Code */       kUint32AttributeActiveFirmware,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "active firmware",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             active_fw_get_value,
        /* SetToString */          active_fw_to_string_routine,
        /* SetFromString */        active_fw_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "serial_id",
        /* Attribute Code */       kUint32AttributeSerialID,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Card Serial ID",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             serial_id_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "copro_type",
        /* Attribute Code */       kUint32AttributeCoPro,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Co-processor type",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             copro_type_set_value,
        /* GetValue */             copro_type_get_value,
        /* SetToString */          copro_type_to_string_routine,
        /* SetFromString */        copro_type_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "pci_info",
        /* Attribute Code */       kStringAttributePCIInfo,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Physical slot information",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             pci_info_set_value,
        /* GetValue */             pci_info_get_value,
        /* SetToString */          pci_info_to_string_routine,
        /* SetFromString */        pci_info_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,

    },

    {
        /* Name */                 "pci_device_code",
        /* Attribute Code */       kUint32AttributePCIDeviceID,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "PCI device code",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value, /* status attribute.Don't care */
        /* GetValue */             pci_device_code_get_value,
        /* SetToString */          attribute_uint32_to_hex_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "board_rev",
        /* Attribute Code */       kUint32AttributeBoardRevision,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Board revision.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             board_rev_set_value,
        /* GetValue */             board_rev_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,

    },
   
};
Attribute_t copro_fw_image_names_attr[]=
{
 {
        /* Name */                 "user_copro_fw",
        /* Attribute Code */       kStringAttributeUserCoproFirmware,
        /* Attribute Type */       kAttributeString,
        /* Description */          "User Co-pro firmware",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             user_copro_fw_set_value,
        /* GetValue */             user_copro_fw_get_value,
        /* SetToString */          fw_image_name_to_string_routine,
        /* SetFromString */        user_fw_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "factory_copro_fw",
        /* Attribute Code */       kStringAttributeFactoryCoproFirmware,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Factory Co-pro firmware",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_GENERAL,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0,
        /* Default Value */        0,
        /* SetValue */             factory_copro_fw_set_value,
        /* GetValue */             factory_copro_fw_get_value,
        /* SetToString */          fw_image_name_to_string_routine,
        /* SetFromString */        factory_fw_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};
/* Number of elements in array */
#define NB_ELEM (sizeof(card_info_attr) / sizeof(Attribute_t))

/* Number of elements in array */
#define NB_ELEM_COPRO_IMG (sizeof(copro_fw_image_names_attr) / sizeof(Attribute_t))

ComponentPtr
card_info_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentCardInfo, card); 
    card_info_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, card_info_dispose);
        component_set_post_initialize_routine(result, card_info_post_initialize);
        component_set_reset_routine(result, card_info_reset);
        component_set_default_routine(result, card_info_default);
        component_set_update_register_base_routine(result, card_info_update_register_base);
        component_set_name(result, "card_info");
        state = (card_info_state_t*)malloc(sizeof(card_info_state_t));
        state->mIndex = index;
	    state->mBase = card_get_register_address(card, DAG_REG_GENERAL, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
card_info_dispose(ComponentPtr component)
{
}

static dag_err_t
card_info_update_register_base(ComponentPtr component)
{
    return kDagErrNone;
}

static int
card_info_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        card_info_state_t* state = NULL;
        coprocessor_t copro = kCoprocessorUnknown;
        volatile uint8_t* iom = NULL;
        DagCardPtr card = component_get_card(component);
        iom = card_get_iom_address(card);
        copro = dagutil_detect_coprocessor(iom);
        state = component_get_private_state(component);
        /* Add attribute of card_info */ 
        read_attr_array(component, card_info_attr, NB_ELEM, state->mIndex);
        /* check if the builtin copro exist,and then add the copro image names to the component */
        if ( kCoprocessorBuiltin == copro)
        {
            read_attr_array(component, copro_fw_image_names_attr, NB_ELEM_COPRO_IMG, state->mIndex );
        }
        

	return 1;
    }
    return kDagErrInvalidParameter;
}

static void
card_info_reset(ComponentPtr component)
{  
	
}


static void
card_info_default(ComponentPtr component)
{

}
static void
user_fw_set_value(AttributePtr attribute, void* value, int length)
{
    /* Not implemented */
    return;
}
    
static void*
user_fw_get_value(AttributePtr attribute)
{
   dag_card_ref_t card_r;
	char* user_fw = NULL;

   if (1 == valid_attribute(attribute))
   {
	DagCardPtr card = attribute_get_card(attribute);
	card_r = (dag_card_ref_t)card;
	user_fw = dag_firmware_read_user_firmware_name(card_r);
   }
   return (void*)user_fw;
}
/*
static void 
user_fw_to_string_routine(AttributePtr attribute)
{
    return;
}*/
static void
user_fw_from_string_routine(AttributePtr attribute, const char * string)
{
    /* Not implemented */
    return;
}

static void
factory_fw_set_value(AttributePtr attribute, void* value, int length)
{
    /* Not implemented */
    return;
}


static void*
factory_fw_get_value(AttributePtr attribute)
{
    dag_card_ref_t card_r;
    char* factory_fw = NULL;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        card_r = (dag_card_ref_t)card;
        factory_fw = dag_firmware_read_factory_firmware_name(card_r);
    }
    return (void*)factory_fw;
}
/*
static void 
factory_fw_to_string_routine(AttributePtr attribute)
{
    return;
}*/
static void
factory_fw_from_string_routine(AttributePtr attribute, const char * string)
{
    /* Not implemented */
    return;
}

//static void
//active_fw_set_value(AttributePtr attribute, void* value, int length)
//{
   /* Not implemented */
//    return;
//}

static void*
active_fw_get_value(AttributePtr attribute)
{
    dag_card_ref_t card_r;
    dag_firmware_t firmware;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        card_r = (dag_card_ref_t)card;
        firmware = dag_firmware_get_active(card_r);
        attribute_set_value_array(attribute, &firmware, sizeof(firmware));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}
static void 
active_fw_to_string_routine(AttributePtr attribute)
{
    void * temp = attribute_get_value(attribute);
    const char * string = NULL;
    dag_firmware_t firmware;
    if(temp)
    {
        firmware = *(dag_firmware_t*)temp;
        string = active_firmware_to_string(firmware);
        if(string)
            attribute_set_to_string(attribute, string);
    }
}
static void
active_fw_from_string_routine(AttributePtr attribute, const char * string)
{
    /* Not implemented */
    return;
}

static void*
serial_id_get_value(AttributePtr attribute)
{
   dag_card_ref_t card_r;
   int serial = 0;

   if (1 == valid_attribute(attribute))
   {
	DagCardPtr card = attribute_get_card(attribute);
	card_r = (dag_card_ref_t)card;
	dag_firmware_read_serial_id(card_r, &serial);
   }
   attribute_set_value_array(attribute, &serial, sizeof(serial));
   return (void *)attribute_get_value_array(attribute);
   
}

static void
copro_type_set_value(AttributePtr attribute, void* value, int length)
{
    /* Not implemented */
    return;
}


static void*
copro_type_get_value(AttributePtr attribute)
{
   coprocessor_t copro = kCoprocessorUnknown;

   if (1 == valid_attribute(attribute))
   {
	volatile uint8_t* iom = NULL;
     
	DagCardPtr card = attribute_get_card(attribute);
	
	iom = card_get_iom_address(card);
	copro = dagutil_detect_coprocessor(iom);	
   }
   attribute_set_value_array(attribute, &copro, sizeof(copro));
   return (void *)attribute_get_value_array(attribute);
   
}

static void 
copro_type_to_string_routine(AttributePtr attribute)
{
    void * temp = attribute_get_value(attribute);
    const char * string = NULL;
    coprocessor_t copro;
    if(temp)
    {
        copro = *(coprocessor_t *)temp;
        string = coprocessor_to_string(copro);
        if(string)
            attribute_set_to_string(attribute, string);
    }
    
    return;
}
static void
copro_type_from_string_routine(AttributePtr attribute, const char * string)
{
    /* Not implemented */
    return;
}

static void
pci_info_set_value(AttributePtr attribute, void* value, int length)
{
    /* Not implemented */
    return;
}


static void*
pci_info_get_value(AttributePtr attribute)
{
	daginf_t* pci_info = NULL;
    char buffer[BUFFER_SIZE] ;
   if (1 == valid_attribute(attribute))
   {
        int dagfd = 0;
        DagCardPtr card = attribute_get_card(attribute);
            
        dagfd = card_get_fd(card);
        pci_info = dag_info(dagfd);
        if ( NULL == pci_info)
        {
            return NULL;
        }
#if defined(__linux__) || defined(__FreeBSD__) || (defined (__SVR4) && defined (__sun))
        sprintf(buffer,"%s",pci_info->bus_id);
#elif defined(_WIN32)
        sprintf(buffer,"%04d:%02d:%02d", pci_info->bus_num, pci_info->dev_num, pci_info->fun_num );
#else 
#error The OS system not supported pleas contact support@endace.com
#endif
        attribute_set_value_array(attribute, buffer, strlen( buffer) + 1);
   }
    return (void *)attribute_get_value_array(attribute);
}

static void 
pci_info_to_string_routine(AttributePtr attribute)
{
	if(1 == valid_attribute(attribute))
	{
		char *info_string = NULL;
		info_string = (char*)attribute_get_value(attribute);
		if(NULL != info_string)
		{
			attribute_set_to_string(attribute,info_string);
		}
    }
    return;
}
static void
pci_info_from_string_routine(AttributePtr attribute, const char * string)
{
    /* Not implemented */
    return;
}

static void
board_rev_set_value(AttributePtr attribute, void* value, int length)
{
    /* Not implemented */
    return;
}


static void*
board_rev_get_value(AttributePtr attribute)
{
    daginf_t* daginf = NULL;
    uint32_t value = 0;

    if (1 == valid_attribute(attribute))
    {
        int dagfd = 0;
        DagCardPtr card = attribute_get_card(attribute);

        dagfd = card_get_fd(card);
        daginf = dag_info(dagfd);
        if (daginf != NULL)
        {
            value = daginf->brd_rev;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
    }
    return (void *)attribute_get_value_array(attribute);
}

static void
factory_copro_fw_set_value(AttributePtr attribute, void* value, int length)
{
    /* Not implemented */
    return;
}


static void*
factory_copro_fw_get_value(AttributePtr attribute)
{
    dag_card_ref_t card_r;
    char* factory_fw = NULL;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        card_r = (dag_card_ref_t)card;
        factory_fw = dag_copro_read_factory_firmware_name(card_r);
        if ( NULL != factory_fw)
        {
            attribute_set_value_array(attribute, factory_fw, strlen(factory_fw) + 1);
        }
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
user_copro_fw_set_value(AttributePtr attribute, void* value, int length)
{
    /* Not implemented */
    return;
}


static void*
user_copro_fw_get_value(AttributePtr attribute)
{
    dag_card_ref_t card_r;
    char* user_fw = NULL;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        card_r = (dag_card_ref_t)card;
        user_fw = dag_copro_read_user_firmware_name(card_r);
        if ( NULL != user_fw)
        {
            attribute_set_value_array(attribute, user_fw, strlen(user_fw) + 1);
        }
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}


static void fw_image_name_to_string_routine(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        char *image_name = NULL;
        
        image_name = (char*) attribute_get_value(attribute);
        if ( NULL != image_name)
        {
            attribute_set_to_string(attribute, image_name);
        }
        return;
    }
}

void* pci_device_code_get_value(AttributePtr attribute)
{
    daginf_t* daginf = NULL;
    uint32_t value = 0;

    if (1 == valid_attribute(attribute))
    {
        int dagfd = 0;
        DagCardPtr card = attribute_get_card(attribute);

        dagfd = card_get_fd(card);
        daginf = dag_info(dagfd);
        if (daginf != NULL)
        {
            value = daginf->device_code;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
    }
    return (void *)attribute_get_value_array(attribute);
}

