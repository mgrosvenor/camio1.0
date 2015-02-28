/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag37d_port_component.c,v 1.29 2009/03/23 02:47:24 dlim Exp $
 */

/* *
 * This is the port component for the DAG 3.7D
*/


/* File Header */
#include "../include/components/dag37d_port_component.h"

#include "dagutil.h"
#include "dag_platform.h"

#include "../include/component.h"
#include "../include/card.h"
#include "../include/cards/dag3_constants.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/modules/generic_read_write.h"
#include "../include/cards/dag37d_impl.h"
#include "../include/attribute_factory.h"
#include "../include/util/enum_string_table.h"



#define BUFFER_SIZE 1024

typedef struct
{
    uint32_t mBase;
    int mIndex;
} dag37d_port_state_t;

static void dag37d_port_dispose(ComponentPtr component);
static void dag37d_port_reset(ComponentPtr component);
static void dag37d_port_default(ComponentPtr component);
static int dag37d_port_post_initialize(ComponentPtr component);


/* Default configuration modes */
static AttributePtr get_new_defaultds3atm(void);
static void defaultds3atm_to_string(AttributePtr attribute);
static void defaultds3atm_from_string(AttributePtr attribute, const char* string);

static AttributePtr get_new_defaultds3hdlc(void);
static void defaultds3hdlc_to_string(AttributePtr attribute);
static void defaultds3hdlc_from_string(AttributePtr attribute, const char* string);

static AttributePtr get_new_defaulte3atm(void);
static void defaulte3atm_to_string(AttributePtr attribute);
static void defaulte3atm_from_string(AttributePtr attribute, const char* string);

static AttributePtr get_new_defaulte3hdlc(void);
static void defaulte3hdlc_to_string(AttributePtr attribute);
static void defaulte3hdlc_from_string(AttributePtr attribute, const char* string);

static AttributePtr get_new_defaultkentrox(void);
static void defaultkentrox_to_string(AttributePtr attribute);
static void defaultkentrox_from_string(AttributePtr attribute, const char* string);

/* Facility Loopback */
static AttributePtr get_new_fcl(void);
static void* fcl_get_value(AttributePtr attribute);
static void fcl_set_value(AttributePtr attribute, void* value, int length);

/* crc attribute. */
static AttributePtr get_new_crc(void);
static void* crc_get_value(AttributePtr attribute);
static void crc_set_value(AttributePtr attribute, void* value, int length);
static void crc_to_string_routine(AttributePtr attribute);
static void crc_from_string_routine(AttributePtr attribute, const char* string);

/* Payload Scramble */
static AttributePtr get_new_pscramble(void);
static void* pscramble_get_value(AttributePtr attribute);
static void pscramble_set_value(AttributePtr attribute, void* value, int length);

/* Descramble */
static AttributePtr get_new_descramble(void);
static void* descramble_get_value(AttributePtr attribute);
static void descramble_set_value(AttributePtr attribute, void* value, int length);

/* for link_discard attribute*/
static void* dag37d_discard_get_value(AttributePtr attribute);
static void dag37d_discard_set_value(AttributePtr attribute, void* value, int length);

#if 0
/* Cell Header Descramble */
static AttributePtr get_new_cellheaderdescramble(void);
static void* cellheaderdescramble_get_value(AttributePtr attribute);
static void cellheaderdescramble_set_value(AttributePtr attribute, void* value, int length);
#endif

/* Network Mode */
static AttributePtr get_new_networkmode(void);
static void* networkmode_get_value(AttributePtr attribute);
static void networkmode_set_value(AttributePtr attribute, void* value, int length);

/* RX Monitor Mode*/
static AttributePtr get_new_rxmonitormode(void);
static void* rxmonitormode_get_value(AttributePtr attribute);
static void rxmonitormode_set_value(AttributePtr attribute, void* value, int length);

/* Framing mode */
static AttributePtr get_new_framingmode(void);
static void* framingmode_get_value(AttributePtr attribute);
static void framingmode_set_value(AttributePtr attribute, void* value, int length);
static void framing_mode_to_string_routine(AttributePtr attribute);
static void framing_mode_from_string_routine(AttributePtr attribute, const char* string);

/* FF00 Delete */
static AttributePtr get_new_ff00del(void);
static void* ff00del_get_value(AttributePtr attribute);
static void ff00del_set_value(AttributePtr attribute, void* value, int length);

/* E3 HDLC Fraction */
static AttributePtr get_new_hdlcfract(void);
static void* hdlcfract_get_value(AttributePtr attribute);
static void hdlcfract_set_value(AttributePtr attribute, void* value, int length);

/*rxlol */
static AttributePtr get_new_rxlol(void);
static void* rxlol_get_value(AttributePtr attribute);

/*rdi */
static AttributePtr get_new_rdi(void);
static void* rdi_get_value(AttributePtr attribute);

/*ais */
static AttributePtr get_new_ais(void);
static void* ais_get_value(AttributePtr attribute);

/* m23_cbit */
static AttributePtr get_new_aic_m23_cbit(void);
static void* aic_m23_cbit_get_value(AttributePtr attribute);


/* Latch and Clear attribute. */
static AttributePtr get_new_latch_and_clear(void);
static void latch_and_clear_dispose(AttributePtr attribute);
static void* latch_and_clear_get_value(AttributePtr attribute);
static void latch_and_clear_set_value(AttributePtr attribute, void* value, int length);
static void latch_and_clear_post_initialize(AttributePtr attribute);

/* RX Frames attribute. */
static AttributePtr get_new_rx_frames(void);
static void rx_frames_dispose(AttributePtr attribute);
static void* rx_frames_get_value(AttributePtr attribute);
static void rx_frames_post_initialize(AttributePtr attribute);

/* TX Frames attribute. */
static AttributePtr get_new_tx_frames(void);
static void tx_frames_dispose(AttributePtr attribute);
static void* tx_frames_get_value(AttributePtr attribute);
static void tx_frames_post_initialize(AttributePtr attribute);

/* TX Bytes attribute. */
static AttributePtr get_new_tx_bytes(void);
static void tx_bytes_dispose(AttributePtr attribute);
static void* tx_bytes_get_value(AttributePtr attribute);
static void tx_bytes_post_initialize(AttributePtr attribute);

/* RX Bytes attribute. */
static AttributePtr get_new_rx_bytes(void);
static void rx_bytes_dispose(AttributePtr attribute);
static void* rx_bytes_get_value(AttributePtr attribute);
static void rx_bytes_post_initialize(AttributePtr attribute);



ComponentPtr
dag37d_get_new_port(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentPort, card); 
    char buffer[BUFFER_SIZE];
    
    if (NULL != result)
    {
        dag37d_port_state_t* state = (dag37d_port_state_t*)malloc(sizeof(dag37d_port_state_t));
        state->mIndex = index;
        
        component_set_dispose_routine(result, dag37d_port_dispose);
        component_set_post_initialize_routine(result, dag37d_port_post_initialize);
        component_set_reset_routine(result, dag37d_port_reset);
        component_set_default_routine(result, dag37d_port_default);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        component_set_description(result, "Port Component");
        component_set_private_state(result, state);
        return result;
    }
    return result;
}

static void
dag37d_port_dispose(ComponentPtr component)
{
}

