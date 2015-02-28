/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
*/

#include "../include/attribute.h"
#include "../include/component.h"
#include "../include/create_attribute.h"

#include "../include/components/sonet_channel_mgmt_component.h"
#include "../include/components/sonet_connection_component.h"

#define BUFFER_SIZE 1024

static void sonet_channel_mgmt_dispose(ComponentPtr component);
static void sonet_channel_mgmt_reset(ComponentPtr component);
static void sonet_channel_mgmt_default(ComponentPtr component);
static int sonet_channel_mgmt_post_initialize(ComponentPtr component);

static void refresh_config_memory_set_value (AttributePtr attribute, void* value, int length);
static void* refresh_config_memory_get_value (AttributePtr attribute);

static void raw_spe_extract_vc_id_set_value (AttributePtr attribute, void* value, int length);
static void* raw_spe_extract_vc_id_get_value (AttributePtr attribute);

//static void raw_spe_extract_slen_set_value (AttributePtr attribute, void* value, int length);
//static void* raw_spe_extract_slen_get_value (AttributePtr attribute);

static void load_channel_config_memory (ComponentPtr component);
static void read_channel_detect (ComponentPtr component);
static void process_channel_size (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset);
static void process_channel_size_oc3 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset);
#if 0
static void process_channel_size_oc12 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset);
static void process_channel_size_oc48 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset);
static void process_channel_size_oc192 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset);
static void process_channel_size_oc768 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset);
#endif
static uint8_t check_valid_pointers (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t offset);
static uint8_t check_valid_pointers_oc3 (ComponentPtr component, uint8_t rate, uint16_t range, uint32_t group, uint16_t factor, uint16_t offset);
static void write_channel_config_memory (ComponentPtr component);
static uint32_t execute_channel_config_mem_write (ComponentPtr component, uint32_t addr, uint32_t data);
static void write_loh_mask (ComponentPtr component);
static uint32_t execute_loh_mask_write (ComponentPtr component, uint32_t addr, uint32_t data);
static void create_connection_subcomponents (ComponentPtr component);

typedef enum
{
	kPointerValid = 0x1,
	kConcatenation = 0x2,
	kPointerInvalid = 0x0
}sonet_channel_ptr_setting_t;

typedef enum
{
	kOC1	= 0x0,
	kOC3	= 0x1,
	kOC12	= 0x2,
	kOC48	= 0x3,
	kOC192	= 0x4,
	kOC768	= 0x5
}sonet_network_rates_t;

typedef enum
{
	kSONETChannelDetectAddress		= 0x00,
	kSONETChannelDetectData			= 0x04,
	kSONETChannelConfigMemoryControlAddress	= 0x00,
	kSONETChannelConfigMemoryDataWrite	= 0x04,
	kSONETChannelConfigMemoryDataRead	= 0x08,
	kSONETRawSPEExtractControl		= 0x00,
	kSONETRawSPEExtractLOHMaskControlAddress = 0x04,
	kSONETRawSPEExtractLOHMaskDataWrite	= 0x08,
	kSONETRawSPEExtractLOHMaskDataRead	= 0x0C
} sonet_channel_mgmt_register_offset_t;

				      // OC1  OC3  OC12 OC48 OC192 OC768 - groups
static uint16_t channel_groups[6][6] = {{1,   0,   0,   0,   0,    0},	// OC1 original rate
					{3,   1,   0,   0,   0,    0},	// OC3 original rate
					{12,  4,   1,   0,   0,    0},	// OC12 original rate
					{48,  16,  4,   1,   0,    0},	// OC48 original rate
					{192, 64,  16,  4,   1,    0},	// OC192 original rate
					{768, 256, 64,  16,  4,    1}};	// OC768 original rate

