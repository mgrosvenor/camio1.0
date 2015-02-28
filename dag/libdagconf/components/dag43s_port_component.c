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
#include "../include/components/pbm_component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/cards/dag4_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/dag43s_port_component.h"
#include "../include/attribute_factory.h"
#include "../include/cards/dag43s_impl.h"
#include "../include/components/gpp_component.h"

#include "dag_platform.h"
#include "dagutil.h"

#define BUFFER_SIZE 1024
#define IW_Enable   0x10000
#define GPP_ATM_Mode 0x20000


typedef enum dagctrl {
  INT_ENABLES     = 0x008c, /* from 0x0 */
  SONET_cs        = 0x00c0  /* from 0x0 */
} dagctrl_t;

typedef struct
{
    uint32_t mBase;
    int mIndex;
} port_state_t;


/* port component. */
static void dag43s_port_dispose(ComponentPtr component);
static void dag43s_port_reset(ComponentPtr component);
static void dag43s_port_default(ComponentPtr component);
static int dag43s_port_post_initialize(ComponentPtr component);
static dag_err_t dag43s_port_update_register_base(ComponentPtr component);

/* Facility Loopback */
static AttributePtr get_new_fcl(void);
static void fcl_dispose(AttributePtr attribute);
static void fcl_post_initialize(AttributePtr attribute);
static void* fcl_get_value(AttributePtr attribute);
static void fcl_set_value(AttributePtr attribute, void* value, int length);

/* Payload Scramble */
static AttributePtr get_new_pscramble(void);
static void pscramble_dispose(AttributePtr attribute);
static void pscramble_post_initialize(AttributePtr attribute);
static void* pscramble_get_value(AttributePtr attribute);
static void pscramble_set_value(AttributePtr attribute, void* value, int length);

/* Master Slave */
static AttributePtr get_new_master_slave(void);
static void master_slave_dispose(AttributePtr attribute);
static void master_slave_post_initialize(AttributePtr attribute);
static void* master_slave_get_value(AttributePtr attribute);
static void master_slave_set_value(AttributePtr attribute, void* value, int length);
static void master_slave_from_string_routine(AttributePtr attribute, const char* string);
static void master_slave_to_string_routine(AttributePtr attribute);

/* Scramble */
static AttributePtr get_new_scramble(void);
static void scramble_dispose(AttributePtr attribute);
static void scramble_post_initialize(AttributePtr attribute);
static void* scramble_get_value(AttributePtr attribute);
static void scramble_set_value(AttributePtr attribute, void* value, int length);

/* Link Reset */
static AttributePtr get_new_link_reset(void);
static void link_reset_dispose(AttributePtr attribute);
static void link_reset_post_initialize(AttributePtr attribute);
static void* link_reset_get_value(AttributePtr attribute);
static void link_reset_set_value(AttributePtr attribute, void* value, int length);

/* Link Discard*/
static AttributePtr get_new_link_discard(void);
static void link_discard_dispose(AttributePtr attribute);
static void link_discard_post_initialize(AttributePtr attribute);
static void* link_discard_get_value(AttributePtr attribute);
static void link_discard_set_value(AttributePtr attribute, void* value, int length);

/* oof attribute. */
static AttributePtr get_new_oof(void);
static void oof_dispose(AttributePtr attribute);
static void* oof_get_value(AttributePtr attribute);
static void oof_post_initialize(AttributePtr attribute);

/* lais attribute. */
static AttributePtr get_new_lais(void);
static void lais_dispose(AttributePtr attribute);
static void* lais_get_value(AttributePtr attribute);
static void lais_post_initialize(AttributePtr attribute);

/* lrdis attribute. */
static AttributePtr get_new_lrdis(void);
static void lrdis_dispose(AttributePtr attribute);
static void* lrdis_get_value(AttributePtr attribute);
static void lrdis_post_initialize(AttributePtr attribute);

/*dool attribute. */
static AttributePtr get_new_dool(void);
static void dool_dispose(AttributePtr attribute);
static void* dool_get_value(AttributePtr attribute);
static void dool_post_initialize(AttributePtr attribute);

/* rool attribute. */
static AttributePtr get_new_rool(void);
static void rool_dispose(AttributePtr attribute);
static void* rool_get_value(AttributePtr attribute);
static void rool_post_initialize(AttributePtr attribute);

/* Path BIP attribute. */
static AttributePtr get_new_path_bip_error(void);
static void path_bip_error_dispose(AttributePtr attribute);
static void* path_bip_error_get_value(AttributePtr attribute);
static void path_bip_error_post_initialize(AttributePtr attribute);

/* Path REI attribute. */
static AttributePtr get_new_path_rei_error(void);
static void path_rei_error_dispose(AttributePtr attribute);
static void* path_rei_error_get_value(AttributePtr attribute);
static void path_rei_error_post_initialize(AttributePtr attribute);

/* latch and clear attribute. */
static AttributePtr get_new_latch_and_clear(void);
static void latch_and_clear_dispose(AttributePtr attribute);
static void* latch_and_clear_get_value(AttributePtr attribute);
static void latch_and_clear_set_value(AttributePtr attribute, void* value, int length);
static void latch_and_clear_post_initialize(AttributePtr attribute);

/* crc attribute. */
static AttributePtr get_new_crc(void);
static void crc_dispose(AttributePtr attribute);
static void* crc_get_value(AttributePtr attribute);
static void crc_set_value(AttributePtr attribute, void* value, int length);
static void crc_post_initialize(AttributePtr attribute);
static void crc_to_string_routine(AttributePtr attribute);
static void crc_from_string_routine(AttributePtr attribute, const char* string);

/* min pkt len*/
static AttributePtr get_new_min_pkt_len(void);
static void min_pkt_len_dispose(AttributePtr attribute);
static void min_pkt_len_post_initialize(AttributePtr attribute);
static void* min_pkt_len_get_value(AttributePtr attribute);
static void min_pkt_len_set_value(AttributePtr attribute, void* value, int length);

/* max pkt len */
static AttributePtr get_new_max_pkt_len(void);
static void max_pkt_len_dispose(AttributePtr attribute);
static void max_pkt_len_post_initialize(AttributePtr attribute);
static void* max_pkt_len_get_value(AttributePtr attribute);
static void max_pkt_len_set_value(AttributePtr attribute, void* value, int length);

/* Path Label attribute. */
static AttributePtr get_new_path_label(void);
static void path_label_dispose(AttributePtr attribute);
static void* path_label_get_value(AttributePtr attribute);
static void path_label_post_initialize(AttributePtr attribute);

/* RX Frames attribute. */
static AttributePtr get_new_rx_frames(void);
static void rx_frames_dispose(AttributePtr attribute);
static void* rx_frames_get_value(AttributePtr attribute);
static void rx_frames_post_initialize(AttributePtr attribute);

/* hec count attribute. */
static AttributePtr get_new_fcs_hec_fail_count(void);
static void fcs_hec_fail_count_dispose(AttributePtr attribute);
static void* fcs_hec_fail_count_get_value(AttributePtr attribute);
static void fcs_hec_fail_count_post_initialize(AttributePtr attribute);

/* tx frames attribute. */
static AttributePtr get_new_tx_frames(void);
static void tx_frames_dispose(AttributePtr attribute);
static void* tx_frames_get_value(AttributePtr attribute);
static void tx_frames_post_initialize(AttributePtr attribute);

/* tx bytes attribute. */
static AttributePtr get_new_tx_bytes(void);
static void tx_bytes_dispose(AttributePtr attribute);
static void* tx_bytes_get_value(AttributePtr attribute);
static void tx_bytes_post_initialize(AttributePtr attribute);

/* txfdrop attribute. */
static AttributePtr get_new_txfdrop(void);
static void txfdrop_dispose(AttributePtr attribute);
static void* txfdrop_get_value(AttributePtr attribute);
static void txfdrop_post_initialize(AttributePtr attribute);

/* rx bytes attribute. */
static AttributePtr get_new_rx_bytes(void);
static void rx_bytes_dispose(AttributePtr attribute);
static void* rx_bytes_get_value(AttributePtr attribute);
static void rx_bytes_post_initialize(AttributePtr attribute);

/* Aborts attribute. */
static AttributePtr get_new_aborts(void);
static void aborts_dispose(AttributePtr attribute);
static void* aborts_get_value(AttributePtr attribute);
static void aborts_post_initialize(AttributePtr attribute);

/* Short attribute. */
static AttributePtr get_new_short(void);
static void short_dispose(AttributePtr attribute);
static void* short_get_value(AttributePtr attribute);
static void short_post_initialize(AttributePtr attribute);

/* long attribute. */
static AttributePtr get_new_long(void);
static void long_dispose(AttributePtr attribute);
static void* long_get_value(AttributePtr attribute);
static void long_post_initialize(AttributePtr attribute);

