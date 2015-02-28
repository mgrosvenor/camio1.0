/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* Public API headers. */

/* Internal project headers. */
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/components/pbm_component.h"
#include "../include/components/optics_component.h"
#include "../include/components/stream_component.h"
#include "../include/components/dag71s_e1t1_component.h"
#include "../include/components/dag71s_channelized_demapper_component.h"
#include "../include/components/dag71s_channelized_mapper_component.h"
#include "../include/components/gpp_component.h"
#include "../include/components/lm_sensor_component.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/components/dag71s_concatenated_demapper_component.h"
#include "../include/components/dag71s_concatenated_mapper_component.h"
#include "../include/components/dag71s_port_component.h"
#include "../include/components/dag71s_phy_component.h"
#include "../include/components/dag71s_erfmux_component.h"
#include "../include/util/logger.h"
#include "../include/components/duck_component.h"
#include "../include/components/sonic_component.h"
#include "../include/components/terf_component.h"
#include "../include/components/card_info_component.h"


/* Endace headers */
#include "dagutil.h"
#include "dagtoken.h"
#include "dagapi.h"
#include "dag_platform.h"


typedef struct
{
    ComponentPtr mGpp;
    ComponentPtr mPbm;
    ComponentPtr mPhy;
    ComponentPtr mHardwareMonitor;
    ComponentPtr *mStream;
    ComponentPtr mOptics[4];
    ComponentPtr mPorts[4];
    ComponentPtr mSonic[4];
    ComponentPtr mChannelizedDemapper;
    ComponentPtr mChannelizedMapper;
    ComponentPtr mE1T1Deframer[4];
    ComponentPtr mConcatenatedDemapper[4];
    ComponentPtr mConcatenatedMapper[4];
    ComponentPtr mErfMux;
    ComponentPtr mTerf;
} dag71s_state_t;


/* Internal routines. */

/* Virtual methods for the card. */
static void dag71s_dispose(DagCardPtr card);
static int dag71s_post_initialize(DagCardPtr card);
static void dag71s_reset(DagCardPtr card);
static void dag71s_default(DagCardPtr card);

typedef enum
{
    kVersionChannelized = 2,
    kVersionConcatenated = 3
} version_t;


dag_err_t
dag71s_initialize(DagCardPtr card)
{
    dag71s_state_t* state;
    ComponentPtr root_component;

    assert(NULL != card);
    
    /* Set up virtual methods. */
    card_set_dispose_routine(card, dag71s_dispose);
    card_set_post_initialize_routine(card, dag71s_post_initialize);
    card_set_reset_routine(card, dag71s_reset);
    card_set_default_routine(card, dag71s_default);

    /* Allocate private state. */
    state = (dag71s_state_t*) malloc(sizeof(dag71s_state_t));
    memset(state, 0, sizeof(*state));
    card_set_private_state(card, (void*) state);
    
    /* Create root component. */
    root_component = component_init(kComponentRoot, card);

    /* Add root component to card. */
    card_set_root_component(card, root_component);
    

    return kDagErrNone;
}


static void
dag71s_dispose(DagCardPtr card)
{
    ComponentPtr component;
    ComponentPtr root;
    AttributePtr attribute;
    int component_count = 0;
    int attribute_count = 0;
    int i = 0;
    dag71s_state_t * state = NULL;

    root = card_get_root_component(card);
    component_count = component_get_subcomponent_count(root);
    for (i = 0; i < component_count; i++)
    {
        component = component_get_indexed_subcomponent(root, i);
        attribute_count = component_get_attribute_count(component);
        while (component_has_attributes(component) == 1)
        {
            attribute = component_get_indexed_attribute(component, 0);
            component_dispose_attribute(component, attribute);
        }
        component_dispose(component);
    }
    /* Deallocate private state. */
    state = (dag71s_state_t*) card_get_private_state(card);
    
    assert(NULL != state);

    /* Free the stream first */
    free(state->mStream);
    
    free(state);
    card_set_private_state(card, NULL);    
}



