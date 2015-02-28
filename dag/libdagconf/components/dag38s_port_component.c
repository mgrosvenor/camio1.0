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
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/cards/dag4_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/dag38s_port_component.h"
#include "../include/components/gpp_component.h"
#include "../include/components/pbm_component.h"
#include "../include/components/gpp_component.h"
#include "../include/attribute_factory.h"

#include "dag_platform.h"
#include "dagutil.h"

#define BUFFER_SIZE 1024
#define IW_Enable   0x10000

typedef struct
{
    uint32_t mBase;
    int mIndex;
} port_state_t;

typedef struct
{
    const char name[16];
    const char description[64];
} counter_entry_t;

static counter_entry_t counter_info[] = 
{
    {"b1_error",	"Count of B1 parity errors."},
    {"b2_error",	"Count of B2 parity errors."},
    {"b3_error",	"Count of B3 parity errors."},
    {"atm_bad_hec",	"Count of ATM cells with bad HEC."},
    {"atm_cor_hec",	"Count of ATM cells with correctable HEC."},
    {"atm_rx_idle",	"Count of received ATM idle cells."},
    {"atm_rx_cell",	"Count of received ATM cells."},
    {"pos_bad_crc",	"Count of PoS frames with bad CRC."},
    {"pos_min_error",	"Count of PoS frames with minimum packet length errors."},
    {"pos_max_error",	"Count of PoS frames with maximum packet length errors."},
    {"pos_abort",	"Count of PoS aborted frames."},
    {"pos_good_frames",	"Count of good PoS frames."},
    {"pos_rx_bytes",	"Count of received PoS bytes."}
};

enum
{
    kIntEnables = 0x8c,
    kDagConfig = 0x88,
    kFramerOffsetDeviceID = 0x00,
    kFramerOffSetRxConfig = 0x04,
    kFramerOffsetCS = 0x08,
    kFramerOffsetATMPOS = 0x0C,
    kFramerOffsetCount1 = 0x10,
    kFramerOffsetCount2 = 0x14,
    kFramerOffsetTest = 0x18,
    kFramerOffsetBIP1 = 0x20,
    kFramerOffsetBIP2 = 0x24,
    kFramerOffsetBIP3 = 0x28,
    kFramerOffsetREIL = 0x2C,
    kFramerOffsetREIP = 0x30
};


/* port component. */
static void dag38s_port_dispose(ComponentPtr component);
static void dag38s_port_reset(ComponentPtr component);
static void dag38s_port_default(ComponentPtr component);
static int dag38s_port_post_initialize(ComponentPtr component);
static dag_err_t dag38s_port_update_register_base(ComponentPtr component);

/* scramble attribute. */
static AttributePtr get_new_scramble_attribute(void);
static void scramble_dispose(AttributePtr attribute);
static void* scramble_get_value(AttributePtr attribute);
static void scramble_set_value(AttributePtr attribute, void* value, int length);
static void scramble_post_initialize(AttributePtr attribute);

/* oof attribute. */
static AttributePtr get_new_oof_attribute(void);
static void oof_dispose(AttributePtr attribute);
static void* oof_get_value(AttributePtr attribute);
static void oof_post_initialize(AttributePtr attribute);

/* lof attribute. */
static AttributePtr get_new_lof_attribute(void);
static void lof_dispose(AttributePtr attribute);
static void* lof_get_value(AttributePtr attribute);
static void lof_post_initialize(AttributePtr attribute);

/* los attribute. */
static AttributePtr get_new_los_attribute(void);
static void los_dispose(AttributePtr attribute);
static void* los_get_value(AttributePtr attribute);
static void los_post_initialize(AttributePtr attribute);

/* lop attribute. */
static AttributePtr get_new_lop_attribute(void);
static void lop_dispose(AttributePtr attribute);
static void* lop_get_value(AttributePtr attribute);
static void lop_post_initialize(AttributePtr attribute);

/* bip1 attribute. */
static AttributePtr get_new_bip1_attribute(void);
static void bip1_dispose(AttributePtr attribute);
static void* bip1_get_value(AttributePtr attribute);
static void bip1_post_initialize(AttributePtr attribute);

/* bip2 attribute. */
static AttributePtr get_new_bip2_attribute(void);
static void bip2_dispose(AttributePtr attribute);
static void* bip2_get_value(AttributePtr attribute);
static void bip2_post_initialize(AttributePtr attribute);

/* bip3 attribute. */
static AttributePtr get_new_bip3_attribute(void);
static void bip3_dispose(AttributePtr attribute);
static void* bip3_get_value(AttributePtr attribute);
static void bip3_post_initialize(AttributePtr attribute);

/* label attribute. */
static AttributePtr get_new_label_attribute(void);
static void label_dispose(AttributePtr attribute);
static void* label_get_value(AttributePtr attribute);
static void label_post_initialize(AttributePtr attribute);


/* rei attribute. */
static AttributePtr get_new_rei_attribute(void);
static void rei_dispose(AttributePtr attribute);
static void* rei_get_value(AttributePtr attribute);
static void rei_post_initialize(AttributePtr attribute);

/* rdi attribute. */
static AttributePtr get_new_rdi_attribute(void);
static void rdi_dispose(AttributePtr attribute);
static void* rdi_get_value(AttributePtr attribute);
static void rdi_post_initialize(AttributePtr attribute);

/* ais attribute. */
static AttributePtr get_new_ais_attribute(void);
static void ais_dispose(AttributePtr attribute);
static void* ais_get_value(AttributePtr attribute);
static void ais_post_initialize(AttributePtr attribute);