Attribute_t sonet_channel_mgmt_attr[]=
{
	{
		/* Name */                 "channel_detect_addr",
		/* Attribute Code */       kUint32AttributeChannelDetectAddr,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Channel Detect module address.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_CHANNEL_DETECT,
		/* Offset */               kSONETChannelDetectAddress,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x0000000F,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

#if 0
	{
		/* Name */                 "channel_detect_rate",
		/* Attribute Code */       kUint32AttributeLineRate,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Channel Detect module rate.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_CHANNEL_DETECT,
		/* Offset */               kSONETChannelDetectAddress,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xF0000000,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},
#endif

	{
		/* Name */                 "channel_detect_data",
		/* Attribute Code */       kUint32AttributeChannelDetectData,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Channel Detect module data readout.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_CHANNEL_DETECT,
		/* Offset */               kSONETChannelDetectData,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_hex_string,
		/* SetFromString */        attribute_uint32_from_hex_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "channel_config_mem_addr",
		/* Attribute Code */       kUint32AttributeChannelConfigMemAddr,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Channel Config Memory module address.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_CHANNEL_CONFIG_MEM,
		/* Offset */               kSONETChannelConfigMemoryControlAddress,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x000003FF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "channel_config_mem_channelized_mode",
		/* Attribute Code */       kBooleanAttributeChannelizedMode,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "SONET Channel Config Memory module channelized mode (applies config settings to the core, also supports concatenated traffic).",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    30,
		/* Register Address */     DAG_REG_SONET_CHANNEL_CONFIG_MEM,
		/* Offset */               kSONETChannelConfigMemoryControlAddress,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT30,
		/* Default Value */        1,
		/* SetValue */             attribute_boolean_set_value,
		/* GetValue */             attribute_boolean_get_value,
		/* SetToString */          attribute_boolean_to_string,
		/* SetFromString */        attribute_boolean_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "channel_config_mem_write_enable",
		/* Attribute Code */       kBooleanAttributeWriteEnable,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "SONET Channel Config Memory module write enable.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    31,
		/* Register Address */     DAG_REG_SONET_CHANNEL_CONFIG_MEM,
		/* Offset */               kSONETChannelConfigMemoryControlAddress,
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
		/* Name */                 "channel_config_mem_data_write",
		/* Attribute Code */       kUint32AttributeChannelConfigMemDataWrite,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Channel Config Memory module write data.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_CHANNEL_CONFIG_MEM,
		/* Offset */               kSONETChannelConfigMemoryDataWrite,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_hex_string,
		/* SetFromString */        attribute_uint32_from_hex_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "channel_config_mem_data_read",
		/* Attribute Code */       kUint32AttributeChannelConfigMemDataRead,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Channel Config Memory module read data.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_CHANNEL_CONFIG_MEM,
		/* Offset */               kSONETChannelConfigMemoryDataRead,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_hex_string,
		/* SetFromString */        attribute_uint32_from_hex_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "raw_spe_extract_vc_id_select",
		/* Attribute Code */       kUint32AttributeVCIDSelect,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Raw Frame Extraction module channel identification (VC-ID) value select.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractControl,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x000003FF,
		/* Default Value */        0,
		/* SetValue */             raw_spe_extract_vc_id_set_value,
		/* GetValue */             raw_spe_extract_vc_id_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "raw_spe_extract_vc_id_force",
		/* Attribute Code */       kBooleanAttributeVCIDForce,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "SONET Raw Frame Extraction module VC-ID force setting to overwrite channel selection.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    10,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractControl,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT10,
		/* Default Value */        1,
		/* SetValue */             attribute_boolean_set_value,
		/* GetValue */             attribute_boolean_get_value,
		/* SetToString */          attribute_boolean_to_string,
		/* SetFromString */        attribute_boolean_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "raw_spe_extract_slen",
		/* Attribute Code */       kUint32AttributeSPESnaplength,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Raw Frame Extraction module SPE snaplength (in bytes).",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    16,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractControl,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x3FFF0000,
		/* Default Value */        128,
		/* SetValue */             attribute_uint32_set_value, //raw_spe_extract_slen_set_value,
		/* GetValue */             attribute_uint32_get_value, //raw_spe_extract_slen_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "raw_spe_select_poh_capture_mode",
		/* Attribute Code */       kBooleanAttributePOHCaptureMode,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "SONET Raw SPE Extraction module POH capture mode.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    31,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractControl,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT31,
		/* Default Value */        1,
		/* SetValue */             attribute_boolean_set_value,
		/* GetValue */             attribute_boolean_get_value,
		/* SetToString */          attribute_boolean_to_string,
		/* SetFromString */        attribute_boolean_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "raw_spe_extract_loh_mask_addr",
		/* Attribute Code */       kUint32AttributeLOHMaskAddr,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Raw SPE Extraction module LOH mask address.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractLOHMaskControlAddress,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x000000FF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "raw_spe_extract_loh_mask_write_enable",
		/* Attribute Code */       kBooleanAttributeLOHMaskWriteEnable,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "SONET Raw SPE Extraction module LOH mask write enable.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    31,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractLOHMaskControlAddress,
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
		/* Name */                 "raw_spe_extract_loh_mask_data_write",
		/* Attribute Code */       kUint32AttributeLOHMaskDataWrite,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Raw SPE Extraction module LOH mask write data.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractLOHMaskDataWrite,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "raw_spe_extract_loh_mask_data_read",
		/* Attribute Code */       kUint32AttributeLOHMaskDataRead,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "SONET Raw SPE Extraction module LOH mask read data.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_SONET_RAW_SPE_EXTRACT,
		/* Offset */               kSONETRawSPEExtractLOHMaskDataRead,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "refresh_config_memory",
		/* Attribute Code */       kBooleanAttributeRefreshConfigMem,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "Refresh the SONET Channel Config Memory using the configuration information from the SONET Channel Detect module.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,					// Don't care
		/* Register Address */     DAG_REG_SONET_CHANNEL_CONFIG_MEM,	// Don't care
		/* Offset */               0,					// Don't care
		/* Size/length */          1,					// Don't care
		/* Read */                 NULL,				// Don't care
		/* Write */                NULL,				// Don't care
		/* Mask */                 0,					// Don't care
		/* Default Value */        0,
		/* SetValue */             refresh_config_memory_set_value,
		/* GetValue */             refresh_config_memory_get_value,
		/* SetToString */          attribute_boolean_to_string,
		/* SetFromString */        attribute_boolean_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	}

};


/* Number of elements in array */
#define NB_ELEM (sizeof(sonet_channel_mgmt_attr) / sizeof(Attribute_t))

ComponentPtr
sonet_channel_mgmt_get_new_component (DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentConnectionSetup, card);
    char buffer[BUFFER_SIZE];

    if (NULL != result)
    {
        sonet_channel_mgmt_state_t* state = (sonet_channel_mgmt_state_t*)malloc(sizeof(sonet_channel_mgmt_state_t));
        component_set_dispose_routine(result, sonet_channel_mgmt_dispose);
        component_set_reset_routine(result, sonet_channel_mgmt_reset);
        component_set_default_routine(result, sonet_channel_mgmt_default);
        component_set_post_initialize_routine(result, sonet_channel_mgmt_post_initialize);
        sprintf(buffer, "sonet_channel_mgmt%d", index);
        component_set_name(result, buffer);
        component_set_description(result, "The SONET/SDH Channel Management Module.");
        state->mIndex = index;
        component_set_private_state(result, state);

    }

    return result;
}

static void
sonet_channel_mgmt_dispose (ComponentPtr component)
{
	// Unused
}

static void
sonet_channel_mgmt_reset (ComponentPtr component)
{
	// Unused
}

static void
sonet_channel_mgmt_default (ComponentPtr component)
{
	AttributePtr attribute = NULL;
	void* val;

	// Set all the attribute default values
	if (1 == valid_component(component))
	{
		int count = component_get_attribute_count(component);
		int index;

		for (index = 0; index < count; index++)
		{
			attribute = component_get_indexed_attribute(component, index);
			val = attribute_get_default_value(attribute);
			attribute_set_value(attribute, &val, 1);
		}

	}
}

static int
sonet_channel_mgmt_post_initialize (ComponentPtr component)
{
	AttributePtr chan_config_mem_chan_mode_en = NULL;
	uint16_t count;
	uint8_t bool_val = 1;

	if (1 == valid_component(component))
	{
		sonet_channel_mgmt_state_t* state = NULL;
		state = component_get_private_state(component);

		// Add module attributes.
		read_attr_array(component, sonet_channel_mgmt_attr, NB_ELEM, state->mIndex);

		// Clear the private state timeslot information
		for (count = 0; count < MAX_TIMESLOTS; count++)
			memset (&(state->timeslot_info[count]), 0, sizeof(timeslot_information_t));

		// Enable channelized mode, to allow the Channel Config Memory to be applied to the core
		chan_config_mem_chan_mode_en = component_get_attribute(component, kBooleanAttributeChannelizedMode);	// Don't need grw object for the write enable boolean
		attribute_boolean_set_value(chan_config_mem_chan_mode_en, (void*)&bool_val, 1);

		// Load Channel Config Memory
		load_channel_config_memory(component);

		// Use channel information to dynamically create connection subcomponents
		create_connection_subcomponents(component);

		return 1;
	}
	return kDagErrInvalidParameter;
}


