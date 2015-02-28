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
#include "../include/components/counter_component.h"
#include "../include/components/counters_interface_component.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"
#include "../include/components/cat_component.h"
#include "../include/dag_attribute_codes.h"
#include "../include/attribute_types.h"
#include "../include/components/hlb_new_component.h"

/*Functions for the HAT Component*/
static void hat_dispose(ComponentPtr component);
static void hat_reset(ComponentPtr component);
static void hat_default(ComponentPtr component);
static int hat_post_initialize(ComponentPtr component);
static dag_err_t hat_update_register_base(ComponentPtr component);

/*Function to get the Range Attribute*/
static AttributePtr hat_get_new_range_attribute(void);
static void* hat_range_get_value(AttributePtr attribute);
static void hat_range_set_value(AttributePtr attribute, void* value, int length);
static void hat_range_dispose(AttributePtr attribute);
static void hat_range_post_initialize(AttributePtr attribute);

/*To string conversion function for struct range attribute*/
void hat_range_get_value_to_string(AttributePtr attribute);
void hat_range_set_value_from_string(AttributePtr attribute, const char *value);

/*Miscellaneous supporting functions*/
bool hat_get_encoding_mode(ComponentPtr component);
uint32_t hat_get_no_of_output_bits(ComponentPtr component);
static uint32_t hlb_range_drb_delay (volatile uint32_t * addr);
uint32_t check_overlapping_ranges(hat_range_t *hat);

void* hash_enable_get_value(AttributePtr attribute);
void hash_enable_set_value(AttributePtr attribute, void* value, int length);

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: hlb_new_component.c,v 1.29.2.10 2009/11/30 02:57:40 wilson.zhu Exp $";
static const char* const kRevisionString = "$Revision: 1.29.2.10 $";

/* HAT Register offset enum */
#define kControl 0x00
#define kInformation 0x04

#define HLB_TABLE_SIZE 1024
#define STRM_CAL_STATE_FOUND_NON 0
#define STRM_CAL_STATE_FOUND_MIN 1
#define STRM_CAL_STATE_FOUND_MAX 2

#define HLB_REG_BANK_MASK		0x40000000
#define HLB_REG_BANK_0			0x00000000
#define HLB_REG_BANK_1			0x40000000
#define HLB_REG_BANK_ADDR_0		0x00000000
#define HLB_REG_BANK_ADDR_1		0x08000000
#define HLB_REG_WRITE_BIT		0x80000000
#define HLB_REG_READ_BIT		0x00000000
#define HLB_REG_DATA_MASK		0x000001ff
#ifndef MIN
#define MIN(X,Y) ((X)<(Y))?(X):(Y)
#endif