/* line_rate attribute. */
static AttributePtr get_new_line_rate_attribute(void);
static void line_rate_dispose(AttributePtr attribute);
static void* line_rate_get_value(AttributePtr attribute);
static void line_rate_set_value(AttributePtr attribute, void* value, int length);
static void line_rate_post_initialize(AttributePtr attribute);
static void line_rate_to_string_routine(AttributePtr attribute);
static void line_rate_from_string_routine(AttributePtr attribute, const char* string);


/* eql attribute. */
static AttributePtr get_new_eql_attribute(void);
static void eql_dispose(AttributePtr attribute);
static void* eql_get_value(AttributePtr attribute);
static void eql_set_value(AttributePtr attribute, void* value, int length);
static void eql_post_initialize(AttributePtr attribute);

/* fcl attribute. */
static AttributePtr get_new_fcl_attribute(void);
static void fcl_dispose(AttributePtr attribute);
static void* fcl_get_value(AttributePtr attribute);
static void fcl_set_value(AttributePtr attribute, void* value, int length);
static void fcl_post_initialize(AttributePtr attribute);

/* network_mode attribute. */
static AttributePtr get_new_network_mode_attribute(void);
static void network_mode_dispose(AttributePtr attribute);
static void* network_mode_get_value(AttributePtr attribute);
static void network_mode_set_value(AttributePtr attribute, void* value, int length);
static void network_mode_post_initialize(AttributePtr attribute);
static void network_mode_to_string_routine(AttributePtr attribute);
static void network_mode_from_string_routine(AttributePtr attribute, const char* string);


/* payload_scramble attribute. */
static AttributePtr get_new_payload_scramble_attribute(void);
static void payload_scramble_dispose(AttributePtr attribute);
static void* payload_scramble_get_value(AttributePtr attribute);
static void payload_scramble_set_value(AttributePtr attribute, void* value, int length);
static void payload_scramble_post_initialize(AttributePtr attribute);

/* crc attribute. */
static AttributePtr get_new_crc_attribute(void);
static void crc_dispose(AttributePtr attribute);
static void* crc_get_value(AttributePtr attribute);
static void crc_set_value(AttributePtr attribute, void* value, int length);
static void crc_post_initialize(AttributePtr attribute);
static void crc_to_string_routine(AttributePtr attribute);
static void crc_from_string_routine(AttributePtr attribute, const char* string);

/* reset attribute. */
static AttributePtr get_new_reset_attribute(void);
static void reset_dispose(AttributePtr attribute);
static void* reset_get_value(AttributePtr attribute);
static void reset_set_value(AttributePtr attribute, void* value, int length);
static void reset_post_initialize(AttributePtr attribute);

/* Latch & Clear */
static AttributePtr get_new_latch_and_clear(void);
static void latch_and_clear_dispose(AttributePtr attribute);
static void* latch_and_clear_get_value(AttributePtr attribute);
static void latch_and_clear_set_value(AttributePtr attribute, void* value, int length);
static void latch_and_clear_post_initialize(AttributePtr attribute);

/* Counter Select 1 */
static AttributePtr get_new_counter_select_1(void);
static void counter_select_1_dispose(AttributePtr attribute);
static void* counter_select_1_get_value(AttributePtr attribute);
static void counter_select_1_set_value(AttributePtr attribute, void* value, int length);
static void counter_select_1_post_initialize(AttributePtr attribute);
static void counter_select_1_to_string_routine(AttributePtr attribute);
static void counter_select_1_from_string_routine(AttributePtr attribute, const char* string);

/* Counter Select 2 */
static AttributePtr get_new_counter_select_2(void);
static void counter_select_2_dispose(AttributePtr attribute);
static void* counter_select_2_get_value(AttributePtr attribute);
static void counter_select_2_set_value(AttributePtr attribute, void* value, int length);
static void counter_select_2_post_initialize(AttributePtr attribute);
static void counter_select_2_to_string_routine(AttributePtr attribute);
static void counter_select_2_from_string_routine(AttributePtr attribute, const char* string);

/* Counter 1 */
static AttributePtr get_new_counter_1(void);
static void counter_1_dispose(AttributePtr attribute);
static void* counter_1_get_value(AttributePtr attribute);
static void counter_1_post_initialize(AttributePtr attribute);

/* Counter 2 */
static AttributePtr get_new_counter_2(void);
static void counter_2_dispose(AttributePtr attribute);
static void* counter_2_get_value(AttributePtr attribute);
static void counter_2_post_initialize(AttributePtr attribute);

/* B1 error count (from phymon, Sonic v2) */
static AttributePtr get_new_b1_error_count_phymon(void);
static void b1_error_count_phymon_dispose(AttributePtr attribute);
static void* b1_error_count_phymon_get_value(AttributePtr attribute);
static void b1_error_count_phymon_post_initialize(AttributePtr attribute);

/* B2 error count (for phymon, Sonic v2) */
static AttributePtr get_new_b2_error_count_phymon(void);
static void b2_error_count_phymon_dispose(AttributePtr attribute);
static void* b2_error_count_phymon_get_value(AttributePtr attribute);
static void b2_error_count_phymon_post_initialize(AttributePtr attribute);

/* B3 error count (for phymon, Sonic v2) */
static AttributePtr get_new_b3_error_count_phymon(void);
static void b3_error_count_phymon_dispose(AttributePtr attribute);
static void* b3_error_count_phymon_get_value(AttributePtr attribute);
static void b3_error_count_phymon_post_initialize(AttributePtr attribute);

