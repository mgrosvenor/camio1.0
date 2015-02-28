/*
 * Copyright (c) 2003-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag37ge_impl.c,v 1.26 2008/09/09 04:55:20 wilson.zhu Exp $
 */


/* Public API headers. */

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/cards/dag4_constants.h"
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/util/enum_string_table.h"
#include "../include/cards/dag37ge_impl.h"
#include "../include/components/gpp_component.h"
#include "../include/components/duck_component.h"
#include "../include/components/terf_component.h"
#include "../include/attribute_factory.h"
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

typedef struct
{
    uint32_t mPhyBase;
    uint32_t mGppBase;
    int mIndex;
} dag37ge_port_state_t;

typedef struct
{
    uint32_t mGppBase;
    int mIndex;
} dag37ge_gpp_state_t;

/* GPP register offsets */
enum {
	GPP_config      = 0x0000, /* from gpp_base */
	GPP_pad         = 0x0008, /* from gpp_base */
	GPP_snaplen     = 0x000c  /* from gpp_base */ 
};

/* Stream Processor offsets, each GPP port has a SP at gpp_base + N*SP_offset */
enum {
	SP_status       = 0x0000, /* from sp base */
	SP_drop         = 0x0004, /* from sp base */
	SP_config       = 0x0008, /* from sp base */
	SP_offset       = 0x0020
};


enum
{
    kGeneralSteering = 0x000c,
    kControl    = 0x00, /* Basic Mode Control Register. */
    kStatus    = 0x01, /* Basic Mode Status Register. */
    kAutoNeg    = 0x04, /* Auto-Negotiation Advertisement Register. */
    kBaseTControl   = 0x09, /* 1000BASE-T Control Register. */
    kBaseTStatus   = 0x0a, /* 1000BASE-T Status Register. */
    kLinkStatus = 0x11, /* PHY Specific Status Register. */
    kErrorCounter   = 0x15 /* Error counter */
} register_offset_t;


typedef struct
{
    ComponentPtr mGpp[2];
    ComponentPtr mPbm;
    ComponentPtr *mStream;
    ComponentPtr mPort[2]; 
    ComponentPtr mMux;

    uint32_t mPbmBase;
    uint32_t mGeneralBase;

} dag37ge_state_t;

typedef struct
{
    uint32_t mMuxBase;
} dag37ge_mux_state_t;


/* Stream Processor offsets, each GPP port has a SP at gpp_base + N*SP_offset */
typedef enum 
{
	kSPStatus       = 0x0000, /* from sp base */
	kSPDrop         = 0x0004, /* from sp base */
	kSPConfig	= 0x0008, /* from sp base */
	kSPOffset       = 0x0020,
} stream_processor_offset_t;


static void dag37ge_phy_write(DagCardPtr card, uint32_t phy_base, uint32_t reg, uint32_t value);
static uint32_t dag37ge_phy_read(DagCardPtr card, uint32_t phy_base, uint32_t reg);

/* Internal routines. */
/* Virtual methods for the card. */
static void dag37ge_dispose(DagCardPtr card);
static int dag37ge_post_initialize(DagCardPtr card);
static void dag37ge_reset(DagCardPtr card);
static void dag37ge_default(DagCardPtr card);
static dag_err_t dag37ge_update_register_base(DagCardPtr card);

/* port component. */
static ComponentPtr dag37ge_get_new_port(DagCardPtr card, int index);
static void dag37ge_port_dispose(ComponentPtr component);
static void dag37ge_port_reset(ComponentPtr component);
static void dag37ge_port_default(ComponentPtr component);
static int dag37ge_port_post_initialize(ComponentPtr component);
static dag_err_t dag37ge_port_update_register_base(ComponentPtr component);

/* mux component. */
static ComponentPtr dag37ge_get_new_mux(DagCardPtr card);
static void dag37ge_mux_dispose(ComponentPtr component);
static void dag37ge_mux_reset(ComponentPtr component);
static void dag37ge_mux_default(ComponentPtr component);
static int dag37ge_mux_post_initialize(ComponentPtr component);
static dag_err_t dag37ge_mux_update_register_base(ComponentPtr component);

/* line_rate attribute */
static AttributePtr dag37ge_get_new_line_rate_attribute(void);
static void dag37ge_line_rate_dispose(AttributePtr attribute);
static void* dag37ge_line_rate_get_value(AttributePtr attribute);
static void dag37ge_line_rate_set_value(AttributePtr attribute, void* value, int length);
static void dag37ge_line_rate_post_initialize(AttributePtr attribute);
static void dag37ge_line_rate_to_string(AttributePtr attribute);
static void dag37ge_line_rate_from_string(AttributePtr attribute, const char* string);

/* error_counter attribute */
static AttributePtr dag37ge_get_new_error_counter_attribute(void);
static void dag37ge_error_counter_dispose(AttributePtr attribute);
static void* dag37ge_error_counter_get_value(AttributePtr attribute);
static void dag37ge_error_counter_post_initialize(AttributePtr attribute);

/* force_line_rate attribute */
static AttributePtr dag37ge_get_new_force_line_rate_attribute(void);
static void dag37ge_force_line_rate_dispose(AttributePtr attribute);
static void dag37ge_force_line_rate_set_value(AttributePtr attribute, void* value, int length);
static void* dag37ge_force_line_rate_get_value(AttributePtr attribute);
static void dag37ge_force_line_rate_post_initialize(AttributePtr attribute);
static void dag37ge_force_line_rate_from_string(AttributePtr attribute, const char* string);
static void dag37ge_force_line_rate_to_string(AttributePtr attribute);

