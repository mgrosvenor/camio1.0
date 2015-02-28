/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/util/enum_string_table.h"

#include "../include/attribute_factory.h"
#include "../include/components/sonic_component.h"

#include "dagutil.h"

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


typedef enum
{
    kSDHStatus          = 0x4,
    kB1ErrorCounter     = 0x8,
    kB2ErrorCounter     = 0xc,
    kB3ErrorCounter     = 0x10,
    kREIErrorCounter    = 0x14,
    kLabels             = 0x18,
    kSDHDemapper        = 0x20,
    kE1T1DeframerStatus = 0x40,
    kE1T1DeframerConfig = 0x44,
    kE1T1LinkStatus     = 0x48
} register_offset_t;

typedef enum
{
    kVersionChannelized = 2,
    kVersionConcatenated = 3
} version_t;

/**
 * attribute functions imported from dag71s_impl.c
 */

static int sonic_get_vc_max(ComponentPtr sonic);

/* connection number attribute */
static AttributePtr sonic_get_new_connection_number(void);
static void sonic_connection_number_dispose(AttributePtr attribute);
static void sonic_connection_number_set_value(AttributePtr attribute, void* value, int length);
static void* sonic_connection_number_get_value(AttributePtr attribute);
static void sonic_connection_number_post_initialize(AttributePtr attribute);

/* refresh SDH status register cache */ // ???
static AttributePtr sonic_get_new_refresh_cache_attribute(void);
static void sonic_refresh_cache_set_value(AttributePtr attribute, void* value, int length);
static void* sonic_refresh_cache_get_value(AttributePtr attribute);

/* deframer_revision_id attribute. */ // ???
static AttributePtr sonic_get_new_deframer_revision_id_attribute(void);
static void sonic_deframer_revision_id_dispose(AttributePtr attribute);
static void* sonic_deframer_revision_id_get_value(AttributePtr attribute);
static void sonic_deframer_revision_id_post_initialize(AttributePtr attribute);

/* payload mapping attribute */ // ???
static AttributePtr sonic_get_new_payload_mapping(void);
static void sonic_payload_mapping_dispose(AttributePtr attribute);
static void sonic_payload_mapping_set_value(AttributePtr attribute, void* value, int length);
static void* sonic_payload_mapping_get_value(AttributePtr attribute);
static void sonic_payload_mapping_post_initialize(AttributePtr attribute);
static void sonic_payload_mapping_to_string(AttributePtr attribute);
static void sonic_payload_mapping_from_string(AttributePtr attribute, const char* string);

/* tributary unit attribute */ // ???
static AttributePtr sonic_get_new_tributary_unit(void);
static void sonic_tributary_unit_dispose(AttributePtr attribute);
static void sonic_tributary_unit_set_value(AttributePtr attribute, void* value, int length);
static void* sonic_tributary_unit_get_value(AttributePtr attribute);
static void sonic_tributary_unit_post_initialize(AttributePtr attribute);
static void sonic_tributary_unit_to_string(AttributePtr attribute);
static void sonic_tributary_unit_from_string(AttributePtr attribute, const char* string);

/* vc_size attribute. */
static AttributePtr sonic_get_new_vc_size_attribute(void);
static void sonic_vc_size_dispose(AttributePtr attribute);
static void* sonic_vc_size_get_value(AttributePtr attribute);
static void sonic_vc_size_set_value(AttributePtr attribute, void* value, int length);
static void sonic_vc_size_post_initialize(AttributePtr attribute);
static void sonic_vc_size_to_string(AttributePtr attribute);
static void sonic_vc_size_from_string(AttributePtr attribute, const char* string);

/* scramble attribute. */
static AttributePtr sonic_get_new_scramble_attribute(void);
static void sonic_scramble_dispose(AttributePtr attribute);
static void* sonic_scramble_get_value(AttributePtr attribute);
static void sonic_scramble_set_value(AttributePtr attribute, void* value, int length);
static void sonic_scramble_post_initialize(AttributePtr attribute);


static AttributePtr sonic_get_new_b1_attribute(void);
static void sonic_b1_dispose(AttributePtr attribute);
static void* sonic_b1_get_value(AttributePtr attribute);
static void sonic_b1_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_b2_attribute(void);
static void sonic_b2_dispose(AttributePtr attribute);
static void* sonic_b2_get_value(AttributePtr attribute);
static void sonic_b2_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_b3_attribute(void);
static void sonic_b3_dispose(AttributePtr attribute);
static void* sonic_b3_get_value(AttributePtr attribute);
static void sonic_b3_post_initialize(AttributePtr attribute);


static AttributePtr sonic_get_new_rei_error_attribute(void);
static void sonic_rei_error_dispose(AttributePtr attribute);
static void* sonic_rei_error_get_value(AttributePtr attribute);
static void sonic_rei_error_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_rei_error_count_attribute(void);
static void sonic_rei_error_count_dispose(AttributePtr attribute);
static void* sonic_rei_error_count_get_value(AttributePtr attribute);
static void sonic_rei_error_count_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_rdi_error_attribute(void);
static void sonic_rdi_error_dispose(AttributePtr attribute);
static void* sonic_rdi_error_get_value(AttributePtr attribute);
static void sonic_rdi_error_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_erdi_error_attribute(void);
static void sonic_erdi_error_dispose(AttributePtr attribute);
static void* sonic_erdi_error_get_value(AttributePtr attribute);
static void sonic_erdi_error_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_ssm_attribute(void);
static void sonic_ssm_dispose(AttributePtr attribute);
static void* sonic_ssm_get_value(AttributePtr attribute);
static void sonic_ssm_post_initialize(AttributePtr attribute);

/* pointer state attribute */
static AttributePtr sonic_get_new_pointer_state_attribute(void);
static void sonic_pointer_state_dispose(AttributePtr attribute);
static void* sonic_pointer_state_get_value(AttributePtr attribute);
static void sonic_pointer_state_post_initialize(AttributePtr attribute);

/* line_rate attribute. */
static AttributePtr sonic_get_new_line_rate_attribute(void);
static void sonic_line_rate_dispose(AttributePtr attribute);
static void* sonic_line_rate_get_value(AttributePtr attribute);
static void sonic_line_rate_set_value(AttributePtr attribute, void* value, int length);
static void sonic_line_rate_post_initialize(AttributePtr attribute);
static void sonic_line_rate_to_string(AttributePtr attribute);
static void sonic_line_rate_from_string(AttributePtr attribute, const char* string);

/* vc_max_index attribute. */
static AttributePtr sonic_get_new_vc_max_index_attribute(void);
static void sonic_vc_max_index_dispose(AttributePtr attribute);
static void* sonic_vc_max_index_get_value(AttributePtr attribute);
static void sonic_vc_max_index_post_initialize(AttributePtr attribute);

/* vc_index attribute. */
static AttributePtr sonic_get_new_vc_index_attribute(void);
static void sonic_vc_index_dispose(AttributePtr attribute);
static void* sonic_vc_index_get_value(AttributePtr attribute);
static void sonic_vc_index_set_value(AttributePtr attribute, void* value, int length);
static void sonic_vc_index_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_b1_error_count(void);
static void sonic_b1_error_count_dispose(AttributePtr attribute);
static void* sonic_b1_error_count_get_value(AttributePtr attribute);
static void sonic_b1_error_count_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_b2_error_count(void);
static void sonic_b2_error_count_dispose(AttributePtr attribute);
static void* sonic_b2_error_count_get_value(AttributePtr attribute);
static void sonic_b2_error_count_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_b3_error_count(void);
static void sonic_b3_error_count_dispose(AttributePtr attribute);
static void* sonic_b3_error_count_get_value(AttributePtr attribute);
static void sonic_b3_error_count_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_data_pointer(void);
static void sonic_data_pointer_dispose(AttributePtr attribute);
static void* sonic_data_pointer_get_value(AttributePtr attribute);
static void sonic_data_pointer_set_value(AttributePtr attribute, void* value, int length);
static void sonic_data_pointer_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_c2_path_label(void);
static void sonic_c2_path_label_dispose(AttributePtr attribute);
static void* sonic_c2_path_label_get_value(AttributePtr attribute);
static void sonic_c2_path_label_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_j0_path_label(void);
static void sonic_j0_path_label_dispose(AttributePtr attribute);
static void* sonic_j0_path_label_get_value(AttributePtr attribute);
static void sonic_j0_path_label_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_j1_path_label(void);
static void sonic_j1_path_label_dispose(AttributePtr attribute);
static void* sonic_j1_path_label_get_value(AttributePtr attribute);
static void sonic_j1_path_label_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_conn_VC_Label_attribute(void);
static void sonic_conn_VC_Label_dispose(AttributePtr attribute);
static void* sonic_conn_VC_Label_get_value(AttributePtr attribute);
static void sonic_conn_VC_Label_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_conn_VC_pointer_attribute(void);
static void sonic_conn_VC_pointer_dispose(AttributePtr attribute);
static void* sonic_conn_VC_pointer_get_value(AttributePtr attribute);
static void sonic_conn_VC_pointer_post_initialize(AttributePtr attribute);

