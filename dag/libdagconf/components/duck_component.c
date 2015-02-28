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
#include "../include/util/enum_string_table.h"
#include "../include/attribute_factory.h"
#include "../../../include/dag_attribute_codes.h"
#include "../include/create_attribute.h"

#include "dagutil.h"
#include "dagapi.h"

void duck_ioctl_read(duckinf_t* duckinf, int fd);
static void duck_dispose(ComponentPtr component);
static void duck_reset(ComponentPtr component);
static void duck_default(ComponentPtr component);
static int duck_post_initialize(ComponentPtr component);

/*  synchronzied attribute. */
static AttributePtr duck_get_synchronized_attribute(void);
static void* duck_get_synchronized_value(AttributePtr attribute);

static AttributePtr duck_get_threshold_attribute(void);
static void* duck_get_threshold_value(AttributePtr attribute);
static void duck_set_threshold_value(AttributePtr attribute, void* value, int len);

static AttributePtr duck_get_failures_attribute(void);
static void* duck_get_failures_value(AttributePtr attribute);

static AttributePtr duck_get_resyncs_attribute(void);
static void* duck_get_resyncs_value(AttributePtr attribute);

static AttributePtr duck_get_frequency_error_attribute(void);
static void* duck_get_frequency_error_value(AttributePtr attribute);

static AttributePtr duck_get_phase_error_attribute(void);
static void* duck_get_phase_error_value(AttributePtr attribute);

static AttributePtr duck_get_worst_frequency_error_attribute(void);
static void* duck_get_worst_frequency_error_value(AttributePtr attribute);

static AttributePtr duck_get_worst_phase_error_attribute(void);
static void* duck_get_worst_phase_error_value(AttributePtr attribute);

static AttributePtr duck_get_crystal_frequency_attribute(void);
static void* duck_get_crystal_frequency_value(AttributePtr attribute);

static AttributePtr duck_get_synth_frequency_attribute(void);
static void* duck_get_synth_frequency_value(AttributePtr attribute);

static AttributePtr duck_get_pulses_attribute(void);
static void* duck_get_pulses_value(AttributePtr attribute);

static AttributePtr duck_get_single_pulses_missing_attribute(void);
static void* duck_get_single_pulses_missing_value(AttributePtr attribute);

static AttributePtr duck_get_longest_pulse_missing_attribute(void);
static void* duck_get_longest_pulse_missing_value(AttributePtr attribute);

static AttributePtr duck_get_time_info_attribute(void);
static void* duck_get_time_info_value(AttributePtr attribute);
static void duck_time_info_to_string(AttributePtr attribute);
static void duck_time_info_from_string(AttributePtr attribute, const char * string);

static AttributePtr duck_get_set_to_host_attribute(void);
static void duck_set_to_host_value(AttributePtr attribute, void* value, int len);
static void duck_set_to_host_to_string(AttributePtr attribute);
static void duck_set_to_host_from_string(AttributePtr attribute, const char * string);

static AttributePtr duck_get_clear_stats_attribute(void);
static void duck_set_clear_stats_value(AttributePtr attribute, void* value, int len);
static void duck_clear_stats_to_string(AttributePtr attribute);
static void duck_clear_stats_from_string(AttributePtr attribute, const char * string);

static AttributePtr duck_get_sync_attribute(void);
static void duck_set_sync_value(AttributePtr attribute, void* value, int len);
static void duck_sync_to_string(AttributePtr attribute);
static void duck_sync_from_string(AttributePtr attribute, const char * string);

static AttributePtr duck_get_sync_timeout_attribute(void);
static void duck_sync_timeout_set_value(AttributePtr attribute, void* value, int len);
static void* duck_sync_timeout_get_value(AttributePtr attribute);
/*This function has been added to retrive the latest value of the time stamp counter.*/
static AttributePtr duck_get_timestamp_counter_attribute(void);
void *duck_get_timestamp_counter_value(AttributePtr attribute);

static AttributePtr
duck_get_phase_correction_attribute(void);

