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

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/components/terf_component.h"
#include "../include/components/mini_mac.h"
#include "../include/components/gpp_component.h"
#include "../include/components/lm93.h"
#include "../include/components/sr_gpp.h"
#include "../include/components/card_info_component.h"

/* DAG API files we rely on being installed in a sys include path somewhere */
#include "dagutil.h"
#include "dag_platform.h"
#include "dagapi.h"

typedef struct
{
	ComponentPtr mGpp[4];
	ComponentPtr mPbm;
	ComponentPtr *mStream;
	ComponentPtr mPort[4]; 
	ComponentPtr mTerf; 
	ComponentPtr mHardwareMonitor;
} dag70ge_state_t;


/* Virtual methods for the card. */
static void dag70ge_dispose(DagCardPtr card);
static int dag70ge_post_initialize(DagCardPtr card);
static void dag70ge_reset(DagCardPtr card);
static void dag70ge_default(DagCardPtr card);

static void
dag70ge_dispose(DagCardPtr card)
{
	ComponentPtr component;
	ComponentPtr root;
	AttributePtr attribute;
	int component_count = 0;
	int i = 0;
    dag70ge_state_t * state = NULL;
	
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
    state = (dag70ge_state_t*) card_get_private_state(card);
    
    assert(NULL != state);

    /* Free the stream first */
    free(state->mStream);
    
    free(state);
    card_set_private_state(card, NULL);
    
}

uint32_t
sr_gpp_count(DagCardPtr card)
{
	uint32_t address;
	uint32_t count;

	address = card_get_register_address(card, DAG_REG_SRGPP, 0);
	count = card_read_iom(card, address);
	return count && 0x70000000;
}

static int
dag70ge_post_initialize(DagCardPtr card)
{
	dag70ge_state_t* state = (dag70ge_state_t*)card_get_private_state(card);
	ComponentPtr root = card_get_root_component(card);
	uint32_t i = 0;
	uint32_t count = 0;
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

	state->mPort[0] = mini_mac_get_new_component(card, 0);
	state->mPort[1] = mini_mac_get_new_component(card, 1);
	state->mPort[2] = mini_mac_get_new_component(card, 2);
	state->mPort[3] = mini_mac_get_new_component(card, 3);
	state->mHardwareMonitor = lm93_get_new_component(card, 0);
	
	component_add_subcomponent(root, state->mPbm);
	component_add_subcomponent(root, state->mPort[0]);
	component_add_subcomponent(root, state->mPort[1]);
	component_add_subcomponent(root, state->mPort[2]);
	component_add_subcomponent(root, state->mPort[3]);
	component_add_subcomponent(root, state->mHardwareMonitor);

	count = sr_gpp_count(card);
	dagutil_verbose_level(2, "Found %u sr gpps\n", count);
	for (i = 0; i < count; i++)
	{
		state->mGpp[i] = sr_gpp_get_new_component(card, i);
		component_add_subcomponent(root, state->mGpp[i]);
	}

	if (card_get_register_address(card, DAG_REG_TERF64, 0))
	{
		state->mTerf = terf_get_new_terf(card, 0);
		component_add_subcomponent(root, state->mTerf);
	}
    /* Add card info component */
    component_add_subcomponent(root, card_info_get_new_component(card, 0));
	/* Return 1 if standard post_initialize() should continue, 0 if not.
	* "Standard" post_initialize() calls post_initialize() on the card's root component.
	*/
	return 1;
}


static void
dag70ge_reset(DagCardPtr card)
{

}


static void
dag70ge_default(DagCardPtr card)
{
	if (1 == valid_card(card))
	{
		ComponentPtr root = card_get_root_component(card);
		ComponentPtr any_component = NULL;
		AttributePtr any_attribute = NULL;
		int i = 0;
		int port_count = 0;
		uint8_t bool_val = 0;
		void* temp = NULL;

		card_reset_datapath(card);
		/*card_pbm_default(card);*/

		port_count = component_get_subcomponent_count_of_type(root, kComponentPort);
		for (i = 0; i < port_count; i++)
		{
			/*turn off eql*/
			bool_val = 0;
			any_component = component_get_subcomponent(root, kComponentPort, i);
			any_attribute = component_get_attribute(any_component, kBooleanAttributeEquipmentLoopback);
			attribute_boolean_set_value(any_attribute, (void*)&bool_val, 1);

			/* enable autonegotiation */
			bool_val = 1;
			any_attribute = component_get_attribute(any_component, kBooleanAttributeNic);
			attribute_boolean_set_value(any_attribute, (void*)&bool_val, 1);
			
			any_attribute = component_get_attribute(any_component, kBooleanAttributeSFPDetect);
			temp = attribute_boolean_get_value(any_attribute);
			if (temp)
			{
				bool_val = *(uint8_t*)temp;
				if (bool_val)
				{
					/* turn on rocket io power */
					any_attribute = component_get_attribute(any_component, kBooleanAttributeRocketIOPower);
					attribute_boolean_set_value(any_attribute, (void*)&bool_val, 1);

					any_attribute = component_get_attribute(any_component, kBooleanAttributeSfpPwr);
					attribute_boolean_set_value(any_attribute, (void*)&bool_val, 1);
				}
			}
		}
		any_component = component_get_subcomponent(root, kComponentPbm, 0);
		component_default(any_component);
	}
}


/* Initialization routine. */
dag_err_t
dag70ge_initialize(DagCardPtr card)
{
	dag70ge_state_t* state;
	ComponentPtr root_component;

	assert(NULL != card);
	
	/* Set up virtual methods. */
	card_set_dispose_routine(card, dag70ge_dispose);
	card_set_post_initialize_routine(card, dag70ge_post_initialize);
	card_set_reset_routine(card, dag70ge_reset);
	card_set_default_routine(card, dag70ge_default);

	/* Allocate private state. */
	state = (dag70ge_state_t*) malloc(sizeof(dag70ge_state_t));
	memset(state, 0, sizeof(*state));
	card_set_private_state(card, (void*) state);
	
	/* Create root component. */
	root_component = component_init(kComponentRoot, card);

	/* Add root component to card. */
	card_set_root_component(card, root_component);
	
	
	return kDagErrNone;
}