/* Line REI count (for phymon, Sonic v2) */
static AttributePtr get_new_line_rei_count_phymon(void);
static void line_rei_count_phymon_dispose(AttributePtr attribute);
static void* line_rei_count_phymon_get_value(AttributePtr attribute);
static void line_rei_count_phymon_post_initialize(AttributePtr attribute);

/* Path REI count (for phymon, Sonic v2) */
static AttributePtr get_new_path_rei_count_phymon(void);
static void path_rei_count_phymon_dispose(AttributePtr attribute);
static void* path_rei_count_phymon_get_value(AttributePtr attribute);
static void path_rei_count_phymon_post_initialize(AttributePtr attribute);


ComponentPtr
sonic_v1_get_new_component(DagCardPtr card, uint32_t index)
{
	return dag38s_get_new_port(card, index);
}


ComponentPtr
dag38s_get_new_port(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentPort, card);

    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        port_state_t* state = (port_state_t*)malloc(sizeof(port_state_t));
        component_set_dispose_routine(result, dag38s_port_dispose);
        component_set_reset_routine(result, dag38s_port_reset);
        component_set_default_routine(result, dag38s_port_default);
        component_set_post_initialize_routine(result, dag38s_port_post_initialize);
        component_set_update_register_base_routine(result, dag38s_port_update_register_base);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;
}

static dag_err_t
dag38s_port_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = component_get_card(component);
        port_state_t* state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_SONIC_3, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static void
dag38s_port_dispose(ComponentPtr component)
{

}

static void
dag38s_port_reset(ComponentPtr component)
{
}

static void
dag38s_port_default(ComponentPtr component)
{

}

static int
dag38s_port_post_initialize(ComponentPtr component)
{
    AttributePtr scramble;
    AttributePtr line_rate;
    AttributePtr eql;
    AttributePtr fcl;
    AttributePtr network_mode;
    AttributePtr reset;
    AttributePtr payload_attribute;
    AttributePtr crc;
    AttributePtr lof;
    AttributePtr los;
    AttributePtr oof;
    AttributePtr lop;
    AttributePtr bip1;
    AttributePtr bip2;
    AttributePtr bip3;
    AttributePtr rei;
    AttributePtr rdi;
    AttributePtr ais;
    AttributePtr label;
    AttributePtr attr;
    AttributePtr latch_clear;
    AttributePtr counter_select_1;
    AttributePtr counter_select_2;
    AttributePtr counter_1;
    AttributePtr counter_2;
    AttributePtr b1_error_count_phymon;
    AttributePtr b2_error_count_phymon;
    AttributePtr b3_error_count_phymon;
    AttributePtr reil_phymon;
    AttributePtr reip_phymon;
    uintptr_t gpp_base = 0;
    uintptr_t address = 0;
    uint32_t mask = 0;
    GenericReadWritePtr grw = NULL;
    port_state_t* state = NULL;
    DagCardPtr card;
    int version;
    uint32_t value = 0;

    state = component_get_private_state(component);
    card = component_get_card(component);
    state->mBase = card_get_register_address(card, DAG_REG_SONIC_3, state->mIndex);
    scramble = get_new_scramble_attribute();
    line_rate = get_new_line_rate_attribute();
    eql = get_new_eql_attribute();
    fcl = get_new_fcl_attribute();
    network_mode = get_new_network_mode_attribute();
    payload_attribute = get_new_payload_scramble_attribute();
    crc = get_new_crc_attribute();
    reset = get_new_reset_attribute();
    lof = get_new_los_attribute();
    los = get_new_lof_attribute();
    oof = get_new_oof_attribute();
    lop = get_new_lop_attribute();
    bip1 = get_new_bip1_attribute();
    bip2 = get_new_bip2_attribute();
    bip3 = get_new_bip3_attribute();
    rei = get_new_rei_attribute();
    rdi = get_new_rdi_attribute();
    ais = get_new_ais_attribute();
    label = get_new_label_attribute();
    latch_clear = get_new_latch_and_clear();
    counter_select_1 = get_new_counter_select_1();
    counter_select_2 = get_new_counter_select_2();
    counter_1 = get_new_counter_1();
    counter_2 = get_new_counter_2();

    component_add_attribute(component, scramble);
    component_add_attribute(component, line_rate);
    component_add_attribute(component, eql);
    component_add_attribute(component, fcl);
    component_add_attribute(component, network_mode);
    component_add_attribute(component, payload_attribute);
    component_add_attribute(component, crc);
    component_add_attribute(component, reset);
    component_add_attribute(component, los);
    component_add_attribute(component, lof);
    component_add_attribute(component, oof);
    component_add_attribute(component, lop);
    component_add_attribute(component, bip1);
    component_add_attribute(component, bip2);
    component_add_attribute(component, bip3);
    component_add_attribute(component, rei);
    component_add_attribute(component, rdi);
    component_add_attribute(component, ais);
    component_add_attribute(component, label);
    component_add_attribute(component, latch_clear);
    component_add_attribute(component, counter_select_1);
    component_add_attribute(component, counter_select_2);
    component_add_attribute(component, counter_1);
    component_add_attribute(component, counter_2);

    /* Overwrite the names/descriptions of the counter attributes based on the corresponding counter select values */
    value = *(uint32_t *)attribute_get_value(counter_select_1);
    attribute_set_name(counter_1, counter_info[value].name);
    attribute_set_description(counter_1, counter_info[value].description);

    value = *(uint32_t *)attribute_get_value(counter_select_2);
    attribute_set_name(counter_2, counter_info[value].name);
    attribute_set_description(counter_2, counter_info[value].description);

    /* Only add the following depending on component version */
    version = card_get_register_version(card, DAG_REG_SONIC_3, 0);
    if (version == 2)
    {
        b1_error_count_phymon = get_new_b1_error_count_phymon();
        b2_error_count_phymon = get_new_b2_error_count_phymon();
        b3_error_count_phymon = get_new_b3_error_count_phymon();
        reil_phymon = get_new_line_rei_count_phymon();
        reip_phymon = get_new_path_rei_count_phymon();

        component_add_attribute(component, b1_error_count_phymon);
        component_add_attribute(component, b2_error_count_phymon);
        component_add_attribute(component, b3_error_count_phymon);
        component_add_attribute(component, reil_phymon);
        component_add_attribute(component, reip_phymon);
    }

    /* add the active and drop count components */
    gpp_base = card_get_register_address(card, DAG_REG_GPP, 0);
    if (gpp_base > 0)
    {
        address = ((uintptr_t)card_get_iom_address(card)) + gpp_base + (uintptr_t)((state->mIndex + 1)*SP_OFFSET) + (uintptr_t)SP_CONFIG;
        mask = BIT12;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        grw_set_on_operation(grw, kGRWClearBit);
        attr = attribute_factory_make_attribute(kBooleanAttributeActive, grw, &mask, 1);
        attribute_set_getvalue_routine(attr, gpp_active_get_value);
        attribute_set_setvalue_routine(attr, gpp_active_set_value);
        component_add_attribute(component, attr);

        mask = 0xffffffff;
        address = ((uintptr_t)card_get_iom_address(card)) + gpp_base + (uintptr_t)((state->mIndex + 1)*SP_OFFSET) + (uintptr_t)SP_DROP;
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        attr = attribute_factory_make_attribute(kUint32AttributeDropCount, grw, &mask, 1);
        component_add_attribute(component, attr);
    }

    return 1;
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

static void
latch_and_clear_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr comp = attribute_get_component(attribute);
    DagCardPtr card = component_get_card(comp);
    port_state_t * state = component_get_private_state(comp);
    uint32_t reg_val = *(uint32_t*)value;

    if (1 == valid_attribute(attribute))
    {
        /* Stop and clear the counters */
        //card_write_iom(card, state->mBase + kFramerOffsetCount1, 0);
        //card_write_iom(card, state->mBase + kFramerOffsetCount2, 0);

        card_write_iom(card, state->mBase + kFramerOffsetDeviceID, reg_val);	/* Global Latch counters */
        card_read_iom(card, state->mBase + kFramerOffsetDeviceID);		/* Wait for the data */
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


static AttributePtr
get_new_counter_select_1(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCounterSelect);
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, counter_select_1_dispose);
        attribute_set_post_initialize_routine(result, counter_select_1_post_initialize);
        attribute_set_getvalue_routine(result, counter_select_1_get_value);
        attribute_set_setvalue_routine(result, counter_select_1_set_value);
        attribute_set_to_string_routine(result, counter_select_1_to_string_routine);
        attribute_set_from_string_routine(result, counter_select_1_from_string_routine);
        attribute_set_name(result, "counter_select_1");
        attribute_set_description(result, "Selects the statistics to be accumulated in general purpose counter 1.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
counter_select_1_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
counter_select_1_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port = NULL;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        val = (card_read_iom(card, state->mBase + kFramerOffsetCount1) & 0xf000000) >> 24;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
counter_select_1_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr port = NULL;
    DagCardPtr card = NULL;
    port_state_t * state = NULL;
    uint32_t reg_val = *(uint32_t*)value;
    uint32_t size = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);

        size = (sizeof (counter_info)/sizeof(counter_entry_t)) - 1;
        if (reg_val > size)
        {
            fprintf(stderr, "Invalid counter select value.\n");
            return;
        }

        /* Set the statistics counter to accumulate */
        reg_val <<= 24;
        card_write_iom(card, state->mBase + kFramerOffsetCount1, reg_val);
    }
}

