/*
 * Copyright (c) 2003-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: sc256.c,v 1.4 2008/10/01 03:21:54 dlim Exp $
 */

#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/sc256.h"

#include "dagutil.h"
#include "dagapi.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif

static void sc256_dispose(ComponentPtr component);
static int sc256_post_initialize(ComponentPtr component);
static void sc256_reset(ComponentPtr component);

static int sc256_write_cam_reg(ComponentPtr component, uint32_t address, const sc256_72bit_data_t* data);
#if 0
static int sc256_read_cam_reg(ComponentPtr component, sc256_72bit_data_t* data);
#endif

/*
Normally we'd setup these attributes via the attribute factory but because the reading/writing of an
attribute in this component involves multiple read/writes accross different addresses to get a result
we set them up in this file. We also don't use a generic_read_write object but go directly iom. Hopefully nobody 
decides to use a RAW SMBus to read/write data to the SC256.
*/

static AttributePtr sc256_72bit_data(void);
static void* sc256_72bit_get_data(AttributePtr attribute);
static void sc256_72bit_set_data(AttributePtr attribute, void* value, int length);
static void sc256_72bit_data_to_string(AttributePtr attribute);
static void sc256_72bit_data_from_string(AttributePtr attribute, const char * string);

static AttributePtr sc256_data_address(void);
static void* sc256_data_address_get_value(AttributePtr attribute);
static void sc256_data_address_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr sc256_72bit_mask(void);
static void* sc256_72bit_get_mask(AttributePtr attribute);
static void sc256_72bit_set_mask(AttributePtr attribute, void* value, int length);
static void sc256_72bit_mask_to_string(AttributePtr attribute);
static void sc256_72bit_mask_from_string(AttributePtr attribute, const char * string);

static AttributePtr sc256_mask_address(void);
static void* sc256_mask_address_get_value(AttributePtr attribute);
static void sc256_mask_address_set_value(AttributePtr attribute, void* value, int length);


static AttributePtr sc256_72bit_search(void);
static void* sc256_72bit_search_get_value(AttributePtr attribute);
static void sc256_72bit_search_set_value(AttributePtr attribute, void* value, int length);
static void sc256_72bit_search_to_string(AttributePtr attribute);
static void sc256_72bit_search_from_string(AttributePtr attribute, const char * string);

static AttributePtr sc256_144bit_search(void);
static void* sc256_144bit_search_get_value(AttributePtr attribute);
static void sc256_144bit_search_set_value(AttributePtr attribute, void* value, int length);
static void sc256_144bit_search_to_string(AttributePtr attribute);
static void sc256_144bit_search_from_string(AttributePtr attribute, const char * string);

static AttributePtr sc256_search_length(void);
static void* sc256_search_length_get_value(AttributePtr attribute);
static void sc256_search_length_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr sc256_init(void);
static void sc256_init_set_value(AttributePtr attribute, void* value, int length);


typedef struct
{
    uint32_t mBase;
} sc256_state_t;