static AttributePtr sonic_get_new_txv5_signal_label(void);
static void sonic_txv5_signal_label_post_initialize(AttributePtr attribute);
static void sonic_txv5_signal_label_set_value(AttributePtr attribute, void* value, int length);
static void* sonic_txv5_signal_label_get_value(AttributePtr attribute);
static bool check_valid_txv5_value_range(txv5_value_t value);


static AttributePtr sonic_get_new_txc2_path_label0(void);
static void* sonic_txc2_path_label0_get_value(AttributePtr attribute);
static void sonic_txc2_path_label0_set_value(AttributePtr attribute, void* value, int length);
static AttributePtr sonic_get_new_txc2_path_label1(void);
static AttributePtr sonic_get_new_txc2_path_label2(void);
static AttributePtr sonic_get_new_txc2_path_label3(void);
static AttributePtr sonic_get_new_txc2_path_label4(void);
static AttributePtr sonic_get_new_txc2_path_label5(void);
static AttributePtr sonic_get_new_txc2_path_label6(void);
static AttributePtr sonic_get_new_txc2_path_label7(void);
static AttributePtr sonic_get_new_txc2_path_label8(void);
static AttributePtr sonic_get_new_txc2_path_label9(void);
static AttributePtr sonic_get_new_txc2_path_label10(void);
static AttributePtr sonic_get_new_txc2_path_label11(void);
static void sonic_txc2_path_label_set_value(AttributePtr attribute, uint16_t byte_index, uint16_t byte_value);
static int sonic_txc2_path_label_get_value(AttributePtr attribute, uint16_t byte_index);
static bool check_valid_txc2_value_range(txc2_value_t value);
dag_err_t
sonic_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        sonic_state_t* state = NULL;
        DagCardPtr card = NULL;
        card = component_get_card(component);
        NULL_RETURN_WV(card, kDagErrInvalidCardRef);
        state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mSonicBase = card_get_register_address(card, DAG_REG_SONIC_3, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

void
sonic_dispose(ComponentPtr component)
{
}

void
sonic_reset(ComponentPtr component)
{
}

void
sonic_default(ComponentPtr component)
{
	DagCardPtr card;
    AttributePtr attr_v5, attr_c2; 
	txv5_value_t v5_val = kTXV5ValueAsync; 
	txc2_value_t c2_val_conc = kTXC2HDLC;
	txc2_value_t c2_val_chan = kTXC2TUG;
	version_t version;    
    card = component_get_card(component);
	attr_c2 = component_get_attribute(component, kUint32AttributeTXC2PathLabel0);
	assert(attr_c2 != kNullAttributeUuid);
	version = card_get_register_version(card, DAG_REG_E1T1_HDLC_DEMAP, 0);    
    if (kVersionChannelized == version){
        attr_v5 = component_get_attribute(component, kUint32AttributeTXV5SignalLabel);
	    assert(attr_v5 != kNullAttributeUuid);
        sonic_txv5_signal_label_set_value(attr_v5,&v5_val,sizeof(txv5_value_t));		//set the default value of TXV5 byte
		sonic_txc2_path_label0_set_value(attr_c2,&c2_val_chan ,sizeof(txc2_value_t));
		return;
    }
	sonic_txc2_path_label0_set_value(attr_c2,&c2_val_conc ,sizeof(txc2_value_t));
}

int
sonic_post_initialize(ComponentPtr component)
{
    AttributePtr vc_size;
    AttributePtr scrambling;
    AttributePtr rev_id;
    AttributePtr b1_error;
    AttributePtr b2_error;
    AttributePtr b3_error;
    AttributePtr rdi_error;
    AttributePtr erdi_error;
    AttributePtr rei_error;
    AttributePtr rei_error_count;
    AttributePtr pointer_state;
    AttributePtr ssm;
    AttributePtr line_rate;
    AttributePtr tributary_unit;
    AttributePtr connection_number;
    AttributePtr payload_mapping;
    AttributePtr b1_error_count;
    AttributePtr b2_error_count;
    AttributePtr b3_error_count;
    AttributePtr data_pointer;
    AttributePtr c2_path_label;
    AttributePtr j0_path_label;
    AttributePtr j1_path_label;
    AttributePtr vc_index;
    AttributePtr vc_max_index;
    AttributePtr connection_vc_label;
    AttributePtr connection_vc_pointer;
	AttributePtr txv5_signal_label;
	AttributePtr txc2_path_label0;						
	AttributePtr txc2_path_label1;						
	AttributePtr txc2_path_label2;						
	AttributePtr txc2_path_label3;						
	AttributePtr txc2_path_label4;						
	AttributePtr txc2_path_label5;						
	AttributePtr txc2_path_label6;						
	AttributePtr txc2_path_label7;						
	AttributePtr txc2_path_label8;						
	AttributePtr txc2_path_label9;						
	AttributePtr txc2_path_label10;
	AttributePtr txc2_path_label11;						

    AttributePtr refresh_cache;
    
    DagCardPtr card;
    sonic_state_t* state;
    GenericReadWritePtr grw = NULL;
    uint32_t bool_attr_mask = 0;
    AttributePtr attr = NULL;
    uintptr_t address = 0;
    version_t version;

    card = component_get_card(component);
    NULL_RETURN_WV(card, 0);
    state = component_get_private_state(component);
    state->mSonicBase = card_get_register_address(card, DAG_REG_SONIC_3, state->mIndex);
    if (state->mSonicBase == 0x0)
    {
        dagutil_warning("No Sonic component? Have you loaded the pp images?\n");
        return 0;
    }
    vc_size = sonic_get_new_vc_size_attribute();
    scrambling = sonic_get_new_scramble_attribute();
    rev_id = sonic_get_new_deframer_revision_id_attribute();
	
	/* Attributes that use the function grw_sonic_status_cache_read need to set an attribute,
	 * hence the call to grw_set_attribute, because grw_sonic_status_cache_read uses the attribute
	 * pointer.
	 */
    /* Loss Of Signal*/
    bool_attr_mask = BIT0;
    address = (uintptr_t)card_get_iom_address(card) + state->mSonicBase + kSDHStatus;
    grw = grw_init(card, address, grw_sonic_status_cache_read, NULL);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfSignal, grw, &bool_attr_mask, 1);
	grw_set_attribute(grw, attr);
    component_add_attribute(component,attr);
    
    /* Loss Of Frame */
    bool_attr_mask = BIT1;
    address = (uintptr_t)card_get_iom_address(card) + state->mSonicBase + kSDHStatus;
    grw = grw_init(card, address, grw_sonic_status_cache_read, NULL);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFrame, grw, &bool_attr_mask, 1);
	grw_set_attribute(grw, attr);
    component_add_attribute(component,attr);

    /* Out Of Frame */
    bool_attr_mask = BIT2;
    address = (uintptr_t)card_get_iom_address(card) + state->mSonicBase + kSDHStatus;
    grw = grw_init(card, address, grw_sonic_status_cache_read, NULL);
    attr = attribute_factory_make_attribute(kBooleanAttributeOutOfFrame, grw, &bool_attr_mask, 1);
	grw_set_attribute(grw, attr);
    component_add_attribute(component,attr);

    /* Latch */
    bool_attr_mask = BIT31;
    address = (uintptr_t)card_get_iom_address(card) + state->mSonicBase;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCounterLatch, grw, &bool_attr_mask, 1);
    component_add_attribute(component,attr);    
    
    b1_error = sonic_get_new_b1_attribute();
    b2_error = sonic_get_new_b2_attribute();
    b3_error = sonic_get_new_b3_attribute();
    rei_error = sonic_get_new_rei_error_attribute();
    rei_error_count = sonic_get_new_rei_error_count_attribute();
    rdi_error = sonic_get_new_rdi_error_attribute();
    erdi_error = sonic_get_new_erdi_error_attribute();
    ssm = sonic_get_new_ssm_attribute();
    pointer_state = sonic_get_new_pointer_state_attribute();
    line_rate = sonic_get_new_line_rate_attribute();
    b1_error_count = sonic_get_new_b1_error_count();
    b2_error_count = sonic_get_new_b2_error_count();
    b3_error_count = sonic_get_new_b3_error_count();
    data_pointer = sonic_get_new_data_pointer();
    c2_path_label = sonic_get_new_c2_path_label();
    j0_path_label = sonic_get_new_j0_path_label();
    j1_path_label = sonic_get_new_j1_path_label();
    vc_index = sonic_get_new_vc_index_attribute();
    vc_max_index = sonic_get_new_vc_max_index_attribute();	
    txc2_path_label0  = sonic_get_new_txc2_path_label0();	
    refresh_cache  = sonic_get_new_refresh_cache_attribute();
	
    component_add_attribute(component, refresh_cache);
    component_add_attribute(component, txc2_path_label0);	
    component_add_attribute(component, line_rate);
    component_add_attribute(component, vc_size);
    component_add_attribute(component, scrambling);
    component_add_attribute(component, rev_id);
    component_add_attribute(component, b1_error);
    component_add_attribute(component, b2_error);
    component_add_attribute(component, b3_error);
    component_add_attribute(component, pointer_state);
    component_add_attribute(component, ssm);
    component_add_attribute(component, b1_error_count);
    component_add_attribute(component, b2_error_count);
    component_add_attribute(component, b3_error_count);
    component_add_attribute(component, data_pointer);
    component_add_attribute(component, c2_path_label);
    component_add_attribute(component, j0_path_label);
    component_add_attribute(component, j1_path_label);
    component_add_attribute(component, vc_index);
    component_add_attribute(component, vc_max_index);
    component_add_attribute(component, rei_error);
    component_add_attribute(component, rei_error_count);
    component_add_attribute(component, rdi_error);
    component_add_attribute(component, erdi_error);
	version = card_get_register_version(card, DAG_REG_E1T1_HDLC_DEMAP, 0);
	
	if (kVersionChannelized == version){
		
	        connection_number		= sonic_get_new_connection_number();
		payload_mapping			= sonic_get_new_payload_mapping();
		tributary_unit			= sonic_get_new_tributary_unit();
		connection_vc_label		= sonic_get_new_conn_VC_Label_attribute();
		connection_vc_pointer		= sonic_get_new_conn_VC_pointer_attribute();
	    	txv5_signal_label		= sonic_get_new_txv5_signal_label();
		txc2_path_label1		= sonic_get_new_txc2_path_label1();				
		txc2_path_label2		= sonic_get_new_txc2_path_label2();				
		txc2_path_label3		= sonic_get_new_txc2_path_label3();				
		txc2_path_label4		= sonic_get_new_txc2_path_label4();				
		txc2_path_label5		= sonic_get_new_txc2_path_label5();				
		txc2_path_label6		= sonic_get_new_txc2_path_label6();				
		txc2_path_label7		= sonic_get_new_txc2_path_label7();				
		txc2_path_label8		= sonic_get_new_txc2_path_label8();				
		txc2_path_label9		= sonic_get_new_txc2_path_label9();				
		txc2_path_label10		= sonic_get_new_txc2_path_label10();				
		txc2_path_label11		= sonic_get_new_txc2_path_label11();			
   
		component_add_attribute(component, connection_number);
		component_add_attribute(component, payload_mapping);
		component_add_attribute(component, tributary_unit);
		component_add_attribute(component, connection_vc_label);
		component_add_attribute(component, connection_vc_pointer);
		component_add_attribute(component, txv5_signal_label);
		component_add_attribute(component, txc2_path_label1);
		component_add_attribute(component, txc2_path_label2);
		component_add_attribute(component, txc2_path_label3);
		component_add_attribute(component, txc2_path_label4);
		component_add_attribute(component, txc2_path_label5);
		component_add_attribute(component, txc2_path_label6);
		component_add_attribute(component, txc2_path_label7);
		component_add_attribute(component, txc2_path_label8);
		component_add_attribute(component, txc2_path_label9);
		component_add_attribute(component, txc2_path_label10);
		component_add_attribute(component, txc2_path_label11);
	}

    
    return 1;
}

