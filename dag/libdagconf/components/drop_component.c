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
#include "../include/components/gpp_component.h"
#include "../include/attribute_factory.h"

/* DAG API files */
#include "dagutil.h"

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

#define BUFFER_SIZE 1024

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} drop_state_t;

/* gpp component. */
static void drop_dispose(ComponentPtr component);
static int drop_post_initialize(ComponentPtr component);

ComponentPtr
drop_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentDrop, card); 
    char buffer[BUFFER_SIZE];
    drop_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, drop_dispose);
        component_set_post_initialize_routine(result, drop_post_initialize);
        sprintf(buffer, "drop%d", index);
        component_set_name(result, buffer);
        state = (drop_state_t*)malloc(sizeof(drop_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
drop_dispose(ComponentPtr component)
{
}

static int
drop_post_initialize(ComponentPtr component)
{
    AttributePtr attr;
    drop_state_t* state = NULL;
    DagCardPtr card = NULL;
    uint32_t mask;
    GenericReadWritePtr grw = NULL;

    state = component_get_private_state(component);
    card = component_get_card(component);
    /* The setting of the base address for this component (and all others) has to happen in
     * the post_initialize function because the card's register address information
     * is needed. This information is set after the card is initialized (see dag_config_init)
     */
    
    state->mBase = card_get_register_address(card, DAG_REG_DROP, state->mIndex);

    mask = 0xffffffff;
    grw = grw_init(card, (uintptr_t)card_get_iom_address(card) + state->mBase, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeStreamDropCount, grw, &mask, 1);
    component_add_attribute(component, attr);

    return 1;
}