static void
duck_set_phase_correction_value(AttributePtr attribute, void* value, int len);

static void*
duck_get_phase_correction_value(AttributePtr attribute);

static AttributePtr
duck_get_dds_rate_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);


/* for new duck reader timestamp */
static void  duck_timestamp_to_string(AttributePtr attribute) ;

typedef struct
{
    int mIndex;
} duck_state_t;

enum
{
    DUCK_COMMAND    = 0x00,
    DUCK_CONFIG    = 0x04,
    DUCK_HIGH_LOAD    = 0x0c,
    DUCK_DDS_RATE    = 0x14,
    DUCK_HIGH_READ    = 0x18,
    DUCK_LOW_READ    = 0x1c,
};

/* for the timestamp attribute- specified in an array */
static Attribute_t duck_reader_attr[] = 
{
    {
        /* Name */                 "duck_timestamp",
        /* Attribute Code */       kUint64AttributeDuckTimestamp,
        /* Attribute Type */       kAttributeUint64,
        /* Description */          "The current duck timestamp",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_DUCK_READER,
        /* Offset */               0x00,
        /* Size/length */          2,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xffffffff, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint64_set_value,
        /* GetValue */             attribute_uint64_get_value,
        /* SetToString */          duck_timestamp_to_string,
        /* SetFromString */        attribute_uint64_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};
static Attribute_t internal_duck_attr[] =
{
     {
        /* Name */                 "reg_id",
        /* Attribute Code */       kBooleanAttributeInternalDuckFlag,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Internal DUCK and CSP identification flag.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_DUCK,
        /* Offset */               DUCK_COMMAND,
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
        /* Name */                 "cfg_is_csp",
        /* Attribute Code */       kBooleanAttributeCSPInputSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "The CSP input signal.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_DUCK,
        /* Offset */               DUCK_CONFIG,
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
        /* Name */                 "cfg_os_csp",
        /* Attribute Code */       kBooleanAttributeCSPOutputSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "The CSP output select signal",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_DUCK,
        /* Offset */               DUCK_CONFIG,
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
        /* Name */                 "async_csp_output_enable",
        /* Attribute Code */       kBooleanAttributeCSPOutputPortEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "The CSP output port enable signal",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_DUCK,
        /* Offset */               DUCK_CONFIG,
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
        /* Name */                 "internal_duck_select_signal",
        /* Attribute Code */       kBooleanAttributeInternalDuckSelectSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "The Internal DUCK select signal.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_DUCK,
        /* Offset */               DUCK_CONFIG,
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
        /* Name */                 "pps_terminate_enable",
        /* Attribute Code */       kBooleanAttributePPSTerminateEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "PPS Terminate A enable.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_DUCK,
        /* Offset */               DUCK_CONFIG,
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
};
#define NB_ELEM     (sizeof(duck_reader_attr) / sizeof(Attribute_t))
#define NB_ELEM_NEW (sizeof(internal_duck_attr)/ sizeof(Attribute_t))


ComponentPtr
duck_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentDUCK, card); 
    
    if (NULL != result)
    {
        duck_state_t* state = NULL;
        component_set_dispose_routine(result, duck_dispose);
        component_set_post_initialize_routine(result, duck_post_initialize);
        component_set_reset_routine(result, duck_reset);
        component_set_default_routine(result, duck_default);
        component_set_name(result, "duck");
        component_set_description(result, "The DUCK (DAG Universal Clock Kit)");
        state = (duck_state_t*)malloc(sizeof(duck_state_t));
        memset(state, 0, sizeof(duck_state_t));
        component_set_private_state(result, state);
        state->mIndex = index;
    }
    
    return result;
}


static void
duck_dispose(ComponentPtr component)
{
}

