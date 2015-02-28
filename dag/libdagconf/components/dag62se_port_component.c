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
#include "../include/cards/dag6_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/dag62se_port_component.h"
#include "../include/attribute_factory.h"
#include "../include/cards/dag62se_impl.h"

#include "dag_platform.h"
#include "dagutil.h"
#include "dag_component.h"

#define BUFFER_SIZE 1024



typedef struct
{
uint32_t mBase;
int mIndex;
} port_state_t;


/* port component. */
static void dag62se_port_dispose(ComponentPtr component);
static void dag62se_port_reset(ComponentPtr component);
static void dag62se_port_default(ComponentPtr component);
static int dag62se_port_post_initialize(ComponentPtr component);
static dag_err_t dag62se_port_update_register_base(ComponentPtr component);

/* Functions to read and write control registers */
static uint64_t readloc8(DagCardPtr card, uint32_t base, uint32_t loc);
static uint32_t readloc4(DagCardPtr card, uint32_t base, uint32_t loc);
static uint32_t readloc3(DagCardPtr card, uint32_t base, uint32_t loc);
static uint32_t readloc2(DagCardPtr card, uint32_t base, uint32_t loc);
static uint32_t readloc(DagCardPtr card, uint32_t base, uint32_t loc);
static void writeloc(DagCardPtr card, uint32_t base, uint32_t loc, uint32_t val);

/* pscramble */
static AttributePtr get_new_pscramble(void);
static void* pscramble_get_value(AttributePtr attribute);
static void pscramble_set_value(AttributePtr attribute, void* value, int length);

/* lsfcl */
static AttributePtr get_new_lsfcl(void);
static void* lsfcl_get_value(AttributePtr attribute);
static void lsfcl_set_value(AttributePtr attribute, void* value, int length);

/* lseql */
static AttributePtr get_new_lseql(void);
static void* lseql_get_value(AttributePtr attribute);
static void lseql_set_value(AttributePtr attribute, void* value, int length);

/* master slave */
//static AttributePtr get_new_masterslave(void);
//static void* masterslave_get_value(AttributePtr attribute);
//static void masterslave_set_value(AttributePtr attribute, void* value, int length);

/* Network Mode */
static AttributePtr get_new_network_mode(void);
static void* network_mode_get_value(AttributePtr attribute);
static void network_mode_set_value(AttributePtr attribute, void* value, int length);

/* Ethernet Mode */
static AttributePtr get_new_ethernet_mode(void);
static void* ethernet_mode_get_value(AttributePtr attribute);
static void ethernet_mode_set_value(AttributePtr attribute, void* value, int length);

/* crc */
static AttributePtr get_new_crc(void);
static void* crc_get_value(AttributePtr attribute);
static void crc_set_value(AttributePtr attribute, void* value, int length);

/* crcstrip */
static AttributePtr get_new_crcstrip(void);
static void* crcstrip_get_value(AttributePtr attribute);
static void crcstrip_set_value(AttributePtr attribute, void* value, int length);

/* pmincheck */
static AttributePtr get_new_pmincheck(void);
static void* pmincheck_get_value(AttributePtr attribute);
static void pmincheck_set_value(AttributePtr attribute, void* value, int length);

/* pmaxcheck */
static AttributePtr get_new_pmaxcheck(void);
static void* pmaxcheck_get_value(AttributePtr attribute);
static void pmaxcheck_set_value(AttributePtr attribute, void* value, int length);

/* minlen */
static AttributePtr get_new_minlen(void);
static void* minlen_get_value(AttributePtr attribute);
static void minlen_set_value(AttributePtr attribute, void* value, int length);

/* maxlen */
static AttributePtr get_new_maxlen(void);
static void* maxlen_get_value(AttributePtr attribute);
static void maxlen_set_value(AttributePtr attribute, void* value, int length);

/* Latch Counter */
static AttributePtr get_new_latch(void);
static void latch_set_value(AttributePtr attribute, void* value, int length);

/* Reveive Alarm Indication */
static AttributePtr get_new_rai(void);
static void* rai_get_value(AttributePtr attribute);

/* Receive Lock Error */
static AttributePtr get_new_rle(void);
static void* rle_get_value(AttributePtr attribute);

/* Receive Power Alarm */
static AttributePtr get_new_rpa(void);
static void* rpa_get_value(AttributePtr attribute);

/* Loss Of Clock */
static AttributePtr get_new_loc(void);
static void* loc_get_value(AttributePtr attribute);

/* BER*/
static AttributePtr get_new_ber(void);
static void* ber_get_value(AttributePtr attribute);

/* Local Fault */
static AttributePtr get_new_lft(void);
static void* lft_get_value(AttributePtr attribute);

/* FCS ERR*/
static AttributePtr get_new_fcserr(void);
static void* fcserr_get_value(AttributePtr attribute);

/* Bad Packet*/
static AttributePtr get_new_badpkt(void);
static void* badpkt_get_value(AttributePtr attribute);

/*  Good Packet*/
static AttributePtr get_new_goodpkt(void);
static void* goodpkt_get_value(AttributePtr attribute);

/* Receive FIFO Errors */
static AttributePtr get_new_rxf(void);
static void* rxf_get_value(AttributePtr attribute);

/* BIP1 Errors*/
static AttributePtr get_new_bip1(void);
static void* bip1_get_value(AttributePtr attribute);

/* BIP2 Errors */
static AttributePtr get_new_bip2(void);
static void* bip2_get_value(AttributePtr attribute);

/* BIP3 Errors */
static AttributePtr get_new_bip3(void);
static void* bip3_get_value(AttributePtr attribute);

/* C2 PathLabel*/
static AttributePtr get_new_c2(void);
static void* c2_get_value(AttributePtr attribute);

/* RxParity*/
static AttributePtr get_new_rxparity(void);
static void* rxparity_get_value(AttributePtr attribute);

/* Transmit FIFO*/
static AttributePtr get_new_txf(void);
static void* txf_get_value(AttributePtr attribute);

/* TxParity*/
static AttributePtr get_new_txparity(void);
static void* txparity_get_value(AttributePtr attribute);

