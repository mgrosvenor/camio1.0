/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
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
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/attribute_factory.h"
#include "../include/components/hlb_component.h"
#include "dag_attribute_codes.h"
#include "dagapi.h"





#define HLB_REG_BANK_MASK		0x40000000
#define HLB_REG_BANK_0			0x00000000
#define HLB_REG_BANK_1			0x40000000
#define HLB_REG_BANK_ADDR_0		0x00000000
#define HLB_REG_BANK_ADDR_1		0x08000000
#define HLB_REG_WRITE_BIT		0x80000000
#define HLB_REG_READ_BIT		0x00000000
#define HLB_REG_DATA_MASK		0x000001ff

#define HLB_TABLE_SIZE		(0x1<<10)


#define STRM_CAL_STATE_FOUND_NON	0
#define STRM_CAL_STATE_FOUND_MIN	1
#define STRM_CAL_STATE_FOUND_MAX	2




uint32_t hlb_range_drb_delay (volatile uint32_t * addr);

static void hlb_component_dispose(ComponentPtr component);
static dag_err_t hlb_component_update_register_base(ComponentPtr component);
static int hlb_component_post_initialize(ComponentPtr component);
static void hlb_component_reset(ComponentPtr component);
static void hlb_component_default(ComponentPtr component);

static AttributePtr hlb_get_new_range_attribute(void);


typedef struct hlb_component_state_s
{
	uint32_t* base_addr;
}hlb_component_state_t;


ComponentPtr hlb_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentHlb, card); 
    hlb_component_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, hlb_component_dispose);
        component_set_post_initialize_routine(result, hlb_component_post_initialize);
        component_set_reset_routine(result, hlb_component_reset);
        component_set_default_routine(result, hlb_component_default);
        component_set_name(result, "hlb");
        component_set_update_register_base_routine(result, hlb_component_update_register_base);
        component_set_description(result, "Hash Load Balancer");
        state = (hlb_component_state_t*)malloc(sizeof(hlb_component_state_t));
        component_set_private_state(result, state);
    }
    return result;
}


static void hlb_component_dispose(ComponentPtr component)
{
}

static dag_err_t hlb_component_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        hlb_component_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->base_addr =   (uint32_t*)(card_get_iom_address(card) + card_get_register_address(card, DAG_REG_HLB, 0));
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int hlb_component_post_initialize(ComponentPtr component)
{
    hlb_component_state_t* state = NULL;
    DagCardPtr card;
	AttributePtr   hlb_range_attr;
    state = component_get_private_state(component);
    card = component_get_card(component);

    state->base_addr =   (uint32_t*)(card_get_iom_address(card) + card_get_register_address(card, DAG_REG_HLB, 0));
    hlb_range_attr = hlb_get_new_range_attribute();

    component_add_attribute(component, hlb_range_attr);    
	return 0;
}

static void hlb_component_reset(ComponentPtr component)
{
}


static void hlb_component_default(ComponentPtr component)
{
}




static void hlb_range_dispose(AttributePtr attribute);
static void* hlb_range_get_value(AttributePtr attribute);
static void hlb_range_set_value(AttributePtr attribute, void* value, int length);
static void hlb_range_post_initialize(AttributePtr attribute);

static AttributePtr hlb_get_new_range_attribute(void)
{
    AttributePtr result = attribute_init(kStructAttributeHlbRange); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, hlb_range_dispose);
        attribute_set_post_initialize_routine(result, hlb_range_post_initialize);
        attribute_set_name(result, "hlb_range");
        attribute_set_getvalue_routine(result, hlb_range_get_value);
        attribute_set_setvalue_routine(result, hlb_range_set_value);
        attribute_set_description(result, "Set hlb range for streams");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeStruct);
    }
    
    return result;
}




static void hlb_range_dispose(AttributePtr attribute)
{

}


static void
hlb_range_post_initialize(AttributePtr attribute)
{

}

