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
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/cards/dag4_constants.h"
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/components/terf_component.h"
#include "../include/components/sac_component.h"
#include "../include/components/pc_component.h"
#include "../include/components/sc256.h"
#include "../include/components/gpp_component.h"
#include "../include/attribute_factory.h"
#include "../include/components/duck_component.h"
#include "../include/components/general_component.h"
#include "../include/components/card_info_component.h"
#include "../include/util/enum_string_table.h"

/* DAG API files we rely on being installed in a sys include path somewhere */
#include <dagutil.h>
#include <dagapi.h>
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

typedef struct
{
    ComponentPtr mGpp;
    ComponentPtr mPhy;
    ComponentPtr mPbm;
    ComponentPtr *mStream;
    ComponentPtr mPort[2]; 
    ComponentPtr mTerf; 
    ComponentPtr mSac;
    ComponentPtr mPacketCapture;
    ComponentPtr mSC256;

    /* cached addresses */
    uint32_t mPhyBase;
    uint32_t mGppBase;
    uint32_t mPbmBase;
    uint32_t mSacBase;

} dag43ge_state_t;


/* Internal routines. */
/* Determine the port (i.e. PortA or PortB) given an attribute */
static int dag43ge_which_port(AttributePtr attribute);

/* Virtual methods for the card. */
static void dag43ge_dispose(DagCardPtr card);
static int dag43ge_post_initialize(DagCardPtr card);
static void dag43ge_reset(DagCardPtr card);
static void dag43ge_default(DagCardPtr card);
static dag_err_t dag43ge_update_register_base(DagCardPtr card);

/* phy component. */
static ComponentPtr dag43ge_get_new_phy(DagCardPtr card);
static void dag43ge_phy_dispose(ComponentPtr component);
static void dag43ge_phy_reset(ComponentPtr component);
static void dag43ge_phy_default(ComponentPtr component);
static int dag43ge_phy_post_initialize(ComponentPtr component);
static dag_err_t dag43ge_phy_update_register_base(ComponentPtr component);

/* port component. */
static ComponentPtr dag43ge_get_new_port(DagCardPtr card);
static void dag43ge_port_dispose(ComponentPtr component);
static void dag43ge_port_reset(ComponentPtr component);
static void dag43ge_port_default(ComponentPtr component);
static int dag43ge_port_post_initialize(ComponentPtr component);
static dag_err_t dag43ge_port_update_register_base(ComponentPtr component);

/* byte_count attribute. */
static AttributePtr dag43ge_get_new_byte_count_attribute(void);
static void dag43ge_byte_count_dispose(AttributePtr attribute);
static void* dag43ge_byte_count_get_value(AttributePtr attribute);
static void dag43ge_byte_count_set_value(AttributePtr attribute, void* value, int length);
static void dag43ge_byte_count_post_initialize(AttributePtr attribute);

/* eql attribute. */
static AttributePtr dag43ge_get_new_eql_attribute(void);
static void dag43ge_eql_dispose(AttributePtr attribute);
static void* dag43ge_eql_get_value(AttributePtr attribute);
static void dag43ge_eql_set_value(AttributePtr attribute, void* value, int length);
static void dag43ge_eql_post_initialize(AttributePtr attribute);

/* nic attribute. */
static AttributePtr dag43ge_get_new_nic_attribute(void);
static void dag43ge_nic_dispose(AttributePtr attribute);
static void* dag43ge_nic_get_value(AttributePtr attribute);
static void dag43ge_nic_set_value(AttributePtr attribute, void* value, int length);
static void dag43ge_nic_post_initialize(AttributePtr attribute);

/* active attribute. */
static AttributePtr dag43ge_get_new_active_attribute(void);
static void dag43ge_active_dispose(AttributePtr attribute);
static void* dag43ge_active_get_value(AttributePtr attribute);
static void dag43ge_active_set_value(AttributePtr attribute, void* value, int length);
static void dag43ge_active_post_initialize(AttributePtr attribute);

/* txpkts attribute. */
static AttributePtr dag43ge_get_new_txpkts_attribute(void);
static void dag43ge_txpkts_dispose(AttributePtr attribute);
static void* dag43ge_txpkts_get_value(AttributePtr attribute);
static void dag43ge_txpkts_set_value(AttributePtr attribute, void* value, int length);
static void dag43ge_txpkts_post_initialize(AttributePtr attribute);

/* rxpkts attribute. */
static AttributePtr dag43ge_get_new_rxpkts_attribute(void);
static void dag43ge_rxpkts_dispose(AttributePtr attribute);
static void* dag43ge_rxpkts_get_value(AttributePtr attribute);
static void dag43ge_rxpkts_set_value(AttributePtr attribute, void* value, int length);
static void dag43ge_rxpkts_post_initialize(AttributePtr attribute);

/* sync attribute */
static AttributePtr dag43ge_get_new_sync_attribute(void);
static void dag43ge_sync_dispose(AttributePtr attribute);
static void* dag43ge_sync_get_value(AttributePtr attribute);
static void dag43ge_sync_post_initialize(AttributePtr attribute);

/* link attribute */
static AttributePtr dag43ge_get_new_link_attribute(void);
static void dag43ge_link_dispose(AttributePtr attribute);
static void* dag43ge_link_get_value(AttributePtr attribute);
static void dag43ge_link_post_initialize(AttributePtr attribute);

/* auto attribute */
static AttributePtr dag43ge_get_new_auto_attribute(void);
static void dag43ge_auto_dispose(AttributePtr attribute);
static void* dag43ge_auto_get_value(AttributePtr attribute);
static void dag43ge_auto_post_initialize(AttributePtr attribute);

/* rflt attribute */
static AttributePtr dag43ge_get_new_rflt_attribute(void);
static void dag43ge_rflt_dispose(AttributePtr attribute);
static void* dag43ge_rflt_get_value(AttributePtr attribute);
static void dag43ge_rflt_post_initialize(AttributePtr attribute);

/* bad_symb attribute */
static AttributePtr dag43ge_get_new_bad_symbol_attribute(void);
static void dag43ge_bad_symbol_dispose(AttributePtr attribute);
static void* dag43ge_bad_symbol_get_value(AttributePtr attribute);
static void dag43ge_bad_symbol_post_initialize(AttributePtr attribute);

/* rx_bytes attribute */
static AttributePtr dag43ge_get_new_rx_bytes_attribute(void);
static void dag43ge_rx_bytes_dispose(AttributePtr attribute);
static void* dag43ge_rx_bytes_get_value(AttributePtr attribute);
static void dag43ge_rx_bytes_post_initialize(AttributePtr attribute);