/* active attribute */
static AttributePtr get_new_active(void);
static void active_dispose(AttributePtr attribute);
static void* dag43s_port_active_get_value(AttributePtr attribute);
static void dag43s_port_active_set_value(AttributePtr attribute, void* value, int length);

/* rxfdrop attribute. */
static AttributePtr get_new_rxfdrop(void);
static void rxfdrop_dispose(AttributePtr attribute);
static void* rxfdrop_get_value(AttributePtr attribute);
static void rxfdrop_post_initialize(AttributePtr attribute);


/* Network Mode*/
static AttributePtr get_new_network_mode(void);
static void network_mode_dispose(AttributePtr attribute);
static void network_mode_post_initialize(AttributePtr attribute);
static void* network_mode_get_value(AttributePtr attribute);
static void network_mode_set_value(AttributePtr attribute, void* value, int length);
static void network_mode_to_string_routine(AttributePtr attribute);
static void network_mode_from_string_routine(AttributePtr attribute, const char* string);

static uint64_t pmreadloc3(DagCardPtr card, uint32_t base, uint32_t loc);
static uint32_t pmreadloc2(DagCardPtr card, uint32_t base, uint32_t loc);
static uint32_t pmreadloc(DagCardPtr card, uint32_t base, uint32_t loc);
static uint32_t pmreadind(DagCardPtr card, uint32_t base, uint32_t addr, uint32_t path, uint32_t reg);
static void pmwriteind(DagCardPtr card, uint32_t addr, uint32_t path, uint32_t reg, uint32_t param);

/* Network Mode Functions */
void fifos_in_reset(DagCardPtr card, uint32_t base, uint8_t hold_release);
void cfp_enable_disable(DagCardPtr card, uint32_t base, network_mode_t mode, uint8_t enable);
void fifo_disable(DagCardPtr card, uint32_t base);
void fifo_flush(DagCardPtr card, uint32_t base);
void fifo_config(DagCardPtr card, uint32_t base, network_mode_t mode);
void switch_gpp_to_pos_atm(DagCardPtr card, uint32_t base, uint8_t pos);
void phy_burst_size(DagCardPtr card, uint32_t base, uint32_t burst_size);
void phy_enable(DagCardPtr card, uint32_t base);
void sirp_enable(DagCardPtr card, uint32_t base);
void rhpp_invcnt_set(DagCardPtr card, uint32_t base);

/* linerate attribute - dummy */
static AttributePtr get_new_linerate_attribute(void);
static void linerate_dispose(AttributePtr attribute);
static void* linerate_get_value(AttributePtr attribute);
static void linerate_post_initialize(AttributePtr attribute);
static void linerate_set_value(AttributePtr attribute, void* value, int length);
static void line_rate_to_string_routine(AttributePtr attribute);

ComponentPtr
dag43s_get_new_port(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentPort, card);

    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        port_state_t* state = (port_state_t*)malloc(sizeof(port_state_t));
        component_set_dispose_routine(result, dag43s_port_dispose);
        component_set_reset_routine(result, dag43s_port_reset);
        component_set_default_routine(result, dag43s_port_default);
        component_set_post_initialize_routine(result, dag43s_port_post_initialize);
        component_set_update_register_base_routine(result, dag43s_port_update_register_base);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;
}

static dag_err_t
dag43s_port_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = component_get_card(component);
        port_state_t* state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_PM5381, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static void
dag43s_port_dispose(ComponentPtr component)
{

}

static void
dag43s_port_reset(ComponentPtr component)
{
}

static void
dag43s_port_default(ComponentPtr component)
{
    DagCardPtr card = NULL;
    AttributePtr any_attribute = NULL;
    network_mode_t mode;
    uint32_t val = 0;
    port_state_t* state = NULL;
		    
    state = component_get_private_state(component);
    card = component_get_card(component);

    do {
        val = pmreadloc(card, state->mBase,P5381_SU2488_MASTER_RESET);
        val |= (BIT13|BIT14|BIT15);
        card_write_iom(card, state->mBase + (P5381_SU2488_MASTER_RESET << 2), val);

        usleep(2000);

        val = pmreadloc(card, state->mBase,P5381_SU2488_MASTER_RESET);
        val &= ~(BIT13|BIT15);
        val |= BIT0;
        card_write_iom(card, state->mBase + (P5381_SU2488_MASTER_RESET << 2), val);

        card_write_iom(card, state->mBase + (P5381_SU2488_IDENTITY_GPMU << 2), 0);
        usleep(1000);
    } while ((pmreadloc(card, state->mBase, P5381_SU2488_IDENTITY_GPMU) & BIT15));

    val = pmreadloc(card, state->mBase, P5381_RX2488_ANALOG_CTRL);
    val |= (BIT13|BIT15);
    card_write_iom(card, state->mBase + (P5381_RX2488_ANALOG_CTRL << 2), val);

    val = pmreadloc(card, state->mBase, P5381_RRMP_AIS_ENB);
    val |= (BIT0|BIT1|BIT2);
    card_write_iom(card, state->mBase + (P5381_RRMP_AIS_ENB << 2), val);

    val = pmreadloc(card, state->mBase, P5381_RRMP_AUX2_AIS_ENB);
    val |= (BIT0|BIT1|BIT2);
    card_write_iom(card, state->mBase + (P5381_RRMP_AUX2_AIS_ENB << 2), val);

    val = pmreadloc(card, state->mBase, P5381_RRMP_AUX3_AIS_ENB);
    val |= (BIT0|BIT1|BIT2);
    card_write_iom(card, state->mBase + (P5381_RRMP_AUX3_AIS_ENB << 2), val);

    val = pmreadloc(card, state->mBase, P5381_RRMP_AUX4_AIS_ENB);
    val |= (BIT0|BIT1|BIT2);
    card_write_iom(card, state->mBase + (P5381_RRMP_AUX4_AIS_ENB << 2), val);

    card_write_iom(card, state->mBase + (P5381_SARC_PATH_REG_ENB << 2), 1);

    val = pmreadloc(card, state->mBase, P5381_SARC_PATH_CONFIG);
    val |= (BIT0|BIT7);
    card_write_iom(card, state->mBase + (P5381_SARC_PATH_CONFIG << 2), val);

    val = pmreadloc(card, state->mBase, P5381_SARC_PATH_RPAISINS_ENB);
    val |= (BIT0|BIT2|BIT3);
    card_write_iom(card, state->mBase + (P5381_SARC_PATH_RPAISINS_ENB << 2), val);

    val = pmreadloc(card, state->mBase, P5381_SIRP_CONFIG);
    val |= (BIT0|BIT2|BIT3|BIT5);
    val &= ~(BIT1|BIT4);
    card_write_iom(card, state->mBase + (P5381_SIRP_CONFIG << 2), val);

    val = 0xfc34;
    card_write_iom(card, state->mBase + (P5381_R8TD_APS1_ANALOG_CTRL1 << 2), val);
    card_write_iom(card, state->mBase + (P5381_R8TD_APS2_ANALOG_CTRL1 << 2), val);
    card_write_iom(card, state->mBase + (P5381_R8TD_APS3_ANALOG_CTRL1 << 2), val);
    card_write_iom(card, state->mBase + (P5381_R8TD_APS4_ANALOG_CTRL1 << 2), val);

    val = pmreadloc(card, state->mBase, P5381_T8TD_APS1_ANALOG_CTRL);
    val |= (BIT7|BIT8);
    val &= ~BIT0;
    card_write_iom(card, state->mBase + (P5381_T8TD_APS1_ANALOG_CTRL << 2), val);

    val = pmreadloc(card, state->mBase,P5381_T8TD_APS2_ANALOG_CTRL);
    val |= (BIT7|BIT8);
    val &= ~BIT0;
    card_write_iom(card, state->mBase + (P5381_T8TD_APS2_ANALOG_CTRL << 2), val);

    val = pmreadloc(card, state->mBase,P5381_T8TD_APS3_ANALOG_CTRL);
    val |= (BIT7|BIT8);
    val &= ~BIT0;
    card_write_iom(card, state->mBase + (P5381_T8TD_APS3_ANALOG_CTRL << 2), val);

    val = pmreadloc(card, state->mBase, P5381_T8TD_APS4_ANALOG_CTRL);
    val |= (BIT7|BIT8);
    val &= ~BIT0;
    card_write_iom(card, state->mBase + (P5381_T8TD_APS4_ANALOG_CTRL << 2), val);

    val = pmreadloc(card, state->mBase, P5381_CSTR_CTRL);
    val |= (BIT4);
    val &= ~BIT3;
    card_write_iom(card, state->mBase + (P5381_CSTR_CTRL << 2), val);

    val = pmreadloc(card, state->mBase, P5381_TX2488_ABC_CTRL);
    val |= (BIT7|BIT6|BIT5|BIT3|BIT2);
    val &= ~BIT0;
    card_write_iom(card, state->mBase + (P5381_TX2488_ABC_CTRL << 2), val);

    val = pmreadloc(card, state->mBase,P5381_TRMP_CONFIG);
    val |= BIT2;
    card_write_iom(card, state->mBase + (P5381_TRMP_CONFIG << 2), val);

    val = pmreadloc(card, state->mBase, P5381_TRMP_AUX2_CONFIG);
    val |= BIT2;
    card_write_iom(card, state->mBase + (P5381_TRMP_AUX2_CONFIG << 2), val);

    val = pmreadloc(card, state->mBase, P5381_TRMP_AUX3_CONFIG);
    val |= BIT2;
    card_write_iom(card, state->mBase + (P5381_TRMP_AUX3_CONFIG << 2), val);

    val = pmreadloc(card, state->mBase, P5381_TRMP_AUX4_CONFIG);
    val |= BIT2;
    card_write_iom(card, state->mBase + (P5381_TRMP_AUX4_CONFIG << 2), val);

    val = pmreadloc(card, state->mBase, P5381_STLI_CLK_CONFIG);
    val |= BIT0;
    card_write_iom(card, state->mBase + (P5381_STLI_CLK_CONFIG << 2), val);

    /* Set Default to PoS */
    mode = kNetworkModePoS;
    any_attribute = component_get_attribute(component, kUint32AttributeNetworkMode);
    attribute_set_value(any_attribute, (void *)&mode, sizeof(uint32_t));
    
}