/* nic attribute. */
static AttributePtr dag37ge_get_new_nic_attribute(void);
static void dag37ge_nic_dispose(AttributePtr attribute);
static void* dag37ge_nic_get_value(AttributePtr attribute);
static void dag37ge_nic_set_value(AttributePtr attribute, void* value, int length);
static void dag37ge_nic_post_initialize(AttributePtr attribute);

#if 0	//NOTE: The merge and split attributes have been replaced with the steer attribute
/* split attribute. */
static AttributePtr dag37ge_get_new_split_attribute(void);
static void dag37ge_split_dispose(AttributePtr attribute);
static void* dag37ge_split_get_value(AttributePtr attribute);
static void dag37ge_split_set_value(AttributePtr attribute, void* value, int length);
static void dag37ge_split_post_initialize(AttributePtr attribute);

/* merge attribute. */
static AttributePtr dag37ge_get_new_merge_attribute(void);
static void dag37ge_merge_dispose(AttributePtr attribute);
static void* dag37ge_merge_get_value(AttributePtr attribute);
static void dag37ge_merge_set_value(AttributePtr attribute, void* value, int length);
static void dag37ge_merge_post_initialize(AttributePtr attribute);
#endif

/* steer attribute. */
static AttributePtr dag37ge_get_new_steer_attribute(void);
static void dag37ge_steer_dispose(AttributePtr attribute);
static void* dag37ge_steer_get_value(AttributePtr attribute);
static void dag37ge_steer_set_value(AttributePtr attribute, void* value, int length);
static void dag37ge_steer_post_initialize(AttributePtr attribute);
static void dag37ge_steer_value_to_string(AttributePtr attribute);
static void dag37ge_steer_value_from_string(AttributePtr attribute, const char* string);

/* swap attribute. */
static AttributePtr dag37ge_get_new_swap_attribute(void);
static void dag37ge_swap_dispose(AttributePtr attribute);
static void* dag37ge_swap_get_value(AttributePtr attribute);
static void dag37ge_swap_set_value(AttributePtr attribute, void* value, int length);
static void dag37ge_swap_post_initialize(AttributePtr attribute);

/* link attribute. */
static AttributePtr dag37ge_get_new_link_attribute(void);
static void dag37ge_link_dispose(AttributePtr attribute);
static void* dag37ge_link_get_value(AttributePtr attribute);
static void dag37ge_link_post_initialize(AttributePtr attribute);

/* full_duplex attribute. */
static AttributePtr dag37ge_get_new_full_duplex_attribute(void);
static void dag37ge_full_duplex_dispose(AttributePtr attribute);
static void* dag37ge_full_duplex_get_value(AttributePtr attribute);
static void dag37ge_full_duplex_post_initialize(AttributePtr attribute);

/* auto_negotiation_complete attribute. */
static AttributePtr dag37ge_get_new_auto_negotiation_complete_attribute(void);
static void dag37ge_auto_negotiation_complete_dispose(AttributePtr attribute);
static void* dag37ge_auto_negotiation_complete_get_value(AttributePtr attribute);
static void dag37ge_auto_negotiation_complete_post_initialize(AttributePtr attribute);

/* jabber attribute. */
static AttributePtr dag37ge_get_new_jabber_attribute(void);
static void dag37ge_jabber_dispose(AttributePtr attribute);
static void* dag37ge_jabber_get_value(AttributePtr attribute);
static void dag37ge_jabber_post_initialize(AttributePtr attribute);

/* master attribute. */
static AttributePtr dag37ge_get_new_master_attribute(void);
static void dag37ge_master_dispose(AttributePtr attribute);
static void* dag37ge_master_get_value(AttributePtr attribute);
static void dag37ge_master_post_initialize(AttributePtr attribute);
static void dag37ge_master_to_string_routine(AttributePtr attribute);

/* remote fault attribute. */
static AttributePtr dag37ge_get_new_remote_fault_attribute(void);
static void dag37ge_remote_fault_dispose(AttributePtr attribute);
static void* dag37ge_remote_fault_get_value(AttributePtr attribute);
static void dag37ge_remote_fault_post_initialize(AttributePtr attribute);

/* active attribute. */
static AttributePtr dag37ge_get_new_active_attribute(void);
static void dag37ge_active_dispose(AttributePtr attribute);
static void* dag37ge_active_get_value(AttributePtr attribute);
static void dag37ge_active_set_value(AttributePtr attribute, void* value, int length);
static void dag37ge_active_post_initialize(AttributePtr attribute);

/* drop_count */
static AttributePtr dag37ge_get_new_drop_count_attribute(void);
static void dag37ge_drop_count_dispose(AttributePtr attribute);
static void* dag37ge_drop_count_get_value(AttributePtr attribute);
static void dag37ge_drop_count_post_initialize(AttributePtr attribute);



static void 
dag37ge_phy_write(DagCardPtr card, uint32_t phy_base, uint32_t reg, uint32_t value)
{
    card_write_iom(card, phy_base, (reg << 16) | (value & 0xffff));
    usleep(100);
}

static uint32_t
dag37ge_phy_read(DagCardPtr card, uint32_t phy_base, uint32_t reg)
{
    card_write_iom(card, phy_base, (reg << 16) | BIT31);
    usleep(100);
    return (card_read_iom(card, phy_base) & 0xffff);
}

/* Implementation of internal routines. */