/* crc_fail attribute */
static AttributePtr dag43ge_get_new_crc_fail_attribute(void);
static void dag43ge_crc_fail_dispose(AttributePtr attribute);
static void* dag43ge_crc_fail_get_value(AttributePtr attribute);
static void dag43ge_crc_fail_post_initialize(AttributePtr attribute);

/* rx_frames attribute */
static AttributePtr dag43ge_get_new_rx_frames_attribute(void);
static void dag43ge_rx_frames_dispose(AttributePtr attribute);
static void* dag43ge_rx_frames_get_value(AttributePtr attribute);
static void dag43ge_rx_frames_post_initialize(AttributePtr attribute);

/* tx_frames attribute */
static AttributePtr dag43ge_get_new_tx_frames_attribute(void);
static void dag43ge_tx_frames_dispose(AttributePtr attribute);
static void* dag43ge_tx_frames_get_value(AttributePtr attribute);
static void dag43ge_tx_frames_post_initialize(AttributePtr attribute);

/* tx_bytes attribute */
static AttributePtr dag43ge_get_new_tx_bytes_attribute(void);
static void dag43ge_tx_bytes_dispose(AttributePtr attribute);
static void* dag43ge_tx_bytes_get_value(AttributePtr attribute);
static void dag43ge_tx_bytes_post_initialize(AttributePtr attribute);

/* internal_mac_error attribute */
static AttributePtr dag43ge_get_new_internal_mac_error_attribute(void);
static void dag43ge_internal_mac_error_dispose(AttributePtr attribute);
static void* dag43ge_internal_mac_error_get_value(AttributePtr attribute);
static void dag43ge_internal_mac_error_post_initialize(AttributePtr attribute);

/* transmit_system_error attribute */
static AttributePtr dag43ge_get_new_transmit_system_error_attribute(void);
static void dag43ge_transmit_system_error_dispose(AttributePtr attribute);
static void* dag43ge_transmit_system_error_get_value(AttributePtr attribute);
static void dag43ge_transmit_system_error_post_initialize(AttributePtr attribute);

/* drop_count */
static AttributePtr dag43ge_get_new_drop_count_attribute(void);
static void dag43ge_drop_count_dispose(AttributePtr attribute);
static void* dag43ge_drop_count_get_value(AttributePtr attribute);
static void dag43ge_drop_count_post_initialize(AttributePtr attribute);

/* linerate attribute - dummy */
static AttributePtr dag43ge_get_new_linerate_attribute(void);
static void dag43ge_linerate_dispose(AttributePtr attribute);
static void* dag43ge_linerate_get_value(AttributePtr attribute);
static void dag43ge_linerate_post_initialize(AttributePtr attribute);
static void dag43ge_linerate_set_value(AttributePtr attribute, void* value, int length);
static void line_rate_to_string_routine(AttributePtr attribute);

/* suppress_error */
static AttributePtr dag43ge_get_new_suppress_error_attribute(void);
static void dag43ge_suppress_error_dispose(AttributePtr attribute);
static void* dag43ge_suppress_error_get_value(AttributePtr attribute);
static void dag43ge_suppress_error_set_value(AttributePtr attribute, void* value, int length);
static void dag43ge_suppress_error_post_initialize(AttributePtr attribute);

/* Implementation of internal routines. */
static int
dag43ge_which_port(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr component = NULL;
    ComponentPtr root_component = NULL;
    ComponentPtr port_component = NULL;
    int which_port = 0;
    dag43ge_state_t* state = NULL;

    component = attribute_get_component(attribute);
    card = attribute_get_card(attribute);
    root_component = card_get_root_component(card);
    state = (dag43ge_state_t*)card_get_private_state(card);
    /* Find out the index of the component. Based on that we know which port to read from*/
    port_component = component_get_subcomponent(root_component, kComponentPort, 0);
    if (port_component != component)
    {
        port_component = component_get_subcomponent(root_component, kComponentPort, 1);
        which_port = 1;
    }
    assert(port_component == component);
    return which_port;
}

static void
dag43ge_dispose(DagCardPtr card)
{
    ComponentPtr component;
    ComponentPtr root;
    AttributePtr attribute;
    int component_count = 0;
    int i = 0;
    dag43ge_state_t* state = NULL;

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
    state = (dag43ge_state_t*) card_get_private_state(card);
    
    assert(NULL != state);

    assert(NULL != state->mStream);
    free(state->mStream);
    
    free(state);
    card_set_private_state(card, NULL);

}


static int
dag43ge_post_initialize(DagCardPtr card)
{    
    int rx_streams = 0;
    int tx_streams = 0;
    int streammax = 0;
    int dagfd = 0;
    int x = 0;
    dag43ge_state_t* state = (dag43ge_state_t*)card_get_private_state(card);
    ComponentPtr root = card_get_root_component(card);
    AttributePtr promMode;
    GenericReadWritePtr grw;
    uint32_t bit_mask = 0xFFFFFFFF;
    uintptr_t address = 0;
    AttributePtr attr;

    /* Find out the number of streams present */
    dagfd = card_get_fd(card);
    /* Get the number of RX Streams */
    rx_streams = dag_rx_get_stream_count(dagfd);
    tx_streams = dag_tx_get_stream_count(dagfd);
    streammax = dagutil_max(rx_streams*2 - 1, tx_streams*2);
    if (streammax <= 0)
        streammax = 0;
    else
    {
        state->mStream = malloc(sizeof(state->mStream)*streammax);
        assert(state->mStream != NULL);
    }
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
    if (card_get_register_address(card, DAG_REG_STATS, 0))
    {
        state->mSac = sac_get_new_component(card);    
        component_add_subcomponent(root, state->mSac);
    }

    if (card_get_register_address(card, DAG_REG_RANDOM_CAPTURE, 0))
    {
        state->mPacketCapture = pc_get_new_component(card);
        component_add_subcomponent(root, state->mPacketCapture);
    }           

    if (card_get_register_address(card, DAG_REG_IDT_TCAM, 0))
    {
        state->mSC256 = sc256_get_new_component(card);
        component_add_subcomponent(root, state->mSC256);
    }

    if (card_get_register_address(card, DAG_REG_DUCK, 0))
    {
        component_add_subcomponent(root, duck_get_new_component(card, 0));
    }

 
    state->mPhyBase = card_get_register_address(card, DAG_REG_PM3386, 0);
    state->mGppBase = card_get_register_address(card, DAG_REG_GPP, 0);
    state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);

    /* Add promisc mode */
    bit_mask = BIT1;
    address = (uintptr_t) card_get_iom_address(card) + state->mPhyBase + (P3386_EGMAC_ADDR_FILTER_CTRL2_A << 2) ;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    promMode= attribute_factory_make_attribute(kBooleanAttributePromiscuousMode, grw, &bit_mask,1);
    component_add_attribute(state->mPort[0], promMode);        
    bit_mask = BIT1;
    address = (uintptr_t) card_get_iom_address(card) + state->mPhyBase + (P3386_EGMAC_ADDR_FILTER_CTRL2_B << 2) ;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    promMode= attribute_factory_make_attribute(kBooleanAttributePromiscuousMode, grw, &bit_mask,1);
    component_add_attribute(state->mPort[1], promMode);          

    /* port a latch attributes */
    bit_mask = BIT0 | BIT1;
    address = (uintptr_t) card_get_iom_address(card) + state->mPhyBase + (P3386_MSTAT_CTRL_A << 2) ;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCounterLatch, grw, &bit_mask, 1);
    component_add_attribute(state->mPort[0], attr);

    /* port b latch attributes */
    bit_mask = BIT0 | BIT1;
    address = (uintptr_t) card_get_iom_address(card) + state->mPhyBase + (P3386_MSTAT_CTRL_B << 2) ;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCounterLatch, grw, &bit_mask, 1);
    component_add_attribute(state->mPort[1], attr);
    /* Add card info component */
    component_add_subcomponent(root, card_info_get_new_component(card, 0));
    /* Return 1 if standard post_initialize() should continue, 0 if not.
    * "Standard" post_initialize() calls post_initialize() on the card's root component.
    */
    return 1;
}

