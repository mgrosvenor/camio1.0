/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#if defined(_WIN32)
#define UNUSED
#endif /* _WIN32 */

/* Public API headers. */

/* Endace headers. */
#include "dagutil.h"
#include "dag_romutil.h"
#include "dagapi.h"
#include "dag37t_api.h"

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/attribute_factory.h"
#include "../include/card.h"
#include "../include/component.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/cards/dag3_constants.h"
#include "../include/util/utility.h"
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/components/led_controller_component.h"
#include "../include/components/duck_component.h"
#include "../include/components/dag37t_connection_component.h"
#include "../include/components/dag37t_mux_component.h"
#include "../include/util/enum_string_table.h"
#include "../include/channel_config/dag37t_chan_cfg.h"
/* Add card info component */
#include "../include/components/card_info_component.h"
/* POSIX headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif

/* C Standard Library headers. */
#include <assert.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>

/* Used to distingish dag3.7T cards and dag3.7T4 cards  */
typedef struct
{
    int maxPort;
    int *uExarPhysicalMapping;
    int *uStePhysicalMapping;

}dag37t4_card_state_t;



/* Used by attributes that exist in multiple components (e.g. 16 ports)
 * to record which channel they are associated with, base addresses etc.
 */
typedef struct
{
    int mLine;
    int mPhysical;
    int mChannel;
    uint32_t mBase;
    uint32_t mRegisterAddress;
    
} dag37t_channel_state_t;

typedef struct
{
    demapper_type_t mDemapperType;
} dag37t_demapper_state_t;


typedef struct 
{
    ComponentPtr mPort[16]; /* Direct reference to ports. */
    ComponentPtr mE1T1Framer;   /* Direct reference to framer. */
    ComponentPtr mPbm;
    ComponentPtr *mStream;
    ComponentPtr mDemapper;
    ComponentPtr mErfMux;
    ComponentPtr mConnectionSetup;
    ComponentPtr mConnection[512];
    
    /* Cached addresses. */
    uint32_t mPhyBase;
    uint32_t mPhyBase2;
    uint32_t mSonicE1T1Base;
    uint32_t mMuxBase;
    uint32_t mDemapperBase;
    uint32_t mSMBusBase;

} dag37t_state_t;

typedef struct
{
    uint32_t mWhich;
} led_state_t;


typedef struct
{
    int mLine;
    
} dag37t_component_state_t;

const char* uSteType[] =
{
    "OFF",
    "T1 ESF",
    "T1 D4 SF",
    "T1 D4",
    "E1",
    "E1 CRC",
    "E1 UNFRAMED",
    "OFF"
};

#define D37T_RTPP_BASE 0x1040

/* DAG3.7T Interface map - physical pod connections to logical channels */

/*
 * 11 10 14 15 8 9 13 12
 *  3  2  6  7 0 1  5  4
 */
/* indexed by logical line numbers (User Interface Line Id's) */
/* Mapping from logical interface number to physical exar interface number */
static int uExarPhysical[D37T_MAX_LINES] =
{
    3, /* 0 -> U5.3 */
    6, /* 1 -> U5.6 */
    4, /* 2 -> U5.4 */
    1, /* 3 -> U5.1 */
    12, /* 4 -> U18.4 */
    9, /* 5 -> U18.1 */
    15, /* 6 -> U18.7 */
    10, /* 7 -> U18.2 */
    2, /* 8 -> U5.2 */
    7, /* 9 -> U5.7 */
    5, /* 10 -> U5.5 */
    0, /* 11 -> U5.0 */
    13, /* 12 -> U18.5 */
    8, /* 13 -> U18.0 */
    14, /* 14 -> U18.6 */
    11 /* 15 -> U18.3 */
};

/* */

static int uExarPhysical_t4[D37T4_MAX_LINES] =
{
    4, /* 0 -> U5.0 */
    5, /* 10 -> U5.1 */
    1, /* 5 -> U5.5 */
    0, /* 4 -> U5.4 */
/* If endace labsa are enabled will be exposed all 8 interfaces of the 37t4 
Aux 4 ports on top of the card if the Board has been wired and connector soldered */

#ifdef ENDACE_LAB
    7,
    6,
    2,
    3
#endif 
};

static struct
{
    int phys;
    const char* name;
}
uExarPhysicalName[D37T_MAX_LINES] __attribute__((unused)) =
{
    { 3, "U5.3" },
    { 6, "U5.6" },
    { 4, "U5.4" },
    { 1, "U5.1" },
    { 12, "U18.4" },
    { 9, "U18.1" },
    { 15, "U18.7" },
    { 10, "U18.2" },
    { 2, "U5.2" },
    { 7, "U5.7" },
    { 5, "U5.5" },
    { 0, "U5.0" },
    { 13, "U18.5" },
    { 8, "U18.0" },
    { 14, "U18.6" },
    { 11, "U18.3" }
};
#if 0   
uExarPhysicalName_t4[D37T_MAX_LINES] __attribute__((unused)) =
{
    { 4, "U5c4" },
    { 5, "U5c5" },
    { 1, "U5c1" },
    { 0, "U5c0" },
#ifdef ENDACE_LAB
    { 7, "U5c7" },
    { 6, "U5c6" },
    { 2, "U5c2" },
    { 3, "U5c3" }
#endif 
};
#endif 

/* Mapping from logical interface number to physical framer interface number */
static int uStePhysical[D37T_MAX_LINES] =
{
    11, /* 0 */
    14, /* 1 */
    12, /* 2 */
    9, /* 3 */
    4, /* 4 */
    1, /* 5 */
    7, /* 6 */
    2, /* 7 */
    10, /* 8 */
    15, /* 9 */
    13, /* 10 */
    8, /* 11 */
    5, /* 12 */
    0, /* 13 */
    6, /* 14 */
    3 /* 15 */
};


typedef struct
{
    uint32_t mPCAAddress;
    uint32_t mRegister;
    uint32_t mBank;
} led_selection_t;

static led_selection_t uLEDTable[D37T_MAX_LINES * 2] =
{
    {0xce, 6, 0},
    {0xce, 6, 2},
    {0xce, 7, 0},
    {0xce, 7, 2},
    {0xce, 8, 0},
    {0xce, 8, 2},
    {0xce, 9, 0},
    {0xce, 9, 2},
    {0xc6, 6, 0},
    {0xc6, 6, 2},
    {0xc6, 7, 0},
    {0xc6, 7, 2},
    {0xc6, 9, 3},
    {0xc6, 9, 1},
    {0xc6, 8, 3},
    {0xc6, 8, 1},

    {0xce, 6, 1},
    {0xce, 6, 3},
    {0xce, 7, 1},
    {0xce, 7, 3},
    {0xce, 8, 1},
    {0xce, 8, 3},
    {0xce, 9, 1},
    {0xce, 9, 3},
    {0xc6, 6, 1},
    {0xc6, 6, 3},
    {0xc6, 7, 1},
    {0xc6, 7, 3},
    {0xc6, 9, 2},
    {0xc6, 9, 0},
    {0xc6, 8, 2},
    {0xc6, 8, 0}
};


//Because we can't read from this component we have to maintain an
//array of the connections configured
static uint32_t connection_num;
static dag37t4_card_state_t* card_state = NULL;

#define STE_SET_INTERFACE(interface_num) \
     ((card_state->uStePhysicalMapping[interface_num] << STE_CONFIG_INTERFACE_OFFSET) & STE_CONFIG_INTERFACE_MASK)


/* Internal routines. */
static int dag37t_read_single_channel(DagCardPtr card, int channel, int address) __attribute__((unused)) ;
static void dag37t_write_channels(DagCardPtr card, int channel, int address, int val);
static void dag37t_set_and_clear_channels(DagCardPtr card, int channel, int address, int set, int clear);
static void dag37t_wait_until_not_busy(DagCardPtr card);
static void dag37t_attribute_set_and_clear(AttributePtr attribute, uint32_t address, int set, int clear);
static uint32_t dag37t_attribute_read(AttributePtr attribute, uint32_t in_address);
static dag37t_channel_state_t* dag37t_create_attribute_channel_state(AttributePtr attribute, int line);

/* Virtual methods for the card. */
static void dag37t_dispose(DagCardPtr card);
static int dag37t_post_initialize(DagCardPtr card);
static void dag37t_reset(DagCardPtr card);
static void dag37t_default(DagCardPtr card);
static dag_err_t dag37t_update_register_base(DagCardPtr card);

/* DAG 3.7T components. */

/* Framer. */
static ComponentPtr dag37t_get_new_framer(DagCardPtr card);
static void dag37t_framer_dispose(ComponentPtr component);
static void dag37t_framer_reset(ComponentPtr component);
static void dag37t_framer_default(ComponentPtr component);

/* Port. */
static ComponentPtr dag37t_get_new_port(DagCardPtr card, int line);
static void dag37t_port_dispose(ComponentPtr component);
static void dag37t_port_reset(ComponentPtr component);
static void dag37t_port_default(ComponentPtr component);
static int dag37t_port_get_line(ComponentPtr component);

/* demapper */
static ComponentPtr dag37t_get_new_demapper(DagCardPtr card);
static void dag37t_demapper_dispose(ComponentPtr component);
static void dag37t_demapper_reset(ComponentPtr component);
static void dag37t_demapper_default(ComponentPtr component);
static demapper_type_t dag37t_demapper_get_type(ComponentPtr component);
static void dag37t_demapper_set_type(ComponentPtr component, demapper_type_t demapper_type);

/* Channel config. */
static ComponentPtr dag37t_get_new_connection_config(DagCardPtr card);
static void dag37t_connection_config_dispose(ComponentPtr component);
static void dag37t_connection_config_reset(ComponentPtr component);
static void dag37t_connection_config_default(ComponentPtr component);


/* DAG 3.7T attributes. */
/* Port attributes. */
static AttributePtr dag37t_get_new_mode_attribute(void);
static void dag37t_mode_dispose(AttributePtr attribute);
static void dag37t_mode_post_initialize(AttributePtr attribute);
static void* dag37t_mode_get_value(AttributePtr attribute);
static void dag37t_mode_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_linetype_attribute(void);
static void dag37t_linetype_dispose(AttributePtr attribute);
static void dag37t_linetype_post_initialize(AttributePtr attribute);
static void* dag37t_linetype_get_value(AttributePtr attribute);
static void dag37t_linetype_set_value(AttributePtr attribute, void* value, int length);
static void dag37t_linetype_to_string(AttributePtr attribute);
static void dag37t_linetype_from_string(AttributePtr attribute, const char* string);

static AttributePtr dag37t_get_new_termination_attribute(void);
static void dag37t_termination_dispose(AttributePtr attribute);
static void dag37t_termination_post_initialize(AttributePtr attribute);
static void* dag37t_termination_get_value(AttributePtr attribute);
static void dag37t_termination_set_value(AttributePtr attribute, void* value, int length);
static void dag37t_termination_to_string(AttributePtr attribute);
static void dag37t_termination_from_string(AttributePtr attribute, const char* string);

static AttributePtr dag37t_get_new_facility_loopback_attribute(void);
static void dag37t_fcloopback_dispose(AttributePtr attribute);
static void dag37t_fcloopback_post_initialize(AttributePtr attribute);
static void* dag37t_fcloopback_get_value(AttributePtr attribute);
static void dag37t_fcloopback_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_equipment_loopback_attribute(void);
static void dag37t_eqloopback_dispose(AttributePtr attribute);
static void dag37t_eqloopback_post_initialize(AttributePtr attribute);
static void* dag37t_eqloopback_get_value(AttributePtr attribute);
static void dag37t_eqloopback_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_zerocode_attribute(void);
static void dag37t_zerocode_dispose(AttributePtr attribute);
static void dag37t_zerocode_post_initialize(AttributePtr attribute);
static void* dag37t_zerocode_get_value(AttributePtr attribute);
static void dag37t_zerocode_set_value(AttributePtr attribute, void* value, int length);
static void dag37t_zerocode_to_string(AttributePtr attribute);
static void dag37t_zerocode_from_string(AttributePtr attribute, const char* string);