static void
refresh_config_memory_set_value (AttributePtr attribute, void* value, int len)
{
	ComponentPtr component = attribute_get_component(attribute);

	if (1 == valid_attribute(attribute))
	{
		if (*(uint8_t*)value == 1)
		{
			load_channel_config_memory(component);
	
			//create_connection_subcomponents(component);
		}
	}
}

static void*
refresh_config_memory_get_value (AttributePtr attribute)
{
	uint8_t value = 1;

	if (1 == valid_attribute(attribute))
	{
		// Always returns 1 for now
		attribute_set_value_array(attribute, &value, sizeof(value));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

static void
raw_spe_extract_vc_id_set_value (AttributePtr attribute, void* value, int len)
{
	ComponentPtr component = attribute_get_component(attribute);
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);
	uint32_t timeslot_value;
	uint16_t count;
	DagCardPtr card = component_get_card (component);
	uint32_t component_base;
	uint32_t register_value;

	if (1 == valid_attribute(attribute))
	{
		if (*(uint32_t *)value >= state->vc_id_total)
		{
			dagutil_warning("invalid VC-ID value, valid values are 0-%d, setting to default 0\n", state->vc_id_total - 1);
			*(uint32_t *)value = 0;	// Set to channel 0 as this channel will always exist
		}

		// Perform the mapping conversion here from vc_id to timeslot_id
		for (count = 0; count < state->timeslots; count++)
		{
			if (*(uint32_t *)value == state->timeslot_info[count].vc_id)
			{
				timeslot_value = state->timeslot_info[count].timeslot_id;
				break;
			}
		}

		attribute_uint32_set_value(attribute, &timeslot_value, 1);

		// Perform LOH mask setup if in Line Mode 2
		component_base = card_get_register_address(card, DAG_REG_SONET_CHANNEL_CONFIG_MEM, state->mIndex);
		register_value = card_read_iom(card, component_base + 0xC);
		register_value = (register_value >> 28) & 0x3;
	
		if (register_value == 0x1)
			write_loh_mask(component);
	}
}

static void*
raw_spe_extract_vc_id_get_value (AttributePtr attribute)
{
	GenericReadWritePtr attribute_grw = NULL;
	ComponentPtr component = attribute_get_component(attribute);
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);
	uint32_t timeslot_value = 0;
	uint32_t *masks;
	uint32_t value = 0;
	uint16_t count;

	if (1 == valid_attribute(attribute))
	{
		attribute_grw = attribute_get_generic_read_write_object(attribute);

		timeslot_value = grw_read(attribute_grw);
		masks = attribute_get_masks(attribute);
		timeslot_value &= *masks;

		// Perform the reverse mapping conversion here from timeslot_id to vc_id
		for (count = 0; count < state->timeslots; count++)
		{
			if (timeslot_value == state->timeslot_info[count].timeslot_id)
			{
				value = state->timeslot_info[count].vc_id;
				break;
			}
		}

		attribute_set_value_array(attribute, &value, sizeof(value));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

#if 0
static void
raw_spe_extract_slen_set_value (AttributePtr attribute, void* value, int len)
{
	AttributePtr slen_attribute;
	ComponentPtr connection_component;
	ComponentPtr component = attribute_get_component(attribute);
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	if (1 == valid_attribute(attribute))
	{
		// Set the snaplength attribute for the selected channel vc-id
		connection_component = component_get_subcomponent(component, kComponentConnection, state->vc_id_selected);
		slen_attribute = component_get_attribute(connection_component, kUint32AttributeSnaplength);
		attribute_uint32_set_value(slen_attribute, &value, 1);

		state->vc_id_slen = *(uint32_t *)value;
		attribute_uint32_set_value(attribute, &value, 1);
	}
}

static void*
raw_spe_extract_slen_get_value (AttributePtr attribute)
{
	ComponentPtr component = attribute_get_component(attribute);
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);
	uint32_t value = 0;

	if (1 == valid_attribute(attribute))
	{
		value = state->vc_id_slen;

		attribute_set_value_array(attribute, &value, sizeof(value));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}
#endif

static void
load_channel_config_memory (ComponentPtr component)
{
	DagCardPtr card = component_get_card (component);
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);
	uint32_t component_base;
	uint32_t register_value;

	component_base = card_get_register_address(card, DAG_REG_SONET_CHANNEL_CONFIG_MEM, state->mIndex);
	register_value = card_read_iom(card, component_base + 0xC);
	register_value = (register_value >> 28) & 0x3;

	read_channel_detect(component);

	write_channel_config_memory(component);

	// Perform LOH mask setup if in Line Mode 2
	if (register_value == 0x1)
		write_loh_mask(component);
}

static void
read_channel_detect (ComponentPtr component)
{
	AttributePtr chan_detect_addr;
	AttributePtr chan_detect_data;
	AttributePtr line_rate;
	GenericReadWritePtr chan_detect_addr_grw;
	GenericReadWritePtr chan_detect_data_grw;
	GenericReadWritePtr line_rate_grw;
	ComponentPtr root = NULL;
	ComponentPtr sonet_pp = NULL;
	uint32_t register_value = 0;
	uint32_t *masks;
	uint32_t iteration;
	uint32_t chan_detect_addr_val = 0;
	uint32_t timeslot = 0;
	uint8_t indicator = 0;
	uint32_t count;
	uint8_t stop = 0;
	uint16_t vc_id = 0;
	uint16_t factor = 0;
	uint8_t run_once = 1;
	uint8_t rate;
	uint16_t range = 0;
	uint16_t group = 0;

// OC48 Channel Config test vectors
#if 0
	// OC48c
	uint32_t test_config[4] = {0xaaaaaaa9,
				   0xaaaaaaaa,
				   0xaaaaaaaa,
				   0x00000000};
#endif
#if 0
	// OC48: 4 x OC12c
	uint32_t test_config[4] = {0xa9a9a9a9,
				   0xaaaaaaaa,
				   0xaaaaaaaa,
				   0x00000000};
#endif
#if 0
	// OC48: 16 x OC3c
	uint32_t test_config[4] = {0x55555555,
				   0xaaaaaaaa,
				   0xaaaaaaaa,
				   0x00000000};
#endif
#if 0
	// OC48: Mixed modes
	uint32_t test_config[4] = {0x55a9a955,
				   0xa9aaaa5a,
				   0xa9aaaa5a,
				   0x00000000};
#endif

	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Create grw objects for the SONET Channel Detect module
	chan_detect_addr = component_get_attribute(component, kUint32AttributeChannelDetectAddr);
	chan_detect_addr_grw = attribute_get_generic_read_write_object(chan_detect_addr);
	chan_detect_data = component_get_attribute(component, kUint32AttributeChannelDetectData);
	chan_detect_data_grw = attribute_get_generic_read_write_object(chan_detect_data);

	// Retrieve line rate value from the SONET PP component
	root = component_get_parent(component);
	sonet_pp = component_get_subcomponent(root, kComponentSonetPP, state->mIndex); // Use the same index as the SONET channel management component
	line_rate = component_get_attribute(sonet_pp, kUint32AttributeLineRate);
	line_rate_grw = attribute_get_generic_read_write_object(line_rate);
	register_value = grw_read(line_rate_grw);
	masks = attribute_get_masks(line_rate);
	register_value &= *masks;
	register_value >>= 4;
	
	state->network_rate = register_value;
	if (state->network_rate == kOC1)
		state->timeslots = 1;
	else if (state->network_rate == kOC3)
		state->timeslots = 3;
	else if (state->network_rate == kOC12)
		state->timeslots = 12;
	else if (state->network_rate == kOC48)
		state->timeslots = 48;
	else if (state->network_rate == kOC192)
		state->timeslots = 192;
	else if (state->network_rate == kOC768)
		state->timeslots = 768;

	//printf("DEBUG: total timeslots: %d\n", state->timeslots);

	// Read timeslot configuration from channel detect module and process vc-id and timeslot-id settings dynamically while reading
	while (!stop)
	{
		register_value = grw_read(chan_detect_addr_grw);
		masks = attribute_get_masks(chan_detect_addr);
		register_value &= ~(*masks);
		register_value |= chan_detect_addr_val;
		grw_write(chan_detect_addr_grw, register_value);

#if 1
		register_value = grw_read(chan_detect_data_grw);
#else
		register_value = test_config[chan_detect_addr_val];
#endif
		//printf("DEBUG: port: %d, channel detect data readout: 0x%08x\n", state->mIndex, register_value);
		
		++chan_detect_addr_val;

		// Iterate over every 2 bits of the 32-bit register value
		for (iteration = 0; iteration < 16; iteration++)
		{
			indicator = (register_value >> (iteration * 2)) & 0x3;
			state->timeslot_info[timeslot].indicator = indicator;	// Store the indicator for later use in calculating channel size

			if (indicator == kPointerValid)
			{
				state->timeslot_info[timeslot].timeslot_id = timeslot;
				state->timeslot_info[timeslot].vc_id = vc_id;
				++vc_id;
			}
			else if ((indicator == kConcatenation) && (timeslot < (state->timeslots/3)))
			{
				// Set the timeslot and VC-ID to the previous values
				state->timeslot_info[timeslot].timeslot_id = state->timeslot_info[timeslot - 1].timeslot_id;
				state->timeslot_info[timeslot].vc_id = state->timeslot_info[timeslot - 1].vc_id;
			}
			else if ((indicator == kConcatenation) && (timeslot >= (state->timeslots/3)))
			{
				// Check what range the timeslot is in to know what type of concatenation is present
				if (run_once)
				{
					if ((timeslot >= 2) && (timeslot <= 7))		// OC3
						factor = 1;
					if ((timeslot >= 4) && (timeslot <= 7))		// OC12
						factor = 4;
					else if ((timeslot >= 16) && (timeslot <= 31))	// OC48
						factor = 16;
					else if ((timeslot >= 64) && (timeslot <= 127))	// OC192
						factor = 64;
					else if ((timeslot >= 256) && (timeslot <= 511))// OC768
						factor = 256;

					run_once = 0;
				}

				state->timeslot_info[timeslot].timeslot_id = state->timeslot_info[timeslot - factor].timeslot_id;
				state->timeslot_info[timeslot].vc_id = state->timeslot_info[timeslot - factor].vc_id;
			}
			//else if (indicator == kPointerInvalid)
			//{
			//	dagutil_warning("invalid pointer 0x00 received in valid timeslot: %d (total timeslots: %d)\n", timeslot, state->timeslots);
			//}

			++timeslot;
			if (state->timeslots == timeslot)
			{
				stop = 1;
				break;
			}
		}
	}

	// Channel size is processed pseudo-recursively based on timeslot indicator information
	rate = state->network_rate;
	range = state->timeslots / 3;
	group = 0;
	//printf("MAIN: DEBUG: rate: %d, range: %d, group: %d, factor: %d\n", rate, range, group, factor);

#if 0
	if (state->network_rate == kOC3)
		process_channel_size_oc3(component, rate, range, group, factor, 0);
	else if (state->network_rate == kOC12)
		process_channel_size_oc12(component, rate, range, group, factor, 0);
	else if (state->network_rate == kOC48)
		process_channel_size_oc48(component, rate, range, group, factor, 0);
	else if (state->network_rate == kOC192)
		process_channel_size_oc192(component, rate, range, group, factor, 0);
	else if (state->network_rate == kOC768)
		process_channel_size_oc768(component, rate, range, group, factor, 0);
#else
	if (state->network_rate == kOC3)
		process_channel_size_oc3(component, rate, range, group, factor, 0);
	else
		process_channel_size(component, rate, range, group, factor, 0);
#endif

#if 1
	// Print the derived configuration
	for (count = 0; count < state->timeslots; count++)
	{
		dagutil_verbose("port: %d, timeslot: %d, indicator: %c, vc-id: %d, timeslot-id: %d, size: %d\n",
		//printf ("DEBUG: port: %d, timeslot: %d, indicator: %c, vc-id: %d, timeslot-id: %d, size: %d\n",
			state->mIndex,
			count,
			state->timeslot_info[count].indicator == 0x1 ? 'P' : 'C',
			state->timeslot_info[count].vc_id,
			state->timeslot_info[count].timeslot_id,
			state->timeslot_info[count].size);
	}
#endif

	// Store the total number of vc-ids
	state->vc_id_total = vc_id;
}

#if 0
static void
process_channel_size_oc768 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset)
{
	uint8_t original_rate = rate;
	uint8_t result;
	uint32_t count;
	uint16_t offset;
	uint32_t iteration;
	uint16_t groups;
	uint8_t stop = 0;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Start testing at next lower rate
	--rate;

	while (!stop)
	{
		offset = (group * range) + parent_offset;
		//printf("OC768: DEBUG-1: rate: %d, range: %d, group: %d, offset: %d\n", rate, range, group, offset);
		result = check_valid_pointers(component, rate, range, offset);
		//printf("OC768: DEBUG-2: pointer check result: %d\n", result);

		if (result == 0) // Concatenation indicators found
		{
			++rate;

			// Concatenation at the network rate
			if (rate == original_rate)
			{
				for(iteration = 0; iteration < range; iteration++)
				{
					state->timeslot_info[iteration + offset].size = original_rate;			// Set lower third
					state->timeslot_info[iteration + offset + factor].size = original_rate;		// Set middle third
					state->timeslot_info[iteration + offset + (factor * 2)].size = original_rate;	// Set upper third
				}
				break;
			}
			else
			{
				if (rate == kOC3)
					range = 3 / 3;
				else if (rate == kOC12)
					range = 12 / 3;
				else if (rate == kOC48)
					range = 48 / 3;
				else if (rate == kOC192)
					range = 192 / 3;

				groups = channel_groups[original_rate][rate];
				//printf("OC768: DEBUG-3: rate: %d, range: %d, total groups: %d, factor: %d\n", rate, range, groups, factor);

				for (count = 0; count < groups; count++)
				{
					if (rate == kOC12)
						process_channel_size_oc12(component, rate, range, count, factor, offset);
					if (rate == kOC48)
						process_channel_size_oc48(component, rate, range, count, factor, offset);
					if (rate == kOC192)
						process_channel_size_oc192(component, rate, range, count, factor, offset);
				}
				stop = 1;
			}
		}
		else if ((result == 1) && (rate != kOC3)) // Valid pointer indicators found
		{
			// Try next lower rate
			--rate;
			//printf("OC768: DEBUG-4: trying next lower rate: %d\n", rate);
		}
		else if ((result == 1) && (rate == kOC3))
		{
			groups = channel_groups[original_rate][rate];

			for (count = 0; count < groups; count++)
				process_channel_size_oc3(component, rate, range, count, factor, offset);

			stop = 1;
		}
	}
}