static void
dag37d_port_reset(ComponentPtr component)
{
}

static void
dag37d_port_default(ComponentPtr component)
{
    uint32_t val = 0;
    DagCardPtr card = NULL;
    dag37d_port_state_t * port_state = NULL;
    card = component_get_card(component);
    port_state = component_get_private_state(component);
    port_state->mBase = card_get_register_address(card, DAG_REG_RAW_MAXDS3182, 0);

    /* Retain for testing purposes */
    //uint32_t temp_val = 0, pattern_val, mask_val;

    val = 0;	/* Note: BIT4 is 0, which means that the performance registers are updated per-port not globally */
    if(port_state->mIndex == 0) 
        dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_1_A, val);
    else
        dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_1_B, val);
			
	/* Enable LIU/JA */
	val = BIT0 | BIT1;
    if(port_state->mIndex == 0) /* add 1 to address to get value can only reay 8bit values */
	    dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_2_A+1, val);
    else
	    dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_2_B+1, val);

	/* Select framing mode (ds3_cbit)*/
    if(port_state->mIndex == 0)
    {
	    val = dag37d_iom_read(card, port_state->mBase, D3182_PORT_CONTROL_2_A);
	    val &= ~(BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
	    dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_2_A, val);
    }
    else
    {
	    val = dag37d_iom_read(card, port_state->mBase, D3182_PORT_CONTROL_2_B);
	    val &= ~(BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
	    dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_2_B, val);
    }

	/* reset fifos */
	val = 0;
    if(port_state->mIndex == 0)
	    dag37d_iom_write(card, port_state->mBase, D3182_FIFO_RECEIVE_CONTROL_A, val);
    else
    	dag37d_iom_write(card, port_state->mBase, D3182_FIFO_RECEIVE_CONTROL_B, val);

	/* disable ais */
    /* add 1 to address to get value can only reay 8bit values */
    val = BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2;
    if(port_state->mIndex == 0)
	    dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_1_A+1, val);
    else
	dag37d_iom_write(card, port_state->mBase, D3182_PORT_CONTROL_1_B+1, val);

	/* set port address */
    if(port_state->mIndex == 0)
    {
	    val = 0;
	    dag37d_iom_write(card, port_state->mBase, D3182_FIFO_RECEIVE_PORT_ADDR_A, val);
    }
    else
    {
	    val = 1;
	    dag37d_iom_write(card, port_state->mBase, D3182_FIFO_RECEIVE_PORT_ADDR_B,val);
    }
        
	
    if(port_state->mIndex == 0)
    {
	    val = 0;
	    dag37d_iom_write(card, port_state->mBase, D3182_FIFO_TRANSMIT_PORT_ADDR_A, val);
    }
    else
    {
	    val = 1;
	    dag37d_iom_write(card, port_state->mBase, D3182_FIFO_TRANSMIT_PORT_ADDR_B, val);
    }

    /* Configure cell processor, first do a read, then write a value */
    if(port_state->mIndex == 0)
    {
        val = dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_A + 1) << 8;
        val |= dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_A);
    }
    else
    {
        val = dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_B + 1) << 8;
	val |= dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_B);
    }

    /* Notes: */
    /* Remember that iom reads/writes are only 8 bits wide! */
    /* Add BIT12 and BIT11 for header pattern matching, consult DS3182 datasheet for bit combinations */
    /* Add BIT10 to enable idle cell pass-through. By default BIT10 is reset, which means that idle cells are discarded */
    /* Add BIT9 to enable filtering of unassigned cells */
    /* Add BIT8 to enable filtering of invalid cells */
    val = BIT1 | BIT2 | BIT4;
    if(port_state->mIndex == 0)
    {
        dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_A + 1,val >> 8);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_A, val);
    }
    else
    {
        dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_B + 1, val >> 8);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_B, val);
    }

#if 0 /* Retain for testing purposes */
    if(port_state->mIndex == 0)
    {
	temp_val = dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_A + 1) << 8;
	temp_val |= dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_A);
	printf ("\nDefault: Cell control register: 0x%x, port: %d\n", temp_val, port_state->mIndex);
    }
    else
    {
	temp_val = dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_B + 1) << 8;
	temp_val |= dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_CONTROL_B);
	printf ("\nDefault: Cell control register: 0x%x, port: %d\n", temp_val, port_state->mIndex);
    }
#endif


#if 0	/* Header pattern matching added for testing purposes */
    pattern_val = 0xFF;
    mask_val = 0x00;
    if(port_state->mIndex == 0)
    {
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_A, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_A + 1, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_A, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_A + 1, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_1_A, mask_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_1_A + 1, mask_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_2_A, mask_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_2_A + 1, mask_val);
	temp_val = dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_A + 1) << 24;
	temp_val |= (dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_A) << 16);
	temp_val |= (dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_A + 1) << 8);
	temp_val |= dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_A);
	printf ("\nDefault: Header pattern control register: 0x%x, port: %d\n", temp_val, port_state->mIndex);
    }
    else
    {
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_B, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_B + 1, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_B, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_B + 1, pattern_val);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_1_B, 0);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_1_B + 1, 0);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_2_B, 0);
	dag37d_iom_write(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_MASK_2_B + 1, 0);
	temp_val = dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_B + 1) << 24;
	temp_val |= (dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_B) << 16);
	temp_val |= (dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_B + 1) << 8);
	temp_val |= dag37d_iom_read(card, port_state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_B);
	printf ("\nDefault: Header pattern control register: 0x%x, port: %d\n", temp_val, port_state->mIndex);
    }
#endif

    /* configure transmit packet processor */
    /* put transmit fifos in reset */
    val = BIT1;
    if(port_state->mIndex ==  0)
        dag37d_iom_write(card, port_state->mBase, D3182_FIFO_TRANSMIT_CONTROL_A, val);
    else
        dag37d_iom_write(card, port_state->mBase, D3182_FIFO_TRANSMIT_CONTROL_B, val);

    if(port_state->mIndex == 0)
    {
        val = 0;
        dag37d_iom_write(card, port_state->mBase, D3182_FIFO_TRANSMIT_PORT_ADDR_A,val);
    }
    else
    {
        val = 1;
        dag37d_iom_write(card, port_state->mBase, D3182_FIFO_TRANSMIT_PORT_ADDR_B, val);
    }

    val = BIT1 | BIT2 | BIT3 | BIT5;
    if(port_state->mIndex == 0)
        dag37d_iom_write(card, port_state->mBase, D3182_CP_TRANSMIT_CONTROL_A, val);
    else
        dag37d_iom_write(card, port_state->mBase, D3182_CP_TRANSMIT_CONTROL_B, val);

    /* throw them out of reset */
    val = 0;
    if(port_state->mIndex == 0)
        dag37d_iom_write(card, port_state->mBase,D3182_FIFO_TRANSMIT_CONTROL_A, val);
    else
        dag37d_iom_write(card, port_state->mBase, D3182_FIFO_TRANSMIT_CONTROL_B,val);
    
}