static dag_err_t
dag43ge_phy_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = NULL;        

        card = component_get_card(component);
        return dag43ge_update_register_base(card);
    }
    return kDagErrInvalidParameter;
}

static void
dag43ge_reset(DagCardPtr card)
{

}


static dag_err_t
dag43ge_update_register_base(DagCardPtr card)
{
    if (1 == valid_card(card))
    {
        dag43ge_state_t* state = NULL;
        
        state = card_get_private_state(card);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mPhyBase = card_get_register_address(card, DAG_REG_PM3386, 0);
        state->mGppBase = card_get_register_address(card, DAG_REG_GPP, 0);
        state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static void
dag43ge_default(DagCardPtr card)
{
    int i = 0;
    uint32_t val = 0;
    dag43ge_state_t* state = (dag43ge_state_t*)card_get_private_state(card);
	ComponentPtr root;
	ComponentPtr pbm;
    ComponentPtr port;

    card_reset_datapath(card);
	root = card_get_root_component(card);
	pbm = component_get_subcomponent(root, kComponentPbm, 0);
    /*card_pbm_default(card);*/
    component_default(pbm);
    
    /* software reset function */
    val = card_read_iom(card, state->mPhyBase + (P3386_RESET_CTRL << 2));
    val &= ~(BIT0|BIT1);
    card_write_iom(card, state->mPhyBase + (P3386_RESET_CTRL << 2), val);
    usleep(1000);
    val |= BIT1;
    card_write_iom(card, state->mPhyBase + (P3386_RESET_CTRL << 2), val);
    usleep(10000);
    val |= BIT0;
    card_write_iom(card, state->mPhyBase + (P3386_RESET_CTRL << 2), val);

    /* TX setup - CRCEN, LONGP */
    val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_L_A << 2));
    val |= (BIT12|BIT4);
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_L_A << 2), val);

    val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_L_B << 2));
    val |= (BIT12|BIT4);
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_L_B << 2), val);

    /* port A and B rx/tx enables */
    val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2));
    val |= (BIT12|BIT14);
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2), val);

    val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2));
    val |= (BIT12|BIT14);
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2), val);

    /* port A and B AUTOS */
    val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GPCSC_L_A << 2));
    val |= BIT8;
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GPCSC_L_A << 2), val);

    val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GPCSC_L_B << 2));
    val |= BIT8;
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GPCSC_L_B << 2), val);

    /* EGMAC RX FIFO Forwarding Thresholds to 1 32-bit word */
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_RX_FIFO_A << 2), 1);
    card_write_iom(card, state->mPhyBase + (P3386_EGMAC_RX_FIFO_B << 2),  1);

    for (i = 0; i < component_get_subcomponent_count_of_type(root, kComponentPort); i++)
    {
        port = component_get_subcomponent(root, kComponentPort, i);
        component_default(port);
    }

}

static ComponentPtr
dag43ge_get_new_phy(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentPhy, card); 
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag43ge_phy_dispose);
        component_set_post_initialize_routine(result, dag43ge_phy_post_initialize);
        component_set_reset_routine(result, dag43ge_phy_reset);
        component_set_default_routine(result, dag43ge_phy_default);
        component_set_update_register_base_routine(result, dag43ge_phy_update_register_base);
        component_set_name(result, "phy");
    }
    
    return result;
}

static void
dag43ge_phy_dispose(ComponentPtr component)
{
    /* FIXME: unimplemented routine. */
}


static int
dag43ge_phy_post_initialize(ComponentPtr component)
{
    /* Return 1 if standard post_initialize() should continue, 0 if not.
    * "Standard" post_initialize() calls post_initialize() recursively on subcomponents.
    */
    return 1;
}


static void
dag43ge_phy_reset(ComponentPtr component)
{
    /* FIXME: unimplemented routine. */
}


static void
dag43ge_phy_default(ComponentPtr component)
{
    /* FIXME: unimplemented routine. */
}

static ComponentPtr
dag43ge_get_new_port(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentPort, card);

    if (NULL != result)
    {
        component_set_dispose_routine(result, dag43ge_port_dispose);
        component_set_reset_routine(result, dag43ge_port_reset);
        component_set_default_routine(result, dag43ge_port_default);
        component_set_post_initialize_routine(result, dag43ge_port_post_initialize);
        component_set_update_register_base_routine(result, dag43ge_port_update_register_base);
        component_set_name(result, "port");
    }

    return result;
}

static void
dag43ge_port_dispose(ComponentPtr component)
{

}

static void
dag43ge_port_reset(ComponentPtr component)
{

}