static void
dag37ge_dispose(DagCardPtr card)
{
    ComponentPtr component;
    ComponentPtr root;
    AttributePtr attribute;
    int component_count = 0;
    int i = 0;
    dag37ge_state_t* state = NULL;

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
    state = (dag37ge_state_t*) card_get_private_state(card);
    
    assert(NULL != state);

    /* Free the stream first */
    free(state->mStream);
    
    free(state);
    card_set_private_state(card, NULL);


}


static int
dag37ge_post_initialize(DagCardPtr card)
{
    int rx_streams = 0;
    int tx_streams = 0;
    int streammax = 0;
    int x = 0;
    int dagfd = 0;
    ComponentPtr root_component;
    dag37ge_state_t* state = (dag37ge_state_t*)card_get_private_state(card);
    state->mGeneralBase = card_get_register_address(card, DAG_REG_GENERAL, 0);
    state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);

    root_component = card_get_root_component(card);
    /* Add the stream components here once we can calculate the number of streams */    
    /* Find out the number of streams present */
    dagfd = card_get_fd(card);
    /* Get the number of RX Streams */
    rx_streams = dag_rx_get_stream_count(dagfd);
    tx_streams = dag_tx_get_stream_count(dagfd);
    streammax = dagutil_max(rx_streams*2 - 1, tx_streams*2);
    if(streammax <= 0)
        streammax = 0;
    else
    {
        state->mStream = malloc(sizeof(state->mStream)*streammax);
        assert(state->mStream != NULL);
    }
    for ( x = 0;x < streammax; x++)
    {
        state->mStream[x] = get_new_stream(card, x);
        component_add_subcomponent(root_component, state->mStream[x]);
    }    

    if (card_get_register_address(card, DAG_REG_DUCK, 0))
    {
        component_add_subcomponent(root_component, duck_get_new_component(card, 0));
    }
    
    /* Add card info component */
    component_add_subcomponent(root_component, card_info_get_new_component(card, 0));
    return 1;
}


static void
dag37ge_reset(DagCardPtr card)
{
    ComponentPtr port;
    ComponentPtr root_component;
    dag37ge_port_state_t* state;

    root_component = card_get_root_component(card);
    port = component_get_subcomponent(root_component, kComponentPort, 0);
    state = component_get_private_state(port);
    dag37ge_phy_write(card, state->mPhyBase, kControl, BIT15);
    port = component_get_subcomponent(root_component, kComponentPort, 1);
    state = component_get_private_state(port);
    dag37ge_phy_write(card, state->mPhyBase, kControl, BIT15);
}


static dag_err_t
dag37ge_update_register_base(DagCardPtr card)
{
    if (1 == valid_card(card))
    {
        dag37ge_state_t* state = (dag37ge_state_t*)card_get_private_state(card);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mGeneralBase = card_get_register_address(card, DAG_REG_GENERAL, 0);
        state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static void
dag37ge_default(DagCardPtr card)
{
    uint32_t val = 0;
    dag37ge_state_t* state = (dag37ge_state_t*)card_get_private_state(card);
    ComponentPtr root;
    ComponentPtr port;
    AttributePtr nic;
    ComponentPtr pbm;
    uint8_t nic_val = 1;

    card_reset_datapath(card);
    /*card_pbm_default(card);*/
    dag37ge_reset(card);
    val = card_read_iom(card, state->mGeneralBase + kGeneralSteering);
    /* reset bits 2 and 3 - single mem hole, no port swapping */
    val &= ~(BIT18 | BIT19);
    card_write_iom(card, state->mGeneralBase + kGeneralSteering, val);
    root = card_get_root_component(card);
    port = component_get_subcomponent(root, kComponentPort, 0);
    nic = component_get_attribute(port, kBooleanAttributeNic);
    dag37ge_nic_set_value(nic, (void*)&nic_val, sizeof(uint8_t));
    port = component_get_subcomponent(root, kComponentPort, 1);
    nic = component_get_attribute(port, kBooleanAttributeNic);
    dag37ge_nic_set_value(nic, (void*)&nic_val, sizeof(uint8_t));
    pbm = component_get_subcomponent(root, kComponentPbm, 0);
    component_default(pbm);
}

static ComponentPtr
dag37ge_get_new_mux(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentMux, card);

    if (NULL != result)
    {
        dag37ge_mux_state_t* state = (dag37ge_mux_state_t*)malloc(sizeof(dag37ge_mux_state_t));
        component_set_dispose_routine(result, dag37ge_mux_dispose);
        component_set_reset_routine(result, dag37ge_mux_reset);
        component_set_default_routine(result, dag37ge_mux_default);
        component_set_post_initialize_routine(result, dag37ge_mux_post_initialize);
        component_set_name(result, "mux");
        component_set_private_state(result, state);
        component_set_update_register_base_routine(result, dag37ge_mux_update_register_base);
    }

    return result;
}

static dag_err_t
dag37ge_mux_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = NULL;
        dag37ge_mux_state_t* state = (dag37ge_mux_state_t*)component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mMuxBase = card_get_register_address(card, DAG_REG_GENERAL, 0);
        return kDagErrNone;
    }
    return  kDagErrInvalidParameter;
}

static void
dag37ge_mux_dispose(ComponentPtr component)
{

}

static void
dag37ge_mux_reset(ComponentPtr component)
{

}

static void
dag37ge_mux_default(ComponentPtr component)
{

}

