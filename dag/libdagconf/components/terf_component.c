/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: terf_component.c,v 1.21 2008/10/03 01:59:37 jomi Exp $
 */
#include "dagutil.h"

#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/attribute_factory.h"
#include "../include/components/terf_component.h"
#include "../include/components/tr_terf_component.h"
#include "../include/create_attribute.h"

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: terf_component.c,v 1.21 2008/10/03 01:59:37 jomi Exp $";
static const char* const kRevisionString = "$Revision: 1.21 $";

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} terf_state_t;

enum
{
    CONTROL_REGISTER                = 0x00,
    ABSOLUTE_MODE_OFFSET_REGISTER   = 0x04,
    CONFIGURABLE_LIMIT_REGISTER     = 0x08,
    TTR_TERF_CONTROL_REGISTER       = 0X10,
    TTR_TERF_TRIGGER_TIME           = 0X18,/* address of the higher 32 bits */
};


/* extern function declarations */
extern void attribute_time_mode_set_value(AttributePtr attribute, void* value, int length);
extern void* attribute_time_mode_get_value(AttributePtr attribute);
extern void time_mode_value_to_string(AttributePtr attribute);
extern void time_mode_value_from_string(AttributePtr attribute, const char* string);

static void trigger_timestamp_to_string(AttributePtr attribute);
static void trigger_timestamp_from_string(AttributePtr attribute, const char* string);
static void add_rx_error_attributes(ComponentPtr component, terf_state_t* state );
static void abs_mode_offset_set_value(AttributePtr attribute, void* value, int len);
static  void* abs_mode_offset_get_value(AttributePtr attribute);
static void conf_limit_set_value(AttributePtr attribute, void* value, int len);
static void*  conf_limit_get_value(AttributePtr attribute);