static void
dag43ge_port_default(ComponentPtr component)
{
    AttributePtr attr = NULL;
    char * mac_buf = NULL;
    uint64_t mac = 0;
    uint32_t a,b,c,d,e,f;
    DagCardPtr card = NULL;
    dag43ge_state_t * state = NULL;
    uint8_t value = 0;

    if (1 == valid_component(component))
    {
        card = component_get_card(component);
        state = card_get_private_state(card);
        attr = component_get_attribute(component, kStringAttributeEthernetMACAddress);
        mac_buf = (char *)attribute_get_value(attr);

        sscanf(mac_buf, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
        mac = ((((uint64_t)a)&0xff)<<40) +
              ((((uint64_t)b)&0xff)<<32) +
              ((((uint64_t)c)&0xff)<<24) +
              ((((uint64_t)d)&0xff)<<16) +
              ((((uint64_t)e)&0xff)<<8) +
              (((uint64_t)f)&0xff);    
        
	printf("Setting MAC address\n");
        if (dag43ge_which_port(attr) == 0)
        {
             card_write_iom(card, state->mPhyBase + (P3386_EGMAC_STATION_ADDR_A << 2), (uint32_t)mac&0xffff);
             card_write_iom(card, state->mPhyBase + ((P3386_EGMAC_STATION_ADDR_A + 2 ) << 2), (uint32_t)(mac>>16)&0xffff);
             card_write_iom(card, state->mPhyBase + ((P3386_EGMAC_STATION_ADDR_A + 4 ) << 2), (uint32_t)(mac>>32)&0xffff);
	
	
	}
        else
        {
             card_write_iom(card, state->mPhyBase + (P3386_EGMAC_STATION_ADDR_B << 2), (uint32_t)mac&0xffff);
             card_write_iom(card, state->mPhyBase + ((P3386_EGMAC_STATION_ADDR_B + 2 ) << 2), (uint32_t)(mac>>16)&0xffff);
             card_write_iom(card, state->mPhyBase + ((P3386_EGMAC_STATION_ADDR_B + 4 ) << 2), (uint32_t)(mac>>32)&0xffff);
	
	
	}

        attr = component_get_attribute(component, kBooleanAttributePromiscuousMode);
        value = 1;
        dag_config_set_boolean_attribute((dag_card_ref_t)card, (attr_uuid_t)attr, value); 
    }


}

static int
dag43ge_port_post_initialize(ComponentPtr component)
{
    return 1;
}

static dag_err_t
dag43ge_port_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = NULL;
        card = component_get_card(component);
        return dag43ge_update_register_base(card);
    }
    return kDagErrInvalidParameter;
}

static AttributePtr
dag43ge_get_new_byte_count_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMaxPktLen); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_byte_count_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_byte_count_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_byte_count_get_value);
        attribute_set_setvalue_routine(result, dag43ge_byte_count_set_value);
        attribute_set_name(result, "max_pkt_len");
        attribute_set_description(result, "Maximum number of RX bytes allowed per packet. Any RX packet longer will be discarded. This is in the framer");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
    }
    
    return result;
}


static void
dag43ge_byte_count_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag43ge_byte_count_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void*
dag43ge_byte_count_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_RX_MAXFR_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_RX_MAXFR_B << 2));
        }
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }

    return NULL;
}


static void
dag43ge_byte_count_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = *(uint32_t*)value;
		uint32_t reg = 0, reg_val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
		reg = card_get_register_address(card, DAG_REG_GENERAL, 0);
        state = (dag43ge_state_t*)card_get_private_state(card);
		reg_val = card_read_iom(card, reg + 0x0c);
        /* if Suppress Error is enable */
		/* then MAX_LEN is locked to 9600 */
		if ((reg_val & BIT28) == 1)
			printf("WARNING: MAX_LEN is locked to 9600 if \"suppresserror\" is enabled\n");			
		else
		{
			if (dag43ge_which_port(attribute) == 0)
			{
				card_write_iom(card, state->mPhyBase + (P3386_EGMAC_RX_MAXFR_A << 2), val);
				card_write_iom(card, state->mPhyBase + (P3386_EGMAC_TX_MAXFR_A << 2), val);
			}
			else
			{
				card_write_iom(card, state->mPhyBase + (P3386_EGMAC_RX_MAXFR_B << 2), val);
				card_write_iom(card, state->mPhyBase + (P3386_EGMAC_TX_MAXFR_B << 2), val);
			}
		}
    }
}

static AttributePtr
dag43ge_get_new_eql_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeEquipmentLoopback); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_eql_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_eql_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_eql_get_value);
        attribute_set_setvalue_routine(result, dag43ge_eql_set_value);
        attribute_set_name(result, "eql");
        attribute_set_description(result, "Turn equipment loopback on or off");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}


static void
dag43ge_eql_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag43ge_eql_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}


static void*
dag43ge_eql_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_SERDES_PORT_CONF_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_SERDES_PORT_CONF_B << 2));
        }
        val = (BIT5 & val) >> 5;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    
    return NULL;
}


static void
dag43ge_eql_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        int val = 0;
        int mask = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_SERDES_PORT_CONF_A << 2));
            if (0 == *(uint8_t*)value)
            {
                mask = 0xffffffdf;
                val &= mask;
            }
            else
            {
                mask = 0x20;
                val |= mask;
            }
            card_write_iom(card, state->mPhyBase + (P3386_SERDES_PORT_CONF_A << 2), val);
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_SERDES_PORT_CONF_B << 2));
            if (0 == *(uint8_t*)value)
            {
                mask = 0xffffffdf;
                val &= mask;
            }
            else
            {
                mask = 0x20;
                val |= mask;
            }
            card_write_iom(card, state->mPhyBase + (P3386_SERDES_PORT_CONF_B << 2), val);
        }
    }
}


static AttributePtr
dag43ge_get_new_nic_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeNic); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_nic_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_nic_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_nic_get_value);
        attribute_set_setvalue_routine(result, dag43ge_nic_set_value);
        attribute_set_name(result, "auto_neg");
        attribute_set_description(result, "Enable or disable ethernet auto-negotiation");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}


static void
dag43ge_nic_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag43ge_nic_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag43ge_nic_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        int val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANCTL_A << 2));
            if (0 == *(uint8_t*)value)
            {
                val &= ~BIT1;
            }
            else
            {
                val |= BIT1;
            }
            card_write_iom(card, state->mPhyBase + (P3386_EGMAC_ANCTL_A << 2), val);
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANCTL_B << 2));
            if (0 == *(uint8_t*)value)
            {
                val &= ~BIT1;
            }
            else
            {
                val |= BIT1;
            }
            card_write_iom(card, state->mPhyBase + (P3386_EGMAC_ANCTL_B << 2), val);
        }
    }
}