static int
dag37ge_mux_post_initialize(ComponentPtr component)
{
    //AttributePtr merge;
    //AttributePtr split;
    AttributePtr steer;
    AttributePtr swap;
    DagCardPtr card;
    dag37ge_mux_state_t* state = (dag37ge_mux_state_t*)component_get_private_state(component);
    assert(state != NULL);
    card = component_get_card(component);
    state->mMuxBase = card_get_register_address(card, DAG_REG_GENERAL, 0);
    //merge = dag37ge_get_new_merge_attribute();
    //split = dag37ge_get_new_split_attribute();
    steer = dag37ge_get_new_steer_attribute();
    swap = dag37ge_get_new_swap_attribute();

    //component_add_attribute(component, merge);
    //component_add_attribute(component, split);
    component_add_attribute(component, steer);
    component_add_attribute(component, swap);

    return 1;
}

static ComponentPtr
dag37ge_get_new_port(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentPort, card);

    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        dag37ge_port_state_t* state = (dag37ge_port_state_t*)malloc(sizeof(dag37ge_port_state_t));
        component_set_dispose_routine(result, dag37ge_port_dispose);
        component_set_reset_routine(result, dag37ge_port_reset);
        component_set_default_routine(result, dag37ge_port_default);
        component_set_post_initialize_routine(result, dag37ge_port_post_initialize);
        component_set_update_register_base_routine(result, dag37ge_port_update_register_base);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;
}

static dag_err_t
dag37ge_port_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        dag37ge_port_state_t* state = NULL;
        DagCardPtr card = component_get_card(component);
        state = (dag37ge_port_state_t*)component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mPhyBase = card_get_register_address(card, DAG_REG_MV88E1111, state->mIndex);
        state->mGppBase = card_get_register_address(card, DAG_REG_GPP, state->mIndex);       
        return kDagErrNone;
    }
    return kDagErrGeneral;
}

static void
dag37ge_port_dispose(ComponentPtr component)
{

}

static void
dag37ge_port_reset(ComponentPtr component)
{

}

static void
dag37ge_port_default(ComponentPtr component)
{

}

static int
dag37ge_port_post_initialize(ComponentPtr component)
{
    AttributePtr active = NULL; 
    AttributePtr nic;
    AttributePtr force_line_rate;
    AttributePtr link;
    AttributePtr auto_neg_complete;
    AttributePtr jabber;
    AttributePtr full_duplex;
    AttributePtr remote_fault;
    AttributePtr master;
    AttributePtr line_rate;
    AttributePtr error_counter;
    AttributePtr ethMac;
    AttributePtr drop_count;
    uint32_t register_val = 0;
    uint8_t active_valid = 0;
    uint32_t address = 0;
    uint32_t bit_mask = 0xFFFFFFFF;
    GenericReadWritePtr grw = NULL;
        

    dag37ge_port_state_t* state = NULL;
    DagCardPtr card;

    state = component_get_private_state(component);
    assert(state != NULL);
    card = component_get_card(component);
    state->mPhyBase = card_get_register_address(card, DAG_REG_MV88E1111, state->mIndex);
    state->mGppBase = card_get_register_address(card, DAG_REG_GPP, state->mIndex);
    /* Check to see if active is valid for current firmware */
    register_val = card_read_iom(card, state->mGppBase + GPP_config);
    if(register_val & BIT4)
    { /* attrib valid for current config */
        active_valid = 1;
    }
    if(active_valid == 1)
        active = dag37ge_get_new_active_attribute();
    
    nic = dag37ge_get_new_nic_attribute();
    force_line_rate = dag37ge_get_new_force_line_rate_attribute();
    link = dag37ge_get_new_link_attribute();
    auto_neg_complete = dag37ge_get_new_auto_negotiation_complete_attribute();
    full_duplex = dag37ge_get_new_full_duplex_attribute();
    jabber = dag37ge_get_new_jabber_attribute();
    master = dag37ge_get_new_master_attribute();
    remote_fault = dag37ge_get_new_remote_fault_attribute();
    line_rate = dag37ge_get_new_line_rate_attribute();
    error_counter = dag37ge_get_new_error_counter_attribute();
    drop_count = dag37ge_get_new_drop_count_attribute();

    if(active_valid == 1)
        component_add_attribute(component, active);

    /* Add Ethernet MAC Address */
    address = 8 +(state->mIndex*8); /* First port */
    
    grw = grw_init(card, address, grw_rom_read, NULL);
    ethMac = attribute_factory_make_attribute(kStringAttributeEthernetMACAddress, grw, &bit_mask, 1);
    component_add_attribute(component, ethMac);    

        
    component_add_attribute(component, nic);
    component_add_attribute(component, force_line_rate);
    component_add_attribute(component, link);
    component_add_attribute(component, auto_neg_complete);
    component_add_attribute(component, full_duplex);
    component_add_attribute(component, jabber);
    component_add_attribute(component, master);
    component_add_attribute(component, remote_fault);
    component_add_attribute(component, line_rate);
    component_add_attribute(component, error_counter);
    component_add_attribute(component, drop_count);

    return 1;
}

#if 0	//NOTE The merge and split attributes have been replaced with the steer attribute
static AttributePtr
dag37ge_get_new_split_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeSplit); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_split_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_split_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_split_get_value);
        attribute_set_setvalue_routine(result, dag37ge_split_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "split");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
dag37ge_split_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_split_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_split_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t val = 0;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        val = card_read_iom(card, state->mMuxBase + kGeneralSteering);
        val |= BIT18;
        card_write_iom(card, state->mMuxBase + kGeneralSteering, val);
    }
}


static void*
dag37ge_split_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t register_val;
        static uint8_t val = 0;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        register_val = card_read_iom(card, state->mMuxBase + kGeneralSteering);
        val = ((register_val & BIT18) == BIT18);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37ge_get_new_merge_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeMerge); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_merge_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_merge_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_merge_get_value);
        attribute_set_setvalue_routine(result, dag37ge_merge_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "merge");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