static int
dag43s_port_post_initialize(ComponentPtr component)
{
    port_state_t* state = NULL;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uintptr_t address = 0;
    uintptr_t gpp_base = 0;
    uint32_t mask = 0;
    uint32_t bool_attr_mask = 0;
    AttributePtr attr = NULL;
    dag43s_state_t * card_state = NULL;

    state = component_get_private_state(component);
    card = component_get_card(component);
    card_state = (dag43s_state_t *)card_get_private_state(card);
    state->mBase = card_get_register_address(card, DAG_REG_PM5381, state->mIndex);
    
    address = (uintptr_t)card_get_iom_address(card) + SONET_cs;
    /* Laser */
    bool_attr_mask = BIT0;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLaser, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    //address = (uintptr_t)card_get_iom_address(card) + state->mBase + (P5381_RX2488_ANALOG_CTRL << 2);
    /* EQL */
    bool_attr_mask = BIT7;
    grw = grw_init(card, P5381_RX2488_ANALOG_CTRL, grw_dag43s_read, grw_dag43s_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEquipmentLoopback, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);

    /* Link Discard */
    attr = get_new_link_discard();
    component_add_attribute(component, attr);


    /* Link Discard */
    //bool_attr_mask = BIT5;
    //grw = grw_init(card, P5381_RCFP_CONFIG, grw_dag43s_read, grw_dag43s_write);
    //attr = attribute_factory_make_attribute(kBooleanAttributeLinkDiscard, grw, &bool_attr_mask, 1);
    //component_add_attribute(component, attr);

    /* Facility Loopback */
    attr = get_new_fcl();
    component_add_attribute(component, attr);

    /* Payload Scramble */
    attr = get_new_pscramble();
    component_add_attribute(component, attr);

    /* Master or Slave */
    attr = get_new_master_slave();
    component_add_attribute(component, attr);

    /* Scramble */
    attr = get_new_scramble();
    component_add_attribute(component,attr);

    /* Link Reset */
    attr = get_new_link_reset();
    component_add_attribute(component, attr);

    /* Network Mode */
    attr = get_new_network_mode();
    component_add_attribute(component, attr);

    /* Loss of Signal */
    bool_attr_mask = BIT2;
    grw = grw_init(card, P5381_RRMP_STATUS, grw_dag43s_read, grw_dag43s_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfSignal, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    /* Out of Frame */
    attr = get_new_oof();
    component_add_attribute(component, attr);

    /* Loss of Frame */
    bool_attr_mask = BIT1;
    grw = grw_init(card, P5381_RRMP_STATUS, grw_dag43s_read, grw_dag43s_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFrame, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    /* Line Alarm Indication Signal */
    attr = get_new_lais();
    component_add_attribute(component, attr);

    /* Line Remote Defect Indication Signal */
    attr = get_new_lrdis();
    component_add_attribute(component, attr);

    /* Data Out of Lock */
    attr = get_new_dool();
    component_add_attribute(component, attr);

    /* Reference Out of Lock */
    attr = get_new_rool();
    component_add_attribute(component, attr);

    /* Loss of Pointer */
    bool_attr_mask = BIT0;
    grw = grw_init(card, P5381_SARC_LOP_PTR_STAT, grw_dag43s_read, grw_dag43s_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfPointer, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    /* Path BIP Error */
    attr = get_new_path_bip_error();
    component_add_attribute(component, attr);
    
    /* Path REI Error */
    attr = get_new_path_rei_error();
    component_add_attribute(component, attr);

    /* Latch Counter */
    attr = get_new_latch_and_clear();
    component_add_attribute(component, attr);

    /* CRC Select */
    attr = get_new_crc();
    component_add_attribute(component, attr);

    /* CRC Strip */
    bool_attr_mask = BIT8;
    grw = grw_init(card, P5381_RCFP_CONFIG, grw_dag43s_read, grw_dag43s_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCrcStrip, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    /* Min Pkt Len */
    attr = get_new_min_pkt_len();
    component_add_attribute(component, attr);

    /* Max Pkt Len */
    attr = get_new_max_pkt_len();
    component_add_attribute(component, attr);

    /* Loss of Cell Delination */
    bool_attr_mask = BIT8;
    grw = grw_init(card, P5381_RCFP_INT_ENB_STAT, grw_dag43s_read, grw_dag43s_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfCellDelineation, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    if (card_state->mSteerBase != 0 )
    {
        bool_attr_mask = 0xFF;
        grw = grw_init(card, card_state->mSteerBase, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kUint32AttributeSteer, grw, &bool_attr_mask, 1);
        component_add_attribute(component, attr);
    }
    /* Path Label */
    attr = get_new_path_label();
    component_add_attribute(component,attr);
    
    /* RX Frames*/
    attr = get_new_rx_frames();
    component_add_attribute(component, attr);

    /* Rx Bytes*/
    attr = get_new_rx_bytes();
    component_add_attribute(component, attr);
    
    /* HEC Fail Count */
    attr = get_new_fcs_hec_fail_count();
    component_add_attribute(component, attr);
    
    /* TX Frames*/
    attr = get_new_tx_frames();
    component_add_attribute(component, attr);
    
    /* TX Bytes */
    attr = get_new_tx_bytes();
    component_add_attribute(component, attr);
    
    /* txfdrop */
    attr = get_new_txfdrop();
    component_add_attribute(component, attr);

    /* rxfdrop */
    attr = get_new_rxfdrop();
    component_add_attribute(component, attr);

    /* aborts */
    attr = get_new_aborts();
    component_add_attribute(component, attr);

    /* short */
    attr = get_new_short();
    component_add_attribute(component, attr);

    /* long */
    attr = get_new_long();
    component_add_attribute(component, attr);
	
     /* active */
    attr = get_new_active();
    component_add_attribute(component, attr);

    /* add the active and drop count components */
    gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
    if (gpp_base > 0)
    {
        mask = 0xffffffff;
        address = ((uintptr_t)card_get_iom_address(card)) + gpp_base + (uintptr_t)((state->mIndex + 1)*SP_OFFSET) + (uintptr_t)SP_DROP;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kUint32AttributeDropCount, grw, &mask, 1);
        component_add_attribute(component, attr);
    }

    /* dummy linerate attribute-always oc48 */
    attr = get_new_linerate_attribute();
    component_add_attribute(component, attr);

    return 1;
}

static AttributePtr
get_new_active(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeActive);

    if (NULL != result)
    {
	attribute_set_dispose_routine(result, active_dispose);
        attribute_set_getvalue_routine(result, dag43s_port_active_get_value);
        attribute_set_setvalue_routine(result, dag43s_port_active_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "active");
        attribute_set_description(result, "Enable or disable this port");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void
active_dispose(AttributePtr attribute)
{
}

static void*
dag43s_port_active_get_value(AttributePtr attribute)
{
   AttributePtr active_attr = NULL; 
   ComponentPtr comp = NULL;
   ComponentPtr gpp_comp = NULL;
   port_state_t *state = NULL;
   DagCardPtr card =  NULL;
   ComponentPtr root = NULL;

   int gpp_count = 0;  
   int attr_count = 0;
   int index = 0, current_index = 0;
   int i, j;

   if (1 == valid_attribute(attribute))
   {
   	comp = attribute_get_component(attribute);
   	card = component_get_card(comp);

   	root = card_get_root_component(card);
   	state = component_get_private_state(comp); 
   	index = state->mIndex;

   	gpp_count = component_get_subcomponent_count_of_type(root, kComponentGpp);
   	for (i = 0; i < gpp_count ; i ++)
   	{
		gpp_comp = component_get_indexed_subcomponent_of_type(root, kComponentGpp, i);
		attr_count = component_get_attribute_count_of_type(gpp_comp, kBooleanAttributeActive);
		for (j = 0; j < attr_count; j ++)
		{
			if( current_index == index)
                        {
				active_attr = component_indexed_get_attribute_of_type(gpp_comp, kBooleanAttributeActive, j);
				attribute_set_value_array(attribute, (void *)attribute_get_value(active_attr), sizeof (uint8_t));
				return (void*) attribute_get_value_array(attribute);

			}
		 	current_index++;
		}

   	}
    } 
   
   return NULL;
}


static void
dag43s_port_active_set_value(AttributePtr attribute, void* value, int length)
{
   AttributePtr active_attr = NULL; 
   ComponentPtr comp = NULL;
   ComponentPtr gpp_comp = NULL;
   port_state_t *state = NULL;
   DagCardPtr card =  NULL;
   ComponentPtr root = NULL;
 
   int gpp_count = 0;  
   int attr_count = 0;
   int index = 0, current_index = 0;
   int i, j;

   if (1 == valid_attribute(attribute))
   {
  
       	comp = attribute_get_component(attribute);
        card = component_get_card(comp);
        root = card_get_root_component(card);
   	
	state = component_get_private_state(comp); 
   	index = state->mIndex;

   	gpp_count = component_get_subcomponent_count_of_type(root, kComponentGpp);
   	for (i = 0; (i < gpp_count) ; i ++)
   	{
		gpp_comp = component_get_indexed_subcomponent_of_type(root, kComponentGpp, i);
		attr_count = component_get_attribute_count_of_type(gpp_comp, kBooleanAttributeActive);
		for (j = 0; j < attr_count; j ++)
		{
			if( current_index == index)
                        {
				active_attr = component_indexed_get_attribute_of_type(gpp_comp, kBooleanAttributeActive, j);
				attribute_set_value(active_attr, value, sizeof (uint8_t));
                                return;
			}
                        current_index++;
		}
   	}
    } 
}

static AttributePtr
get_new_fcl(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFacilityLoopback);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, fcl_dispose);
        attribute_set_post_initialize_routine(result, fcl_post_initialize);
        attribute_set_getvalue_routine(result, fcl_get_value);
        attribute_set_setvalue_routine(result, fcl_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "fcl");
        attribute_set_description(result, "facility loopback");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}
static void
fcl_dispose(AttributePtr attribute)
{
}

static void*
fcl_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase, P5381_RX2488_ANALOG_CLK);
        if (register_value & BIT9)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static void
fcl_post_initialize(AttributePtr attribute)
{
}

static void
fcl_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == 0) {
            register_value = pmreadloc(card, state->mBase, P5381_RX2488_ANALOG_CLK);
            register_value &= ~BIT9;
            card_write_iom(card, state->mBase + (P5381_RX2488_ANALOG_CLK << 2), register_value);
        
            register_value = pmreadloc(card, state->mBase, P5381_TX2488_ANALOG_CTRL);
            register_value &= ~BIT3;
            card_write_iom(card, state->mBase + (P5381_TX2488_ANALOG_CTRL << 2), register_value);

            register_value = pmreadloc(card, state->mBase, P5381_TX2488_ABC_CTRL);
            register_value |= (BIT7|BIT6|BIT5|BIT3);
            register_value &= ~BIT0;
            card_write_iom(card, state->mBase + (P5381_TX2488_ABC_CTRL << 2), register_value);

         } else {
            register_value = pmreadloc(card, state->mBase, P5381_RX2488_ANALOG_CLK);
            register_value |= BIT9;
            card_write_iom(card, state->mBase + (P5381_RX2488_ANALOG_CLK << 2), register_value);

            register_value = pmreadloc(card, state->mBase, P5381_TX2488_ANALOG_CTRL);
            register_value |= BIT3;
            card_write_iom(card, state->mBase + (P5381_TX2488_ANALOG_CTRL << 2), register_value);

            register_value = pmreadloc(card, state->mBase, P5381_TX2488_ABC_CTRL);
            register_value &= ~(BIT7|BIT6|BIT5|BIT3);
            register_value |= BIT0;
            card_write_iom(card, state->mBase + (P5381_TX2488_ABC_CTRL << 2), register_value);
         }
    }
}
static AttributePtr
get_new_pscramble(void)
{
    AttributePtr result = attribute_init(kBooleanAttributePayloadScramble);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pscramble_dispose);
        attribute_set_post_initialize_routine(result, pscramble_post_initialize);
        attribute_set_getvalue_routine(result, pscramble_get_value);
        attribute_set_setvalue_routine(result, pscramble_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "pscramble");
        attribute_set_description(result, "payload scramble");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
   }

   return result;
}
static void
pscramble_dispose(AttributePtr attribute)
{
}