static AttributePtr dag37t_get_new_rxpkts_attribute(void);
static void dag37t_rxpkts_dispose(AttributePtr attribute);
static void dag37t_rxpkts_post_initialize(AttributePtr attribute);
static void* dag37t_rxpkts_get_value(AttributePtr attribute);
static void dag37t_rxpkts_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_txpkts_attribute(void);
static void dag37t_txpkts_dispose(AttributePtr attribute);
static void dag37t_txpkts_post_initialize(AttributePtr attribute);
static void* dag37t_txpkts_get_value(AttributePtr attribute);
static void dag37t_txpkts_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_scrambling_attribute(void);
static void dag37t_scrambling_dispose(AttributePtr attribute);
static void dag37t_scrambling_post_initialize(AttributePtr attribute);
static void dag37t_scrambling_set_value(AttributePtr attribute, void* value, int length);
static void* dag37t_scrambling_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_hec_correction_attribute(void);
static void dag37t_hec_correction_dispose(AttributePtr attribute);
static void dag37t_hec_correction_post_initialize(AttributePtr attribute);
static void dag37t_hec_correction_set_value(AttributePtr attribute, void* value, int length);
static void* dag37t_hec_correction_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_active_port_attribute(void);
static void dag37t_active_port_dispose(AttributePtr attribute);
static void dag37t_active_port_post_initialize(AttributePtr attribute);
static void dag37t_active_port_set_value(AttributePtr attribute, void* value, int length);
static void* dag37t_active_port_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_latch_and_clear_attribute(void);
static void dag37t_latch_and_clear_dispose(AttributePtr attribute);
static void dag37t_latch_and_clear_post_initialize(AttributePtr attribute);
static void dag37t_latch_and_clear_set_value(AttributePtr attribute, void* value, int length);
static void* dag37t_latch_and_clear_get_value(AttributePtr attribute);

/* Framer attributes. */
static AttributePtr dag37t_get_new_clear_attribute(void);
static void dag37t_clear_dispose(AttributePtr attribute);
static void* dag37t_clear_get_value(AttributePtr attribute);
static void dag37t_clear_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_reset_attribute(void);
static void dag37t_reset_dispose(AttributePtr attribute);
static void* dag37t_reset_get_value(AttributePtr attribute);
static void dag37t_reset_set_value(AttributePtr attribute, void* value, int length);



/* receive_loss_of_signal */
static AttributePtr dag37t_get_new_receive_loss_of_signal_attribute(void);
static void dag37t_receive_loss_of_signal_dispose(AttributePtr attribute);
static void* dag37t_receive_loss_of_signal_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_alarm_signal_attribute(void);
static void dag37t_alarm_signal_dispose(AttributePtr attribute);
static void* dag37t_alarm_signal_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_driver_monitor_output_attribute(void);
static void dag37t_driver_monitor_output_dispose(AttributePtr attribute);
static void* dag37t_driver_monitor_output_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_fifo_limit_status_attribute(void);
static void dag37t_fifo_limit_status_dispose(AttributePtr attribute);
static void* dag37t_fifo_limit_status_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_line_code_violation_attribute(void);
static void dag37t_line_code_violation_dispose(AttributePtr attribute);
static void* dag37t_line_code_violation_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_cable_loss_attribute(void);
static void dag37t_cable_loss_dispose(AttributePtr attribute);
static void* dag37t_cable_loss_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_rx0_attribute(void);
static void dag37t_e1t1_rx0_dispose(AttributePtr attribute);
static void* dag37t_e1t1_rx0_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_rx1_attribute(void);
static void dag37t_e1t1_rx1_dispose(AttributePtr attribute);
static void* dag37t_e1t1_rx1_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_tx0_attribute(void);
static void dag37t_e1t1_tx0_dispose(AttributePtr attribute);
static void* dag37t_e1t1_tx0_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_tx1_attribute(void);
static void dag37t_e1t1_tx1_dispose(AttributePtr attribute);
static void* dag37t_e1t1_tx1_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_framer_error_attribute(void);
static void dag37t_e1t1_framer_error_dispose(AttributePtr attribute);
static void* dag37t_e1t1_framer_error_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_crc_error_attribute(void);
static void dag37t_e1t1_crc_error_dispose(AttributePtr attribute);
static void* dag37t_e1t1_crc_error_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_ais_error_attribute(void);
static void dag37t_e1t1_ais_error_dispose(AttributePtr attribute);
static void* dag37t_e1t1_ais_error_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_framer_counter_attribute(void);
static void dag37t_e1t1_framer_counter_dispose(AttributePtr attribute);
static void* dag37t_e1t1_framer_counter_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_ais_counter_attribute(void);
static void dag37t_e1t1_ais_counter_dispose(AttributePtr attribute);
static void* dag37t_e1t1_ais_counter_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_crc_counter_attribute(void);
static void dag37t_e1t1_crc_counter_dispose(AttributePtr attribute);
static void* dag37t_e1t1_crc_counter_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_e1t1_link_attribute(void);
static void dag37t_e1t1_link_dispose(AttributePtr attribute);
static void* dag37t_e1t1_link_get_value(AttributePtr attribute);


/* Demapper attributes */

static AttributePtr dag37t_get_new_drop_count_attribute(void);
static void dag37t_drop_count_dispose(AttributePtr attribute);
static void* dag37t_drop_count_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_lcd_attribute(void);
static void dag37t_lcd_dispose(AttributePtr attribute);
static void* dag37t_lcd_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_timestamp_end_attribute(void);
static void dag37t_timestamp_end_dispose(AttributePtr attribute);
static void* dag37t_timestamp_end_get_value(AttributePtr attribute);
static void dag37t_timestamp_end_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_erf_mux_attribute(void);
static void dag37t_erf_mux_dispose(AttributePtr attribute);
static void dag37t_erf_mux_set_value(AttributePtr attribute, void* value, int length);

/* channel config attributes. */
static AttributePtr dag37t_get_new_add_connection_attribute(void);
static void dag37t_add_connection_dispose(AttributePtr attribute);
static void dag37t_add_connection_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_get_connection_number_attribute(void);
static void dag37t_get_connection_number_dispose(AttributePtr attribute);
static void* dag37t_get_connection_number_get_value(AttributePtr attribute);

static AttributePtr dag37t_get_new_delete_connection_attribute(void);
static void dag37t_delete_connection_dispose(AttributePtr attribute);
static void dag37t_delete_connection_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr dag37t_get_new_delete_all_connections_attribute(void);
static void dag37t_delete_all_connections_dispose(AttributePtr attribute);
static void dag37t_delete_all_connections_set_value(AttributePtr attribute, void* value, int length);

#define BUFFER_SIZE 1024

static dag37t_channel_state_t*
dag37t_create_attribute_channel_state(AttributePtr attribute, int line)
{
    dag37t_channel_state_t* channel_state = (dag37t_channel_state_t*)malloc(sizeof(dag37t_channel_state_t));
    DagCardPtr card = attribute_get_card(attribute);
    dag37t_state_t* state = card_get_private_state(card);
    
    channel_state->mLine = line;
    channel_state->mPhysical = card_state->uExarPhysicalMapping[line];
    if (channel_state->mPhysical < 8)
    {
        channel_state->mBase = state->mPhyBase;
    }
    else
    {
        channel_state->mBase = state->mPhyBase2;
    }
    channel_state->mChannel = channel_state->mPhysical % 8;

    return channel_state;
}

/* Implementation of internal routines. */
static int
dag37t_read_single_channel(DagCardPtr card, int channel, int address)
{
    dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
    int line;

    for (line = 0; line < card_state->maxPort; line++)
    {
        int physical = card_state->uExarPhysicalMapping[line];
        uint32_t base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        int c = physical % 8;
        
        if (channel & (1 << physical))
        {
            return card_read_iom(card, base + (c*16 + address)*4);
        }
    }

    return 0;
}


static void
dag37t_write_channels(DagCardPtr card, int channel, int address, int val)
{
    dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
    int line;
    
    for (line = 0; line < card_state->maxPort; line++)
    {
        int physical = card_state->uExarPhysicalMapping[line];
        uint32_t base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        int c = physical % 8;
        
        if (channel & (1 << physical))
        {
            card_write_iom(card, base + (c*16 + address)*4, val);
        }
    }
}


static void
dag37t_set_and_clear_channels(DagCardPtr card, int channel, int address, int set, int clear)
{
    dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
    int line;

    for (line = 0; line < card_state->maxPort; line++)
    {
        int physical = card_state->uExarPhysicalMapping[line];
        uint32_t base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        int c = physical % 8;

        if (channel & (1 << physical))
        {
            int val = card_read_iom(card, base + (c*16 + address)*4);
            
            val |= set;
            val &= ~clear;
            card_write_iom(card, base + (c*16 + address)*4, val);
        }
    }
}


static void
dag37t_attribute_set_and_clear(AttributePtr attribute, uint32_t in_address, int set, int clear)
{
    dag37t_channel_state_t* state = (dag37t_channel_state_t*) attribute_get_private_state(attribute);
    DagCardPtr card = attribute_get_card(attribute);
    uint32_t base = state->mBase;
    uint32_t address = base + (state->mChannel*16 + in_address)*4;
    int val = card_read_iom(card, address);
    
    val |= set;
    val &= ~clear;
    card_write_iom(card, address, val);
}


static uint32_t
dag37t_attribute_read(AttributePtr attribute, uint32_t in_address)
{
    dag37t_channel_state_t* state = (dag37t_channel_state_t*) attribute_get_private_state(attribute);
    DagCardPtr card = attribute_get_card(attribute);
    uint32_t base = state->mBase;
    uint32_t address = base + (state->mChannel*16 + in_address)*4;
    uint32_t value = card_read_iom(card, address);
    
    return value;
}


static void
dag37t_wait_until_not_busy(DagCardPtr card)
{
    dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
    uint32_t sonic_e1t1_base = state->mSonicE1T1Base;
    int safety = 1000000;

    assert(0 != sonic_e1t1_base);

    while (card_read_iom(card, sonic_e1t1_base + SONIC_E1T1_CONFIG) & STE_CONFIG_BUSY)
    {
        usleep(1000);
        safety--;
        if (0 == safety)
        {
            dagutil_panic("timed out waiting for DAG3.7T busy signal to clear.\n");
        }
    }
}


static void
dag37t_dispose(DagCardPtr card)
{
    ComponentPtr component;
    ComponentPtr root;
    AttributePtr attribute;
    int component_count = 0;
    int i = 0;
    
    /* Deallocate private state. */
    dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
   
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
    
    assert(NULL != state);
    
    assert(NULL != state->mStream);
    free (state->mStream);

    free(state);
    card_set_private_state(card, NULL);
}