dag37ge_merge_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_merge_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_merge_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t val = 0;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        val = card_read_iom(card, state->mMuxBase + kGeneralSteering);
        val &= ~BIT18;
        card_write_iom(card, state->mMuxBase + kGeneralSteering, val);
    }
}


static void*
dag37ge_merge_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t register_val;
        static uint8_t val = 0;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        register_val = card_read_iom(card, state->mMuxBase + kGeneralSteering);
        /* streams merge if bit 18 is 0 */
        val = ((register_val & BIT18) == BIT18);
        val = !val;
        return (void*)&val;
    }
    return NULL;
}
#endif

static AttributePtr
dag37ge_get_new_steer_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeSteer); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_steer_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_steer_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_steer_get_value);
        attribute_set_setvalue_routine(result, dag37ge_steer_set_value);
        attribute_set_to_string_routine(result, dag37ge_steer_value_to_string);
        attribute_set_from_string_routine(result, dag37ge_steer_value_from_string);
        attribute_set_name(result, "steer");
        attribute_set_description(result, "The method to use to steer the incoming packet");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}


static void
dag37ge_steer_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_steer_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_steer_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t val = 0;
    	steer_t temp = *(steer_t*)value;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        val = card_read_iom(card, state->mMuxBase + kGeneralSteering);
        if (temp == kSteerStream0)
		val &= ~BIT18;
	else if (temp == kSteerIface)
		val |= BIT18;

        card_write_iom(card, state->mMuxBase + kGeneralSteering, val);
    }
}


static void*
dag37ge_steer_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t register_val;
        static uint32_t val = 0;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        register_val = card_read_iom(card, state->mMuxBase + kGeneralSteering);

        if((register_val & BIT18) == 0)
		val = kSteerStream0;
	else if((register_val & BIT18) == BIT18)
		val = kSteerIface;

        return (void*)&val;
    }
    return NULL;
}

static void
dag37ge_steer_value_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        const char* temp = NULL;
        steer_t steer = *(steer_t*)dag37ge_steer_get_value(attribute);
        temp = steer_to_string(steer);
        attribute_set_to_string(attribute, temp);
    }
}

static void
dag37ge_steer_value_from_string(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        steer_t steer = string_to_steer(string);
        dag37ge_steer_set_value(attribute, (void*)&steer, sizeof(steer_t));
    }
}


static AttributePtr
dag37ge_get_new_swap_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeSwap); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_swap_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_swap_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_swap_get_value);
        attribute_set_setvalue_routine(result, dag37ge_swap_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "swap");
        attribute_set_description(result, "Disable or enable tx interface swapping on the card in the ERF header");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
dag37ge_swap_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_swap_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_swap_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t val = 0;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        val = card_read_iom(card, state->mMuxBase + kGeneralSteering);
        if (*(uint8_t*)value == 0)
        {
            val &= ~BIT19;
        }
        else
        {
            val |= BIT19;
        }
        card_write_iom(card, state->mMuxBase + kGeneralSteering, val);
    }
}


static void*
dag37ge_swap_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr mux;
        uint32_t register_val;
        static uint8_t val = 0;
        dag37ge_mux_state_t* state = NULL;

        card = attribute_get_card(attribute);
        mux = attribute_get_component(attribute);
        state = (dag37ge_mux_state_t*)component_get_private_state(mux);
        register_val = card_read_iom(card, state->mMuxBase + kGeneralSteering);
        val = ((register_val & BIT19) == BIT19);
        return (void*)&val;
    }
    return NULL;
}


static AttributePtr
dag37ge_get_new_nic_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeNic); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_nic_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_nic_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_nic_get_value);
        attribute_set_setvalue_routine(result, dag37ge_nic_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "auto_neg");
        attribute_set_description(result, "Enable or disable ethernet auto-negotiation");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
dag37ge_nic_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_nic_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_nic_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr port;
        int val = 0;
        dag37ge_port_state_t* state = NULL;

        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);

        /* Advertise all speeds */
        /* Enable 1000Base-T advertisement */
        val = dag37ge_phy_read(card, state->mPhyBase, kBaseTControl);
        val |= BIT11|BIT10|BIT9;
        dag37ge_phy_write(card, state->mPhyBase, kBaseTControl, val);

        /* Enable 10Base-T and 100Base-T advertisement */
        val = dag37ge_phy_read(card, state->mPhyBase, kAutoNeg);
        val |= BIT8|BIT6|BIT0;
        dag37ge_phy_write(card, state->mPhyBase, kAutoNeg, val);

        val = dag37ge_phy_read(card, state->mPhyBase, kControl);
        if (*(uint8_t*)value == 0)
        {
            val &= ~(BIT12);
        }
        else
        {
            val |= (BIT12 | BIT9);
        }

        val |= BIT15;
        dag37ge_phy_write(card, state->mPhyBase, kControl, val);

    }
}


static void*
dag37ge_nic_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr port;
        uint32_t register_val;
        static uint8_t val = 0;
        dag37ge_port_state_t* state = NULL;

        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);
        register_val = dag37ge_phy_read(card, state->mPhyBase, kControl);
        val = ((register_val & BIT12) == BIT12);
        return (void*)&val;
    }
    return NULL;
}
static AttributePtr
dag37ge_get_new_active_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_active_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_active_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_active_get_value);
        attribute_set_setvalue_routine(result, dag37ge_active_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "active");
        attribute_set_description(result, "Enable or Disable Port");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