static int
dag71s_post_initialize(DagCardPtr card)
{
    dag71s_state_t* state = card_get_private_state(card);
    version_t version;
    ComponentPtr root_component;
    int i = 0;
    int port_to_sonic_map[] = {0,2,1,3};
    int rx_streams = 0;
    int tx_streams = 0;
    int streammax = 0;
    int dagfd = 0;
    int x = 0;


    root_component = card_get_root_component(card);
    /* Add components as appropriate. */
    if (card_get_register_address(card, DAG_REG_GPP, 0))
        state->mGpp = gpp_get_new_gpp(card, 0);
    if (card_get_register_address(card, DAG_REG_PBM, 0))
        state->mPbm = pbm_get_new_pbm(card, 0);
	if (card_get_register_address(card, DAG_REG_AMCC, 0))
        state->mPhy = dag71s_get_new_phy_component(card);
    /* note there is no TERF component with using a channelised image */
    if (card_get_register_address(card, DAG_REG_TERF64, 0))
        state->mTerf = terf_get_new_terf(card, 0);
    /* erf mux is not enumerated so can't check for it's existence */
    state->mErfMux = dag71s_erfmux_get_new_component(card);
   
    /* Add card info component */
    component_add_subcomponent(root_component, card_info_get_new_component(card, 0));
 
    // Add hardware monitor
    state->mHardwareMonitor = lm_sensor_get_new_component(card);
    if( state->mHardwareMonitor )
      component_add_subcomponent(root_component, state->mHardwareMonitor);

    for (i = 0; i < 4; i++)
    {
        state->mPorts[i] = dag71s_get_new_port(card, i);
        component_add_subcomponent(root_component, state->mPorts[i]);
        state->mOptics[i] = optics_get_new_optics(card, i);
        component_add_subcomponent(root_component, state->mOptics[i]);
    }
    for (i = 0; i < 4; i++)
    {
        state->mSonic[i] = sonic_get_new_component(card, port_to_sonic_map[i]);
        if (state->mSonic[i])
            component_add_subcomponent(root_component, state->mSonic[i]);
        else
            dagutil_verbose_level(2, "No SONIC component\n");
    }

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
        component_add_subcomponent(root_component, state->mStream[x]);
    }

    if (state->mGpp)
        component_add_subcomponent(root_component, state->mGpp);
    if (state->mPbm)
        component_add_subcomponent(root_component, state->mPbm);
    if (state->mPhy)
        component_add_subcomponent(root_component, state->mPhy);
    if (state->mErfMux)
        component_add_subcomponent(root_component, state->mErfMux);
    if (state->mTerf)
        component_add_subcomponent(root_component, state->mTerf);

    /* Decide which mapper/demapper to use */
    version = card_get_register_version(card, DAG_REG_E1T1_HDLC_DEMAP, 0);
    if (kVersionChannelized == version)
    {
		for (i = 0; i < 4; i++)
		{
			state->mE1T1Deframer[i] = dag71s_get_new_e1t1(card, i);
			component_add_subcomponent(root_component, state->mE1T1Deframer[i]);
		}
        dagutil_verbose("Loading channelized demapper...\n");
        state->mChannelizedDemapper = dag71s_get_new_demapper(card);
        assert(state->mChannelizedDemapper != NULL);
        component_add_subcomponent(root_component, state->mChannelizedDemapper);
    }
    else if (kVersionConcatenated == version)
    {
        dagutil_verbose("Loading concatenated demapper...\n");
        for (i = 0; i < 4; i++)
        {
            state->mConcatenatedDemapper[i] = concatenated_atm_pos_demapper_get_new_component(card, i);
            assert(state->mConcatenatedDemapper[i] != NULL);
            component_add_subcomponent(root_component, state->mConcatenatedDemapper[i]);
        }
    }
    else
    {
        dagutil_panic("no demapper!\n");
    }

    version = card_get_register_version(card, DAG_REG_E1T1_HDLC_MAP, 0);
    if (kVersionChannelized == version)
    {
        dagutil_verbose("Loading channelized mapper...\n");
        state->mChannelizedMapper = dag71s_get_new_mapper(card);
        assert(state->mChannelizedMapper != NULL);
        component_add_subcomponent(root_component, state->mChannelizedMapper);
    }
    else if (kVersionConcatenated == version)
    {
        dagutil_verbose("Loading concatenated mapper...\n");
        for (i = 0; i < 4; i++)
        {
            state->mConcatenatedMapper[i] = concatenated_atm_pos_mapper_get_new_component(card, i);
            assert(state->mConcatenatedMapper[i] != NULL);
            component_add_subcomponent(root_component, state->mConcatenatedMapper[i]);
        }
    }

    if (card_get_register_address(card, DAG_REG_DUCK, 0))
    {
        component_add_subcomponent(root_component, duck_get_new_component(card, 0));
    }

    return 1;
}