/* attribute array for TR-TERF - Version 2 */
Attribute_t tr_rx_error_attr[] = 
{
    {
        /* Name */                 "rx_error_a",
        /* Attribute Code */       kBooleanAttributeRXErrorA,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Mask for the RX-Error bit in ERF header",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,
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
        /* Name */                 "rx_error_b",
        /* Attribute Code */       kBooleanAttributeRXErrorB,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Mask for the RX-Error bit in ERF header for the 2nd ouput port if available",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    3,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,
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
        /* Name */                 "rx_error_c",
        /* Attribute Code */       kBooleanAttributeRXErrorC,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Mask for the RX-Error bit in ERF header for the 3rd ouput port if available",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    4,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,
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
        /* Name */                 "rx_error_d",
        /* Attribute Code */       kBooleanAttributeRXErrorD,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Mask for the RX-Error bit in ERF header for the 4th ouput port if available",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    5,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,
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
};

Attribute_t tr_terf_attr[] = 
{
	{
        /* Name */                 "time_mode",
        /* Attribute Code */       kUint32AttributeTimeMode,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Determine the timed release mode",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    6,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT6|BIT7, 
        /* Default Value */        0,
        /* SetValue */             attribute_time_mode_set_value,
        /* GetValue */             attribute_time_mode_get_value,
        /* SetToString */          time_mode_value_to_string,
        /* SetFromString */        time_mode_value_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	{
        /* Name */                 "scale_range",
        /* Attribute Code */       kUint32AttributeScaleRange,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of logical shifts performed between 2 packet's timestamps",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    9,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT9|BIT10|BIT11|BIT12|BIT13|BIT14, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "shift_direction",
        /* Attribute Code */       kBooleanAttributeShiftDirection,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Determines the shift direction, left or right, multiplication or division",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    15,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT15,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "abs_mode_offset",
        /* Attribute Code */       kUint64AttributeAbsModeOffset,
        /* Attribute Type */       kAttributeUint64,
        /* Description */          "abs mode offset value",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    16,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,/* only for MSB 4 bits and  LSB implementation done in abs_mode_offset_set_value*/
        /* Size/length */          2,
        /* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT16|BIT17|BIT18|BIT19,
        /* Default Value */        0,
        /* SetValue */             abs_mode_offset_set_value,
        /* GetValue */             abs_mode_offset_get_value,
        /* SetToString */          attribute_uint64_to_string,
        /* SetFromString */        attribute_uint64_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "conf_limit",
        /* Attribute Code */       kUint64AttributeConfLimit,
        /* Attribute Type */       kAttributeUint64,
        /* Description */          "the conf limit value",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    20,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               CONTROL_REGISTER,/* only for MSB 6 bits and  LSB implementation done in conf_limit_set_value*/
        /* Size/length */          2,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT20|BIT21|BIT22|BIT23|BIT24|BIT25,
        /* Default Value */        0,
        /* SetValue */             conf_limit_set_value,
        /* GetValue */             conf_limit_get_value,
        /* SetToString */          attribute_uint64_to_string,
        /* SetFromString */        attribute_uint64_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};

Attribute_t ttr_terf_attr[] = 
{
    {
        /* Name */                 "clear_trigger",
        /* Attribute Code */       kBooleanAttributeClearTrigger,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "If the trigger is armed, setting this attribute will manually release packets, without trigger time is reached",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               TTR_TERF_CONTROL_REGISTER,
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
        /* Name */                 "trigger_pending",
        /* Attribute Code */       kBooleanAttributeTriggerPending,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Trigger pending to happen",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    30,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               TTR_TERF_CONTROL_REGISTER,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT30, 
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "trigger_occured",
        /* Attribute Code */       kBooleanAttributeTriggerOccured,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Trigger has occured",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    31,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               TTR_TERF_CONTROL_REGISTER,
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
        /* Name */                 "trigger_timestamp",
        /* Attribute Code */       kUint64AttributeTriggerTimestamp,
        /* Attribute Type */       kAttributeUint64,
        /* Description */          "The time at which TTR-TERF has to be triggered to send the packet",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_TERF64,
        /* Offset */               TTR_TERF_TRIGGER_TIME,
        /* Size/length */          2,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xffffffff, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint64_set_value_reverse,
        /* GetValue */             attribute_uint64_get_value_reverse,
        /* SetToString */          trigger_timestamp_to_string,
        /* SetFromString */        trigger_timestamp_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};

#define NB_ELEM_RX (sizeof(tr_rx_error_attr) / sizeof(Attribute_t))
#define NB_ELEM (sizeof(tr_terf_attr) / sizeof(Attribute_t))
#define NB_ELEM_TTR  (sizeof(ttr_terf_attr) / sizeof(Attribute_t))


/* terf component. */
static void terf_dispose(ComponentPtr component);
static void terf_reset(ComponentPtr component);
static void terf_default(ComponentPtr component);
static int terf_post_initialize(ComponentPtr component);
static dag_err_t terf_update_register_base(ComponentPtr component);

/* strip attribute. */
static AttributePtr get_new_strip_attribute(void);
static void strip_dispose(AttributePtr attribute);
static void* strip_get_value(AttributePtr attribute);
static void strip_set_value(AttributePtr attribute, void* value, int length);
static void strip_post_initialize(AttributePtr attribute);
static void strip_to_string_routine(AttributePtr attribute);
static void strip_from_string_routine(AttributePtr attribute, const char* string);

ComponentPtr
terf_get_new_terf(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentTerf, card); 
    terf_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, terf_dispose);
        component_set_post_initialize_routine(result, terf_post_initialize);
        component_set_reset_routine(result, terf_reset);
        component_set_default_routine(result, terf_default);
        component_set_update_register_base_routine(result, terf_update_register_base);
        component_set_name(result, "terf");
        state = (terf_state_t*)malloc(sizeof(terf_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
terf_dispose(ComponentPtr component)
{
}

static dag_err_t
terf_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        terf_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_TERF64, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
terf_post_initialize(ComponentPtr component)
{
    AttributePtr strip = NULL;
    terf_state_t* state = NULL;
    DagCardPtr card;
    uint32_t version = 0;
    uintptr_t address = 0;
    state = component_get_private_state(component);
    card = component_get_card(component);

    state->mBase = card_get_register_address(card, DAG_REG_TERF64, 0);

    strip = get_new_strip_attribute();

    component_add_attribute(component, strip);
    
    /* Get the version of the component */
//CHECKME FIXME: different in the 3.0.0 and 3.1.0 
//Maybe bug in 3.0.0
    version = card_get_register_version(card, DAG_REG_TERF64, state->mIndex);
	
    address = ((uintptr_t)card_get_iom_address(card)) + state->mBase;
	
	if (version >= 1)
	{
		/* Add the RX-Error  attributes */
		add_rx_error_attributes(component, state);
	}
    /* Determine if there is a Time Relese (TR) feature, only version 2 support time release */
    if (version >= 2)
    {
	    /* Here add the version 2 attributes (TR-TERF specific ) */
        read_attr_array(component, tr_terf_attr, NB_ELEM, state->mIndex); 
    }
    if ( version == 3 )
    {
	    /* add the version 3 attributes ( TTR-TERF specific ) */
	    read_attr_array( component, ttr_terf_attr, NB_ELEM_TTR, state->mIndex );
    } 

    return 1;
}

static void
terf_reset(ComponentPtr component)
{
}


static void
terf_default(ComponentPtr component)
{
}

static AttributePtr
get_new_strip_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTerfStripCrc); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, strip_dispose);
        attribute_set_post_initialize_routine(result, strip_post_initialize);
        attribute_set_getvalue_routine(result, strip_get_value);
        attribute_set_setvalue_routine(result, strip_set_value);
        attribute_set_to_string_routine(result, strip_to_string_routine);
        attribute_set_from_string_routine(result, strip_from_string_routine);
        attribute_set_name(result, "terf_strip");
        attribute_set_description(result, "Transmit ERF records without or after CRC stripping. If CRC stripping, the last 16 or 32 bits can be stripped ");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}


static void
strip_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
strip_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void*
strip_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr terf;
        terf_state_t* state = NULL;
        terf_strip_t val = 0;
        uint32_t register_value;

        card = attribute_get_card(attribute);
        terf = attribute_get_component(attribute);
        state = (terf_state_t*)component_get_private_state(terf);
        register_value = card_read_iom(card, state->mBase);
        register_value &= (BIT0 | BIT1);
        if (register_value & BIT0)
        {
            val = kTerfStrip16;
        }
        else if (register_value & BIT1)
        {
            val = kTerfStrip32;
        }
        else if (register_value == 0)
        {
            val = kTerfNoStrip;
        }
        else
        {
            val = kTerfStripInvalid;
        }
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    
    return NULL;
}


static void
strip_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        terf_state_t* state = NULL;
        terf_strip_t val = *(terf_strip_t*)value;
        uint32_t register_value = 0;
        ComponentPtr terf;

        card = attribute_get_card(attribute);
        terf =  attribute_get_component(attribute);
        state = (terf_state_t*)component_get_private_state(terf);
        register_value = card_read_iom(card, state->mBase);
        register_value &= ~(BIT1 | BIT0);
        if (kTerfStrip16 == val)
        {
            register_value |= BIT0;
        }
        else if (kTerfStrip32 == val)
        {
            register_value |= BIT1;
        }
        card_write_iom(card, state->mBase, register_value);
    }
}