static void
process_channel_size_oc192 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset)
{
	uint8_t original_rate = rate;
	uint8_t result;
	uint32_t count;
	uint16_t offset;
	uint32_t iteration;
	uint16_t groups;
	uint8_t stop = 0;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Start testing at next lower rate
	--rate;

	while (!stop)
	{
		offset = (group * range) + parent_offset;
		//printf("OC192: DEBUG-1: rate: %d, range: %d, group: %d, offset: %d\n", rate, range, group, offset);
		result = check_valid_pointers(component, rate, range, offset);
		//printf("OC192: DEBUG-2: pointer check result: %d\n", result);

		if (result == 0) // Concatenation indicators found
		{
			++rate;

			// Concatenation at the network rate
			if (rate == original_rate)
			{
				for(iteration = 0; iteration < range; iteration++)
				{
					state->timeslot_info[iteration + offset].size = original_rate;			// Set lower third
					state->timeslot_info[iteration + offset + factor].size = original_rate;		// Set middle third
					state->timeslot_info[iteration + offset + (factor * 2)].size = original_rate;	// Set upper third
				}
				break;
			}
			else
			{
				if (rate == kOC3)
					range = 3 / 3;
				else if (rate == kOC12)
					range = 12 / 3;
				else if (rate == kOC48)
					range = 48 / 3;

				groups = channel_groups[original_rate][rate];
				//printf("OC192: DEBUG-3: rate: %d, range: %d, total groups: %d, factor: %d\n", rate, range, groups, factor);

				for (count = 0; count < groups; count++)
				{
					if (rate == kOC12)
						process_channel_size_oc12(component, rate, range, count, factor, offset);
					if (rate == kOC48)
						process_channel_size_oc48(component, rate, range, count, factor, offset);
				}
				stop = 1;
			}
		}
		else if ((result == 1) && (rate != kOC3)) // Valid pointer indicators found
		{
			// Try next lower rate
			--rate;
			//printf("OC192: DEBUG-4: trying next lower rate: %d\n", rate);
		}
		else if ((result == 1) && (rate == kOC3))
		{
			groups = channel_groups[original_rate][rate];

			for (count = 0; count < groups; count++)
				process_channel_size_oc3(component, rate, range, count, factor, offset);

			stop = 1;
		}
	}
}