static int
dag37t_post_initialize(DagCardPtr card)
{    
    int rx_streams = 0;
    int tx_streams = 0;
    int streammax = 0;
    int dagfd = 0;
    int x = 0;
    ComponentPtr root_component;
    demapper_type_t demapper_type;
    dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
 
    root_component = card_get_root_component(card);
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

    state->mPhyBase = card_get_register_address(card, DAG_REG_E1T1_CTRL, 0);
    state->mPhyBase2 = card_get_register_address(card, DAG_REG_E1T1_CTRL, 1);
    /* verify if only one EXAR exists that is true for the DAG 37t4 
     Will assigne the same address as the Fisrt EXAR 
     Note that is quick fix to make work the software with single EXAR chip 
     It is done based on the assumption that in the enum table if it finds a single entry
     the second one will return 0 
    */
    if( state->mPhyBase2 == 0 ) {
	state->mPhyBase2 = state->mPhyBase; 
    }

    state->mSonicE1T1Base = card_get_register_address(card, DAG_REG_SONIC_E1T1, 0);
    state->mMuxBase = MUX_BASE;
    state->mDemapperBase = card_get_register_address(card, DAG_REG_E1T1_ATM_DEMAP, 0);
    state->mSMBusBase = card_get_register_address(card, DAG_REG_SMB, 0);
    if (state->mDemapperBase == 0)
    {
        /* Try for HDLC */
        state->mDemapperBase = card_get_register_address(card, DAG_REG_E1T1_HDLC_DEMAP, 0);
        demapper_type = kDemapperTypeHDLC;
    }
    else
    {
        demapper_type = kDemapperTypeATM;
    }
    assert(state->mDemapperBase != 0);
    dag37t_demapper_set_type(state->mDemapper, demapper_type);

    if (card_get_register_address(card, DAG_REG_DUCK, 0))
    {
        component_add_subcomponent(root_component, duck_get_new_component(card, 0));
    }
    
    /* Add card info component */
    component_add_subcomponent(root_component, card_info_get_new_component(card, 0));

    /* Create channel state. */
    /*
    for (line = 0; line < 16; line++)
    {
        state->mChannelState[line].mLine = line;
        state->mChannelState[line].mPhysical = card_state->uExarPhysicalMapping[line];
        if (state->mChannelState[line].mPhysical < 8)
        {
            state->mChannelState[line].mBase = state->mPhyBase;
        }
        else
        {
            state->mChannelState[line].mBase = state->mPhyBase2;
        }
        state->mChannelState[line].mChannel = state->mChannelState[line].mPhysical % 8;
    }
    */
    /* Add card info component */
    component_add_subcomponent(root_component, card_info_get_new_component(card,0));
    /* Return 1 if standard post_initialize() should continue, 0 if not.
    * "Standard" post_initialize() calls post_initialize() on the card's root component.
    */
    return 1;
}


static void
dag37t_reset(DagCardPtr card)
{
   dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
   uint32_t sonic_e1t1_base = state->mSonicE1T1Base;
                  
   if (0 != sonic_e1t1_base)
   {
        dag37t_wait_until_not_busy(card);
        card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_CONFIG, STE_CONFIG_RESET);

        card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_COUNTER_CTRL, STE_CNTR_CLEAR);
        card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_CONFIG, STE_CONFIG_CLEAR_MOST);
   }

   card_reset_datapath(card);
}

static int
dag37t_clock_detect(DagCardPtr card)
{
/*
The E1T1 RTPP Command/Status Register (currently at 0x1040) contains the
detection bit and the result.
Bits 27:20 (0x0ff00000) = analysis result
Bit  11    (0x00000800) = Presence bit                                                                                                                          
                                                                                                                                                                
If bit 11 = '1', then the analysis code is available. If bit 11 = '0' then
bits 27:20 should be 0x00, meaning no analysis available.                                                                                                       
                                                                                                                                                                
The code continually samples the incoming (firmware) RX Clock 0 to
determine it's approximate frequency. After any change ("dagthree
default") the code may take several microseconds to notice the change:
(1544 kHz crystal)
Average case        = 4 * 648 ns -> 2.6 us
Likely worst case   = 8 * 648 ns -> 5.2 us
Absolute worst case = 8 * 859 ns -> 6.9 us                                                                                                                      
                                                                                                                                                                
The value returned is expected to vary by +-2 points.                                                                                                           
                                                                                                                                                                
The RX Clock into the xilinx is not actually the crystal frequency, but
rather the crystal frequency modulated by what mode the Exar chip is in.
So good practise would be to set the Exar (or at least Channel 0) to a
2048kHz crystal E1 mode and then sample the register.                                                                                                           
                                                                                                                                                                
Approximate Frequency (kHz) = 200000 / value                                                                                                                    

Typical Values:
0x46 =  72 = 2731 kHz (Misconfigured)
0x64 = 100 = 2048 kHz (E1)
0x82 = 130 = 1544 kHz (T1)
0xac = 172 = 1163 kHz (Misconfigured)                                                                                                                           

Note well that any frequency less than 790 kHz will produce garbage
results.                                                                                                                                                        
*/

    /* Autodetect the crystal speed and set appropriate variable. */
    dag37t_state_t* state = card_get_private_state(card);
    uint32_t val = card_read_iom(card, state->mPhyBase + D37T_RTPP_BASE);
    uint32_t freq;
    int guess;

    if (0 == (0x00000800 & val))
    {
        /* Autodetect not available. */
        dagutil_verbose("DAG 3.7T autodetect not supported.\n");
        return 0;
    }

    /* Autodetect is available - what is the frequency? */
    freq = (0x0ff00000 & val) >> 20;

    val = card_read_iom(card, state->mPhyBase + EXAR8338_CLK*4);
    if (val & BIT4)
    {
        /* Exar thinks the crystal is 1544kHz. */
        guess = 1544;
    }
    else
    {
        /* Exar thinks the crystal is 2048kHz. */
        guess = 2048;
    }

    val = card_read_iom(card, state->mPhyBase + EXAR8338_CLK*4);
    if (val & 0x8)
    {
        /* Exar is set to T1. */
        if ((125 <= freq) && (freq <= 135))
        {
            /* Frequency is 1544kHz, correct for T1. */
            dagutil_verbose("DAG 3.7T autodetect: Exar correctly set to %dkHz\n", guess);
        }
        else
        {
            /* Guess was wrong. */
            dagutil_verbose("DAG 3.7T autodetect: Exar incorrectly set to %dkHz\n", guess);
            guess = (guess == 1544) ? 2048 : 1544;
        }
    }
    else
    {
        /* Exar is set to E1. */
        if ((95 <= freq) && (freq <= 105))
        {
            /* Frequency is 2048kHz, correct for E1. */
            dagutil_verbose("DAG 3.7T autodetect: Exar correctly set to %dkHz\n", guess);
        }
        else
        {
            /* Guess was wrong. */
            dagutil_verbose("DAG 3.7T autodetect: Exar incorrectly set to %dkHz\n", guess);
            guess = (guess == 1544) ? 2048 : 1544;
        }
    }

    dagutil_verbose("DAG 3.7T crystal autodetect: %dkHz\n", guess);
    return guess;
}

static dag_err_t
dag37t_update_register_base(DagCardPtr card)
{
    if (1 == valid_card(card))
    {
        dag37t_state_t* state = NULL;
        state = card_get_private_state(card);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mPhyBase = card_get_register_address(card, DAG_REG_E1T1_CTRL, 0);
        state->mPhyBase2 = card_get_register_address(card, DAG_REG_E1T1_CTRL, 1);

	if( state->mPhyBase2 == 0 ) {
		state->mPhyBase2 = state->mPhyBase; 
	}

        state->mSonicE1T1Base = card_get_register_address(card, DAG_REG_SONIC_E1T1, 0);
        state->mMuxBase = MUX_BASE;
        state->mDemapperBase = card_get_register_address(card, DAG_REG_E1T1_ATM_DEMAP, 0);
        state->mSMBusBase = card_get_register_address(card, DAG_REG_SMB, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static void
dag37t_default(DagCardPtr card)
{
    dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
    uint32_t phy_base = state->mPhyBase;
    uint32_t phy_base2 = state->mPhyBase2;
    uint32_t sonic_e1t1_base = state->mSonicE1T1Base;
    uint32_t mux_base = state->mMuxBase;
    ComponentPtr pbm;
    ComponentPtr root;
    int line;
    
    card_reset_datapath(card);
    /*card_pbm_default(card);*/
    root = card_get_root_component(card);
    pbm = component_get_subcomponent(root, kComponentPbm, 0);
    component_default(pbm);

    dag37t_clock_detect(card);
    /* Software reset. */
    card_write_iom(card, phy_base + EXAR8338_CTRL*4, 0x01);
    card_write_iom(card, phy_base2 + EXAR8338_CTRL*4, 0x01);
    usleep(1000);

    /* Single-rail mode. */
    card_write_iom(card, phy_base + EXAR8338_CTRL*4, 0x80);
    card_write_iom(card, phy_base2 + EXAR8338_CTRL*4, 0x80);

    /* Clock select: 2048. */
    card_write_iom(card, phy_base + EXAR8338_CLK*4, 0x00);
    card_write_iom(card, phy_base2 + EXAR8338_CLK*4, 0x00);

    /* RX Enable */
    dag37t_write_channels(card, 0xffff, EXAR8338_CHAN_EQC, 0x20);

    /* TX Enable */
    dag37t_write_channels(card, 0xffff, EXAR8338_CHAN_LOOP, 0x08);

    /* enable B8ZS/HDB3 */
    dag37t_set_and_clear_channels(card, 0xffff, EXAR8338_CHAN_CODE, 0, EXAR8338_B8ZS);

    for (line = 0; line < card_state->maxPort; line++)
    {
        dag37t_wait_until_not_busy(card);
        card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_CONFIG,
            (STE_CONFIG_WRITE | STE_CONFIG_TYPE_E1 | STE_SET_INTERFACE(line))); /* STE_CONFIG_IS_T1 unset. good. */
    
    }
    dag37t_wait_until_not_busy(card);

    /* Reset all stats, TX DP reset is acceptable, all links are garbled anyway. */
    card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_CONFIG, STE_CONFIG_CLEAR_FATAL);

    /* Connect host Tx and Rx BM to FPGA Tx and Rx, loopback XScale Tx and Rx. */
    card_write_iom(card, mux_base + MUX_RX_HOST, MUX_CONNECT_TX_PHY);
    card_write_iom(card, mux_base + MUX_RX_PHY, MUX_CONNECT_TX_HOST);
    card_write_iom(card, mux_base + MUX_RX_IOP, MUX_CONNECT_TX_IOP);

    /* Set termination. */
    dag37t_set_and_clear_channels(card, 0xffff, EXAR8338_CHAN_TERM, 0x00, 0xf0);

    /* Set noeql and nofcl. */
    dag37t_set_and_clear_channels(card, 0xffff, EXAR8338_CHAN_LOOP, 0x00, 0x07);
}




static int
dag37t_port_get_line(ComponentPtr component)
{
    dag37t_component_state_t* state = (dag37t_component_state_t*) component_get_private_state(component);

    assert(kComponentPort == component_get_component_code(component));
    assert(NULL != state);
    
    return state->mLine;
}

static ComponentPtr
dag37t_get_new_demapper(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentDemapper, card);
    dag37t_demapper_state_t* state;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag37t_demapper_dispose);
        component_set_reset_routine(result, dag37t_demapper_reset);
        component_set_default_routine(result, dag37t_demapper_default);
		component_set_name(result, "demapper");
        state = (dag37t_demapper_state_t*)malloc(sizeof(dag37t_demapper_state_t));
        component_set_private_state(result, state);
    }
    return result;
}

static void
dag37t_demapper_dispose(ComponentPtr component)
{
}

static void
dag37t_demapper_reset(ComponentPtr component)
{

}

static void
dag37t_demapper_default(ComponentPtr component)
{

}

static ComponentPtr
dag37t_get_new_connection_config(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentConnectionSetup, card);
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag37t_connection_config_dispose);
        component_set_reset_routine(result, dag37t_connection_config_reset);
        component_set_default_routine(result, dag37t_connection_config_default);
		component_set_name(result, "Connection");
    }
    return result;
}

static void
dag37t_connection_config_dispose(ComponentPtr component)
{
}

static void
dag37t_connection_config_reset(ComponentPtr component)
{

}

static void
dag37t_connection_config_default(ComponentPtr component)
{

}

static ComponentPtr
dag37t_get_new_framer(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentFramer, card);
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag37t_framer_dispose);
        component_set_reset_routine(result, dag37t_framer_reset);
        component_set_default_routine(result, dag37t_framer_default);
		component_set_name(result, "framer");
    }
    
    return result;
}


static void
dag37t_framer_dispose(ComponentPtr component)
{
}


static void
dag37t_framer_reset(ComponentPtr component)
{
}


