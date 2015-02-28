/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* Public API headers. */
#include "dag_config_util.h"
#include "dag_config.h"
/* Endace headers. */
#include "dagutil.h"

/* Internal Headers */
#include "./dag_component.h"
/* C Standard Library headers. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

//extern char *dagopttext;

int 
dag_config_util_domemconfig(dag_card_ref_t card, const char* text)
{
    dag_component_t any_comp= NULL;
    dag_component_t root_component = dag_config_get_root_component(card);
    attr_uuid_t any_attr;
    const char* startptr = NULL;
    char* endptr = NULL;
    char to_find[128] = {"="};
    uint32_t i = 0;
    uint32_t uint32_val;
    uint32_t total_buffer_size = 0;
    uint32_t rx_stream_count = 0;
    uint32_t tx_stream_count = 0;
    uint32_t streammax = 0;
    uint32_t requested = 0;
    char text_copy[256] = "";
    char *result = NULL;

    any_comp = dag_component_get_subcomponent(root_component, kComponentPbm, 0);
    assert(any_comp != NULL);
    /* Get the total available buffer size */
    any_attr = dag_component_get_attribute_uuid(any_comp, kUint32AttributeBufferSize);
    assert(any_attr != kNullAttributeUuid);
    total_buffer_size = dag_config_get_uint32_attribute(card, any_attr);
    /* Get the RX Stream Count */
    any_attr = dag_component_get_attribute_uuid(any_comp, kUint32AttributeRxStreamCount);
    assert(any_attr != kNullAttributeUuid);
    rx_stream_count = dag_config_get_uint32_attribute(card, any_attr);
    /* Get the TX Stream Count */
    any_attr = dag_component_get_attribute_uuid(any_comp, kUint32AttributeTxStreamCount);
    assert(any_attr != kNullAttributeUuid);
    tx_stream_count = dag_config_get_uint32_attribute(card, any_attr);
    
    streammax = dagutil_max(rx_stream_count*2 - 1, tx_stream_count*2);
    /*Parse dagopttext*/
    strcpy(text_copy, text);	// This needs to be done to prevent a compiler warning if we pass the constant string "text" to strtok()
    result = strtok(text_copy, " ");
    while(result != NULL)
    {
        if (strstr(result, "mem") != NULL)
        {
            startptr = result;
            break;
        }
        else
            result = strtok( NULL, " ");
    }

    endptr = strstr(startptr, to_find);
    endptr++;
    i = 0;
    strcpy(to_find, ":");

    /* Set all memory sizes to 0 first, start with clean slate */
    for( i  = 0 ; i < streammax; i++)
    {
        any_comp = dag_component_get_subcomponent(root_component, kComponentStream, i);
        assert(any_comp != NULL);
        any_attr = dag_component_get_attribute_uuid(any_comp, kUint32AttributeMem);
        assert(any_attr != kNullAttributeUuid);
        dag_config_set_uint32_attribute(card, any_attr, 0);
    }
     i = 0;
    do
    {
        uint32_val = strtol(endptr, NULL, 10);
        requested += uint32_val;
        if(requested > total_buffer_size)
        {
           dagutil_warning("More memory requested (%dMiB) than available (%dMiB)!\n", requested, total_buffer_size);
           return -1;
        }
        if(i&1)
        {
            if(uint32_val && (i > tx_stream_count*2))
            {
                dagutil_warning("Invalid assignment of memory to nonexistent tx stream %d!\n", i);
                return -1;
            }
        }
        else
        {
            if(uint32_val && (i > rx_stream_count*2))
            {
                dagutil_warning("Invalid assignment of memory to nonexistent rx stream %d!\n", i);
                return -1;
            }
        }
        /* Get the Stream Component */
        any_comp = dag_component_get_subcomponent(root_component, kComponentStream, i);
        assert(any_comp != NULL);
        /* Get the mem attribute */
        any_attr = dag_component_get_attribute_uuid(any_comp, kUint32AttributeMem);
        assert(any_attr != kNullAttributeUuid);
        dag_config_set_uint32_attribute(card, any_attr, uint32_val);
        i++;
        endptr = strstr(endptr, to_find);
        if (endptr)
            endptr++;
    } while (endptr);

    return 0;
}