/* Transmit FIFO Error*/
static AttributePtr get_new_tfe(void);
static void* tfe_get_value(AttributePtr attribute);

/* Transmit Alarm Indication*/
static AttributePtr get_new_tai(void);
static void* tai_get_value(AttributePtr attribute);

/* Laser Bias Alarm*/
static AttributePtr get_new_lba(void);
static void* lba_get_value(AttributePtr attribute);

/* Laser Temperature Alarm*/
static AttributePtr get_new_lta(void);
static void* lta_get_value(AttributePtr attribute);

/* Transmit Lock Error*/
static AttributePtr get_new_tle(void);
static void* tle_get_value(AttributePtr attribute);

/* RxBytes*/
static AttributePtr get_new_rxbytes(void);
static void* rxbytes_get_value(AttributePtr attribute);

/* RxBytes Bad*/
static AttributePtr get_new_rxbytesbad(void);
static void* rxbytesbad_get_value(AttributePtr attribute);

/* drop_count */
static AttributePtr get_new_drop_count_attribute(void);
static void* drop_count_get_value(AttributePtr attribute);


ComponentPtr
dag62se_get_new_port(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentPort, card);

    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        port_state_t* state = (port_state_t*)malloc(sizeof(port_state_t));
        component_set_dispose_routine(result, dag62se_port_dispose);
        component_set_reset_routine(result, dag62se_port_reset);
        component_set_default_routine(result, dag62se_port_default);
        component_set_post_initialize_routine(result, dag62se_port_post_initialize);
        component_set_update_register_base_routine(result, dag62se_port_update_register_base);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;
}

static dag_err_t
dag62se_port_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = component_get_card(component);
        port_state_t* state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_S19205, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static void
dag62se_port_dispose(ComponentPtr component)
{

}

static void
dag62se_port_reset(ComponentPtr component)
{
}

static void
dag62se_port_default(ComponentPtr component)
{
    DagCardPtr card = NULL;
    attr_uuid_t any_attribute;
    uint32_t val = 0;
    char * mac_buf = NULL;
    uint64_t mac = 0;
    uint32_t a,b,c,d,e,f;    
    port_state_t* state = component_get_private_state(component);
    card = component_get_card(component);
    state = component_get_private_state(component);

    /* Default */
    val = card_read_iom(card, PCI_OPTICS);
    val &= ~BIT4; /* assert reset */
    card_write_iom(card, PCI_OPTICS, val);
    usleep(10000);
    val |= BIT4; /* deassert reset */
    card_write_iom(card, PCI_OPTICS, val);

    val = card_read_iom(card, PCI_OPTICS);
    val &= ~BIT1; /* Set clock to OC192 */
    card_write_iom(card, PCI_OPTICS, val);

    val = readloc(card, state->mBase, S19205_RX_LOSEXT_LEVEL);
    /* RX_LOSEXT is not implemented in hardware - inhibit
    * RX_LOS_ALL_ZERO doesn't work either - inhibit */
    val |= BIT2|BIT3|BIT6|BIT7;
    writeloc(card, state->mBase, S19205_RX_LOSEXT_LEVEL, val);

    /* RX_STATE_RESET to unlatch RX_LOS from faulty RX_LOS_ALL_ZERO */
    val = readloc(card, state->mBase, S19205_LATCH_CNT);
    val |= BIT1;
    writeloc(card, state->mBase, S19205_LATCH_CNT, val);
    usleep(10000);
    val &= ~BIT1;
    writeloc(card, state->mBase, S19205_LATCH_CNT, val);

    val = readloc(card, state->mBase, S19205_TX_XMIT_HOLDOFF);
    val |= BIT7;  /* wait till tx fifo 25% full = 256B */
    writeloc(card, state->mBase, S19205_TX_XMIT_HOLDOFF, val);

    val = readloc(card, state->mBase, S19205_TX_SIZE_MODE);
    val |= BIT0;  /* ctrl parity inhibit */
    val |= BIT1;  /* num of valid bytes in word */
    writeloc(card, state->mBase, S19205_TX_SIZE_MODE, val);

    val = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
    val |= BIT0;  /* RX_POS_ADRCTL_INH on, don't strip address */
    writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, val);

    val = readloc(card, state->mBase, S19205_TX_POS_ADRCTL_INS);
    val &= ~BIT0; /* disable address insertion */
    writeloc(card, state->mBase, S19205_TX_POS_ADRCTL_INS, val);

    /*
    * Thermal configuration
    */
    //if (has_temp)
    //{
     //   dagutil_start_copro_fan(dagiom, smb_base);
    //}                
    /* PoS */
    any_attribute = dag_component_get_attribute_uuid((dag_component_t)component, kUint32AttributeNetworkMode);
    dag_config_set_uint32_attribute((dag_card_ref_t)card, any_attribute, kNetworkModePoS);
    
    /*pscramble */
    any_attribute = dag_component_get_attribute_uuid((dag_component_t)component, kBooleanAttributePayloadScramble);
    dag_config_set_boolean_attribute((dag_card_ref_t)card, any_attribute, 1);

    /* Copy the mac address to the PHY */
    any_attribute = dag_component_get_attribute_uuid((dag_component_t)component, kStringAttributeEthernetMACAddress);
    mac_buf = (char *)attribute_get_value((AttributePtr)any_attribute);

    sscanf(mac_buf, "%x:%x:%x:%x:%x:%x", &a, &b, &c, &d, &e, &f);
    mac = ((((uint64_t)a)&0xff)<<40) +
          ((((uint64_t)b)&0xff)<<32) +
          ((((uint64_t)c)&0xff)<<24) +
          ((((uint64_t)d)&0xff)<<16) +
          ((((uint64_t)e)&0xff)<<8) +
          (((uint64_t)f)&0xff);
    writeloc(card, state->mBase, S19205_XG_MAC_ADDR, (uint32_t)mac);
    writeloc(card, state->mBase, S19205_XG_MAC_ADDR + 4, (uint32_t)(mac>>32));

}