/**********************************************************************/

static AttributePtr
sonic_get_new_line_rate_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineRate); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_line_rate_dispose);
        attribute_set_post_initialize_routine(result, sonic_line_rate_post_initialize);
        attribute_set_getvalue_routine(result, sonic_line_rate_get_value);
        attribute_set_setvalue_routine(result, sonic_line_rate_set_value);
        attribute_set_to_string_routine(result, sonic_line_rate_to_string);
        attribute_set_from_string_routine(result, sonic_line_rate_from_string);
        attribute_set_name(result, "line_rate");
        attribute_set_description(result, "Set the line rate to a specific speed");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_line_rate_dispose(AttributePtr attribute)
{
}

static void*
sonic_line_rate_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    ComponentPtr root_component;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t register_value;
    uint32_t line_rate;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        root_component = card_get_root_component(card);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        if (sonic_state->mSonicBase)
        {
            register_value = card_read_iom(card, sonic_state->mSonicBase);
            if (register_value & BIT4)
            {
                line_rate = kLineRateOC12c;
            }
            else
            {
                line_rate = kLineRateOC3c;
            }
        }
        attribute_set_value_array(attribute, &line_rate, sizeof(line_rate));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static void
sonic_line_rate_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr port;
    ComponentPtr sonic;
    ComponentPtr root_component;
    AttributePtr line_rate_attribute;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    line_rate_t line_rate = *(line_rate_t*)value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        /* the line rate on the AMC1213 and the one on the SONET/SDH component should match.
         * This is why we write to both components here 
         */
        card = attribute_get_card(attribute);
        NULL_RETURN(card);;
        root_component = card_get_root_component(card);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        /* set the line rate on the AMC1213 via the port component */
        port = component_get_subcomponent(root_component, kComponentPort, sonic_state->mIndex);
        line_rate_attribute = component_get_attribute(port, kUint32AttributeLineRate);
        attribute_set_value(line_rate_attribute, (void*)&line_rate, sizeof(line_rate));

        /* set the line rate on the sonic component. */
        sonic = component_get_subcomponent(root_component, kComponentSonic, sonic_state->mIndex);
        if (sonic_state->mSonicBase)
        {
            register_value = card_read_iom(card, sonic_state->mSonicBase);
            if (line_rate == kLineRateOC12c)
            {
                register_value &= ~BIT31;
                register_value |= BIT4;
            }
            else if (line_rate == kLineRateOC3c)
            {
                register_value &= ~(BIT4 | BIT31);
            }
            card_write_iom(card, sonic_state->mSonicBase, register_value);
        }
    }
}

static void
sonic_line_rate_post_initialize(AttributePtr attribute)
{
}

void
sonic_line_rate_to_string(AttributePtr attribute)
{
    line_rate_t value = *(line_rate_t*)sonic_line_rate_get_value(attribute);
    const char* temp = line_rate_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}

void
sonic_line_rate_from_string(AttributePtr attribute, const char* string)
{
    line_rate_t value = string_to_line_rate(string);
    if (kLineRateInvalid != value)
        sonic_line_rate_set_value(attribute, (void*)&value, sizeof(line_rate_t));
}