void
dag_config_util_set_value_to_lval(dag_card_ref_t card_ref, dag_component_t root_component,
        dag_component_code_t component_code, int component_index, dag_attribute_code_t attribute_code, int lval)
{
    dag_component_t any_component;
    attr_uuid_t any_attribute;
    uint8_t uint8_val;
    uint32_t uint32_val;
    dag_attr_t attr_valuetype;

    any_component = dag_component_get_subcomponent(root_component, component_code, component_index);
    assert(any_component != NULL);
    if (!any_component)
        return;
    any_attribute = dag_component_get_attribute_uuid(any_component, attribute_code);
    assert(any_attribute != 0);
    if (!any_attribute)
        return;
    attr_valuetype = dag_config_get_attribute_valuetype(card_ref, any_attribute);
    switch (attr_valuetype)
    {
        case kAttributeBoolean:
            uint8_val = !(lval == 0);
            dag_config_set_boolean_attribute(card_ref, any_attribute, uint8_val);
            break;

        case kAttributeUint32:
            uint32_val = (uint32_t)lval;
            dag_config_set_uint32_attribute(card_ref, any_attribute, uint32_val);
            break;

        default:
            dagutil_warning("set_value_to_lval does not support attribute valuetype %d\n", attr_valuetype);
            break;
    }
}


void
dag_config_util_set_value_to_lval_verify(dag_card_ref_t card_ref, dag_component_t root_component,
        dag_component_code_t component_code, int component_index, dag_attribute_code_t attribute_code, int lval, char *token_name)
{
    dag_component_t any_component;
    attr_uuid_t any_attribute;
    uint8_t uint8_val, uint8_retval;
    uint32_t uint32_val, uint32_retval;
    dag_attr_t attr_valuetype;

	any_component = dag_component_get_subcomponent(root_component, component_code, component_index);
    assert(any_component != NULL);
    if (!any_component)
        return;
    any_attribute = dag_component_get_attribute_uuid(any_component, attribute_code);
    assert(any_attribute != 0);
    if (!any_attribute)
        return;
    attr_valuetype = dag_config_get_attribute_valuetype(card_ref, any_attribute);
    switch (attr_valuetype)
    {
        case kAttributeBoolean:
            uint8_val = !(lval == 0);
            dag_config_set_boolean_attribute(card_ref, any_attribute, uint8_val);
            uint8_retval = dag_config_get_boolean_attribute(card_ref, any_attribute);

            if (uint8_retval != uint8_val)
                dagutil_warning("Unsupported or invalid boolean configuration for this firmware or card: %s\n", token_name);
            break;

        case kAttributeUint32:
            uint32_val = (uint32_t)lval;
            dag_config_set_uint32_attribute(card_ref, any_attribute, uint32_val);
            uint32_retval = dag_config_get_uint32_attribute(card_ref, any_attribute);
            if (uint32_retval != uint32_val)
            {
                if (attribute_code == kUint32AttributeSnaplength)
                    dagutil_warning("Snaplength value not aligned to 64-bit boundary. Setting to closest lower 64-bit boundary.\n");
                else
                    dagutil_warning("Unsupported or invalid uint32 configuration for this firmware or card: %s\n", token_name);
            }
            break;

        default:
            dagutil_warning("set_value_to_lval does not support attribute valuetype %d\n", attr_valuetype);
            break;
    }
}

void
dag_config_util_set_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, void* value_p)
{
    dag_attr_t attr_valuetype;

    attr_valuetype = dag_config_get_attribute_valuetype(card_ref, uuid);
    switch (attr_valuetype)
    {
        case kAttributeBoolean:
        {
            uint8_t value = *(uint8_t*)value_p;
            dag_config_set_boolean_attribute(card_ref, uuid, !(value == 0));
            break;
        }
        case kAttributeChar:
        {
            char value = *(char*)value_p;
            dag_config_set_char_attribute(card_ref, uuid, value);
            break;
        }
        case kAttributeInt32:
        {
            int32_t value = *(int32_t*)value_p;
            dag_config_set_int32_attribute(card_ref, uuid, value);
            break;
        }
        case kAttributeUint32:
        {
            uint32_t value = *(uint32_t*)value_p;
            dag_config_set_uint32_attribute(card_ref, uuid, value);
            break;
        }
        case kAttributeInt64:
        {
            int64_t value = *(int64_t*)value_p;
            dag_config_set_int64_attribute(card_ref, uuid, value);
            break;
        }
        case kAttributeUint64:
        {
            uint64_t value = *(uint64_t*)value_p;
            dag_config_set_uint64_attribute(card_ref, uuid, value);
            break;
        }
        case kAttributeFloat:
        {
            float value = *(float*)value_p;
            dag_config_set_float_attribute(card_ref, uuid, value);
            break;
        }
        default:
            dagutil_warning("%s does not support attribute valuetype %d\n", __FUNCTION__, attr_valuetype);
            break;
    }
}