static int
dag62se_port_post_initialize(ComponentPtr component)
{
    port_state_t* state = NULL;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t bool_attr_mask = 0;
    AttributePtr attr = NULL;
    dag62se_state_t * card_state = NULL;
    uintptr_t address = 0;

    state = component_get_private_state(component);
    card = component_get_card(component);
    card_state = (dag62se_state_t *)card_get_private_state(card);
    state->mBase = card_get_register_address(card, DAG_REG_S19205, state->mIndex);

    /* laser */
    //address = (uintptr_t)card_get_iom_address(card) + TX_OPTICS;
    //bool_attr_mask = BIT6;
    //grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    //attr = attribute_factory_make_attribute(kBooleanAttributeLaser, grw, &bool_attr_mask, 1);
    //component_add_attribute(component, attr);            

    /* facility loopback */
    bool_attr_mask = BIT4;
    grw = grw_init(card, S19205_RX_TO_TX_SYS_LOOPBACK, grw_dag62se_read, grw_dag62se_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeFacilityLoopback, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);

    /* equipment loopback */
    bool_attr_mask = BIT2;
    grw = grw_init(card, S19205_RX_MAP, grw_dag62se_read, grw_dag62se_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEquipmentLoopback, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);

    /* Loss Of Signal*/
    bool_attr_mask = BIT5;
    grw = grw_init(card, S19205_RX_LOS, grw_dag62se_read, grw_dag62se_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfSignal, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);

    /* Out Of Frame*/
    bool_attr_mask = BIT3;
    grw = grw_init(card, S19205_RX_LOS, grw_dag62se_read, grw_dag62se_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeOutOfFrame, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);

    /* Loss Of Frame*/
    bool_attr_mask = BIT2;
    grw = grw_init(card, S19205_RX_LOS, grw_dag62se_read, grw_dag62se_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFrame, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);

    /* Loss Of Pointer*/
    bool_attr_mask = BIT1;
    grw = grw_init(card, S19205_RX_PI_LOP, grw_dag62se_read, grw_dag62se_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfPointer, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);
    
    /* Remote Fault*/
    bool_attr_mask = BIT6;
    grw = grw_init(card, S19205_RX_ETH_PHY, grw_dag62se_read, grw_dag62se_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRemoteFault, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);

    /* Add Ethernet MAC Address */
    address = 8 + (state->mIndex*8); /* First port */
    
    grw = grw_init(card, address, grw_rom_read, NULL);
    attr = attribute_factory_make_attribute(kStringAttributeEthernetMACAddress, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);
    
    if (card_state->mSteerBase != 0)
    {
        bool_attr_mask = 0xFF;
        grw = grw_init(card, card_state->mSteerBase, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kUint32AttributeSteer, grw, &bool_attr_mask, 1);
        component_add_attribute(component, attr);
    }
    /* ethernet mode */
    attr = get_new_ethernet_mode();
    component_add_attribute(component, attr);

    /* network mode */
    attr = get_new_network_mode();
    component_add_attribute(component, attr);
    
    /* payload scramble */
    attr = get_new_pscramble();
    component_add_attribute(component, attr);
    
    /* line side facility loopback */
    attr = get_new_lsfcl();
    component_add_attribute(component, attr);
    
    /* line side equipment loopback */
    attr = get_new_lseql();
    component_add_attribute(component,attr);
    
    /* Master / Slave */
    /* This not valid for 6.2SE is for other DAG 6 cards */
//    attr = get_new_masterslave();
//    component_add_attribute(component, attr);
    
    /* crc */
    attr = get_new_crc();
    component_add_attribute(component,attr);
    
    /* crcstrip */
    attr = get_new_crcstrip();
    component_add_attribute(component, attr);
    
    /* min check */
    attr = get_new_pmincheck();
    component_add_attribute(component, attr);
    
    /* max check */
    attr = get_new_pmaxcheck();
    component_add_attribute(component, attr);
    
    /* min len*/
    attr = get_new_minlen();
    component_add_attribute(component, attr);
    
    /* max len */
    attr = get_new_maxlen();
    component_add_attribute(component, attr);
    
    /* latch counter */
    attr = get_new_latch();
    component_add_attribute(component, attr);

    /* Receive Alarm Indication */
    attr = get_new_rai();
    component_add_attribute(component, attr);

    /* Receive Lock Error */
    attr = get_new_rle();
    component_add_attribute(component, attr);

    /* Receive Power Alarm */
    attr = get_new_rpa();
    component_add_attribute(component, attr);

    /* Loss Of Clock*/
    attr = get_new_loc();
    component_add_attribute(component, attr);    

    /* Local Fault*/
    attr = get_new_lft();
    component_add_attribute(component, attr);    

    /* FCS Error*/
    attr = get_new_fcserr();
    component_add_attribute(component, attr);    

    /* Bad Packet*/
    attr = get_new_badpkt();
    component_add_attribute(component, attr);    

    /* Good Packet*/
    attr = get_new_goodpkt();
    component_add_attribute(component, attr);    

    /* Receive FIFO Error*/
    attr = get_new_rxf();
    component_add_attribute(component, attr);    

    /* BER*/
    attr = get_new_ber();
    component_add_attribute(component, attr);    

    /* BIP1*/
    attr = get_new_bip1();
    component_add_attribute(component, attr);    

    /* BIP2*/
    attr = get_new_bip2();
    component_add_attribute(component, attr);    

    /* BIP3*/
    attr = get_new_bip3();
    component_add_attribute(component, attr);    

    /* C2*/
    attr = get_new_c2();
    component_add_attribute(component, attr);   

    /* RxParity*/
    attr = get_new_rxparity();
    component_add_attribute(component, attr); 

    /* RxBytes*/
    attr = get_new_rxbytes();
    component_add_attribute(component, attr); 

    /* RxBytes Bad*/
    attr = get_new_rxbytesbad();
    component_add_attribute(component, attr);  

    attr = get_new_drop_count_attribute();
    component_add_attribute(component, attr);    

    /* Check to see if it supports transmit */
    /* DAG 6.0 is the only one that does so unlikely */
    if ((card_read_iom(card, 0x348)&0xfb) == 0x03)
    {
    
        /* Transmit FIFO Error*/
        attr = get_new_txf();
        component_add_attribute(component, attr);    

        /* TxParity*/
        attr = get_new_txparity();
        component_add_attribute(component, attr);    

        /* Transmit FIFO Error*/
        attr = get_new_tfe();
        component_add_attribute(component, attr);    

        /* Transmit Alarm Indication*/
        attr = get_new_tai();
        component_add_attribute(component, attr);    

        /* Laser Bias Alarm*/
        attr = get_new_lba();
        component_add_attribute(component, attr);    

        /* Laser Temperature Alarm*/
        attr = get_new_lta();
        component_add_attribute(component, attr);    

        /* Transmit Lock Error*/
        attr = get_new_tle();
        component_add_attribute(component, attr);    
    }
    return 1;
}