static void
process_channel_size_oc48 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset)
{
	uint8_t original_rate = rate;
	uint8_t result;
	uint32_t count;
	uint16_t offset;
	uint32_t iteration;
	uint16_t groups;
	uint8_t stop = 0;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Start testing at next lower rate
	--rate;

	while (!stop)
	{
		offset = (group * range) + parent_offset;
		//printf("OC48: DEBUG-1: rate: %d, range: %d, group: %d, offset: %d\n", rate, range, group, offset);
		result = check_valid_pointers(component, rate, range, offset);
		//printf("OC48: DEBUG-2: pointer check result: %d\n", result);

		if (result == 0) // Concatenation indicators found
		{
			++rate;

			// Concatenation at the network rate
			if (rate == original_rate)
			{
				for(iteration = 0; iteration < range; iteration++)
				{
					state->timeslot_info[iteration + offset].size = original_rate;			// Set lower third
					state->timeslot_info[iteration + offset + factor].size = original_rate;		// Set middle third
					state->timeslot_info[iteration + offset + (factor * 2)].size = original_rate;	// Set upper third
				}
				break;
			}
			else
			{
				if (rate == kOC3)
					range = 3 / 3;
				else if (rate == kOC12)
					range = 12 / 3;

				groups = channel_groups[original_rate][rate];
				//printf("OC48: DEBUG-3: rate: %d, range: %d, total groups: %d, factor: %d\n", rate, range, groups, factor);

				for (count = 0; count < groups; count++)
				{
					if (rate == kOC12)
						process_channel_size_oc12(component, rate, range, count, factor, offset);
				}
				stop = 1;
			}
		}
		else if ((result == 1) && (rate != kOC3)) // Valid pointer indicators found
		{
			// Try next lower rate
			--rate;
			//printf("OC48: DEBUG-4: trying next lower rate: %d\n", rate);
		}
		else if ((result == 1) && (rate == kOC3))
		{
			groups = channel_groups[original_rate][rate];

			for (count = 0; count < groups; count++)
				process_channel_size_oc3(component, rate, range, count, factor, offset);

			stop = 1;
		}
	}
}