static void
counter_select_1_to_string_routine(AttributePtr attribute)
{
    void* temp = attribute_get_value(attribute);
    const char* string = NULL;
    counter_select_t cs;
    if (temp)
    {
        cs = *(counter_select_t*)temp;
        string = counter_select_to_string(cs);
        if (string)
            attribute_set_to_string(attribute, string);
    }
}

static void
counter_select_1_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        counter_select_t mode = string_to_counter_select(string);
        counter_select_1_set_value(attribute, (void*)&mode, sizeof(mode));
    }
}

static void
counter_select_1_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static AttributePtr
get_new_counter_select_2(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCounterSelect); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, counter_select_2_dispose);
        attribute_set_post_initialize_routine(result, counter_select_2_post_initialize);
        attribute_set_getvalue_routine(result, counter_select_2_get_value);
        attribute_set_setvalue_routine(result, counter_select_2_set_value);
        attribute_set_to_string_routine(result, counter_select_2_to_string_routine);
        attribute_set_from_string_routine(result, counter_select_2_from_string_routine);
        attribute_set_name(result, "counter_select_2");
        attribute_set_description(result, "Selects the statistics to be accumulated in general purpose counter 2.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
counter_select_2_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
counter_select_2_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port = NULL;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        val = (card_read_iom(card, state->mBase + kFramerOffsetCount2) & 0xf000000) >> 24;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
counter_select_2_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr port = NULL;
    DagCardPtr card = NULL;
    port_state_t * state = NULL;
    uint32_t reg_val = *(uint32_t*)value;
    uint32_t size = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);

        size = (sizeof (counter_info)/sizeof(counter_entry_t)) - 1;
        if (reg_val > size)
        {
            fprintf(stderr, "Invalid counter select value.\n");
            return;
        }

        /* Set the statistics counter to accumulate */
        reg_val <<= 24;
        card_write_iom(card, state->mBase + kFramerOffsetCount2, reg_val);
    }
}