static AttributePtr
get_new_pscramble(void)
{
    AttributePtr result = attribute_init(kBooleanAttributePayloadScramble);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, pscramble_get_value);
        attribute_set_setvalue_routine(result, pscramble_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "pscramble");
        attribute_set_description(result, "Payload Scramble");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
pscramble_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_RX_MAP);
        if (register_value & BIT3)
        {
            /* no pscramble */
            value = 0;
        }
        else
        {
            value = 1;
        }
        return (void*)&value;
    }
    return NULL;
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
        register_value = readloc(card, state->mBase, S19205_RX_TO_TX_SYS_LOOPBACK);
        if (*(uint8_t*)value == 0)
        {
            register_value |= BIT1;
        }
        else
        {
            register_value &= ~BIT1;
        }
        writeloc(card, state->mBase, S19205_RX_TO_TX_SYS_LOOPBACK, register_value);

        register_value = readloc(card, state->mBase, S19205_RX_MAP);
        if (*(uint8_t*)value == 0) 
        {
            register_value |= BIT3;
        }
        else
        {
            register_value &= ~BIT3;
        }
        writeloc(card, state->mBase, S19205_RX_MAP, register_value);
    }
}

static AttributePtr
get_new_lsfcl(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLineSideFacilityLoopback);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, lsfcl_get_value);
        attribute_set_setvalue_routine(result, lsfcl_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "lsfcl");
        attribute_set_description(result, "Line Side Facility Loopback");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
lsfcl_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_RX_TO_TX_SONET_LOOPBACK);
        if (register_value & BIT4)
        {
            value = 0;
        }
        else
        {
            value = 1;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
lsfcl_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_RX_TO_TX_SONET_LOOPBACK);
        if (*(uint8_t*)value == 0) 
        {
            register_value |= BIT4;
        } 
        else 
        {
            register_value &= ~BIT4;
        }
        writeloc(card, state->mBase, S19205_RX_TO_TX_SONET_LOOPBACK, register_value);
    }
}

static AttributePtr
get_new_lseql(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLineSideEquipmentLoopback);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, lseql_get_value);
        attribute_set_setvalue_routine(result, lseql_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "lseql");
        attribute_set_description(result, "Line Side Equipment Loopback");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
lseql_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_TX_TO_RX_SONET_LOOPBACK);
        if (register_value & BIT3)
        {
            value = 0;
        }
        else
        {
            value = 1;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
lseql_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_TX_TO_RX_SONET_LOOPBACK);
        if (*(uint8_t*)value == 0) 
        {
            register_value |= BIT3;
        } 
        else 
        {
            register_value &= ~BIT3;
        }
        writeloc(card, state->mBase, S19205_TX_TO_RX_SONET_LOOPBACK, register_value);
    }
}

static AttributePtr
get_new_crc(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCrcSelect);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, crc_get_value);
        attribute_set_setvalue_routine(result, crc_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "crc_select");
        attribute_set_description(result, "crc");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
crc_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint8_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
	        register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
            if (register_value & BIT1)
            {
                value = kCrcOff;
            }
            else
            {
                value = kCrc32;
            }
        }
        else if (net_mode == kNetworkModeEth)
        {
            register_value = readloc(card, state->mBase, S19205_RX_ETH_INH2);
            if (register_value & BIT0)
            {
                value = kCrcOff;
            }
            else
            {
                value = kCrc32;
            }
        }
        else
        {
            return kCrcInvalid;
        }
        return (void *)&value;
    }
    return NULL;
}

static void
crc_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == kCrcOff) {
            /* POS */
            register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
			register_value |= BIT1;
			writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
			/* ETH */
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH2);
			register_value |= BIT0;
			writeloc(card, state->mBase, S19205_RX_ETH_INH2, register_value);
		} else {
			/* POS */
			register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
			register_value &= ~BIT1;
			writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
			/* ETH */
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH2);
			register_value &= ~BIT0;
			writeloc(card, state->mBase, S19205_RX_ETH_INH2, register_value);
		}
    }
}

static AttributePtr
get_new_crcstrip(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeCrcStrip);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, crcstrip_get_value);
        attribute_set_setvalue_routine(result, crcstrip_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "crc_strip");
        attribute_set_description(result, "Crcstrip");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
crcstrip_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint8_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
	        register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
            if (register_value & BIT1)
            {
                value = 0;
            }
            else
            {
                value = 1;
            }
        }
        else if (net_mode == kNetworkModeEth)
        {
            register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
            if (register_value & BIT7)
            {
                value = 0;
            }
            else
            {
                value = 1;
            }
        }
        else
        {
            return 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
crcstrip_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
		if (*(uint8_t*)value == 0) {
			/* POS */
			register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
			register_value |= BIT1;
			writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
			/* ETH */
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
			register_value |= BIT3|BIT7;
			writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
		} else {
			/* POS */
			register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
			register_value &= ~BIT1;
			writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
			/* ETH */
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
			register_value &= ~(BIT3|BIT7);
			writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
		}
        
    }
}

static AttributePtr
get_new_pmincheck(void)
{
    AttributePtr result = attribute_init(kBooleanAttributePMinCheck);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, pmincheck_get_value);
        attribute_set_setvalue_routine(result, pmincheck_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "pmincheck");
        attribute_set_description(result, "Predefined Minimum Check");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