static void*
pscramble_get_value(AttributePtr attribute)
{
   ComponentPtr comp = attribute_get_component(attribute);
   DagCardPtr card = component_get_card(comp);
   port_state_t * state = component_get_private_state(comp);
   uint32_t register_value;
   uint8_t value;

   if (1 == valid_attribute(attribute))
   {
      register_value = pmreadloc(card, state->mBase, P5381_RCFP_CONFIG);
      if (register_value & BIT1)
      {
          value = 1;
      }
      else
      {
          value = 0;
      }
      attribute_set_value_array(attribute, &value, sizeof(value));
      return (void *)attribute_get_value_array(attribute);
   }
   return NULL;
}

static void
pscramble_post_initialize(AttributePtr attribute)
{
}
static void
pscramble_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == 0) 
        {
            register_value = pmreadloc(card, state->mBase,P5381_RCFP_CONFIG);
            register_value &= ~BIT1;
            card_write_iom(card, state->mBase + (P5381_RCFP_CONFIG << 2), register_value);

            register_value = pmreadloc(card, state->mBase, P5381_TCFP_CONFIG);
            register_value &= ~BIT1;
            card_write_iom(card, state->mBase + (P5381_TCFP_CONFIG << 2), register_value);
        } else {
            register_value = pmreadloc(card, state->mBase, P5381_RCFP_CONFIG);
            register_value |= BIT1;
            card_write_iom(card, state->mBase + (P5381_RCFP_CONFIG << 2), register_value);

            register_value = pmreadloc(card, state->mBase,P5381_TCFP_CONFIG);
            register_value |= BIT1;
            card_write_iom(card, state->mBase + (P5381_TCFP_CONFIG << 2), register_value);
        }
    }
}
static AttributePtr
get_new_master_slave(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMasterSlave);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, master_slave_dispose);
        attribute_set_post_initialize_routine(result, master_slave_post_initialize);
        attribute_set_getvalue_routine(result, master_slave_get_value);
        attribute_set_setvalue_routine(result, master_slave_set_value);
        attribute_set_to_string_routine(result, master_slave_to_string_routine);
        attribute_set_from_string_routine(result, master_slave_from_string_routine);
        attribute_set_name(result, "master_or_slave");
        attribute_set_description(result, "Set master or slave mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);

    }
    return result;
}

static void
master_slave_dispose(AttributePtr attribute)
{
}

static void
master_slave_post_initialize(AttributePtr attribute)
{
}
static void*
master_slave_get_value(AttributePtr attribute)
{
    uint32_t register_value = 0;
    master_slave_t  value = 0;
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);

    if (1 == valid_attribute(attribute))
    {
       register_value = pmreadloc(card, state->mBase,P5381_TX2488_ABC_CTRL);
       if (register_value & BIT7)
       {
           value = kMaster;
       }
       else
       {
           value = kSlave;
       }
       attribute_set_value_array(attribute, &value, sizeof(value));
       return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
master_slave_set_value(AttributePtr attribute, void* value, int len)
{
    uint32_t register_value = 0;
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase,P5381_TX2488_ABC_CTRL);
        if (*(master_slave_t*)value == kMaster)
        {
            register_value |= BIT7;
        }
        else
        {
            register_value &= ~BIT7;
        }
        card_write_iom(card, state->mBase + (P5381_TX2488_ABC_CTRL << 2), register_value);
    }
}
void
master_slave_to_string_routine(AttributePtr attribute)
{
    master_slave_t value = *(master_slave_t*)master_slave_get_value(attribute);
    const char* temp = master_slave_to_string(value);
    if (temp)
        attribute_set_to_string(attribute, temp);
}

void
master_slave_from_string_routine(AttributePtr attribute, const char* string)
{
    master_slave_t value = string_to_master_slave(string);
    if (kLineRateInvalid != value)
         master_slave_set_value(attribute, (void*)&value, sizeof(master_slave_t));
}