static int
duck_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        AttributePtr attr;
        GenericReadWritePtr grw;
        DagCardPtr card = component_get_card(component);
        uintptr_t address = 0;
        uint32_t mask = 0;
		uint32_t value = 0;
        dag_reg_t result[DAG_REG_MAX_ENTRIES];
        duck_state_t *state = NULL;
        uint8_t* iom = NULL;
        
        /* We didn't use the generic read write object and attribute factory for reading/writing some of the duck attributes 
         * that we have to get using an ioctl from the driver. 
         */

        attr = duck_get_synchronized_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_threshold_attribute();
        component_add_attribute(component, attr);

		attr = duck_get_phase_correction_attribute();
		component_add_attribute(component,attr);

        attr = duck_get_failures_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_resyncs_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_frequency_error_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_phase_error_attribute();
        component_add_attribute(component, attr);
        
        attr = duck_get_worst_frequency_error_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_worst_phase_error_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_single_pulses_missing_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_pulses_attribute();
        component_add_attribute(component, attr);

	/*To add the Time Stamp Counter Value attribute*/
        attr = duck_get_timestamp_counter_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_crystal_frequency_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_synth_frequency_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_longest_pulse_missing_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_time_info_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_set_to_host_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_clear_stats_attribute();
        component_add_attribute(component, attr);

        attr = duck_get_sync_attribute();
        component_add_attribute(component, attr);

        address = (uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_DUCK, 0);
        address += DUCK_CONFIG;
        mask = BIT0;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKRS422Input, grw, &mask, 1);
        component_add_attribute(component, attr);

		mask = BIT1;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKHostInput, grw, &mask, 1);
        component_add_attribute(component, attr);

        mask = BIT2;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKOverInput, grw, &mask, 1);
        component_add_attribute(component, attr);

        mask = BIT3;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKAuxInput, grw, &mask, 1);
        component_add_attribute(component, attr);

        mask = BIT8;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKRS422Output, grw, &mask, 1);
        component_add_attribute(component, attr);

        mask = BIT9;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKLoop, grw, &mask, 1);
        component_add_attribute(component, attr);
 
        mask = BIT10;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKHostOutput, grw, &mask, 1);
        component_add_attribute(component, attr);

        mask = BIT11;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kBooleanAttributeDUCKOverOutput, grw, &mask, 1);
        component_add_attribute(component, attr);

		address = (uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_DUCK, 0);
		address += DUCK_DDS_RATE;
		mask = 0xffffffff;
		grw = grw_init(card,address,grw_iom_read,grw_iom_write);
		attr = duck_get_dds_rate_attribute(grw,&mask,1);
		component_add_attribute(component,attr);


        attr = duck_get_sync_timeout_attribute();
        component_add_attribute(component, attr);
	/* check for duck reader module, and if found, add the timestamp attribute */
        iom = (uint8_t*) card_get_iom_address(card);
        if (dag_reg_find((char*) iom, DAG_REG_DUCK_READER, result) > 0)
        {
            state = component_get_private_state(component);
            if ( NULL != state )
                read_attr_array( component, duck_reader_attr, NB_ELEM, state->mIndex );

        }
	/*Check out of the flag is set for Internal duck.If it is set add the other attributes.*/
	
	address = (uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_DUCK, 0);
    address += DUCK_COMMAND;
	
	grw = grw_init(card, address, grw_iom_read, grw_iom_write);
	value = grw_read(grw);
    grw_dispose(grw);
	if(value & BIT31)
	{
		state = component_get_private_state(component);
		if(NULL != state)
			read_attr_array(component,internal_duck_attr,NB_ELEM_NEW,state->mIndex );
	}
    }
    return 1;
}

static void
duck_reset(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = component_get_card(component);
	    uint32_t	magic = DAGRESET_DUCK;
#if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

        if(ioctl(card_get_fd(card), DAGIOCRESET, &magic) < 0)
            dagutil_warning("duck reset failed: %s\n", strerror(errno));

#elif defined(_WIN32)

		DWORD BytesTransfered = 0;
        if(DeviceIoControl(dag_gethandle(card_get_fd(card)),
                   IOCTL_DUCK_RESET,
                   NULL,
                   0,
                   NULL,
                   0,
                   &BytesTransfered,
                   NULL) == FALSE)
            dagutil_warning("duck reset failed: %s\n", strerror(errno));

#endif /* Platform-specific code. */
        duck_default(component);
    }
}