pmincheck_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint8_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
		    register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
            if (register_value & BIT5)
            {
                value = 1;
            }
            else
            {
                value = 0;
            }

        }
        else if (net_mode == kNetworkModeEth)
        {
		    register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
            if (register_value & BIT6)
            {
                value = 0;
            }
            else 
            {
                value = 1;
            }
        }
        else
        {
            value = 0;

        }
        return (void*)&value;
    }
    return NULL;
}

static void
pmincheck_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == 0) {
            /* POS */
            register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
            register_value &= ~BIT5;
            writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
            /* ETH */
            register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
            register_value |= BIT6;
            writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
         } else {
            register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
            register_value |= BIT5;
		    writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
			/* ETH */
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
			register_value &= ~BIT6;
			writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
         }
    }
}

static AttributePtr
get_new_pmaxcheck(void)
{
    AttributePtr result = attribute_init(kBooleanAttributePMaxCheck);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, pmaxcheck_get_value);
        attribute_set_setvalue_routine(result, pmaxcheck_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "pmaxcheck");
        attribute_set_description(result, "Predefined Maximum Check");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}
static void*
pmaxcheck_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint8_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
            register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
            if (register_value & BIT4)
            {
                value = 1;
            }
            else
            {
                value = 0;
            }
        }            
        else if (net_mode == kNetworkModeEth)
        {
            register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
            if (register_value & BIT5)
            {
                value = 0;
            }
            else
            {
                value = 1;
            }
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
pmaxcheck_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == 0) {
            /* POS */
            register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
            register_value &= ~BIT4;
			writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
			/* ETH */
			register_value = readloc(card, state->mBase,S19205_RX_ETH_INH1);
			register_value |= BIT5;
			writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
		} else {
			register_value = readloc(card, state->mBase, S19205_RX_POS_PMIN_ENB);
			register_value |= BIT4;
			writeloc(card, state->mBase, S19205_RX_POS_PMIN_ENB, register_value);
			/* ETH */
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
			register_value &= ~BIT5;
			writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
		}        
    }
}

/*static AttributePtr
get_new_masterslave(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMasterSlave);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, masterslave_get_value);
        attribute_set_setvalue_routine(result, masterslave_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "master_or_slave");
        attribute_set_description(result, "master or slave");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
masterslave_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    uint32_t register_value;
    static uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, PCI_OPTICS);
        if (register_value & BIT0)
        {
            value = kMaster;
        }
        else
        {
            value = kSlave;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
masterslave_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    uint32_t register_value = 0;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, PCI_OPTICS);
        if (*(uint8_t*)value == kMaster)
        {
            register_value |= BIT0;
        }
        else
        {
            register_value &= ~BIT0;
        }
    }
}
*/
static AttributePtr
get_new_ethernet_mode(void)
{
    AttributePtr result = attribute_init(kUint32AttributeEthernetMode);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, ethernet_mode_get_value);
        attribute_set_setvalue_routine(result, ethernet_mode_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "ethernet_mode");
        attribute_set_description(result, "Set the Ethernet mode");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}
static void*
ethernet_mode_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    uint32_t register_value;
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, PCI_OPTICS);
        if (register_value & BIT1)
        {
            value = kEthernetMode10GBase_LR; /* LAN */
        }
        else 
        {
            value = kEthernetMode10GBase_LW; /* WAN */
        }
        return (void*)&value; 
    }
    return NULL;
}
static void
ethernet_mode_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;
    attr_uuid_t network_attr;

    if (1 == valid_attribute(attribute))
    {
        /* set network mode to Ethernet */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        dag_config_set_uint32_attribute((dag_card_ref_t)card, network_attr, kNetworkModeEth);

		if (*(uint8_t*)value == 0) {
            register_value = card_read_iom(card, PCI_OPTICS);
			register_value |= BIT1; /* Set clock to LAN */
			card_write_iom(card, PCI_OPTICS, register_value);
			
			register_value = card_read_iom(card, RX_OPTICS);
			register_value &= ~(BIT3|BIT4); /* Set RX Optics to LAN */
			card_write_iom(card, RX_OPTICS, register_value);
	
			register_value = readloc(card, state->mBase, S19205_TX_ETH_INH1);
			register_value |= BIT5; /* TX_XG_WAN_INH */
			writeloc(card, state->mBase, S19205_TX_ETH_INH1, register_value);
	
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
			register_value |= BIT0; /* RX_XG_WAN_INH */
			writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
	
			if ((card_read_iom(card, 0x348)&0xfb) == 0x03) {
				register_value = card_read_iom(card, TX_OPTICS);
				register_value &= ~(BIT8|BIT9); /* Set TX Optics to LAN */
				card_write_iom(card, TX_OPTICS, register_value);
            }
        } else {
            register_value = card_read_iom(card, PCI_OPTICS);
			register_value &= ~BIT1; /* Set clock to WAN */
			card_write_iom(card, PCI_OPTICS, register_value);
	
			register_value = card_read_iom(card, RX_OPTICS);
			register_value |= (BIT3|BIT4); /* Set RX Optics to WAN */
			card_write_iom(card, RX_OPTICS, register_value);
	
			register_value = readloc(card, state->mBase, S19205_TX_ETH_INH1);
			register_value &= ~BIT5; /* TX_XG_WAN_INH */
			writeloc(card, state-> mBase, S19205_TX_ETH_INH1, register_value);
	
			register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
			register_value &= ~BIT0; /* RX_XG_WAN_INH */
			writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
	
			if ((card_read_iom(card,0x348)&0xfb) == 0x03) {
				register_value = card_read_iom(card, TX_OPTICS);
				register_value |= (BIT8|BIT9); /* Set TX Optics to LAN */
				card_write_iom(card, TX_OPTICS, register_value);
			}
		}
    }
}

static AttributePtr
get_new_network_mode(void)
{
    AttributePtr result = attribute_init(kUint32AttributeNetworkMode);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, network_mode_get_value);
        attribute_set_setvalue_routine(result, network_mode_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "network_mode");
        attribute_set_description(result, "Network Mode");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}