static int
dag37d_port_post_initialize(ComponentPtr component)
{
    dag37d_port_state_t * port_state = NULL;
    DagCardPtr card = NULL;
    GenericReadWritePtr grw = NULL;
    uintptr_t address = 0;
    uint32_t bool_attr_mask = 0;
    uintptr_t gpp_base = 0;
    AttributePtr attr = NULL;
    dag37d_state_t * card_state = NULL;

    port_state = component_get_private_state(component);
    card = component_get_card(component);
    card_state = (dag37d_state_t *)card_get_private_state(card);
    port_state->mBase = card_state->mPhyBase;


    /* Default configuration modes */
    attr = get_new_defaultds3atm();
    component_add_attribute(component, attr);
    attr = get_new_defaultds3hdlc();
    component_add_attribute(component, attr);
    attr = get_new_defaulte3atm();
    component_add_attribute(component, attr);
    attr = get_new_defaulte3hdlc();
    component_add_attribute(component, attr);
    attr = get_new_defaultkentrox();
    component_add_attribute(component, attr);

    /* EQL */
/* add 1 to address to get value can only reay 8bit values */
    bool_attr_mask = BIT2;
    if(port_state->mIndex == 0)
        address = D3182_PORT_CONTROL_4_A+1;
    else
        address = D3182_PORT_CONTROL_4_B+1;
    
    grw = grw_init(card, address, grw_dag37d_read, grw_dag37d_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEquipmentLoopback, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    

    /* Discard */
    bool_attr_mask = BIT3;
    if(port_state->mIndex == 0)
        address = D3182_CP_RECEIVE_CONTROL_A;
    else
        address = D3182_CP_RECEIVE_CONTROL_B;
    
    grw = grw_init(card, address, grw_dag37d_read, grw_dag37d_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLinkDiscard, grw, &bool_attr_mask, 1);
    /* use local get-value and set-value functions,which checks for the mode(atm or hdlc) */
    attribute_set_setvalue_routine(attr, dag37d_discard_set_value);
    attribute_set_getvalue_routine(attr, dag37d_discard_get_value);
    component_add_attribute(component,attr);
    
    /* crcselect */
    attr = get_new_crc();
    component_add_attribute(component, attr);

    /* fcl */
    attr = get_new_fcl();
    component_add_attribute(component, attr);


    /* ds3_cbit */
    attr = get_new_framingmode();
    component_add_attribute(component,attr);
    
    /*payload type */
    attr = get_new_networkmode();
    component_add_attribute(component, attr);

    /*rx monitor mode */
    attr = get_new_rxmonitormode();
    component_add_attribute(component, attr);

    /*payload scramble*/
    attr = get_new_pscramble();
    component_add_attribute(component,attr);

    /*descramble*/
    attr = get_new_descramble();
    component_add_attribute(component,attr);

#if 0
    /*cell header descramble*/
    attr = get_new_cellheaderdescramble();
    component_add_attribute(component,attr);
#endif

    /*ff00 delete*/
    attr = get_new_ff00del();
    component_add_attribute(component,attr);

    /*e3 hdlc fraction*/
    attr = get_new_hdlcfract();
    component_add_attribute(component,attr);

    /* Status Attributes */
    /*rlol*/
    attr = get_new_rxlol();
    component_add_attribute(component,attr);
    
    /*los*/
    bool_attr_mask = BIT0;
    if(port_state->mIndex == 0)
        address = D3182_LINE_RECEIVE_STATUS_A;
    else
        address = D3182_LINE_RECEIVE_STATUS_B;
    
    grw = grw_init(card, address, grw_dag37d_read, grw_dag37d_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfSignal, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);
    /*lof*/
    bool_attr_mask = BIT4;
    if(port_state->mIndex == 0)
        address = D3182_T3_RECEIVE_STATUS_A;
    else
        address = D3182_T3_RECEIVE_STATUS_B;
    
    grw = grw_init(card, address, grw_dag37d_read, grw_dag37d_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFrame, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);    
    
    /*rdi*/
    attr = get_new_rdi();
    component_add_attribute(component,attr);
    
    /*rdi*/
    attr = get_new_aic_m23_cbit();
    component_add_attribute(component,attr);
    
    /*ais*/
    attr = get_new_ais();
    component_add_attribute(component,attr);

    /*oof*/
    bool_attr_mask = BIT1;
    if(port_state->mIndex == 0)
        address = D3182_T3_RECEIVE_STATUS_A;
    else
        address = D3182_T3_RECEIVE_STATUS_B;
    
    grw = grw_init(card, address, grw_dag37d_read, grw_dag37d_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeOutOfFrame, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);    

    /* Latch and Clear */
    attr = get_new_latch_and_clear();
    component_add_attribute(component, attr);

    /* RX Frames */
    attr = get_new_rx_frames();
    component_add_attribute(component, attr);

    /* Rx Bytes */
    attr = get_new_rx_bytes();
    component_add_attribute(component, attr);
    
    /* TX Frames */
    attr = get_new_tx_frames();
    component_add_attribute(component, attr);
    
    /* TX Bytes */
    attr = get_new_tx_bytes();
    component_add_attribute(component, attr);

    /* drop count */                                    
    gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
    bool_attr_mask = 0xffffffff;
    address = ((uintptr_t)card_get_iom_address(card)) + gpp_base + (uintptr_t)((port_state->mIndex + 1)*SP_OFFSET) + (uintptr_t)SP_DROP;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeDropCount, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    
    return 1;
}



static AttributePtr
get_new_defaultds3atm(void)
{
    AttributePtr result = attribute_init(kNullAttributeDefaultDS3ATM);

    if (NULL != result)
    {
        attribute_set_name(result, "default_ds3_atm");
        attribute_set_description(result, "Configure the card into DS3 framed full line rate ATM mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, defaultds3atm_to_string);
        attribute_set_from_string_routine(result, defaultds3atm_from_string);
    }
    return result;
}

static void 
defaultds3atm_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
defaultds3atm_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
get_new_defaultds3hdlc(void)
{
    AttributePtr result = attribute_init(kNullAttributeDefaultDS3HDLC);

    if (NULL != result)
    {
        attribute_set_name(result, "default_ds3_hdlc");
        attribute_set_description(result, "Configure the card into DS3 framed full line rate HDLC mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, defaultds3hdlc_to_string);
        attribute_set_from_string_routine(result, defaultds3hdlc_from_string);
    }
    return result;
}

static void 
defaultds3hdlc_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
defaultds3hdlc_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
get_new_defaulte3atm(void)
{
    AttributePtr result = attribute_init(kNullAttributeDefaultE3ATM);

    if (NULL != result)
    {
        attribute_set_name(result, "default_e3_atm");
        attribute_set_description(result, "Configure the card into E3 framed full line rate ATM mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, defaulte3atm_to_string);
        attribute_set_from_string_routine(result, defaulte3atm_from_string);
    }
    return result;
}

static void 
defaulte3atm_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
defaulte3atm_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
get_new_defaulte3hdlc(void)
{
    AttributePtr result = attribute_init(kNullAttributeDefaultE3HDLC);

    if (NULL != result)
    {
        attribute_set_name(result, "default_e3_hdlc");
        attribute_set_description(result, "Configure the card into E3 framed full line rate HDLC mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, defaulte3hdlc_to_string);
        attribute_set_from_string_routine(result, defaulte3hdlc_from_string);
    }
    return result;
}

static void 
defaulte3hdlc_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
defaulte3hdlc_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

static AttributePtr
get_new_defaultkentrox(void)
{
    AttributePtr result = attribute_init(kNullAttributeDefaultKentrox);

    if (NULL != result)
    {
        attribute_set_name(result, "default_kentrox");
        attribute_set_description(result, "Configure the card into Kentrox mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        attribute_set_to_string_routine(result, defaultkentrox_to_string);
        attribute_set_from_string_routine(result, defaultkentrox_from_string);
    }
    return result;
}

static void 
defaultkentrox_to_string(AttributePtr attribute)
{
    /* not implemented */
    return;
}
static void
defaultkentrox_from_string(AttributePtr attribute, const char * string)
{
    /* Not Implented */
    return;
}

/* ------------------------------------------------------------------------------------ */

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
        attribute_set_description(result, "Latch the framer counters and clear them.");
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
    dag37d_port_state_t* state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
	if (state->mIndex == 0)	/* Port A */
	{
		/* Set the PMU bit */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_A);
		register_val |= *(uint32_t*)value << 3; /* BIT3 */
		//register_val |= BIT3;
		dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_A, register_val);

		/* After setting the PMU bit, check that the PMS bit has been set */
		/* Note that the PMU registers are in per port update mode NOT global update mode (as set in dag37d_port_default()) */
		while (1)
		{
			/* Check the PMS bit */
			register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_STATUS_A);
			if ((register_val & BIT0) == BIT0)
			{
				/* Reset the PMU bit */
				register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_A);
				register_val &= ~BIT3;
				dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_A, register_val);

				break;
			}
		}
	}
	else /* Port B */
	{
		/* Set the PMU bit */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_B);
		register_val |= *(uint32_t*)value << 3;	/* BIT3 */
		//register_val |= BIT3;
		dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_B, register_val);

		/* After setting the PMU bit, check that the PMS bit has been set */
		/* Note that the PMU registers are in per port update mode NOT global update mode (as set in dag37d_port_default()) */
		while (1)
		{
			/* Check the PMS bit */
			register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_STATUS_B);
			if ((register_val & BIT0) == BIT0)
			{
				/* Reset the PMU bit */
				register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_B);
				register_val &= ~BIT3;
				dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_B, register_val);

				break;
			}
		}
	}
    }
}