static void
dag71s_reset(DagCardPtr card)
{
    uint32_t val;
    int index = 0;
    ComponentPtr phy;
    dag71s_state_t* state = NULL;
	//daginf_t* dag_card_info;  
	//dag_card_info = card_get_info(card);		

    state = (dag71s_state_t*)card_get_private_state(card);
    phy = state->mPhy;    

    /* Reset the AMCC Part*/
    val = card_read_iom(card, 0x174);
    val |= BIT0;
    card_write_iom(card, 0x174, val);

    dagutil_nanosleep(200);

    /* Release RESET */
    val = card_read_iom(card, 0x174);
    val &= ~BIT0;
    card_write_iom(card, 0x174, val);

    dagutil_nanosleep(300);
    /* Set the electric modes */
    val = dag71s_phy_read(phy, 0x01);
    val |= (BIT1|BIT3|BIT6);
    val &= ~(BIT2|BIT5|BIT7);
    dag71s_phy_write(phy, 0x01, val);


    // dag71s_phy_write(phy, (0x06, 0x10);

    /* Set the REFCLK to 155.52MHz */
    val = dag71s_phy_read(phy, 0x00);
    val |= (BIT0 | BIT1);
    dag71s_phy_write(phy, 0x00, val);

    /* Setting to 19.44MHz is depricated
    val = dag71s_phy_read(phy, (0x00);
    val &= ~(BIT0 | BIT1);
    dag71s_phy_write(phy, (0x00, val);
    */

    /* Turn OFF the SER/DES Cores */
    val = dag71s_phy_read(phy, 0x02);
    val |= BIT0;
    dag71s_phy_write(phy, 0x02, val);

    for (index = 0; index < 4; index++)
    {
        val = dag71s_phy_read(phy, 0x08 + (index*2));
        val |= BIT0;
        dag71s_phy_write(phy, 0x08 + (index*2), val);
    }

    /* Turn ON the SER/DES Cores */
    val = dag71s_phy_read(phy, 0x02);
    val &= ~BIT0;
    dag71s_phy_write(phy, 0x02, val);

    for (index = 0; index < 4; index++)
    {
        val = dag71s_phy_read(phy, 0x08 + (index*2));
        val &= ~BIT0;
        dag71s_phy_write(phy, 0x08 + (index*2), val);
    }

    /* Turn ON the Transmit Clocks */
    for (index = 0; index < 4; index++)
    {
        val = dag71s_phy_read(phy, 0x08 + (index*2));
        val &= ~BIT3;
        dag71s_phy_write(phy, 0x08 + (index*2), val);
    }

    /* Set the AMCC part to CSU@OC3, nofcl, noeql, SD low, */
    val = dag71s_phy_read(phy, 0x00);
//    val |= (BIT7 | BIT5 | BIT4 | BIT3 | BIT2);
/**	if(dag_card_info->brd_rev==1){	//this is an AMCC 1221 framer
		val |= (BIT7 | BIT3 | BIT2);
		dag71s_phy_write(phy, 0x00, val);
		//set DLEB
		for(index=0;index<4;index++){
					val = dag71s_phy_read(phy, 0x08+(index*2));	
					val |= BIT5;
					dag71s_phy_write(phy,0x08+(index*2), val);
		}//for loop
				
		//set LLEB
		for(index=0;index<4;index++){
					val = dag71s_phy_read(phy, 0x08+(index*2));	
					val |= BIT6;
					dag71s_phy_write(phy,0x08+(index*2), val);
		}//for loop
		
	}
	else{ //this is an AMCC 1213 framer **/
       val |= (BIT7 | BIT5 | BIT4 | BIT3);
       dag71s_phy_write(phy, 0x00, val);
	//}

    for (index = 0; index < 4; index++)
    {
        // Set OC3
        val = dag71s_phy_read(phy, 0x08 + (index*2));
        val &= ~BIT2;
        dag71s_phy_write(phy, 0x08 + (index*2), val);
    }

    /* Disable the SFP Transmit and Laser */
    val = card_read_iom(card, 0x300);
    val &= 0xFFFF0000;
    card_write_iom(card, 0x300, val);
}


