/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
*/

#include "dagapi.h"
#include "../include/attribute.h"
#include "../include/component.h"
#include "../include/create_attribute.h"
#include "../include/util/enum_string_table.h"

#include "../include/components/memtx_component.h"

#define PKT_MEM_SIZE		8*1024	// bytes
#define SEQ_MEM_MAX_ENTRIES	512
#define ERF_HEADER_SIZE		16	// bytes
#define ERF_EXT_HEADER_SIZE	8	// bytes
#define MAX_PKT_SIZE		1024	// bytes

static void memtx_dispose(ComponentPtr component);
static void memtx_reset(ComponentPtr component);
static void memtx_default(ComponentPtr component);
static int memtx_post_initialize(ComponentPtr component);

static void pkt_rate_set_value (AttributePtr attribute, void* value, int length);
static void* pkt_rate_get_value (AttributePtr attribute);

static void tx_command_set_value (AttributePtr attribute, void* value, int length);
static void* tx_command_get_value (AttributePtr attribute);
static void tx_command_to_string_routine (AttributePtr attribute);
static void tx_command_from_string_routine (AttributePtr attribute, const char* string);

static void load_erf_file_set_value (AttributePtr attribute, void* value, int length);
static void* load_erf_file_get_value (AttributePtr attribute);
static void load_erf_file_to_string_routine (AttributePtr attribute);
static void load_erf_file_from_string_routine (AttributePtr attribute, const char* string);

typedef struct memtx_state
{
	uint32_t	mIndex;
} memtx_state_t;

typedef enum
{
	kCommand		= 0x00,
	kMemoryAddress		= 0x04,
	kPacketMemoryData	= 0x08,
	kSequenceMemoryData	= 0x0C,
	kPRCUControlStatus	= 0x10,		// Packet Rate Control Unit (PRCU)
	kPRCURate		= 0x14,
	kPRCURateReport		= 0x18,
	kReserved		= 0x1C		// NOTE: Changed in the future?
} memtx_register_offset_t;

// FIXME: Do we need this???
typedef struct
{
	char		filename[256];
	uint32_t	len;
} memtx_file_t;

typedef struct
{
	uint32_t	addr;
	uint32_t	data[1];
} memtx_packet_t;

typedef struct
{
	uint32_t	seq_mem_addr;
	uint32_t	num_pkts;
	uint32_t	pkt_len;
	uint32_t	pkt_mem_addr;
} memtx_sequence_t;