static void
strip_to_string_routine(AttributePtr attribute)
{
    terf_strip_t val = *(terf_strip_t*)attribute_get_value(attribute);
    assert(val != kTerfStripInvalid);
    attribute_set_to_string(attribute, terf_strip_to_string(val));
}

static void
strip_from_string_routine(AttributePtr attribute, const char* string)
{
    terf_strip_t val = string_to_terf_strip(string);
    assert(val != kTerfStripInvalid);
    attribute_set_value(attribute, (void*)&val, sizeof(terf_strip_t));
}


void 
trigger_timestamp_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        char *time_string = NULL;
        time_t time_val;
        uint64_t *ptr_time = (uint64_t*) attribute_get_value( attribute );
        if ( NULL != ptr_time )
        {
            #if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		time_val = (time_t)((*ptr_time >> 32) + (int)(((*ptr_time &0xffffffff) + (double)0x80000000)/0x100000000ll));
#elif defined(_WIN32)
		time_val = (time_t)((*ptr_time  >> 32) + (int)(((*ptr_time &0xffffffff) + (double)0x80000000)/0x100000000));
#endif
		time_string = ctime(&time_val);
                attribute_set_to_string(attribute, time_string);
        }

    }
}

void add_rx_error_attributes(ComponentPtr component, terf_state_t* state )
{
    AttributePtr iface_count = NULL;
    ComponentPtr root_component = NULL;
    ComponentPtr gpp_component = NULL;
    DagCardPtr card_ptr = NULL;
    uint32_t *ptr_count = NULL;
    uint32_t rx_error_count = 0;

    /* get the number of interfaces from GPP component and that many rx_error(s) */
    card_ptr = component_get_card(component);
    if ( NULL == card_ptr )
        return ;
    root_component = card_get_root_component(card_ptr);
    if ( NULL == root_component )
        return ;
    gpp_component = component_get_subcomponent(root_component, kComponentGpp, 0/* index */); 
    if ( NULL == gpp_component)
        return ;
    iface_count = component_get_attribute(gpp_component, kUint32AttributeInterfaceCount);
    if ( NULL != (ptr_count = (uint32_t*)attribute_get_value(iface_count) ) && *ptr_count )
    {
        rx_error_count = ( *ptr_count < NB_ELEM_RX )? *ptr_count: NB_ELEM_RX;
        read_attr_array(component, tr_rx_error_attr, rx_error_count, state->mIndex);
    }
	return;
}