/* Scramble */
static AttributePtr 
get_new_scramble(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeScramble);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, scramble_dispose);
        attribute_set_post_initialize_routine(result, scramble_post_initialize);
        attribute_set_getvalue_routine(result, scramble_get_value);
        attribute_set_setvalue_routine(result, scramble_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "scramble");
        attribute_set_description(result, "scramble");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;

}
static void 
scramble_dispose(AttributePtr attribute)
{
}
static void 
scramble_post_initialize(AttributePtr attribute)
{
}
static void* 
scramble_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase,P5381_SU2488_DIAGNOSTICS);
        /* If it's set then it's noscramble */
        if (register_value & BIT0)
        {
            value = 0;
        }
        else
        {
            value = 1;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}
static void 
scramble_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;
        
    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase,P5381_SU2488_DIAGNOSTICS);
        if (*(uint8_t*)value == 0)
        {
            register_value |= (BIT0|BIT1);
        }
        else
        {
            register_value &= ~(BIT0|BIT1);
        }
        card_write_iom(card, state->mBase + (P5381_SU2488_DIAGNOSTICS << 2), register_value);
    }
}

/* Link Discard*/
static AttributePtr
get_new_link_discard(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLinkDiscard);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, link_discard_dispose);
        attribute_set_post_initialize_routine(result, link_discard_post_initialize);
        attribute_set_getvalue_routine(result, link_discard_get_value);
        attribute_set_setvalue_routine(result, link_discard_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "link_discard");
        attribute_set_description(result, "link discard");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;
}

static void
link_discard_dispose(AttributePtr attribute)
{
}
static void
link_discard_post_initialize(AttributePtr attribute)
{
}


static void*
link_discard_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase, P5381_RCFP_CONFIG);
        /* If it's set it's nodiscard */
        if (register_value & BIT5)
        {
            value = 0;
        }
        else
        {
            value = 1;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}
static void
link_discard_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase,P5381_RCFP_CONFIG);
        if (*(uint8_t*)value == 0)
        {
            register_value |= BIT5;
        }
        else
        {
            register_value &= ~BIT5;
        }
        card_write_iom(card, state->mBase + (P5381_RCFP_CONFIG << 2), register_value);
    }
}


/* Link Reset */
static AttributePtr
get_new_link_reset(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReset);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, link_reset_dispose);
        attribute_set_post_initialize_routine(result, link_reset_post_initialize);
        attribute_set_getvalue_routine(result, link_reset_get_value);
        attribute_set_setvalue_routine(result, link_reset_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "reset");
        attribute_set_description(result, "link reset");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;

}
static void
link_reset_dispose(AttributePtr attribute)
{
}
static void
link_reset_post_initialize(AttributePtr attribute)
{
}

static void*
link_reset_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase,P5381_SU2488_MASTER_RESET);
        if (register_value & BIT15)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}
static void
link_reset_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == 0)
        {
            do {
                register_value = pmreadloc(card, state->mBase, P5381_SU2488_MASTER_RESET);
                register_value |= (BIT13|BIT14|BIT15);
                card_write_iom(card, state->mBase + (P5381_SU2488_MASTER_RESET << 2), register_value);

                register_value = pmreadloc(card, state->mBase, P5381_SU2488_MASTER_RESET);
                register_value &= ~(BIT13|BIT14);

                pmreadloc(card,state->mBase, P5381_SU2488_IDENTITY_GPMU);
                usleep(1000);
            } while ((pmreadloc(card, state->mBase, P5381_SU2488_IDENTITY_GPMU) & BIT15));
        }
        else
        {
            register_value = pmreadloc(card, state->mBase,P5381_SU2488_MASTER_RESET);
            register_value |= (BIT13|BIT14|BIT15);
            card_write_iom(card, state->mBase + (P5381_SU2488_MASTER_RESET << 2), register_value);
        }
   }
}
static AttributePtr
get_new_oof(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeOutOfFrame);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, oof_dispose);
        attribute_set_post_initialize_routine(result, oof_post_initialize);
        attribute_set_getvalue_routine(result, oof_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "oof");
        attribute_set_description(result, "Out of frame.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;
}
static void
oof_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
oof_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
oof_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RRMP_STATUS);
        val = ((register_val & BIT0) == BIT0);
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_lais(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLineAlarmIndicationSignal);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, lais_dispose);
        attribute_set_post_initialize_routine(result, lais_post_initialize);
        attribute_set_getvalue_routine(result, lais_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "lais");
        attribute_set_description(result, "Line Alarm Indication Signal.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;
}
static void
lais_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
lais_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
lais_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RRMP_STATUS);
        val = ((register_val & BIT3) == BIT3);
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_lrdis(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLineRemoteDefectIndicationSignal);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, lrdis_dispose);
        attribute_set_post_initialize_routine(result, lrdis_post_initialize);
        attribute_set_getvalue_routine(result, lrdis_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "lrdis");
        attribute_set_description(result, "Line Remote Defect Indication Signal.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;
}
static void
lrdis_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
lrdis_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
lrdis_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RRMP_STATUS);
        val = ((register_val & BIT4) == BIT4);
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_dool(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeDataOutOfLock);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dool_dispose);
        attribute_set_post_initialize_routine(result, dool_post_initialize);
        attribute_set_getvalue_routine(result, dool_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "dool");
        attribute_set_description(result, "Data Out Of Lock.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;
}
static void
dool_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
dool_post_initialize(AttributePtr attribute)
{
   if (1 == valid_attribute(attribute))
   {
       /* FIXME: unimplemented method. */
   }
}

static void*
dool_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RX2488_ANALOG_INT_STAT);
        val = ((register_val & BIT1) == BIT1);
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static AttributePtr
get_new_rool(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReferenceOutOfLock);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, rool_dispose);
        attribute_set_post_initialize_routine(result, rool_post_initialize);
        attribute_set_getvalue_routine(result, rool_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "rool");
        attribute_set_description(result, "Reference Out of Lock.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;
}

static void
rool_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */

    }
}
static void
rool_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
rool_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RX2488_ANALOG_INT_STAT);
        val = ((register_val & BIT0) == BIT0);
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static AttributePtr
get_new_path_bip_error(void)
{
    AttributePtr result = attribute_init(kUint32AttributePathBIPError);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, path_bip_error_dispose);
        attribute_set_post_initialize_routine(result, path_bip_error_post_initialize);
        attribute_set_getvalue_routine(result, path_bip_error_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "path_bip_error");
        attribute_set_description(result, "Path BIP Error.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}

static void
path_bip_error_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
path_bip_error_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
path_bip_error_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port); 
        register_val = pmreadind(card, state->mBase, P5381_RHPP_IND_ADDR, (6<<6)|1, P5381_RHPP_IND_DATA);
        val = register_val & 0xffff;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_path_rei_error(void)
{
    AttributePtr result = attribute_init(kUint32AttributePathREIError);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, path_rei_error_dispose);
        attribute_set_post_initialize_routine(result, path_rei_error_post_initialize);
        attribute_set_getvalue_routine(result,path_rei_error_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "path_rei_error");
        attribute_set_description(result, "Path REI Error.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}

static void
path_rei_error_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
path_rei_error_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
path_rei_error_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadind(card, state->mBase, P5381_RHPP_IND_ADDR, (7<<6)|1, P5381_RHPP_IND_DATA);
        val = register_val & 0xffff;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);;
    }
    return NULL;
}

static AttributePtr
get_new_path_label(void)
{
    AttributePtr result = attribute_init(kUint32AttributeC2PathLabel);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, path_label_dispose);
        attribute_set_post_initialize_routine(result, path_label_post_initialize);
        attribute_set_getvalue_routine(result,path_label_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "c2_path_label");
        attribute_set_description(result, "Path signal label. Reads teh SONET/SDH C2 path label byte");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
            return result;
}

static void
path_label_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
path_label_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
path_label_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadind(card, state->mBase, P5381_RHPP_IND_ADDR, (3<<6)|1, P5381_RHPP_IND_DATA);
        val = register_val & 0xffff;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_latch_and_clear(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeCounterLatch);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, latch_and_clear_dispose);
        attribute_set_post_initialize_routine(result, latch_and_clear_post_initialize);
	attribute_set_getvalue_routine(result,latch_and_clear_get_value);
        attribute_set_setvalue_routine(result,latch_and_clear_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "latch_and_clear");
        attribute_set_description(result, "Latch Counter");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    return result;
}

static void
latch_and_clear_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
latch_and_clear_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void
latch_and_clear_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t reg_val = *(uint32_t*)value;


    if (1 == valid_attribute(attribute))
    {
        card_write_iom(card, state->mBase + (P5381_SU2488_IDENTITY_GPMU << 2), reg_val);
    }
}