Attribute_t memtx_attr[]=
{
	{
		/* Name */                 "seq_mem_end_addr",
		/* Attribute Code */       kUint32AttributeSequenceMemoryEndAddress,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Sequence memory end address.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    16,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kCommand,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x01FF0000,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "tx_command",
		/* Attribute Code */       kUint32AttributeTxCommand,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Transmit Command.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kCommand,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT1|BIT0,
		/* Default Value */        0,
		/* SetValue */             tx_command_set_value,
		/* GetValue */             tx_command_get_value,
		/* SetToString */          tx_command_to_string_routine,
		/* SetFromString */        tx_command_from_string_routine,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "seq_mem_addr",
		/* Attribute Code */       kUint32AttributeSequenceMemoryAddress,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Sequence memory address.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    16,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kMemoryAddress,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x01FF0000,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "pkt_mem_addr",
		/* Attribute Code */       kUint32AttributePacketMemoryAddress,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Packet memory address.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kMemoryAddress,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x00000FFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "pkt_mem_data",
		/* Attribute Code */       kUint32AttributePacketMemoryData,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Data to be read from or written to the packet memory.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPacketMemoryData,
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
		/* Name */                 "seq_mem_num_pkts",
		/* Attribute Code */       kUint32AttributeSequenceMemoryNumPackets,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "The number of packets transmitted is one less than this field. If 0, one packet is transmitted for this sequence.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    22,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kSequenceMemoryData,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFC00000,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},
	
	{
		/* Name */                 "seq_mem_pkt_len",
		/* Attribute Code */       kUint32AttributeSequenceMemoryPacketLength,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Packet length - 1. All packets in this sequence are one byte longer than specified in this field.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    12,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kSequenceMemoryData,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x003FF000,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "seq_mem_pkt_mem_addr",
		/* Attribute Code */       kUint32AttributeSequenceMemoryTxPacketAddr,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Starting address in the packet memory of the packet to be transmitted for this sequence.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kSequenceMemoryData,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x000007FF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "prcu_start",
		/* Attribute Code */       kBooleanAttributePRCUStart,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "Start the Packet Rate Control Unit (PRCU).",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    31,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPRCUControlStatus,
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
		/* Name */                 "rate_report_config",
		/* Attribute Code */       kBooleanAttributeRateReportConfig,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "Rate reporting configuration.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    29,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPRCUControlStatus,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT29,
		/* Default Value */        0,
		/* SetValue */             attribute_boolean_set_value,
		/* GetValue */             attribute_boolean_get_value,
		/* SetToString */          attribute_boolean_to_string,
		/* SetFromString */        attribute_boolean_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "rate_report_capability",
		/* Attribute Code */       kBooleanAttributeRateReportCapability,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "Rate reporting capability.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    28,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPRCUControlStatus,
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
	},
	
	{
		/* Name */                 "report_config_interval",
		/* Attribute Code */       kUint32AttributeReportConfigInterval,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Configuration interval reporting.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    20,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPRCUControlStatus,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x03F00000,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "datapath_clk_freq",
		/* Attribute Code */       kUint32AttributeDatapathClockFrequency,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Data-path clock frequency in kHz.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPRCUControlStatus,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x000FFFFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "pkt_rate",
		/* Attribute Code */       kUint32AttributePacketRate,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Shows the calculated packet rate control register value for a provided packet rate (packets/sec).",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPRCURate,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF,
		/* Default Value */        1,
		/* SetValue */             pkt_rate_set_value,
		/* GetValue */             pkt_rate_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string	,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},

	{
		/* Name */                 "pkt_rate_report",
		/* Attribute Code */       kUint32AttributePacketRateReport,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Reports actual packet rate (packets/sec) achieved during transmission.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_MEMTX,
		/* Offset */               kPRCURateReport,
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
		/* Name */                 "load_erf_file",
		/* Attribute Code */       kStringAttributeLoadERFFile,
		/* Attribute Type */       kAttributeString,
		/* Description */          "Load an ERF file to transmit.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,				// Don't care
		/* Register Address */     DAG_REG_MEMTX,		// Don't care
		/* Offset */               0,				// Don't care
		/* Size/length */          1,				// Don't care
		/* Read */                 NULL,			// Don't care
		/* Write */                NULL,			// Don't care
		/* Mask */                 0,				// Don't care
		/* Default Value */        0,
		/* SetValue */             load_erf_file_set_value,
		/* GetValue */             load_erf_file_get_value,
		/* SetToString */          load_erf_file_to_string_routine,	// Don't care
		/* SetFromString */        load_erf_file_from_string_routine,	// Don't care
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	}

};

/* Number of elements in array */
#define NB_ELEM (sizeof(memtx_attr) / sizeof(Attribute_t))

ComponentPtr
memtx_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentMemTx, card);

    if (NULL != result)
    {
        memtx_state_t* state = (memtx_state_t*)malloc(sizeof(memtx_state_t));
        component_set_dispose_routine(result, memtx_dispose);
        component_set_reset_routine(result, memtx_reset);
        component_set_default_routine(result, memtx_default);
        component_set_post_initialize_routine(result, memtx_post_initialize);
        component_set_name(result, "MemTx");
        component_set_description(result, "The Memory Transmit Module.");
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;
}

static void
memtx_dispose(ComponentPtr component)
{
	// Unused
}

static void
memtx_reset(ComponentPtr component)
{
	// Unused
}

static void
memtx_default(ComponentPtr component)
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
			if (attribute_get_attribute_code(attribute) != kStringAttributeLoadERFFile)
			{
				val = attribute_get_default_value(attribute);
				attribute_set_value(attribute, &val, 1);
			}
		}

		// Start the PRCU
		attribute = component_get_attribute(component,kBooleanAttributePRCUStart);
		val = (void *)1;
	        attribute_set_value(attribute, &val, 1);
	}
}