static AttributePtr
sonic_get_new_vc_max_index_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeVCMaxIndex); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_vc_max_index_dispose);
        attribute_set_post_initialize_routine(result, sonic_vc_max_index_post_initialize);
        attribute_set_getvalue_routine(result, sonic_vc_max_index_get_value);
        attribute_set_name(result, "vc_max_index");
        attribute_set_description(result, "Set the vc number.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_vc_max_index_dispose(AttributePtr attribute)
{
}

static void*
sonic_vc_max_index_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t vc_max_index;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        vc_max_index = sonic_get_vc_max(sonic);
        attribute_set_value_array(attribute, &vc_max_index, sizeof(vc_max_index));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_vc_max_index_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_vc_index_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeVCIndex); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_vc_index_dispose);
        attribute_set_post_initialize_routine(result, sonic_vc_index_post_initialize);
        attribute_set_getvalue_routine(result, sonic_vc_index_get_value);
        attribute_set_setvalue_routine(result, sonic_vc_index_set_value);
        attribute_set_name(result, "vc_index");
        attribute_set_description(result, "Set the vc number.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_vc_index_dispose(AttributePtr attribute)
{
}

static void*
sonic_vc_index_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t vc_index;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        vc_index = (register_value & (BIT19 | BIT18 | BIT17 | BIT16)) >> 16;
        attribute_set_value_array(attribute, &vc_index, sizeof(vc_index));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static void
sonic_vc_index_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN(card);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        register_value &= ~(BIT19 | BIT18 | BIT17 | BIT16);
        register_value |= (*(uint32_t*)value) << 16;
        card_write_iom(card, sonic_state->mSonicBase, register_value);
    }
}

static void
sonic_vc_index_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_vc_size_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeVCSize); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_vc_size_dispose);
        attribute_set_post_initialize_routine(result, sonic_vc_size_post_initialize);
        attribute_set_getvalue_routine(result, sonic_vc_size_get_value);
        attribute_set_setvalue_routine(result, sonic_vc_size_set_value);
        attribute_set_to_string_routine(result, sonic_vc_size_to_string);
        attribute_set_from_string_routine(result, sonic_vc_size_from_string);
        attribute_set_name(result, "vc_size");
        attribute_set_description(result, "Change the VC Size");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_vc_size_dispose(AttributePtr attribute)
{
}

static void*
sonic_vc_size_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    vc_size_t vc_size;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        if ((register_value & (BIT8 | BIT9)) == 0)
        {
            vc_size = kVC3;
        }
        else if ((register_value & (BIT8 | BIT9)) == BIT8)
        {
            vc_size = kVC4;
        }
        else if ((register_value & (BIT8 | BIT9)) == BIT9)
        {
            vc_size = kVC4C;
        }
        attribute_set_value_array(attribute, &vc_size, sizeof(vc_size));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_vc_size_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    vc_size_t vc_size = *(vc_size_t*)value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN(card);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        if (vc_size == kVC3)
        {
            register_value &= ~(BIT8 | BIT9 | BIT31);
            /* There's a problem with the firmware that requires it to move
             * from vc3 to vc4 for things to work correctly */
			/*
            card_write_iom(card, sonic_state->mSonicBase, register_value);
            register_value |= BIT8;
            card_write_iom(card, sonic_state->mSonicBase, register_value);
			*/
            register_value &= ~(BIT8 | BIT9 | BIT31);
            card_write_iom(card, sonic_state->mSonicBase, register_value);
        }
        else if (vc_size == kVC4)
        {
            register_value &= ~(BIT8 | BIT9 | BIT31);
            /* There's a problem with the firmware that requires it to move
             * from vc3 to vc4 for things to work correctly */
			/*
            card_write_iom(card, sonic_state->mSonicBase, register_value);
            register_value = card_read_iom(card, sonic_state->mSonicBase);
            register_value |= BIT8;
            card_write_iom(card, sonic_state->mSonicBase, register_value);
            register_value = card_read_iom(card, sonic_state->mSonicBase);
            register_value &= ~(BIT8 | BIT9 | BIT31);
            card_write_iom(card, sonic_state->mSonicBase, register_value);
            register_value = card_read_iom(card, sonic_state->mSonicBase);
			*/
            register_value |= BIT8;
            card_write_iom(card, sonic_state->mSonicBase, register_value);
        }
        else if (vc_size == kVC4C)
        {
            register_value &= ~(BIT8 | BIT9 | BIT31);
            register_value |= BIT9;
            card_write_iom(card, sonic_state->mSonicBase, register_value);
        }
    }
}

static void
sonic_vc_size_post_initialize(AttributePtr attribute)
{
}

static void
sonic_vc_size_to_string(AttributePtr attribute)
{
    vc_size_t value = *(vc_size_t*)sonic_vc_size_get_value(attribute);
    const char* temp = vc_size_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}

static void
sonic_vc_size_from_string(AttributePtr attribute, const char* string)
{
    vc_size_t value = string_to_vc_size(string);
    if (value != kVCInvalid)
        sonic_vc_size_set_value(attribute, (void*)&value, sizeof(vc_size_t));
}

ComponentPtr
sonic_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result;
    sonic_state_t* state = NULL;
    char buffer[BUFFER_SIZE];

    dagutil_verbose_level(2, "Checking SONIC register address\n");
    if (card_get_register_address(card, DAG_REG_SONIC_3, index) == 0)
    {
        dagutil_verbose_level(2, "Bad SONIC register address\n");
        return NULL;
    }
    dagutil_verbose_level(2, "good register address\n");
    result = component_init(kComponentSonic, card); 
    if (NULL != result)
    {
        component_set_dispose_routine(result, sonic_dispose);
        component_set_post_initialize_routine(result, sonic_post_initialize);
        component_set_reset_routine(result, sonic_reset);
        component_set_default_routine(result, sonic_default);
        component_set_update_register_base_routine(result, sonic_update_register_base); 
        sprintf(buffer, "sonic%d", index);
        component_set_name(result, buffer);
        state = (sonic_state_t*)malloc(sizeof(sonic_state_t));
		memset(state, 0, sizeof(*state));
        component_set_private_state(result, state);
        state->mIndex = index;
    }
    
    return result;
}

static AttributePtr
sonic_get_new_scramble_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeScramble); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_scramble_dispose);
        attribute_set_post_initialize_routine(result, sonic_scramble_post_initialize);
        attribute_set_getvalue_routine(result, sonic_scramble_get_value);
        attribute_set_setvalue_routine(result, sonic_scramble_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "scramble");
        attribute_set_description(result, "Enable or disable scramble.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
sonic_scramble_dispose(AttributePtr attribute)
{
}

static void*
sonic_scramble_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t scramble;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        scramble = (register_value & BIT12) == BIT12;
        attribute_set_value_array(attribute, &scramble, sizeof(scramble));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static void
sonic_scramble_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t scramble = *(uint8_t*)value;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN(card);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        if (scramble == 1)
        {
            register_value &= ~(BIT12 | BIT31);
            register_value |= BIT12;
        }
        else
        {
            register_value &= ~(BIT12 | BIT31);
        }
        card_write_iom(card, sonic_state->mSonicBase, register_value);
    }
}

static void
sonic_scramble_post_initialize(AttributePtr attribute)
{
}