static void*
latch_and_clear_get_value(AttributePtr attribute)
{
   uint8_t value = 0;
    if (1 == valid_attribute(attribute))
    {
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
   return NULL;
}

static AttributePtr
get_new_crc(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCrcSelect);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, crc_dispose);
        attribute_set_post_initialize_routine(result, crc_post_initialize);
        attribute_set_getvalue_routine(result, crc_get_value);
        attribute_set_setvalue_routine(result, crc_set_value);
        attribute_set_to_string_routine(result, crc_to_string_routine);
        attribute_set_from_string_routine(result, crc_from_string_routine);
        attribute_set_name(result, "crc_select");
        attribute_set_description(result, "Select the crc.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void
crc_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void
crc_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void
crc_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_value;
    port_state_t* state = NULL;
    crc_t crc_value = *(crc_t*)value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        register_value = pmreadloc(card, state->mBase, P5381_RCFP_CONFIG);
        if (kCrc16 == crc_value)
        {
            register_value |= BIT4;
            register_value &= ~BIT3;
            card_write_iom(card, state->mBase + (P5381_RCFP_CONFIG << 2), register_value);
        }
        else if (kCrc32 == crc_value)
        {
            register_value |= (BIT4|BIT3);
            card_write_iom(card, state->mBase + (P5381_RCFP_CONFIG << 2), register_value);
        }

        register_value = pmreadloc(card, state->mBase, P5381_TCFP_CONFIG);
        if (kCrc16 == crc_value)
        {
            register_value |= BIT7;
            register_value &= ~BIT6;
            card_write_iom(card, state->mBase + (P5381_TCFP_CONFIG << 2), register_value);
        }
        else if (kCrc32 == crc_value)
        {
            register_value |= (BIT7|BIT6);
            card_write_iom(card, state->mBase + (P5381_TCFP_CONFIG << 2), register_value);
        }
    }
}
static void*
crc_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_value;
    port_state_t* state = NULL;
    crc_t value = 0;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        register_value = pmreadloc(card, state->mBase, P5381_RCFP_CONFIG);
        if (register_value & BIT3)
        {
            value = kCrc32;
        }
        else
        {
            value = kCrc16;
        }
    }
    attribute_set_value_array(attribute, &value, sizeof(value));
    return (void *)attribute_get_value_array(attribute);
}
static void
crc_to_string_routine(AttributePtr attribute)
{
   void* temp = attribute_get_value(attribute);
   const char* string = NULL;
   crc_t crc;
   if (temp)
   {
       crc = *(crc_t*)temp;
       string = crc_to_string(crc);
       if (string)
           attribute_set_to_string(attribute, string);
  }
}

static void
crc_from_string_routine(AttributePtr attribute, const char* string)
{
  if (1 == valid_attribute(attribute))
  {
      if (string)
      {
          crc_t mode = string_to_crc(string);
          crc_set_value(attribute, (void*)&mode, sizeof(mode));
      }
  }
}

/* Min Pkt Len*/
static AttributePtr
get_new_min_pkt_len(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMinPktLen);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, min_pkt_len_dispose);
        attribute_set_post_initialize_routine(result, min_pkt_len_post_initialize);
        attribute_set_getvalue_routine(result, min_pkt_len_get_value);
        attribute_set_setvalue_routine(result, min_pkt_len_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "min_pkt_len");
        attribute_set_description(result, "POS min pkt len");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;

}
static void
min_pkt_len_dispose(AttributePtr attribute)
{
}
static void
min_pkt_len_post_initialize(AttributePtr attribute)
{
}

static void*
min_pkt_len_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase, P5381_RCFP_MIN_PKT_LEN);
        value = (register_value >> 8) & 0xff;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}
static void
min_pkt_len_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t val = *(uint32_t *)value;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase, P5381_RCFP_MIN_PKT_LEN);
        register_value = (val << 8);
        card_write_iom(card, state->mBase + (P5381_RCFP_MIN_PKT_LEN << 2), register_value);
    }
}

/* Max Pkt Len*/
static AttributePtr
get_new_max_pkt_len(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMaxPktLen);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, max_pkt_len_dispose);
        attribute_set_post_initialize_routine(result, max_pkt_len_post_initialize);
        attribute_set_getvalue_routine(result, max_pkt_len_get_value);
        attribute_set_setvalue_routine(result, max_pkt_len_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "max_pkt_len");
        attribute_set_description(result, "POS Max Pkt Len");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
max_pkt_len_dispose(AttributePtr attribute)
{
}
static void
max_pkt_len_post_initialize(AttributePtr attribute)
{
}

static void*
max_pkt_len_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = pmreadloc(card, state->mBase, P5381_RCFP_MAX_PKT_LEN);
        value = (register_value << 1) & 0x1ffff;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;

}
static void
max_pkt_len_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t val = *(uint32_t *)value;

    if (1 == valid_attribute(attribute))
    {
        register_value = (val >> 1);
        card_write_iom(card, state->mBase + (P5381_RCFP_MAX_PKT_LEN << 2), register_value);
    }
}
static AttributePtr
get_new_rx_frames(void)
{
    AttributePtr result = attribute_init(kUint32AttributeRxFrames);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, rx_frames_dispose);
        attribute_set_post_initialize_routine(result, rx_frames_post_initialize);
        attribute_set_getvalue_routine(result, rx_frames_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "rx_frames");
        attribute_set_description(result, "RX frames.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
rx_frames_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
rx_frames_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
rx_frames_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc2(card, state->mBase, P5381_RCFP_RXPKT_CELL_CNT_L);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_rx_bytes(void)
{
    AttributePtr result = attribute_init(kUint64AttributeRxBytes);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, rx_bytes_dispose);
        attribute_set_post_initialize_routine(result, rx_bytes_post_initialize);
        attribute_set_getvalue_routine(result, rx_bytes_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_name(result, "rx_bytes");
        attribute_set_description(result, "RX bytes.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }
    return result;
}
static void
rx_bytes_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
rx_bytes_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
rx_bytes_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint64_t register_val;
    uint64_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc3(card, state->mBase, P5381_RCFP_RXBYTE_IDLE_L);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_fcs_hec_fail_count(void)
{
    AttributePtr result = attribute_init(kUint32AttributeHECCount);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, fcs_hec_fail_count_dispose);
        attribute_set_post_initialize_routine(result, fcs_hec_fail_count_post_initialize);
        attribute_set_getvalue_routine(result, fcs_hec_fail_count_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "fcs_hec_fail");
        attribute_set_description(result, "HEC Fail Count.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
fcs_hec_fail_count_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
fcs_hec_fail_count_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
fcs_hec_fail_count_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RCFP_RXFCS_HEC_CNT);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_tx_frames(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTxFrames);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, tx_frames_dispose);
        attribute_set_post_initialize_routine(result, tx_frames_post_initialize);
        attribute_set_getvalue_routine(result, tx_frames_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "tx_frames");
        attribute_set_description(result, "TX Frames.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
tx_frames_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
tx_frames_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
tx_frames_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc2(card, state->mBase, P5381_TCFP_TXPKT_CELL_CNT_L);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_tx_bytes(void)
{
    AttributePtr result = attribute_init(kUint64AttributeTxBytes);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, tx_bytes_dispose);
        attribute_set_post_initialize_routine(result, tx_bytes_post_initialize);
        attribute_set_getvalue_routine(result, tx_bytes_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_name(result, "tx_bytes");
        attribute_set_description(result, "TX Bytes.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }
    return result;
}
static void
tx_bytes_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
tx_bytes_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
tx_bytes_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint64_t register_val;
    uint64_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc3(card, state->mBase, P5381_TCFP_TXBYTE_L);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_txfdrop(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTxFDrop);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, txfdrop_dispose);
        attribute_set_post_initialize_routine(result, txfdrop_post_initialize);
        attribute_set_getvalue_routine(result, txfdrop_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "txfdrop");
        attribute_set_description(result, "TxFDrop.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
txfdrop_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
txfdrop_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
txfdrop_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_TXSDQ_DROP_CNT);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_rxfdrop(void)
{
    AttributePtr result = attribute_init(kUint32AttributeRxFDrop);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, rxfdrop_dispose);
        attribute_set_post_initialize_routine(result, rxfdrop_post_initialize);
        attribute_set_getvalue_routine(result, rxfdrop_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "rxfdrop");
        attribute_set_description(result, "RxFDrop.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
rxfdrop_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
rxfdrop_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
rxfdrop_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RXSDQ_DROP_CNT);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_aborts(void)
{
    AttributePtr result = attribute_init(kUint32AttributeAborts);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, aborts_dispose);
        attribute_set_post_initialize_routine(result, aborts_post_initialize);
        attribute_set_getvalue_routine(result, aborts_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "aborts");
        attribute_set_description(result, "Aborts.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
aborts_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
aborts_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
aborts_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RCFP_RXPOS_ABORT_CNT);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_short(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMinPktLenError);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, short_dispose);
        attribute_set_post_initialize_routine(result, short_post_initialize);
        attribute_set_getvalue_routine(result, short_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "short_error");
        attribute_set_description(result, "Min packet len counter. Counts packets with len smaller than min_pkt_len (short cmd option)");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
short_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void
short_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
short_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc(card, state->mBase, P5381_RCFP_RXPOS_SHORT_CNT);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_long(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMaxPktLenError);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, long_dispose);
        attribute_set_post_initialize_routine(result, long_post_initialize);
        attribute_set_getvalue_routine(result, long_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "long_error");
        attribute_set_description(result, " Receive Maximim length Packet Error counter. Packets with size greater than max_pkt_len(long cmd option)");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}