static void
process_channel_size_oc12 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset)
{
	uint8_t original_rate = rate;
	uint8_t result;
	uint32_t count;
	uint16_t offset;
	uint32_t iteration;
	uint16_t groups;
	uint8_t stop = 0;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Start testing at next lower rate
	--rate;

	while (!stop)
	{
		offset = (group * range) + parent_offset;
		//printf("OC12: DEBUG-1: rate: %d, range: %d, group: %d, offset: %d\n", rate, range, group, offset);
		result = check_valid_pointers(component, rate, range, offset);
		//printf("OC12: DEBUG-2: pointer check result: %d\n", result);

		if (result == 0) // Concatenation indicators found
		{
			++rate;

			// Concatenation at the network rate
			if (rate == original_rate)
			{
				for(iteration = 0; iteration < range; iteration++)
				{
					state->timeslot_info[iteration + offset].size = original_rate;			// Set lower third
					state->timeslot_info[iteration + offset + factor].size = original_rate;		// Set middle third
					state->timeslot_info[iteration + offset + (factor * 2)].size = original_rate;	// Set upper third
				}
				break;
			}
			else
			{
				//printf("ENTERING OC12 DEBUG - should not be here?!\n");
				if (rate == kOC3)
					range = 3 / 3;

				groups = channel_groups[original_rate][rate];
				//printf("OC12: DEBUG-3: rate: %d, range: %d, total groups: %d, factor: %d\n", rate, range, groups, factor);

				for (count = 0; count < groups; count++)
				{
					if (rate == kOC3)
						process_channel_size_oc3(component, rate, range, count, factor, offset);
				}
				stop = 1;
			}
		}
		else if ((result == 1) && (rate != kOC3)) // Valid pointer indicators found
		{
			// Try next lower rate
			--rate;
			//printf("OC12: DEBUG-4: trying next lower rate: %d\n", rate);
		}
		else if ((result == 1) && (rate == kOC3))
		{
			groups = channel_groups[original_rate][rate];

			for (count = 0; count < groups; count++)
				process_channel_size_oc3(component, rate, range, count, factor, offset);

			stop = 1;
		}
	}
}
#endif

static void
process_channel_size_oc3 (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset)
{
	uint8_t result;
	uint16_t offset = parent_offset;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	//printf("OC3 DEBUG: rate: %d, range: %d, group: %d, offset: %d\n", rate, range, group, offset);
	result = check_valid_pointers_oc3(component, rate, range, group, factor, offset);
	//printf("OC3 DEBUG: pointer check result: %d\n", result);

	if (result == 0) // Concatenation indicators found, i.e. OC3c
	{
		state->timeslot_info[group + offset].size = rate;			// Set lower third
		state->timeslot_info[group + offset + factor].size = rate;		// Set middle third
		state->timeslot_info[group + offset + (factor * 2)].size = rate;	// Set upper third
	}
	else if (result == 1) // Valid pointer indicators found, i.e. channelised OC1
	{
		state->timeslot_info[group + offset].size = rate - 1;			// Set lower third
		state->timeslot_info[group + offset + factor].size = rate - 1;		// Set middle third
		state->timeslot_info[group + offset + (factor * 2)].size = rate - 1;	// Set upper third
	}
}

static void
process_channel_size (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t group, uint16_t factor, uint16_t parent_offset)
{
	static uint8_t instance = 0;	// For debugging, to keep track of the number of function instances
	uint8_t original_rate = rate;
	uint8_t result;
	uint32_t count;
	uint16_t offset;
	uint32_t iteration;
	uint16_t groups;
	uint8_t stop = 0;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Start testing at next lower rate
	--rate;

	while (!stop)
	{
		offset = (group * range) + parent_offset;
		//printf("A: DEBUG: instance: %d, rate: %d, range: %d, group: %d, offset: %d\n", instance, rate, range, group, offset);
		result = check_valid_pointers(component, rate, range, offset);
		//printf("B: DEBUG: instance: %d, pointer check result: %d\n", instance, result);

		if (result == 0) // Concatenation indicators found
		{
			// Revert back to previous test rate, which is the rate where valid pointers were found
			++rate;

			// Concatenation at the network rate
			if (rate == original_rate)
			{
				for(iteration = 0; iteration < range; iteration++)
				{
					state->timeslot_info[iteration + offset].size = original_rate;			// Set lower third
					state->timeslot_info[iteration + offset + factor].size = original_rate;		// Set middle third
					state->timeslot_info[iteration + offset + (factor * 2)].size = original_rate;	// Set upper third
				}
				stop = 1;
			}
			else
			{
				// Set the range and groups for the next test at the lower rate
				if (rate == kOC3)
					range = 3 / 3;
				else if (rate == kOC12)
					range = 12 / 3;
				else if (rate == kOC48)
					range = 48 / 3;
				else if (rate == kOC192)
					range = 192 / 3;
				//else if (rate == kOC768)
				//	range = 768 / 3;

				groups = channel_groups[original_rate][rate];

				//printf("C: DEBUG: instance: %d, rate: %d, range: %d, total groups: %d, factor: %d\n", instance, rate, range, groups, factor);
				++instance;

				for (count = 0; count < groups; count++)
					process_channel_size(component, rate, range, count, factor, offset);

				stop = 1;
			}
		}
		else if ((result == 1) && (rate != kOC3)) // Valid pointer indicators found
		{
			// Try next lower rate
			--rate;
			//printf("D: DEBUG: instance: %d, trying next lower rate: %d\n", instance, rate);
		}
		else if ((result == 1) && (rate == kOC3))
		{
			// Set the groups for the OC3 test
			groups = channel_groups[original_rate][rate];

			for (count = 0; count < groups; count++)
				process_channel_size_oc3(component, rate, range, count, factor, offset);

			stop = 1;
		}
	}
}