static void
counter_select_2_to_string_routine(AttributePtr attribute)
{
    void* temp = attribute_get_value(attribute);
    const char* string = NULL;
    counter_select_t cs;
    if (temp)
    {
        cs = *(counter_select_t*)temp;
        string = counter_select_to_string(cs);
        if (string)
            attribute_set_to_string(attribute, string);
    }
}

static void
counter_select_2_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        counter_select_t mode = string_to_counter_select(string);
        counter_select_2_set_value(attribute, (void*)&mode, sizeof(mode));
    }
}

static void
counter_select_2_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static AttributePtr
get_new_counter_1(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCounter1); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, counter_1_dispose);
        attribute_set_post_initialize_routine(result, counter_1_post_initialize);
        attribute_set_getvalue_routine(result, counter_1_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "counter_1");
        attribute_set_description(result, "General purpose counter 1:");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
counter_1_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
counter_1_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        val = card_read_iom(card, state->mBase + kFramerOffsetCount1) & 0xffffff;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
counter_1_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}



static AttributePtr
get_new_counter_2(void)
{
    AttributePtr result = attribute_init(kUint32AttributeCounter2); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, counter_2_dispose);
        attribute_set_post_initialize_routine(result, counter_2_post_initialize);
        attribute_set_getvalue_routine(result, counter_2_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "counter_2");
        attribute_set_description(result, "General purpose counter 2:");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
counter_2_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
counter_2_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        val = card_read_iom(card, state->mBase + kFramerOffsetCount2) & 0xffffff;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
counter_2_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static AttributePtr
get_new_b1_error_count_phymon(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB1ErrorCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, b1_error_count_phymon_dispose);
        attribute_set_post_initialize_routine(result, b1_error_count_phymon_post_initialize);
        attribute_set_getvalue_routine(result, b1_error_count_phymon_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "b1_error_phymon");
        attribute_set_description(result, "Count of B1 parity errors (from phymon).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
b1_error_count_phymon_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
b1_error_count_phymon_get_value(AttributePtr attribute)
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
        register_val = card_read_iom(card, state->mBase + kFramerOffsetBIP1);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
b1_error_count_phymon_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static AttributePtr
get_new_b2_error_count_phymon(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB2ErrorCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, b2_error_count_phymon_dispose);
        attribute_set_post_initialize_routine(result, b2_error_count_phymon_post_initialize);
        attribute_set_getvalue_routine(result, b2_error_count_phymon_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "b2_error_phymon");
        attribute_set_description(result, "Count of B2 parity errors (from phymon).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
b2_error_count_phymon_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
b2_error_count_phymon_get_value(AttributePtr attribute)
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
        register_val = card_read_iom(card, state->mBase + kFramerOffsetBIP2);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
b2_error_count_phymon_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static AttributePtr
get_new_b3_error_count_phymon(void)
{
    AttributePtr result = attribute_init(kUint32AttributeB3ErrorCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, b3_error_count_phymon_dispose);
        attribute_set_post_initialize_routine(result, b3_error_count_phymon_post_initialize);
        attribute_set_getvalue_routine(result, b3_error_count_phymon_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "b3_error_phymon");
        attribute_set_description(result, "Count of B3 parity errors (from phymon).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
b3_error_count_phymon_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
b3_error_count_phymon_get_value(AttributePtr attribute)
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
        register_val = card_read_iom(card, state->mBase + kFramerOffsetBIP3);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
b3_error_count_phymon_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static AttributePtr
get_new_line_rei_count_phymon(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineREIError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, line_rei_count_phymon_dispose);
        attribute_set_post_initialize_routine(result, line_rei_count_phymon_post_initialize);
        attribute_set_getvalue_routine(result, line_rei_count_phymon_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "reil_count");
        attribute_set_description(result, "Count of Remote Error Indication (REI) Line errors (from phymon).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
line_rei_count_phymon_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
line_rei_count_phymon_get_value(AttributePtr attribute)
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
        register_val = card_read_iom(card, state->mBase + kFramerOffsetREIL);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
line_rei_count_phymon_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static AttributePtr
get_new_path_rei_count_phymon(void)
{
    AttributePtr result = attribute_init(kUint32AttributePathREIError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, path_rei_count_phymon_dispose);
        attribute_set_post_initialize_routine(result, path_rei_count_phymon_post_initialize);
        attribute_set_getvalue_routine(result, path_rei_count_phymon_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "reip_count");
        attribute_set_description(result, "Count of Remote Error Indication (REI) Path errors (from phymon).");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
path_rei_count_phymon_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
path_rei_count_phymon_get_value(AttributePtr attribute)
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
        register_val = card_read_iom(card, state->mBase + kFramerOffsetREIP);
        val = register_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
path_rei_count_phymon_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static AttributePtr
get_new_ais_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeAlarmSignal); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, ais_dispose);
        attribute_set_post_initialize_routine(result, ais_post_initialize);
        attribute_set_getvalue_routine(result, ais_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "ais");
        attribute_set_description(result, "Alarm Indication Signal");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
ais_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
ais_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
ais_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT4) == BIT4);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_rdi_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeRDIError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, rdi_dispose);
        attribute_set_post_initialize_routine(result, rdi_post_initialize);
        attribute_set_getvalue_routine(result, rdi_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "rdi");
        attribute_set_description(result, "Remote Data Indication Error");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
rdi_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
rdi_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
rdi_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT3) == BIT3);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_rei_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeREIError); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, rei_dispose);
        attribute_set_post_initialize_routine(result, rei_post_initialize);
        attribute_set_getvalue_routine(result, rei_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "rei");
        attribute_set_description(result, "Remote Error Indication");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