static void
dag37t_framer_default(ComponentPtr component)
{
}


static ComponentPtr
dag37t_get_new_port(DagCardPtr card, int line)
{
    ComponentPtr result = component_init(kComponentPort, card);
    
    if (NULL != result)
    {    
        dag37t_component_state_t* state = (dag37t_component_state_t*) malloc(sizeof(dag37t_component_state_t));

        state->mLine = line;
        component_set_private_state(result, (void*) state);

        component_set_dispose_routine(result, dag37t_port_dispose);
        component_set_reset_routine(result, dag37t_port_reset);
        component_set_default_routine(result, dag37t_port_default);
    }
    
    return result;
}


static void
dag37t_port_dispose(ComponentPtr component)
{
}


static void
dag37t_port_reset(ComponentPtr component)
{
}


static void
dag37t_port_default(ComponentPtr component)
{
}

static AttributePtr
dag37t_get_new_rxpkts_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRxPkts);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_rxpkts_dispose);
        attribute_set_post_initialize_routine(result, dag37t_rxpkts_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_rxpkts_get_value);
        attribute_set_setvalue_routine(result, dag37t_rxpkts_set_value);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "rxpkts");
	attribute_set_description(result, "Enable or disable receive packets");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}


static void
dag37t_rxpkts_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_rxpkts_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_rxpkts_get_value(AttributePtr attribute)
{
    static uint8_t value = 0;
    uint32_t reg_val = 0;
    if (1 == valid_attribute(attribute))
    {
        reg_val = dag37t_attribute_read(attribute, EXAR8338_CHAN_EQC);        
        value = (reg_val & BIT5) ? 1:0;
        return (void*)&value;
    }
    
    return NULL;
}


static void
dag37t_rxpkts_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        uint8_t rxpkts = *(uint8_t*)value;
        if (rxpkts == 0)
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_EQC, 0, 0x20);
        else
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_EQC, 0x20, 0);
    }
}


static AttributePtr
dag37t_get_new_txpkts_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeTxPkts);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_txpkts_dispose);
        attribute_set_post_initialize_routine(result, dag37t_txpkts_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_txpkts_get_value);
        attribute_set_setvalue_routine(result, dag37t_txpkts_set_value);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_config_status(result, kDagAttrConfig);
	attribute_set_name(result, "txpkts");
	attribute_set_description(result, "Enable or disable transmit packets");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}


static void
dag37t_txpkts_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_txpkts_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_txpkts_get_value(AttributePtr attribute)
{
    static uint32_t value = 0;
    if (1 == valid_attribute(attribute))
    {
        value = dag37t_attribute_read(attribute, EXAR8338_CHAN_LOOP);
        value = (value & BIT3) ? 1:0;
        return (void*)&value;
    }
    
    return NULL;
}


static void
dag37t_txpkts_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        uint8_t txpkts = *(uint8_t*)value;
        if (txpkts == 0)
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0, 0x8);
        else
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0x8, 0);
    }
}

static AttributePtr 
dag37t_get_new_scrambling_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeATMScramble);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_scrambling_dispose);
        attribute_set_post_initialize_routine(result, dag37t_scrambling_post_initialize);
        attribute_set_setvalue_routine(result, dag37t_scrambling_set_value);
        attribute_set_getvalue_routine(result, dag37t_scrambling_get_value);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_config_status(result, kDagAttrConfig);
	    attribute_set_name(result, "pscramble");
 	    attribute_set_description(result, "Enable or disable payload scrambling");	
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void 
dag37t_scrambling_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}
static void 
dag37t_scrambling_post_initialize(AttributePtr attribute)
{
	if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}

static void 
dag37t_scrambling_set_value(AttributePtr attribute, void* value, int length)
{
    dag37t_channel_state_t* state = (dag37t_channel_state_t*) attribute_get_private_state(attribute);

    if (1 == valid_attribute(attribute))
    {
	
        *(int*)value = dag_ifc_cell_scrambling(card_get_fd( attribute_get_card(attribute)), 
			state->mChannel,*(uint8_t*)value);
    }
    
}

static void*
dag37t_scrambling_get_value(AttributePtr attribute)
{
    dag37t_channel_state_t* state = (dag37t_channel_state_t*) attribute_get_private_state(attribute);
    static uint32_t value = 0;

    if (1 == valid_attribute(attribute))
    {
	
        value = get_dag_ifc_cell_scrambling(card_get_fd( attribute_get_card(attribute)), state->mChannel);
        return (void*)&value;
    }

    
    return NULL;
}


static AttributePtr 
dag37t_get_new_hec_correction_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeHECCorrection);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_hec_correction_dispose);
        attribute_set_post_initialize_routine(result, dag37t_hec_correction_post_initialize);
        attribute_set_setvalue_routine(result, dag37t_hec_correction_set_value);
        attribute_set_getvalue_routine(result, dag37t_hec_correction_get_value);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_config_status(result, kDagAttrConfig);
	attribute_set_name(result, "hec_correction");
	attribute_set_description(result, "Enable or disable HEC correction on this connection");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void 
dag37t_hec_correction_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}
static void 
dag37t_hec_correction_post_initialize(AttributePtr attribute)
{
	if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}

static void 
dag37t_hec_correction_set_value(AttributePtr attribute, void* value, int length)
{
    dag37t_channel_state_t* state = (dag37t_channel_state_t*) attribute_get_private_state(attribute);

    if (1 == valid_attribute(attribute))
    {
	
        *(int*)value = dag_ifc_hec(card_get_fd( attribute_get_card(attribute)), 
			state->mChannel,*(uint8_t*)value);
    }
    
}

static void*
dag37t_hec_correction_get_value(AttributePtr attribute)
{
    dag37t_channel_state_t* state = (dag37t_channel_state_t*) attribute_get_private_state(attribute);
    static uint32_t value = 0;

    if (1 == valid_attribute(attribute))
    {
	
        value = get_dag_ifc_hec(card_get_fd( attribute_get_card(attribute)), state->mChannel);
        return (void*)&value;
    }

    
    return NULL;
}

static AttributePtr 
dag37t_get_new_active_port_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive);
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_active_port_dispose);
        attribute_set_post_initialize_routine(result, dag37t_active_port_post_initialize);
        attribute_set_setvalue_routine(result, dag37t_active_port_set_value);
        attribute_set_getvalue_routine(result, dag37t_active_port_get_value);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_name(result, "active_port");
        attribute_set_description(result, "Enable or disable on this port");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
     }
                    
     return result;
}

static void 
dag37t_active_port_dispose(AttributePtr attribute)
{
}

static void 
dag37t_active_port_post_initialize(AttributePtr attribute)
{
}

static void 
dag37t_active_port_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void *
dag37t_active_port_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
            uint8_t value = 1;
            attribute_set_value_array(attribute, &value, sizeof(value));
            return ( attribute_get_value_array(attribute) );
    }
      
    return NULL;
}

static AttributePtr 
dag37t_get_new_latch_and_clear_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeCounterLatch);
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_latch_and_clear_dispose);
        attribute_set_post_initialize_routine(result, dag37t_latch_and_clear_post_initialize);
        attribute_set_setvalue_routine(result, dag37t_latch_and_clear_set_value);
        attribute_set_getvalue_routine(result, dag37t_latch_and_clear_get_value);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_name(result, "latch_and_clear");
        attribute_set_description(result, "Latch the port counters and clear them");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
     }
     return result;
}

static void 
dag37t_latch_and_clear_dispose(AttributePtr attribute)
{
}

static void 
dag37t_latch_and_clear_post_initialize(AttributePtr attribute)
{
}

static void 
dag37t_latch_and_clear_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    dag37t_state_t* state = NULL;
    uint32_t val;
    uint32_t bool_val = *(uint32_t*)value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);

	// Set the clear bit
        val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL);
        card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL, val | (bool_val<<31));

	// Wait for the clear busy bit to reset
	do {	
        	val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL);
	} while (((val >> 28) & 0x1) == 1);

	// Set the flip bit
        val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL);
        card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL, val | (bool_val<<19));

	// Wait for the flip busy bit to reset
	do {	
        	val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL);
	} while (((val >> 16) & 0x1) == 1);
    }
}

static void *
dag37t_latch_and_clear_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
            uint8_t value = 0;
            attribute_set_value_array(attribute, &value, sizeof(value));
            return ( attribute_get_value_array(attribute) );
    }
      
    return NULL;
}

static AttributePtr
dag37t_get_new_mode_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMode);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_mode_dispose);
        attribute_set_post_initialize_routine(result, dag37t_mode_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_mode_get_value);
        attribute_set_setvalue_routine(result, dag37t_mode_set_value);
	    attribute_set_name(result, "mode");
	    attribute_set_description(result, "Line characteristics");
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
    }
    
    return result;
}


static void
dag37t_mode_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_mode_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_mode_get_value(AttributePtr attribute)
{
    static uint32_t value = 0;
    if (1 == valid_attribute(attribute))
    {
        value = dag37t_attribute_read(attribute, EXAR8338_CHAN_EQC);
        value = value & 0x1f;
        return (void*)&value;
    }
    
    return NULL;
}


static void
dag37t_mode_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t mode = *(uint32_t*)value;
        
        dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_EQC, mode & 0x1f, (~mode) & 0x1f);
    }
}


static AttributePtr
dag37t_get_new_linetype_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineType);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_linetype_dispose);
        attribute_set_post_initialize_routine(result, dag37t_linetype_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_linetype_get_value);
        attribute_set_setvalue_routine(result, dag37t_linetype_set_value);
        attribute_set_valuetype(result, kAttributeUint32);
	attribute_set_name(result, "linetype");
	attribute_set_description(result, "Line Characteristics");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_to_string_routine(result, dag37t_linetype_to_string);
        attribute_set_from_string_routine(result, dag37t_linetype_from_string);
    }
    
    return result;
}


static void
dag37t_linetype_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_linetype_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_linetype_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    dag37t_state_t* state;
    int line;
    ComponentPtr port;
    static line_type_t line_type;
    uint32_t register_val = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        if (state->mSonicE1T1Base)
        {
            register_val = card_read_iom(card, state->mSonicE1T1Base + card_state->uStePhysicalMapping[line]*4);
            register_val = (register_val >> STE_STAT_TYPE_OFFSET) & STE_STAT_TYPE_MASK;
        }
        if (register_val == 1)
        {
            line_type = kLineTypeT1esf;
        }
        else if (register_val == 2)
        {
            line_type = kLineTypeT1sf;
        }
        else if (register_val == 3)
        {
            line_type = kLineTypeT1;
        }
        else if (register_val == 4)
        {
            line_type = kLineTypeE1;
        }
        else if (register_val == 5)
        {
            line_type = kLineTypeE1crc;
        }
        else if (register_val == 6)
        {
            line_type = kLineTypeE1unframed;
        }
        else
        {
            line_type = kLineTypeNoPayload;
        }
        return (void*)&line_type;
    }
    
    return NULL;
}