dag37ge_active_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag37ge_active_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void
dag37ge_active_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr port;
        int val = 0;
        dag37ge_port_state_t* state = NULL;

        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
		val = card_read_iom(card, state->mGppBase + GPP_config);
		val |= BIT5; /* enable per-stream config registers */
        card_write_iom(card, state->mGppBase + GPP_config, val);
        
		val = card_read_iom(card, state->mGppBase + SP_config + SP_offset);
        if (*(uint8_t*)value == 0)
        {
            val &= ~BIT12;
        }
        else
        {
            val |= BIT12;
        }
        card_write_iom(card, state->mGppBase +SP_config + SP_offset, val);
    }
}


static void*
dag37ge_active_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr port;
        uint32_t register_val;
        static uint8_t val = 0;
        dag37ge_port_state_t* state = NULL;

        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);
		register_val = card_read_iom(card, state->mGppBase + GPP_config);
        if (register_val & BIT5) {
            register_val = card_read_iom(card, state->mGppBase + SP_config + SP_offset);
			
            if(register_val & BIT12)
            {
                val = 0;
            }
            else
            {
                val = 1;
            }
        }
        val = 1; /* Disabled */
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37ge_get_new_error_counter_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeErrorCounter); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_error_counter_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_error_counter_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_error_counter_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "error_counter");
        attribute_set_description(result, "Use this attribute to count the errors.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag37ge_error_counter_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_error_counter_get_value(AttributePtr attribute)
{
    DagCardPtr card = attribute_get_card(attribute);
    dag37ge_port_state_t* state = NULL;
    ComponentPtr port;
    static uint32_t return_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);
        return_value = dag37ge_phy_read(card, state->mPhyBase, kErrorCounter);
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_error_counter_post_initialize(AttributePtr attribute)
{
}
static AttributePtr
dag37ge_get_new_line_rate_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineRate); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_line_rate_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_line_rate_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_line_rate_get_value);
        attribute_set_setvalue_routine(result, dag37ge_line_rate_set_value);
        attribute_set_to_string_routine(result, dag37ge_line_rate_to_string);
        attribute_set_from_string_routine(result, dag37ge_line_rate_from_string);
        attribute_set_name(result, "line_rate");
        attribute_set_description(result, "Use this attribute to check the speed of the line.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag37ge_line_rate_to_string(AttributePtr attribute)
{
    const char* temp = NULL;
    line_rate_t line_rate = *(line_rate_t*)dag37ge_line_rate_get_value(attribute);
    temp = line_rate_to_string(line_rate);
    attribute_set_to_string(attribute, temp);
}

static void
dag37ge_line_rate_from_string(AttributePtr attribute, const char* string)
{
    line_rate_t value;
    
    if (1 == valid_attribute(attribute))
    {
        value = string_to_line_rate(string);
        if (kLineRateInvalid != value)
            dag37ge_line_rate_set_value(attribute, (void*)&value, sizeof(line_rate_t));
    }
}

static void
dag37ge_line_rate_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_line_rate_get_value(AttributePtr attribute)
{
    DagCardPtr card = attribute_get_card(attribute);
    dag37ge_port_state_t* state = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static line_rate_t return_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);
	register_val = dag37ge_phy_read(card, state->mPhyBase, kLinkStatus);
	if ((register_val & BIT15) == BIT15)
        {
            return_value = kLineRateEthernet1000;
        }
	else if ((register_val & BIT14) == BIT14)
        {
            return_value = kLineRateEthernet100;
        }
        else
        {
            return_value = kLineRateEthernet10;
        }
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_line_rate_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        dag37ge_port_state_t* state = NULL;
        ComponentPtr port;
        line_rate_t val = *(line_rate_t*)value;
        uint32_t register_val;

        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);

        if (val == kLineRateEthernet10)
        {
            // Set mode control register
            register_val = dag37ge_phy_read(card, state->mPhyBase, kControl);
            register_val &= ~(BIT13|BIT8|BIT6);
            register_val |= BIT8;
            dag37ge_phy_write(card, state->mPhyBase, kControl, register_val);

            // Disable 1000Base-T advertisement
            register_val = dag37ge_phy_read(card, state->mPhyBase, kBaseTControl);
            register_val |= BIT11|BIT10;
            register_val &= ~BIT9;
            dag37ge_phy_write(card, state->mPhyBase, kBaseTControl, register_val);

            // Enable 10Base-T advertisement, disable 100Base-T advertisement
            register_val = dag37ge_phy_read(card, state->mPhyBase, kAutoNeg);
            register_val |= BIT6|BIT0;
            register_val &= ~BIT8;
            dag37ge_phy_write(card, state->mPhyBase, kAutoNeg, register_val);

        }
        else if (val == kLineRateEthernet100)
        {
            // Set mode control register
            register_val = dag37ge_phy_read(card, state->mPhyBase, kControl);
            register_val &= ~(BIT13|BIT8|BIT6);
            register_val |= BIT13|BIT8;
            dag37ge_phy_write(card, state->mPhyBase, kControl, register_val);

            // Disable 1000Base-T advertisement
            register_val = dag37ge_phy_read(card, state->mPhyBase, kBaseTControl);
            register_val |= BIT11|BIT10;
            register_val &= ~BIT9;
            dag37ge_phy_write(card, state->mPhyBase, kBaseTControl, register_val);

            // Enable 100Base-T advertisement, disable 10Base-T advertisement
            register_val = dag37ge_phy_read(card, state->mPhyBase, kAutoNeg);
            register_val |= BIT8|BIT0;
            register_val &= ~BIT6;
            dag37ge_phy_write(card, state->mPhyBase, kAutoNeg, register_val);

        }
        else if (val == kLineRateEthernet1000)
        {
            // Set mode control register
            register_val = dag37ge_phy_read(card, state->mPhyBase, kControl);
            register_val &= ~(BIT13|BIT8|BIT6);
            register_val |= BIT8|BIT6;
            dag37ge_phy_write(card, state->mPhyBase, kControl, register_val);

            // Enable 1000Base-T advertisement
            register_val = dag37ge_phy_read(card, state->mPhyBase, kBaseTControl);
            register_val |= BIT11|BIT10|BIT9;
            dag37ge_phy_write(card, state->mPhyBase, kBaseTControl, register_val);

            // Disable 100Base-T and 10Base-T advertisement
            register_val = dag37ge_phy_read(card, state->mPhyBase, kAutoNeg);
            register_val &= ~(BIT8|BIT6);
            dag37ge_phy_write(card, state->mPhyBase, kAutoNeg, register_val);

        }

        // Perform a software reset
        register_val = dag37ge_phy_read(card, state->mPhyBase, kControl);
        register_val |= BIT15;
        dag37ge_phy_write(card, state->mPhyBase, kControl, register_val);
    }
}