ComponentPtr
sc256_get_new_component(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentSC256, card); 
    
    if (NULL != result)
    {
        sc256_state_t* state;
        component_set_dispose_routine(result, sc256_dispose);
        component_set_post_initialize_routine(result, sc256_post_initialize);
        component_set_reset_routine(result, sc256_reset);
        component_set_name(result, "sc256");
        component_set_description(result, "SC256 Copro");
        state = malloc(sizeof(sc256_state_t));
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
sc256_dispose(ComponentPtr component)
{
}

static int
sc256_post_initialize(ComponentPtr component)
{
    /* Return 1 if standard post_initialize() should continue, 0 if not.
    * "Standard" post_initialize() calls post_initialize() recursively on subcomponents.
    */
    DagCardPtr card = NULL;
    uint32_t address = 0;

    if (1 == valid_component(component))
    {
        card = component_get_card(component);
        if ((address = card_get_register_address(card, DAG_REG_IDT_TCAM, 0)))
        {
            sc256_state_t* state = (sc256_state_t*)component_get_private_state(component);
            AttributePtr attr;

            state->mBase = address;

            attr = sc256_72bit_data();
            component_add_attribute(component, attr);

            attr = sc256_data_address();
            component_add_attribute(component, attr);

            attr = sc256_72bit_search();
            component_add_attribute(component, attr);

            attr = sc256_144bit_search();
            component_add_attribute(component, attr);

            attr = sc256_init();
            component_add_attribute(component, attr);
            
            attr = sc256_72bit_mask();
            component_add_attribute(component, attr);

            attr = sc256_mask_address();
            component_add_attribute(component, attr);

            attr = sc256_search_length();
            component_add_attribute(component, attr);
            return 1;
        }
    }

    return 0;
}

static void
sc256_reset(ComponentPtr component)
{
}

static AttributePtr
sc256_init(void)
{
    AttributePtr result = NULL;

    result = attribute_init(kNullAttributeSC256Init);
    attribute_set_name(result, "init");
    attribute_set_description(result, "Use to initialize the SC256. Clears contents of the TCAM.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_setvalue_routine(result, sc256_init_set_value);
    return result;
}

static void
sc256_init_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        sc256_72bit_data_t reg_val;
        sc256_state_t* state;
        ComponentPtr sc256;
        DagCardPtr card = attribute_get_card(attribute);

        
        sc256 = attribute_get_component(attribute);
        // Reset the CAM
        state = component_get_private_state(sc256);
        card_write_iom(card, state->mBase + SC256_CSR2, 0x80000000);
        dagutil_nanosleep(640); /* can be dropped down to 320ns on RevB cards */

        // Blank the Data space

        card_write_iom(card, state->mBase + SC256_VAL_ARRAY, 0);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 4, 0);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 8, 0);

        card_write_iom(card, state->mBase + SC256_CSR0,0xffff);
        card_write_iom(card, state->mBase + SC256_CSR1,0x10000000);
        card_write_iom(card, state->mBase + SC256_CSR2,0x2);
        
        while (card_read_iom(card, state->mBase + SC256_CSR2) & 0x2);

        // Blank the Mask space

        card_write_iom(card, state->mBase + SC256_VAL_ARRAY, 0xffffffff);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 4, 0xffffffff);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 8, 0xff);

        card_write_iom(card, state->mBase + SC256_CSR0,0xffff);
        card_write_iom(card, state->mBase + SC256_CSR1,0x10100000);
        card_write_iom(card, state->mBase + SC256_CSR2,0x2);
        
        while (card_read_iom(card, state->mBase + SC256_CSR2) & 0x2);

        // Init LAR for 144 bit searches
        reg_val.data0 = 0xffff0000;
        reg_val.data1 = 0;
        reg_val.data2 = 0;
        sc256_write_cam_reg(sc256, LAR, &reg_val);

        // Init SSR0
        reg_val.data0 = 0xffffffff;
        reg_val.data1 = 0xffffffff;
        reg_val.data2 = 0xff;
        sc256_write_cam_reg(sc256, SSR0,&reg_val);

        // Init SSR1
        reg_val.data0 = 0xffffffff;
        reg_val.data1 = 0xffffffff;
        reg_val.data2 = 0xff;
        sc256_write_cam_reg(sc256, SSR1,&reg_val);

        //Init SCR
        reg_val.data2 = 0;
        reg_val.data1 = 0;
        reg_val.data0 = 0x81;
        sc256_write_cam_reg(sc256, SCR, &reg_val);
    }
}

static AttributePtr
sc256_mask_address(void)
{
    AttributePtr result = NULL;
    uint32_t* state = NULL;

    result = attribute_init(kUint32AttributeSC256MaskAddress);
    attribute_set_name(result, "mask_address");
    attribute_set_description(result, "Use to set the address of the mask to read/write from/to.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, sc256_mask_address_get_value);
    attribute_set_setvalue_routine(result, sc256_mask_address_set_value);
    state = malloc(sizeof(uint32_t));
    *state = 0;
    attribute_set_private_state(result, state);
    return result;
}

static void
sc256_mask_address_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t* address = (uint32_t*)attribute_get_private_state(attribute);
        *address = *(uint32_t*)value;
    }
}

static void*
sc256_mask_address_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return (void*)attribute_get_private_state(attribute);
    }
    return NULL;
}