static uint32_t hlb_range_ui2reg(uint32_t ui_value)
{
	uint32_t ret_val;
	if(ui_value==999)
		return 1023;
	ret_val = ui_value*1024/1000;
	return ret_val;	
}


static uint32_t g_hlb_reg2ui_tbl_flag = 0;
static uint16_t g_hlb_reg2ui_tbl[1025];


static void hlb_range_reg2ui_tbl_create()
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

static int32_t hlb_range_reg2ui(uint32_t reg_value)
{
	if(g_hlb_reg2ui_tbl_flag==0)
		hlb_range_reg2ui_tbl_create();
	if(reg_value>1024)
		reg_value=1024;
	return g_hlb_reg2ui_tbl[reg_value];
}

static void hlb_range_dump_table(uint16_t * table)
{
#if 0 
	int loop=0;
	for(loop=0;loop<HLB_TABLE_SIZE;loop++)
	{
		printf("%08x: %08x\n",loop,table[loop]);
	}	
#endif
}

static void hlb_range_construct_table(hlb_range_t* range_data, uint16_t* table,uint32_t stream_max)
{
	uint32_t loop,idx;
	uint32_t reg_idx_start[DAG_STREAM_MAX];
	uint32_t reg_idx_end[DAG_STREAM_MAX];
	int bit_mask;
	uint32_t stream_num = range_data->stream_num;

	memset(table,0,sizeof(uint16_t)*HLB_TABLE_SIZE);
	
	/*get range for each memory hole*/
	if(stream_num>stream_max)
	{
		stream_num = stream_max;
	}
	for(loop=0;loop<stream_num;loop++)
	{
		if(range_data->stream_range[loop].min>=
			range_data->stream_range[loop].max)
		{
			/* zero range or invalid range*/
			reg_idx_start[loop]=HLB_TABLE_SIZE;
			reg_idx_end[loop]=HLB_TABLE_SIZE;
			continue;
		}
		reg_idx_start[loop] = hlb_range_ui2reg(range_data->stream_range[loop].min);
		reg_idx_end[loop] = hlb_range_ui2reg(range_data->stream_range[loop].max)-1;
	}
	
	bit_mask = 1;
	for(loop=0;loop<stream_num;loop++)
	{
		if(reg_idx_start[loop]!=HLB_TABLE_SIZE)
			for(idx=reg_idx_start[loop];idx<=reg_idx_end[loop];idx++)
			{
				table[idx]|=bit_mask;
			}
		bit_mask = bit_mask<<1;
	}
	return;
}


static void hlb_range_construct_range(uint16_t* table, hlb_range_t* range_data,uint32_t stream_max)
{
	uint32_t tbl_idx, stream;
	uint8_t stream_cal_state[DAG_STREAM_MAX];
	uint32_t stream_bit_mask[DAG_STREAM_MAX];

	uint32_t reg_idx_start[DAG_STREAM_MAX];
	uint32_t reg_idx_end[DAG_STREAM_MAX];

	range_data->stream_num = stream_max;
	for(stream=0;stream<range_data->stream_num;stream++)
	{
		reg_idx_start[stream] = HLB_TABLE_SIZE;
		reg_idx_end[stream] = HLB_TABLE_SIZE;
		stream_bit_mask[stream] = (1<<stream);
	}
	
	memset(stream_cal_state,0,sizeof(stream_cal_state));
	for(tbl_idx=0;tbl_idx<HLB_TABLE_SIZE;tbl_idx++)
	{
		for(stream=0;stream<range_data->stream_num;stream++)
		{
			if((stream_cal_state[stream]==STRM_CAL_STATE_FOUND_NON)&&
				(table[tbl_idx]&stream_bit_mask[stream]))
			{
				reg_idx_start[stream] = tbl_idx;
				stream_cal_state[stream]=STRM_CAL_STATE_FOUND_MIN;
			}
			else if((stream_cal_state[stream]==STRM_CAL_STATE_FOUND_MIN)&&
					(!(table[tbl_idx]&stream_bit_mask[stream])))
			{
				reg_idx_end[stream] = tbl_idx-1;
				stream_cal_state[stream]=STRM_CAL_STATE_FOUND_MAX;
			}
			else if((stream_cal_state[stream]==STRM_CAL_STATE_FOUND_MAX)&&
				(table[tbl_idx]&stream_bit_mask[stream]))
			{
				/* error , we should have more than one range for a single stream*/
			}
		}
	}
	for(stream=0;stream<range_data->stream_num;stream++)
	{
		if(stream_cal_state[stream]==STRM_CAL_STATE_FOUND_MIN)
		{
			reg_idx_end[stream] = HLB_TABLE_SIZE-1;
		}
		if(stream_cal_state[stream]==STRM_CAL_STATE_FOUND_NON)
		{
			/*range not found set to 0-0*/
			range_data->stream_range[stream].min = 0;
			range_data->stream_range[stream].max = 0;
			continue;
		}
		/*convert the value*/
		range_data->stream_range[stream].min = 
				hlb_range_reg2ui(reg_idx_start[stream]);
		range_data->stream_range[stream].max = 
				hlb_range_reg2ui(reg_idx_end[stream]+1);
	}
	return;
}