static void
dag37ge_line_rate_post_initialize(AttributePtr attribute)
{
}


static AttributePtr
dag37ge_get_new_force_line_rate_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeForceLineRate); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_force_line_rate_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_force_line_rate_post_initialize);
        attribute_set_setvalue_routine(result, dag37ge_force_line_rate_set_value);
        attribute_set_getvalue_routine(result, dag37ge_force_line_rate_get_value);
        attribute_set_from_string_routine(result, dag37ge_force_line_rate_from_string);
        attribute_set_to_string_routine(result, dag37ge_force_line_rate_to_string);
        attribute_set_name(result, "force_line_rate");
        attribute_set_description(result, "Use this attribute to force the card to operate at the specified line rate.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag37ge_force_line_rate_to_string(AttributePtr attribute)
{
    const char* temp = NULL;
    line_rate_t line_rate = *(line_rate_t*)dag37ge_force_line_rate_get_value(attribute);
    temp = line_rate_to_string(line_rate);
    attribute_set_to_string(attribute, temp);
}

static void
dag37ge_force_line_rate_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_force_line_rate_get_value(AttributePtr attribute)
{
    DagCardPtr card = attribute_get_card(attribute);
    dag37ge_port_state_t* state = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static line_rate_t return_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);
        register_val = dag37ge_phy_read(card, state->mPhyBase, kControl);
        if ((register_val & (BIT6 | BIT13 | BIT12)) == 0)
        {
            return_value = kLineRateEthernet10;
        }
        else if ((register_val & (BIT6 | BIT12 | BIT13)) == BIT13)
        {
            return_value = kLineRateEthernet100;
        }
        else if ((register_val & (BIT6 | BIT12 | BIT13)) == BIT6)
        {
            return_value = kLineRateEthernet1000;
        }
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_force_line_rate_set_value(AttributePtr attribute, void* value, int length)
{

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        dag37ge_port_state_t* state = NULL;
        ComponentPtr port;
        line_rate_t val = *(line_rate_t*)value;
        uint32_t register_val;

        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);
        register_val = dag37ge_phy_read(card, state->mPhyBase, kControl);
        if (val == kLineRateEthernet10)
        {
            register_val &= ~(BIT6 | BIT13 | BIT12);
            register_val |= BIT15;
        }
        else if (val == kLineRateEthernet100)
        {
            register_val &= ~(BIT6 | BIT12);
            register_val |= BIT13 | BIT15;
        }
        else if (val == kLineRateEthernet1000)
        {
            register_val &= ~(BIT13 | BIT12);
            register_val |= BIT6 | BIT15;
        }
        dag37ge_phy_write(card, state->mPhyBase, 0, register_val);
    }
}

static void
dag37ge_force_line_rate_post_initialize(AttributePtr attribute)
{
}


static void
dag37ge_force_line_rate_from_string(AttributePtr attribute, const char* string)
{
    line_rate_t value;
    
    if (1 == valid_attribute(attribute))
    {
        value = string_to_line_rate(string);
        if (kLineRateInvalid != value)
            dag37ge_force_line_rate_set_value(attribute, (void*)&value, sizeof(line_rate_t));
    }
}

static AttributePtr
dag37ge_get_new_link_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLink); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_link_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_link_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_link_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "link");
        attribute_set_description(result, "Indicates whether the link is up.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag37ge_link_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_link_get_value(AttributePtr attribute)
{
    static uint8_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag37ge_port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        register_value = dag37ge_phy_read(card, state->mPhyBase, kLinkStatus);
        return_value = (register_value & BIT10) == BIT10;
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_link_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag37ge_get_new_full_duplex_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFullDuplex); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_full_duplex_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_full_duplex_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_full_duplex_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "full_duplex");
        attribute_set_description(result, "Indicates whether the link is in full duplex mode or half duplex mode.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag37ge_full_duplex_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_full_duplex_get_value(AttributePtr attribute)
{
    static uint8_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag37ge_port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        register_value = dag37ge_phy_read(card, state->mPhyBase, kLinkStatus);
        return_value = (register_value & BIT13) == BIT13;
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_full_duplex_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag37ge_get_new_auto_negotiation_complete_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAutoNegotiationComplete); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_auto_negotiation_complete_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_auto_negotiation_complete_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_auto_negotiation_complete_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "auto_neg_complete");
        attribute_set_description(result, "Indicates whether the auto-negotiation process has completed.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag37ge_auto_negotiation_complete_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_auto_negotiation_complete_get_value(AttributePtr attribute)
{
    static uint8_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag37ge_port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        register_value = dag37ge_phy_read(card, state->mPhyBase, kLinkStatus);
        return_value = (register_value & BIT11) == BIT11;
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_auto_negotiation_complete_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag37ge_get_new_jabber_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeJabber); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_jabber_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_jabber_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_jabber_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "jabber");
        attribute_set_description(result, "Indicates whether jabber is being detected.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag37ge_jabber_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_jabber_get_value(AttributePtr attribute)
{
    static uint8_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag37ge_port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        register_value = dag37ge_phy_read(card, state->mPhyBase, kLinkStatus);
        return_value = (register_value & BIT0) == BIT0;
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_jabber_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag37ge_get_new_remote_fault_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRemoteFault); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_remote_fault_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_remote_fault_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_remote_fault_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "remote_fault");
        attribute_set_description(result, "Indicates a fault at the remote end of the link.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        return result;
    }
    return NULL;
}