static void
dag71s_default(DagCardPtr card)
{    
    int val, index;
    ComponentPtr sonic;
    ComponentPtr root_component;
    ComponentPtr pbm;
    ComponentPtr erfmux;
    sonic_state_t* sonic_state = NULL;
    ComponentPtr phy;
    dag71s_state_t* state = NULL;

//	daginf_t* dag_card_info;
//	dag_card_info = card_get_info(card);		
    card_reset_datapath(card);
    
    state = (dag71s_state_t*)card_get_private_state(card);
    phy = state->mPhy;    

    /* Set the AMCC part to CSU@OC12, nofcl, noeql, SD low, */
    val = dag71s_phy_read(phy, 0x00);
/** 	if(dag_card_info->brd_rev==1){	//The card contains a AMCC1221 framer

		val |= (BIT7 |BIT3|BIT2);
		dag71s_phy_write(phy, 0x00, val);


		//set DLEB
		for(index=0;index<4;index++){
					val = dag71s_phy_read(phy, 0x08+(index*2));	
					val |= BIT5;
					dag71s_phy_write(phy,0x08+(index*2), val);
		}//for loop
				
		//set LLEB
		for(index=0;index<4;index++){
					val = dag71s_phy_read(phy, 0x08+(index*2));	
					val |= BIT6;
					dag71s_phy_write(phy,0x08+(index*2), val);
		}//for loop

	}
	else{		//The card contains an AMCC1213 framer. **/

    val |= (BIT5 | BIT4 | BIT3 | BIT2);
    dag71s_phy_write(phy, 0x00, val);
	//}
    root_component = card_get_root_component(card);
    for (index=0; index<4; index++)
    {
        sonic = component_get_subcomponent(root_component, kComponentSonic, index);
		
        // Set OC3
        val = dag71s_phy_read(phy, 0x08 + (index*2));
        val &= ~BIT2;
        dag71s_phy_write(phy, 0x08 + (index*2), val);
        if (sonic)
        {
            sonic_state = (sonic_state_t*)component_get_private_state(sonic);
            if (sonic_state->mSonicBase)
            {
                val = card_read_iom(card, sonic_state->mSonicBase);
				/*
            	val &= ~(BIT8 | BIT9 | BIT31);
                val |= BIT4;
				*/
				// set vc4
            	val &= ~(BIT4 | BIT8 | BIT9 | BIT31);
                val |= BIT8;
                card_write_iom(card, sonic_state->mSonicBase, val);
            }
	    
	    component_default(sonic);
        }
	
	
    }

    
    /* Set the default ERF MUX settings, line->host & host->line */
    erfmux = component_get_subcomponent(root_component, kComponentErfMux, 0);
    component_default(erfmux);
    
    
    /* Disable the SFP Transmit and Laser */
    val = card_read_iom(card, 0x300);
    val &= 0xFFFF0000;
    card_write_iom(card, 0x300, val);
    pbm = component_get_subcomponent(root_component, kComponentPbm, 0);
    component_default(pbm);
}