static void*
dag43ge_nic_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANCTL_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANCTL_B << 2));
        }
        val = (val & BIT1) >> 1;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
dag43ge_get_new_active_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_active_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_active_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_active_get_value);
        attribute_set_setvalue_routine(result, dag43ge_active_set_value);
        attribute_set_name(result, "active");
        attribute_set_description(result, "Enable or disable the port");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }

    return result;
}

static void
dag43ge_active_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag43ge_active_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
dag43ge_active_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);

        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mGppBase + GPP_CONFIG);
            /* stream processor config registers exist? */
            if (val & BIT4)
            {
                val = card_read_iom(card, state->mGppBase + SP_CONFIG + SP_OFFSET);
                /* The bit is set if the stream is inactive. However this function retrievs the active value,
                not the inactive value. Therefore we negate the result. */
                val = !((val & BIT12) >> 12);
                attribute_set_value_array(attribute, &val, sizeof(val));
                return (void *)attribute_get_value_array(attribute);
            }
        }
        else
        {
            val = card_read_iom(card, state->mGppBase + GPP_CONFIG);
            /* stream processor config registers exist? */
            if (val & BIT4)
            {
                val = card_read_iom(card, state->mGppBase + SP_CONFIG + SP_OFFSET*2);
                /* The bit is set if the stream is inactive. However this function retrievs the active value,
                not the inactive value. Therefore we negate the result. */
                val = !((val & BIT12) >> 12);
                attribute_set_value_array(attribute, &val, sizeof(val));
                return (void *)attribute_get_value_array(attribute);
            }
        }
    }
    return NULL;
}

static void
dag43ge_active_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        int val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);

        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mGppBase + GPP_CONFIG);
            /* stream processor config registers exist? */
            if (val & BIT4)
            {
                val |= BIT5; /* enable per-stream config registers */
                card_write_iom(card, state->mGppBase + GPP_CONFIG, val);
                val = card_read_iom(card, state->mGppBase + SP_CONFIG + SP_OFFSET);
                /* The bit is set if the stream is inactive. */
                if (0 == *(uint8_t*)value)
                {
                    val |= BIT12;
                }
                else
                {
                    val &= ~BIT12;
                }
                card_write_iom(card, state->mGppBase + SP_CONFIG + SP_OFFSET, val);
            }
        }
        else
        {
            val = card_read_iom(card, state->mGppBase + GPP_CONFIG);
            /* stream processor config registers exist? */
            if (val & BIT4)
            {
                val |= BIT5; /* enable per-stream config registers */
                card_write_iom(card, state->mGppBase + GPP_CONFIG, val);
                val = card_read_iom(card, state->mGppBase + SP_CONFIG + SP_OFFSET * 2);
                if (0 == *(uint8_t*)value)
                {
                    val |= BIT12;
                }
                else
                {
                    val &= ~BIT12;
                }
                card_write_iom(card, state->mGppBase + SP_CONFIG + SP_OFFSET * 2, val);
            }
        }
    }
}

/* rxpkts attribute. */
static AttributePtr
dag43ge_get_new_rxpkts_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRxPkts); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_rxpkts_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_rxpkts_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_rxpkts_get_value);
        attribute_set_setvalue_routine(result, dag43ge_rxpkts_set_value);
        attribute_set_name(result, "rxpkts");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
dag43ge_rxpkts_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag43ge_rxpkts_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);

        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2));
        }
        val = (BIT12 & val) >> 12;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_rxpkts_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        int val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2));
            if (0 == *(uint8_t*)value)
            {
                val &= ~BIT12;
            }
            else
            {
                val |= BIT12;
            }
            card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2), val);
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2));
            if (0 == *(uint8_t*)value)
            {
                val &= ~BIT12;
            }
            else
            {
                val |= BIT12;
            }
            card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2), val);
        }
    }
}


static void
dag43ge_rxpkts_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* txpkts attribute. */
static AttributePtr
dag43ge_get_new_txpkts_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeTxPkts); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_txpkts_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_txpkts_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_txpkts_get_value);
        attribute_set_setvalue_routine(result, dag43ge_txpkts_set_value);
        attribute_set_name(result, "txpkts");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
dag43ge_txpkts_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag43ge_txpkts_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);

        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2));
        }
        val = (BIT14 & val) >> 14;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_txpkts_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        int val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2));
            if (0 == *(uint8_t*)value)
            {
                val &= ~BIT14;
            }
            else
            {
                val |= BIT14;
            }
            card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_A << 2), val);
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2));
            if (0 == *(uint8_t*)value)
            {
                val &= ~BIT14;
            }
            else
            {
                val |= BIT14;
            }
            card_write_iom(card, state->mPhyBase + (P3386_EGMAC_GMACC1_H_B << 2), val);
        }
    }
}

static void
dag43ge_txpkts_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* sync attribute. */
static AttributePtr
dag43ge_get_new_sync_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeSync); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_sync_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_sync_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_sync_get_value);
        attribute_set_name(result, "sync");
        attribute_set_description(result, "Indicates the synchronization status.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag43ge_sync_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag43ge_sync_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_B << 2));
        }
        val = (val & BIT0) != 0;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_sync_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* link attribute. */
static AttributePtr
dag43ge_get_new_link_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLink); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_link_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_link_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_link_get_value);
        attribute_set_name(result, "link");
        attribute_set_description(result, "Indicates that the link is ok. In general if there is sync then the link will be ok.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag43ge_link_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag43ge_link_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_B << 2));
        }
        val = (val & BIT1) != 0;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_link_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* auto attribute. */
static AttributePtr
dag43ge_get_new_auto_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAutoNegotiationComplete);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_auto_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_auto_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_auto_get_value);
        attribute_set_name(result, "auto");
        attribute_set_description(result, "When nic is enabled this indicates if auto-negotiation has completed.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag43ge_auto_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag43ge_auto_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_B << 2));
        }
        val = (val & BIT3) != 0;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_auto_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* rflt attribute. */
static AttributePtr
dag43ge_get_new_rflt_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRemoteFault); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_rflt_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_rflt_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_rflt_get_value);
        attribute_set_name(result, "remote_fault");
        attribute_set_description(result, "The remote fault indicator. There is something wrong at the remote end.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag43ge_rflt_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag43ge_rflt_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_A << 2));
        }
        else
        {
            val = card_read_iom(card, state->mPhyBase + (P3386_EGMAC_ANSTT_B << 2));
        }
        val = (val & BIT4) != 0;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_rflt_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* bad_symbol attribute. */
