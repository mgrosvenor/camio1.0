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
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/gpp_component.h"
#include "../include/components/terf_component.h"
#include "../include/components/dag62se_port_component.h"
#include "../include/cards/dag62se_impl.h"
#include "../include/components/lm63.h"
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

/* Internal routines. */
/* Virtual methods for the card. */
static void dag62se_dispose(DagCardPtr card);
static int dag62se_post_initialize(DagCardPtr card);
static void dag62se_reset(DagCardPtr card);
static void dag62se_default(DagCardPtr card);
int valid_lm63_smb(DagCardPtr card, uint32_t smb_base);

/* Counter for smb_init() loop that reads the ready bit. */
#define SMB_INIT_SAFETY_ITERATIONS 500
dag_err_t
dag62se_initialize(DagCardPtr card)
{
    dag62se_state_t* state;
    ComponentPtr root_component;

    
    assert(NULL != card);
    
    /* Set up virtual methods. */
    card_set_dispose_routine(card, dag62se_dispose);
    card_set_post_initialize_routine(card, dag62se_post_initialize);
    card_set_reset_routine(card, dag62se_reset);
    card_set_default_routine(card, dag62se_default);
//    card_set_load_firmware_routine(card, generic_card_load_firmware);

    /* Allocate private state. */
    state = (dag62se_state_t*) malloc(sizeof(dag62se_state_t));
    memset(state, 0, sizeof(*state));
    card_set_private_state(card, (void*) state);
    
    /* Create root component. */
    root_component = component_init(kComponentRoot, card);

    /* Add root component to card. */
    card_set_root_component(card, root_component);
    
    /* Add components as appropriate. */
    state->mGpp = gpp_get_new_gpp(card, 0);
    state->mPbm = pbm_get_new_pbm(card, 0);


    state->mPort = dag62se_get_new_port(card, 0);

    component_add_subcomponent(root_component, state->mGpp);
    component_add_subcomponent(root_component, state->mPbm);
    component_add_subcomponent(root_component, state->mPort);
    
    return kDagErrNone;
}

static void
dag62se_dispose(DagCardPtr card)
{
    ComponentPtr component;
    ComponentPtr root;
    AttributePtr attribute;
    int component_count = 0;
    int i = 0;
    dag62se_state_t* state = NULL;

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
    state = (dag62se_state_t*) card_get_private_state(card);
    
    assert(NULL != state);
    
    assert(NULL != state->mStream);
    free(state->mStream);
    free(state);
    card_set_private_state(card, NULL);
}


static int
dag62se_post_initialize(DagCardPtr card)
{
    dag62se_state_t* state = (dag62se_state_t*)card_get_private_state(card);
    ComponentPtr root = card_get_root_component(card);
    int valid_smb_base = 0;
    int steer_version = 0;
    int rx_streams = 0;
    int tx_streams = 0;
    int streammax = 0;
    int dagfd = 0;
    int x = 0;    

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
    state->mPhyBase = card_get_register_address(card, DAG_REG_S19205, 0);
    state->mGppBase = card_get_register_address(card, DAG_REG_GPP, 0);
    state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);
    state->mSteerBase = card_get_register_address(card, DAG_REG_STEERING, 0);
    steer_version = card_get_register_version(card, DAG_REG_STEERING, 0);
    if(steer_version < 1)
        state->mSteerBase = 0;

    if(card_read_iom(card, RX_OPTICS) & BIT27)
    {
        valid_smb_base = 0;
    }
    else
    {
        /* Find SMBus controller */
        state->mSMBBase = card_get_register_address(card, DAG_REG_SMB, 0);
        valid_smb_base = valid_lm63_smb(card, state->mSMBBase);
        if(valid_smb_base == 0)
        {
            state->mSMBBase = card_get_register_address(card, DAG_REG_SMB, 1);
            valid_smb_base = valid_lm63_smb(card, state->mSMBBase);
        }
    }
    if(valid_smb_base)
    {
        /* Add Hardware Monitor */
        state->mHardwareMonitor = lm63_get_new_component(card);        
        component_add_subcomponent(root, state->mHardwareMonitor);
    }
    else
    {
        state->mSMBBase = 0;
    }

    /* Return 1 if standard post_initialize() should continue, 0 if not.
    *     * "Standard" post_initialize() calls post_initialize() on the card's root component.
    *         */
    /* Add card info component */
    component_add_subcomponent(root, card_info_get_new_component(card, 0));

    return 1;
}


static void
dag62se_reset(DagCardPtr card)
{
}


static void
dag62se_default(DagCardPtr card)
{
    ComponentPtr root;
    dag62se_state_t* state = (dag62se_state_t*)card_get_private_state(card);

    root = card_get_root_component(card);
    state = (dag62se_state_t*)card_get_private_state(card);
    card_reset_datapath(card);
    component_default(state->mPbm);

    component_default(state->mPort);
    component_default(state->mHardwareMonitor);

}

/*
 * SMBus related functions
 */

int
valid_lm63_smb(DagCardPtr card, uint32_t smb_base)
{
   volatile uint32_t readval = 0;
   uint32_t safety_counter = SMB_INIT_SAFETY_ITERATIONS;
   uint32_t val = 0;
   /* Probe for thermal chip */
   val = (uint32_t)LM63_TEMP_LOC << 8 | (uint32_t)LM63 | 0x01000000;
   card_write_iom(card, smb_base + LM63_SMB_Ctrl, val);
   usleep(400);
   /* Wait for ready bit to be set */
   readval = card_read_iom(card, smb_base + LM63_SMB_Read);
   while(0 == (readval & BIT1))
   {
      usleep(400);
      readval = card_read_iom(card, smb_base + LM63_SMB_Read);
      safety_counter--;
      if(safety_counter == 0)
      {
         dagutil_warning("Exceeded safety counter while looking for thermal chip\n");
         return 0;
      }
   }
   /* Valid SMB Bus should have 1 in BIT24 () and 0 in BIT3 (error bit) */
   if((readval & BIT24) && (0 == (readval & BIT3)))
   {
       return 1;
   }
   return 0;
   
}