static void*
latch_and_clear_get_value(AttributePtr attribute)
{
    uint8_t value = 0;
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t* state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_val;

    if (1 == valid_attribute(attribute))
    {
	if (state->mIndex == 0)	/* Port A */
	{
		/* Set the PMU bit */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_A);
		register_val |= BIT3;
		dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_A, register_val);

		/* After setting the PMU bit, check that the PMS bit has been set */
		/* Note that the PMU registers are in per port update mode NOT global update mode (as set in dag37d_port_default()) */
		while (1)
		{
			/* Check the PMS bit */
			register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_STATUS_A);
			if ((register_val & BIT0) == BIT0)
			{
				/* Reset the PMU bit */
				register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_A);
				register_val &= ~BIT3;
				dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_A, register_val);

				break;
			}
		}
	}
	else /* Port B */
	{
		/* Set the PMU bit */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_B);
		register_val |= BIT3;
		dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_B, register_val);

		/* After setting the PMU bit, check that the PMS bit has been set */
		/* Note that the PMU registers are in per port update mode NOT global update mode (as set in dag37d_port_default()) */
		while (1)
		{
			/* Check the PMS bit */
			register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_STATUS_B);
			if ((register_val & BIT0) == BIT0)
			{
				/* Reset the PMU bit */
				register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_1_B);
				register_val &= ~BIT3;
				dag37d_iom_write(card, state->mBase, D3182_PORT_CONTROL_1_B, register_val);

				break;
			}
		}
	}
    }

    attribute_set_value_array(attribute, &value, sizeof(value));
    return (void *)attribute_get_value_array(attribute);
}

/* ------------------------------------------------------------------------------------ */

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
        attribute_set_description(result, "Number of transmitted frames.");
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
    ComponentPtr comp;
    uint32_t register_val;
    uint32_t val = 0;
    dag37d_port_state_t* state = NULL;
    uint32_t loc = 0;
    
    if (1 == valid_attribute(attribute))
    {
	card = attribute_get_card(attribute);
	comp = attribute_get_component(attribute);
	state = (dag37d_port_state_t*)component_get_private_state(comp);
	
	if (state->mIndex == 0)	/* Port A */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_A);
		if ((register_val & BIT6) == BIT6) /* ATM */
		{
			loc = D3182_CP_TRANSMIT_CELL_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
		else /* HDLC */
		{
			loc = D3182_PP_TRANSMIT_PACKET_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_PP_TRANSMIT_PACKET_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_TRANSMIT_PACKET_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}
	else /* Port B */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_B);
		if((register_val & BIT6) == BIT6) /* ATM */
		{
			loc = D3182_CP_TRANSMIT_CELL_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
		else /* HDLC */
		{
			loc = D3182_PP_TRANSMIT_PACKET_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_PP_TRANSMIT_PACKET_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_TRANSMIT_PACKET_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}

	val = register_val;
    attribute_set_value_array(attribute, &val, sizeof(val));
    return (void *)attribute_get_value_array(attribute);;
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------ */

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
        attribute_set_description(result, "Number of received frames.");
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
    ComponentPtr comp;
    uint32_t register_val;
    uint32_t val = 0;
    dag37d_port_state_t* state = NULL;
    uint32_t loc = 0;

    /* Retain for testing purposes */
    //uint32_t reg_val_filter, temp_val, pattern_count_val;

    if (1 == valid_attribute(attribute))
    {
	card = attribute_get_card(attribute);
	comp = attribute_get_component(attribute);
	state = (dag37d_port_state_t*)component_get_private_state(comp);
	
	if (state->mIndex == 0)	/* Port A */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_A);
		if ((register_val & BIT6) == BIT6) /* ATM */
		{
			loc = D3182_CP_RECEIVE_CELL_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_CP_RECEIVE_CELL_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_RECEIVE_CELL_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);

#if 0 /* Retain for testing purposes */
			reg_val_filter = dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_FILTERED_CELL_COUNT_2_A) << 16;
			reg_val_filter |= (dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_FILTERED_CELL_COUNT_1_A + 1) << 8);
			reg_val_filter |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_FILTERED_CELL_COUNT_1_A);
			printf ("\nReceive filtered cell count: %d, port: %d\n", reg_val_filter, state->mIndex);

       			temp_val = dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_CONTROL_A + 1) << 8;
       			temp_val |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_CONTROL_A);
			printf ("\nCell control register: 0x%x, port: %d\n", temp_val, state->mIndex);

			temp_val = dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_A + 1) << 24;
			temp_val |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_A) << 16;
			temp_val |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_A + 1) << 8;
			temp_val |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_A);
			printf ("\nHeader pattern control register: 0x%x, port: %d\n", temp_val, state->mIndex);

			pattern_count_val = dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_2_A + 1) << 24;
			pattern_count_val |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_2_A) << 16;
			pattern_count_val |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_1_A + 1) << 8;
			pattern_count_val |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_1_A);
			printf ("\nHeader pattern cell count: %d, port: %d\n", pattern_count_val, state->mIndex);