static AttributePtr
sonic_get_new_b1_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeB1Error); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_b1_dispose);
        attribute_set_post_initialize_routine(result, sonic_b1_post_initialize);
        attribute_set_getvalue_routine(result, sonic_b1_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "b1");
        attribute_set_description(result, "B1 error has occured.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
sonic_b1_dispose(AttributePtr attribute)
{
}

static void*
sonic_b1_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t b1;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        b1 = (sonic_state->mStatusCache & BIT3) == BIT3;
        attribute_set_value_array(attribute, &b1, sizeof(b1));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_b1_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_b2_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeB2Error); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_b2_dispose);
        attribute_set_post_initialize_routine(result, sonic_b2_post_initialize);
        attribute_set_getvalue_routine(result, sonic_b2_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "b2");
        attribute_set_description(result, "B2 error has occurred.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
sonic_b2_dispose(AttributePtr attribute)
{
}

static void*
sonic_b2_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t b2;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        b2 = (sonic_state->mStatusCache & BIT4) == BIT4;
        attribute_set_value_array(attribute, &b2, sizeof(b2));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_b2_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_b3_attribute()
{
    AttributePtr result = attribute_init(kBooleanAttributeB3Error); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_b3_dispose);
        attribute_set_post_initialize_routine(result, sonic_b3_post_initialize);
        attribute_set_getvalue_routine(result, sonic_b3_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "b3");
        attribute_set_description(result, "B3 error has occurred.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
sonic_b3_dispose(AttributePtr attribute)
{
}

static void*
sonic_b3_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = (sonic_state_t*)component_get_private_state(sonic);
        value = (sonic_state->mStatusCache & BIT5) == BIT5;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_b3_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_rei_error_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeREIError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_rei_error_dispose);
        attribute_set_post_initialize_routine(result, sonic_rei_error_post_initialize);
        attribute_set_getvalue_routine(result, sonic_rei_error_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "rei_error");
        attribute_set_description(result, "remote error indicator.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
sonic_rei_error_dispose(AttributePtr attribute)
{
}

static void*
sonic_rei_error_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t rei;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);
        sonic = attribute_get_component(attribute);
        sonic_state = (sonic_state_t*)component_get_private_state(sonic);
        rei = ((sonic_state->mStatusCache & BIT6) == BIT6);
        attribute_set_value_array(attribute, &rei, sizeof(rei));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_rei_error_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_rei_error_count_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeREIErrorCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_rei_error_count_dispose);
        attribute_set_post_initialize_routine(result, sonic_rei_error_count_post_initialize);
        attribute_set_getvalue_routine(result, sonic_rei_error_count_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "rei_error_count");
        attribute_set_description(result, "remote error count.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_rei_error_count_dispose(AttributePtr attribute)
{
}

static void*
sonic_rei_error_count_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);
        sonic = attribute_get_component(attribute);
        sonic_state = (sonic_state_t*)component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        /* read the rei value */
        register_value = card_read_iom(card, sonic_state->mSonicBase + kREIErrorCounter);
        attribute_set_value_array(attribute, &register_value, sizeof(register_value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_rei_error_count_post_initialize(AttributePtr attribute)
{
}


static AttributePtr
sonic_get_new_rdi_error_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRDIError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_rdi_error_dispose);
        attribute_set_post_initialize_routine(result, sonic_rdi_error_post_initialize);
        attribute_set_getvalue_routine(result, sonic_rdi_error_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "rdi_error");
        attribute_set_description(result, "rdi_error error has occurred.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
sonic_rdi_error_dispose(AttributePtr attribute)
{
}

static void*
sonic_rdi_error_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t rdi_error;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        rdi_error = (sonic_state->mStatusCache & BIT7) == BIT7;
        attribute_set_value_array(attribute, &rdi_error, sizeof(rdi_error));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_rdi_error_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_erdi_error_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeERDIError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_erdi_error_dispose);
        attribute_set_post_initialize_routine(result, sonic_erdi_error_post_initialize);
        attribute_set_getvalue_routine(result, sonic_erdi_error_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "e-rdi_error");
        attribute_set_description(result, "e-rdi_error error has occurred.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_erdi_error_dispose(AttributePtr attribute)
{
}

static void*
sonic_erdi_error_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint8_t erdi_error;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        erdi_error = (sonic_state->mStatusCache & 0x300);	
	erdi_error >>= 8;
        attribute_set_value_array(attribute, &erdi_error, sizeof(erdi_error));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_erdi_error_post_initialize(AttributePtr attribute)
{
}


static AttributePtr
sonic_get_new_pointer_state_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributePointerState);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_pointer_state_dispose);
        attribute_set_post_initialize_routine(result, sonic_pointer_state_post_initialize);
        attribute_set_getvalue_routine(result, sonic_pointer_state_get_value);
        attribute_set_name(result, "pointer_state");
        attribute_set_description(result, "pointer_state error has occurred.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_pointer_state_dispose(AttributePtr attribute)
{
}

static void*
sonic_pointer_state_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t register_value;
    vc_pointer_state_t pointer_state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = (sonic_state_t*)component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        /* read the register */
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHStatus);
        switch ((register_value & (BIT13 | BIT12)) >> 12)
        {
            case 0:
                pointer_state = kPointerStateLossOfPointer;
                break;

            case 1:
                pointer_state = kPointerStateAlarmSignalIndicator;
                break;

            case 2:
                pointer_state = kPointerStateValid;
                break;

            case 3:
                pointer_state = kPointerStateConcatenationIndicator;
                break;

        }
        attribute_set_value_array(attribute, &pointer_state, sizeof(pointer_state));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_pointer_state_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_ssm_attribute(void)
{ 
    AttributePtr result = attribute_init(kUint32AttributeSSM); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_ssm_dispose);
        attribute_set_post_initialize_routine(result, sonic_ssm_post_initialize);
        attribute_set_getvalue_routine(result, sonic_ssm_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "ssm");
        attribute_set_description(result, "received synchronization status message");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_ssm_dispose(AttributePtr attribute)
{
}

static void*
sonic_ssm_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    uint32_t ssm;
    sonic_state_t* sonic_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHStatus);
        ssm = (register_value & (BIT24 | BIT25 | BIT26 | BIT27)) >> 24;
        attribute_set_value_array(attribute, &ssm, sizeof(ssm));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_ssm_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_tributary_unit(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTributaryUnit); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_tributary_unit_dispose);
        attribute_set_post_initialize_routine(result, sonic_tributary_unit_post_initialize);
        attribute_set_setvalue_routine(result, sonic_tributary_unit_set_value);
        attribute_set_getvalue_routine(result, sonic_tributary_unit_get_value);
        attribute_set_to_string_routine(result, sonic_tributary_unit_to_string);
        attribute_set_from_string_routine(result, sonic_tributary_unit_from_string);
        attribute_set_name(result, "tributary_unit");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_tributary_unit_dispose(AttributePtr attribute)
{

}

static void
sonic_tributary_unit_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    tributary_unit_t val = *(tributary_unit_t*)value;
    sonic_state_t* sonic_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN(card);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHDemapper);
        register_value &= ~BIT4;
        register_value |= val << 4;
        card_write_iom(card, sonic_state->mSonicBase + kSDHDemapper, register_value);
    }
}

static void*
sonic_tributary_unit_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    tributary_unit_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHDemapper);
        register_value &= BIT4;
        register_value >>= 4;
        value = register_value;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_tributary_unit_post_initialize(AttributePtr attribute)
{
}

static void
sonic_tributary_unit_to_string(AttributePtr attribute)
{
    char buffer[BUFFER_SIZE];
    tributary_unit_t tu = *(tributary_unit_t*)sonic_tributary_unit_get_value(attribute);
    if (tu == kTU11)
    {
        strcpy(buffer, "tu11");
    }
    else if (tu == kTU12)
    {
        strcpy(buffer, "tu12");
    }
    attribute_set_to_string(attribute, buffer);
}

static void
sonic_tributary_unit_from_string(AttributePtr attribute, const char* string)
{
    tributary_unit_t value;
    if (1 == valid_attribute(attribute))
    {
        if (strcmp(string, "tu11") == 0)
        {
            value = kTU11;
        }
        else if (strcmp(string, "tu12") == 0)
        {
            value = kTU12;
        }
        sonic_tributary_unit_set_value(attribute, (void*)&value, sizeof(tributary_unit_t));
    }
}

static AttributePtr
sonic_get_new_connection_number(void)
{
    AttributePtr result = attribute_init(kUint32AttributeConnectionNumber); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_connection_number_dispose);
        attribute_set_post_initialize_routine(result, sonic_connection_number_post_initialize);
        attribute_set_setvalue_routine(result, sonic_connection_number_set_value);
        attribute_set_getvalue_routine(result, sonic_connection_number_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "connection_number");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_connection_number_dispose(AttributePtr attribute)
{
}