static void
duck_default(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        AttributePtr attr;
        uint8_t uint8_val = 0;

        attr = component_get_attribute(component, kBooleanAttributeDUCKRS422Input);
        uint8_val = 1;
        attribute_set_value(attr, &uint8_val, 1);

        attr = component_get_attribute(component, kBooleanAttributeDUCKHostInput);
        uint8_val = 0;
        attribute_set_value(attr, &uint8_val, 1);

        attr = component_get_attribute(component, kBooleanAttributeDUCKOverInput);
        attribute_set_value(attr, &uint8_val, 1);

        attr = component_get_attribute(component, kBooleanAttributeDUCKAuxInput);
        attribute_set_value(attr, &uint8_val, 1);

        attr = component_get_attribute(component, kBooleanAttributeDUCKRS422Output);
        attribute_set_value(attr, &uint8_val, 1);

        attr = component_get_attribute(component, kBooleanAttributeDUCKHostOutput);
        attribute_set_value(attr, &uint8_val, 1);

        attr = component_get_attribute(component, kBooleanAttributeDUCKOverOutput);
        attribute_set_value(attr, &uint8_val, 1);

        attr = component_get_attribute(component, kBooleanAttributeDUCKLoop);
        attribute_set_value(attr, &uint8_val, 1);
    }
}
void
duck_ioctl_read(duckinf_t* duckinf, int dagfd)
{

#if defined(_WIN32)
    DWORD BytesTransfered = 0;
#else
    int error;
#endif

#if defined (_WIN32)
    if(DeviceIoControl(dag_gethandle(dagfd),
        IOCTL_GET_DUCKINFO,
        duckinf,
        sizeof(duckinf_t),
        duckinf,
        sizeof(duckinf_t),
        &BytesTransfered,
        NULL) == FALSE)
        panic("DeviceIoControl IOCTL_GET_DUCKINFO: %s\n",strerror(errno));
#else
    if((error = ioctl(dagfd, DAGIOCDUCK, duckinf)))
        dagutil_panic("DAGIOCDUCK failed with %d\n", error);
#endif
}

static AttributePtr
duck_get_synchronized_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeDUCKSynchronized); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_synchronized_value);
        attribute_set_name(result, "synchronized");
        attribute_set_description(result, "Is the DUCK synchronized.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void*
duck_get_synchronized_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint8_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Health;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_threshold_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKThreshold); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_threshold_value);
        attribute_set_setvalue_routine(result, duck_set_threshold_value);
        attribute_set_name(result, "threshold");
        attribute_set_description(result, "Set threshold level for DUCK synchronisation. If phase error is less than threshold, DUCK is synchronised");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
    }
    
    return result;
}

static void
duck_set_threshold_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duckinf.Set_Duck_Field |= 0x2;
		duckinf.Health_Thresh = *(uint32_t*)value * (0x100000000ll/1000000000.0);
        duck_ioctl_read(&duckinf, card_get_fd(card));
    }
}