Attribute_t hat_attr[]=
{
	{
		/* Name */                 "encoding_mode",
		/* Attribute Code */       kBooleanAttributeHatEncodingMode,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "HAT Encoding Mode.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    31,
		/* Register Address */     DAG_REG_HLB,
		/* Offset */               kInformation,
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
		/* Name */                 "no_of_output_bits",
		/* Attribute Code */       kUint32AttributeHatOutputBits,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "HAT Output Bits.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    16,
		/* Register Address */     DAG_REG_HLB,
		/* Offset */               kInformation,
		/* Size/length */          4,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT16 | BIT17 | BIT18 | BIT19,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},
	{
		/* Name */                 "no_of_input_bits",
		/* Attribute Code */       kUint32AttributeHatInputBits,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Hlb Input Bits.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_HLB,
		/* Offset */               kInformation,
		/* Size/length */          4,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT0 | BIT1 | BIT2 | BIT3,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},
	{
		/* Name */                 "hash_encoding_from_ipf",
		/* Attribute Code */       kBooleanAttributeHashEncoding,
		/* Attribute Type */       kAttributeBoolean,
		/* Description */          "Hash Encoding from IPF.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_PPF,
		/* Offset */               0x00,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT2,
		/* Default Value */        0,
		/* SetValue */             hash_enable_set_value,
		/* GetValue */             hash_enable_get_value,
		/* SetToString */          attribute_boolean_to_string,
		/* SetFromString */        attribute_boolean_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	},
	{
		/* Name */                 "n_tuple_select",
		/* Attribute Code */       kUint32AttributeNtupleSelect,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "Selects the N-tuple used for Hashing.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_PPF,
		/* Offset */               0x00,
		/* Size/length */          4,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 BIT20 | BIT21 | BIT22 | BIT23,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
	}
};
/* Number of elements in the array */
#define NB_ELEM (sizeof(hat_attr) / sizeof(Attribute_t))
ComponentPtr
hat_get_new_component(DagCardPtr card, uint32_t index)
{
	ComponentPtr result = component_init(kComponentHAT, card);
	hat_state_t* state = NULL;
	if (NULL != result)
	{
		component_set_dispose_routine(result, hat_dispose);
		component_set_post_initialize_routine(result, hat_post_initialize);
		component_set_reset_routine(result, hat_reset);
		component_set_default_routine(result, hat_default);
		component_set_update_register_base_routine(result, hat_update_register_base);
		component_set_name(result, "hat_interface");
		state = (hat_state_t*)malloc(sizeof(hat_state_t));
		state->mIndex = index;
		state->mHatBase = (uint32_t*)(card_get_iom_address(card) + card_get_register_address(card, DAG_REG_HLB, state->mIndex));
		component_set_private_state(result, state);
	}
	return result;
}
static
dag_err_t hat_update_register_base(ComponentPtr component)
{
	if (1 == valid_component(component))
	{
		hat_state_t* state = NULL;
		DagCardPtr card;
		state = component_get_private_state(component);
		card = component_get_card(component);
		NULL_RETURN_WV(state, kDagErrGeneral);
		state->mHatBase = (uint32_t*)(card_get_iom_address(card) + card_get_register_address(card, DAG_REG_HLB, state->mIndex)); 
		return kDagErrNone;
	}
	return kDagErrInvalidParameter;
}
static
int hat_post_initialize(ComponentPtr component)
{
	if (1 == valid_component(component))
	{
		DagCardPtr card = NULL;
		hat_state_t* state = NULL;
		AttributePtr hat_range_attribute;
		/* Get card reference */
		card = component_get_card(component);
		/* Get counter state structure */
		state = component_get_private_state(component);
		/*the base address*/
		state->mHatBase = (uint32_t*)(card_get_iom_address(card) + card_get_register_address(card, DAG_REG_HLB, state->mIndex));
		/* Add attribute of counter */
		read_attr_array(component, hat_attr, NB_ELEM, state->mIndex);
		/*Addting the hat range component*/
		hat_range_attribute = hat_get_new_range_attribute();
		component_add_attribute(component, hat_range_attribute);    

		return 1;
	}
	return kDagErrInvalidParameter;
}
static  
void hat_default(ComponentPtr component)
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
static
void hat_dispose(ComponentPtr component)
{
}
static
void hat_reset(ComponentPtr component)
{
}
/*Function for the HAT RANGE attribute*/
static
AttributePtr hat_get_new_range_attribute(void)
{
	AttributePtr result = attribute_init(kStructAttributeHATRange); 
	if (NULL != result)
	{
		attribute_set_dispose_routine(result, hat_range_dispose);
		attribute_set_post_initialize_routine(result, hat_range_post_initialize);
		attribute_set_name(result, "hat_range");
		attribute_set_getvalue_routine(result, hat_range_get_value);
		attribute_set_setvalue_routine(result, hat_range_set_value);
		attribute_set_to_string_routine(result,hat_range_get_value_to_string);
		attribute_set_from_string_routine(result,hat_range_set_value_from_string);
		attribute_set_description(result, "Set hat range for bins.");
		attribute_set_config_status(result, kDagAttrConfig);
		attribute_set_valuetype(result, kAttributeStruct);
	}
	return result;
}
static
void hat_range_dispose(AttributePtr attribute)
{
}
static
void hat_range_post_initialize(AttributePtr attribute)
{
}
static
uint32_t hlb_range_ui2reg(uint32_t ui_value)
{
	uint32_t ret_val;
	if(ui_value==999)
		return 1023;
	ret_val = ui_value*1024/1000;
		return ret_val;	
}
static
uint32_t g_hlb_reg2ui_tbl_flag = 0;
static
uint16_t g_hlb_reg2ui_tbl[1025];
static
void hlb_range_reg2ui_tbl_create()
{
	int loop;
	for(loop=0;loop<1025;loop++)
	{
		g_hlb_reg2ui_tbl[loop]=1000;
	}
	for(loop=0;loop<1000;loop++)
	{
		g_hlb_reg2ui_tbl[hlb_range_ui2reg(loop)]=loop;
	}
	g_hlb_reg2ui_tbl_flag=1;
}
static
int32_t hlb_range_reg2ui(uint32_t reg_value)
{
	if(g_hlb_reg2ui_tbl_flag==0)
		hlb_range_reg2ui_tbl_create();
	if(reg_value>1024)
		reg_value=1024;
	return g_hlb_reg2ui_tbl[reg_value];
}
static
void hlb_range_dump_table(uint16_t * table)
{
#if 0
 	int loop=0;
	for(loop=0;loop<HLB_TABLE_SIZE;loop++)
	{
		printf("%d: %d\n",loop,table[loop]);
	}	
#endif
}
static
void hlb_range_construct_table(ComponentPtr component,hat_range_t* range_data, uint16_t* table)
{
	uint32_t loop,idx;
	uint32_t reg_idx_start[DAG_STREAM_MAX];
	uint32_t reg_idx_end[DAG_STREAM_MAX];
	int bit_mask;
	uint32_t OutputBits;
	bool mode;
	uint32_t bin_num;
	uint32_t max_bins;

	mode = hat_get_encoding_mode(component);
	bin_num = range_data->bin_num;
	OutputBits = hat_get_no_of_output_bits(component);
	max_bins = (1 << OutputBits);
	if (bin_num  > max_bins)
	{
		bin_num = max_bins;
	}
	memset(table,0,sizeof(uint16_t)*HLB_TABLE_SIZE);
	/*get range for each memory hole*/
	for(loop=0;loop < bin_num;loop++)
	{
		if(range_data->bin_range[loop].min>=range_data->bin_range[loop].max)
		{
			/* zero range or invalid range*/
			reg_idx_start[loop]=HLB_TABLE_SIZE;
			reg_idx_end[loop]=HLB_TABLE_SIZE;
			continue;
		}
		
		reg_idx_start[loop] = hlb_range_ui2reg(range_data->bin_range[loop].min);
		reg_idx_end[loop] = hlb_range_ui2reg(range_data->bin_range[loop].max)-1;
	}
	bit_mask = 1;
	for(loop=0;loop<bin_num;loop++)
	{
	if(reg_idx_start[loop]!=HLB_TABLE_SIZE)
		for(idx=reg_idx_start[loop];idx<=reg_idx_end[loop];idx++)
		{
			if (mode == 0)
			{
				table[idx]= (loop);	
			}
			else/*mode value = 1*/
			 table[idx]|=bit_mask;
		}
		bit_mask = bit_mask<<1;
	}
	return;
}
static
void hlb_range_construct_range(ComponentPtr component,uint16_t* table, hat_range_t* range_data)
{
	uint32_t tbl_idx, stream;
	uint8_t stream_cal_state[DAG_STREAM_MAX];
	uint32_t stream_bit_mask[DAG_STREAM_MAX];

	uint32_t reg_idx_start[DAG_STREAM_MAX];
	uint32_t reg_idx_end[DAG_STREAM_MAX];
	bool mode;
	int OutputBits;
	uint32_t temp_variable = 0;
	ComponentPtr root;

	mode = hat_get_encoding_mode(component);
	OutputBits = hat_get_no_of_output_bits(component);

	root = component_get_parent(component);
	
	range_data->bin_num = (1<< OutputBits);/*DAG_STREAM_MAX_SUPPORTED;*/
	
	for(stream=0;stream<range_data->bin_num;stream++)
	{
		reg_idx_start[stream] = HLB_TABLE_SIZE;
		reg_idx_end[stream] = HLB_TABLE_SIZE;
		stream_bit_mask[stream] = (1<<stream);
	}
	memset(stream_cal_state,0,sizeof(stream_cal_state));
	for(tbl_idx=0;tbl_idx<HLB_TABLE_SIZE;tbl_idx++)
	{
		if(mode == 0)
		{
			temp_variable = (1 << (table[tbl_idx]));
		}
		else /*mode is 1*/
		{
			temp_variable = table[tbl_idx];
		}
		for(stream=0;stream<range_data->bin_num;stream++)
		{
			if((stream_cal_state[stream]==STRM_CAL_STATE_FOUND_NON)&&
				(temp_variable&stream_bit_mask[stream]))
			{
				reg_idx_start[stream] = tbl_idx;
				stream_cal_state[stream]=STRM_CAL_STATE_FOUND_MIN;
			}
			else if((stream_cal_state[stream]==STRM_CAL_STATE_FOUND_MIN)&&
					(!(temp_variable&stream_bit_mask[stream])))
			{
				reg_idx_end[stream] = tbl_idx-1;
				stream_cal_state[stream]=STRM_CAL_STATE_FOUND_MAX;
			}
			else if((stream_cal_state[stream]==STRM_CAL_STATE_FOUND_MAX)&&
				(temp_variable&stream_bit_mask[stream]))
			{
				/* error , we should have more than one range for a single stream*/
			}
		}
	}
	for(stream=0;stream<range_data->bin_num;stream++)
	{
		if(stream_cal_state[stream]==STRM_CAL_STATE_FOUND_MIN)
		{
			reg_idx_end[stream] = HLB_TABLE_SIZE-1;
		}
		if(stream_cal_state[stream]==STRM_CAL_STATE_FOUND_NON)
		{
			/*range not found set to 0-0*/
			range_data->bin_range[stream].min = 0;
			range_data->bin_range[stream].max = 0;
			continue;
		}
		/*convert the value*/
		range_data->bin_range[stream].min = 
				hlb_range_reg2ui(reg_idx_start[stream]);
		range_data->bin_range[stream].max = 
				hlb_range_reg2ui(reg_idx_end[stream]+1);

	}
	return;
}
static
void hlb_range_read_table(volatile uint32_t* addr, uint16_t* table)
{

	int loop;
	uint32_t data;
	uint32_t bank_bits;
	uint32_t cmd;


	/* get which bank is the current bank*/
	data = dag_readl (addr);
	if((data&HLB_REG_BANK_MASK)==HLB_REG_BANK_1)
	{
		/*We read the current bank*/
		bank_bits = HLB_REG_BANK_1|HLB_REG_BANK_ADDR_1;
	}
	else
	{
		bank_bits = HLB_REG_BANK_0|HLB_REG_BANK_ADDR_0;
	}
	/*write the table*/
	for(loop=0;loop<HLB_TABLE_SIZE;loop+=2)
	{
		cmd = HLB_REG_READ_BIT|bank_bits;
		/* We should shift the address 18 bit,
		   but each address contains two table entries, 
		   and loop is always a even number, so we shift 17 bit here*/
		cmd = cmd|(loop<<17);
		dag_writel(cmd,addr);
		hlb_range_drb_delay (addr);
		data = dag_readl(addr);
		table[loop] = (uint16_t)(data&HLB_REG_DATA_MASK);
		table[loop+1] = (uint16_t)(data>>9)&HLB_REG_DATA_MASK;
	}
}
static
void hlb_range_write_table(volatile uint32_t * addr,uint16_t* table)
{

	int loop;
	uint32_t data;
	uint32_t bank_bits;
	uint32_t bank_to_enable;
	uint32_t cmd;


	/* get which bank is the current bank*/
	data = dag_readl (addr);
	if((data&HLB_REG_BANK_MASK)==HLB_REG_BANK_1)
	{
		/*keep the current select bank, and program the other bank*/
		bank_bits = HLB_REG_BANK_1|HLB_REG_BANK_ADDR_0;
		bank_to_enable = HLB_REG_BANK_0;
	}
	else
	{
		bank_bits = HLB_REG_BANK_0|HLB_REG_BANK_ADDR_1;
		bank_to_enable = HLB_REG_BANK_1;
	}
	
	/*write the table*/
	for(loop=0;loop<HLB_TABLE_SIZE;loop+=2)
	{
		cmd = HLB_REG_WRITE_BIT|bank_bits;
		/* We should shift the address 18 bit,
		   but each address contains two table entries, 
		   and loop is always a even number, so we shift 17 bit here*/
		cmd = cmd|(loop<<17);  
		cmd = cmd|(table[loop]&HLB_REG_DATA_MASK);
		cmd = cmd|((table[loop+1]&HLB_REG_DATA_MASK)<<9);
		dag_writel(cmd,addr);
		hlb_range_drb_delay (addr);
	}
	
	/*switch to the bank we just programmed*/
	cmd = bank_to_enable;
	dag_writel (cmd, addr);

}
static hat_range_t hat_range_data;

