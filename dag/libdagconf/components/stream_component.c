/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#include "dagutil.h"
/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/components/stream_component.h"
#include "../include/attribute_factory.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* POSIX headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif

typedef struct
{
    uint32_t mStreamIdx;
} stream_state_t;

/* stream component. */
static void stream_dispose(ComponentPtr component);
static void stream_reset(ComponentPtr component);
static void stream_default(ComponentPtr component);
static int stream_post_initialize(ComponentPtr component);

/* mem */
static void mem_dispose(AttributePtr attribute);
static void* mem_get_value(AttributePtr attribute);
static void mem_set_value(AttributePtr attribute, void* value, int length);
static void mem_post_initialize(AttributePtr attribute);

/* mem_bytes */
static void mem_bytes_dispose(AttributePtr attribute);
static void* mem_bytes_get_value(AttributePtr attribute);
static void mem_bytes_set_value(AttributePtr attribute, void* value, int length);
static void mem_bytes_post_initialize(AttributePtr attribute);

/* Sanity checks for buffer allocation */
uint32_t get_buffer_size(ComponentPtr root);
uint32_t get_already_allocated_buffer_size(ComponentPtr root, int streamidx);
#define BUFFER_SIZE 1024

/* pci_bus_speed */
AttributePtr
get_new_mem_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMem); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, mem_dispose);
        attribute_set_post_initialize_routine(result, mem_post_initialize);
        attribute_set_getvalue_routine(result, mem_get_value);
        attribute_set_setvalue_routine(result, mem_set_value);
        attribute_set_name(result, "mem");
        attribute_set_description(result, "The memory allocated to the stream in mebibytes.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
mem_dispose(AttributePtr attribute)
{

}

static void*
mem_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr component = NULL;
    stream_state_t* state = NULL;
    int memory = 0;
    uint32_t val = 0;

    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        card = attribute_get_card(attribute);
        state = (stream_state_t*)component_get_private_state(component);
        memory = card_pbm_get_stream_mem_size(card, state->mStreamIdx);
        if (memory < 0)
            val = 0;
        else
            val = memory;
        /* Convert to mebibytes */
        val /= 1024*1024;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
mem_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr component = NULL;
    ComponentPtr root = NULL;
    uint32_t size = *(uint32_t*)value;
    stream_state_t* state = NULL;
    uint32_t total_buffer_size = 0;
    uint32_t current_allocated_size = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        root = card_get_root_component(card);
        component = attribute_get_component(attribute);
        state = (stream_state_t*)component_get_private_state(component);
        /* Check to see what the sum of the other streams are first */
        total_buffer_size = get_buffer_size(root);
        current_allocated_size = get_already_allocated_buffer_size(root, state->mStreamIdx);
        if( (size + current_allocated_size) > total_buffer_size)
        {
            dagutil_warning("More memory requested (%dMiB) than available (%dMiB)!\n", size+current_allocated_size, total_buffer_size);
            return;
        }

        /* Convert to bytes */
        size *= 1024*1024;
        card_pbm_config_stream(card, state->mStreamIdx, size);
    }
}

static void
mem_post_initialize(AttributePtr attribute)
{

}



ComponentPtr
get_new_stream(DagCardPtr card, uint32_t idx)
{
    ComponentPtr result = component_init(kComponentStream, card);
    stream_state_t* state = NULL;
    char buffer[BUFFER_SIZE];
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, stream_dispose);
        component_set_post_initialize_routine(result, stream_post_initialize);
        component_set_reset_routine(result, stream_reset);
        component_set_default_routine(result, stream_default);
        state = (stream_state_t*)malloc(sizeof(stream_state_t));
        memset(state, 0, sizeof(stream_state_t));
        state->mStreamIdx = idx;
        component_set_private_state(result, state);
        sprintf(buffer, "stream%u", idx);
        component_set_name(result, buffer);
    }
    
    return result;
}


static void
stream_dispose(ComponentPtr component)
{
}