static void
long_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
    }
static void
long_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}
static void*
long_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = pmreadloc3(card, state->mBase, P5381_RCFP_RXPOS_LONG_CNT);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static AttributePtr
get_new_network_mode(void)
{
    AttributePtr result = attribute_init(kUint32AttributeNetworkMode);

    if (NULL != result)
    {
        attribute_set_dispose_routine(result, network_mode_dispose);
        attribute_set_post_initialize_routine(result, network_mode_post_initialize);
        attribute_set_getvalue_routine(result, network_mode_get_value);
        attribute_set_setvalue_routine(result, network_mode_set_value);
        attribute_set_to_string_routine(result, network_mode_to_string_routine);
        attribute_set_from_string_routine(result, network_mode_from_string_routine);
        attribute_set_name(result, "network_mode");
        attribute_set_description(result, "Select ATM or POS");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void
network_mode_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
network_mode_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void
network_mode_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    network_mode_t mode = *(network_mode_t*)value;
    port_state_t* state = NULL;
    ComponentPtr comp = NULL;
    ComponentPtr root = NULL;
    ComponentPtr gpp = NULL;
    ComponentPtr pbm = NULL;
    uint32_t pbm_version = 0;

    AttributePtr any_attribute = NULL;
    uint8_t boolean_value = 0;
    uint8_t att_val = 0;
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        comp = attribute_get_component(attribute);
        root = card_get_root_component(card);
        state = (port_state_t*)component_get_private_state(comp);
        if (mode == kNetworkModePoS)
        {
           /* hold fifos in reset */
           fifos_in_reset(card, state->mBase, 1);

           /* CFP disable */
           cfp_enable_disable(card, state->mBase, mode, 0);

           /* fifo disable */
           fifo_disable(card, state->mBase);

           /* fifo flush */
           fifo_flush(card, state->mBase);

           /* fifo config */
           fifo_config(card, state->mBase, mode);

           /* CFP enable */
           cfp_enable_disable(card, state->mBase, mode, 1);

           /* Switch GPP to POS record mode */
           register_value = card_read_iom(card, INT_ENABLES);
           register_value &= ~GPP_ATM_Mode;
           card_write_iom(card, INT_ENABLES, register_value);

           /* release fifos from reset */
           fifos_in_reset(card, state->mBase, 0);


           /* PHY Burst Size */
           register_value = 4;
           pmwriteind(card, state->mBase +(P5381_RXPHY_IND_BURST_SIZE << 2), register_value, state->mBase +(P5381_RXPHY_IND_BURST_SIZE << 2), register_value);

           /* PHY enable */
           phy_enable(card, state->mBase);

           /* SIRP enable */
           sirp_enable(card, state->mBase);

           /* RHPP INVCNT set */
           rhpp_invcnt_set(card, state->mBase);

           gpp = component_get_subcomponent(root, kComponentGpp, 0);
           assert(gpp != NULL);
           /* default slen to 48  */
           any_attribute = component_get_attribute(gpp, kUint32AttributeSnaplength);
           att_val = 48;
           attribute_set_value(any_attribute, (void *)&att_val, sizeof(uint32_t));

           /* default to variable length mode */
           any_attribute = component_get_attribute(gpp, kBooleanAttributeVarlen);
           boolean_value = 1;
           attribute_set_value(any_attribute, (void *)&boolean_value, sizeof(boolean_value));

        }
        else if (kNetworkModeATM == mode)
        {
           /* Check for Coprocessor route source select.  ATM mode cannot be selected if this is 0x01 (in bits 13-12). */
           pbm = component_get_subcomponent(root, kComponentPbm, 0);
           pbm_version = pbm_get_version(pbm);
           register_value = pbm_get_global_status_register(pbm);
           if ((0 == (register_value & BIT13)) && (0 != (register_value & BIT12)))
           {
               dagutil_error("route source select is nonzero - is there an Endace Filter image loaded?  Endace Filter only supports PoS.\n");
               return;
           }
                                                                                                                    
            /* hold fifos in reset */
           fifos_in_reset(card, state->mBase, 1);

            /* CFP disable */
           cfp_enable_disable(card, state->mBase, mode, 0);

            /* fifo disable */
           fifo_disable(card, state->mBase);

           /* fifo flush */
           fifo_flush(card, state->mBase);

            /* fifo config */
           fifo_config(card, state->mBase, mode);

            /* CFP enable */
            cfp_enable_disable(card, state->mBase, mode, 1);

            /* Switch GPP to ATM record mode */
            register_value = card_read_iom(card, INT_ENABLES);
            register_value |= GPP_ATM_Mode;
            card_write_iom(card, INT_ENABLES, register_value);

            /* release fifos from reset */
            fifos_in_reset(card, state->mBase, 0);

            /* PHY Burst Size */
            register_value = 3;
            pmwriteind(card, state->mBase + (P5381_RXPHY_IND_BURST_SIZE << 2), register_value, state->mBase + (P5381_RXPHY_IND_BURST_SIZE << 2), register_value);

            /* PHY enable */
            phy_enable(card, state->mBase);

            /* SIRP enable */
            sirp_enable(card, state->mBase);

            /* RHPP INVCNT set */
            rhpp_invcnt_set(card, state->mBase);

            gpp = component_get_subcomponent(root, kComponentGpp, 0);
            assert(gpp != NULL);
            /* default slen to 52 (not 64bit aligned) */
            any_attribute = component_get_attribute(gpp, kUint32AttributeSnaplength);
            att_val = 52;
            attribute_set_value(any_attribute, (void *)&att_val, sizeof(uint32_t));

            /* default to fixed length mode */
            any_attribute = component_get_attribute(gpp, kBooleanAttributeVarlen);
            boolean_value = 0;
            attribute_set_value(any_attribute, (void *)&boolean_value, sizeof(boolean_value));

        }
    
     }
}
static void*
network_mode_get_value(AttributePtr attribute)
{
   DagCardPtr card = NULL;
   network_mode_t mode;
   port_state_t* state = NULL;
   ComponentPtr comp = NULL;
   uint32_t register_value = 0;
   card = attribute_get_card(attribute);
   comp = attribute_get_component(attribute);
   state = (port_state_t*)component_get_private_state(comp);
   register_value  = pmreadloc(card, state->mBase, P5381_RCFP_CONFIG);
   if (register_value & BIT10) {
       mode = kNetworkModePoS;
   }
   else {
       mode = kNetworkModeATM;
   }
   attribute_set_value_array(attribute, &mode, sizeof(mode));
   return (void *)attribute_get_value_array(attribute);
}
static void
network_mode_to_string_routine(AttributePtr attribute)
{
    network_mode_t nm = *(network_mode_t*)attribute_get_value(attribute);
    if (nm != kLineRateInvalid && 1 == valid_attribute(attribute))
    {
        const char* temp = NULL;
        temp = network_mode_to_string(nm);
        attribute_set_to_string(attribute, temp);
    }
}