static int
memtx_post_initialize(ComponentPtr component)
{
	if (1 == valid_component(component))
	{
		memtx_state_t* state = NULL;
		state = component_get_private_state(component);

		/* Add attributes of the module */ 
		read_attr_array(component, memtx_attr, NB_ELEM, state->mIndex);

		return 1;
	}
	return kDagErrInvalidParameter;
}


static void
pkt_rate_set_value(AttributePtr attribute, void* value, int length)
{
	ComponentPtr component = NULL;
	AttributePtr clock_freq = NULL;
	GenericReadWritePtr clock_freq_grw = NULL;
	GenericReadWritePtr pkt_rate_grw = NULL;
	uint32_t reg_val = 0;
	uint32_t clock_freq_val = 0;
	uint32_t *masks;

	component = attribute_get_component (attribute);

	if (1 == valid_attribute(attribute))
	{
		// Get data-path clock frequency
		clock_freq = component_get_attribute(component, kUint32AttributeDatapathClockFrequency);
	        clock_freq_grw = attribute_get_generic_read_write_object(clock_freq);
		reg_val = grw_read(clock_freq_grw);
		masks = attribute_get_masks(clock_freq);
		clock_freq_val = reg_val & (*masks);	// in kHz
		//printf ("Clock frequency: %d kHz, 0x%x\n", clock_freq_val, clock_freq_val);
	
		// Calculate the packet rate value to write to the PRCU register
		reg_val = ((clock_freq_val*1000)/ *(int *)value) - 1;
		//printf ("Calculated register value to write: %d, 0x%x\n", reg_val, reg_val);
		pkt_rate_grw = attribute_get_generic_read_write_object(attribute);
		grw_write(pkt_rate_grw, reg_val);
	}
}