static void hlb_range_write_table(volatile uint32_t* addr, uint16_t* table)
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

static void hlb_range_read_table(volatile uint32_t* addr, uint16_t* table)
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


static uint32_t hlb_range_get_stream_number(DagCardPtr card)
{
	ComponentPtr any_comp;
	ComponentPtr root_component;
	AttributePtr any_attr;
	uint32_t stream_num;

	root_component = card_get_root_component(card);	
    any_comp = component_get_subcomponent(root_component, kComponentPbm, 0);
    assert(any_comp != NULL);

    /* Get the RX Stream Count */
    any_attr = component_get_attribute(any_comp, kUint32AttributeRxStreamCount);
    assert(any_attr != (AttributePtr)kNullAttributeUuid);
    stream_num =  *((uint32_t*)attribute_get_value(any_attr));
    return stream_num;
}

static hlb_range_t hlb_range_data;
static void* hlb_range_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr component = NULL;
    hlb_component_state_t* state = NULL;

	uint16_t* table;
	hlb_range_t* ret_val = NULL;
	uint32_t stream_max;


    if (1 == valid_attribute(attribute))
    {
        ret_val = &hlb_range_data;
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (hlb_component_state_t*)component_get_private_state(component);
		
		stream_max = hlb_range_get_stream_number(card);
		
		table = malloc(HLB_TABLE_SIZE*sizeof(uint16_t));
		if(table==NULL)
			return NULL;
		hlb_range_read_table(state->base_addr,table);
		hlb_range_dump_table(table);
		hlb_range_construct_range(table,ret_val,stream_max);
		free(table);
        attribute_set_value_array(attribute, ret_val, sizeof(hlb_range_t));
    }
    return (void *)attribute_get_value_array(attribute);
}



static void hlb_range_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr component = NULL;
    hlb_component_state_t* state = NULL;

	uint16_t* table;
	hlb_range_t* range_data = value;
	uint32_t stream_max;

	if(range_data==NULL)
		return;
	
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (hlb_component_state_t*)component_get_private_state(component);

		stream_max = hlb_range_get_stream_number(card);

		table = malloc(HLB_TABLE_SIZE*sizeof(uint16_t));
		if(table==NULL)
			return;
		hlb_range_construct_table(range_data,table,stream_max);
		hlb_range_dump_table(table);
		hlb_range_write_table(state->base_addr,table);
		free(table);
	}
}


uint32_t hlb_range_drb_delay (volatile uint32_t * addr)
{
	uint32_t value = 0;
	uint32_t i;
	
	for (i=0; i<16; i++)
		value += dag_readl (addr);
	
	return value;
}