void* hat_range_get_value(AttributePtr attribute)
{
	DagCardPtr card = NULL;
	ComponentPtr component = NULL;
	hat_state_t* state = NULL;
	uint16_t* table;
	hat_range_t* ret_val = NULL;
	if (1 == valid_attribute(attribute))
	{
		ret_val = &hat_range_data;
		card = attribute_get_card(attribute);
		component = attribute_get_component(attribute);
		state = (hat_state_t*)component_get_private_state(component);
		table = malloc(HLB_TABLE_SIZE*sizeof(uint16_t));
		if(table==NULL)
			return NULL;
		hlb_range_read_table(state->mHatBase,table);
		hlb_range_dump_table(table);
		hlb_range_construct_range(component,table,ret_val);
		free(table);
        attribute_set_value_array(attribute,ret_val, sizeof(hat_range_t));
	}
	return (void *)attribute_get_value_array(attribute);
}

void hat_range_set_value(AttributePtr attribute, void* value, int length)
{
	DagCardPtr card = NULL;
	ComponentPtr component = NULL;
	hat_state_t* state = NULL;
	uint16_t* table;
	hat_range_t* range_data = value;
	uint32_t overlapping;
	/*Code to verify the overlapping*/
	card = attribute_get_card(attribute);
	overlapping = check_overlapping_ranges(range_data);
	if(overlapping != 0)
	{
		printf("Please Enter proper ranges \n");
		/*Setting Invalid parameter error code and returning */
		card_set_last_error(card,kDagErrInvalidParameter);
		return;
	}
	if(range_data==NULL)
		return;

	if (1 == valid_attribute(attribute))
	{
		component = attribute_get_component(attribute);
		state = (hat_state_t*)component_get_private_state(component);
		table = malloc(HLB_TABLE_SIZE*sizeof(uint16_t));
		if(table==NULL)
			return;

		hlb_range_construct_table(component,range_data,table);
		hlb_range_dump_table(table);
		hlb_range_write_table(state->mHatBase,table);
		free(table);
		card_set_last_error(card,kDagErrNone);
	}
}
static
uint32_t hlb_range_drb_delay (volatile uint32_t * addr)
{
	uint32_t value = 0;
	uint32_t i;
				
	for (i=0; i<16; i++)
		value += dag_readl (addr);
					
	return value;
}
bool hat_get_encoding_mode(ComponentPtr component)
{

	 AttributePtr Mode;
	 bool *mode = NULL;
	 Mode = component_get_attribute(component,kBooleanAttributeHatEncodingMode);
	 if(Mode)
	 {
		mode = attribute_boolean_get_value(Mode);
	 }
	 return *mode;
}
uint32_t hat_get_no_of_output_bits(ComponentPtr component)
{
	 uint32_t OutputBits = 0;
	 AttributePtr attribute;
	 uint32_t hash_width = 0;
	 uint32_t hash_output = 0;
	 ComponentPtr cat_component;
	 ComponentPtr root;
	 bool bool_value = false;
	 attribute = component_get_attribute(component,kUint32AttributeHatOutputBits);
	 if(attribute)
	 {
		OutputBits = *(uint32_t*)attribute_get_value(attribute);
	 }
	 root = component_get_parent(component);
	 cat_component = component_get_subcomponent(root,kComponentCAT,0);
         if(cat_component == NULL)
         {
             dagutil_verbose_level(2,"Faild to get CAT component. \n");
         }
	 attribute = component_get_attribute(cat_component,kBooleanAttributeVariableHashSupport);
	 if(attribute)
	 {
	 	bool_value = *(uint8_t*)attribute_get_value(attribute);
	 }
	 if(bool_value)
	 {
		attribute = component_get_attribute(cat_component,kUint32AttributeHashSize);
		if(attribute == NULL)
		{
			dagutil_verbose_level(2,"Failed to get Variable HASH size attribute.\n");
		}
		else
		{
			hash_width = *(uint32_t*)attribute_get_value(attribute);
		}
	}
	else/*For older cards which does not support variable hashing*/
	{
		/*hash_width = OutputBits returned by the firmware.the min function will return the same number */
		hash_width=OutputBits;
	}
	hash_output = MIN(OutputBits,hash_width);
	return hash_output;
}
uint32_t check_overlapping_ranges(hat_range_t *hat)
{
	int i,j;
	for(i = 0;i < hat->bin_num;i++)
	{
		for(j = 0;j < hat->bin_num;j++)
		{
			if((hat->bin_range[j].min <= 1000 && hat->bin_range[j].max <= 1000))
			{
				if((i != j))
				{
					if(hat->bin_range[i].min <= hat->bin_range[j].min)
					{
						if((hat->bin_range[i].max > hat->bin_range[j].min))
						{
							if(((hat->bin_range[i].min == 0)&& (hat->bin_range[j].max ==0))||
			 	 			((hat->bin_range[j].min == 0)&& (hat->bin_range[j].max ==0)))
							{	
								continue;
							}
							else
							{
								printf("overlapping indices %d: %d \n",i,j);
								return -1;
							}
						}
					}
				}
			}
			else
			{
				printf("One of the values is greater that 1000.Please check \n");
				return -1;
			}
		
		}
	}	
	return 0;
}