static void*
network_mode_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value;
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase,S19205_TX_MAP)&0xFF;
        switch(register_value)
        {
            case 0x04:
                value = kNetworkModePoS;
                break;
            case 0x06:
                value = kNetworkModeRAW;
                break;
            case 0x0C:
                value = kNetworkModeEth;
                break;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
network_mode_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;
    dag62se_state_t * card_state = (dag62se_state_t *)card_get_private_state(card);

    if (1 == valid_attribute(attribute))
    {
        switch(*(network_mode_t*)value)
        {
            case kNetworkModePoS:
				register_value = card_read_iom(card, PCI_OPTICS);
				register_value &= ~BIT1; /* Set clock to OC192 */
				card_write_iom(card, PCI_OPTICS, register_value);
	
				register_value = card_read_iom(card, RX_OPTICS);
				register_value |= (BIT3|BIT4); /* Set RX Optics to SONET */
				card_write_iom(card,RX_OPTICS, register_value);
				
				if ((card_read_iom(card, 0x348)&0xfb) == 0x03) {
					register_value = card_read_iom(card, TX_OPTICS);
					register_value |= (BIT8|BIT9); /* Set TX Optics to SONET */
					card_write_iom(card, TX_OPTICS, register_value);
				}
	
				register_value = 0x04;
				writeloc(card, state->mBase, S19205_TX_MAP, register_value);
				register_value = readloc(card, state->mBase, S19205_RX_MAP);
				register_value |= BIT1;
				register_value &= ~(BIT0|BIT4);
				writeloc(card, state->mBase, S19205_RX_MAP, register_value);
				register_value = 0x16;
				writeloc(card, state->mBase, S19205_RX_C2EXP, register_value);
				writeloc(card, state->mBase, S19205_TX_C2INS, register_value);
	
				register_value = card_read_iom(card, card_state->mGppBase + GPP_CONFIG);
				register_value &= ~(0xff<<16);
				register_value |= (0x01<<16);
                card_write_iom(card, card_state->mGppBase + GPP_CONFIG, register_value);
                break;
            case kNetworkModeRAW:
				register_value = 0x06;
				writeloc(card, state->mBase, S19205_TX_MAP, register_value);
				register_value = readloc(card, state->mBase, S19205_RX_MAP);
				register_value |= (BIT1|BIT0);
				register_value &= ~BIT4;
				writeloc(card, state->mBase, S19205_RX_MAP, register_value);                
                break;
            case kNetworkModeEth:
				register_value = card_read_iom(card, card_state->mGppBase + GPP_CONFIG);
				register_value &= ~(0xff<<16);
				register_value |= (0x02<<16);
                card_write_iom(card, card_state->mGppBase + GPP_CONFIG, register_value);
				register_value = card_read_iom(card, card_state->mGppBase + GPP_CONFIG);
				if ((register_value & (0xff<<16)) != (0x02<<16))
					break;
	
				register_value = card_read_iom(card, PCI_OPTICS);
				register_value |= BIT1; /* Set clock to 10GE */
				card_write_iom(card, PCI_OPTICS, register_value);
	
				register_value = 0x0C;
				writeloc(card, state->mBase, S19205_TX_MAP, register_value);
				register_value = readloc(card, state->mBase, S19205_RX_MAP);
				register_value |= BIT4;
				writeloc(card, state->mBase, S19205_RX_MAP, register_value);
				register_value = 0x1A;
				writeloc(card, state->mBase, S19205_RX_C2EXP, register_value);
				writeloc(card, state->mBase, S19205_TX_C2INS, register_value);
				register_value = readloc(card, state->mBase, S19205_RX_ETH_INH1);
				register_value |= (BIT3|BIT7); /* RX_XG_PAD_DELETE_INH, RX_XG_FCS_DEL_INH */
				writeloc(card, state->mBase, S19205_RX_ETH_INH1, register_value);
                break;
            default:
                break;
        }
    }
}

static AttributePtr
get_new_minlen(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMinPktLen);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, minlen_get_value);
        attribute_set_setvalue_routine(result, minlen_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "minpktlen");
        attribute_set_description(result, "Mininum Packet Length");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
minlen_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint8_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
            value = readloc(card, state->mBase, S19205_RX_POS_PMIN);            
        }
        else if (net_mode == kNetworkModeEth)
        {
            value = 64; /* always 64 for ethernet */
        }
        return (void*)&value;
    }
    return NULL;
}

static void
minlen_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t val = *(uint32_t *)value;

    if (1 == valid_attribute(attribute))
    {
		/* POS */
		writeloc(card, state->mBase, S19205_RX_POS_PMIN, val);
		writeloc(card, state->mBase, S19205_TX_POS_PMIN, val);
		/* ETH */
		/* Always 64 */
    }
}

static AttributePtr
get_new_maxlen(void)
{
    AttributePtr result = attribute_init(kUint32AttributeMaxPktLen);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, maxlen_get_value);
        attribute_set_setvalue_routine(result, maxlen_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "maxlen");
        attribute_set_description(result, "Maximum Packet Length");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