static uint8_t
check_valid_pointers_oc3 (ComponentPtr component, uint8_t rate, uint16_t range, uint32_t group, uint16_t factor, uint16_t offset)
{
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

#if 0
	printf("OC3 CHECK: group: %d, offset: %d, indicator[%d]: %c, indicator[%d]: %c, indicator[%d]: %c\n",
		group,
		offset,
		group + offset,
		state->timeslot_info[group + offset].indicator == kPointerValid ? 'P' : 'C',
		group + offset + factor,
		state->timeslot_info[group + offset + factor].indicator == kPointerValid ? 'P' : 'C',
		group + offset + (factor * 2),
		state->timeslot_info[group + offset + (factor * 2)].indicator == kPointerValid ? 'P' : 'C');
#endif

	if (state->timeslot_info[group + offset].indicator == kConcatenation ||			// Check lower third
	    state->timeslot_info[group + offset + factor].indicator == kConcatenation ||	// Check middle third
	    state->timeslot_info[group + offset + (factor * 2)].indicator == kConcatenation)	// Check upper third
		return 0;

	return 1;
}

static uint8_t
check_valid_pointers (ComponentPtr component, uint8_t rate, uint16_t range, uint16_t offset)
{
	uint32_t iteration;
	uint8_t interval = 0;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Valid pointers are located at defined timeslot intervals
	if (rate == kOC3)
		interval = 1;
	else if (rate == kOC12)
		interval = 4;
	else if (rate == kOC48)
		interval = 16;
	else if (rate == kOC192)
		interval = 64;

	// Only need to iterate over the first third of the timeslots to find concatenation
	for (iteration = 0; iteration < range; iteration+=interval)
	{
#if 0
		printf("CHECK: iteration: %d, range: %d, interval: %d, offset: %d, indicator[%d]: %c\n",
			iteration,
			range,
			interval,
			offset,
			iteration + offset,
			state->timeslot_info[iteration + offset].indicator == kPointerValid ? 'P' : 'C');
#endif

		if (state->timeslot_info[iteration + offset].indicator == kConcatenation)
			return 0;
	}

	return 1;
}

static void
write_channel_config_memory (ComponentPtr component)
{
	uint32_t iteration;
	uint32_t write_data = 0;
	uint32_t write_addr = 0;
	uint8_t timeslots_per_write = 0;
	uint8_t shift = 0;
	uint32_t return_value;

	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	if (state->network_rate == kOC48)
		timeslots_per_write = 2;
	else if (state->network_rate == kOC192)
		timeslots_per_write = 8;

	shift = timeslots_per_write - 1;

	// Configure the SONET channel config memory with channel size information. Note that vc-ids calculated previously are not loaded.
	for (iteration = 0; iteration < state->timeslots; iteration++)
	{
		if ((state->network_rate == kOC3) || (state->network_rate == kOC12))
		{
			return_value = execute_channel_config_mem_write(component, iteration, state->timeslot_info[iteration].size);

			if (return_value != state->timeslot_info[iteration].size)
				dagutil_warning("Port %d: Channel Config Memory write verification failed (at OC3/OC12), read: 0x%x, write: 0x%x\n", state->mIndex, return_value, state->timeslot_info[iteration].size);

		}
		else if ((state->network_rate == kOC48) || (state->network_rate == kOC192))
		{
			write_data |= ((state->timeslot_info[iteration].size & 0x7) << (shift * 3));
			--shift;

#if 0
			printf("DEBUG: port: %d, Config Mem write: 0x%08x, size[%d]: %d\n",
				state->mIndex,
				write_data,
				iteration,
				state->timeslot_info[iteration].size);
#endif

			// Perform a write every 2 timeslots for OC48 and every 8 timeslots for OC192
			if (((iteration + 1) % timeslots_per_write == 0) && iteration != 0)
			{
				//printf("DEBUG: write[%d]: write data: 0x%06x\n", write_addr, write_data);

				return_value = execute_channel_config_mem_write(component, write_addr, write_data);

				if (return_value != write_data)
					dagutil_warning("Port %d: Channel Config Memory write verification failed (at OC48/OC192), read: 0x%x, write: 0x%x\n", state->mIndex, return_value, write_data);

				++write_addr;
				write_data = 0;
				shift = timeslots_per_write - 1;
			}
		}
	}
}


static uint32_t
execute_channel_config_mem_write (ComponentPtr component, uint32_t addr, uint32_t data)
{
	AttributePtr chan_config_mem_addr = NULL;
	AttributePtr chan_config_mem_write_en = NULL;
	AttributePtr chan_config_mem_data_write = NULL;
	AttributePtr chan_config_mem_data_read = NULL;
	GenericReadWritePtr chan_config_mem_addr_grw = NULL;
	GenericReadWritePtr chan_config_mem_data_write_grw = NULL;
	GenericReadWritePtr chan_config_mem_data_read_grw = NULL;
	uint32_t register_value = 0;
	uint32_t *masks;
	uint8_t write_en_bool = 1;

	// Create grw objects for the SONET Channel Config Memory module
	chan_config_mem_addr = component_get_attribute(component, kUint32AttributeChannelConfigMemAddr);
	chan_config_mem_addr_grw = attribute_get_generic_read_write_object(chan_config_mem_addr);
	chan_config_mem_write_en = component_get_attribute(component, kBooleanAttributeWriteEnable);	// Don't need grw object for the write enable boolean
	chan_config_mem_data_write = component_get_attribute(component, kUint32AttributeChannelConfigMemDataWrite);
	chan_config_mem_data_write_grw = attribute_get_generic_read_write_object(chan_config_mem_data_write);
	chan_config_mem_data_read = component_get_attribute(component, kUint32AttributeChannelConfigMemDataRead);
	chan_config_mem_data_read_grw = attribute_get_generic_read_write_object(chan_config_mem_data_read);

	// Write data register
	register_value = grw_read(chan_config_mem_data_write_grw);
	masks = attribute_get_masks(chan_config_mem_data_write);
	register_value &= ~(*masks);
	register_value |= data;
	grw_write(chan_config_mem_data_write_grw, register_value);	

	// Write address register
	register_value = grw_read(chan_config_mem_addr_grw);
	masks = attribute_get_masks(chan_config_mem_addr);
	register_value &= ~(*masks);
	register_value |= addr;
	grw_write(chan_config_mem_addr_grw, register_value);

	// Set write enable
	attribute_boolean_set_value(chan_config_mem_write_en, (void*)&write_en_bool, 1);

	// Clear write enable
	write_en_bool = 0;
	attribute_boolean_set_value(chan_config_mem_write_en, (void*)&write_en_bool, 1);

	// Perform a read back for verification
	register_value = grw_read(chan_config_mem_data_read_grw);
	masks = attribute_get_masks(chan_config_mem_data_read);
	register_value &= *masks;

	return register_value;
}