int dag_config_util_do_hlb_config(dag_card_ref_t card, const char* text)
{
    dag_component_t any_comp= NULL;
    dag_component_t root_component = dag_config_get_root_component(card);
    attr_uuid_t any_attr;
    const char* startptr = NULL;
    char* endptr = NULL;
    uint32_t loop = 0, loop_inner;
    uint32_t configured_stream;
    uint32_t rx_stream_count;
    
    uint32_t int_part,frac_part, final_int;
	hlb_range_t hlb_range_data;
	dag_card_t card_type;

    any_comp = dag_component_get_subcomponent(root_component, kComponentPbm, 0);
    assert(any_comp != NULL);
    /* Get the RX Stream Count */
    any_attr = dag_component_get_attribute_uuid(any_comp, kUint32AttributeRxStreamCount);
    assert(any_attr != kNullAttributeUuid);
    rx_stream_count = dag_config_get_uint32_attribute(card, any_attr);
    
    /*Parse dagopttext*/
    startptr = strstr(text, "hlb");
    startptr = strstr(startptr, "=");
    startptr++;/*omit the =*/

    loop = 0;
    memset(&hlb_range_data,0,sizeof(hlb_range_data));
    hlb_range_data.stream_num = rx_stream_count;
	while(1)
	{
		/*get range start*/
		int_part = strtoul(startptr,&endptr,10);
		if(startptr==endptr)
		{
			if(startptr=='\0')
				break;
			else{
				dagutil_warning("Invalid format\n");
				return -1;
			}
		}
		startptr = endptr;
		frac_part = 0;
		if(startptr[0]=='.')
		{
			startptr++;
			frac_part = strtoul(startptr,&endptr,10);
			startptr = endptr;
			while(frac_part>10)
				frac_part = frac_part/10;
		}
		final_int = int_part*10+frac_part;
		if(final_int >1000)
		{
			dagutil_warning("Invalid range, %d.%01d large than 100%%\n",int_part,frac_part);
			return -1;
		}
		hlb_range_data.stream_range[loop].min = final_int;
		
		if(startptr[0]=='-')
		{
			startptr++;
			/* get range end*/
			int_part = strtoul(startptr,&endptr,10);
			startptr = endptr;
			frac_part = 0;
			if(endptr[0]=='.'){
				startptr++;
				frac_part = strtoul(startptr,&endptr,10);
				startptr = endptr;
				while(frac_part>10)
					frac_part = frac_part/10;
			}
			final_int = int_part*10+frac_part;
			if(final_int >1000){
				dagutil_warning("Invalid range, %d.%01d large than 100%%\n",int_part,frac_part);
				return -1;
			}
			if(final_int<hlb_range_data.stream_range[loop].min)
			{
				dagutil_warning("Invalid range, start larger than end %d.%01d > %d.%10d \n",
							hlb_range_data.stream_range[loop].min/10, hlb_range_data.stream_range[loop].min%10,
							int_part,frac_part);
				return -1;
			}
			
			hlb_range_data.stream_range[loop].max = final_int;
		}
		else
		{
			dagutil_warning("Invalid format, need a range end\n");
			return -1;
		}
		loop++;
		if(startptr[0]==':')
		{
			startptr++;
			continue; /*continue with next*/
		}
		else if(startptr[0]=='\0'||startptr[0]==' '||startptr[0]=='\t')
			break; /*this is the last one*/
		else
		{
			dagutil_warning("Invalid format, cannot find :\n");
			return -1;
		}
	}

	if(loop>rx_stream_count)
	{
		dagutil_warning("Only %d streams are available, the rest of configuratin will be ignored\n",rx_stream_count);
		loop = rx_stream_count;
	}
	configured_stream=loop;
	
	/*check for overlap for 4.5Z, 8.2Z*/
	card_type = dag_config_get_card_type(card);
	if(card_type==kDag452z||card_type==kDag82z)
	{
	    uint32_t temp_min,temp_max;
    	uint32_t compare_min, compare_max;
		for(loop=0;loop<configured_stream;loop++)
		{
			temp_min = hlb_range_data.stream_range[loop].min;
			temp_max = hlb_range_data.stream_range[loop].max;
			if(temp_min==temp_max)
				continue;
			for(loop_inner=0;loop_inner<configured_stream;loop_inner++)
			{
				if(loop_inner==loop)
					continue;
				compare_min = hlb_range_data.stream_range[loop_inner].min;
				compare_max = hlb_range_data.stream_range[loop_inner].max;
				
				if(compare_min==compare_max)
					continue;
				if((temp_min<=compare_min&&temp_max>compare_min)||
					(temp_min<compare_max&&temp_max>=compare_max))
				{
					dagutil_warning("Illegal Duplicate is not enabled for this card model\n");
					return -1;
				}
			}
		}
	}

    any_comp = dag_component_get_subcomponent(root_component, kComponentHlb, 0);
    assert(any_comp != NULL);
    /* Get the hlb range attribute */
    any_attr = dag_component_get_attribute_uuid(any_comp, kStructAttributeHlbRange);
    assert(any_attr != kNullAttributeUuid);
    dag_config_set_struct_attribute(card, any_attr, &hlb_range_data);
	
    return 0;	
}