static int
stream_post_initialize(ComponentPtr component)
{
    AttributePtr mem;
    AttributePtr mem_bytes;
    DagCardPtr card;
    uintptr_t pbm_base;
    uintptr_t address;
    uint32_t pbm_ver;
    stream_state_t* state = component_get_private_state(component);
    pbm_offsets_t* PBM = NULL;
    GenericReadWritePtr grw = NULL;
    AttributePtr attr = NULL;
    uint32_t masks = 0xffffffff;

    assert(state);
    PBM = dagutil_get_pbm_offsets();
    mem = get_new_mem_attribute();
    component_add_attribute(component, mem);
    mem_bytes = get_new_mem_bytes_attribute();
    card = component_get_card(component);
    component_add_attribute(component, mem_bytes);

    pbm_base = card_get_register_address(card, DAG_REG_PBM, 0);
    pbm_ver = card_get_register_version(card, DAG_REG_PBM, 0);

    address = (uintptr_t)card_get_iom_address(card) + pbm_base + PBM[pbm_ver].streambase + (state->mStreamIdx * PBM[pbm_ver].streamsize) + (uintptr_t)PBM[pbm_ver].record_ptr;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeRecordPointer, grw, &masks, 1);
    component_add_attribute(component, attr);

    address = (uintptr_t)card_get_iom_address(card) + pbm_base + PBM[pbm_ver].streambase + (state->mStreamIdx * PBM[pbm_ver].streamsize) + (uintptr_t)PBM[pbm_ver].limit_ptr;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeLimitPointer, grw, &masks, 1);
    component_add_attribute(component, attr);

    if (card_get_register_address(card, DAG_REG_HLB, 0))
    {
        /* don't create the hlb_component from hlb_component.c - it's awkward to use - create a new
         * component, set the code the kComponentHlb and bind the stream max and stream min range attributes
         * to it. Then append this new hlb component as a subcomponent of the stream component
         * The grw_hlb_* functions handle the setting/getting of the min/max range values 
         * by using the original hlb component. We only create them for the receive streams, which are at even indices
         */
        if (state->mStreamIdx % 2 == 0)
        {
            ComponentPtr hlb = component_init(kComponentHlb, card);
            component_set_name(hlb, "hlb");
            component_set_description(hlb, "Hash Load Balancer");
           
            /* HLB stream index refers only to rx streams i.e.
            in the underlying hlb components world stream 0 is receive stream 0, stream 1 is receive stream 2
            */
            address = state->mStreamIdx/2;
            grw = grw_init(card, address, grw_hlb_component_get_min_range, grw_hlb_component_set_min_range);
            attr = attribute_factory_make_attribute(kUint32AttributeHLBRangeMin, grw, &masks, 1);
            component_add_attribute(hlb, attr);

            grw = grw_init(card, address, grw_hlb_component_get_max_range, grw_hlb_component_set_max_range);
            attr = attribute_factory_make_attribute(kUint32AttributeHLBRangeMax, grw, &masks, 1);
            component_add_attribute(hlb, attr);

            component_add_subcomponent(component, hlb);
        }
    }
    return 1;
}


static void
stream_reset(ComponentPtr component)
{
}


static void
stream_default(ComponentPtr component)
{
}

AttributePtr
get_new_mem_bytes_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMemBytes); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, mem_bytes_dispose);
        attribute_set_post_initialize_routine(result, mem_bytes_post_initialize);
        attribute_set_getvalue_routine(result, mem_bytes_get_value);
        attribute_set_setvalue_routine(result, mem_bytes_set_value);
        attribute_set_name(result, "mem_bytes");
        attribute_set_description(result, "The memory allocated to the stream in bytes.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
mem_bytes_dispose(AttributePtr attribute)
{

}

static void*
mem_bytes_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr component = NULL;
    stream_state_t* state = NULL;
    int memory;
    uint32_t val = 0;

    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        card = attribute_get_card(attribute);
        state = (stream_state_t*)component_get_private_state(component);
        memory = card_pbm_get_stream_mem_size(card, state->mStreamIdx);
        if (memory < 0)
        {
            val = 0;
        }
        else
        {
            val = memory;
        }
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
mem_bytes_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr component = NULL;
    uint32_t size = *(uint32_t*)value;
    stream_state_t* state = NULL;
    /* make sure the size is 4Kbytes aligned */
    if ( size % (4 * 1024 ) )
    {
        size -= (size % (4 * 1024 ) );
        /* make sure size does not go negative */
        size = dagutil_max( 0, size );
        dagutil_warning("mem_bytes set value is not 4K aligned.Making it as %u \n",size);
    }
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        card = attribute_get_card(attribute);
        state = (stream_state_t*)component_get_private_state(component);
        card_pbm_config_stream(card, state->mStreamIdx, size);
    }
}

static void
mem_bytes_post_initialize(AttributePtr attribute)
{

}
uint32_t
get_buffer_size(ComponentPtr root)
{
   ComponentPtr pbm = NULL;
   AttributePtr bufsize = NULL;
   pbm = component_get_subcomponent(root, kComponentPbm, 0);
   assert(pbm != NULL);
   bufsize = component_get_attribute(pbm, kUint32AttributeBufferSize);
   assert(bufsize != NULL);
   return *(uint32_t *)attribute_get_value(bufsize);
}
uint32_t
get_already_allocated_buffer_size(ComponentPtr root, int streamidx)
{
    ComponentPtr stream = NULL;
    AttributePtr mem = NULL;
    int x = 0;
    uint32_t allocated_size = 0;
    for(x = 0; x < component_get_subcomponent_count_of_type(root, kComponentStream); x++)
    {
        if(x != streamidx)
        {
            stream = component_get_indexed_subcomponent_of_type(root, kComponentStream, x);
            assert(stream != NULL);
            mem = component_get_attribute(stream, kUint32AttributeMem);
            allocated_size += *(uint32_t *)attribute_get_value(mem);
        }
    }
    return allocated_size;
}