void hat_range_get_value_to_string(AttributePtr attribute)
{
	DagCardPtr card = NULL;
	ComponentPtr component = NULL;
	char *final_string;
	hat_state_t* state = NULL;
	uint16_t* table;
	hat_range_t* ret_val = NULL;
	int loop;
	
	char *tmp_string; 
	if (1 == valid_attribute(attribute))
	{
		ret_val = &hat_range_data;
		card = attribute_get_card(attribute);
		component = attribute_get_component(attribute);
		state = (hat_state_t*)component_get_private_state(component);
		table = malloc(HLB_TABLE_SIZE*sizeof(uint16_t));
		if(table==NULL)
			return;
		hlb_range_read_table(state->mHatBase,table);
		hlb_range_dump_table(table);
		hlb_range_construct_range(component,table,ret_val);
		free(table);
	}
	/*reconstructing the ranges*/
	final_string = (char*)malloc((4*DAG_STREAM_MAX*2)+20);
	memset(final_string,0,((4*DAG_STREAM_MAX*2)+20));
	tmp_string = (char*)malloc(5);
	for (loop = 0; loop < ret_val->bin_num;loop++)
	{
		sprintf(tmp_string,"%d",ret_val->bin_range[loop].min);
		strcat(final_string,tmp_string);	
		sprintf(tmp_string,"%s","-");
		strcat(final_string,tmp_string);
		sprintf(tmp_string,"%d",ret_val->bin_range[loop].max);
		strcat(final_string,tmp_string);
		if (loop != 32)
		{
			sprintf(tmp_string,"%s",":");
			strcat(final_string,tmp_string);
		}
	}
	/*printf("Reconstructed length of the string = %d\n",strlen(final_string));*/
	attribute_set_to_string(attribute,final_string);
	free(tmp_string);
	free(final_string);
}
void hat_range_set_value_from_string(AttributePtr attribute, const char *value)
{
	/*String to structure conversion*/
	char *subtoken,*token ;
 	char *saveptr = NULL; 
	char *saveptr2 = NULL;
	
	int loop;
	char *str;
	char *array = (char*)value;
	static int count;
	DagCardPtr card = NULL;
	ComponentPtr component = NULL;
	hat_state_t* state = NULL;
	uint16_t* table;
	uint32_t overlapping;
	hat_range_t* range_data = &hat_range_data;
	for(loop = 1,str = array;;loop++,str = NULL)
	{
		 token = strtok_r(str, ":", &saveptr);
		 if (token == NULL)
		 	break;
		 subtoken = strtok_r(token,"-",&saveptr2);
		 if(subtoken == NULL)
		 	return;
		 hat_range_data.bin_range[loop -1].min = atoi(subtoken);
		 hat_range_data.bin_range[loop -1].max = atoi(saveptr2);
		 count++;
	}
	hat_range_data.bin_num = count;
	/*Code to verify the overlapping*/

	card = attribute_get_card(attribute);
	overlapping = check_overlapping_ranges(&hat_range_data);
	if(overlapping != 0)
	{
		printf("The ranges are overlapping.Please Enter proper ranges \n");
		/*Setting Invalid parameter error code and returning */
		card_set_last_error(card,kDagErrInvalidParameter);
		return;
	}
	if(range_data==NULL)
		return;
	if (1 == valid_attribute(attribute))
	{
		component = attribute_get_component(attribute);
		/*Mode*/
		state = (hat_state_t*)component_get_private_state(component);
		table = malloc(HLB_TABLE_SIZE*sizeof(uint16_t));
		if(table==NULL)
			return;

		hlb_range_construct_table(component,range_data,table);
		hlb_range_dump_table(table);
		hlb_range_write_table(state->mHatBase,table);
		free(table);
		card_set_last_error(card,kDagErrNone);
	}
}
void hash_enable_set_value(AttributePtr attribute, void* value, int length)
{
	if(1 == valid_attribute(attribute))
	{
		ComponentPtr component;
		AttributePtr attr;
		ComponentPtr root;
		uint32_t int_val = 4; /*By default we are setting the value of HASH width to be four.*/
		uint32_t hash_width = 0;
		bool bool_value = false;
		component = attribute_get_component(attribute);
		root = component_get_parent(component);

		component = component_get_subcomponent(root,kComponentCAT,0);
		if(component == NULL)
		{
			dagutil_verbose_level(2,"Faild to get CAT component. \n");
			attribute_boolean_set_value(attribute,value,length);
		}
		else
		{
			attr = component_get_attribute(component,kBooleanAttributeVariableHashSupport);
			bool_value = *(uint8_t*)attribute_get_value(attr);
			if(bool_value)
			{
				attr = component_get_attribute(component,kUint32AttributeHashSize);
				if(attribute == NULL)
				{
					dagutil_verbose_level(2,"Failed to get Variable HASH size attribute.\n");
				}
				else
				{
					hash_width = *(uint32_t*)attribute_get_value(attr);
					if(*(uint8_t*)value == 1)
					{
						if(hash_width == 0)
						{		
							attribute_set_value(attr,&int_val,1);
						}else
						{
							/*Do nothing - user has set the hash width to some non-zero value.*/	
						}
					}
					else
					{
						int_val = 0; //turn it off
						attribute_set_value(attr,&int_val,1);
					}
				}
			}
			else
			{					
				/*Set the value to the register.This is the actual bit.*/
				attribute_boolean_set_value(attribute,value,length);
			}
		}
	}
}
void* hash_enable_get_value(AttributePtr attribute)
{
	if(1 == valid_attribute(attribute))
	{
		ComponentPtr component;
		AttributePtr attr;
		ComponentPtr root;
		uint32_t value = 0;
		bool bool_value;
		component = attribute_get_component(attribute);
		root = component_get_parent(component);

		component = component_get_subcomponent(root,kComponentCAT,0);
		if(component == NULL)
		{
			dagutil_verbose_level(2,"Failed to get CAT component. \n");
			/*Set the value to the register.This is the actual bit.*/
            bool_value = *(uint8_t*)attribute_boolean_get_value(attribute);

		}
		else
		{
			attr = component_get_attribute(component,kBooleanAttributeVariableHashSupport);
			bool_value = *(uint8_t*)attribute_get_value(attr);
			if(bool_value)
			{
				/*This version supports Variable Hash Width.Interpreted as follows*/
				attr = component_get_attribute(component,kUint32AttributeHashSize);
				if(attr == NULL)
				{
					dagutil_verbose_level(2,"Failed to get Variable HASH size attribute.\n");
				}
				else
				{
					value = *(uint32_t*)attribute_get_value(attr);
					if(value > 0)
					{
						bool_value = true;
					}
					else
					{		
						bool_value = false;
					}
				}
			}
			else
			{
				/*Set the value to the register.This is the actual bit.*/
				bool_value = *(uint8_t*)attribute_boolean_get_value(attribute);
			}
		}
		attribute_set_value_array(attribute,&bool_value,sizeof(bool_value));
		return (void*)attribute_get_value_array(attribute);		
	}
	return NULL;
}