#endif

		}
		else /* HDLC */
		{
			loc = D3182_PP_RECEIVE_PACKET_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_PP_RECEIVE_PACKET_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_RECEIVE_PACKET_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}
	else /* Port B */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_B);
	        if((register_val & BIT6) == BIT6) /* ATM */
		{
	       		loc = D3182_CP_RECEIVE_CELL_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
        		loc = D3182_CP_RECEIVE_CELL_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_RECEIVE_CELL_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);

#if 0 /* Retain for testing purposes */
			reg_val_filter = dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_FILTERED_CELL_COUNT_2_B) << 16;
			reg_val_filter |= (dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_FILTERED_CELL_COUNT_1_B + 1) << 8);
			reg_val_filter |= dag37d_iom_read(card, state->mBase, D3182_CP_RECEIVE_FILTERED_CELL_COUNT_1_B);
			printf ("\nReceive filtered cell count: %d, port: %d\n", reg_val_filter, state->mIndex);
#endif

		}
		else /* HDLC */
		{
        		loc = D3182_PP_RECEIVE_PACKET_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
	        	loc = D3182_PP_RECEIVE_PACKET_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_RECEIVE_PACKET_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}

        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------ */

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
        attribute_set_description(result, "Number of transmitted bytes.");
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
    ComponentPtr comp;
    uint64_t register_val;
    uint64_t val = 0;
    dag37d_port_state_t* state = NULL;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
	card = attribute_get_card(attribute);
	comp = attribute_get_component(attribute);
	state = (dag37d_port_state_t*)component_get_private_state(comp);
	
	if (state->mIndex == 0)	/* Port A */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_A);
		if ((register_val & BIT6) == BIT6) /* ATM */
		{
			loc = D3182_CP_TRANSMIT_CELL_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
			
			register_val *= 53;
		}
		else /* HDLC */
		{
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_2_A + 1;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 24;
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}
	else /* Port B */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_B);
		if((register_val & BIT6) == BIT6) /* ATM */
		{
			loc = D3182_CP_TRANSMIT_CELL_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_TRANSMIT_CELL_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
				
			register_val *= 53;
		}
		else /* HDLC */
		{
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_2_B + 1;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 24;
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_TRANSMIT_BYTE_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}

	val = register_val;
    attribute_set_value_array(attribute, &val, sizeof(val));
    return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------ */

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
        attribute_set_description(result, "Number of received bytes.");
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
    ComponentPtr comp;
    uint64_t register_val;
    uint64_t val = 0;
    dag37d_port_state_t* state = NULL;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
	card = attribute_get_card(attribute);
	comp = attribute_get_component(attribute);
	state = (dag37d_port_state_t*)component_get_private_state(comp);
	
	if (state->mIndex == 0)	/* Port A */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_A);
		if ((register_val & BIT6) == BIT6) /* ATM */
		{
			loc = D3182_CP_RECEIVE_CELL_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_CP_RECEIVE_CELL_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_RECEIVE_CELL_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
				
			register_val *= 53;
		}
		else /* HDLC */
		{
			loc = D3182_PP_RECEIVE_BYTE_COUNT_2_A + 1;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 24;
			loc = D3182_PP_RECEIVE_BYTE_COUNT_2_A;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_PP_RECEIVE_BYTE_COUNT_1_A + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_RECEIVE_BYTE_COUNT_1_A;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}
	else /* Port B */
	{
		/* Check the PMCPE bit to determine whether cell/packet processing is performed */
		register_val = dag37d_iom_read(card, state->mBase, D3182_PORT_CONTROL_2_B);
		if((register_val & BIT6) == BIT6) /* ATM */
		{
			loc = D3182_CP_RECEIVE_CELL_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_CP_RECEIVE_CELL_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_CP_RECEIVE_CELL_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
				
			register_val *= 53;
		}
		else /* HDLC */
		{
			loc = D3182_PP_RECEIVE_BYTE_COUNT_2_B + 1;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 24;
			loc = D3182_PP_RECEIVE_BYTE_COUNT_2_B;
			register_val = dag37d_iom_read(card, state->mBase, loc) << 16;
			loc = D3182_PP_RECEIVE_BYTE_COUNT_1_B + 1;
			register_val |= (dag37d_iom_read(card, state->mBase, loc) << 8);
			loc = D3182_PP_RECEIVE_BYTE_COUNT_1_B;
			register_val |= dag37d_iom_read(card, state->mBase, loc);
		}
	}	

	val = register_val;
    attribute_set_value_array(attribute, &val, sizeof(val));
    return (void *)attribute_get_value_array(attribute);

    }
    return NULL;	
}

/* ------------------------------------------------------------------------------------ */

static AttributePtr
get_new_fcl(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFacilityLoopback);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, fcl_get_value);
        attribute_set_setvalue_routine(result, fcl_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "fcl");
        attribute_set_description(result, "Enable or disable facility loopback");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}
static void*
fcl_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {/* add 1 to address to get value can only reay 8bit values */
        if(state->mIndex == 0)
            loc = D3182_PORT_CONTROL_4_A+1;
        else
            loc = D3182_PORT_CONTROL_4_B+1;

        register_value = dag37d_iom_read(card, state->mBase, loc);

        if((register_value &  (BIT0 | BIT1)) == BIT1)
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
fcl_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {/* add 1 to address to get value can only reay 8bit values */
        if(state->mIndex == 0)
            loc = D3182_PORT_CONTROL_4_A+1;
        else
            loc = D3182_PORT_CONTROL_4_B+1;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(*(uint8_t*)value == 0) 
        {
            register_value &= ~BIT1;
         } 
        else 
        {
             register_value &= ~(BIT1 | BIT0);
             register_value |= BIT1;
         }
         dag37d_iom_write(card, state->mBase, loc, register_value);
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
        attribute_set_name(result, "crc_select");
        attribute_set_description(result, "Select the crc.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, crc_to_string_routine);
        attribute_set_from_string_routine(result, crc_from_string_routine);
	
    }

    return result;
}

static void
crc_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_value;
    dag37d_port_state_t* state = NULL;
    crc_t crc_value = *(crc_t*)value;
    uint32_t loc = 0;
    AttributePtr network_mode_attr = kNullAttributeUuid;
    network_mode_t *ptr_network_mode = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        network_mode_attr = component_get_attribute(port, kUint32AttributeNetworkMode);
        /* check for the modes and set the value only for hdlc mode */
        if ( NULL != (ptr_network_mode = (network_mode_t*)attribute_get_value(network_mode_attr) ) && (*ptr_network_mode != kNetworkModeHDLC)  )
            return;
        state = (dag37d_port_state_t *)component_get_private_state(port);
        if(state->mIndex == 0)
            loc = D3182_CP_RECEIVE_CONTROL_A;
        else
            loc = D3182_CP_RECEIVE_CONTROL_B;
            
        register_value = dag37d_iom_read(card,state->mBase, loc);
        switch(crc_value)
        {
            case kCrcOff:
                register_value |= BIT5;
                break;
            case kCrc16:
                register_value &= ~(BIT5 | BIT4);
                register_value |= BIT4;
                break;
            case kCrc32:
                register_value &= ~(BIT5|BIT4);
                break;
            default:
                break;
        }
        dag37d_iom_write(card, state->mBase, loc, register_value);
    }
}
static void*
crc_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_value;
    dag37d_port_state_t* state = NULL;
    crc_t value = kCrcOff;
    uint32_t loc = 0;
    AttributePtr network_mode_attr = kNullAttributeUuid;
    network_mode_t *ptr_network_mode = NULL;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        
        network_mode_attr = component_get_attribute(port, kUint32AttributeNetworkMode);
        /* check for the modes and get the value only for hdlc mode */
        if ( NULL != (ptr_network_mode = (network_mode_t*)attribute_get_value(network_mode_attr) ) && (*ptr_network_mode == kNetworkModeHDLC)  )
        {
            state = (dag37d_port_state_t *)component_get_private_state(port);
            if(state->mIndex == 0)
                loc = D3182_CP_RECEIVE_CONTROL_A;
            else
                loc = D3182_CP_RECEIVE_CONTROL_B;
            register_value = dag37d_iom_read(card, state->mBase, loc);
        
            if((register_value & BIT5) == BIT5)
            {
                value = kCrcOff;
            }
            else if((register_value & BIT4) == BIT4)
            {
            value = kCrc16;
            }
            else
            {
                value = kCrc32;
            }
        }
    }
    attribute_set_value_array(attribute, &value, sizeof(value));
    return (void *)attribute_get_value_array(attribute);
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
        attribute_set_description(result, "Enable or disable payload scrambling");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