static AttributePtr
dag43ge_get_new_bad_symbol_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeBadSymbol); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_bad_symbol_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_bad_symbol_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_bad_symbol_get_value);
        attribute_set_name(result, "bad_symbol");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_bad_symbol_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_bad_symbol_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        dag43ge_state_t* state = NULL;
        uint64_t val = 0;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_A << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_SYMB_ERR_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_SYMB_ERR_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_SYMB_ERR_A << 2));
        }
        else
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_B << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_SYMB_ERR_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_SYMB_ERR_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_SYMB_ERR_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_bad_symbol_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* rx_bytes attribute. */
static AttributePtr
dag43ge_get_new_rx_bytes_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeRxBytes); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_rx_bytes_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_rx_bytes_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_rx_bytes_get_value);
        attribute_set_name(result, "rx_bytes");
        attribute_set_description(result, "The number of bytes successfully received.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_rx_bytes_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_rx_bytes_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint64_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_A << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_OCTETS_OK_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_OCTETS_OK_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_OCTETS_OK_A << 2));
        }
        else
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_B << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_OCTETS_OK_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_OCTETS_OK_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_OCTETS_OK_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_rx_bytes_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* crc_fail attribute. */
static AttributePtr
dag43ge_get_new_crc_fail_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeCRCFail); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_crc_fail_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_crc_fail_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_crc_fail_get_value);
        attribute_set_name(result, "fcs_errors");
        attribute_set_description(result, "A count of the frames received that do not pass the FCS check");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_crc_fail_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_crc_fail_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint64_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FCS_ERR_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FCS_ERR_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_FCS_ERR_A << 2));
        }
        else
        {
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FCS_ERR_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FCS_ERR_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_FCS_ERR_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_crc_fail_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* rx frames attribute. */
static AttributePtr
dag43ge_get_new_rx_frames_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeRxFrames); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_rx_frames_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_rx_frames_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_rx_frames_get_value);
        attribute_set_name(result, "rx_frames");
        attribute_set_description(result, "Count of valid frames received");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_rx_frames_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_rx_frames_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint64_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_A << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FRAMES_OK_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FRAMES_OK_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_FRAMES_OK_A << 2));
        }
        else
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_B << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FRAMES_OK_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_FRAMES_OK_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_FRAMES_OK_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_rx_frames_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* tx frames attribute. */
static AttributePtr
dag43ge_get_new_tx_frames_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeTxFrames); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_tx_frames_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_tx_frames_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_tx_frames_get_value);
        attribute_set_name(result, "tx_frames");
        attribute_set_description(result, "Count of valid frames transmitted.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_tx_frames_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_tx_frames_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint64_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_A << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_FRAMES_OK_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_FRAMES_OK_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_FRAMES_OK_A << 2));
        }
        else
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_B << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_FRAMES_OK_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_FRAMES_OK_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_FRAMES_OK_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_tx_frames_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* tx_bytes attribute. */
static AttributePtr
dag43ge_get_new_tx_bytes_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeTxBytes); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_tx_bytes_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_tx_bytes_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_tx_bytes_get_value);
        attribute_set_name(result, "tx_bytes");
        attribute_set_description(result, "Count of the bytes transmitted");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_tx_bytes_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_tx_bytes_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint64_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_A << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_OCTETS_OK_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_OCTETS_OK_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_OCTETS_OK_A << 2));
        }
        else
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_B << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_OCTETS_OK_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_OCTETS_OK_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_OCTETS_OK_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_tx_bytes_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* internal_mac_error attribute. */
static AttributePtr
dag43ge_get_new_internal_mac_error_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeInternalMACError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_internal_mac_error_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_internal_mac_error_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_internal_mac_error_get_value);
        attribute_set_name(result, "internal_mac_error");
        attribute_set_description(result, "A count of the frames that could not be sent correctly");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_internal_mac_error_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_internal_mac_error_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint64_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_A << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_INTERR_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_INTERR_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_INTERR_A << 2));
        }
        else
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_B << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_INTERR_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_INTERR_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_INTERR_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_internal_mac_error_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* transmit_system_error attribute. */
static AttributePtr
dag43ge_get_new_transmit_system_error_attribute(void)
{
    AttributePtr result = attribute_init(kUint64AttributeTransmitSystemError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_transmit_system_error_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_transmit_system_error_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_transmit_system_error_get_value);
        attribute_set_name(result, "transmit_system_error");
        attribute_set_description(result, "A count of the frames that could not be sent correctly due to various errors. Frames that are already counted by kUint64AttributeInternalMACError are not included in this count.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
    }
    
    return result;
}

static void
dag43ge_transmit_system_error_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void*
dag43ge_transmit_system_error_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint64_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_A << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_SYSERR_A + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_SYSERR_A + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_SYSERR_A << 2));
        }
        else
        {
            /* latch the stats counters */
            /*card_write_iom(card, state->mPhyBase + (P3386_MSTAT_CTRL_B << 2), (BIT0|BIT1));*/
            val = (uint64_t)card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_SYSERR_B + 2) << 2)) << 32 |
                    card_read_iom(card, state->mPhyBase + ((P3386_MSTAT_TX_SYSERR_B + 1) << 2)) << 16 |
                    card_read_iom(card, state->mPhyBase + (P3386_MSTAT_TX_SYSERR_B << 2));
        }
        val = val & 0xffffffffffll; /* Only 40 bits are valid hence the bit mask */
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_transmit_system_error_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

/* drop_count */
static AttributePtr
dag43ge_get_new_drop_count_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDropCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_drop_count_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_drop_count_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_drop_count_get_value);
        attribute_set_name(result, "drop_count");
        attribute_set_description(result, "A count of the packets dropped on a port");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
dag43ge_drop_count_dispose(AttributePtr attribute)
{

}

static void*
dag43ge_drop_count_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t val = 0;
        dag43ge_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag43ge_state_t*)card_get_private_state(card);
        if (dag43ge_which_port(attribute) == 0)
        {
            val = card_read_iom(card, state->mGppBase + SP_OFFSET + SP_DROP);
        }
        else
        {
            val = card_read_iom(card, state->mGppBase + 2*SP_OFFSET + SP_DROP);
        }
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_drop_count_post_initialize(AttributePtr attribute)
{

}