static void
sonic_connection_number_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    tributary_unit_t val = *(tributary_unit_t*)value;
    sonic_state_t* sonic_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN(card);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHDemapper);
        register_value &= ~0x1ff0000;
        register_value |= val << 16;
        card_write_iom(card, sonic_state->mSonicBase + kSDHDemapper, register_value);
    }
}

static void*
sonic_connection_number_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHDemapper);
        register_value &= 0x1ff0000;
        register_value >>= 16;
        value = register_value;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_connection_number_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_payload_mapping(void)
{
    AttributePtr result = attribute_init(kUint32AttributePayloadMapping); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_payload_mapping_dispose);
        attribute_set_post_initialize_routine(result, sonic_payload_mapping_post_initialize);
        attribute_set_setvalue_routine(result, sonic_payload_mapping_set_value);
        attribute_set_getvalue_routine(result, sonic_payload_mapping_get_value);
        attribute_set_to_string_routine(result, sonic_payload_mapping_to_string);
        attribute_set_from_string_routine(result, sonic_payload_mapping_from_string);
        attribute_set_name(result, "payload_mapping");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
sonic_payload_mapping_to_string(AttributePtr attribute)
{
    payload_mapping_t value = *(payload_mapping_t*)sonic_payload_mapping_get_value(attribute);
    const char* temp = payload_mapping_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}

static void
sonic_payload_mapping_from_string(AttributePtr attribute, const char* string)
{
    payload_mapping_t value = string_to_payload_mapping(string);
    sonic_payload_mapping_set_value(attribute, (void*)&value, sizeof(payload_mapping_t));    
}

static void
sonic_payload_mapping_dispose(AttributePtr attribute)
{
}

static void
sonic_payload_mapping_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    uint32_t val = *(uint32_t*)value;
    sonic_state_t* sonic_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN(card);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHDemapper);
        register_value &= ~(BIT30 | BIT29 | BIT28);
        register_value |= val << 28;
        register_value |= BIT31; /* set the change payload mapping bit */
        card_write_iom(card, sonic_state->mSonicBase + kSDHDemapper, register_value);
    }
}

static void*
sonic_payload_mapping_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kSDHDemapper);
        register_value &= (BIT30 | BIT29 | BIT28);
        register_value >>= 28;
        value = register_value;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_payload_mapping_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_b1_error_count(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB1ErrorCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_b1_error_count_dispose);
        attribute_set_post_initialize_routine(result, sonic_b1_error_count_post_initialize);
        attribute_set_getvalue_routine(result, sonic_b1_error_count_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "b1_error_count");
        attribute_set_description(result, "The number of SONET/SDH section bit interleaved parity 1 errors seen.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
sonic_b1_error_count_dispose(AttributePtr attribute)
{
}

static void*
sonic_b1_error_count_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        /* read the count */
        register_value = card_read_iom(card, sonic_state->mSonicBase + kB1ErrorCounter);
        value = register_value;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_b1_error_count_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_b2_error_count(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB2ErrorCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_b2_error_count_dispose);
        attribute_set_post_initialize_routine(result, sonic_b2_error_count_post_initialize);
        attribute_set_getvalue_routine(result, sonic_b2_error_count_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "b2_error_count");
        attribute_set_description(result, "The number of SONET/SDH section bit interleaved parity 2 errors seen.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
sonic_b2_error_count_dispose(AttributePtr attribute)
{
}

static void*
sonic_b2_error_count_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kB2ErrorCounter);
        value = register_value;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static void
sonic_b2_error_count_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_b3_error_count(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB3ErrorCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_b3_error_count_dispose);
        attribute_set_post_initialize_routine(result, sonic_b3_error_count_post_initialize);
        attribute_set_getvalue_routine(result, sonic_b3_error_count_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "b3_error_count");
        attribute_set_description(result, "The number of SONET/SDH section bit interleaved parity 3 errors seen.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
sonic_b3_error_count_dispose(AttributePtr attribute)
{
}

static void*
sonic_b3_error_count_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kB3ErrorCounter);
        value = register_value;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_b3_error_count_post_initialize(AttributePtr attribute)
{
}


static AttributePtr
sonic_get_new_data_pointer(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDataPointer); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_data_pointer_dispose);
        attribute_set_post_initialize_routine(result, sonic_data_pointer_post_initialize);
        attribute_set_getvalue_routine(result, sonic_data_pointer_get_value);
        attribute_set_setvalue_routine(result, sonic_data_pointer_set_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_name(result, "data_pointer");
        attribute_set_description(result, "Indicates which data byte to read for J0 path label and J1 path label.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;

}
static void
sonic_data_pointer_dispose(AttributePtr attribute)
{
}

static void*
sonic_data_pointer_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kLabels);
        value = register_value & (BIT0 | BIT1 | BIT2 | BIT3);
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_data_pointer_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t val = *(uint32_t*)value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN(card);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kLabels);
        register_value &= ~(BIT0 | BIT1 | BIT2 | BIT3);
        register_value |= val;
        card_write_iom(card, sonic_state->mSonicBase + kLabels, register_value);
    }
}

static void
sonic_data_pointer_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_c2_path_label(void)
{
    AttributePtr result = attribute_init(kUint32AttributeC2PathLabel); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_c2_path_label_dispose);
        attribute_set_post_initialize_routine(result, sonic_c2_path_label_post_initialize);
        attribute_set_getvalue_routine(result, sonic_c2_path_label_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "c2_path_label");
        attribute_set_description(result, "The SONET/SDH C2 path label byte");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;

}
static void
sonic_c2_path_label_dispose(AttributePtr attribute)
{
}

static void*
sonic_c2_path_label_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kLabels);
        value = register_value & 0xff00;
        value >>= 8;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_c2_path_label_post_initialize(AttributePtr attribute)
{
}

static AttributePtr sonic_get_new_txv5_signal_label(void){
 AttributePtr result = attribute_init(kUint32AttributeTXV5SignalLabel); 
    if (NULL != result)
    {
        attribute_set_post_initialize_routine(result,sonic_txv5_signal_label_post_initialize);
        attribute_set_getvalue_routine(result, sonic_txv5_signal_label_get_value);
        attribute_set_setvalue_routine(result,sonic_txv5_signal_label_set_value);
		attribute_set_name(result, "txv5_signal_label");
        attribute_set_description(result, "TX V5 Signal Label");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}
static void sonic_txv5_signal_label_post_initialize(AttributePtr attribute){
}
static void sonic_txv5_signal_label_set_value(AttributePtr attribute, void* value, int length){
	DagCardPtr card;
	ComponentPtr component;
	sonic_state_t* sonic_state = NULL;
	uint32_t register_value;
	uint32_t word_to_write = 0;
	if(!check_valid_txv5_value_range(*(txv5_value_t *)value))
		dagutil_warning("Unknown value for TXV5 byte. Attempting to write nevertheless.\n");
	card = attribute_get_card(attribute);
	assert(card!=NULL);
	component = attribute_get_component(attribute);
	assert(component!=NULL);
    sonic_state = component_get_private_state(component);	
	register_value = card_read_iom(card, sonic_state->mSonicBase+0x24);
	register_value &= 0xFFF8FFFF;		//set the [18:16] values equal to zero first
	word_to_write = ((*(txv5_value_t *)value)<<16)|register_value;	
	card_write_iom(card, sonic_state->mSonicBase+0x24,word_to_write);
}
static void* sonic_txv5_signal_label_get_value(AttributePtr attribute){
	DagCardPtr card;
	ComponentPtr component;
	sonic_state_t* sonic_state = NULL;
	uint32_t register_value;
	card = attribute_get_card(attribute);
    assert(card!=NULL);
	component = attribute_get_component(attribute);
	assert(component!=NULL);
    sonic_state = component_get_private_state(component);	
	register_value = card_read_iom(card, sonic_state->mSonicBase+0x24);	
	register_value &=0x70000;		//We are interested in bits [18:16]	
	register_value >>= 16;
	attribute_set_value_array(attribute, &register_value, sizeof(register_value));
    return (void *)attribute_get_value_array(attribute);
}
static bool 
check_valid_txv5_value_range(txv5_value_t value){
	int i;
	int num_of_values = 4;
	int valid_txv5_values[] =  {
	kTXV5ValueDisabled,
    kTXV5ValueAsync,			
    kTXV5ValueBitSync,
    kTXV5ValueByteSync
	};
	for(i=0;i<num_of_values;i++)
		if(valid_txv5_values[i]==value)
			return true;
	return false;
}
static AttributePtr
sonic_get_new_j0_path_label(void)
{
    AttributePtr result = attribute_init(kUint32AttributeJ0PathLabel); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_j0_path_label_dispose);
        attribute_set_post_initialize_routine(result, sonic_j0_path_label_post_initialize);
        attribute_set_getvalue_routine(result, sonic_j0_path_label_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "j0_path_label");
        attribute_set_description(result, "The Section trace byte specified by data pointer from the virtual container specified by vc index.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;

}
static void
sonic_j0_path_label_dispose(AttributePtr attribute)
{
}

static void*
sonic_j0_path_label_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kLabels);
        value = register_value & 0xff0000;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static void