static void
dag37t_linetype_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        line_type_t desired_value = *(line_type_t*) value;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* state = (dag37t_state_t*)card_get_private_state(card);
        uint32_t val;
        uint32_t mode=0;
        int line;

        line = dag37t_port_get_line(component);
        if ((kLineTypeT1 == desired_value) || (kLineTypeT1sf == desired_value) || (kLineTypeT1esf == desired_value) || (kLineTypeT1unframed == desired_value))
        {
            val = card_read_iom(card, state->mPhyBase + EXAR8338_CLK*4);
            val |= 0x8;
            card_write_iom(card, state->mPhyBase + EXAR8338_CLK*4, val);

            val = card_read_iom(card, state->mPhyBase2 + EXAR8338_CLK*4);
            val |= 0x8;
            card_write_iom(card, state->mPhyBase2 + EXAR8338_CLK*4, val);

            if (state->mSonicE1T1Base)
            {
                /* Determine line mode */
                switch (desired_value)
                {
                    case kLineTypeT1esf:
                        mode = STE_CONFIG_TYPE_T1_ESF;
                        break;
                    case kLineTypeT1sf:
                        mode = STE_CONFIG_TYPE_T1_D4_SF;
                        break;
                    case kLineTypeT1:
                    case kLineTypeT1unframed:
                        mode = STE_CONFIG_TYPE_T1_D4;
                        break;
                    default:
                        break;
                }
    
                dag37t_wait_until_not_busy(card);
                card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_CONFIG,
                    (STE_CONFIG_WRITE | STE_CONFIG_IS_T1 | mode | STE_SET_INTERFACE(line)));
                dag37t_wait_until_not_busy(card);
                /* Reset all stats, TX DP reset is acceptable, all links are garbled anyway*/
                card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_CONFIG, STE_CONFIG_CLEAR_FATAL);
                card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL, STE_CNTR_CLEAR);
            }
        }
        else if ((kLineTypeE1 == desired_value) || (kLineTypeE1crc == desired_value) || (kLineTypeE1unframed == desired_value))
        {
            val = card_read_iom(card, state->mPhyBase + EXAR8338_CLK*4);
            val &= ~0x8;
            card_write_iom(card, state->mPhyBase + EXAR8338_CLK*4, val);

            val = card_read_iom(card, state->mPhyBase2 + EXAR8338_CLK*4);
            val &= ~0x8;
            card_write_iom(card, state->mPhyBase2 + EXAR8338_CLK*4, val);

            /* Configure a default E1 setting for the framer */
            if (state->mSonicE1T1Base)
            {
                switch (desired_value)
                {
                    case kLineTypeE1:
                        mode = STE_CONFIG_TYPE_E1;
                        break;
                    case kLineTypeE1crc:
                        mode = STE_CONFIG_TYPE_E1_CRC;
                        break;
                    case kLineTypeE1unframed:
                        mode = STE_CONFIG_TYPE_UNFRAMED;
                        break;
                    default:
                        assert(0);
                        break;
                }

                dag37t_wait_until_not_busy(card);
                card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_CONFIG,
                    (STE_CONFIG_WRITE | mode | STE_SET_INTERFACE(line)));
                dag37t_wait_until_not_busy(card);

                /* Reset all stats, TX DP reset is acceptable, all links are garbled anyway*/
                card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_CONFIG, STE_CONFIG_CLEAR_FATAL);
                card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL, STE_CNTR_CLEAR);
            }
        }
    }
}

static void
dag37t_linetype_to_string(AttributePtr attribute)
{
    line_type_t value;
    char buffer[BUFFER_SIZE];
    if (1 == valid_attribute(attribute))
    {
        value = *(line_type_t*)dag37t_linetype_get_value(attribute);
        if (kLineTypeE1unframed == value)
        {
            strcpy(buffer, "e1_unframed");
        }
        else if (kLineTypeE1crc == value)
        {
            strcpy(buffer, "e1_crc");
        }
        else if (kLineTypeE1 == value)
        {
            strcpy(buffer, "e1");
        }
        else if (kLineTypeT1 == value)
        {
            strcpy(buffer, "t1");
        }
        else if (kLineTypeT1unframed == value)
        {
            strcpy(buffer, "t1_unframed");
        }
        else if (kLineTypeT1sf == value)
        {
            strcpy(buffer, "t1sf");
        }
        else if (kLineTypeT1esf == value)
        {
            strcpy(buffer, "t1esf");
        }
        else
        {
            strcpy(buffer, "no_payload");
        }
        attribute_set_to_string(attribute, buffer);
    }
}

static void
dag37t_linetype_from_string(AttributePtr attribute, const char* string)
{
    line_type_t value;
    if (1 == valid_attribute(attribute))
    {
        if (strcmp(string, "e1_unframed") == 0)
        {
            value = kLineTypeE1unframed;
        }
        else if (strcmp(string, "e1_crc") == 0)
        {
            value = kLineTypeE1crc;
        }
        else if (strcmp(string, "e1") == 0)
        {
            value = kLineTypeE1;
        }
        else if (strcmp(string, "t1_unframed") == 0)
        {
            value = kLineTypeT1unframed;
        }
        else if (strcmp(string, "t1") == 0)
        {
            value = kLineTypeT1;
        }
        else if (strcmp(string, "t1sf") == 0)
        {
            value = kLineTypeT1sf;
        }
        else if (strcmp(string, "t1esf") == 0)
        {
            value = kLineTypeT1esf;
        }
        else
        {
            value = kLineTypeNoPayload;
        }
        dag37t_linetype_set_value(attribute, (void*)&value, sizeof(line_type_t));
    }
}

static AttributePtr
dag37t_get_new_termination_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTermination);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_termination_dispose);
        attribute_set_post_initialize_routine(result, dag37t_termination_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_termination_get_value);
        attribute_set_setvalue_routine(result, dag37t_termination_set_value);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
	attribute_set_name(result, "term");
	attribute_set_description(result, "Line Characteristics");
        attribute_set_to_string_routine(result, dag37t_termination_to_string);
        attribute_set_from_string_routine(result, dag37t_termination_from_string);
    }
    
    return result;
}


static void
dag37t_termination_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_termination_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_termination_get_value(AttributePtr attribute)
{
    static termination_t termination = kTerminationExternal;
    if (1 == valid_attribute(attribute))
    {
        uint32_t value = dag37t_attribute_read(attribute, EXAR8338_CHAN_TERM);

        if ((0 == (value & 0x80)) && (0 == (value & 0x40)))
        {
            /* Both rx and tx are external. */
            termination = kTerminationExternal;
        }
        else if ((0 != (value & 0x80)) && (0 != (value & 0x40)))
        {
            /* Both rx and tx are internal. */
            switch (value & 0x30)
            {
                case 0x00:
                    termination = kTermination100ohm;
                    break;
                    
                case 0x20:
                    termination = kTermination75ohm;
                    break;
                    
                case 0x30:
                    termination = kTermination120ohm;
                    break;
                    
                default:
                    assert(0);
                    break;
            }
        }
        else if (0 == (value & 0x80))
        {
            /* RX termination is external, TX termination is internal. */
            switch (value & 0x30)
            {
                case 0x00:
                    termination = kTerminationRxExternalTx100ohm;
                    break;
                    
                case 0x20:
                    termination = kTerminationRxExternalTx75ohm;
                    break;
                    
                case 0x30:
                    termination = kTerminationRxExternalTx120ohm;
                    break;
                    
                default:
                    assert(0);
                    break;
            }
        }
        else if (0 == (value & 0x40))
        {
            /* TX termination is external, RX termination is internal. */
            switch (value & 0x30)
            {
                case 0x00:
                    termination = kTerminationRx100ohmTxExternal;
                    break;
                    
                case 0x20:
                    termination = kTerminationRx75ohmTxExternal;
                    break;
                    
                case 0x30:
                    termination = kTerminationRx120ohmTxExternal;
                    break;
                    
                default:
                    assert(0);
                    break;
            }
        }
        
        return (void*)&termination;
    }
    
    return NULL;
}


static void
dag37t_termination_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        int int_value;
        
        assert(4 == length);
        
        int_value = *((int*) value);

        switch (int_value)
        {
        case kTerminationExternal:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0, 0xf0);
            break;

        case kTermination75ohm:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0xe0, 0x10);
            break;

        case kTermination100ohm:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0xc0, 0x30);
            break;

        case kTermination120ohm:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0xf0, 0x00);
            break;
    
        case kTerminationRxExternalTx75ohm:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0x60, 0x90);
            break;

        case kTerminationRxExternalTx100ohm:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0x50, 0xb0);
            break;

        case kTerminationRxExternalTx120ohm:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0x70, 0x80);
            break;

        case kTerminationRx75ohmTxExternal:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0xa0, 0x50);
            break;

        case kTerminationRx100ohmTxExternal:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0x80, 0x70);
            break;

        case kTerminationRx120ohmTxExternal:
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_TERM, 0xb0, 0x40);
            break;

        default:
            assert(0);
            break;
        }
    }
}

static void
dag37t_termination_to_string(AttributePtr attribute)
{
    termination_t value;
    const char* buffer;
    if (1 == valid_attribute(attribute))
    {
        value = *(termination_t*)dag37t_termination_get_value(attribute);
        buffer = termination_to_string(value);
        if (buffer)
            attribute_set_to_string(attribute, buffer);
    }
}

static void
dag37t_termination_from_string(AttributePtr attribute, const char* string)
{
    termination_t value;
    if (1 == valid_attribute(attribute))
    {
        value = string_to_termination(string);
        if (value != kTerminationInvalid)
            dag37t_termination_set_value(attribute, (void*)&value, sizeof(termination_t));
    }
}

static AttributePtr
dag37t_get_new_facility_loopback_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFacilityLoopback);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_fcloopback_dispose);
        attribute_set_post_initialize_routine(result, dag37t_fcloopback_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_fcloopback_get_value);
        attribute_set_setvalue_routine(result, dag37t_fcloopback_set_value);
	attribute_set_name(result, "fcl");
	attribute_set_description(result, "Facility Loopback");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}


static void
dag37t_fcloopback_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_fcloopback_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_fcloopback_get_value(AttributePtr attribute)
{
    static uint8_t val;
    int physical;
    int line;
    ComponentPtr port;
    int base;
    int c;
    dag37t_state_t* state;
    DagCardPtr card;
    uint32_t loopback;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = (dag37t_state_t*)card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;

        register_val = card_read_iom(card, base + (EXAR8338_CHAN_LOOP + c*16)*4);

        /* Display loopback. */
        loopback = (register_val & 0x07);
        val = (loopback == 7 || loopback == 6);
        return (void*)&val;
    }
    
    return NULL;
}


static void
dag37t_fcloopback_set_value(AttributePtr attribute, void* value, int length)
{
    int physical;
    int line;
    ComponentPtr port;
    int base;
    int c;
    dag37t_state_t* state;
    DagCardPtr card;
    int val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = (dag37t_state_t*)card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;

        if (*(uint8_t*)value == 0)
        {
            val = card_read_iom(card, base + (EXAR8338_CHAN_LOOP + c*16)*4);
            if (7 == (val & 0x07))
            {
                /* Both fcl and eql are set, so just clear bit 1. */
                dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0x0, 0x02);
            }
            else
            {
                /* Only fcl is set, so clear bit 2. */
                dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0x0, 0x04);
            }
        }
        else
        {
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0x06, 0x00);
        }
    }
}


static AttributePtr
dag37t_get_new_equipment_loopback_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeEquipmentLoopback);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_eqloopback_dispose);
        attribute_set_post_initialize_routine(result, dag37t_eqloopback_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_eqloopback_get_value);
        attribute_set_setvalue_routine(result, dag37t_eqloopback_set_value);
        attribute_set_config_status(result, kDagAttrConfig);
	attribute_set_name(result, "eql");
	attribute_set_description(result, "Equipment Loopback");
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}


static void
dag37t_eqloopback_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_eqloopback_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_eqloopback_get_value(AttributePtr attribute)
{
    static uint8_t val;
    int physical;
    int line;
    ComponentPtr port;
    int base;
    int c;
    dag37t_state_t* state;
    DagCardPtr card;
    uint32_t loopback;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = (dag37t_state_t*)card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;

        register_val = card_read_iom(card, base + (EXAR8338_CHAN_LOOP + c*16)*4);

        /* Display loopback. */
        loopback = (register_val & 0x07);
        val = (loopback == 7 || loopback == 5);
        return (void*)&val;
    }
    
    return NULL;
}