/* suppress_error */
static AttributePtr
dag43ge_get_new_suppress_error_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeSuppressError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_suppress_error_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_suppress_error_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_suppress_error_get_value);
        attribute_set_setvalue_routine(result, dag43ge_suppress_error_set_value);
        attribute_set_name(result, "suppress_error");
        attribute_set_description(result, "Suppress most of error");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
dag43ge_suppress_error_dispose(AttributePtr attribute)
{

}

static void*
dag43ge_suppress_error_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t val = 0;
		uint32_t reg = 0;
		
        card = attribute_get_card(attribute);
		reg = card_get_register_address(card, DAG_REG_GENERAL, 0);
		val = card_read_iom(card, reg + 0x0c);
        
        val = (BIT28 & val) >> 28;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
dag43ge_suppress_error_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        int val = 0;
		dag43ge_state_t* state = NULL;
		uint32_t reg = 0;
		
        card = attribute_get_card(attribute);
		reg = card_get_register_address(card, DAG_REG_GENERAL, 0);
        state = (dag43ge_state_t*)card_get_private_state(card);
		val = card_read_iom(card, reg + 0x0c);
		
        if (0 == *(uint8_t*)value)
			val &= ~BIT28;
        else
		{
			val |= BIT28;
			/* then set number of bytes per packet to 9600 */
			card_write_iom(card, state->mPhyBase + (P3386_EGMAC_RX_MAXFR_A << 2), 9600);
			card_write_iom(card, state->mPhyBase + (P3386_EGMAC_TX_MAXFR_A << 2), 9600);
			
			card_write_iom(card, state->mPhyBase + (P3386_EGMAC_RX_MAXFR_B << 2), 9600);
			card_write_iom(card, state->mPhyBase + (P3386_EGMAC_TX_MAXFR_B << 2), 9600);
		}
        card_write_iom(card, reg + 0x0c, val);
    }
}

static void
dag43ge_suppress_error_post_initialize(AttributePtr attribute)
{

}

