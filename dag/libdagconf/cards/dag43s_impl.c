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
#include "../include/card_types.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/component_types.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/cards/dag4_constants.h"
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/gpp_component.h"
#include "../include/components/terf_component.h"
#include "../include/components/dag43s_port_component.h"
#include "../include/components/duck_component.h"
#include "../include/cards/dag43s_impl.h"
#include "../include/components/card_info_component.h"

#include "dag_romutil.h"
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

#define BUFFER_SIZE 1024


#define GPP_ATM_Mode 0x20000
/* Internal routines. */
/* Virtual methods for the card. */
static void dag43s_dispose(DagCardPtr card);
static int dag43s_post_initialize(DagCardPtr card);
static void dag43s_reset(DagCardPtr card);
static void dag43s_default(DagCardPtr card);

dag_err_t
dag43s_initialize(DagCardPtr card)
{
    dag43s_state_t* state;
    ComponentPtr root_component;

    
    assert(NULL != card);
    
    /* Set up virtual methods. */
    card_set_dispose_routine(card, dag43s_dispose);
    card_set_post_initialize_routine(card, dag43s_post_initialize);
    card_set_reset_routine(card, dag43s_reset);
    card_set_default_routine(card, dag43s_default);
//    card_set_load_firmware_routine(card, generic_card_load_firmware);

    /* Allocate private state. */
    state = (dag43s_state_t*) malloc(sizeof(dag43s_state_t));
    memset(state, 0, sizeof(*state));
    card_set_private_state(card, (void*) state);
    
    /* Create root component. */
    root_component = component_init(kComponentRoot, card);

    /* Add root component to card. */
    card_set_root_component(card, root_component);
    
    /* Add components as appropriate. */
    state->mGpp = gpp_get_new_gpp(card, 0);
    state->mPbm = pbm_get_new_pbm(card, 0);

    state->mPort = dag43s_get_new_port(card, 0);
    //state->mPort[1] = dag43s_get_new_port(card, 1);

    component_add_subcomponent(root_component, state->mGpp);
    component_add_subcomponent(root_component, state->mPbm);
    component_add_subcomponent(root_component, state->mPort);
    //component_add_subcomponent(root_component, state->mPort[1]);
    
    return kDagErrNone;
}

static void
dag43s_dispose(DagCardPtr card)
{
    ComponentPtr component;
    ComponentPtr root;
    AttributePtr attribute;
    int component_count = 0;
    int i = 0;
    dag43s_state_t* state = NULL;

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
    state = (dag43s_state_t*) card_get_private_state(card);
    
    assert(NULL != state);

    assert(NULL != state->mStream);
    free(state->mStream);
    
    free(state);
    card_set_private_state(card, NULL);
}


static int
dag43s_post_initialize(DagCardPtr card)
{
    int steer_version = 0;
    int rx_streams = 0;
    int tx_streams = 0;
    int streammax = 0;
    int dagfd = 0;
    int x = 0;    
    dag43s_state_t* state = (dag43s_state_t*)card_get_private_state(card);
    ComponentPtr root = card_get_root_component(card);
    
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
    
    if (card_get_register_address(card, DAG_REG_TERF64, 0))
    {
        dagutil_verbose_level(4, "adding terf component...\n");
        state->mTerf = terf_get_new_terf(card, 0);
        component_add_subcomponent(root, state->mTerf);
    }
    state->mPhyBase = card_get_register_address(card, DAG_REG_PM5381, 0);
    state->mGppBase = card_get_register_address(card, DAG_REG_GPP, 0);
    state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);

    state->mSteerBase = card_get_register_address(card, DAG_REG_STEERING, 0);
    steer_version = card_get_register_version(card, DAG_REG_STEERING, 0);
    if (card_get_register_address(card, DAG_REG_DUCK, 0))
    {
        component_add_subcomponent(root, duck_get_new_component(card, 0));
    }
    if(steer_version < 1)
        state->mSteerBase = 0;

    /* Add card info component */
    component_add_subcomponent(root, card_info_get_new_component(card, 0));
    /* Return 1 if standard post_initialize() should continue, 0 if not.
    *     * "Standard" post_initialize() calls post_initialize() on the card's root component.
    *         */

    return 1;
}


static void
dag43s_reset(DagCardPtr card)
{
}


static void
dag43s_default(DagCardPtr card)
{
    ComponentPtr root;
    ComponentPtr port;
    AttributePtr any_attribute;
    uint8_t boolean_value;
    line_rate_t line_rate;
    crc_t crc_select;
    network_mode_t network_mode;
    int i = 0;
    int count = 0;
    dag43s_state_t* state = NULL;
	
   state = (dag43s_state_t*)card_get_private_state(card);
    
    root = card_get_root_component(card);
    state = (dag43s_state_t*)card_get_private_state(card);
    card_reset_datapath(card);
    component_default(state->mPbm);

    component_default(state->mPort);

    for (i = 0; i < count; i++)
    {
        port = component_get_subcomponent(root, kComponentPort, i);

        /* disable scrambling */
        any_attribute = component_get_attribute(port, kBooleanAttributeScramble);
        boolean_value = 0;
        attribute_set_value(any_attribute, (void*)&boolean_value, sizeof(boolean_value));

        /* Enable payload scramble */
        any_attribute = component_get_attribute(port, kBooleanAttributePayloadScramble);
        boolean_value = 1;
        attribute_set_value(any_attribute, (void*)&boolean_value, sizeof(boolean_value));

        /* Set oc3 pos */
        line_rate = kLineRateOC3c;
        any_attribute = component_get_attribute(port, kUint32AttributeLineRate);
        boolean_value = 1;
        attribute_set_value(any_attribute, (void*)&line_rate, sizeof(line_rate));
        any_attribute = component_get_attribute(port, kUint32AttributeNetworkMode);
        network_mode = kNetworkModePoS;
        attribute_set_value(any_attribute, (void*)&line_rate, sizeof(line_rate));

        /* enable fcl */
        boolean_value = 1;
        any_attribute = component_get_attribute(port, kBooleanAttributeFacilityLoopback);
        attribute_set_value(any_attribute, (void*)&boolean_value, sizeof(uint8_t));

        /* disable eql */
        boolean_value = 0;
        any_attribute = component_get_attribute(port, kBooleanAttributeEquipmentLoopback);
        attribute_set_value(any_attribute, (void*)&boolean_value, sizeof(uint8_t));
        
        /* no crc */
        crc_select = kCrcOff;
        any_attribute = component_get_attribute(port, kUint32AttributeCrcSelect);
        attribute_set_value(any_attribute, (void*)&crc_select, sizeof(crc_t));
    }
    
    /* issue a  datpath reset */
    dagutil_reset_datapath( card_get_iom_address(card) ) ;
}