static void
dag37t_eqloopback_set_value(AttributePtr attribute, void* value, int length)
{
    int physical;
    int line;
    ComponentPtr port;
    int base;
    int c;
    dag37t_state_t* state;
    DagCardPtr card;
    int val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = (dag37t_state_t*)card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;

        if (*(uint8_t*)value == 0)
        {
            val = card_read_iom(card, base + (EXAR8338_CHAN_LOOP + c*16)*4);
            if (7 == (val & 0x07))
            {
                /* Both fcl and eql are set, so just clear bit 0. */
                dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0x0, 0x01);
            }
            else
            {
                /* Only eql is set, so clear bit 2. */
                dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0x0, 0x04);
            }
        }
        else
        {
            dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_LOOP, 0x05, 0x00);
        }
    }
}


static AttributePtr
dag37t_get_new_zerocode_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeZeroCodeSuppress);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_zerocode_dispose);
        attribute_set_post_initialize_routine(result, dag37t_zerocode_post_initialize);
        attribute_set_getvalue_routine(result, dag37t_zerocode_get_value);
        attribute_set_setvalue_routine(result, dag37t_zerocode_set_value);
        attribute_set_to_string_routine(result, dag37t_zerocode_to_string);
        attribute_set_from_string_routine(result, dag37t_zerocode_from_string);
        attribute_set_config_status(result, kDagAttrConfig);
	attribute_set_name(result, "zerocode");
	attribute_set_description(result, "Line coding");
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
dag37t_zerocode_to_string(AttributePtr attribute)
{
	if (1 == valid_attribute(attribute))
	{
		zero_code_suppress_t code;
		code = *(zero_code_suppress_t*)attribute_get_value(attribute);
		attribute_set_to_string(attribute, zero_code_to_string(code));
	}
}

static void
dag37t_zerocode_from_string(AttributePtr attribute, const char* string)
{
	if (1 == valid_attribute(attribute) && string != NULL)
	{
		zero_code_suppress_t code;
		code = string_to_zero_code(string);
		attribute_set_value(attribute, &code, 1);
	}
}

static void
dag37t_zerocode_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_zerocode_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = attribute_get_component(attribute);
        dag37t_state_t* card_state;
        int line;
        
        assert(NULL != card);
        assert(NULL != component);
        
        card_state = (dag37t_state_t*) card_get_private_state(card);
        assert(NULL != card_state);

        line = dag37t_port_get_line(component);
        attribute_set_private_state(attribute, (void*)dag37t_create_attribute_channel_state(attribute, line));
    }
}


static void*
dag37t_zerocode_get_value(AttributePtr attribute)
{
    static zero_code_suppress_t zerocode;
    if (1 == valid_attribute(attribute))
    {
        uint32_t value = dag37t_attribute_read(attribute, EXAR8338_CHAN_CODE);
        
        if (EXAR8338_B8ZS & value)
        {
            zerocode = kZeroCodeSuppressAMI;
        }
        else
        {
            zerocode = kZeroCodeSuppressB8ZS;
        }
        
        return (void*)&zerocode;
    }
    
    return NULL;
}


static void
dag37t_zerocode_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        zero_code_suppress_t which = *((zero_code_suppress_t*) value);
    
        switch (which)
        {
            case kZeroCodeSuppressB8ZS:
                dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_CODE, 0, EXAR8338_B8ZS);
                break;

            case kZeroCodeSuppressAMI:
                dag37t_attribute_set_and_clear(attribute, EXAR8338_CHAN_CODE, EXAR8338_B8ZS, 0);
                break;
            
            default:
                assert(0);
                break;
        }
    }
}


static AttributePtr
dag37t_get_new_clear_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeClear);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_clear_dispose);
        attribute_set_getvalue_routine(result, dag37t_clear_get_value);
        attribute_set_setvalue_routine(result, dag37t_clear_set_value);
	    attribute_set_name(result, "clear");
	    attribute_set_description(result, "Clear the statistics counters on the framer");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}


static void
dag37t_clear_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void*
dag37t_clear_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        /*read register */
        register_val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_CONFIG);
        val = (register_val & BIT31)?1:0;
        
        return (void*)&val;
    }
    
    return NULL;

}


static void
dag37t_clear_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
        uint32_t sonic_e1t1_base = state->mSonicE1T1Base;
        
        /* Only reset the framer. */
        if (0 != sonic_e1t1_base)
        {
            card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_COUNTER_CTRL, STE_CNTR_CLEAR);
            card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_CONFIG, STE_CONFIG_CLEAR_MOST);
            
            usleep(1000);
            while (0 != (card_read_iom(card, sonic_e1t1_base + SONIC_E1T1_COUNTER_CTRL) & STE_CNTR_CBSY))
            {
                /* Do nothing. */
            }
        }
    }
}


static AttributePtr
dag37t_get_new_reset_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReset);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_reset_dispose);
        attribute_set_getvalue_routine(result, dag37t_reset_get_value);
        attribute_set_setvalue_routine(result, dag37t_reset_set_value);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
	    attribute_set_name(result, "reset");
	    attribute_set_description(result, "Use to rest the framer");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}


static void
dag37t_reset_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void*
dag37t_reset_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        /*read register */
        register_val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_CONFIG);
        val = (register_val & BIT13)?1:0;

        return (void*)&val;
    }
    
    return NULL;
}


static void
dag37t_reset_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        dag37t_state_t* state = (dag37t_state_t*) card_get_private_state(card);
        uint32_t sonic_e1t1_base = state->mSonicE1T1Base;
        
        /* Only reset the framer. */
        if (0 != sonic_e1t1_base)
        {
            dag37t_wait_until_not_busy(card);
            card_write_iom(card, sonic_e1t1_base + SONIC_E1T1_CONFIG, STE_CONFIG_RESET);
        }
    }
}

static AttributePtr
dag37t_get_new_timestamp_end_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeTimeStampEnd);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_timestamp_end_dispose);
        attribute_set_getvalue_routine(result, dag37t_timestamp_end_get_value);
        attribute_set_setvalue_routine(result, dag37t_timestamp_end_set_value);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "ts_end");
	attribute_set_description(result, "Indicates when the timestamp is to be added to the record");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}


static void
dag37t_timestamp_end_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void*
dag37t_timestamp_end_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;


    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);

		/*read register */
        register_val = card_read_iom(card, state->mDemapperBase);

		val = (register_val & BIT14)?1:0;

        return (void*)&val;
    }
    return NULL;

}


static void
dag37t_timestamp_end_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    dag37t_state_t* state = NULL;
    uint32_t register_val;
    uint8_t tail = *((uint8_t *)value);

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);

        /*read, re/set bit 14, and write to the latch register */
        register_val = card_read_iom(card, state->mDemapperBase);


		if(tail == 1)
			register_val |= BIT14;
		else
			register_val &= ~BIT14;

        card_write_iom(card, state->mDemapperBase, register_val);
    }

}

static AttributePtr
dag37t_get_new_receive_loss_of_signal_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReceiveLossOfSignal);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_receive_loss_of_signal_dispose);
        attribute_set_getvalue_routine(result, dag37t_receive_loss_of_signal_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "los");
        attribute_set_description(result, "Loss of Signal");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_receive_loss_of_signal_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag37t_receive_loss_of_signal_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    int base;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;
    int c;


    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;
        register_val = card_read_iom(card, base + (c*16 + EXAR8338_CHAN_STAT)*4);
        val = (register_val & BIT1) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_alarm_signal_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAlarmSignal);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_alarm_signal_dispose);
        attribute_set_getvalue_routine(result, dag37t_alarm_signal_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "ais");
        attribute_set_description(result, "Alarm Indication Signal");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_alarm_signal_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}

static void*
dag37t_alarm_signal_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    int base;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;
    int c;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;
        register_val = card_read_iom(card, base + (c*16 + EXAR8338_CHAN_STAT)*4);
        val = (register_val & BIT2) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_driver_monitor_output_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeDriverMonitorOutput);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_driver_monitor_output_dispose);
        attribute_set_getvalue_routine(result, dag37t_driver_monitor_output_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "dmo");
	attribute_set_description(result, "Indicates when a transmit driver failure is detected");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;

}

static void
dag37t_driver_monitor_output_dispose(AttributePtr attribute)
{

}

static void*
dag37t_driver_monitor_output_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    int base;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    static uint32_t register_val;
    int c;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;
        register_val = card_read_iom(card, base + (c*16 + EXAR8338_CHAN_STAT)*4);
        val = (register_val & BIT6) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_fifo_limit_status_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFIFOLimitStatus);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_fifo_limit_status_dispose);
        attribute_set_getvalue_routine(result, dag37t_fifo_limit_status_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "fls");
	attribute_set_description(result, "Indicates that the jitter attenuator read/write FIFO pointers are within 3bits");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_fifo_limit_status_dispose(AttributePtr attribute)
{

}

static void*
dag37t_fifo_limit_status_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    int base;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;
    int c;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;
        register_val = card_read_iom(card, base + (c*16 + EXAR8338_CHAN_STAT)*4);
        val = (register_val & BIT5) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_line_code_violation_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLineCodeViolation);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_line_code_violation_dispose);
        attribute_set_getvalue_routine(result, dag37t_line_code_violation_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "lcv");
        attribute_set_description(result, "Line Code Violation");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_line_code_violation_dispose(AttributePtr attribute)
{

}

static void*
dag37t_line_code_violation_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    int base;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;
    int c;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;
        register_val = card_read_iom(card, base + (c*16 + EXAR8338_CHAN_STAT)*4);
        val = (register_val & BIT4) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_cable_loss_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCableLoss);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_cable_loss_dispose);
        attribute_set_getvalue_routine(result, dag37t_cable_loss_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
	attribute_set_name(result, "clos");
	attribute_set_description(result, "Represents the cable attenuation indication within 1dB");
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
dag37t_cable_loss_dispose(AttributePtr attribute)
{

}

static void*
dag37t_cable_loss_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    int base;
    dag37t_state_t* state = NULL;
    static uint32_t val;
    uint32_t register_val;
    int c;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uExarPhysicalMapping[line];
        base = (physical < 8) ? state->mPhyBase : state->mPhyBase2;
        c = physical % 8;
        register_val = card_read_iom(card, base + (c*16 + EXAR8338_CHAN_CLOS)*4);
        val = (register_val & 0x3f);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_e1t1_rx0_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1Rx0);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_rx0_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_rx0_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "rx0");
        attribute_set_description(result, "Indicates nothing is being processed by SONIC E1/T1 Framer");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_e1t1_rx0_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_rx0_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        register_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (register_val & STE_STAT_EVER_RX_ZERO) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_e1t1_rx1_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1Rx1);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_rx1_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_rx1_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "rx1");
        attribute_set_description(result, "Indicates nothing is being processed by SONIC E1/T1 Framer");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;

}

static void
dag37t_e1t1_rx1_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_rx1_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        register_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (register_val & STE_STAT_EVER_RX_ONE) != 0;
        return (void*)&val;
    }
    return NULL;

}

static AttributePtr
dag37t_get_new_e1t1_tx0_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1Tx0);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_tx0_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_tx0_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "tx0");
        attribute_set_description(result, "Indicates nothing is being processed by SONIC E1/T1 Framer");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_e1t1_tx0_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_tx0_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        register_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (register_val & STE_STAT_EVER_TX_ZERO) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_e1t1_tx1_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1Tx1);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_tx1_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_tx1_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "tx1");
        attribute_set_description(result, "Indicates nothing is being processed by SONIC E1/T1 Framer");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_e1t1_tx1_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_tx1_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        register_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (register_val & STE_STAT_EVER_TX_ONE) != 0;
        return (void*)&val;
    }
    return NULL;

}

static AttributePtr
dag37t_get_new_e1t1_framer_error_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1FramerError);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_framer_error_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_framer_error_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "ferr");
        attribute_set_description(result,"Indicates if there is a framing error");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_e1t1_framer_error_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_framer_error_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        register_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (register_val & STE_STAT_SEEN_FRAMER_ERROR) != 0;
        return (void*)&val;
    }
    return NULL;

}

static AttributePtr
dag37t_get_new_e1t1_crc_error_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1CRCError);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_crc_error_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_crc_error_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "crcerr");
        attribute_set_description(result, "Indicates if there was a CRC error");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;
}