/* Initialization routine. */
dag_err_t
dag43ge_initialize(DagCardPtr card)
{
    dag43ge_state_t* state;
    ComponentPtr root_component, general;
    uint32_t address = 0;

	
    /* Port A attributes */
    AttributePtr eql0;
    AttributePtr nic0;
    AttributePtr active0;
    AttributePtr rxpkts0;
    AttributePtr txpkts0;
    AttributePtr byte_count0;
    AttributePtr sync0;
    AttributePtr link0;
    AttributePtr auto0;
    AttributePtr rflt0;
    AttributePtr bad_symbol0;
    AttributePtr rx_bytes0;
    AttributePtr crc_fail0;
    AttributePtr rx_frames0;
    AttributePtr tx_frames0;
    AttributePtr tx_bytes0;
    AttributePtr internal_mac_error0;
    AttributePtr transmit_system_error0;
    AttributePtr drop_count0;
    AttributePtr dummy_linerate0;

    /* Port B attributes */
    AttributePtr eql1;
    AttributePtr nic1;
    AttributePtr active1;
    AttributePtr rxpkts1;
    AttributePtr txpkts1;
    AttributePtr byte_count1;
    AttributePtr sync1;
    AttributePtr link1;
    AttributePtr auto1;
    AttributePtr rflt1;
    AttributePtr bad_symbol1;
    AttributePtr rx_bytes1;
    AttributePtr crc_fail1;
    AttributePtr rx_frames1;
    AttributePtr tx_frames1;
    AttributePtr tx_bytes1;
    AttributePtr internal_mac_error1;
    AttributePtr transmit_system_error1;
    AttributePtr drop_count1;
    AttributePtr ethMac;
    AttributePtr dummy_linerate1;

	AttributePtr supp_error;
    GenericReadWritePtr grw;
    uint32_t bit_mask = 0xFFFFFFFF;

    assert(NULL != card);
    
    /* Set up virtual methods. */
    card_set_dispose_routine(card, dag43ge_dispose);
    card_set_post_initialize_routine(card, dag43ge_post_initialize);
    card_set_reset_routine(card, dag43ge_reset);
    card_set_default_routine(card, dag43ge_default);
    //card_set_load_firmware_routine(card, dag43ge_load_firmware);

    /* Allocate private state. */
    state = (dag43ge_state_t*) malloc(sizeof(dag43ge_state_t));
    memset(state, 0, sizeof(dag43ge_state_t));
    card_set_private_state(card, (void*) state);
    
    /* Create root component. */
    root_component = component_init(kComponentRoot, card);

    /* Add root component to card. */
    card_set_root_component(card, root_component);
    
    /* Add components as appropriate. */
    state->mGpp = gpp_get_new_gpp(card, 0);
    state->mPhy = dag43ge_get_new_phy(card);
    state->mPbm = pbm_get_new_pbm(card, 0);
    
	general = general_get_new_component(card, 0);

    state->mPort[0] = dag43ge_get_new_port(card);
    state->mPort[1] = dag43ge_get_new_port(card);
    

    /* Have to rename some components since there are more than 1 of them */
    component_set_name(state->mPort[0], "port0");
    component_set_name(state->mPort[1], "port1");
    
    /* Create new attributes. */
    eql0 = dag43ge_get_new_eql_attribute();
    nic0 = dag43ge_get_new_nic_attribute();
    active0 = dag43ge_get_new_active_attribute();
    rxpkts0 = dag43ge_get_new_rxpkts_attribute();
    txpkts0 = dag43ge_get_new_txpkts_attribute();
    byte_count0 = dag43ge_get_new_byte_count_attribute();
    sync0 = dag43ge_get_new_sync_attribute();
    link0 = dag43ge_get_new_link_attribute();
    auto0 = dag43ge_get_new_auto_attribute();
    rflt0 = dag43ge_get_new_rflt_attribute();
    bad_symbol0 = dag43ge_get_new_bad_symbol_attribute();
    rx_bytes0 = dag43ge_get_new_rx_bytes_attribute();
    crc_fail0 = dag43ge_get_new_crc_fail_attribute();
    rx_frames0 = dag43ge_get_new_rx_frames_attribute();
    tx_frames0 = dag43ge_get_new_tx_frames_attribute();
    tx_bytes0 = dag43ge_get_new_tx_bytes_attribute();
    internal_mac_error0 = dag43ge_get_new_internal_mac_error_attribute();
    transmit_system_error0 = dag43ge_get_new_transmit_system_error_attribute();
    drop_count0 = dag43ge_get_new_drop_count_attribute();
    dummy_linerate0 = dag43ge_get_new_linerate_attribute();

    eql1 = dag43ge_get_new_eql_attribute();
    nic1 = dag43ge_get_new_nic_attribute();
    active1 = dag43ge_get_new_active_attribute();
    rxpkts1 = dag43ge_get_new_rxpkts_attribute();
    txpkts1 = dag43ge_get_new_txpkts_attribute();
    byte_count1 = dag43ge_get_new_byte_count_attribute();
    sync1 = dag43ge_get_new_sync_attribute();
    link1 = dag43ge_get_new_link_attribute();
    auto1 = dag43ge_get_new_auto_attribute();
    rflt1 = dag43ge_get_new_rflt_attribute();
    bad_symbol1 = dag43ge_get_new_bad_symbol_attribute();
    rx_bytes1 = dag43ge_get_new_rx_bytes_attribute();
    crc_fail1 = dag43ge_get_new_crc_fail_attribute();
    rx_frames1 = dag43ge_get_new_rx_frames_attribute();
    tx_frames1 = dag43ge_get_new_tx_frames_attribute();
    tx_bytes1 = dag43ge_get_new_tx_bytes_attribute();
    internal_mac_error1 = dag43ge_get_new_internal_mac_error_attribute();
    transmit_system_error1 = dag43ge_get_new_transmit_system_error_attribute();
    drop_count1 = dag43ge_get_new_drop_count_attribute();
    dummy_linerate1 = dag43ge_get_new_linerate_attribute();

	supp_error = dag43ge_get_new_suppress_error_attribute();
    
    component_add_subcomponent(root_component, state->mGpp);
    /* Commenting out temporarily mPhy as its not used - To be verified */
    /*component_add_subcomponent(root_component, state->mPhy);*/
    component_add_subcomponent(root_component, state->mPbm);
    component_add_subcomponent(root_component, state->mPort[0]);
    component_add_subcomponent(root_component, state->mPort[1]);
    component_add_subcomponent(root_component, general);
    
    /*
    From the firmware perspective enabling or disabling a port is done by toggling a 
    bit in the gpp. However from the users perspective it makes more sense to make 
    the active attribute part of the port component and not the gpp component even though the
    address used is in the gpp module.
    */

    /* Add Ethernet MAC Address */
    address = 8; /* First port */
    
    grw = grw_init(card, address, grw_rom_read, NULL);
    ethMac = attribute_factory_make_attribute(kStringAttributeEthernetMACAddress, grw, &bit_mask, 1);
    component_add_attribute(state->mPort[0], ethMac);    

    address = 16; /* Second port */
    
    grw = grw_init(card, address, grw_rom_read, NULL);
    ethMac = attribute_factory_make_attribute(kStringAttributeEthernetMACAddress, grw, &bit_mask,1);
    component_add_attribute(state->mPort[1], ethMac);     



    component_add_attribute(state->mPort[0], eql0);
    component_add_attribute(state->mPort[0], nic0);
    component_add_attribute(state->mPort[0], active0);
    component_add_attribute(state->mPort[0], rxpkts0);
    component_add_attribute(state->mPort[0], txpkts0);
    component_add_attribute(state->mPort[0], byte_count0);
    component_add_attribute(state->mPort[0], sync0);
    component_add_attribute(state->mPort[0], link0);
    component_add_attribute(state->mPort[0], auto0);
    component_add_attribute(state->mPort[0], rflt0);
    component_add_attribute(state->mPort[0], bad_symbol0);
    component_add_attribute(state->mPort[0], rx_bytes0);
    component_add_attribute(state->mPort[0], crc_fail0);
    component_add_attribute(state->mPort[0], rx_frames0);
    component_add_attribute(state->mPort[0], tx_bytes0);
    component_add_attribute(state->mPort[0], tx_frames0);
    component_add_attribute(state->mPort[0], internal_mac_error0);
    component_add_attribute(state->mPort[0], transmit_system_error0);
    component_add_attribute(state->mPort[0], drop_count0);
    component_add_attribute(state->mPort[0], dummy_linerate0);

    component_add_attribute(state->mPort[1], eql1);
    component_add_attribute(state->mPort[1], nic1);
    component_add_attribute(state->mPort[1], active1);
    component_add_attribute(state->mPort[1], rxpkts1);
    component_add_attribute(state->mPort[1], txpkts1);
    component_add_attribute(state->mPort[1], byte_count1);
    component_add_attribute(state->mPort[1], sync1);
    component_add_attribute(state->mPort[1], link1);
    component_add_attribute(state->mPort[1], auto1);
    component_add_attribute(state->mPort[1], rflt1);
    component_add_attribute(state->mPort[1], bad_symbol1);
    component_add_attribute(state->mPort[1], rx_bytes1);
    component_add_attribute(state->mPort[1], crc_fail1);
    component_add_attribute(state->mPort[1], rx_frames1);
    component_add_attribute(state->mPort[1], tx_bytes1);
    component_add_attribute(state->mPort[1], tx_frames1);
    component_add_attribute(state->mPort[1], internal_mac_error1);
    component_add_attribute(state->mPort[1], transmit_system_error1);
    component_add_attribute(state->mPort[1], drop_count1);
    component_add_attribute(state->mPort[1], dummy_linerate1);
    
	component_add_attribute(general, supp_error);

    return kDagErrNone;
}

static AttributePtr
dag43ge_get_new_linerate_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineRate); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag43ge_linerate_dispose);
        attribute_set_post_initialize_routine(result, dag43ge_linerate_post_initialize);
        attribute_set_getvalue_routine(result, dag43ge_linerate_get_value);
        attribute_set_setvalue_routine(result, dag43ge_linerate_set_value);
        attribute_set_name(result, "line_rate");
        attribute_set_description(result, "Set the line rate to a specific speed");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, line_rate_to_string_routine);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
    }
    
    return result;
}


static void
dag43ge_linerate_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
dag43ge_linerate_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}


static void*
dag43ge_linerate_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* always its 1GigE */
        line_rate_t lr = kLineRateEthernet1000;
        attribute_set_value_array(attribute, &lr, sizeof(lr));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}


static void
dag43ge_linerate_set_value(AttributePtr attribute, void* value, int length)
{
    /* dont do anything - Its a dummy function */
    return;  
}

static void
line_rate_to_string_routine(AttributePtr attribute)
{
    void* temp = attribute_get_value(attribute);
    const char* string = NULL;
    line_rate_t lr;
    if (temp)
    {
        lr = *(line_rate_t*)temp;
        string = line_rate_to_string(lr);
        if (string)
            attribute_set_to_string(attribute, string);
    }
}