static void
dag37ge_remote_fault_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_remote_fault_get_value(AttributePtr attribute)
{
    static uint8_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag37ge_port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        register_value = dag37ge_phy_read(card, state->mPhyBase, kBaseTStatus);
        return_value = (register_value & BIT12) == 0;
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_remote_fault_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
dag37ge_get_new_master_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMasterSlave); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_master_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_master_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_master_get_value);
        attribute_set_to_string_routine(result, dag37ge_master_to_string_routine);
        attribute_set_name(result, "master_or_slave");
        attribute_set_description(result, "Set master or slave mode.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag37ge_master_dispose(AttributePtr attribute)
{
}

static void*
dag37ge_master_get_value(AttributePtr attribute)
{
    static master_slave_t return_value;
    uint32_t register_value;
    DagCardPtr card;
    ComponentPtr component;
    dag37ge_port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        register_value = dag37ge_phy_read(card, state->mPhyBase, kBaseTStatus);
        return_value = (register_value & BIT14) == BIT14 ? kMaster:kSlave;
        return (void*)&return_value;
    }
    return NULL;
}

static void
dag37ge_master_to_string_routine(AttributePtr attribute)
{
    master_slave_t ms;
    const char* temp;
    ms = *(master_slave_t*)attribute_get_value(attribute);
    temp = master_slave_to_string(ms);
    attribute_set_to_string(attribute, temp);
}


static void
dag37ge_master_post_initialize(AttributePtr attribute)
{
}
/* Initialization routine. */
dag_err_t
dag37ge_initialize(DagCardPtr card)
{
    dag37ge_state_t* state;
    ComponentPtr root_component;
    ComponentPtr terf;
    
    assert(NULL != card);
    
    /* Set up virtual methods. */
    card_set_dispose_routine(card, dag37ge_dispose);
    card_set_post_initialize_routine(card, dag37ge_post_initialize);
    card_set_reset_routine(card, dag37ge_reset);
    card_set_default_routine(card, dag37ge_default);
    card_set_update_register_base_routine(card, dag37ge_update_register_base);
//    card_set_load_firmware_routine(card, generic_card_load_firmware);

    /* Allocate private state. */
    state = (dag37ge_state_t*) malloc(sizeof(dag37ge_state_t));
    memset(state, 0, sizeof(*state));
    card_set_private_state(card, (void*) state);
    
    /* Create root component. */
    root_component = component_init(kComponentRoot, card);

    /* Add root component to card. */
    card_set_root_component(card, root_component);
    
    /* Add components as appropriate. */
    state->mGpp[0] = gpp_get_new_gpp(card, 0);
    state->mGpp[1] = gpp_get_new_gpp(card, 1);
    state->mPbm = pbm_get_new_pbm(card, 0);
    terf = terf_get_new_terf(card, 0);

    
    state->mPort[0] = dag37ge_get_new_port(card, 0);
    state->mPort[1] = dag37ge_get_new_port(card, 1);
    state->mMux = dag37ge_get_new_mux(card);

    component_add_subcomponent(root_component, state->mGpp[0]);
    component_add_subcomponent(root_component, state->mGpp[1]);
    component_add_subcomponent(root_component, state->mPbm);
    component_add_subcomponent(root_component, state->mPort[0]);
    component_add_subcomponent(root_component, state->mPort[1]);
    component_add_subcomponent(root_component, state->mMux);
    
    component_add_subcomponent(root_component, terf);

    return kDagErrNone;
}

/* drop_count */
static AttributePtr
dag37ge_get_new_drop_count_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDropCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37ge_drop_count_dispose);
        attribute_set_post_initialize_routine(result, dag37ge_drop_count_post_initialize);
        attribute_set_getvalue_routine(result, dag37ge_drop_count_get_value);
        attribute_set_name(result, "drop_count");
        attribute_set_description(result, "A count of the packets dropped on a port");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
dag37ge_drop_count_dispose(AttributePtr attribute)
{

}

static void*
dag37ge_drop_count_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        static uint32_t val = 0;
        dag37ge_port_state_t* state = NULL;
        ComponentPtr port;

        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (dag37ge_port_state_t*)component_get_private_state(port);
        //val = card_read_iom(card, state->mGppBase + (state->mIndex+1)*SP_OFFSET + SP_DROP);
        val = card_read_iom(card, state->mGppBase + SP_OFFSET + SP_DROP);	// Note: mIndex is not needed as there is only one port per GPP on this card
	return (void*)&val;
    }
    return NULL;
}

static void
dag37ge_drop_count_post_initialize(AttributePtr attribute)
{

}