static void*
duck_get_threshold_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Health_Thresh;
        return &val;
    }
    return NULL;
}
/*Adding the new attribute for phase correction.*/
static AttributePtr
duck_get_phase_correction_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeDUCKPhaseCorrection); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_phase_correction_value);
        attribute_set_setvalue_routine(result, duck_set_phase_correction_value);
        attribute_set_name(result, "phase_correction");
        attribute_set_description(result, "Phase Correction Parameter for DUCK.Positive values correct for long GPS cable runs.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_from_string_routine(result, attribute_uint64_from_string);
    }
    
    return result;
}
static void
duck_set_phase_correction_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duckinf.Set_Duck_Field |= Set_Duck_Phase_Corr;
		duckinf.Phase_Correction = *(uint64_t*)value * (0x100000000ll/1000000000.0);
        duck_ioctl_read(&duckinf, card_get_fd(card));
    }
}
static void*
duck_get_phase_correction_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint64_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Phase_Correction;
        return &val;
    }
    return NULL;
}
/*End of new attribute for DUCK.*/
static AttributePtr
duck_get_failures_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKFailures); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_failures_value);
        attribute_set_name(result, "failures");
        attribute_set_description(result, "When DUCK phase error is greater than the DUCK threshold for 10 consecutive seconds, failures increments by one.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_failures_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Sickness;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_resyncs_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKResyncs); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_resyncs_value);
        attribute_set_name(result, "resyncs");
        attribute_set_description(result, "If the DUCK phase error (kUint32AttributeDUCKPhaseError) >= 1 second,"
                                          "the DUCK will 'reset' to factory defaults, except for the input"
                                          "selection and attempt to resynchronise. kUint32AttributeDUCKResyncs"
                                          "increments by one.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_resyncs_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Resyncs;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_dds_rate_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = attribute_init(kUint32AttributeDDSRate); 
	if (NULL != result)
	{
		attribute_set_generic_read_write_object(result,grw);
		attribute_set_name(result, "dds_rate");
		attribute_set_getvalue_routine(result, attribute_uint32_get_value);
		attribute_set_setvalue_routine(result, attribute_uint32_set_value);
		attribute_set_to_string_routine(result,attribute_uint32_to_string);
		attribute_set_from_string_routine(result,attribute_uint32_from_string);
		attribute_set_description(result, "Reports the rate at which DAG clock runs.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_masks(result,bit_masks,len);
		attribute_set_valuetype(result, kAttributeUint32);
	}
	return result;
}


static AttributePtr
duck_get_frequency_error_attribute(void)
{
    AttributePtr result = attribute_init(kInt32AttributeDUCKFrequencyError); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_frequency_error_value);
        attribute_set_name(result, "frequency_error");
        attribute_set_description(result, "DUCK clock frequency error in parts per billion (ppb).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeInt32);
        attribute_set_to_string_routine(result, attribute_int32_to_string);
    }
    
    return result;
}

static void*
duck_get_frequency_error_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Freq_Err;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_phase_error_attribute(void)
{
    AttributePtr result = attribute_init(kInt32AttributeDUCKPhaseError); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_phase_error_value);
        attribute_set_name(result, "phase_error");
        attribute_set_description(result, "DUCK phase error (offset) in nanoseconds (ns).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeInt32);
        attribute_set_to_string_routine(result, attribute_int32_to_string);
    }
    
    return result;
}

static void*
duck_get_phase_error_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Phase_Err;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_worst_frequency_error_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKWorstFrequencyError); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_worst_frequency_error_value);
        attribute_set_name(result, "worst_frequency_error");
        attribute_set_description(result, "Highest value of kUint32AttributeDUCKPhaseError since last DUCK reset or DUCK statistics reset.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_worst_frequency_error_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Worst_Freq_Err;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_worst_phase_error_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKWorstPhaseError); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_worst_phase_error_value);
        attribute_set_name(result, "worst_phase_error");
        attribute_set_description(result, "Highest value of kUint32AttributeDUCKPhaseError since last DUCK reset or DUCK statistics reset.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_worst_phase_error_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
	/*changing becasue this function should return the Worst Phase Error field in duckinf and not Phase Error*/
        val = duckinf.Worst_Phase_Err;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_crystal_frequency_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKCrystalFrequency); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_crystal_frequency_value);
        attribute_set_name(result, "crystal_frequency");
        attribute_set_description(result, "Estimated crystal oscillator frequency (Hz).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeInt32);
	//uses the deafult attribute to string handler which will be type depended 
//        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_crystal_frequency_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static int32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Crystal_Freq;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_synth_frequency_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKSynthFrequency); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_synth_frequency_value);
        attribute_set_name(result, "synth_frequency");
        attribute_set_description(result, "Target DUCK operating frequency (Hz)");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeInt32);