maxlen_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint32_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);        
        if (net_mode == kNetworkModePoS)
        {
            value = readloc2(card, state->mBase, S19205_RX_POS_PMAX1);
        }
        else if ( net_mode == kNetworkModeEth)
        {
            value = readloc2(card, state->mBase, S19205_RX_ETH_PMAX1)&0x3fff;
        }
        else 
        {
            return NULL;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
maxlen_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t val = *(uint32_t *)value;

    if (1 == valid_attribute(attribute))
    {
        /* POS */
        writeloc(card, state->mBase, S19205_RX_POS_PMAX1, val&0xff);
		writeloc(card, state->mBase, S19205_RX_POS_PMAX2, val>>8);
		writeloc(card, state->mBase, S19205_TX_POS_PMAX1, val&0xff);
		writeloc(card, state->mBase, S19205_TX_POS_PMAX2, val>>8);
		/* ETH */
		writeloc(card, state->mBase, S19205_RX_ETH_PMAX1, val&0xff);
		writeloc(card, state->mBase, S19205_RX_ETH_PMAX2, val>>8);
		writeloc(card, state->mBase, S19205_TX_ETH_PMAX1, val&0xff);
		writeloc(card, state->mBase, S19205_TX_ETH_PMAX2, val>>8);        
    }
}
static AttributePtr
get_new_latch(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeCounterLatch);

    if (NULL != result)
    {
        attribute_set_setvalue_routine(result, latch_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "latch");
        attribute_set_description(result, "Latch Counters");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void
latch_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t register_value = 0;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* latch all counters in all blocks */
        register_value = readloc(card, state->mBase, S19205_LATCH_CNT);
        register_value &= ~BIT2;
        writeloc(card, state->mBase, S19205_LATCH_CNT, register_value);
        register_value |= BIT2;
        writeloc(card, state->mBase, S19205_LATCH_CNT, register_value);
        register_value = 1;
        writeloc(card, state->mBase, S19205_TX_SYS_PRTY_ERR_E, register_value);

        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);        
        if (net_mode == kNetworkModeEth)
        {
            register_value = readloc(card, state->mBase, S19205_XG_CTR_CLR);
            register_value ^= BIT1;
            writeloc(card, state->mBase, S19205_XG_CTR_CLR, register_value);
            register_value ^= BIT0;
            writeloc(card, state->mBase, S19205_XG_CTR_CLR, register_value);
        }
    }
}

static AttributePtr
get_new_rai(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReceiveAlarmIndication);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rai_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "rai");
        attribute_set_description(result, "Receive Alarm Indication");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
rai_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, RX_OPTICS);
        if (register_value & BIT5)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_rle(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReceiveLockError);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rle_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "rle");
        attribute_set_description(result, "Receive Lock Error");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
rle_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, RX_OPTICS);
        if (register_value & BIT6)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_rpa(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReceivePowerAlarm);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rpa_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "rpa");
        attribute_set_description(result, "Receive Power Alarm");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
rpa_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, RX_OPTICS);
        if (register_value & BIT7)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_loc(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLossOfClock);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, loc_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "loc");
        attribute_set_description(result, "Loss Of Clock");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
loc_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_RX_LOS);
        if (register_value & BIT4)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_ber(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeHighBitErrorRateDetected);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, ber_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "ber");
        attribute_set_description(result, "High Bit Error Rate Detected");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
ber_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_RX_ETH_PHY);
        if (register_value & BIT3)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_lft(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLocalFault);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, lft_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "lft");
        attribute_set_description(result, "Local Fault");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
lft_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_RX_ETH_PHY);
        if (register_value & BIT5)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}
static AttributePtr
get_new_fcserr(void)
{
    AttributePtr result = attribute_init(kUint64AttributeFCSErrors);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, fcserr_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_from_string_routine(result, attribute_uint64_from_string);
        attribute_set_name(result, "fcs_err");
        attribute_set_description(result, "Receive FIFO Errors");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }
    return result;
}

static void*
fcserr_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint64_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
            value = readloc3(card, state->mBase, S19205_RX_POS_FCS_ERRCNT1)  & 0x3fffff;
        }
        else 
        {
            value = readloc8(card, state->mBase, S19205_RX_ETH_FCSCNT1);
        }
        return (void*)&value;
    }
    return NULL;
}
static AttributePtr
get_new_badpkt(void)
{
    AttributePtr result = attribute_init(kUint64AttributeBadPackets);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, badpkt_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_from_string_routine(result, attribute_uint64_from_string);
        attribute_set_name(result, "bad_packet");
        attribute_set_description(result, "Bad Packets");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }

    return result;
}

static void*
badpkt_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint64_t value;

    if (1 == valid_attribute(attribute))
    {
		value = readloc8(card, state->mBase, S19205_RX_ETH_BADCNT1);
        return (void*)&value;
    }
    return NULL;
}
static AttributePtr
get_new_goodpkt(void)
{
    AttributePtr result = attribute_init(kUint64AttributeGoodPackets);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, goodpkt_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_from_string_routine(result, attribute_uint64_from_string);
        attribute_set_name(result, "good_packet");
        attribute_set_description(result, "Good Packets");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }

    return result;
}

static void*
goodpkt_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint64_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);   		
        if (net_mode == kNetworkModePoS)
        {
            value = readloc4(card, state->mBase, S19205_RX_POS_PKT_CNT1);
        }
        else
        {
            value = readloc8(card, state->mBase, S19205_RX_ETH_GOODCNT1);
        }
        return (void*)&value;
    }
    return NULL;
}
static AttributePtr
get_new_rxf(void)
{
    AttributePtr result = attribute_init(kUint64AttributeFIFOOverrunCount);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rxf_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_from_string_routine(result, attribute_uint64_from_string);
        attribute_set_name(result, "rxf");
        attribute_set_description(result, "Receive FIFO Errors");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }

    return result;
}

static void*
rxf_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint64_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);   		
        if (net_mode == kNetworkModePoS)
        {
            value = readloc(card, state->mBase, S19205_RX_FIFOERR_CNT);
        }
        else
        {
            value = readloc8(card, state->mBase, S19205_RX_ETH_OVERRUNCNT1);
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_bip1(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB1ErrorCount);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, bip1_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "bip1");
        attribute_set_description(result, "Bit Interleaved Parity 1");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
bip1_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        value = readloc2(card, state->mBase, S19205_RX_B1_ERRCNT1);
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_bip2(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB2ErrorCount);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, bip2_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "bip2");
        attribute_set_description(result, "Bit Interleaved Parity 2");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
bip2_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        value = readloc3(card, state->mBase, S19205_RX_B2_ERRCNT1);
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_bip3(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB3ErrorCount);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, bip3_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "bip3");
        attribute_set_description(result, "Bit Interleaved Parity 3");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
bip3_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        value = readloc2(card, state->mBase, S19205_RX_B3_ERRCNT1);
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_c2(void)
{
    AttributePtr result = attribute_init(kUint32AttributeC2PathLabel);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result,c2_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "c2_path_label");
        attribute_set_description(result, "C2 Path Label");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