static AttributePtr
sc256_data_address(void)
{
    AttributePtr result = NULL;
    uint32_t* state = NULL;

    result = attribute_init(kUint32AttributeSC256DataAddress);
    attribute_set_name(result, "data_address");
    attribute_set_description(result, "Use to set the address of the data to read/write from/to.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, sc256_data_address_get_value);
    attribute_set_setvalue_routine(result, sc256_data_address_set_value);
    state = malloc(sizeof(uint32_t));
    *state = 0;
    attribute_set_private_state(result, state);
    return result;
}

static void
sc256_data_address_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t* address = (uint32_t*)attribute_get_private_state(attribute);
        *address = *(uint32_t*)value;
    }
}

static void*
sc256_data_address_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return (void*)attribute_get_private_state(attribute);
    }
    return NULL;
}

static AttributePtr
sc256_144bit_search(void)
{
    AttributePtr result = NULL;
    sc256_144bit_search_data_t* data = NULL;

    result = attribute_init(kStructAttributeSC256144BitSearch);
    attribute_set_name(result, "search_144bit");
    attribute_set_description(result, "Use to search for a 144 bit term.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeStruct);
    attribute_set_getvalue_routine(result, sc256_144bit_search_get_value);
    attribute_set_setvalue_routine(result, sc256_144bit_search_set_value);
    attribute_set_to_string_routine(result, sc256_144bit_search_to_string);
    attribute_set_from_string_routine(result, sc256_144bit_search_from_string);
    data = (sc256_144bit_search_data_t*)malloc(sizeof(sc256_144bit_search_data_t));
    memset(data, 0, sizeof(sc256_144bit_search_data_t));
    attribute_set_private_state(result, (void*)data);
    return result;
}

/* Used to set the the search term */
static void
sc256_144bit_search_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        sc256_144bit_search_data_t* sc256_data = (sc256_144bit_search_data_t*)value;
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        DagCardPtr card = attribute_get_card(attribute);
        int i = 0;
        sc256_72bit_data_t lar_val;
        
        /* write to the LAR to tell it we want to do a 144 bit search */
        lar_val.data0 = 0xffff0000;
        lar_val.data1 = 0x0;
        lar_val.data2 = 0x0;
	    sc256_write_cam_reg(sc256, LAR, &lar_val);

        for (i = 0; i < 5; i++)
        {
            card_write_iom(card, state->mBase + SC256_VAL_ARRAY + (4*i), sc256_data->data[i]);
        }

        card_write_iom(card, state->mBase + SC256_CSR0, ((sc256_data->ssel << 26) | (sc256_data->cmpr << 23) | (sc256_data->rslt << 20)));
        card_write_iom(card, state->mBase + SC256_CSR1, ((2 << 28) | (sc256_data->gmsk << 24)));
        card_write_iom(card, state->mBase + SC256_CSR2, 0x1); //Set action bit.
    }
}

static void*
sc256_144bit_search_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        DagCardPtr card = attribute_get_card(attribute);
        sc256_144bit_search_data_t* data = attribute_get_private_state(attribute);
        uint32_t reg_val0;
        uint32_t reg_val1;
        uint32_t reg_val2;

        reg_val0 = card_read_iom(card, state->mBase + SC256_RES_ARRAY);
        reg_val1 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 4);
        reg_val2 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 8);

        dagutil_verbose_level(4, "Contents of result registers: %08x %08x %08x\n", reg_val2, reg_val1, reg_val0);

        data->found = (reg_val2 & BIT8) == BIT8;
        data->index = reg_val0 & 0xfffff;
        return (void*)data;
    }
    return NULL;
}

static void 
sc256_144bit_search_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
sc256_144bit_search_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
sc256_72bit_search(void)
{
    AttributePtr result = NULL;
    sc256_72bit_search_data_t* data = NULL;

    result = attribute_init(kStructAttributeSC25672BitSearch);
    attribute_set_name(result, "search_72bit");
    attribute_set_description(result, "Use to search for a 72 bit term.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeStruct);
    attribute_set_getvalue_routine(result, sc256_72bit_search_get_value);
    attribute_set_setvalue_routine(result, sc256_72bit_search_set_value);
    attribute_set_to_string_routine(result, sc256_72bit_search_to_string);
    attribute_set_from_string_routine(result, sc256_72bit_search_from_string);
    data = (sc256_72bit_search_data_t*)malloc(sizeof(sc256_72bit_search_data_t));
    memset(data, 0, sizeof(*data));
    attribute_set_private_state(result, (void*)data);
    return result;
}

/* Used to set the the search term */
static void
sc256_72bit_search_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        sc256_72bit_search_data_t* sc256_data = (sc256_72bit_search_data_t*)value;
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        DagCardPtr card = attribute_get_card(attribute);
        sc256_72bit_data_t lar_val;
        
        /* write to the LAR to tell it we want to do a 144 bit search */
        lar_val.data0 = 0x0000ffff;
        lar_val.data1 = 0x0;
        lar_val.data2 = 0x0;
        sc256_write_cam_reg(sc256, LAR, &lar_val);

        card_write_iom(card, state->mBase + SC256_VAL_ARRAY, sc256_data->data0);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 4, sc256_data->data1);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 8, sc256_data->data2);

        card_write_iom(card, state->mBase + SC256_CSR0, ((sc256_data->ssel << 26) | (sc256_data->cmpr << 23) | (sc256_data->rslt << 20)));
        card_write_iom(card, state->mBase + SC256_CSR1, ((2 << 28) | (sc256_data->gmsk << 24)));
        card_write_iom(card, state->mBase + SC256_CSR2, 0x1); //Set action bit.
    }
}

