	#include "dagutil.h"
	#include "../libdagconf/include/attribute.h"
	#include "../libdagconf/include/util/set.h"
	#include "../libdagconf/include/card.h"
	#include "../libdagconf/include/cards/card_initialization.h"
	#include "../libdagconf/include/component.h"
	#include "../libdagconf/include/util/utility.h"
	#include "../libdagconf/include/cards/common_dagx_constants.h"
	#include "../libdagconf/include/util/enum_string_table.h"
	#include "../libdagconf/include/components/counter_component.h"
	#include "../libdagconf/include/components/counters_interface_component.h"
	#include "../libdagconf/include/cards/dag71s_impl.h"
	#include "../libdagconf/include/attribute_factory.h"
	#include "../libdagconf/include/create_attribute.h"
	#include "../libdagconf/include/components/cat_component.h"
	#include "../../include/dag_attribute_codes.h"
	#include "../libdagconf/include/attribute_types.h"
	#include "./cat_interface.h"
	#include "dag_component.h"
	#include "dag_config.h"
	
	#define CAT_IFACE_BITS_COUNT 2
	
	static uint32_t cat_get_color_mask(uint32_t max_color);
	
	static uint32_t config_mode = 0;
	
	dag_card_ref_t card_ref;
	
	int32_t 
	cat_set_raw_entry(dag_component_t  component,uint16_t address,int32_t destination)
	{
		bool set;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		bool value;
		/*Verify the address and destination*/
		uint32_t OutputBits = cat_get_no_of_output_bits(component);
		uint32_t InputBits  = cat_get_no_of_input_bits(component);
		if(!(address < (1 << InputBits)))
		{
			return kInvalidAddress;	
		}
		if(!(destination < (1 << OutputBits)))
		{
			return kInvalidDestination;
		}
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeBankSelect);
		if(attribute == kNullAttributeUuid)
		{
				printf("Failed to get Bank Select Attribute\n");
		}
		value = dag_config_get_boolean_attribute(card_ref,attribute);
		if(value == 1)
		{
				/*Bank 1 is active have to use Bank 0*/
				address = address|!(BIT14);
		}
		else
		{
				/*Bank 0 is active have to use Bank 1*/
				address = address|BIT14;
		}
		attribute = dag_component_get_attribute_uuid(component,kUint32AttributeAddressRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Address Register attribute \n");
		}
		dag_config_set_uint32_attribute(card_ref,attribute,address);
		attribute = dag_component_get_attribute_uuid(component,kInt32AttributeDataRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Data Register Attribute \n");
		}
		dag_config_set_int32_attribute(card_ref,attribute,destination);
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeWriteEnable);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Write Enable PIN Attribute\n");
		}
		set = 1;
		dag_config_set_boolean_attribute(card_ref,attribute,set);
		return kErrorNone;
	}
	int32_t 
	cat_set_raw_entry_verify(dag_component_t component,uint16_t address,int32_t destination)
	{
			bool set;
			int32_t verify_value;
			dag_card_ref_t card_ref = dag_component_get_card_ref(component);
			attr_uuid_t attribute;
			bool value;
			/*Verify the address and destination*/
			unsigned int OutputBits = cat_get_no_of_output_bits(component);
			unsigned int InputBits  = cat_get_no_of_input_bits(component);
			
			if(!(address < (1 << InputBits)))
			{
				dagutil_verbose("Invalid Address: address %x\n",address);
				return kInvalidAddress;
			}
			if(!(destination < (1 << OutputBits)))
			{
				dagutil_verbose("Invalid Address: destination %x\n",destination);
				return kInvalidDestination;
			}
			attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeBankSelect);
			if(attribute == kNullAttributeUuid)
			{
					printf("Failed to get Bank Select Attribute\n");
			}
			value = dag_config_get_boolean_attribute(card_ref,attribute);
			if(value == 1)
			{
				   /*Bank 1 is active have to use Bank 0*/
				   address = address|!(BIT14);
			}
			else
			{
					/*Bank 0 is active have to use Bank 1*/
					address = address|BIT14;
			}
			attribute = dag_component_get_attribute_uuid(component,kUint32AttributeAddressRegister);
			if(attribute == kNullAttributeUuid)
			{
					printf("Failed to get Address Register attribute \n");
			}
			dag_config_set_uint32_attribute(card_ref,attribute,address);
			attribute = dag_component_get_attribute_uuid(component,kInt32AttributeDataRegister);
			if(attribute == kNullAttributeUuid)
			{
					printf("Failed to get Data Register Attribute \n");
			}
			dag_config_set_int32_attribute(card_ref,attribute,destination);
			attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeWriteEnable);
			if(attribute == kNullAttributeUuid)
			{
					printf("Failed to get Write Enable PIN Attribute\n");
			}
			set = 1;
			dag_config_set_boolean_attribute(card_ref,attribute,set);
			/*Add code to read out the value to verify*/
			attribute = dag_component_get_attribute_uuid(component,kUint32AttributeAddressRegister);
			if(attribute == kNullAttributeUuid)
			{
					printf("Failed to get Address Register Attribute \n");
			}
			dag_config_set_uint32_attribute(card_ref,attribute,address);
			dagutil_microsleep(100);
			attribute = dag_component_get_attribute_uuid(component,kInt32AttributeDataRegister);
			if(attribute == kNullAttributeUuid)
			{
					printf("Failed to get Data Register Attribute \n");
			}
			verify_value = dag_config_get_int32_attribute(card_ref,attribute);
			return verify_value;
	}
	uint64_t 
	cat_new_set_raw_entry_verify(dag_component_t component,uint16_t address,uint64_t destination)
	{
		bool set;
		uint64_t verify_value;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		bool value;
		/*Verify the address and destination*/
		unsigned int OutputBits = cat_get_no_of_output_bits(component);
		unsigned int InputBits  = cat_get_no_of_input_bits(component);
		uint64_t max_destination = 1;
		max_destination = ((max_destination << OutputBits));
		if(!(address < (1 << InputBits)))
		{
			dagutil_verbose("Invalid Address: address %x\n",address);
			return kInvalidAddress;
		}
		if(!(destination < max_destination))
		{
			dagutil_verbose("Invalid Address: destination %"PRIu64"\n",destination);
			return kInvalidDestination;
		}
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeBankSelect);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Bank Select Attribute\n");
		}
		value = dag_config_get_boolean_attribute(card_ref,attribute);
		if(value == 1)
		{
			/*Bank 1 is active have to use Bank 0*/
			address = address|!(1 << InputBits);
		}
		else
		{
			/*Bank 0 is active have to use Bank 1*/
			address = address|(1 << InputBits);
		}
		attribute = dag_component_get_attribute_uuid(component,kUint32AttributeAddressRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Address Register attribute \n");
		}
		dag_config_set_uint32_attribute(card_ref,attribute,address);
		attribute = dag_component_get_attribute_uuid(component,kInt32AttributeDataRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Data Register Attribute \n");
		}
		dag_config_set_int32_attribute(card_ref,attribute,destination);
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeWriteEnable);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Write Enable PIN Attribute\n");
		}
		set = 1;
		dag_config_set_boolean_attribute(card_ref,attribute,set);
		/*Add code to read out the value to verify*/
		attribute = dag_component_get_attribute_uuid(component,kUint32AttributeAddressRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Address Register Attribute \n");
		}
		dag_config_set_uint32_attribute(card_ref,attribute,address);
		dagutil_microsleep(100);
		attribute = dag_component_get_attribute_uuid(component,kInt32AttributeDataRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Data Register Attribute \n");
		}
		verify_value = (uint64_t)(uint32_t)dag_config_get_int32_attribute(card_ref,attribute);
		return verify_value;
	}	
	int32_t 
	cat_get_raw_entry(dag_component_t component,uint16_t bank,uint16_t address)
	{
		uint32_t InputBits;
		int32_t value;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		attribute = dag_component_get_attribute_uuid(component,kUint32AttributeAddressRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Address Register Attribute \n");
		}
		/*Verify the address and destination*/
		InputBits  = cat_get_no_of_input_bits(component);
		if(!(address < (1 << InputBits)))
		{
			return kInvalidAddress;
		}
		if(bank == 0)
		{	
			/*do nothing*/
		}
		else
		{
			address = address + BANK_SIZE;
		}
		dag_config_set_uint32_attribute(card_ref,attribute,address);
		dagutil_microsleep(100);
		attribute = dag_component_get_attribute_uuid(component,kInt32AttributeDataRegister);
		if(attribute == kNullAttributeUuid)
		{
			printf("Failed to get Data Register Attribute \n");
		}
		value = dag_config_get_int32_attribute(card_ref,attribute);
		return value;
	}
	int8_t 
	cat_get_current_bank(dag_component_t component)
	{
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		bool value;
		/*Get bank select component*/
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeBankSelect);
		if(attribute == kNullAttributeUuid)
		{
				printf("Failed to get Bank Select Attribute\n");
		}
		value = dag_config_get_boolean_attribute(card_ref,attribute);
		return value;
	}
	void 
	cat_swap_banks(dag_component_t component)
	{
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute ;
		bool value;
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeBankSelect);
		if(attribute == kNullAttributeUuid) 
		{
			printf("Failed to get Bank Select Attribute\n");
		}
		value = dag_config_get_boolean_attribute(card_ref,attribute);
		if(value == 1)
		{
				/*Bank 1 is active have to select Bank 0*/
			value = 0;
			dag_config_set_boolean_attribute(card_ref,attribute,value);
		}
		else
		{
			/*Bank 0 is active have to select Bank 1*/
			value = 1;
			dag_config_set_boolean_attribute(card_ref,attribute,value);
		}
	}
	int32_t 
	cat_set_entry(dag_component_t component,uint8_t iface,uint16_t color,uint8_t hash,int32_t destination)
	{
		bool interface_overwrite;
		int32_t ret_val;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		uint32_t address = 0;
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeEnableInterfaceOverwrite);
					
		interface_overwrite = dag_config_get_boolean_attribute(card_ref,attribute);
		if (config_mode == ETH_HDLC_POS)
		{
			if(1 == interface_overwrite)
			{
				address  = (iface & 0x3) << 12;
			}
			else /*interface -off*/
			{
				/*wirte to the base address.of either bank 1 or bank 0*/
			}
		}
		else if(config_mode == COLOR_HDLC_POS_COLOR_ETH)
		{
			if(1 == interface_overwrite)
			{
				address = (iface & 0x03) << 12 | (color & 0x3FF) << 2 | (hash&0x03); /*steering bits*/
			}
			else/*interface - off*/
			{
				address = (color & 0XFFF) << 2 | (hash&0x03); /*steering bits*/
			}
		}
		else if (config_mode == COLOR_HASH_POS_COLOR_HASH_ETH)
		{
			if (1 == interface_overwrite)
			{
				address = (iface & 0x03) << 12  | (color&0xFF) << 4 | (hash&0xF);
			}
			else /*interface - off*/
			{
				address = (color & 0x03FF) << 4 | (hash & 0x0F);
			}
		}
		else if(config_mode == COLOR_HASH_POS_COLOR_HASH_ETH)
		{
			if(1 == interface_overwrite)
			{
				address = (iface & 0x03) << 12 | ((color >> 2) & 0xFF) << 4| (hash&0xF);
			}
			else
			{
				address = ((color >> 2) & 0x03FF) << 4 | (hash & 0x0F);
			}
		}
		ret_val = cat_set_raw_entry(component,address,destination);
		return ret_val;
	}
	int32_t 
	cat_set_entry_verify(dag_component_t component,uint8_t iface,uint16_t color,uint8_t hash,int32_t destination)
	{
		bool interface_overwrite;
		int32_t verify_destination;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		uint32_t address = 0;
		/*Getting Interface -  Enable OverWrite Attribute*/
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeEnableInterfaceOverwrite);

		interface_overwrite = dag_config_get_boolean_attribute(card_ref,attribute);
		if (config_mode == ETH_HDLC_POS)
		{
				if(1 == interface_overwrite)
				{
						address  = (iface & 0x3) << 12;
				}
				else /*interface -off*/
				{
						/*do nothing*/
				}
		}
		else if(config_mode == COLOR_HDLC_POS_COLOR_ETH)
		{
				if(1 == interface_overwrite)
				{
						address = (iface & 0x03) << 12 | (color & 0x3FF) << 2 | (hash & 0x03);
				}
				else/*interface - off*/
				{
						address = (color & 0XFFF) << 2 | (hash & 0x03);
				}
		}
		else if (config_mode == COLOR_HASH_POS_COLOR_HASH_ETH)
		{
				if (1 == interface_overwrite)
				{
						address = (iface & 0x03) << 12  | (color&0xFF) << 4 | (hash&0xF);
				}
				else /*interface - off*/
				{
						address = (color & 0x03FF) << 4 | (hash & 0x0F);
				}
		}
		else if(config_mode == COLOR_HASH_POS_COLOR_HASH_ETH_SHIFTED)
		{
			if(1 == interface_overwrite)
			{
				address = (iface & 0x03) << 12 | ((color >> 2)&0xFF) << 4 | (hash&0xF);
			}
			else
			{
				address = ((color >> 2)&0x3FF) << 4 | (hash&0xF);
			}
		}
		verify_destination = cat_set_raw_entry_verify(component,address,destination);
		return verify_destination;
	}
	uint64_t
	cat_new_set_entry_verify(dag_component_t component,uint8_t iface,uint16_t color,uint8_t hash,uint64_t destination)
	{
		bool interface_overwrite;
		uint64_t verify_destination;
		uint32_t bitmax = 0;
		uint32_t max_color, color_mask = 0;
		uint32_t hash_width, hash_mask = 0;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		uint32_t address = 0;
		bitmax = cat_get_no_of_input_bits(component);
	
		max_color = cat_get_max_color(component);
		color_mask = cat_get_color_mask(max_color);
		
		/*Are just stub calls .needs to be implemented based on the firmware.*/
		
		hash_width = get_configured_hash_width(component);
		hash_mask = cat_get_hash_mask(hash_width);
		
		dagutil_verbose_level(2,"color bits : %d color mask %x\n",max_color,color_mask);
		/*Getting Interface -  Enable OverWrite Attribute*/
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeEnableInterfaceOverwrite);
		interface_overwrite = dag_config_get_boolean_attribute(card_ref,attribute);
		
		dagutil_verbose_level(2,"Config Mode : %d \n",config_mode);
		
		if (config_mode == ETH_HDLC_POS)
		{
			if(1 == interface_overwrite)
			{
				address  = (iface & 0x3) << (bitmax -2);
			}
			else /*interface -off*/
			{
					/*do nothing*/
			}
		}
		else if(config_mode == COLOR_HDLC_POS_COLOR_ETH)
		{
			if(1 == interface_overwrite)
			{
				address = (iface & 0x03) << (bitmax -2) | (color & color_mask) << (hash_width) | (hash & hash_mask);
			}
			else/*interface - off*/
			{
				address = (color & color_mask) << (hash_width) | (hash & hash_mask);
			}
		}
		else if (config_mode == COLOR_HASH_POS_COLOR_HASH_ETH)
		{
			if (1 == interface_overwrite)
			{
				address = (iface & 0x03) << (bitmax - 2)  | (color & color_mask) << (hash_width) | (hash & hash_mask);
			}
			else /*interface - off*/
			{
				address = (color & color_mask) << (hash_width) | (hash & hash_mask);
			}
		}
		else if(config_mode == COLOR_HASH_POS_COLOR_HASH_ETH_SHIFTED)
		{
			if(1 == interface_overwrite)
			{
				address = (iface & 0x03) << (bitmax -2) | ((color >> 2) & color_mask) << (hash_width) | (hash & hash_mask);
			}
			else
			{
				address = ((color >> 2) & color_mask) << (hash_width) | (hash & hash_mask);
			}
		}
		verify_destination = cat_new_set_raw_entry_verify(component,address,destination);
		return verify_destination;
	}
	
	int32_t 
	cat_get_entry(dag_component_t component,uint8_t iface,uint16_t color,uint8_t hash,uint8_t bank)
	{
		bool interface_overwrite;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		int32_t destination = 0;
		uint32_t address = 0;
		/*Getting Interface -  Enable OverWrite Attribute*/
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeEnableInterfaceOverwrite);
		interface_overwrite = dag_config_get_boolean_attribute(card_ref,attribute);
		if (config_mode == ETH_HDLC_POS)
		{
				if(1 == interface_overwrite)
				{
						address  = (iface & 0x3) << 12;
				}
				else /*interface -off*/
				{
						/*do nothing*/
				}
		}
		else if(config_mode == COLOR_HDLC_POS_COLOR_ETH)
		{
				if(1 == interface_overwrite)
				{
						address = (iface & 0x03) << 12 | (color & 0x3FF) << 2 | (hash & 0x3);
				}
				else/*interface - off*/
				{
						address = (color & 0XFFF) << 2 | (hash & 0x3);
				}
		}
		else if (config_mode == COLOR_HASH_POS_COLOR_HASH_ETH)
		{
				if (1 == interface_overwrite)
				{
						address = (iface & 0x03) << 12  | (color&0xFF) << 4 | (hash&0xF);
				}
				else /*interface - off*/
				{
						address = (color & 0x03FF) << 4 | (hash & 0x0F);
				}
		}
		else if(config_mode == COLOR_HASH_POS_COLOR_HASH_ETH_SHIFTED)
		{
			if(1 == interface_overwrite)
			{
				address = (iface & 0x03) << 12 | ((color >> 2)&0xFF) << 4 | (hash&0xF);
			}
			else
			{
				address = ((color >> 2)&0x3FF) << 4 | (hash&0xF);
			}
		}
		destination = cat_get_raw_entry((dag_component_t)component,bank,address);
		return destination;	
	}
	void 
	cat_bypass(dag_component_t component)
	{
		bool value;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		/*Getting Interface -  ByPass Attribute*/
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeByPass);
		value = true;
		dag_config_set_boolean_attribute(card_ref,attribute,value);
	}
	void cat_set_entry_mode(int mode)
	{
		config_mode = mode;	
	}
	uint32_t cat_get_entry_mode()
	{
		return config_mode;
	}
	
	uint32_t cat_get_hash_mask(uint32_t hash_width)
	{
		uint32_t mask = 0;
		switch(hash_width)
		{
			case 0:
				mask = 0x0;
				break;
			case 1: 
				mask = 0x1;
				break;
			case 2:
				mask = 0x3;
				break;
			case 3:
				mask = 0x7;
				break;
			case 4: 
				mask = 0xf;
				break;
			case 5:
				mask = 0x1f;
				break;
			case 6:
				mask = 0x3f;
				break;
			case 7:
				mask = 0x7f;
				break;
			case 8:
				mask = 0xff;
				break;
			default:
				dagutil_panic("This mode is not supported");
				break;
		}
		return mask;	
	}
	uint32_t cat_get_color_mask(uint32_t max_color)
	{
		uint32_t mask = 0;
		switch(max_color)
		{
			case 0:
				mask = 0x0;
				break;
			case 1:
				mask = 0x1;
				break;
			case 2:
				mask = 0x3;
				break;
			case 3:
				mask = 0x7;
				break;
			case 4:
				mask = 0xf;
				break;
			case 5:
				mask = 0x1f;
				break;
			case 6:
				mask = 0x3f;
				break;
			case 7:
				mask = 0x7f;
				break;
			case 8: 
				mask = 0xff;
				break;
			case 9:
				mask = 0x1ff;
				break;
			case 10:
				mask = 0x3ff;
				break;
			case 11:
				mask = 0x7ff;
				break;
			case 12:
				mask = 0xfff;
				break;
			default:
				dagutil_panic("This mode is not supported");
				break;
		}
		return mask;	
	}
	
	uint32_t cat_get_max_color(dag_component_t component)
	{
		int input_bits_count = 0;
		int  hash_bits_count  = 0;
		int  iface_bits_count = 0;
	
		if (cat_get_entry_mode() == ETH_HDLC_POS)
			return 0;
		input_bits_count = cat_get_no_of_input_bits(component);
		hash_bits_count = get_configured_hash_width(component);
		iface_bits_count = cat_get_max_interface(component);
		if ( (input_bits_count < 0 ) || (hash_bits_count  < 0 ) || (iface_bits_count < 0) )
			return 0;
		return (input_bits_count - hash_bits_count - iface_bits_count);
	}
	
	uint32_t cat_get_max_interface(dag_component_t component)
	{
		bool interface_overwrite;
		attr_uuid_t attribute;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attribute = dag_component_get_attribute_uuid(component,kBooleanAttributeEnableInterfaceOverwrite);
		
		interface_overwrite = dag_config_get_boolean_attribute(card_ref,attribute);
	
		switch(cat_get_entry_mode())
		{
			case ETH_HDLC_POS:
			case COLOR_HDLC_POS_COLOR_ETH:
			case COLOR_HASH_POS_COLOR_HASH_ETH:
			if(interface_overwrite == 0)
			{
				return 0;
			}
			else
			{
				return CAT_IFACE_BITS_COUNT;	
			}
			break;
			default:
				return -1;
		}
	}
	/*Currently Hardcoded - need to have a more general implementation*/
	uint32_t cat_get_max_hlb_or_hash()
	{
		/*have to call get_configured_hash_width -- here.for HSBM.*/
		/*else - OLD STYLE*/	
		switch(cat_get_entry_mode())
		{
			case ETH_HDLC_POS:
				return 0;
				break;
			case COLOR_HDLC_POS_COLOR_ETH:
				return 2;
				break;
			case COLOR_HASH_POS_COLOR_HASH_ETH:
					return 4;
				break;
			case COLOR_HASH_POS_COLOR_HASH_ETH_SHIFTED:
					return 4;
				break;
			default:
					return -1;      
				break;
		}
	
	}
	uint32_t get_configured_hash_width(dag_component_t component)
	{
		bool value_bool = 0;
		uint32_t value = 0;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		dag_component_t root = dag_config_get_root_component(card_ref);
		dag_component_t stream_component = dag_component_get_subcomponent(root,kComponentCAT,0);

		if(root ==NULL)
		{
			return -1;
		}
		
		if(stream_component == NULL)
		{
			return -1;
		}
		/*Getting Interface -  ByPass Attribute*/
		attribute = dag_component_get_attribute_uuid(stream_component,kBooleanAttributeVariableHashSupport);
		if(attribute == kAttributeInvalid)
		{
				return -1;
		}
		value_bool = dag_config_get_boolean_attribute(card_ref,attribute);
		
		if(value_bool)
		{
			/*we know that variable hash width is supported and it is configurable.*/
			attribute = dag_component_get_attribute_uuid(component,kUint32AttributeHashSize);
			if(attribute == kAttributeInvalid)
			{
				return -1;
			}	
			value = dag_config_get_uint32_attribute(card_ref,attribute);
			return value; /*This is the hash width on the packet.*/	
		}
		else /*BFS Morphing module not present.variable hash width not supported we have to implement the original logic.*/
		{
			switch(cat_get_entry_mode())
			{
				case ETH_HDLC_POS:
					return 0;
					break;
				case COLOR_HDLC_POS_COLOR_ETH:
					return 2;
					break;
				case COLOR_HASH_POS_COLOR_HASH_ETH:
					return 4;
					break;
				case COLOR_HASH_POS_COLOR_HASH_ETH_SHIFTED:
					return 4;
					break;
				default:
					return -1;      
					break;
			}	
		}
	}

	uint32_t cat_get_no_of_input_bits(dag_component_t component)
	{
		uint32_t mInputBits;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		attribute = dag_component_get_attribute_uuid(component, kUint32AttributeNumberOfInputBits);	
		mInputBits = dag_config_get_uint32_attribute(card_ref,attribute);
		return mInputBits;
		
	}
	uint32_t cat_get_no_of_output_bits(dag_component_t component)
	{
		uint32_t mOutputBits;
		dag_card_ref_t card_ref = dag_component_get_card_ref(component);
		attr_uuid_t attribute;
		attribute = dag_component_get_attribute_uuid(component,kUint32AttributeNumberOfOutputBits);
		mOutputBits = dag_config_get_uint32_attribute(card_ref,attribute);
		return mOutputBits;
	}