//        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_synth_frequency_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static int32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Synth_Freq;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_pulses_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKPulses); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_pulses_value);
        attribute_set_name(result, "pulses");
        attribute_set_description(result, "Count of synchronisation signals received.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_pulses_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Pulses;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_single_pulses_missing_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKSinglePulsesMissing); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_single_pulses_missing_value);
        attribute_set_name(result, "single_pulses_missing");
        attribute_set_description(result, "Count of times when a single expected synchronisation signal is missing.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_single_pulses_missing_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Single_Pulses_Missing;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_longest_pulse_missing_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDUCKLongestPulseMissing); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_longest_pulse_missing_value);
        attribute_set_name(result, "longest_pulse_missing");
        attribute_set_description(result, "Longest contiguous sequence of synchronisation signals missing.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void*
duck_get_longest_pulse_missing_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val = duckinf.Longest_Pulse_Missing;
        return &val;
    }
    return NULL;
}

static AttributePtr
duck_get_time_info_attribute(void)
{
    AttributePtr result = attribute_init(kStructAttributeDUCKTimeInfo); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, duck_get_time_info_value);
        attribute_set_name(result, "time_info");
        attribute_set_description(result, "The DUCK time information structure holds host time values for when DUCK statistics were last cleared and at last synchronisation event.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeStruct);
        attribute_set_to_string_routine(result, duck_time_info_to_string);
        attribute_set_from_string_routine(result, duck_time_info_from_string);
    }
    
    return result;
}

static void*
duck_get_time_info_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static duck_time_info_t val;
        duckinf_t duckinf;

        time_t last;
        
        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duck_ioctl_read(&duckinf, card_get_fd(card));
        val.mStart = duckinf.Stat_Start;
        val.mEnd = duckinf.Stat_End;
        /*Added to accomodate DUCK time at last sync pulse*/
        if(duckinf.Last_Ticks) 
        {
#if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
            last = (time_t)((duckinf.Last_Ticks >> 32) + (int)(((duckinf.Last_Ticks&0xffffffff) + (double)0x80000000)/0x100000000ll));
#elif defined(_WIN32)
            last = (time_t)((duckinf.Last_Ticks >> 32) + (int)(((duckinf.Last_Ticks&0xffffffff) + (double)0x80000000)/0x100000000));
#else 
#error The OS not supported contact support@endace.com
#endif
            val.dagTime = last;
        } 
        else
        {
            val.dagTime = 0;
        }
        return &val;
    }
    return NULL;
}

static void 
duck_time_info_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
duck_time_info_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
duck_get_set_to_host_attribute(void)
{
    AttributePtr result = attribute_init(kNullAttributeDUCKSetToHost); 
    
    if (NULL != result)
    {
        attribute_set_setvalue_routine(result, duck_set_to_host_value);
        attribute_set_name(result, "set_to_host");
        attribute_set_description(result, "Set the DAG clock to the host PC clock.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, duck_set_to_host_to_string);
        attribute_set_from_string_routine(result, duck_set_to_host_from_string);
    }
    
    return result;
}

static void
duck_set_to_host_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
	    uint32_t	magic = DAGRESET_DUCK;
#if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

        if(ioctl(card_get_fd(card), DAGIOCRESET, &magic) < 0)
            dagutil_warning("duck reset failed: %s\n", strerror(errno));

#elif defined(_WIN32)

		DWORD BytesTransfered = 0;
        if(DeviceIoControl(dag_gethandle(card_get_fd(card)),
                   IOCTL_DUCK_RESET,
                   NULL,
                   0,
                   NULL,
                   0,
                   &BytesTransfered,
                   NULL) == FALSE)
            dagutil_warning("duck reset failed: %s\n", strerror(errno));

#endif /* Platform-specific code. */
    }
}

static void 
duck_set_to_host_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
duck_set_to_host_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}