static void*
sc256_72bit_search_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        DagCardPtr card = attribute_get_card(attribute);
        sc256_72bit_search_data_t* data = attribute_get_private_state(attribute);
        uint32_t reg_val0;
        uint32_t reg_val1;
        uint32_t reg_val2;

        reg_val0 = card_read_iom(card, state->mBase + SC256_RES_ARRAY);
        reg_val1 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 4);
        reg_val2 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 8);

        dagutil_verbose_level(4, "sc256_72bit_search_get_value: %08x %08x %08x\n", reg_val2, reg_val1, reg_val0);

        data->found = (reg_val2 & BIT8) == BIT8;
        data->index = reg_val0 & 0xfffff;
        return (void*)data;
    }
    return NULL;
}

static void 
sc256_72bit_search_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
sc256_72bit_search_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
sc256_72bit_data(void)
{
    AttributePtr result = NULL;
    sc256_72bit_data_t* data = NULL;

    result = attribute_init(kStructAttributeSC25672BitData);
    attribute_set_name(result, "data_72bit");
    attribute_set_description(result, "Use to read/write data to the SC256.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeStruct);
    attribute_set_getvalue_routine(result, sc256_72bit_get_data);
    attribute_set_setvalue_routine(result, sc256_72bit_set_data);
    attribute_set_to_string_routine(result, sc256_72bit_data_to_string);
    attribute_set_from_string_routine(result, sc256_72bit_data_from_string);
    data = (sc256_72bit_data_t*)malloc(sizeof(sc256_72bit_data_t));
    attribute_set_private_state(result, (void*)data);
    return result;
}

static void
sc256_72bit_set_data(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        sc256_72bit_data_t* sc256_data = (sc256_72bit_data_t*)value;
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        AttributePtr addr_attr;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        uint8_t inst = 1, acc = 0;

        addr_attr = component_get_attribute(sc256, kUint32AttributeSC256DataAddress);
        address = *(uint32_t*)attribute_get_value(addr_attr);
        dagutil_verbose_level(4, "sc256_72bit_set_data: Writing 72 bit data 0x%02x 0x%08x 0x%08x\n", sc256_data->data0, sc256_data->data1, sc256_data->data2);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY, sc256_data->data0);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 4, sc256_data->data1);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 8, sc256_data->data2);
        card_write_iom(card, state->mBase + SC256_CSR0, 0);
        card_write_iom(card, state->mBase + SC256_CSR1, ((inst << 28) | (acc << 20) | address));
        card_write_iom(card, state->mBase + SC256_CSR2, 1);
    }
}

static void*
sc256_72bit_get_data(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        DagCardPtr card = attribute_get_card(attribute);
        sc256_72bit_data_t* data = attribute_get_private_state(attribute);
        uint8_t inst = 0, acc = 0;
        AttributePtr addr_attr;
        uint32_t address = 0;

        addr_attr = component_get_attribute(sc256, kUint32AttributeSC256DataAddress);
        address = *(uint32_t*)attribute_get_value(addr_attr);
        card_write_iom(card, state->mBase + SC256_CSR0, 0);
        card_write_iom(card, state->mBase + SC256_CSR1, ((inst << 28) | (acc << 20) | address));
        card_write_iom(card, state->mBase + SC256_CSR2, 1);
        data->data2 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 8) & 0xff;
        data->data1 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 4);
        data->data0 = card_read_iom(card, state->mBase + SC256_RES_ARRAY);
        dagutil_verbose_level(4, "%02x %08x %08x\n", data->data2, data->data1, data->data0);
        return (void*)data;
    }
    return NULL;
}


