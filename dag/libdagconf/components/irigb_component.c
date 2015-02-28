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
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/util/enum_string_table.h"
#include "../include/attribute_factory.h"
#include "../../../include/dag_attribute_codes.h"
#include "../include/create_attribute.h"

#include "dagutil.h"

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} irigb_state_t;

enum {
    IRIGB_CONFIG    = 0x00,
    IRIGB_DATA      = 0x04
};


/* IRIG-B */
static void irigb_dispose(ComponentPtr component);
static void irigb_default(ComponentPtr component);
static int irigb_post_initialize(ComponentPtr component);
static dag_err_t irigb_update_register_base(ComponentPtr component);

static void* irigb_time_stamp_get_value(AttributePtr attribute);
static void irigb_read(AttributePtr attribute, uint32_t *irigb_frame);

void input_source_to_string(AttributePtr attribute);
void auto_mode_to_string(AttributePtr attribute);
void irigb_time_to_string(AttributePtr attribute);

static Attribute_t irigb_config_attr[] = 
{
    {
        /* Name */                 "input_source",
        /* Attribute Code */       kUint32AttributeIRIGBInputSource,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Select the signal type on input source",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_IRIGB,
        /* Offset */               IRIGB_CONFIG,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x3, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          input_source_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "auto_detect_mode",
        /* Attribute Code */       kUint32AttributeIRIGBAutoMode,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "The format detected from serial input port",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_IRIGB,
        /* Offset */               IRIGB_CONFIG,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xC, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          auto_mode_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "irigb_time",
        /* Attribute Code */       kUint64AttributeIRIGBtimestamp,
        /* Attribute Type */        kAttributeUint64,
        /* Description */          "IRIG-B time",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_IRIGB,
        /* Offset */               IRIGB_DATA,
        /* Size/length */          1,
        /* Read */                 NULL,
        /* Write */                NULL,
        /* Mask */                 0, 
        /* Default Value */        0,
        /* SetValue */             attribute_uint64_set_value,
        /* GetValue */             irigb_time_stamp_get_value,
        /* SetToString */          irigb_time_to_string,
        /* SetFromString */        attribute_uint64_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

};

#define NB_ELEM     (sizeof(irigb_config_attr) / sizeof(Attribute_t))

ComponentPtr
irigb_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentIRIGB, card); 
    
    if (NULL != result)
    {
        irigb_state_t* state = NULL;
        component_set_dispose_routine(result, irigb_dispose);
        component_set_post_initialize_routine(result, irigb_post_initialize);
        component_set_default_routine(result, irigb_default);
        component_set_update_register_base_routine(result, irigb_update_register_base);
        component_set_name(result, "irigb");
        component_set_description(result, "The IRIG-B (Inter-range instrument group B time code)");
        state = (irigb_state_t*)malloc(sizeof(irigb_state_t));
        memset(state, 0, sizeof(irigb_state_t));
        component_set_private_state(result, state);
        state->mIndex = index;
    }
    
    return result;
}

static void
irigb_dispose(ComponentPtr component)
{
}

static void 
irigb_default(ComponentPtr component)
{
}

static int
irigb_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {

   	DagCardPtr card = component_get_card(component);
	irigb_state_t* state = (irigb_state_t*)component_get_private_state(component);
	state->mBase = card_get_register_address(card, DAG_REG_IRIGB, state->mIndex);

	state = component_get_private_state(component);
        if ( NULL != state )
		read_attr_array( component, irigb_config_attr, NB_ELEM, state->mIndex );

    	return 1;			
    }
    return kDagErrInvalidParameter;
}


static dag_err_t
irigb_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        irigb_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_IRIGB, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static void
irigb_read(AttributePtr attribute, uint32_t *irigb_frame)
{
        int i;
	DagCardPtr card = attribute_get_card(attribute);
	ComponentPtr component = attribute_get_component(attribute);
        irigb_state_t* state = (irigb_state_t*)component_get_private_state(component);

        for (i = 0; i < 4; i ++)
        {
                card_write_iom(card, state->mBase + IRIGB_CONFIG, i << 8);
                irigb_frame[i] = card_read_iom(card, state->mBase + IRIGB_DATA);
        }
}

uint32_t
select_bits(uint32_t *resp, int start, int size)
{
        int mask = (size < 32 ? 1 << size : 0) - 1;
        int off = ((start) / 32);
        int shft = (start) & 31;
        uint32_t res;

        res = resp[off] >> shft;
        if (size + shft > 32)
                res |= resp[off+1] << ((32 - shft) % 32);
        return res & mask;
}