static void
dag37t_e1t1_crc_error_dispose(AttributePtr attribute)
{
}

static void*
dag37t_e1t1_crc_error_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        register_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (register_val & STE_STAT_SEEN_CRC_ERROR) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_e1t1_ais_error_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1AISError);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_ais_error_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_ais_error_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "aiserr");
	attribute_set_description(result, "Indicates if there was an Alarm Indication Signal error");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;

}

static void
dag37t_e1t1_ais_error_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_ais_error_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        register_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (register_val & STE_STAT_SEEN_AIS_ERROR) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr dag37t_get_new_e1t1_framer_counter_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeE1T1FramerCounter);
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_framer_counter_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_framer_counter_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_name(result, "frm_counter");
        attribute_set_description(result,"Number of framing errors");
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
                    
    return result;
}

static void
dag37t_e1t1_framer_counter_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_framer_counter_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint32_t val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL, 1 | (physical<<4));
        val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_DATA);
        return (void*)&val;
    }
    
    return NULL;
}

static AttributePtr dag37t_get_new_e1t1_ais_counter_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeE1T1AISCounter);
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_ais_counter_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_ais_counter_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_name(result, "ais_counter");
        attribute_set_description(result,"Number of Alarm Indication Signal errors");
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
                    
    return result;
}

static void
dag37t_e1t1_ais_counter_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_ais_counter_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint32_t val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL, 4 | (physical<<4));
        val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_DATA);
        return (void*)&val;
    }
    
    return NULL;
}

static AttributePtr dag37t_get_new_e1t1_crc_counter_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeE1T1CRCCounter);
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_crc_counter_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_crc_counter_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_name(result, "crc_counter");
        attribute_set_description(result,"Number of CRC errors");
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
                    
    return result;
}

static void
dag37t_e1t1_crc_counter_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_crc_counter_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint32_t val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        card_write_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_CTRL, 2 | (physical<<4));
        val = card_read_iom(card, state->mSonicE1T1Base + SONIC_E1T1_COUNTER_DATA);
        return (void*)&val;
    }
    
    return NULL;
}

static AttributePtr
dag37t_get_new_e1t1_link_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeE1T1Link);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_e1t1_link_dispose);
        attribute_set_getvalue_routine(result, dag37t_e1t1_link_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
	attribute_set_name(result, "link");
	attribute_set_description(result, "Indicates if the link is up");
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
    }
    
    return result;

}

static void
dag37t_e1t1_link_dispose(AttributePtr attribute)
{

}

static void*
dag37t_e1t1_link_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    int line = 0;
    int physical = 0;
    ComponentPtr port;
    dag37t_state_t* state = NULL;
    static uint8_t val;
    uint32_t read_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        port = attribute_get_component(attribute);
        line = dag37t_port_get_line(port);
        physical = card_state->uStePhysicalMapping[line];
        read_val = card_read_iom(card, state->mSonicE1T1Base + physical*4);
        val = (read_val & STE_STAT_IS_UP) != 0;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_drop_count_attribute(void)
{    
    AttributePtr result = attribute_init(kUint32AttributeDropCount);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_drop_count_dispose);
        attribute_set_getvalue_routine(result, dag37t_drop_count_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
	attribute_set_name(result, "drop_count");
	attribute_set_description(result, "Number of packets dropped");
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
dag37t_drop_count_dispose(AttributePtr attribute)
{

}

static void*
dag37t_drop_count_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    demapper_type_t demapper_type = 0;
    ComponentPtr demapper;
    dag37t_state_t* state = NULL;
    static uint32_t val;
    //uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        demapper = attribute_get_component(attribute);
        demapper_type = dag37t_demapper_get_type(demapper);
        /*read, set bit 15, and write to the latch register */
        //register_val = card_read_iom(card, state->mDemapperBase);
       // register_val |= BIT15;
        //card_write_iom(card, state->mDemapperBase, register_val);
        if (demapper_type == kDemapperTypeHDLC)
        {
            val = card_read_iom(card, state->mDemapperBase + HDLC_DROP_COUNT_REG);
        }
        else if (demapper_type == kDemapperTypeATM)
        {
            val = card_read_iom(card, state->mDemapperBase + ATM_DROP_COUNT_REG);
        }
        return (void*)&val;
    }
    return NULL;
}


static AttributePtr
dag37t_get_new_lcd_attribute(void)
{    
    AttributePtr result = attribute_init(kUint32AttributeLossOfCellDelineationCount);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_lcd_dispose);
        attribute_set_getvalue_routine(result, dag37t_lcd_get_value);
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
	attribute_set_name(result, "lcd");
	attribute_set_description(result, "Number of LCD (Loss of Cell Delineation)  instances");
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
dag37t_lcd_dispose(AttributePtr attribute)
{

}

static void*
dag37t_lcd_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    demapper_type_t demapper_type = 0;
    ComponentPtr demapper;
    dag37t_state_t* state = NULL;
    static uint32_t val;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        state = card_get_private_state(card);
        demapper = attribute_get_component(attribute);
        demapper_type = dag37t_demapper_get_type(demapper);

		if (demapper_type == kDemapperTypeATM)
        {
            val = card_read_iom(card, state->mDemapperBase + ATM_LCD_STATUS_REGISTER);
        }
        return (void*)&val;
    }
    return NULL;
}



static AttributePtr 
dag37t_get_new_erf_mux_attribute(void)
{
    AttributePtr result = attribute_init(kStructAttributeErfMux);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_erf_mux_dispose);
        attribute_set_setvalue_routine(result, dag37t_erf_mux_set_value);
	attribute_set_name(result, "d37t_set_erf_mux");
        attribute_set_description(result, "Set the Erf Mux on the 3.7T card (DEPRECIATED)");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeStruct);
    }
    
    return result;
}
static void 
dag37t_erf_mux_dispose(AttributePtr attribute)
{

}

static void 
dag37t_erf_mux_set_value(AttributePtr attribute, void* value, int length)
{
    erf_mux_37t_t* mux = (erf_mux_37t_t*)value;
    DagCardPtr card;

    if (1 == valid_attribute(attribute))
    { 
        card = attribute_get_card(attribute);
	dag_set_mux(card_get_fd(card), mux->mHost, mux->mLine, mux->mXscale);
    }
}


static demapper_type_t
dag37t_demapper_get_type(ComponentPtr demapper)
{
    dag37t_demapper_state_t* state = component_get_private_state(demapper);

    assert(component_get_component_code(demapper) == kComponentDemapper);
    return state->mDemapperType;
}
    
static void
dag37t_demapper_set_type(ComponentPtr demapper, demapper_type_t demapper_type)
{
    dag37t_demapper_state_t* state;
    assert(component_get_component_code(demapper) == kComponentDemapper);
    
    state = component_get_private_state(demapper);
    state->mDemapperType = demapper_type;
}


static AttributePtr
dag37t_get_new_add_connection_attribute(void)
{
    AttributePtr result = attribute_init(kStructAttributeAddConnection);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_add_connection_dispose);
        attribute_set_setvalue_routine(result, dag37t_add_connection_set_value);
	attribute_set_name(result, "d37t_add_connection");
        attribute_set_description(result, "Add a connection on the 3.7T card");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeStruct);
    }
    
    return result;
}


static void
dag37t_add_connection_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_add_connection_set_value(AttributePtr attribute, void* value, int length)
{   
	connection_description_37t_t* cd = (connection_description_37t_t*)value;
    DagCardPtr card;

    if (1 == valid_attribute(attribute))
    { 
        card = attribute_get_card(attribute);

        if(cd->mPayloadType == kPayloadTypeATM)
            cd->mPayloadType = CT_FMT_ATM;
        else if(cd->mPayloadType == kPayloadTypeHDLC)
            cd->mPayloadType = CT_FMT_HDLC;
        else
            cd->mPayloadType = CT_FMT_DEFAULT;
        
		switch(cd->mConnectionType)
		{
		case kConnectionTypeChan:
		case kConnectionTypeChanRaw:
            if(cd->mTimeslot == 0 || cd->mTimeslot >= 32 )
			{
				dagutil_verbose_level(1, "Timeslot out of range (1-31), Timeslot = %d\n", cd->mTimeslot);
				connection_num = -1;
			}
			else
			{
                connection_num = dag_add_channel(card_get_fd(card), cd->mDirection, 
					(cd->mConnectionType | cd->mPayloadType), cd->mline, cd->mTimeslot, 0);
                cd->mConnectionNumber = connection_num;
			}
			break;

		case kConnectionTypeHyper:
		case kConnectionTypeHyperRaw:
			connection_num = dag_add_channel(card_get_fd(card), cd->mDirection, 
				(cd->mConnectionType | cd->mPayloadType), cd->mline, cd->mMask, 0);
            cd->mConnectionNumber = connection_num;
            break;

		case kConnectionTypeSub:
		case kConnectionTypeSubRaw:
            if(cd->mTimeslot == 0 || cd->mTimeslot >= 32 )
			{
                dagutil_verbose_level(1, "Timeslot out of range (1-31), timeslot = 0x%x\n", cd->mTimeslot);
				connection_num = -1;
			}
			else
			{
				connection_num = dag_add_channel(card_get_fd(card), cd->mDirection, 
                    (cd->mConnectionType | cd->mPayloadType), cd->mline, cd->mMask | (cd->mTimeslot<<16), 0);
                cd->mConnectionNumber = connection_num;
			}

			break;

		case kConnectionTypeRaw:
			connection_num = dag_add_channel(card_get_fd(card), cd->mDirection, 
				(cd->mConnectionType | cd->mPayloadType), cd->mline, 0xFFFFFFFF, 0);
            cd->mConnectionNumber = connection_num;

			break;

		default:
			return;
			break;


		}
    }
}





static AttributePtr
dag37t_get_new_get_connection_number_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeGetLastConnectionNumber); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_get_connection_number_dispose);
        attribute_set_getvalue_routine(result, dag37t_get_connection_number_get_value);
        attribute_set_name(result, "get_connection_number");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag37t_get_connection_number_dispose(AttributePtr attribute)
{
}

static void*
dag37t_get_connection_number_get_value(AttributePtr attribute)
{
    static uint32_t temp = 0;
    if (1 == valid_attribute(attribute))
    { 
        temp = connection_num;
        return (void*)&temp;
    }
    return NULL;
}

static AttributePtr
dag37t_get_new_delete_connection_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDeleteConnection);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_delete_connection_dispose);
        attribute_set_setvalue_routine(result, dag37t_delete_connection_set_value);
	attribute_set_name(result, "d37t_delete_connection");
        attribute_set_description(result, "Delete a connection on the 3.7T card");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}


static void
dag37t_delete_connection_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_delete_connection_set_value(AttributePtr attribute, void* value, int length)
{   
    DagCardPtr card;

    if (1 == valid_attribute(attribute))
    { 
        card = attribute_get_card(attribute);
		dag_delete_channel(card_get_fd(card), *(uint32_t*)value);
    }
}

static AttributePtr
dag37t_get_new_delete_all_connections_attribute(void)
{
    AttributePtr result = attribute_init(kNullAttributeClearConnections);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag37t_delete_all_connections_dispose);
        attribute_set_setvalue_routine(result, dag37t_delete_all_connections_set_value);
	attribute_set_name(result, "d37t_delete_all_connections");
        attribute_set_description(result, "Deletes all connections on the 3.7T card");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
    }
    
    return result;
}


static void
dag37t_delete_all_connections_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
dag37t_delete_all_connections_set_value(AttributePtr attribute, void* value, int length)
{   
    DagCardPtr card;

    if (1 == valid_attribute(attribute))
    { 
        card = attribute_get_card(attribute);
		dag_delete_board_channels(card_get_fd(card));
    }
}