static void 
sc256_72bit_data_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
sc256_72bit_data_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
sc256_72bit_mask(void)
{
    AttributePtr result = NULL;
    sc256_72bit_mask_t* mask = NULL;

    result = attribute_init(kStructAttributeSC25672BitMask);
    attribute_set_name(result, "mask_72bit");
    attribute_set_description(result, "Use to read/write mask to the SC256.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeStruct);
    attribute_set_getvalue_routine(result, sc256_72bit_get_mask);
    attribute_set_setvalue_routine(result, sc256_72bit_set_mask);
    attribute_set_to_string_routine(result, sc256_72bit_mask_to_string);
    attribute_set_from_string_routine(result, sc256_72bit_mask_from_string);
    mask = (sc256_72bit_mask_t*)malloc(sizeof(sc256_72bit_mask_t));
    attribute_set_private_state(result, (void*)mask);
    return result;
}

static void
sc256_72bit_set_mask(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        sc256_72bit_mask_t* sc256_mask = (sc256_72bit_mask_t*)value;
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        AttributePtr addr_attr;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        uint8_t inst = 1, acc = 1;

        addr_attr = component_get_attribute(sc256, kUint32AttributeSC256MaskAddress);
        address = *(uint32_t*)attribute_get_value(addr_attr);
        dagutil_verbose_level(4, "sc256_72bit_set_mask: Writing 72 bit mask 0x%02x 0x%08x 0x%08x\n", sc256_mask->mask0, sc256_mask->mask1, sc256_mask->mask2);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY, sc256_mask->mask0);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 4, sc256_mask->mask1);
        card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 8, sc256_mask->mask2);
        card_write_iom(card, state->mBase + SC256_CSR0, 0);
        card_write_iom(card, state->mBase + SC256_CSR1, ((inst << 28) | (acc << 20) | address));
        card_write_iom(card, state->mBase + SC256_CSR2, 1);
    }
}

static void*
sc256_72bit_get_mask(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_state_t* state = component_get_private_state(sc256);
        DagCardPtr card = attribute_get_card(attribute);
        sc256_72bit_mask_t* mask = attribute_get_private_state(attribute);
        uint8_t inst = 0, acc = 1;
        AttributePtr addr_attr;
        uint32_t address = 0;

        addr_attr = component_get_attribute(sc256, kUint32AttributeSC256MaskAddress);
        address = *(uint32_t*)attribute_get_value(addr_attr);
        card_write_iom(card, state->mBase + SC256_CSR0, 0);
        card_write_iom(card, state->mBase + SC256_CSR1, ((inst << 28) | (acc << 20) | address));
        card_write_iom(card, state->mBase + SC256_CSR2, 1);
        mask->mask2 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 8) & 0xff;
        mask->mask1 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 4);
        mask->mask0 = card_read_iom(card, state->mBase + SC256_RES_ARRAY);
        dagutil_verbose_level(4, "%02x %08x %08x\n", mask->mask2, mask->mask1, mask->mask0);
        return (void*)mask;
    }
    return NULL;
}

static void 
sc256_72bit_mask_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
sc256_72bit_mask_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
sc256_search_length(void)
{
    AttributePtr result = NULL;

    result = attribute_init(kUint32AttributeSC256SearchLength);
    attribute_set_name(result, "search_length");
    attribute_set_description(result, "Use to set the number of bits to use when searching.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, sc256_search_length_get_value);
    attribute_set_setvalue_routine(result, sc256_search_length_set_value);
    return result;
}

static void
sc256_search_length_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        ComponentPtr sc256 = attribute_get_component(attribute);
        sc256_search_length_t sl = *(sc256_search_length_t*)value;
        sc256_72bit_data_t lar_val;

        if (sl == kSC256SearchLength72)
        {
            lar_val.data0 = 0x0000ffff;
            lar_val.data1 = 0;
            lar_val.data2 = 0;
            sc256_write_cam_reg(sc256, LAR, &lar_val);
        }
        else if (sl == kSC256SearchLength144)
        {
            lar_val.data0 = 0xffff0000;
            lar_val.data1 = 0;
            lar_val.data2 = 0;
            sc256_write_cam_reg(sc256, LAR, &lar_val);
        }
    }
}