static void
network_mode_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute) && string != NULL)
    {
        network_mode_t nm = string_to_line_rate(string);
        network_mode_set_value(attribute, (void*)&nm, sizeof(nm));
    }
}
/* Network Mode Functions */
void 
fifos_in_reset(DagCardPtr card, uint32_t base, uint8_t hold_release)
{
    uint32_t register_value = 0;
    register_value = pmreadloc(card, base, P5381_RXSDQ_RESET);
    if (hold_release)
    {
        /* hold fifos in reset */
        register_value |= BIT0;
    }
    else
    {
        /* release fifos from reset */
        register_value &= ~BIT0;
    }
    card_write_iom(card, base + (P5381_RXSDQ_RESET << 2), register_value);

    register_value = pmreadloc(card, base,P5381_TXSDQ_RESET);
    if (hold_release)
    {
        /* hold fifos in reset */
        register_value |= BIT0;
    }
    else
    {
        /* release fifos from reset */
        register_value &= ~BIT0;
    }
    card_write_iom(card, base + (P5381_TXSDQ_RESET << 2), register_value);
}
void 
cfp_enable_disable(DagCardPtr card, uint32_t base, network_mode_t mode, uint8_t enable)
{
    uint32_t register_value = 0;
    register_value = pmreadloc(card, base, P5381_RCFP_CONFIG);
    if (enable)
    {
        if (mode == kNetworkModePoS)
        {
            register_value |= (BIT0|BIT10);
        }
        else
        {
            register_value |= (BIT0|BIT8);
            register_value &= ~BIT10;
        }
    }
    else 
    {
        register_value &= ~BIT0;
    }
    card_write_iom(card, base + (P5381_RCFP_CONFIG << 2), register_value);

    if (mode == kNetworkModePoS)
    {
        register_value = pmreadloc(card, base, P5381_RCFP_MIN_PKT_LEN);
        register_value &= 0xff;
        register_value |= (16 << 8);
        card_write_iom(card, base +(P5381_RCFP_MIN_PKT_LEN << 2), register_value);
    }

    register_value = pmreadloc(card, base, P5381_TCFP_CONFIG);
    if (enable)
    {
        if (mode == kNetworkModePoS)
        {
            register_value |= (BIT0|BIT8);
        }
        else
        {
            register_value |= BIT0;
            register_value &= ~BIT8;
        }
    }
    else
    {
        register_value &= ~BIT0;
    }
    card_write_iom(card, base + (P5381_TCFP_CONFIG << 2), register_value);
}
void
fifo_disable(DagCardPtr card, uint32_t base)
{
    uint32_t register_value = 0;
    /* fifo disable */
    register_value = pmreadind(card, base, P5381_RXSDQ_IND_ADDR, 0, P5381_RXSDQ_IND_CONFIG);
    register_value &= ~BIT15;
    pmwriteind(card, base + (P5381_RXSDQ_IND_ADDR << 2),0 , base + (P5381_RXSDQ_IND_CONFIG << 2), register_value);

    register_value = pmreadind(card,base, P5381_TXSDQ_IND_ADDR, 0 , P5381_TXSDQ_IND_CONFIG);
    register_value &= ~BIT15;
    pmwriteind(card, base +(P5381_TXSDQ_IND_ADDR<< 2), 0 , base + (P5381_TXSDQ_IND_CONFIG << 2), register_value);
}
void 
fifo_flush(DagCardPtr card, uint32_t base)
{
    uint32_t register_value = 0;
    /* fifo flush */
    register_value = pmreadloc(card, base, P5381_RXSDQ_IND_ADDR);
    register_value |= BIT13;
    card_write_iom(card, base + (P5381_RXSDQ_IND_ADDR << 2), register_value);

    do {
        register_value |= BIT14;
        card_write_iom(card, base + (P5381_RXSDQ_IND_ADDR << 2), register_value);
        register_value = pmreadloc(card, base, P5381_RXSDQ_IND_ADDR);
    } while (!(register_value & BIT12));
    register_value &= ~BIT13;
    register_value |= BIT14;
    card_write_iom(card,base+ (P5381_RXSDQ_IND_ADDR << 2), register_value);

    register_value = pmreadloc(card, base, P5381_RXSDQ_IND_ADDR);
    register_value |= BIT13;
    card_write_iom(card, base + (P5381_TXSDQ_IND_ADDR << 2), register_value);
    do {
        register_value |= BIT14;
        card_write_iom(card, base + (P5381_TXSDQ_IND_ADDR << 2), register_value);
        register_value = pmreadloc(card, base, P5381_TXSDQ_IND_ADDR);
    } while (!(register_value& BIT12));
    register_value &= ~BIT13;
    register_value |= BIT14;
    card_write_iom(card, base + (P5381_TXSDQ_IND_CONFIG << 2), register_value);
}
void
fifo_config(DagCardPtr card, uint32_t base, network_mode_t mode)
{
    uint32_t register_value = 0;
    /* fifo config */
    register_value = pmreadind(card, base, P5381_RXSDQ_IND_ADDR, 0, P5381_RXSDQ_IND_CONFIG);
    if (mode == kNetworkModePoS)
    {
        register_value |= (BIT6|BIT7|BIT14|BIT15);
    }
    else
    {
        register_value |= (BIT6|BIT7|BIT15);
        register_value &= ~BIT14;
    }
    pmwriteind(card, base + (P5381_RXSDQ_IND_ADDR << 2), 0, base + (P5381_RXSDQ_IND_CONFIG << 2), register_value);

    if (mode == kNetworkModePoS)
    {
        register_value = 4<<8;
    }
    else
    {
        register_value = 3<<8;
    }

    pmwriteind(card, base +(P5381_RXSDQ_IND_ADDR << 2), 0, base + (P5381_RXSDQ_IND_DATA_AV_THRSH << 2),register_value);

    register_value = pmreadind(card, base, P5381_TXSDQ_IND_ADDR, 0, P5381_TXSDQ_IND_CONFIG);
    if (mode == kNetworkModePoS)
    {
        register_value |= (BIT6|BIT7|BIT14|BIT15);
    }
    else
    {
        register_value |= (BIT6|BIT7|BIT15);
        register_value &= ~BIT14;
    }
    pmwriteind(card, base + (P5381_TXSDQ_IND_ADDR << 2), 0, base + (P5381_TXSDQ_IND_CONFIG << 2), register_value);
}
void 
switch_gpp_to_pos_atm(DagCardPtr card, uint32_t base, uint8_t pos)
{
}
void 
phy_burst_size(DagCardPtr card, uint32_t base, uint32_t burst_size)
{
}
void 
phy_enable(DagCardPtr card, uint32_t base)
{
    uint32_t register_value = 0;
    /* PHY enable */
    register_value = pmreadloc(card, base, P5381_RXPHY_CONFIG);
    register_value |= BIT3;
    register_value &= ~BIT0;
    card_write_iom(card, base +(P5381_RXPHY_CONFIG << 2), register_value);

    register_value = pmreadloc(card, base, P5381_TXPHY_CONFIG);
    register_value &= ~(BIT0|BIT6);
    card_write_iom(card, base + (P5381_TXPHY_CONFIG << 2), register_value);

}
void 
sirp_enable(DagCardPtr card, uint32_t base)
{
    uint32_t register_value = 0;
    /* SIRP enable */
    register_value = pmreadloc(card, base, P5381_SIRP_CONFIG_TIMESLOT);
    register_value |= BIT0;
    card_write_iom(card, base + (P5381_SIRP_CONFIG_TIMESLOT << 2), register_value);

}
void 
rhpp_invcnt_set(DagCardPtr card, uint32_t base)
{
    uint32_t register_value = 0;
    /* RHPP INVCNT set */
    register_value = pmreadind(card, base, P5381_RHPP_IND_ADDR, 1, P5381_RHPP_IND_DATA);
    register_value |= BIT4;
    pmwriteind(card, base + (P5381_RHPP_IND_ADDR << 2), 1, base + (P5381_RHPP_IND_DATA << 2), register_value);
}

static uint64_t
pmreadloc3(DagCardPtr card, uint32_t base, uint32_t loc)
{
        return (((uint64_t)pmreadloc(card, base, loc+2) << 32) | (pmreadloc(card, base, loc+1) << 16) | pmreadloc(card, base, loc));
}

static uint32_t
pmreadloc2(DagCardPtr card, uint32_t base, uint32_t loc)
{
        return ((pmreadloc(card, base, loc+1) << 16) | pmreadloc(card, base, loc));
}

static uint32_t
pmreadloc(DagCardPtr card, uint32_t base, uint32_t loc)
{
    loc = ((loc << 2) + base);
    return (card_read_iom(card, loc) & 0xffff);
}

static uint32_t
pmreadind(DagCardPtr card, uint32_t base, uint32_t addr, uint32_t path, uint32_t reg)
{
    int val;

    /* spin on busy */
    do {
        val = pmreadloc(card, base, addr);
    } while (val & BIT15);

    val = path;
    val |= BIT14; /* read */
    card_write_iom(card, base + (addr << 2), val);

    /* spin on busy */
    do {
        val = pmreadloc(card, base, addr);
    } while (val & BIT15);

    return pmreadloc(card, base, reg);
}
static void
pmwriteind(DagCardPtr card, uint32_t addr, uint32_t path, uint32_t reg, uint32_t parm)
{
 int val;

 /* spin on busy */
 do {
     val = card_read_iom(card, addr);
     val = val & 0xffff;
 } while (val & BIT15);

 card_write_iom(card, reg, parm);

 val = path;
 val &= ~BIT14; /* write */
 card_write_iom(card, addr, val);

 /* spin on busy */
 do {
     val = card_read_iom(card, addr);
     val = val & 0xffff;
 } while (val & BIT15);
}


static AttributePtr
get_new_linerate_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineRate); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, linerate_dispose);
        attribute_set_post_initialize_routine(result, linerate_post_initialize);
        attribute_set_getvalue_routine(result, linerate_get_value);
        attribute_set_setvalue_routine(result, linerate_set_value);
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
linerate_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
linerate_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

    }
}


static void*
linerate_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* always its 1GigE */
        line_rate_t lr = kLineRateOC48c;
        attribute_set_value_array(attribute, &lr, sizeof(lr));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}


static void
linerate_set_value(AttributePtr attribute, void* value, int length)
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