static AttributePtr
duck_get_clear_stats_attribute(void)
{
    AttributePtr result = attribute_init(kNullAttributeDUCKClearStats); 
    
    if (NULL != result)
    {
        attribute_set_setvalue_routine(result, duck_set_clear_stats_value);
        attribute_set_name(result, "clear_stats");
        attribute_set_description(result, "Clear the duck statistics.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, duck_clear_stats_to_string);
        attribute_set_from_string_routine(result, duck_clear_stats_from_string);
    }
    
    return result;
}

static void
duck_set_clear_stats_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        duckinf_t duckinf;

        memset(&duckinf, 0, sizeof(duckinf));
        card = attribute_get_card(attribute);
        duckinf.Set_Duck_Field = 1;
        duck_ioctl_read(&duckinf, card_get_fd(card));
    }
}

static void 
duck_clear_stats_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
duck_clear_stats_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
duck_get_sync_attribute(void)
{
    AttributePtr result = attribute_init(kNullAttributeDUCKSync); 
    
    if (NULL != result)
    {
        attribute_set_setvalue_routine(result, duck_set_sync_value);
        attribute_set_name(result, "sync");
        attribute_set_description(result, "Try to synchronize the DUCK.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, duck_sync_to_string);
        attribute_set_from_string_routine(result, duck_sync_from_string);
    }
    
    return result;
}

static void
duck_set_sync_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
#if defined(_WIN32)
        DWORD BytesTransfered = 0;
#else 
        int error;
#endif

        int timeout=0;
        duckinf_t duckinf;
        uint32_t* sync_timeout = NULL;
        AttributePtr attr_sync_timeout;
        ComponentPtr duck;
        DagCardPtr card = attribute_get_card(attribute);

        assert(card);
        duck = attribute_get_component(attribute);
        assert(duck);
        attr_sync_timeout = component_get_attribute(duck, kUint32AttributeDUCKSyncTimeout);
        sync_timeout = (uint32_t*)attribute_get_value(attr_sync_timeout);
        if (!sync_timeout)
            return;

        memset(&duckinf, 0, sizeof(duckinf));
/*this ioctl has been added to read the health bit just before synchronization starts so as to avoid the time delay*/
/*to reduce sync time dealy*/
        duckinf.Set_Duck_Field |= 0x1;
#if defined (_WIN32)
        if(DeviceIoControl(dag_gethandle(card_get_fd(card)),
                           IOCTL_GET_DUCKINFO,
                           &duckinf,
                           sizeof(duckinf_t),
                           &duckinf,
                           sizeof(duckinf_t),
                           &BytesTransfered,
                           NULL) == FALSE)
             dagutil_warning("DeviceIoControl IOCTL_GET_DUCKINFO: %s\n",strerror(errno));
#else
             if((error = ioctl(card_get_fd(card), DAGIOCDUCK, &duckinf)))
             dagutil_warning("DAGIOCDUCK failed with %d\n", error);
#endif
/*to reduce sync time dealy*/
        while((!duckinf.Health)&&(timeout < *sync_timeout)) {
            sleep(1);
            timeout++;
            duckinf.Set_Duck_Field |= Set_Duck_Clear_Stats;
#if defined (_WIN32)
        if(DeviceIoControl(dag_gethandle(card_get_fd(card)),
            IOCTL_GET_DUCKINFO,
            &duckinf,
            sizeof(duckinf_t),
            &duckinf,
            sizeof(duckinf_t),
            &BytesTransfered,
            NULL) == FALSE)
            dagutil_warning("DeviceIoControl IOCTL_GET_DUCKINFO: %s\n",strerror(errno));
#else 
        if((error = ioctl(card_get_fd(card), DAGIOCDUCK, &duckinf)))
            dagutil_warning("DAGIOCDUCK failed with %d\n", error);
#endif
            
        }
        if(!duckinf.Health)
            dagutil_warning("Failed to synchronise after %us\n", *sync_timeout);
        else /* if we had to wait, wait 3 more seconds and clear to ensure stats are stable */
            if(timeout) {
                sleep(3);
                duckinf.Set_Duck_Field |= Set_Duck_Clear_Stats;
#if defined (_WIN32)
        if(DeviceIoControl(dag_gethandle(card_get_fd(card)),
            IOCTL_GET_DUCKINFO,
            &duckinf,
            sizeof(duckinf_t),
            &duckinf,
            sizeof(duckinf_t),
            &BytesTransfered,
            NULL) == FALSE)
            dagutil_warning("DeviceIoControl IOCTL_GET_DUCKINFO: %s\n",strerror(errno));
#else
        if((error = ioctl(card_get_fd(card), DAGIOCDUCK, &duckinf)))
            dagutil_warning("DAGIOCDUCK failed with %d\n", error);
#endif
        }
    }
}