static void
write_loh_mask (ComponentPtr component)
{
	AttributePtr vc_id_attr;
	uint32_t timeslot_id;
	uint32_t iteration;
	uint32_t return_value;
	uint32_t write_data = 0;
	uint32_t write_addr = 0;
	uint8_t shift = 0;

	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	// Retrieve the selected VC-ID attribute and corresponding timeslot-ID value
	// NOTE: The actual value read from I/O memory is the timeslot-ID value, the get/set VC-ID attribute functions do a mapping from VC-ID to timeslot-ID and vice versa.
	vc_id_attr = component_get_attribute(component, kUint32AttributeVCIDSelect);
	timeslot_id = *(uint32_t *)attribute_uint32_get_value(vc_id_attr);

	for (iteration = 0; iteration < state->timeslots; iteration++)
	{
		if ((state->network_rate == kOC3) || (state->network_rate == kOC12))
		{
			// Write a 1 to the mask bit location if the selected VC-ID matches the timeslot-ID value
			if (timeslot_id == state->timeslot_info[iteration].timeslot_id)
				write_data |= 0x1 << iteration;

			if (iteration == state->timeslots - 1)
			{
				return_value = execute_loh_mask_write(component, write_addr, write_data);

				if (return_value != write_data)
					dagutil_warning("Port %d: LOH mask write verification failed (at OC3/OC12), address: %d, write: 0x%08x, read: 0x%08x\n", state->mIndex, write_addr, write_data, return_value);
			}
		}
		else if ((state->network_rate == kOC48) || (state->network_rate == kOC192))
		{
			// Write a 1 to the mask bit location if the selected VC-ID matches the timeslot-ID value
			if (timeslot_id == state->timeslot_info[iteration].timeslot_id )
				write_data |= 0x1 << shift;

#if 0
			printf ("DEBUG: port: %d, LOH mask write: 0x%08x, selected timeslot_id: 0x%x, timeslot_id (%d): %d\n",
				state->mIndex,
				write_data,
				timeslot_id,
				iteration,
				state->timeslot_info[iteration].timeslot_id);
#endif

			++shift;

			// Perform a write every 32 iterations or at the last timeslot
			if ((iteration == (32 * (write_addr + 1)) - 1) || (iteration == state->timeslots - 1))
			{
				return_value = execute_loh_mask_write(component, write_addr, write_data);

				if (return_value != write_data)
					dagutil_warning("Port %d: LOH mask write verification failed (at OC48/OC192), address: %d, write: 0x%08x, read: 0x%08x\n", state->mIndex, write_addr, write_data, return_value);

				++write_addr;
				write_data = 0;
				shift = 0;
			}
		}
	}
}

static uint32_t
execute_loh_mask_write (ComponentPtr component, uint32_t addr, uint32_t data)
{
	AttributePtr loh_mask_addr = NULL;
	AttributePtr loh_mask_write_en = NULL;
	AttributePtr loh_mask_data_write = NULL;
	AttributePtr loh_mask_data_read = NULL;
	GenericReadWritePtr loh_mask_addr_grw = NULL;
	GenericReadWritePtr loh_mask_data_write_grw = NULL;
	GenericReadWritePtr loh_mask_data_read_grw = NULL;
	uint32_t register_value = 0;
	uint32_t *masks;
	uint8_t write_en_bool = 1;

	// Create grw objects for the LOH Mask
	loh_mask_addr = component_get_attribute(component, kUint32AttributeLOHMaskAddr);
	loh_mask_addr_grw = attribute_get_generic_read_write_object(loh_mask_addr);
	loh_mask_write_en = component_get_attribute(component, kBooleanAttributeLOHMaskWriteEnable);	// Don't need grw object for the write enable boolean
	loh_mask_data_write = component_get_attribute(component, kUint32AttributeLOHMaskDataWrite);
	loh_mask_data_write_grw = attribute_get_generic_read_write_object(loh_mask_data_write);
	loh_mask_data_read = component_get_attribute(component, kUint32AttributeLOHMaskDataRead);
	loh_mask_data_read_grw = attribute_get_generic_read_write_object(loh_mask_data_read);

	// Write data register
	register_value = grw_read(loh_mask_data_write_grw);
	masks = attribute_get_masks(loh_mask_data_write);
	register_value &= ~(*masks);
	register_value |= data;
	grw_write(loh_mask_data_write_grw, register_value);	

	// Write address register
	register_value = grw_read(loh_mask_addr_grw);
	masks = attribute_get_masks(loh_mask_addr);
	register_value &= ~(*masks);
	register_value |= addr;
	grw_write(loh_mask_addr_grw, register_value);

	// Set write enable
	attribute_boolean_set_value(loh_mask_write_en, (void*)&write_en_bool, 1);

	// Clear write enable
	write_en_bool = 0;
	attribute_boolean_set_value(loh_mask_write_en, (void*)&write_en_bool, 1);

	// Perform a read back for verification
	register_value = grw_read(loh_mask_data_read_grw);
	masks = attribute_get_masks(loh_mask_data_read);
	register_value &= *masks;

	return register_value;
}


static void
create_connection_subcomponents (ComponentPtr component)
{
	DagCardPtr card = component_get_card(component);
	uint32_t connection_index;
	sonet_channel_mgmt_state_t *state = (sonet_channel_mgmt_state_t *)component_get_private_state(component);

	//printf("total connection subcomponents: %d\n", state->vc_id_total);
	
	for(connection_index = 0; connection_index < state->vc_id_total; connection_index++)
	{
		//printf("creating connection: %d\n", connection_index);

		state->mConnection[connection_index] = sonet_get_new_connection(card, connection_index);
		component_add_subcomponent(component, state->mConnection[connection_index]);
		/* For each subcomponent set the parent correctly */
		component_set_parent(state->mConnection[connection_index], component);
	}
}