static void*
sc256_search_length_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
    return NULL;
}

static int
sc256_write_cam_reg(ComponentPtr comp, uint32_t address, const sc256_72bit_data_t* data)
{
    uint8_t inst = 1, acc = 3;
    DagCardPtr card = component_get_card(comp);
    sc256_state_t* state = component_get_private_state(comp);

    /*
    *(copro_regs + SC256_VAL_ARRAY) = reg_val.data0;
    *(copro_regs + SC256_VAL_ARRAY + 1) = reg_val.data1;
    *(copro_regs + SC256_VAL_ARRAY + 2) = reg_val.data2;
    */
    card_write_iom(card, state->mBase + SC256_VAL_ARRAY, data->data0);
    card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 4, data->data1);
    card_write_iom(card, state->mBase + SC256_VAL_ARRAY + 8, data->data2);

    /*
    *(copro_regs + SC256_CSR0) = 0;
    *(copro_regs + SC256_CSR1) = (((inst & 0x7) << 28 ) | ((acc & 0x3) << 20) | (addr & 0xfffff)); 
    *(copro_regs + SC256_CSR2) = 0x1; // Set 'action' bit
    */
    card_write_iom(card, state->mBase + SC256_CSR0, 0);
    card_write_iom(card, state->mBase + SC256_CSR1, (((inst & 0x7) << 28 ) | ((acc & 0x3) << 20) | (address & 0xfffff)));
    card_write_iom(card, state->mBase + SC256_CSR2, 1);

    dagutil_nanosleep(180); /* can be dropped down to 90ns on RevB cards */
    dagutil_verbose_level(4, "Contents written to the value array: %02x %08x %08x\n",data->data2,data->data1,data->data0);
    dagutil_verbose_level(4, "Contents written to CSR0 0x00000000 CSR1 %08x CSR2 0x1\n",(((inst & 0x7) << 28 ) | ((acc & 0x3) << 20) | (address & 0xfffff)));
    return 1;
}

#if 0
static int
sc256_read_cam_reg(ComponentPtr comp, sc256_72bit_data_t* data)
{
    uint8_t inst = 0, acc = 3;
    int timeout = 1023;
    sc256_state_t* state = NULL;
    DagCardPtr card = component_get_card(comp);
    
    state = component_get_private_state(comp);
    if (!state)
        return -1;
    /*
    *(copro_regs + SC256_CSR0) = 0;
    *(copro_regs + SC256_CSR1) = (((inst & 0x7) << 28 ) | ((acc & 0x3) << 20) | (data->address & 0xfffff)); 
    *(copro_regs + SC256_CSR2) = 0x1; // Set 'action' bit
    */
    card_write_iom(card, state->mBase + SC256_CSR0, 0);
    card_write_iom(card, state->mBase + SC256_CSR1, (((inst & 0x7) << 28 ) | ((acc & 0x3) << 20) | (data->address & 0xfffff)));
    card_write_iom(card, state->mBase + SC256_CSR2, 1);

    /*
    while ((timeout > 0) && ( *(copro_regs + SC256_CSR2) & 0x000c0000)== 0x0)
    */
    while ((timeout > 0) && (card_read_iom(card, state->mBase + SC256_CSR2) & 0x000c0000) == 0x0)
    {
        dagutil_nanosleep(110);
        timeout--;
    }

    dagutil_verbose_level(4, "Contents written to the CSR0: 0 CSR1: %08x CSR2: 0x00000001\n",(((inst & 0x7) << 28 ) | ((acc & 0x3) << 20) | (data->address & 0xfffff)));

/*
    data->data0 = *(copro_regs + SC256_RES_ARRAY);
    data->data1 = *(copro_regs + SC256_RES_ARRAY + 1);
    data->data2 = (*(copro_regs + SC256_RES_ARRAY + 2) & 0xff);
    */
    data->data0 = card_read_iom(card, state->mBase + SC256_RES_ARRAY);
    data->data1 = card_read_iom(card, state->mBase + SC256_RES_ARRAY + 1);
    data->data2 = card_read_iom(card, (state->mBase + SC256_RES_ARRAY + 2) & 0xff);
}
#endif