rei_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
rei_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
rei_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT2) == BIT2);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_label_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeC2PathLabel); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, label_dispose);
        attribute_set_post_initialize_routine(result, label_post_initialize);
        attribute_set_getvalue_routine(result, label_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_name(result, "c2_path_label");
        attribute_set_description(result, "Path signal label. Reads the SONET/SDH C2 path label byte");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}


static void
label_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
label_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
label_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint32_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = register_val & 0xff000000;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_bip3_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeB3Error); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, bip3_dispose);
        attribute_set_post_initialize_routine(result, bip3_post_initialize);
        attribute_set_getvalue_routine(result, bip3_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "bip3");
        attribute_set_description(result, "Bit Interleaved Parity byte 3 status.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
bip3_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
bip3_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
bip3_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT22) == BIT22);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_bip2_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeB2Error); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, bip2_dispose);
        attribute_set_post_initialize_routine(result, bip2_post_initialize);
        attribute_set_getvalue_routine(result, bip2_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "bip2");
        attribute_set_description(result, "Bit Interleaved Parity byte 2 status.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
bip2_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
bip2_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
bip2_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT21) == BIT21);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_bip1_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeB1Error); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, bip1_dispose);
        attribute_set_post_initialize_routine(result, bip1_post_initialize);
        attribute_set_getvalue_routine(result, bip1_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "bip1");
        attribute_set_description(result, "Bit Interleaved Parity byte 1 status.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
bip1_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
bip1_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
bip1_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT20) == BIT20);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_lop_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLossOfPointer); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, lop_dispose);
        attribute_set_post_initialize_routine(result, lop_post_initialize);
        attribute_set_getvalue_routine(result, lop_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "lop");
        attribute_set_description(result, "Loss of pointer.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
lop_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
lop_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
lop_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT19) == BIT19);
        return (void*)&val;
    }
    return NULL;
}


static AttributePtr
get_new_oof_attribute(void)
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
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT18) == BIT18);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_lof_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLossOfFrame); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, lof_dispose);
        attribute_set_post_initialize_routine(result, lof_post_initialize);
        attribute_set_getvalue_routine(result, lof_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "lof");
        attribute_set_description(result, "Loss of frame.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
lof_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
lof_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
lof_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT17) == BIT17);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_los_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeLossOfSignal); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, los_dispose);
        attribute_set_post_initialize_routine(result, los_post_initialize);
        attribute_set_getvalue_routine(result, los_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_name(result, "los");
        attribute_set_description(result, "Loss of signal.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
los_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
los_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}

static void*
los_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = ((register_val & BIT16) == BIT16);
        return (void*)&val;
    }
    return NULL;
}


static AttributePtr
get_new_scramble_attribute(void)
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
        attribute_set_description(result, "Enable or disable SONET scrambling.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
scramble_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
scramble_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
scramble_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = *(uint8_t*)value;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        if (val == 0)
        {
            /* disable scrambling */
            register_val |= BIT0;
        }
        else
        {
            register_val &= ~BIT0;
        }
        card_write_iom(card, state->mBase + kFramerOffsetCS, register_val);
    }
}


static void*
scramble_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffsetCS);
        val = !((register_val & BIT0) == BIT0);
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_line_rate_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeLineRate); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, line_rate_dispose);
        attribute_set_post_initialize_routine(result, line_rate_post_initialize);
        attribute_set_getvalue_routine(result, line_rate_get_value);
        attribute_set_setvalue_routine(result, line_rate_set_value);
        attribute_set_to_string_routine(result,line_rate_to_string_routine );
        attribute_set_from_string_routine(result, line_rate_from_string_routine);
        attribute_set_name(result, "line_rate");
        attribute_set_description(result, "Set the line rate of the card");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}


static void
line_rate_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
line_rate_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
line_rate_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    line_rate_t val = *(line_rate_t*)value;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        if (val == kLineRateOC3c)
        {
            register_val &= ~(BIT21 | BIT22);
            card_write_iom(card, state->mBase + kFramerOffSetRxConfig, register_val);
        }
        else if (val == kLineRateOC12c)
        {
            register_val &= ~(BIT21 | BIT22);
            register_val |= BIT22;
            card_write_iom(card, state->mBase + kFramerOffSetRxConfig, register_val);
        }
    }
}


static void*
line_rate_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static line_rate_t val = kLineRateInvalid;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        if ((register_val & (BIT22 | BIT21)) == BIT22)
        {
            val = kLineRateOC12c;
        }
        else if ((register_val & (BIT22 | BIT21)) == 0)
        {
            val = kLineRateOC3c;
        }
        return (void*)&val;
    }
    return NULL;
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

static void
line_rate_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        line_rate_t mode = string_to_line_rate(string);
        line_rate_set_value(attribute, (void*)&mode, sizeof(mode));
    }
}

static AttributePtr
get_new_eql_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeEquipmentLoopback); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, eql_dispose);
        attribute_set_post_initialize_routine(result, eql_post_initialize);
        attribute_set_getvalue_routine(result, eql_get_value);
        attribute_set_setvalue_routine(result, eql_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "eql");
        attribute_set_description(result, "Put the port into equipment loopback mode");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
eql_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
eql_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented method. */
    }
}