pscramble_get_value(AttributePtr attribute)
{
	return descramble_get_value(attribute);
}

static void
pscramble_set_value(AttributePtr attribute, void* value, int length)
{
	descramble_set_value(attribute, value, length);
}

static AttributePtr
get_new_descramble(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeDescramble);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, descramble_get_value);
        attribute_set_setvalue_routine(result, descramble_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "descramble");
        attribute_set_description(result, "Enable or disable receive descrambling");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
descramble_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_CP_RECEIVE_CONTROL_A;
        else
            loc = D3182_CP_RECEIVE_CONTROL_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(register_value & BIT2 )
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
descramble_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_CP_RECEIVE_CONTROL_A;
        else
            loc = D3182_CP_RECEIVE_CONTROL_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(*(uint8_t*)value == 0) 
        {
            register_value |= BIT2;
         } 
        else 
        {
            register_value &= ~BIT2;
        }
        dag37d_iom_write(card, state->mBase, loc, register_value);
    }
}

void*
dag37d_discard_get_value(AttributePtr attribute)
{
    ComponentPtr component = NULL;
    AttributePtr network_mode_attr = kNullAttributeUuid;
    network_mode_t *ptr_network_mode = NULL;

    component = attribute_get_component(attribute);
    if ( NULL == component)
    {
        dagutil_warning("Invalid component for discard attribute\n");
        return NULL;
    }
    network_mode_attr = component_get_attribute(component, kUint32AttributeNetworkMode);
    /* check for the modes and apply the getvalue function only for hdlc mode */
    if ( NULL != (ptr_network_mode = (network_mode_t*)attribute_get_value(network_mode_attr) ) && (*ptr_network_mode == kNetworkModeHDLC)  )
    {
        /* use the function as initialized in 'attribute factory' with the 37d specifc grw */
        return attribute_boolean_get_value( attribute);
    }
    else
    {
        /* return 0 ( nodiscard ) for ATM */
        uint8_t value = 0;
        attribute_set_value_array(attribute, &value, sizeof(uint8_t));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

void
dag37d_discard_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr component = NULL;
    AttributePtr network_mode_attr = kNullAttributeUuid;
    network_mode_t *ptr_network_mode = NULL;

    component = attribute_get_component(attribute);
    if ( NULL == component)
    {
        dagutil_warning("Invalid component for discard attribute\n");
        return ;
    }
    network_mode_attr = component_get_attribute(component, kUint32AttributeNetworkMode);
    /* check for the modes and apply the setvalue function only for hdlc mode */
    if ( NULL != (ptr_network_mode = (network_mode_t*)attribute_get_value(network_mode_attr) ) && (*ptr_network_mode == kNetworkModeHDLC)  )
    {
        /* use the function as initialized in 'attribute factory' with the 37d specifc grw */
        attribute_boolean_set_value( attribute, value, length );
    }
    else
    {
        /* do nothing for ATM or invalid modes */
    }
    return ;
}
#if 0
static AttributePtr
get_new_cellheaderdescramble(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeCellHeaderDescramble);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, cellheaderdescramble_get_value);
        attribute_set_setvalue_routine(result, cellheaderdescramble_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "cell_header_descramble");
        attribute_set_description(result, "Enable or disable receive cell header descrambling. If enabled, entire data stream (cell header and payload) is descrambled. If disabled, only payload is descrambled.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;

}

static void*
cellheaderdescramble_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_CP_RECEIVE_CONTROL_A;
        else
            loc = D3182_CP_RECEIVE_CONTROL_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(register_value & BIT3 )
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
cellheaderdescramble_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_CP_RECEIVE_CONTROL_A;
        else
            loc = D3182_CP_RECEIVE_CONTROL_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(*(uint8_t*)value == 0) 
        {
            register_value |= BIT3;
         } 
        else 
        {
            register_value &= ~BIT3;
        }
        dag37d_iom_write(card, state->mBase, loc, register_value);
    }
}
#endif

static AttributePtr
get_new_ff00del(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeFF00Delete);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, ff00del_get_value);
        attribute_set_setvalue_routine(result, ff00del_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "ff00_del");
        attribute_set_description(result, "Enable deletion of extra bits inserted by some routers (e.g. Cisco) when receiving E3 HDLC.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
ff00del_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)	// Port A
        {
            if ((dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A) == 0x0)
             && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A+1) == 0x0)
             && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_A) == 0x0))
                value = 0;
            else if ((dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A) == 0xf4)
                  && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A+1) == 0x5)
                  && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_A) == 0x4))
                value = 1;
        }
        else	// Port B
        {
            if ((dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B) == 0x0)
             && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B+1) == 0x0)
             && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_B) == 0x0))
                value = 0;
            else if ((dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B) == 0xf4)
                  && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B+1) == 0x5)
                  && (dag37d_iom_read(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_B) == 0x4))
                value = 1;
        }

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
ff00del_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);

    if (1 == valid_attribute(attribute))
    {
        if(*(uint8_t*)value == 0) 
        {
            if(state->mIndex == 0)	// Port A
            {
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A, 0x0);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A+1, 0x0);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_A, 0x0);
            }
            else	// Port B
            {
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B, 0x0);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B+1, 0x0);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_B, 0x0);
            }
        }
        else
        {
            if(state->mIndex == 0)	// Port A
            {
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A, 0xf4);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A+1, 0x5);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_A, 0x4);
            }
            else	// Posrt B
            {
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B, 0xf4);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B+1, 0x5);
                dag37d_iom_write(card, state->mBase, D3182_FRACT_RECEIVE_SECTION_A_SIZE_B, 0x4);
            }
        }
    }
}


static AttributePtr
get_new_hdlcfract(void)
{
    AttributePtr result = attribute_init(kUint32AttributeHDLCFraction);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, hdlcfract_get_value);
        attribute_set_setvalue_routine(result, hdlcfract_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "hdlc_fract");
        attribute_set_description(result, "A number out of 95 specifying the number of timeslots used by whatever is sending the data. This is used with the fractional E3 HDLC firmware.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    return result;
}

