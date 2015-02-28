/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: lm93.c,v 1.8 2007/05/18 02:12:23 vladimir Exp $
 */

/* 
This module is for communicating with components on a card via a SMBus (System Management Bus).
See http://www.smbus.org/specs for protocol specification. This is exposed by a firmware interface (RAW_SMBUS)
however we don't make this a component because it is unlikely that a user of the Config API will need
direct access to the SMBus.
*/

#include "dagutil.h"
#include "dag_platform.h"
#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/attribute_factory.h"
#include "../include/components/mini_mac_statistics_component.h"
#include "../include/modules/generic_read_write.h"

static void lm93_dispose(ComponentPtr component);
static void lm93_reset(ComponentPtr component);
static void lm93_default(ComponentPtr component);
static int lm93_post_initialize(ComponentPtr component);

//static dag_err_t lm93_smbus_test(ComponentPtr component);

enum
{
    kLM93Address = 0x58,
    kLM93Test = 0x1,
    kLM93Temperature = 0x50,
    kLM93TempFiltered = 0x54,
    kLM93FanTach1LSB = 0x6e,
    kLM93FanTach1MSB = 0x6f,
    kLM93Voltage = 0x56, 
    kLM93Config = 0xe3,
	kLM93PWM1Control = 0xc9
};

typedef struct
{
    uint32_t mIndex;
} lm93_state_t;

ComponentPtr
lm93_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentHardwareMonitor, card); 
    lm93_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, lm93_dispose);
        component_set_post_initialize_routine(result, lm93_post_initialize);
        component_set_reset_routine(result, lm93_reset);
        component_set_default_routine(result, lm93_default);
        component_set_name(result, "lm93");
        component_set_description(result, "The LM93 hardware monitor.");
        state = (lm93_state_t*)malloc(sizeof(lm93_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
        return result;
    }
    return result;
}

#if 0
static dag_err_t
lm93_smbus_test(ComponentPtr component)
{
    /* test read/write via the smbus */
    GenericReadWritePtr grw;
    DagCardPtr card = component_get_card(component);
    uint32_t address = 0;
    uint32_t value = 0;
    lm93_state_t* state = component_get_private_state(component);

    assert(state);
    address |= kLM93Address << 8;
    address |= kLM93Test;
    address |= state->mIndex << 16;
    if (card_get_card_type(card) == kDag70ge)
    {
        grw = grw_init(card, address, grw_raw_smbus_read, grw_raw_smbus_write);
    }
    else
    {
        grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
    }

    /* do a few write/reads to test*/
    grw_write(grw, 0);
    value = grw_read(grw);
    if (value != 0)
        return kDagErrGeneral;
    
    grw_write(grw, 255);
    value = grw_read(grw);
    if (value != 255)
        return kDagErrGeneral;

    grw_write(grw, 42);
    value = grw_read(grw);
    if (value != 42)
        return kDagErrGeneral;

    return kDagErrNone;
}
#endif

static void
lm93_dispose(ComponentPtr component)
{
}

static void
lm93_reset(ComponentPtr component)
{
}

static void
lm93_default(ComponentPtr component)
{
	uint32_t reg_val = 0;
	uintptr_t address = 0;
    lm93_state_t* state = component_get_private_state(component);
	DagCardPtr card = NULL;

    card = component_get_card(component);
    //FIXME: Remove the card specific shit
	if (card_get_card_type(card) == kDag50s)
	{
		GenericReadWritePtr grw = NULL;
		dagutil_verbose_level(2, "Setting the PWM1 on the LM93\n");
		address = kLM93Address << 8; /* shift 8 because the grw_read/write functions expect the device address there */
		address |= kLM93PWM1Control; 
		address |= state->mIndex << 16; /* Use the smbus interface at the given index */
		grw = grw_init(card, address, grw_drb_smbus_read, grw_drb_smbus_write);
		reg_val = grw_read(grw);
		reg_val |= BIT0;
		reg_val &= ~(BIT7 | BIT6 | BIT5 | BIT4);
		grw_write(grw, reg_val);
		grw_dispose(grw);
	}
}