static void
eql_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = *(uint8_t*)value;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        if (val == 1)
        {
            register_val |= BIT9;
        }
        else
        {
            register_val &= ~BIT9;
        }
        card_write_iom(card, state->mBase + kFramerOffSetRxConfig, register_val);
    }
}


static void*
eql_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        val = (register_val & BIT9) == BIT9;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_fcl_attribute(void)
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
        attribute_set_description(result, "Put the port into facility loopback mode");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
fcl_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
fcl_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
fcl_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    uint8_t val = *(uint8_t*)value;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        if (val == 1)
        {
            register_val |= BIT10;
        }
        else
        {
            register_val &= ~BIT10;
        }
        card_write_iom(card, state->mBase + kFramerOffSetRxConfig, register_val);
    }
}


static void*
fcl_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static uint8_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        val = (register_val & BIT10) == BIT10;
        return (void*)&val;
    }
    return NULL;
}

static AttributePtr
get_new_network_mode_attribute(void)
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
    ComponentPtr port;
    ComponentPtr gpp;
    ComponentPtr root;
    ComponentPtr pbm;
    AttributePtr snaplen;
    AttributePtr varlen;
    uint32_t rxconfig_regsiter;
    uint32_t cs_register;
    uint32_t ap_register;
    uint32_t dagconfig = 0;
    uint32_t val;
    uint32_t int_enable;
    uint32_t snaplen_value = 0;
    uint32_t pbm_version = 0;
    uint8_t varlen_value = 1;
    network_mode_t mode = *(network_mode_t*)value;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        root = card_get_root_component(card);
        dagconfig = card_read_iom(card, kDagConfig);
        gpp = component_get_subcomponent(root, kComponentGpp, 0);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        if (mode == kNetworkModePoS)
        {
            rxconfig_regsiter = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
            rxconfig_regsiter |= BIT30;
            assert(state);
            cs_register = card_read_iom(card, state->mBase + kFramerOffsetCS);
            assert(state);
            ap_register = card_read_iom(card, state->mBase + kFramerOffsetATMPOS);
            if ((ap_register & BIT3) == BIT3)
            {
                cs_register |= (0x16 << 8);
            }
            else
            {
                cs_register |= (0xcf << 8);
            }
            dagconfig &= ~(3<<4);	/* mask bits 5:4 in 0x88 */
            card_write_iom(card, kDagConfig, dagconfig);
            card_write_iom(card, state->mBase + kFramerOffSetRxConfig, rxconfig_regsiter);

            /* Disable ARM inspection window for POS */
            int_enable = card_read_iom(card, kIntEnables);
            int_enable &= ~IW_Enable;
            card_write_iom(card, kIntEnables, int_enable);

            /* Ensure GPP record type is set appropriately */
            gpp_record_type_set_value(gpp, kGppHDLCPOS);

            /* default slen to 48 */
            snaplen = component_get_attribute(gpp, kUint32AttributeSnaplength);
            snaplen_value = 48;
            attribute_set_value(snaplen, (void*)&snaplen_value, sizeof(value));
            
            /* default to variable length mode */
            varlen = component_get_attribute(gpp, kBooleanAttributeVarlen);
            varlen_value = 1;
            attribute_set_value(varlen, (void*)&varlen_value, sizeof(varlen_value));
        }
        else if (kNetworkModeATM == mode)
        {
			/* Check for Coprocessor route source select.  ATM mode cannot be selected if this is 0x01 (in bits 13-12). */
            pbm = component_get_subcomponent(root, kComponentPbm, 0);
            pbm_version = pbm_get_version(pbm);
			val = pbm_get_global_status_register(pbm);
			if ((0 == (val & BIT13)) && (0 != (val & BIT12)))
			{
				dagutil_error("route source select is nonzero - is there an Endace Filter image loaded?  Endace Filter only supports PoS.\n");
				return;
			}

            assert(state);
            rxconfig_regsiter = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
            rxconfig_regsiter &= ~(BIT31 | BIT30);
            assert(state);
            cs_register = card_read_iom(card, state->mBase + kFramerOffsetCS);
            cs_register = 0x13 << 8;

            dagconfig &= ~(3<<4);	/* mask bits 5:4 in 0x88 */
			dagconfig |=  (1<<4);	/* ATM format for trace records */
            card_write_iom(card, kDagConfig, dagconfig);
            card_write_iom(card, state->mBase + kFramerOffSetRxConfig, rxconfig_regsiter);

			/* Enable ARM inspection window for ATM */
            int_enable = card_read_iom(card, kIntEnables);
            int_enable |= IW_Enable;
            card_write_iom(card, kIntEnables, int_enable);

			/* Ensure GPP record type is set appropriately */
			gpp_record_type_set_value(gpp, kGppATM);

            /* default slen to 52 */
            snaplen = component_get_attribute(gpp, kUint32AttributeSnaplength);
            snaplen_value = 52;
            attribute_set_value(snaplen, (void*)&snaplen_value, sizeof(value));
            
            /* default to fixed length mode */
            varlen = component_get_attribute(gpp, kBooleanAttributeVarlen);
            varlen_value = 0;
            attribute_set_value(varlen, (void*)&varlen_value, sizeof(varlen_value));

        }
    }
}


static void*
network_mode_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t register_val;
    static network_mode_t val = 0;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = (port_state_t*)component_get_private_state(port);
        register_val = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        if ((register_val & BIT30) == BIT30)
        {
            val = kNetworkModePoS;
        }
        else if ((register_val & (BIT31 | BIT30)) == 0)
        {
            val = kNetworkModeATM;
        }
        else if ((register_val & BIT31) == BIT31)
        {
            val = kNetworkModeRAW;
        }
        else
        {
            val = kNetworkModeInvalid;
        }
        return (void*)&val;
    }
    return NULL;
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