static void 
duck_sync_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
duck_sync_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
duck_get_sync_timeout_attribute(void)
{
    AttributePtr result = NULL;
    uint32_t* val = (uint32_t*)malloc(sizeof(uint32_t));

    memset((void*)val, 0, sizeof(*val));
    *val = 60;
    result = attribute_init(kUint32AttributeDUCKSyncTimeout);
    attribute_set_name(result, "sync_timeout");
    attribute_set_description(result, "The time out value in seconds before the synchronization operation fails.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, duck_sync_timeout_get_value);
    attribute_set_setvalue_routine(result, duck_sync_timeout_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_private_state(result, val);
    
    return result;
}

void
duck_sync_timeout_set_value(AttributePtr attribute, void* val, int len)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t* ps = (uint32_t*)attribute_get_private_state(attribute);
        if (!ps)
            return;
        *ps = *(uint32_t*)val;
    }
}

void*
duck_sync_timeout_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t* ps = (uint32_t*)attribute_get_private_state(attribute);
        if (!ps)
            return NULL;
        return ps;
    }
    return NULL;
}
/*function to get time stamp counter value*/
void*
duck_get_timestamp_counter_value(AttributePtr attribute)
{
      if (1 == valid_attribute(attribute))
      {
          DagCardPtr card = NULL;
          static uint64_t val = 0;
          duckinf_t duckinf;

          memset(&duckinf, 0, sizeof(duckinf));
          card = attribute_get_card(attribute);
          duck_ioctl_read(&duckinf, card_get_fd(card));
          val = duckinf.Last_TSC;
          return &val;
      }
      return NULL;
}
static AttributePtr duck_get_timestamp_counter_attribute(void)
{
       AttributePtr result = attribute_init(kUint64AttributeDUCKTSC);
       if (NULL != result)
       {
           attribute_set_getvalue_routine(result, duck_get_timestamp_counter_value);
           attribute_set_name(result, "time_stamp_counter_value");
           attribute_set_description(result, "Value of the time stamp counter.");
           attribute_set_config_status(result, kDagAttrStatus);
           attribute_set_valuetype(result, kAttributeUint64);
           attribute_set_to_string_routine(result, attribute_uint64_to_string);
       }
       return result;
}

void 
duck_timestamp_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        struct tm* dtime;
		char time_string[80];
		char final_string[150];
        time_t time_val;
        uint64_t *ptr_time = (uint64_t*) attribute_uint64_get_value( attribute );
        if ( NULL != ptr_time )
        {
#if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
         time_val  = (time_t)((*ptr_time  >> 32)); 
#elif defined(_WIN32)
         time_val  = (time_t)((*ptr_time  >> 32)); 
#endif
         dtime = gmtime((time_t*)(&time_val));
	     strftime(time_string, sizeof(time_string),"%Y-%m-%d %H:%M:%S",dtime);
	     memset(final_string,'\0',150);
#if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	     sprintf(final_string,"%s.%09.0f UTC \n",time_string,(double)(*ptr_time & 0xffffffffll)/0x100000000ll * 1000000000);
#elif defined(_WIN32)
		 sprintf(final_string,"%s.%09.0f UTC \n",time_string,(double)(ULONG)(*ptr_time & 0xffffffffll)/0x100000000i64 * 1000000000);
#endif
         attribute_set_to_string(attribute,final_string);
    	}
	}
}