void abs_mode_offset_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uintptr_t address = grw_get_address(grw);
        uintptr_t lsb_address;
        uint64_t value_64 = *(uint64_t*)value;
        uint32_t temp = 0;
        ComponentPtr component = NULL;
        DagCardPtr card_ptr = NULL;
        terf_state_t *state;

        component = attribute_get_component(attribute);
        if ( NULL == component)
            return ;
        card_ptr = component_get_card(component);
        if ( NULL == card_ptr )
            return ;

        /* check if value is greater than 36 bit */
        if ( value_64 > 0xfffffffffLL )
        {
             card_set_last_error(card_ptr, kDagErrInvalidParameter);
             return;
        }

        /*first fill MSB 4 bits */
        /* Get the content of the control register */
        reg_val = grw_read( grw);
        /* get the first 32 bits of the 64 bit value and mask */
        temp = (uint32_t) (( value_64 >> 32 ) &  0x0000000f);
        /* clear the specific bits of reg_val and  get the new value */
        reg_val = ( reg_val & 0xfff0ffff ) | (temp << 16);
        /*write the value */
        grw_write(grw, reg_val);
        
        /* Now fill the LSB */
                state = component_get_private_state(component);
        
        reg_val = (uint32_t) (value_64 & 0xffffffff);
        /* Get the LSB address which is at offset ABSOLUTE_MODE_OFFSET_REGISTER*/
        lsb_address = (uintptr_t) (card_get_iom_address(card_ptr) + card_get_register_address(card_ptr, DAG_REG_TERF64, state->mIndex) + ABSOLUTE_MODE_OFFSET_REGISTER);
        grw_set_address(grw,lsb_address);
        grw_write(grw,reg_val);

        /* restore the original address */
        grw_set_address(grw, address);
    }        
    return;
}
void* abs_mode_offset_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uintptr_t address = grw_get_address(grw);
        uintptr_t lsb_address;
        uint64_t value_64 = 0;
        uint32_t temp = 0;
        ComponentPtr component = NULL;
        DagCardPtr card_ptr = NULL;
        terf_state_t *state;
        
        /* Get the MSB ( 4 bits) */
        reg_val = grw_read(grw);
        /* get the 4 bits [16:19] */
        temp = (reg_val & 0x000f0000) >> 16 ;

        value_64 = (((uint64_t)temp) << 32);
        /* Get the LSB */ 
        component = attribute_get_component(attribute);
        if ( NULL == component)
            return NULL;
        card_ptr = component_get_card(component);
        if ( NULL == card_ptr )
            return NULL;
        state = component_get_private_state(component);
        
        /* Get the LSB address which is at offset ABSOLUTE_MODE_OFFSET_REGISTER*/
        lsb_address = (uintptr_t) (card_get_iom_address(card_ptr) + card_get_register_address(card_ptr, DAG_REG_TERF64, state->mIndex) + ABSOLUTE_MODE_OFFSET_REGISTER);
        grw_set_address(grw,lsb_address);
        reg_val = grw_read( grw);
        value_64 |= reg_val;
        attribute_set_value_array(attribute, &value_64, sizeof(value_64));
        grw_set_address(grw, address);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