static void*
hdlcfract_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)	// Port A
        {
            register_value = card_read_iom(card, DAG3_REG_FRACT);
            value = register_value & 0x7f;
        }
        else	// Port B
        {
            register_value = card_read_iom(card, DAG3_REG_FRACT);
            value = (register_value >> 8) & 0x7f;
        }

        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
hdlcfract_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t fract = *(uint32_t *)value;

    if (1 == valid_attribute(attribute))
    {
        if (fract > 95 || fract < 0)
        {
            dagutil_warning("!! FRACTION: The number of fraction(s) must be between 0 and 95. Default is 95 !!\n");
            fract = 95;
        }

        if(state->mIndex == 0)	// Port A
        {
            register_value = card_read_iom(card, DAG3_REG_FRACT);
            register_value &= ~(BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
            register_value |= fract;

            card_write_iom(card, DAG3_REG_FRACT, register_value);


        }
        else	// Port B
        {
            register_value = card_read_iom(card, DAG3_REG_FRACT);
            register_value &= ~(BIT14 | BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8);
            register_value |= (fract << 8);

            card_write_iom(card, DAG3_REG_FRACT, register_value);
        }
    }
}

static AttributePtr
get_new_networkmode(void)
{
    AttributePtr result = attribute_init(kUint32AttributeNetworkMode);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, networkmode_get_value);
        attribute_set_setvalue_routine(result, networkmode_set_value);
        attribute_set_name(result, "network_mode");
        attribute_set_description(result, "Select HDLC or ATM mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}

static void*
networkmode_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_value;
    payload_type_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {/* add 1 to address to get value can only reay 8bit values */
        if(state->mIndex == 0)
            loc = D3182_PORT_CONTROL_2_A;
        else
            loc = D3182_PORT_CONTROL_2_B;
        register_value = dag37d_iom_read(card, state->mBase, loc);
        if((register_value & BIT6 ) == BIT6 )
        {
            value = kNetworkModeATM;
        }
        else if((register_value & BIT6) == 0)
        {
            value = kNetworkModeHDLC;
        }
        else
        {
            value = kNetworkModeInvalid;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
networkmode_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t*)component_get_private_state(comp);
    dag37d_state_t * card_state = card_get_private_state(card);
    uint32_t register_value = 0;
    uint32_t loc = 0;
    payload_type_t payload_value = *(payload_type_t*)value;

    if (1 == valid_attribute(attribute))
    {/* add 1 to address to get value can only reay 8bit values */
        if(payload_value == kNetworkModeATM)
        {
            register_value = dag37d_iom_read(card, state->mBase, D3182_GLOBAL_CONTROL_1+1);
            register_value &= ~(BIT0 | BIT1);
            register_value |= BIT1 | BIT0;
            dag37d_iom_write(card, state->mBase, D3182_GLOBAL_CONTROL_1+1, register_value);

            if(state->mIndex == 0)
                loc = D3182_PORT_CONTROL_2_A;
            else
                loc = D3182_PORT_CONTROL_2_B;
            register_value = dag37d_iom_read(card, state->mBase, loc);
            register_value |= BIT6;
            dag37d_iom_write(card, state->mBase, loc, register_value);

            register_value = 0;
            if(state->mIndex == 0)
                loc = D3182_CP_RECEIVE_CONTROL_A;
            else
                loc = D3182_CP_RECEIVE_CONTROL_B;

            dag37d_iom_write(card, state->mBase, loc, register_value);

            /* Set the card into ATM mode, the bit is located in the the interrupt mask register of the General Registers */ 
            /* NOTE: This bit would normally be set in the GPP */
            register_value = card_read_iom(card, card_state->mGeneralBase + 0xc);
            register_value |= BIT17;
            card_write_iom(card, card_state->mGeneralBase + 0xc, register_value);
        }
        else
        {
            register_value = dag37d_iom_read(card, state->mBase, D3182_GLOBAL_CONTROL_1+1);
            register_value |= BIT1 | BIT0;
            dag37d_iom_write(card, state->mBase, D3182_GLOBAL_CONTROL_1+1, register_value);


            if(state->mIndex == 0)
                loc = D3182_PORT_CONTROL_2_A;
            else
                loc = D3182_PORT_CONTROL_2_B;
            register_value = dag37d_iom_read(card, state->mBase, loc);
            register_value &= ~BIT6;
            dag37d_iom_write(card, state->mBase, loc, register_value);

            /* Set the card into HDLC/POS mode, the bit is located in the the interrupt mask register of the General Registers */ 
            /* NOTE: This bit would normally be set in the GPP */
            register_value = card_read_iom(card, card_state->mGeneralBase + 0xc);
            register_value &= ~BIT17;
            card_write_iom(card, card_state->mGeneralBase + 0xc, register_value);
        }
    }
}

static AttributePtr
get_new_rxmonitormode(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRXMonitorMode);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rxmonitormode_get_value);
        attribute_set_setvalue_routine(result, rxmonitormode_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "rx_monitor_mode");
        attribute_set_description(result, "RX Monitor Mode");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}
static void*
rxmonitormode_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    { /* add 1 to address to get value can only reay 8bit values */
        if(state->mIndex == 0)
            loc = D3182_PORT_CONTROL_2_A+1;
        else
            loc = D3182_PORT_CONTROL_2_B+1;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if((register_value &  (BIT5)) == BIT5)
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
rxmonitormode_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {/* add 1 to address to get value can only reay 8bit values */
        if(state->mIndex == 0)
            loc = D3182_PORT_CONTROL_2_A+1;
        else
            loc = D3182_PORT_CONTROL_2_B+1;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(*(uint8_t*)value == 0) 
        {
            register_value &= ~BIT5;
         } 
        else 
        {
             register_value |= BIT5;
         }
         dag37d_iom_write(card, state->mBase, loc, register_value);
    }
}

static AttributePtr
get_new_framingmode(void)
{
    AttributePtr result = attribute_init(kUint32AttributeFramingMode);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, framingmode_get_value);
        attribute_set_setvalue_routine(result, framingmode_set_value);
        attribute_set_to_string_routine(result, framing_mode_to_string_routine);
        attribute_set_from_string_routine(result, framing_mode_from_string_routine);
        attribute_set_name(result, "framing_mode");
        attribute_set_description(result, "Framing Mode");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }

    return result;
}
static void
framing_mode_to_string_routine(AttributePtr attribute)
{
    void* temp = attribute_get_value(attribute);
    const char* string = NULL;
    framing_mode_t fm;
    if (temp)
    {
        fm = *(framing_mode_t*)temp;
        string = framing_mode_to_string(fm);
        if (string)
            attribute_set_to_string(attribute, string);
    }
}

static void
framing_mode_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        framing_mode_t mode = string_to_framing_mode(string);
        framingmode_set_value(attribute, (void*)&mode, sizeof(mode));
    }
}


