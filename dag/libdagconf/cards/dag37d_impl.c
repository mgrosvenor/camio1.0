/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* File Header */
#include "../include/cards/dag37d_impl.h"
/* Public API headers. */

/* Internal project headers. */

/* DAG API files we rely on being installed in a sys include path somewhere */
#include "dagutil.h"
#include "dag_platform.h"
#include "dagapi.h"


#include "../include/attribute_types.h"
#include "../include/card.h"
#include "../include/component.h"

#include "../include/components/dag37d_port_component.h"
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/components/gpp_component.h"
#include "../include/cards/dag3_constants.h"
#include "../include/components/duck_component.h"
#include "../include/components/sc256.h"
#include "../include/components/card_info_component.h"
#include "../include/components/pattern_match_component.h"



/* Virtual methods for the card. */
static void dag37d_dispose(DagCardPtr card);
static int dag37d_post_initialize(DagCardPtr card);
static void dag37d_reset(DagCardPtr card);
static void dag37d_default(DagCardPtr card);

static void
dag37d_dispose(DagCardPtr card)
{
	ComponentPtr component;
	ComponentPtr root;
	AttributePtr attribute;
	int component_count = 0;
	int i = 0;
    dag37d_state_t * state = NULL;
	
	root = card_get_root_component(card);
	component_count = component_get_subcomponent_count(root);
	for (i = 0; i < component_count; i++)
	{
		component = component_get_indexed_subcomponent(root, i);
		while (component_has_attributes(component) == 1)
		{
			attribute = component_get_indexed_attribute(component, 0);
			component_dispose_attribute(component, attribute);
		}
		component_dispose(component);
	}
    /* Deallocate private state. */
    state = (dag37d_state_t*) card_get_private_state(card);
    
    assert(NULL != state);

    /* Free the stream first */
    free(state->mStream);
    
    free(state);
    card_set_private_state(card, NULL);
    
}

static int
dag37d_post_initialize(DagCardPtr card)
{
	dag37d_state_t* state = (dag37d_state_t*)card_get_private_state(card);
	ComponentPtr root = card_get_root_component(card);
    int rx_streams = 0;
    int tx_streams = 0;
    int streammax = 0;
    int dagfd = 0;
    int x = 0;

	dagutil_verbose("Loading components...\n");
	/* Add components as appropriate. */
	state->mPbm = pbm_get_new_pbm(card, 0);
    /* Find out the number of streams present */
    dagfd = card_get_fd(card);
    /* Get the number of RX Streams */
    rx_streams = dag_rx_get_stream_count(dagfd);
    tx_streams = dag_tx_get_stream_count(dagfd);
    streammax = dagutil_max(rx_streams*2 - 1, tx_streams*2);
    state->mStream = malloc(sizeof(state->mStream)*streammax);
    assert(state->mStream != NULL);
    for ( x = 0;x < streammax; x++)
    {
        state->mStream[x] = get_new_stream(card, x);
        component_add_subcomponent(root, state->mStream[x]);
    }
    if (card_get_register_address(card, DAG_REG_IDT_TCAM, 0))
    {
        state->mSC256 = sc256_get_new_component(card);
        component_add_subcomponent(root, state->mSC256);
    }    

    state->mPhyBase = card_get_register_address(card, DAG_REG_RAW_MAXDS3182, 0);
    state->mGppBase = card_get_register_address(card, DAG_REG_GPP, 0);
    state->mGeneralBase = card_get_register_address(card, DAG_REG_GENERAL, 0);

	state->mPort[0] = dag37d_get_new_port(card, 0);
	state->mPort[1] = dag37d_get_new_port(card, 1);

	component_add_subcomponent(root, state->mPbm);
	component_add_subcomponent(root, state->mPort[0]);
	component_add_subcomponent(root, state->mPort[1]);

    if (card_get_register_address(card, DAG_REG_PATTERN_MATCH, 0))
    {
	state->mPatternMatch[0] = pattern_match_get_new_component(card, 0);
	state->mPatternMatch[1] = pattern_match_get_new_component(card, 1);

	component_add_subcomponent(root, state->mPatternMatch[0]);
	component_add_subcomponent(root, state->mPatternMatch[1]);
    }

    state->mGpp = gpp_get_new_gpp(card, 0);
    component_add_subcomponent(root, state->mGpp);
    
    if (card_get_register_address(card, DAG_REG_DUCK, 0))
    {
        component_add_subcomponent(root, duck_get_new_component(card, 0));
    }

    /* Add card info component */
    component_add_subcomponent(root, card_info_get_new_component(card, 0));
	/* Return 1 if standard post_initialize() should continue, 0 if not.
	* "Standard" post_initialize() calls post_initialize() on the card's root component.
	*/
	return 1;
}


static void
dag37d_reset(DagCardPtr card)
{
	/*FIXME: there is no implemented reset for this card */

}


static void
dag37d_default(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		ComponentPtr root = card_get_root_component(card);
        dag37d_state_t * state = (dag37d_state_t *)card_get_private_state(card);
		ComponentPtr any_component = NULL;
		int i = 0;
		int port_count = 0;
        uint32_t val = 0;

		card_reset_datapath(card);
		/*card_pbm_default(card);*/
        
        val = 0;
        dag37d_iom_write(card, state->mPhyBase, D3182_GLOBAL_CONTROL_1, val);
        /* Set POS/PHY Mode */
        /* add 1 to address to get value can only reay 8bit values */ 
        val = BIT0 | BIT1 ; 
        dag37d_iom_write(card, state->mPhyBase, D3182_GLOBAL_CONTROL_1+1, val);

    	/* Configure the CLAD */
	    val = BIT2;
	    dag37d_iom_write(card, state->mPhyBase, D3182_GLOBAL_CONTROL_2, val);        

    	/* Select RXLOS and RXLOF for the GPIO pins */
	    val = BIT0 | BIT2 | BIT4 | BIT6 ;
	    dag37d_iom_write(card, state->mPhyBase, D3182_GLOBAL_GPIO_CONTROL, val);
        /* add 1 to address to get value can only reay 8bit values */
        val = BIT0|BIT2|BIT4;
	    dag37d_iom_write(card, state->mPhyBase, D3182_GLOBAL_GPIO_CONTROL+1, val);
        
		port_count = component_get_subcomponent_count_of_type(root, kComponentPort);
		for (i = 0; i < port_count; i++)
		{
            any_component = component_get_subcomponent(root, kComponentPort, i);
            component_default(any_component);
		}
		any_component = component_get_subcomponent(root, kComponentPbm, 0);
		component_default(any_component);
	}
}


/* Initialization routine. */
dag_err_t
dag37d_initialize(DagCardPtr card)
{
	dag37d_state_t* state;
	ComponentPtr root_component;

	assert(NULL != card);
	
	/* Set up virtual methods. */
	card_set_dispose_routine(card, dag37d_dispose);
	card_set_post_initialize_routine(card, dag37d_post_initialize);
	/*FIXME: there is no implemented reset for this card */
	card_set_reset_routine(card, dag37d_reset);
	card_set_default_routine(card, dag37d_default);

	/* Allocate private state. */
	state = (dag37d_state_t*) malloc(sizeof(dag37d_state_t));
	memset(state, 0, sizeof(*state));
	card_set_private_state(card, (void*) state);
	
	/* Create root component. */
	root_component = component_init(kComponentRoot, card);

	/* Add root component to card. */
	card_set_root_component(card, root_component);
	
	
	return kDagErrNone;
}