sonic_j0_path_label_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_j1_path_label(void)
{
    AttributePtr result = attribute_init(kUint32AttributeJ1PathLabel); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_j1_path_label_dispose);
        attribute_set_post_initialize_routine(result, sonic_j1_path_label_post_initialize);
        attribute_set_getvalue_routine(result, sonic_j1_path_label_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "j1_path_label");
        attribute_set_description(result, "The Path trace byte specified by data pointer from the virtual container specified by vc index.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;

}
static void
sonic_j1_path_label_dispose(AttributePtr attribute)
{
}

static void*
sonic_j1_path_label_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    uint32_t register_value;
    sonic_state_t* sonic_state = NULL;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kLabels);
        value = register_value & 0xff0000;
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_j1_path_label_post_initialize(AttributePtr attribute)
{
}


static AttributePtr
sonic_get_new_refresh_cache_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRefreshCache); 
    
    if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_refresh_cache_set_value);
		attribute_set_getvalue_routine(result, sonic_refresh_cache_get_value);
        attribute_set_name(result, "refresh_cache");
		attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_description(result, "Refresh the cache that stores the SDH status.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
sonic_refresh_cache_set_value(AttributePtr attribute, void* value, int length)
{
	if (1 == valid_attribute(attribute))
	{
		if (*(uint8_t*)value == 1)
		{
			sonic_state_t* state = NULL;
			ComponentPtr comp = attribute_get_component(attribute);
			DagCardPtr card = attribute_get_card(attribute);
			state = component_get_private_state(comp);
			state->mStatusCache = card_read_iom(card, state->mSonicBase + kSDHStatus);
		}
	}
}
static void*
sonic_refresh_cache_get_value(AttributePtr attribute)
{
	uint32_t card_value;
	uint8_t result = 0;		//off by default
	if (1 == valid_attribute(attribute))
	{
			sonic_state_t* state = NULL;
			ComponentPtr comp = attribute_get_component(attribute);
			DagCardPtr card = attribute_get_card(attribute);
			state = component_get_private_state(comp);
			card_value = card_read_iom(card, state->mSonicBase + kSDHStatus);
			if(card_value==state->mStatusCache)
				result =1;	//cache is consistent with the value in the card
			else
				result =0;
	}
    attribute_set_value_array(attribute, &result, sizeof(result));
    return (void *)attribute_get_value_array(attribute);
}


static AttributePtr
sonic_get_new_deframer_revision_id_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDeframerRevisionID); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_deframer_revision_id_dispose);
        attribute_set_post_initialize_routine(result, sonic_deframer_revision_id_post_initialize);
        attribute_set_getvalue_routine(result, sonic_deframer_revision_id_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "deframer_revision_id");
        attribute_set_description(result, "Retrieve the revision id of the deframer");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_deframer_revision_id_dispose(AttributePtr attribute)
{
}

static void*
sonic_deframer_revision_id_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t revision_id;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase);
        revision_id = (register_value & (BIT0 | BIT1 | BIT2 | BIT3));
        attribute_set_value_array(attribute, &revision_id, sizeof(revision_id));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_deframer_revision_id_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_conn_VC_Label_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeConnectionVCLabel); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_conn_VC_Label_dispose);
        attribute_set_post_initialize_routine(result, sonic_conn_VC_Label_post_initialize);
        attribute_set_getvalue_routine(result, sonic_conn_VC_Label_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "connection_vc_label");
        attribute_set_description(result, "Retrieve the VC Label for the connection specified");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_conn_VC_Label_dispose(AttributePtr attribute)
{
}

static void*
sonic_conn_VC_Label_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t vc_label;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kE1T1DeframerStatus);
        vc_label = ((register_value & (BIT12 | BIT13 | BIT14 )) >> 12);
        attribute_set_value_array(attribute, &vc_label, sizeof(vc_label));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_conn_VC_Label_post_initialize(AttributePtr attribute)
{
}

static AttributePtr
sonic_get_new_conn_VC_pointer_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeConnectionVCPointer); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, sonic_conn_VC_pointer_dispose);
        attribute_set_post_initialize_routine(result, sonic_conn_VC_pointer_post_initialize);
        attribute_set_getvalue_routine(result, sonic_conn_VC_pointer_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "connection_vc_pointer");
        attribute_set_description(result, "Retrieve the VC pointer for the connection specified");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
sonic_conn_VC_pointer_dispose(AttributePtr attribute)
{
}