c2_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        value = readloc(card, state->mBase, S19205_RX_C2MON);
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_rxparity(void)
{
    AttributePtr result = attribute_init(kUint32AttributeRxParityError);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result,rxparity_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "rx_parity");
        attribute_set_description(result, "Receive Parity Error");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
rxparity_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        value = card_read_iom(card, RX_PARITY); 
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_txf(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTxFIFOError);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, txf_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "TXF");
        attribute_set_description(result, "Transmit FIFO Errors");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
txf_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        value = readloc(card, state->mBase, S19205_TX_FIFOERR_CNT);
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_txparity(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeTxParityError);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, txparity_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "TXP");
        attribute_set_description(result, "TX Parity Error");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
txparity_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = readloc(card, state->mBase, S19205_TX_SYS_PRTY_ERR_E);
        if (register_value & BIT0)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_tfe(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeTransmitFIFOError);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, tfe_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "TFE");
        attribute_set_description(result, "TFE");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
tfe_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, TX_OPTICS); 
        if (register_value & BIT0)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}


static AttributePtr
get_new_tai(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeTransmitAlarmIndication);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, tai_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "TAI");
        attribute_set_description(result, "TAI");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
tai_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, TX_OPTICS); 
        if (register_value & BIT1)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_lba(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLaserBiasAlarm);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, lba_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "LBA");
        attribute_set_description(result, "LBA");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
lba_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, TX_OPTICS); 
        if (register_value & BIT2)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}


static AttributePtr
get_new_lta(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLaserTemperatureAlarm);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, lta_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "LTA");
        attribute_set_description(result, "LTA");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
lta_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, TX_OPTICS); 
        if (register_value & BIT3)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}

static AttributePtr
get_new_tle(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeTransmitLockError);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, tle_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "TLE");
        attribute_set_description(result, "TLE");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
tle_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    static uint8_t value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        register_value = card_read_iom(card, TX_OPTICS); 
        if (register_value & BIT7)
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        return (void*)&value;
    }
    return NULL;
}
static AttributePtr
get_new_rxbytes(void)
{
    AttributePtr result = attribute_init(kUint64AttributeRxBytes);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rxbytes_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_from_string_routine(result, attribute_uint64_from_string);
        attribute_set_name(result, "rx_bytes");
        attribute_set_description(result, "Received Bytes ");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }
    return result;
}

static void*
rxbytes_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint64_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
            value = 0;
        }
        else 
        {
            value = readloc8(card, state->mBase, S19205_RX_ETH_OCTETS_OK);
        }
        return (void*)&value;
    }
    return NULL;
}
static AttributePtr
get_new_rxbytesbad(void)
{
    AttributePtr result = attribute_init(kUint64AttributeRxBytesBad);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rxbytesbad_get_value);
        attribute_set_to_string_routine(result, attribute_uint64_to_string);
        attribute_set_from_string_routine(result, attribute_uint64_from_string);
        attribute_set_name(result, "rx_bytes_bad");
        attribute_set_description(result, "Received bytes in errored frames");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint64);
    }
    return result;
}

static void*
rxbytesbad_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    static uint64_t value;
    attr_uuid_t network_attr;
    network_mode_t net_mode;

    if (1 == valid_attribute(attribute))
    {
        /* get the network mode to determine where to read the value from */
        network_attr = dag_component_get_attribute_uuid((dag_component_t)comp, kUint32AttributeNetworkMode);
        net_mode = dag_config_get_uint32_attribute((dag_card_ref_t)card, network_attr);    
        if (net_mode == kNetworkModePoS)
        {
            value = 0;
        }
        else 
        {
            value = readloc8(card, state->mBase, S19205_RX_ETH_OCTETS_BAD);
        }
        return (void*)&value;
    }
    return NULL;
}

/* drop_count */
static AttributePtr
get_new_drop_count_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDropCount); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, drop_count_get_value);
        attribute_set_name(result, "drop_count");
        attribute_set_description(result, "A count of the packets dropped on a port");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}


static void*
drop_count_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        static uint32_t val = 0;
        dag62se_state_t* state = NULL;

        card = attribute_get_card(attribute);
        state = (dag62se_state_t*)card_get_private_state(card);
    
        val = card_read_iom(card, state->mGppBase + SP_OFFSET + SP_DROP);
         
        return (void*)&val;
    }
    return NULL;
}
/*
 * Functions to read and write control registers
 */
static uint64_t
readloc8(DagCardPtr card, uint32_t base, uint32_t loc)
{
	return (uint64_t)((((uint64_t)readloc(card, base, loc+7)) << 56) | (((uint64_t)readloc(card, base, loc+6)) << 48) |
		(((uint64_t)readloc(card, base, loc+5)) << 40) | (((uint64_t)readloc(card, base, loc+4)) << 32) |
		((uint64_t)readloc(card, base, loc+3) << 24)   |   (uint64_t)(readloc(card, base, loc+2) << 16) |
		(uint64_t)(readloc(card, base, loc+1) <<  8)   |   (uint64_t)readloc(card, base, loc+0));
}

static uint32_t
readloc4(DagCardPtr card, uint32_t base, uint32_t loc)
{
	return ((readloc(card, base, loc+3) << 24) | (readloc(card, base, loc+2) << 16) |
		(readloc(card, base, loc+1) <<  8) |  readloc(card, base, loc+0));
}

static uint32_t
readloc3(DagCardPtr card, uint32_t base, uint32_t loc)
{
	return ((readloc(card, base, loc+2) << 16) | (readloc(card, base, loc+1) << 8) | readloc(card, base, loc));
}

static uint32_t
readloc2(DagCardPtr card, uint32_t base, uint32_t loc)
{
	return ((readloc(card, base, loc+1) << 8) | readloc(card, base, loc));
}

static uint32_t
readloc(DagCardPtr card, uint32_t base, uint32_t loc)
{
	loc = ((loc << 2) + base);
    return (card_read_iom(card, loc) & 0xffff);
}

static void
writeloc(DagCardPtr card, uint32_t base, uint32_t loc, uint32_t val)
{
	loc = ((loc << 2) + base);
    card_write_iom(card, loc, val);
}