/* Initialization routine. */
dag_err_t
dag37t_initialize(DagCardPtr card)
{
    dag37t_state_t* state;
    AttributePtr framer_clear;
    AttributePtr framer_reset;
    AttributePtr drop_count;
    AttributePtr latch;
    AttributePtr lcd;
    AttributePtr timestamp_end;
    ComponentPtr root_component;
    ComponentPtr led_controller;
    AttributePtr period_0;
    AttributePtr period_1;
    AttributePtr duty_cycle_0;
    AttributePtr duty_cycle_1;
    AttributePtr led_status_0;
    AttributePtr led_status_1;
    AttributePtr erf_mux;
    AttributePtr add_connection;
    AttributePtr connection_number;
    AttributePtr delete_connection;
    AttributePtr delete_all_connections;
    int line;
    int connections;
    char buffer[BUFFER_SIZE];
    uint32_t bit_mask = 0xFFFFFFFF;
    GenericReadWritePtr grw = NULL;
    
    card_state = (dag37t4_card_state_t*) malloc(sizeof(dag37t4_card_state_t));

    assert(NULL != card);
     
    if(card_get_card_type(card) == kDag37t4)
    {
	card_state->maxPort = D37T4_MAX_LINES;
	card_state->uExarPhysicalMapping = uExarPhysical_t4;
	//card_state->uStePhysicalMapping = uStePhysical_t4;
        card_state->uStePhysicalMapping = uStePhysical;

    }
    else if(card_get_card_type(card) == kDag37t)
    {
	card_state->maxPort = D37T_MAX_LINES;
        card_state->uExarPhysicalMapping = uExarPhysical;
        card_state->uStePhysicalMapping = uStePhysical;
    }

    /* Set up virtual methods. */
    card_set_dispose_routine(card, dag37t_dispose);
    card_set_post_initialize_routine(card, dag37t_post_initialize);
    card_set_reset_routine(card, dag37t_reset);
    card_set_default_routine(card, dag37t_default);
    card_set_update_register_base_routine(card, dag37t_update_register_base);

    /* Allocate private state. */
    state = (dag37t_state_t*) malloc(sizeof(dag37t_state_t));
    memset(state, 0, sizeof(dag37t_state_t));
    card_set_private_state(card, (void*) state);


    /* Create root component. */
    root_component = component_init(kComponentRoot, card);

    /* Add root component to card. */
    card_set_root_component(card, root_component);
     
   
    state->mPbm = pbm_get_new_pbm(card, 0);
    component_add_subcomponent(root_component, state->mPbm);


    for (line = 0; line < card_state->maxPort; line++)
    {
        AttributePtr mode_attribute;
        AttributePtr linetype_attribute;
        AttributePtr termination_attribute;
        AttributePtr floopback_attribute;
        AttributePtr eqloopback_attribute;
        AttributePtr zerocode_attribute;
        AttributePtr receive_loss_of_signal;
        AttributePtr line_code_violation;
        AttributePtr fifo_limit_status;
        AttributePtr driver_monitor_output;
        AttributePtr alarm_signal;
        AttributePtr cable_loss;
        AttributePtr e1t1_rx0;
        AttributePtr e1t1_rx1;
        AttributePtr e1t1_tx0;
        AttributePtr e1t1_tx1;
        AttributePtr e1t1_framer_error;
        AttributePtr e1t1_ais_error;
        AttributePtr e1t1_crc_error;
        AttributePtr e1t1_framer_counter;
        AttributePtr e1t1_ais_counter;
        AttributePtr e1t1_crc_counter;
        AttributePtr e1t1_link;
        AttributePtr rxpkts;
        AttributePtr txpkts;
        AttributePtr scrambling;
        AttributePtr hec_correction;
        AttributePtr active_port;
        AttributePtr latch_and_clear;

        /* Create new attributes. */
        mode_attribute = dag37t_get_new_mode_attribute();
        linetype_attribute = dag37t_get_new_linetype_attribute();
        termination_attribute = dag37t_get_new_termination_attribute();
        floopback_attribute = dag37t_get_new_facility_loopback_attribute();
        eqloopback_attribute = dag37t_get_new_equipment_loopback_attribute();
        zerocode_attribute = dag37t_get_new_zerocode_attribute();
        receive_loss_of_signal = dag37t_get_new_receive_loss_of_signal_attribute();
        line_code_violation = dag37t_get_new_line_code_violation_attribute();
        fifo_limit_status = dag37t_get_new_fifo_limit_status_attribute();
        driver_monitor_output = dag37t_get_new_driver_monitor_output_attribute();
        alarm_signal = dag37t_get_new_alarm_signal_attribute();
        cable_loss = dag37t_get_new_cable_loss_attribute();
        e1t1_rx0 = dag37t_get_new_e1t1_rx0_attribute();
        e1t1_rx1 = dag37t_get_new_e1t1_rx1_attribute();
        e1t1_tx0 = dag37t_get_new_e1t1_tx0_attribute();
        e1t1_tx1 = dag37t_get_new_e1t1_tx1_attribute();
        e1t1_framer_error = dag37t_get_new_e1t1_framer_error_attribute();
        e1t1_ais_error = dag37t_get_new_e1t1_ais_error_attribute();
        e1t1_crc_error = dag37t_get_new_e1t1_crc_error_attribute();
        e1t1_framer_counter = dag37t_get_new_e1t1_framer_counter_attribute();
        e1t1_ais_counter = dag37t_get_new_e1t1_ais_counter_attribute();
        e1t1_crc_counter = dag37t_get_new_e1t1_crc_counter_attribute();
        e1t1_link = dag37t_get_new_e1t1_link_attribute();
        rxpkts = dag37t_get_new_rxpkts_attribute();
        txpkts = dag37t_get_new_txpkts_attribute();
	scrambling = dag37t_get_new_scrambling_attribute();
	hec_correction = dag37t_get_new_hec_correction_attribute();
        active_port = dag37t_get_new_active_port_attribute();
        latch_and_clear = dag37t_get_new_latch_and_clear_attribute();

        /* Add kComponentPort to the root component. */
        state->mPort[line] = dag37t_get_new_port(card, line);
        
        /* Add attributes to the port. */
        component_add_attribute(state->mPort[line], mode_attribute);
        component_add_attribute(state->mPort[line], linetype_attribute);
        component_add_attribute(state->mPort[line], termination_attribute);
        component_add_attribute(state->mPort[line], floopback_attribute);
        component_add_attribute(state->mPort[line], eqloopback_attribute);
        component_add_attribute(state->mPort[line], zerocode_attribute);
        component_add_attribute(state->mPort[line], fifo_limit_status);
        component_add_attribute(state->mPort[line], alarm_signal);
        component_add_attribute(state->mPort[line], driver_monitor_output);
        component_add_attribute(state->mPort[line], line_code_violation);
        component_add_attribute(state->mPort[line], receive_loss_of_signal);
        component_add_attribute(state->mPort[line], cable_loss);
        component_add_attribute(state->mPort[line], e1t1_link);
        component_add_attribute(state->mPort[line], e1t1_crc_error);
        component_add_attribute(state->mPort[line], e1t1_ais_error);
        component_add_attribute(state->mPort[line], e1t1_framer_error);
        component_add_attribute(state->mPort[line], e1t1_framer_counter);
        component_add_attribute(state->mPort[line], e1t1_ais_counter);
        component_add_attribute(state->mPort[line], e1t1_crc_counter);
        component_add_attribute(state->mPort[line], e1t1_tx1);
        component_add_attribute(state->mPort[line], e1t1_tx0);
        component_add_attribute(state->mPort[line], e1t1_rx1);
        component_add_attribute(state->mPort[line], e1t1_rx0);
        component_add_attribute(state->mPort[line], rxpkts);
        component_add_attribute(state->mPort[line], txpkts);
        component_add_attribute(state->mPort[line], scrambling);
        component_add_attribute(state->mPort[line], hec_correction);
        component_add_attribute(state->mPort[line], active_port);
        component_add_attribute(state->mPort[line], latch_and_clear);

        sprintf(buffer, "port%d", line);
        component_set_name(state->mPort[line], buffer);
        
        component_add_subcomponent(root_component, state->mPort[line]);
    }

    led_controller = get_new_led_controller(card);
    period_0 = get_new_period_attribute(kPCAAddress0, kPeriodRegister0, 0);
    period_1 = get_new_period_attribute(kPCAAddress1, kPeriodRegister0, 1);
    duty_cycle_0 = get_new_duty_cycle_attribute(kPCAAddress0, kDutyCycleRegister0, 0);
    duty_cycle_1 = get_new_duty_cycle_attribute(kPCAAddress1, kDutyCycleRegister0, 1);
    component_add_attribute(led_controller, period_0);
    component_add_attribute(led_controller, period_1);
    component_add_attribute(led_controller, duty_cycle_0);
    component_add_attribute(led_controller, duty_cycle_1);

    for (line = 0; line < card_state->maxPort; line++)
    {
        led_status_0 = get_new_led_status_attribute(uLEDTable[line*2].mPCAAddress, uLEDTable[line*2].mRegister, uLEDTable[line*2].mBank, line);
        led_status_1 = get_new_led_status_attribute(uLEDTable[line*2+1].mPCAAddress, uLEDTable[line*2+1].mRegister, uLEDTable[line*2+1].mBank, line+16);
        component_add_attribute(led_controller, led_status_0);
        component_add_attribute(led_controller, led_status_1);
    }
    component_add_subcomponent(root_component, led_controller);
 
    state->mE1T1Framer = dag37t_get_new_framer(card);
    
    /* Create new attributes. */
    framer_clear = dag37t_get_new_clear_attribute();
    framer_reset = dag37t_get_new_reset_attribute();

    /* Add attributes to kComponentFramer. */
    component_add_attribute(state->mE1T1Framer, framer_clear);
    component_add_attribute(state->mE1T1Framer, framer_reset);

    /* Add kComponentFramer to the root component. */
    component_add_subcomponent(root_component, state->mE1T1Framer);

    state->mDemapper = dag37t_get_new_demapper(card);
    
    /* Add latch */
    bit_mask = BIT15;
    grw = grw_init(card, state->mDemapperBase , grw_iom_read, grw_iom_write);
    latch = attribute_factory_make_attribute(kBooleanAttributeCounterLatch, grw, &bit_mask,1);
    component_add_attribute(state->mDemapper, latch);
    drop_count = dag37t_get_new_drop_count_attribute();
    component_add_attribute(state->mDemapper, drop_count);

    lcd = dag37t_get_new_lcd_attribute();
    component_add_attribute(state->mDemapper, lcd);

    timestamp_end = dag37t_get_new_timestamp_end_attribute();
    component_add_attribute(state->mDemapper, timestamp_end);

    erf_mux = dag37t_get_new_erf_mux_attribute();
    component_add_attribute(state->mDemapper, erf_mux);

    component_add_subcomponent(root_component, state->mDemapper);  

    /* Add erfmux component */
    state->mErfMux = dag37t_mux_get_new_component(card);
    component_add_subcomponent(root_component, state->mErfMux);

    /* Connection attributes */
    state->mConnectionSetup = dag37t_get_new_connection_config(card);

    add_connection = dag37t_get_new_add_connection_attribute();
    component_add_attribute(state->mConnectionSetup, add_connection);

    connection_number = dag37t_get_new_get_connection_number_attribute();
    component_add_attribute(state->mConnectionSetup, connection_number);

    delete_connection = dag37t_get_new_delete_connection_attribute();
    component_add_attribute(state->mConnectionSetup, delete_connection);

    delete_all_connections = dag37t_get_new_delete_all_connections_attribute();
    component_add_attribute(state->mConnectionSetup, delete_all_connections);

    component_add_subcomponent(root_component, state->mConnectionSetup); 

    for (connections = 0; connections < 512; connections++)
    {
 	    state->mConnection[connections] = dag37t_get_new_connection(card, connections);
    	component_add_subcomponent(state->mConnectionSetup, state->mConnection[connections]); 
    }

    return kDagErrNone;
}