void conf_limit_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uintptr_t address = grw_get_address(grw);
        uintptr_t lsb_address;
        uint64_t value_64 = *(uint64_t*)value;
		uint32_t temp = 0;
		ComponentPtr component = NULL;
		DagCardPtr card_ptr = NULL;
		terf_state_t *state;

        component = attribute_get_component(attribute);
        if ( NULL == component)
            return ;
        card_ptr = component_get_card(component);
        if ( NULL == card_ptr )
            return ;

        /* check if value is greater than 38 bit */
        if ( value_64 > 0x3fffffffffLL )
        {
             card_set_last_error(card_ptr, kDagErrInvalidParameter);
             return;
        }

        /*first fill MSB 4 bits */
        /* Get the content of the control register */
        reg_val = grw_read( grw);
        /* get the first 32 bits of the 64 bit value and mask */
        temp = (uint32_t) ( value_64 >> 32 ) &  0x0000003f;
        /* clear the specific bits of reg_val and  get the new value */
        reg_val = ( reg_val & 0xfc0fffff) | ( temp << 20);
        /*write the value */
        grw_write(grw, reg_val);
        
        /* Now fill the LSB */
        component = attribute_get_component(attribute);
        if ( NULL == component)
            return ;
        card_ptr = component_get_card(component);
        if ( NULL == card_ptr )
            return ;
        state = component_get_private_state(component);
        
        reg_val = (uint32_t) (value_64 & 0xffffffff);
        /* Get the LSB address which is at offset CONFIGURABLE_LIMIT_REGISTER*/
        lsb_address = (uintptr_t) (card_get_iom_address(card_ptr) + card_get_register_address(card_ptr, DAG_REG_TERF64, state->mIndex) + CONFIGURABLE_LIMIT_REGISTER);
        grw_set_address(grw,lsb_address);
        grw_write(grw,reg_val);

        /* restore the original address */
        grw_set_address(grw, address);
    }
    return;
}
void* conf_limit_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uintptr_t address = grw_get_address(grw);
        uintptr_t lsb_address;
        uint64_t value_64 = 0;
        uint32_t temp = 0;
        ComponentPtr component = NULL;
        DagCardPtr card_ptr = NULL;
        terf_state_t *state;

        /* Get the MSB ( 6  bits) */
        reg_val = grw_read(grw);
        /* get the 6 bits [20:25] */
        temp = (reg_val & 0x03f00000) >> 20 ;

        value_64 = (((uint64_t)temp) << 32);
        /* Get the LSB */ 
        component = attribute_get_component(attribute);
        if ( NULL == component)
            return NULL;
        card_ptr = component_get_card(component);
        if ( NULL == card_ptr )
            return NULL;
        state = component_get_private_state(component);
        
        /* Get the LSB address which is at offset CONFIGURABLE_LIMIT_REGISTER*/
        lsb_address = (uintptr_t) (card_get_iom_address(card_ptr) + card_get_register_address(card_ptr, DAG_REG_TERF64, state->mIndex) + CONFIGURABLE_LIMIT_REGISTER);
        grw_set_address(grw,lsb_address);
        reg_val = grw_read( grw);
        value_64 |= reg_val;
        attribute_set_value_array(attribute, &value_64, sizeof(value_64));
        grw_set_address(grw, address);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

void trigger_timestamp_from_string(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        uint64_t value = 0;
        char *parse_string = NULL;
        if ( NULL == string )
        {
            return ;
        }
        parse_string = strtok( (char*)string ,"." );
        if ( NULL != parse_string)
        {
            value = (((uint64_t)strtol( parse_string, NULL, 0))<<32) & 0xffffffff00000000ll;
        }
        parse_string = strtok(NULL,".");
        if ( NULL != parse_string)
        {
            value |= ((uint32_t)strtol( parse_string, NULL, 0) & 0x00000000ffffffffll)  ;
        }
        attribute_set_value(attribute, (void*)&value, sizeof(uint64_t));
    }
    return;
}