static void*
sonic_conn_VC_pointer_get_value(AttributePtr attribute)
{
    ComponentPtr sonic;
    DagCardPtr card;
    sonic_state_t* sonic_state = NULL;
    uint32_t vc_pointer;
    uint32_t register_value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        NULL_RETURN_WV(card, NULL);;
        sonic = attribute_get_component(attribute);
        sonic_state = component_get_private_state(sonic);
        register_value = card_read_iom(card, sonic_state->mSonicBase + kE1T1DeframerStatus);
        vc_pointer = ((register_value & (BIT8 | BIT9 )) >> 8);
        attribute_set_value_array(attribute, &vc_pointer, sizeof(vc_pointer));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
sonic_conn_VC_pointer_post_initialize(AttributePtr attribute)
{
}

/*
Return the maximum number of virtual containers used.
This number depends on the cards configuration
*/
static int
sonic_get_vc_max(ComponentPtr sonic)
{
    sonic_state_t* sonic_state = component_get_private_state(sonic);
    uint32_t register_value;
    uint32_t line_rate;
    uint32_t vc_size;
    int vc_max = 0;
    DagCardPtr card;

    card = component_get_card(sonic);
        NULL_RETURN_WV(card, 0);;
    register_value = card_read_iom(card, sonic_state->mSonicBase);
    line_rate = register_value & (BIT4 | BIT5);
    vc_size = register_value & (BIT8 | BIT9);
    if (line_rate == 0 || line_rate == BIT5) /* OC3 or STM1 */
    {
        if (vc_size == 0)
            vc_max = 3;
        else
            vc_max = 1;
    }
    else /* OC12 or STM4 */
    {
        if (vc_size == 0)
            vc_max = 12;
        else if (vc_size == BIT8)
            vc_max = 4;
        else
            vc_max = 1;
    }

    return vc_max;
}

static AttributePtr
sonic_get_new_txc2_path_label0(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel0); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 0;
	if (NULL != result)
    {
		attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label0");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 0.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static void
sonic_txc2_path_label0_set_value(AttributePtr attribute, void* value, int length)
{
	uint16_t byte_index;
	uint16_t byte_value;
	if (1 == valid_attribute(attribute))
    {
		byte_index = *(uint16_t*)attribute_get_private_state(attribute);      
		byte_value = *(uint16_t*)value;
		sonic_txc2_path_label_set_value(attribute,byte_index,byte_value);
	}
}
static void*
sonic_txc2_path_label0_get_value(AttributePtr attribute)
{
int byte_value;
uint16_t byte_index;
	if (1 == valid_attribute(attribute))
    {
		byte_index = *(uint16_t*)attribute_get_private_state(attribute);      
		byte_value = sonic_txc2_path_label_get_value(attribute,byte_index);
	}
    attribute_set_value_array(attribute, &byte_value, sizeof(byte_value));
        return (void *)attribute_get_value_array(attribute);
}
static AttributePtr
sonic_get_new_txc2_path_label1(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel1); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 1;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label1");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 1.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label2(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel2); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 2;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label2");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 2.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label3(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel3); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 3;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label3");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 3.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label4(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel4); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 4;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label4");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 4.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label5(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel5); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 5;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label5");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 5.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label6(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel6); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 6;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label6");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 6.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label7(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel7); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 7;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label7");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 7.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label8(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel8); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 8;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label8");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 8.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label9(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel9); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 9;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label9");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 9.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label10(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel10); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 10;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label10");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 10.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static AttributePtr
sonic_get_new_txc2_path_label11(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTXC2PathLabel11); 
     uint16_t* index = (uint16_t*)malloc(sizeof(uint16_t));
     *index = 11;
	if (NULL != result)
    {
        attribute_set_setvalue_routine(result, sonic_txc2_path_label0_set_value);
		attribute_set_getvalue_routine(result, sonic_txc2_path_label0_get_value);
        attribute_set_name(result, "txc2_path_label11");
        attribute_set_description(result, "Attribute to set and get transmit C2 byte at index 11.");
		attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
		attribute_set_private_state(result, index);		//this is to store the byte index so that it can be quickly retrieved.
        return result;
    }
    return NULL;
}
static void
sonic_txc2_path_label_set_value(AttributePtr attribute, uint16_t byte_index, uint16_t byte_value)
{
	DagCardPtr card;
	ComponentPtr root_component, component; 
	AttributePtr attr; 
	uint32_t  linerate;
	uint32_t  vc_size; 
	sonic_state_t* sonic_state = NULL;
	uint16_t register_value;
	uint16_t word_to_write = 0;
	if(!check_valid_txc2_value_range((txc2_value_t)byte_value))
		dagutil_warning("Unknown value for TXC2 byte. Attempting to write nevertheless.");
	card = attribute_get_card(attribute);
	root_component = card_get_root_component(card);
	component = component_get_subcomponent(root_component, kComponentPort, 0);
	assert(component != NULL);
	attr = component_get_attribute(component, kUint32AttributeLineRate);
	assert(attr != kNullAttributeUuid);
	linerate = (uint32_t)dag_config_get_uint32_attribute((dag_card_ref_t)card,(attr_uuid_t)attr);
	component = attribute_get_component(attribute);
	attr = component_get_attribute(component, kUint32AttributeVCSize);
	assert(attr != kNullAttributeUuid);
	vc_size = (uint32_t)dag_config_get_uint32_attribute((dag_card_ref_t)card,(attr_uuid_t)attr);
		if(kLineRateOC3c == linerate){
            switch(vc_size)
			{
				case kVC3: 
						if(byte_index>2)
							dagutil_panic("Invalid byte index %d for VC3.\n", byte_index);
						break;
				case kVC4: 
						if(byte_index!=0)
							dagutil_panic("Invalid byte index %d for VC4.\n", byte_index);
						break;
				case kVC4C: 
						dagutil_panic("Attribute not defined for VC4C.");
						break;
				default: 
						dagutil_panic("Could not determine the VC size.");
			}//switch 
		}//linerate=0c3c
		else if (kLineRateOC12c == linerate){
			switch(vc_size)
			{
                case kVC3: 
                      if(byte_index>11)
                        dagutil_panic("Invalid byte index %d for VC3.\n", byte_index);
                      break;
                case kVC4: 
						if(byte_index>3)
							dagutil_panic("Invalid byte index %d for VC4.\n", byte_index);
						break;
				case kVC4C: 
						if(byte_index!=0)
							dagutil_panic("Invalid byte index %d for VC4C.\n", byte_index);
						break;
				default: 
						dagutil_panic("Could not determine the VC size.");
			}//switch
		}//linerate==0c12c
		else
			dagutil_panic("Could not determine the line rate");
	sonic_state = component_get_private_state((ComponentPtr)component);	//component still points to sonic
	register_value = card_read_iom(card, sonic_state->mSonicBase+0x1C);		//reading from register 1C
	word_to_write = byte_index|(1<<4)|(byte_value<<8);
	register_value &= 0xFFFF0000;		//set the [15:0] values equal to zero first
	word_to_write |= register_value;
	card_write_iom(card, sonic_state->mSonicBase+0x1C,word_to_write);
}
static bool 
check_valid_txc2_value_range(txc2_value_t value){
	int i;
	int num_of_values = 17;
	int valid_txc2_values[] =  {
	   kTXC2Unequiped,
       kTXC2Reserved,
       kTXC2TUG,
       kTXC2LockedTU,
       kTXC2E3T3,
       kTXC2E4,
       kTXC2ATM,
       kTXC2DQDB,
       kTXC2FDDI,
       kTXC2HDLC,
       kTXC2SDL,
       kTXC2HDLS,
       kTXC210GE,
       kTXC2IPinPPP,
       kTXC2PDI,   
       kTXC2TestSignalMapping,
       kTXC2AIS
	};
	for(i=0;i<num_of_values;i++)
		if(valid_txc2_values[i]==value)
			return true;
	return false;
}
static int 
sonic_txc2_path_label_get_value(AttributePtr attribute, uint16_t byte_index)
{
	DagCardPtr card;
	ComponentPtr component, root_component;
	AttributePtr attr;
	version_t version;
	uint32_t  linerate;
	uint32_t  vc_size; 
	sonic_state_t* sonic_state = NULL;
	static uint16_t register_value;
	uint16_t word_to_write = 0;
	card = attribute_get_card(attribute);
	version = card_get_register_version(card, DAG_REG_E1T1_HDLC_DEMAP, 0);
	root_component = card_get_root_component(card);
	component = component_get_subcomponent(root_component, kComponentPort, 0);
	assert(component != NULL);
	attr = component_get_attribute(component, kUint32AttributeLineRate);
	assert(attr != kNullAttributeUuid);
	linerate = (uint32_t)dag_config_get_uint32_attribute((dag_card_ref_t)card,(attr_uuid_t)attr);
	component = attribute_get_component(attribute);
	attr = component_get_attribute(component, kUint32AttributeVCSize);
	assert(attr != kNullAttributeUuid);
	vc_size = (uint32_t)dag_config_get_uint32_attribute((dag_card_ref_t)card,(attr_uuid_t)attr);
		if(kLineRateOC3c == linerate){
			switch(vc_size)
			{
				case kVC3: 
					if(byte_index>2)										
						return kTXC2Undefined;
					break;
				case kVC4: 
					if(byte_index!=0)
						return kTXC2Undefined;	
					break;
				case kVC4C: 
						return kTXC2Undefined;
				default: 
						dagutil_panic("Could not determine the VC size.");
			}//switch 
		}//linerate=0c3c
		else if (kLineRateOC12c == linerate){
			switch(vc_size)
			{
                case kVC3: 
                       if(byte_index>11)
                            return kTXC2Undefined;
                       break;
                case kVC4: 
						if(byte_index>3)
							return kTXC2Undefined;	
						break;
				case kVC4C: 
						if(byte_index!=0)
							return kTXC2Undefined;	
						break;
				default: 
						dagutil_panic("Could not determine the VC size.");
			}//switch
		}//linerate==0c12c
		else
			dagutil_panic("Could not determine the line rate");
	sonic_state = component_get_private_state((ComponentPtr)component);	//component still points to sonic
    register_value = card_read_iom(card, sonic_state->mSonicBase+0x1C);		//reading from register 1C
	register_value &= 0xFFFF0000;		//set the [15:0] values equal to zero first
	word_to_write = byte_index;
	word_to_write |= register_value;
	card_write_iom(card, sonic_state->mSonicBase+0x1C,word_to_write);
	dagutil_microsleep(1000);	//sleep and then read the value again
	register_value = card_read_iom(card, sonic_state->mSonicBase+0x1C);		//reading from register 1C
	register_value &=0x0000FF00;	
	register_value >>=8;	
	return register_value;
}