static void*
pkt_rate_get_value(AttributePtr attribute)
{
	GenericReadWritePtr pkt_rate_grw = NULL;
	uint32_t reg_val = 0;

	if (1 == valid_attribute(attribute))
	{
		pkt_rate_grw = attribute_get_generic_read_write_object(attribute);
		reg_val = grw_read(pkt_rate_grw);

		attribute_set_value_array(attribute, (void *)&reg_val, sizeof(reg_val));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}


static void
tx_command_set_value(AttributePtr attribute, void* value, int length)
{
	GenericReadWritePtr tx_command_grw = NULL;
	tx_command_t tx_command = *(tx_command_t*)value;
	uint32_t *masks, mask, new_mask = 0;
	uint32_t register_value = 0;

	if (1 == valid_attribute(attribute))
	{
		tx_command_grw = attribute_get_generic_read_write_object(attribute);
		register_value = grw_read(tx_command_grw);
		masks = attribute_get_masks(attribute);
		mask = masks[0];
		register_value &= ~(mask);
		switch(tx_command)
		{
			case kTxCommandInvalid:
				break;
			case kTxCommandStop:
				new_mask = 0;
				break;
			case kTxCommandBurst:
				new_mask = 1;
				break;
			case kTxCommandStart:
				new_mask = 2;
				break;
		}

		while (!(mask & 1))
        	{
			mask >>= 1;
			new_mask <<= 1;
		}
		register_value |= new_mask;
		grw_write(tx_command_grw, register_value);
	}
}


static void*
tx_command_get_value(AttributePtr attribute)
{
	GenericReadWritePtr tx_command_grw = NULL;
	tx_command_t tx_command = kTxCommandInvalid;
	uint32_t register_value = 0;
	uint32_t *masks, mask;

	if (1 == valid_attribute(attribute))
	{
		tx_command_grw = attribute_get_generic_read_write_object(attribute);
		register_value = grw_read(tx_command_grw);
		masks = attribute_get_masks(attribute);
		mask = masks[0];
		register_value &= mask;

		while (!(mask & 1))
		{
			mask >>= 1;
			register_value >>= 1;
		}

		switch(register_value)
		{
			case 0x00:
				tx_command = kTxCommandStop;
				break;
			case 0x01:
				tx_command = kTxCommandBurst;
				break;
			case 0x02:
				tx_command = kTxCommandStart;
				break;
		}
	}
	attribute_set_value_array(attribute, &tx_command, sizeof(tx_command));
	return (void *)attribute_get_value_array(attribute);
}

static void
tx_command_to_string_routine (AttributePtr attribute)
{
	void* temp = attribute_get_value(attribute);
	const char* string = NULL;
	tx_command_t tx_command;
	if (temp)
	{
		tx_command = *(tx_command_t*)temp;
		string = tx_command_to_string(tx_command);
		if (string)
			attribute_set_to_string(attribute, string);
	}
}

static void
tx_command_from_string_routine (AttributePtr attribute, const char* string)
{
	if (1 == valid_attribute(attribute))
	{
		if (string)
		{
			tx_command_t mode = string_to_tx_command(string);
			tx_command_set_value(attribute, (void*)&mode, sizeof(mode));
		}
	}
}


static void
load_erf_file_set_value(AttributePtr attribute, void* value, int length)
{
	ComponentPtr component = NULL;
	FILE *file = NULL;
	uint32_t file_size = 0;
	AttributePtr pkt_mem_addr = NULL;
	AttributePtr pkt_mem_data = NULL;
	AttributePtr seq_mem_addr = NULL;
	AttributePtr seq_mem_num_pkts = NULL;
	AttributePtr seq_mem_pkt_len = NULL;
	AttributePtr seq_mem_pkt_mem_addr = NULL;
	GenericReadWritePtr pkt_mem_addr_grw = NULL;
	GenericReadWritePtr pkt_mem_data_grw = NULL;
	GenericReadWritePtr seq_mem_addr_grw = NULL;
	GenericReadWritePtr seq_mem_num_pkts_grw = NULL;
	GenericReadWritePtr seq_mem_pkt_len_grw = NULL;
	GenericReadWritePtr seq_mem_pkt_mem_addr_grw = NULL;
	uint8_t header[ERF_HEADER_SIZE];
	uint8_t ext_header[ERF_EXT_HEADER_SIZE];
	uint8_t packet[PKT_MEM_SIZE];		// The largest packet that can fit into packet memory is 8kB, i.e. the size of the memory itself
	uint8_t *packet_ptr = packet;
	uint32_t *packet_ptr_32;
	uint32_t i;
	uint32_t iteration_32 = 0;
	uint32_t header_bytes = 0;
	uint32_t ext_header_bytes = 0;
	uint32_t packet_bytes = 0;
	uint32_t total_bytes = 0;
	uint32_t wlen = 0;
	uint32_t rlen = 0;
	uint32_t padding = 0;
	uint32_t actual_len = 0;
	uint32_t snapped_len = 0;
	memtx_packet_t packet_entry;
	memtx_sequence_t sequence_entry;
	uint32_t pkt_mem_data_value = 0;
	uint32_t pkt_mem_current_addr = 0;
	uint32_t pkt_mem_current_addr_64 = 0;
	uint32_t seq_mem_current_addr = 0;
	uint32_t register_value = 0;
	uint32_t *masks;
	uint32_t extension = 0;
	uint32_t counter = 1;
	uint32_t snapped = 0;

	component = attribute_get_component (attribute);

	if (1 == valid_attribute(attribute))
	{
		// Store the filename so when retrieved later it won't be null
		attribute_set_value_array(attribute, value, strlen((char *)value));

		printf ("File name: %s\n", (char *)value);

		// Open file and check size, if bigger than 8kB, give a warning
		if ((file = fopen (value, "rb")) == NULL)
		{
			fprintf (stderr, "Unable to open file: \"%s\"\n", (char *)value);
			exit(EXIT_FAILURE);
		}
		else
		{
			if (fseek(file, 0, SEEK_END) == -1)
				fprintf (stderr, "Unable to retrieve EOF: \"%s\"\n", (char *)value);
			else
				if ((file_size = ftell (file)) == -1)
				{
					fprintf (stderr, "Unable to get file size: \"%s\"\n", (char *)value);
					exit(EXIT_FAILURE);
				}
		}
		
		if (file_size > PKT_MEM_SIZE)
			fprintf (stderr, "ERF file is bigger than %d kB packet memory size: %d\n", PKT_MEM_SIZE/1024, file_size);

		printf ("File size: %d bytes\n", file_size);

		// Reset file access position
		rewind (file);
		
		// Create grw objects to write to the package memory
		pkt_mem_addr = component_get_attribute(component, kUint32AttributePacketMemoryAddress);
		pkt_mem_addr_grw = attribute_get_generic_read_write_object(pkt_mem_addr);
		pkt_mem_data = component_get_attribute(component, kUint32AttributePacketMemoryData);
	        pkt_mem_data_grw = attribute_get_generic_read_write_object(pkt_mem_data);

		// Create grw objects to write to the sequence memory
		seq_mem_addr = component_get_attribute(component, kUint32AttributeSequenceMemoryAddress);
	        seq_mem_addr_grw = attribute_get_generic_read_write_object(seq_mem_addr);
		seq_mem_num_pkts = component_get_attribute(component, kUint32AttributeSequenceMemoryNumPackets);
	        seq_mem_num_pkts_grw = attribute_get_generic_read_write_object(seq_mem_num_pkts);
		seq_mem_pkt_len = component_get_attribute(component, kUint32AttributeSequenceMemoryPacketLength);
	        seq_mem_pkt_len_grw = attribute_get_generic_read_write_object(seq_mem_pkt_len);
		seq_mem_pkt_mem_addr = component_get_attribute(component, kUint32AttributeSequenceMemoryTxPacketAddr);
	        seq_mem_pkt_mem_addr_grw = attribute_get_generic_read_write_object(seq_mem_pkt_mem_addr);

		printf ("Initiating ERF file loading...\n");

		while (total_bytes <= PKT_MEM_SIZE || feof(file) == 0)
		{
			// Retrieve the ERF header
			if ((header_bytes = fread(header, 1, ERF_HEADER_SIZE, file)) != ERF_HEADER_SIZE)
			{
				fprintf (stderr, "Unable to retrieve ERF header: %d bytes, ERF header: %d bytes\n", header_bytes, ERF_HEADER_SIZE);
				break;
			}

			wlen = ntohs(((dag_record_t *)header)->wlen);
			rlen = ntohs(((dag_record_t *)header)->rlen);

			// Check for the presence of extension headers
			if ((((dag_record_t *)header)->type & 0x80) >> 7)
			{
				extension = 1;
				if ((ext_header_bytes = fread(ext_header, 1, ERF_EXT_HEADER_SIZE, file)) != ERF_EXT_HEADER_SIZE)
				{
					fprintf (stderr, "Unable to retrieve ERF extension header: %d bytes, ERF extension header: %d bytes\n", ext_header_bytes, ERF_EXT_HEADER_SIZE);
					break;
				}
			}

			// Determine the length of padding
			if (rlen > wlen)
			{
				if (extension)
					padding = rlen - wlen - ERF_HEADER_SIZE - ERF_EXT_HEADER_SIZE;
				else
					padding = rlen - wlen - ERF_HEADER_SIZE;
			}
			//else
				// TODO: Handle cases where rlen is smaller than wlen (i.e. packet was snapped)	

			// Determine actual packet length, note that CRC is included in length
			actual_len = wlen;

			// Check that packet length is less than the maximum allowable (i.e. 10-bits of a sequence entry). If not, snap the packet
			if (actual_len > MAX_PKT_SIZE)
			{
				fprintf (stderr, "Packet length is longer than maximum allowable length: %d bytes (max %d bytes)\n", actual_len, MAX_PKT_SIZE);
				snapped_len = actual_len - MAX_PKT_SIZE;
				actual_len = MAX_PKT_SIZE;
				snapped = 1;
			}

			printf ("%d: Packet info (bytes), wlen: %d, rlen: %d, actual: %d\n", counter, wlen, rlen, actual_len);
			printf ("%d: Packet memory info (bytes), memory size: %d, total used: %d, remaining: %d\n", counter, PKT_MEM_SIZE, total_bytes, PKT_MEM_SIZE - total_bytes);

			// Ensure there's still enough space in the packet memory for another packet
			total_bytes += actual_len;
			if (total_bytes > PKT_MEM_SIZE)
			{
				fprintf (stderr, "Packet memory data limit reached. Insufficient space for next packet\n");
				break;
			}
			
			// Retrieve the packet
			if ((packet_bytes = fread(packet, 1, actual_len, file)) != actual_len)
			{
				fprintf (stderr, "Unable to retrieve full packet, packet bytes: %d, actual_len: %d\n", packet_bytes, actual_len);
				break;
			}

			printf ("%d: Packet memory address (32-bit aligned): %d\n", counter, pkt_mem_current_addr);
			printf ("%d: Sequence memory address: %d\n", counter, seq_mem_current_addr);
			
			// Store the sequence memory data at a sequence address
			sequence_entry.seq_mem_addr = seq_mem_current_addr;
			//printf("sequence_entry.seq_mem_addr: 0x%x\n", sequence_entry.seq_mem_addr);
			register_value = grw_read(seq_mem_addr_grw);
			masks = attribute_get_masks(seq_mem_addr);
			register_value &= ~(*masks);
			register_value |= (sequence_entry.seq_mem_addr << 16);
			//printf("register_value: 0x%x\n", register_value);
			grw_write(seq_mem_addr_grw, register_value);
			//printf("register_value read: 0x%x\n", grw_read(seq_mem_addr_grw));	

			sequence_entry.num_pkts = 0;		// default
			//printf("sequence_entry.num_pkts: 0x%x\n", sequence_entry.num_pkts);
			register_value = grw_read(seq_mem_num_pkts_grw);
			masks = attribute_get_masks(seq_mem_num_pkts);
			register_value &= ~(*masks);
			register_value |= (sequence_entry.num_pkts << 22);
			//printf("register_value num_pkts: 0x%x\n", register_value);
			grw_write(seq_mem_num_pkts_grw, register_value);
			//printf("register_value num_pkts read: 0x%x\n", grw_read(seq_mem_num_pkts_grw));	

			sequence_entry.pkt_len = actual_len - 1;
			//printf("sequence_entry.pkt_len: 0x%x\n", sequence_entry.pkt_len);
			register_value = grw_read(seq_mem_pkt_len_grw);
			masks = attribute_get_masks(seq_mem_pkt_len);
			register_value &= ~(*masks);
			register_value |= (sequence_entry.pkt_len << 12);
			//printf("register_value pkt_len: 0x%x\n", register_value);
			grw_write(seq_mem_pkt_len_grw, register_value);
			//printf("register_value pkt_len read: 0x%x\n", grw_read(seq_mem_pkt_len_grw));

#if 1
			// The following logic translates 32-bit word aligned packet memory addresses to 64-bit word aligned packet memory addresses used in the sequence memory
			if ((pkt_mem_current_addr % 2))
			{	// Odd 32-bit word aligned addresses
				pkt_mem_current_addr_64 = (pkt_mem_current_addr / 2) + 1;
				//printf ("ODD: pkt_mem_current_addr_64: %d\n", pkt_mem_current_addr_64);
			}	
			else
			{	// Even 32-bit word aligned addresses
				pkt_mem_current_addr_64 = (pkt_mem_current_addr + 1) / 2;
				//printf ("EVEN: pkt_mem_current_addr_64: %d\n", pkt_mem_current_addr_64);
			}
#endif
			sequence_entry.pkt_mem_addr = pkt_mem_current_addr_64;
			//printf("sequence_entry.pkt_mem_addr: 0x%x\n", sequence_entry.pkt_mem_addr);
			register_value = grw_read(seq_mem_pkt_mem_addr_grw);
			masks = attribute_get_masks(seq_mem_pkt_mem_addr);
			register_value &= ~(*masks);
			register_value |= sequence_entry.pkt_mem_addr;
			//printf("register_value pkt_mem_addr: 0x%x\n", register_value);
			grw_write(seq_mem_pkt_mem_addr_grw, register_value);
			//printf("register_value pkt_mem_addr read: 0x%x\n", grw_read(seq_mem_pkt_mem_addr_grw));

			// Update sequence memory address
			++seq_mem_current_addr;

			// Store the packet memory address/data combination
			packet_ptr_32 = (uint32_t *)packet_ptr;	// Translate the packet data pointer to point to 32-bit chunks
			iteration_32 = (actual_len+3)/4; // Iterate over 32-bits by dividing 4 as actual_len is in bytes, add 3 to round up to the next 32-bit word
			if (iteration_32 % 2) // If there an odd number of 32-bit words, we add another iteration to allow for 64-bit alignment
				++iteration_32;
			for (i = 1; i <= iteration_32; i++)	
			{
				// Store packet memory address
				packet_entry.addr = pkt_mem_current_addr;
				//printf("packet_entry.addr: 0x%x\n", packet_entry.addr);
				register_value = grw_read(pkt_mem_addr_grw);
				masks = attribute_get_masks(pkt_mem_addr);
				register_value &= ~(*masks);
				register_value |= packet_entry.addr;
				//printf("register_value: 0x%x\n", register_value);
				grw_write(pkt_mem_addr_grw, register_value);
				//printf("register_value read: 0x%x\n", grw_read(pkt_mem_addr_grw));

				// Store packet memory data
				*packet_entry.data = *packet_ptr_32;
				register_value = grw_read(pkt_mem_data_grw);
				masks = attribute_get_masks(pkt_mem_data);
				register_value &= ~(*masks);
				register_value |= (*packet_entry.data);
				
				// Check byte order (endianness)
				register_value = htonl(register_value);

				grw_write(pkt_mem_data_grw, register_value);

				// Read back to check that data was correctly written
				pkt_mem_data_value = grw_read(pkt_mem_data_grw);
				if (pkt_mem_data_value != register_value)
					printf ("Packet data MISMATCH #%d, entered: 0x%8x, retrieved: 0x%8x\n", pkt_mem_current_addr, register_value, pkt_mem_data_value);
#if 0	// Retain for debugging
				else 
					printf ("Packet data matches #%d, entered: 0x%8x, retrieved: 0x%8x\n", pkt_mem_current_addr, *packet_entry.data, pkt_mem_data_value);
#endif
				// Update packet memory address and packet memory data pointer
				++pkt_mem_current_addr;
				++packet_ptr_32;
			}

			// Update the file access position to the start of the next packet if snapped - unused at present
			if (snapped)
				if (fseek(file, snapped_len, SEEK_CUR) == -1)
				{
					printf ("Unable to move file access position to end of packet after snapping.\n");
					break;
				}

			// Update the file access position to ignore padding
			if (fseek(file, padding, SEEK_CUR) == -1)
			{
				printf ("Unable to move file access position past padding.\n");
				break;
			}

			// Check present location in the file
			if (ftell(file) >= file_size)
				break;
			
			// Ensure we don't exceed the maximum number of allowable sequences
			if (seq_mem_current_addr > SEQ_MEM_MAX_ENTRIES)
			{
				fprintf (stderr, "Maximum sequence memory entry limit reached.\n");
				break;
			}

			// Clear the packet buffer after previously storing its contents
			for (i = 0; i < (sizeof(packet)/sizeof(uint8_t)); i++)
				packet[i] = 0;

			// Reset extension header and snapped packet indicators and increment packet counter
			extension = 0;
			++counter;
			snapped = 0;

			printf ("Next packet...\n");

		}
		
		printf ("SUMMARY: Packet memory info (bytes), memory size: %d, total used: %d, remaining: %d\n", PKT_MEM_SIZE, total_bytes, PKT_MEM_SIZE - total_bytes);

		fclose (file);
	}
}

static void*
load_erf_file_get_value(AttributePtr attribute)
{
	if (1 == valid_attribute(attribute))
	{
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

static void
load_erf_file_to_string_routine (AttributePtr attribute)
{
	if (1 == valid_attribute(attribute))
	{
		char *file_name = NULL;
		
		file_name = (char*) attribute_get_value(attribute);
		if ( NULL != file_name)
			attribute_set_to_string(attribute, file_name);
	}
}

static void
load_erf_file_from_string_routine (AttributePtr attribute, const char* string)
{
	if (1 == valid_attribute(attribute))
	{
		int length = strlen (string);
		load_erf_file_set_value(attribute, (void*) string, length + 1);
	}
}

