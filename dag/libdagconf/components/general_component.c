/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: general_component.c,v 1.1 2006/12/20 04:49:11 patricia Exp $
 */

#include "dagutil.h"

#include "../include/attribute.h"
#include "../include/util/set.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/gpp_component.h"
#include "../include/components/dag71s_port_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"

#define OFFSET 0x1c

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: general_component.c,v 1.1 2006/12/20 04:49:11 patricia Exp $";
static const char* const kRevisionString = "$Revision: 1.1 $";
static void general_dispose(ComponentPtr component);
static void general_reset(ComponentPtr component);
static void general_default(ComponentPtr component);
static int general_post_initialize(ComponentPtr component);
static dag_err_t general_update_register_base(ComponentPtr component);


/**
 * General Attribute definition array 
 
Attribute_t general_attr[]=
{
    
};
*/
/* Number of elements in array */
//#define NB_ELEM (sizeof(general_attr) / sizeof(Attribute_t))

typedef struct
{
    uint32_t mBase;
    uint32_t mIndex;
} general_state_t;

/* general component. */
ComponentPtr
general_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentGeneral, card); 
    general_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, general_dispose);
        component_set_post_initialize_routine(result, general_post_initialize);
        component_set_reset_routine(result, general_reset);
        component_set_default_routine(result, general_default);
        component_set_update_register_base_routine(result, general_update_register_base);
        component_set_name(result, "general_register");
        state = (general_state_t*)malloc(sizeof(general_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
general_dispose(ComponentPtr component)
{
}

static dag_err_t
general_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        general_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_GENERAL, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
general_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        general_state_t* state = NULL;
	
        state = component_get_private_state(component);
        /* Add attribute of general */ 
        //read_attr_array(component, general_attr, NB_ELEM, state->mIndex);
        
		return 1;
    }
    return kDagErrInvalidParameter;
}

static void
general_reset(ComponentPtr component)
{   

}


static void
general_default(ComponentPtr component)
{

}