static AttributePtr
get_new_payload_scramble_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributePayloadScramble); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, payload_scramble_dispose);
        attribute_set_post_initialize_routine(result, payload_scramble_post_initialize);
        attribute_set_getvalue_routine(result, payload_scramble_get_value);
        attribute_set_setvalue_routine(result, payload_scramble_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "pscramble");
        attribute_set_description(result, "Enable or disable payload scrambling.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}


static void
payload_scramble_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
payload_scramble_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}

static void
payload_scramble_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t cs_register;
    uint32_t ap_register;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        cs_register = card_read_iom(card, state->mBase + kFramerOffsetCS);
        ap_register = card_read_iom(card, state->mBase + kFramerOffsetATMPOS);
        if (1 == *(uint8_t*)value)
        {
            ap_register |= BIT23; 
            ap_register |= BIT3; 
            cs_register |= (0x16 << 8);
            
        }
        else
        {
            ap_register &= ~BIT23; 
            ap_register &= ~BIT3; 
            cs_register |= (0xcf << 8);
        }
        card_write_iom(card, state->mBase + kFramerOffsetCS, cs_register);
        card_write_iom(card, state->mBase + kFramerOffsetATMPOS, ap_register);
    }
}


static void*
payload_scramble_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t ap_register;
    port_state_t* state = NULL;
    static uint8_t value = 0;
    
    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        ap_register = card_read_iom(card, state->mBase + kFramerOffsetATMPOS);
        value = ((ap_register &  BIT3) == BIT3);
    }
    return (void*)&value;
}

static AttributePtr
get_new_crc_attribute(void)
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
    uint32_t ap_register;
    port_state_t* state = NULL;
    crc_t crc_value = *(crc_t*)value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        ap_register = card_read_iom(card, state->mBase + kFramerOffsetATMPOS);
        if (kCrcOff == crc_value)
        {
            ap_register &= ~(BIT21 | BIT22);
            card_write_iom(card, state->mBase + kFramerOffsetATMPOS, ap_register);
        }
        else if (kCrc16 == crc_value)
        {
            ap_register &= ~(BIT21 | BIT22);
            ap_register |= BIT21;
            card_write_iom(card, state->mBase + kFramerOffsetATMPOS, ap_register);
        }
        else if (kCrc32 == crc_value)
        {
            ap_register &= ~(BIT21 | BIT22);
            ap_register |= BIT22;
            card_write_iom(card, state->mBase + kFramerOffsetATMPOS, ap_register);
        }
    }
}


static void*
crc_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t ap_register;
    port_state_t* state = NULL;
    static crc_t value = 0;
    
    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        ap_register = card_read_iom(card, state->mBase + kFramerOffsetATMPOS);
        if ((ap_register & (BIT21 | BIT22)) == 0)
        {
            value = kCrcOff;
        }
        else if ((ap_register & BIT21) == BIT21)
        {
            value = kCrc16;
        }
        else if ((ap_register & BIT22) == BIT22)
        {
            value = kCrc32;
        }
        else
        {
            value = kCrcInvalid;
        }
    }
    return (void*)&value;
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

/*  If you  look at the 3.8s manual the comment about the looptimer stuff is '(un)set looptimer. Don't touch'
* we don't want to expose this value as an attribute, however it is used 
* by the 3.8s code in dagthree default, so we only expose access to the looptime
* stuff inside the API, but don't make an attribute code for it. 
*/

void
dag38s_looptimer0_set_value(ComponentPtr port, uint8_t value)
{
    port_state_t* state = component_get_private_state(port);
    DagCardPtr card = component_get_card(port);
    uint32_t register_value = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
    if (0 == value)
    {
        register_value &= ~BIT11;
    }
    else
    {
        register_value |= BIT11;
    }
    card_write_iom(card, state->mBase + kFramerOffSetRxConfig, register_value);
}

void
dag38s_looptimer1_set_value(ComponentPtr port, uint8_t value)
{
    port_state_t* state = component_get_private_state(port);
    DagCardPtr card = component_get_card(port);
    uint32_t register_value = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
    if (0 == value)
    {
        register_value &= ~BIT13;
    }
    else
    {
        register_value |= BIT13;
    }
    card_write_iom(card, state->mBase + kFramerOffSetRxConfig, register_value);
}

/* reset attribute. */
static AttributePtr
get_new_reset_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeReset); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, reset_dispose);
        attribute_set_post_initialize_routine(result, reset_post_initialize);
        attribute_set_getvalue_routine(result, reset_get_value);
        attribute_set_setvalue_routine(result, reset_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "reset");
        attribute_set_description(result, "Reset the component.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
    }
    
    return result;
}

static void
reset_dispose(AttributePtr attribute)
{
}

static void*
reset_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t rxconfig_regsiter;
    port_state_t* state = NULL;
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        rxconfig_regsiter = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        value = ((rxconfig_regsiter & BIT23) == BIT23);
        return (void*)&value;
    }
    return NULL;
}

static void
reset_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t rxconfig_regsiter;
    port_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        rxconfig_regsiter = card_read_iom(card, state->mBase + kFramerOffSetRxConfig);
        if (*(uint8_t*)value == 0)
        {
            rxconfig_regsiter &= ~BIT23;
        }
        else
        {
            rxconfig_regsiter |= BIT23;
        }
        card_write_iom(card, state->mBase + kFramerOffSetRxConfig, rxconfig_regsiter);
    }
}

static void
reset_post_initialize(AttributePtr attribute)
{
}