static void
irigb_decode(uint32_t *irigb_frame, struct tm *date)
{
        uint32_t secs = 0, mins = 0, hours = 0, days = 0;
        uint32_t years = 0;

        /* decode the IRIG-B time */
        secs = select_bits(irigb_frame, 0, 4) + select_bits(irigb_frame, 5, 3) * 10;
        mins = select_bits(irigb_frame, 9, 4) + select_bits(irigb_frame, 14, 3) * 10;
        hours = select_bits(irigb_frame, 19, 4) + select_bits(irigb_frame, 24, 2) * 10;
        days = select_bits(irigb_frame, 29, 4) + select_bits(irigb_frame, 34, 4) * 10
                + select_bits(irigb_frame, 39, 2) * 100;

	years = select_bits(irigb_frame, 51, 4) + select_bits(irigb_frame, 56, 4) *10;
	if(!years)
		years = 100 + years;

        /* fill up break-down time structure */
        date->tm_sec = secs;                    /* seconds */
        date->tm_min = mins;                    /* minutes */
        date->tm_hour = hours;                  /* hours */
        date->tm_year = years;                  /* year */
        date->tm_yday = days;                   /* day in the year */
	date->tm_year = years;
}


static void* 
irigb_time_stamp_get_value(AttributePtr attribute)
{
	uint32_t irigb_frame[4];
	struct tm date; 
	uint32_t val1, val2;
	AttributePtr input_source = NULL;
	AttributePtr auto_detect = NULL;

	ComponentPtr comp = attribute_get_component(attribute);
	input_source = component_get_attribute(comp, kUint32AttributeIRIGBInputSource);
	auto_detect = component_get_attribute(comp,  kUint32AttributeIRIGBAutoMode);
	val1 = *(uint32_t *)attribute_get_value(input_source);
	val2 = *(uint32_t *)attribute_get_value(auto_detect);

	if (1 == valid_attribute(attribute))
	{
		
        	memset(&date, 0, sizeof(struct tm));
		// check the input source and auto detect mode are IRIG-B input.
		if ((val1 == 0 && val2 == 2) || val1 == 2 )
		{
			irigb_read(attribute, irigb_frame);
			irigb_decode(irigb_frame, &date);
		}
		attribute_set_value_array(attribute, &date, sizeof(struct tm));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}


void input_source_to_string(AttributePtr attribute)
{

    uint8_t value; 
    uint8_t *value_ptr = (uint8_t*)attribute_get_value(attribute); 
    char buffer[32];

    if (1 == valid_attribute(attribute))
    {
        if( value_ptr == NULL )
        {
            strcpy(buffer,"error");
            attribute_set_to_string(attribute, buffer); 
            return;
        }
        value = *value_ptr;
        if (value == 0)
        {
            strcpy(buffer, "auto-detect");
        }
        else if (value == 1)
        {
            strcpy(buffer, "forced pps");
        }
	else if (value == 2)
	{ 
            strcpy(buffer, "forced irig-b");
	}
	else if (value == 3)
	{
	    strcpy(buffer, "error");
	}
        attribute_set_to_string(attribute, buffer); 
    }
}

void 
auto_mode_to_string(AttributePtr attribute)
{

    uint8_t value; 
    uint8_t *value_ptr = (uint8_t*)attribute_get_value(attribute); 
    char buffer[32];

    if (1 == valid_attribute(attribute))
    {
        if( value_ptr == NULL )
        {
            strcpy(buffer,"error");
            attribute_set_to_string(attribute, buffer); 
            return;
        }
        value = *value_ptr;
        if (value == 0 || value == 3)
        {
            strcpy(buffer, "error");
        }
        else if (value == 1)
        {
            strcpy(buffer, "pps");
        }
	else if (value == 2)
	{ 
            strcpy(buffer, "irig-b");
	}
        attribute_set_to_string(attribute, buffer); 
    }
}

void 
irigb_time_to_string(AttributePtr attribute)
{
    char format[256];
    if (1 == valid_attribute(attribute))
    {
        struct tm *time_val = (struct tm *)irigb_time_stamp_get_value(attribute);
	
	if (time_val != NULL)
	{
	   sprintf(format, "%03d %02d:%02d:%02d + 1 sec", time_val->tm_yday, 
							time_val->tm_hour, 
							time_val->tm_min, 
							time_val->tm_sec);
           attribute_set_to_string(attribute, format);
	}
    }
}