/* This function exists because the value read from the voltage register needs to be converted to a percentage */
uint32_t
lm93_voltage_read_routine(GenericReadWritePtr grw)
{
    GenericReadWritePtr grw1;
    GenericReadRoutine grw_read_routine = NULL;
    uint32_t val;

    if (card_get_card_type(grw_get_card(grw)) == kDag70ge)
    {
        dagutil_verbose("lm93: Using RAW SMBus interface\n");
        grw_read_routine = grw_raw_smbus_read;
    }
    else
    {
        dagutil_verbose("lm93: Using DRB SMBus interface\n");
        grw_read_routine = grw_drb_smbus_read;
    }

    grw1 = grw_init(grw_get_card(grw), grw_get_address(grw), grw_read_routine, NULL);
    val = grw_read(grw1);
    grw_dispose(grw1);
    /* Convert to a percentage */
    return (float)val/(float)0xc0 * 100.0;
}

static int
lm93_post_initialize(ComponentPtr component)
{
    GenericReadWritePtr grw;
    uint32_t address;
    DagCardPtr card = component_get_card(component);
    AttributePtr attr;
    uint32_t mask = 0xffffffff;
    GenericReadRoutine grw_read_routine = NULL;
    GenericWriteRoutine grw_write_routine = NULL;
    lm93_state_t* state = component_get_private_state(component);
    uint32_t status;
    uint32_t retries; 

    assert(state);
    if (card_get_card_type(card) == kDag70ge)
    {
        dagutil_verbose("lm93: Using RAW SMBus interface\n");
        grw_read_routine = grw_raw_smbus_read;
        grw_write_routine = grw_raw_smbus_write;
    }
    else
    {
        dagutil_verbose("lm93: Using DRB SMBus interface\n");
        grw_read_routine = grw_drb_smbus_read;
        grw_write_routine = grw_drb_smbus_write;
    }

    address = kLM93Address << 8; /* shift 8 because the grw_read/write functions expect the device address there */
    address |= kLM93Temperature; 
    address |= state->mIndex << 16; /* Use the smbus interface index*/
    grw = grw_init(card, address, grw_read_routine, NULL);
    attr = attribute_factory_make_attribute(kUint32AttributeTemperature, grw, &mask, 1);
    component_add_attribute(component, attr);

    address = kLM93Address << 8; /* shift 8 because the grw_read/write functions expect the device address there */
    address |= kLM93Voltage; 
    address |= state->mIndex << 16; /* Use the smbus interface at the given index */
    grw = grw_init(card, address, lm93_voltage_read_routine, NULL);
    attr = attribute_factory_make_attribute(kUint32AttributeVoltage, grw, &mask, 1);
    component_add_attribute(component, attr);

    /*
    dagutil_verbose("lm93: Testing SMBus communication.\n");
    if (lm93_smbus_test(component) != kDagErrNone)
    {
        dagutil_verbose("lm93: SMBus test failed\n");
    }
    else
    {
        dagutil_verbose("lm93: SMBus test succeeded\n");
    }
    */

    address = kLM93Address << 8;
    address |= kLM93Config;
    address |= state->mIndex << 16;
    grw = grw_init(card, address, grw_read_routine, grw_write_routine);
    
    status = grw_read(grw);
    dagutil_verbose("LM93 init status: 0x%08x\n", status);
    status |= 0x01; /* start monitoring */
    grw_write(grw, status);

    retries = 0;
    do {
        status = grw_read(grw);
        dagutil_verbose("LM93 init status: 0x%08x\n", status);
        retries++;
    } while (!(status & 0x80) && retries < 100);
    /* data has been collected */ 

    grw_dispose(grw);

    return 1;
}