static void*
framingmode_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * port_state = (dag37d_port_state_t*)component_get_private_state(comp);
    uint32_t register_value;
    framing_mode_t value;
    uint32_t loc = 0;
    uint32_t mask = 0x3f;

    if (1 == valid_attribute(attribute))
    {
        if(port_state->mIndex == 0)
            loc = D3182_PORT_CONTROL_2_A;
        else
            loc = D3182_PORT_CONTROL_2_B;

        register_value = dag37d_iom_read(card, port_state->mBase, loc);
        switch(register_value & mask)
        {
            case 0:
                value = kFramingModeDs3Cbit;
                break;
            case BIT1:
                value = kFramingModeDs3CbitIF;
                break;
            case (BIT1|BIT0):
                value = kFramingModeDs3CbitEF;
                break;
            case (BIT2|BIT1):
                value = kFramingModeDs3CbitFF;
                break;
            case BIT3:
                value = kFramingModeDs3m23;
                break;
            case (BIT3 |BIT1):
                value = kFramingModeDs3m23IF;
                break;
            case (BIT3|BIT1|BIT0):
                value = kFramingModeDs3m23EF;
                break;
            case (BIT3|BIT2|BIT1):
                value = kFramingModeDs3m23FF;
                break;
            case BIT4:
                value = kFramingModeE3;
                break;
            case (BIT4|BIT2):
                value = kFramingModeE3G751PLCP;
                break;
            case (BIT4|BIT1):
                value = kFramingModeE3G751IF;
                break;
            case BIT2:
                value = kFramingModeDS3CbitPLCP;
                break;
            case (BIT2|BIT3):
                value = kFramingModeDS3M23PLCP;
                break;
            case (BIT5|BIT4):
                value = kFramingModeE3CC;
                break;

            default:
                value = kFramingModeInvalid;
                break;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
framingmode_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * port_state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value = 0;
    uint32_t loc = 0;
    framing_mode_t framing_value = *(framing_mode_t*)value;

    if (1 == valid_attribute(attribute))
    {
        if(port_state->mIndex == 0)
            loc = D3182_PORT_CONTROL_2_A;
        else
            loc = D3182_PORT_CONTROL_2_B;
        
        register_value = dag37d_iom_read(card, port_state->mBase, loc);
        register_value &= ~(BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
        switch(framing_value)
        {
            case kFramingModeDs3Cbit:
                /* leave all bits cleared */
                break;
            case kFramingModeDs3CbitIF:
                register_value |= BIT1;
                break;
            case kFramingModeDs3CbitFF:
                register_value |= BIT1 | BIT2;
                break;
            case kFramingModeDs3CbitEF:
                register_value |= BIT1|BIT0;
                break;
            case kFramingModeDs3m23:
                register_value |= BIT3;
                break;
            case kFramingModeDs3m23IF:
                register_value |= BIT3|BIT1;
                break;
            case kFramingModeDs3m23EF:
                register_value |= BIT3 | BIT1 | BIT0;
                break;
            case kFramingModeDs3m23FF:
                register_value |= BIT1|BIT2|BIT3;
                break;  
            case kFramingModeE3:
                register_value |=BIT4;
                break;
            case kFramingModeE3G751PLCP:
                register_value |= BIT4 | BIT2;
                break;
            case kFramingModeE3G751IF:
                register_value |= BIT4 | BIT1;
                break;
            case kFramingModeDS3CbitPLCP:
                register_value |= BIT2;
                break;
            case kFramingModeDS3M23PLCP:
                register_value |= BIT2 | BIT3;
                break;
            case kFramingModeE3CC:
                register_value |= BIT5 | BIT4;
                break;
            case kFramingModeInvalid:
                break;
        }
        dag37d_iom_write(card,port_state->mBase, loc, register_value);

        if (framing_value == kFramingModeE3CC)	// Enable discard on the port
        {
            if(port_state->mIndex == 0)
                loc = D3182_CP_RECEIVE_CONTROL_A;
            else
                loc = D3182_CP_RECEIVE_CONTROL_B;
            register_value = dag37d_iom_read(card, port_state->mBase, loc);
            register_value &= ~(BIT4 | BIT3 | BIT2 | BIT1 | BIT0);
            register_value |= BIT4 |BIT2 | BIT1 | BIT0;
            dag37d_iom_write(card, port_state->mBase, loc, register_value);
        }

        if (framing_value == kFramingModeDs3CbitEF)	// Select RCLKOn and RSOFOn signals
        {
            if(port_state->mIndex == 0)
                loc = D3182_PORT_CONTROL_3_A + 1;
            else
                loc = D3182_PORT_CONTROL_3_B + 1;
            register_value = dag37d_iom_read(card, port_state->mBase, loc);
            register_value &= ~(BIT5 | BIT4);
            register_value |= BIT5 | BIT4;
            dag37d_iom_write(card, port_state->mBase, loc, register_value);
        }
    }
}

static AttributePtr
get_new_rxlol(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReceiveLockError);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rxlol_get_value);
        attribute_set_name(result, "rle");
        attribute_set_description(result, "Receive lock error");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
rxlol_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_PORT_STATUS_A;
        else
            loc = D3182_PORT_STATUS_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(register_value & BIT1 )
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

static AttributePtr
get_new_rdi(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRemoteDefectIndication);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, rdi_get_value);
        attribute_set_name(result, "rdi");
        attribute_set_description(result, "Remote Defect Indication");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
rdi_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_T3_RECEIVE_STATUS_A;
        else
            loc = D3182_T3_RECEIVE_STATUS_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(register_value & BIT3 )
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

static AttributePtr
get_new_aic_m23_cbit(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAICM23Cbit);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, aic_m23_cbit_get_value);
        attribute_set_name(result, "aic");
        attribute_set_description(result, "Application Identification Channel (AIC), Cbit/M23 mode");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
aic_m23_cbit_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_T3_RECEIVE_STATUS_A;
        else
            loc = D3182_T3_RECEIVE_STATUS_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(register_value & BIT10 )
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


static AttributePtr
get_new_ais(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAlarmIndicationSignal);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, ais_get_value);
        attribute_set_name(result, "ais");
        attribute_set_description(result, "Alarm Indication Signal");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }

    return result;
}

static void*
ais_get_value(AttributePtr attribute)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    dag37d_port_state_t * state = (dag37d_port_state_t *)component_get_private_state(comp);
    uint32_t register_value;
    uint8_t value;
    uint32_t loc = 0;

    if (1 == valid_attribute(attribute))
    {
        if(state->mIndex == 0)
            loc = D3182_T3_RECEIVE_STATUS_A;
        else
            loc = D3182_T3_RECEIVE_STATUS_B;

        register_value = dag37d_iom_read(card, state->mBase, loc);
        if(register_value & BIT2 )
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

/* make sure the address is specified using the 8 bit address mode (see datasheet ds3181-ds3184 from Maxim)*/
uint32_t
dag37d_iom_read(DagCardPtr card, uint32_t base, uint32_t address)
{
	uint32_t reg_val = 0;

	/* convert to 32 bit address */
	address *= 4;
	usleep(100);
	reg_val = card_read_iom(card, base + address);
	dagutil_verbose("Read: 0x%.8x = 0x%.8x\n", address, reg_val);
	return reg_val;
}

/* make sure the address is specified using the 8 bit address mode (see datasheet ds3181-ds3184 from Maxim)*/
void
dag37d_iom_write(DagCardPtr card, uint32_t base, uint32_t address, uint32_t val)
{
	/* convert to 32 bit address */
	address *= 4;
	usleep(100);
	card_write_iom(card, base + address, val);
	dagutil_verbose("Write: 0x%.8x = 0x%.8x\n", address, val);
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
