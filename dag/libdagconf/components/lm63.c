/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
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
#include "../include/modules/generic_read_write.h"
#include "../include/cards/dag62se_impl.h"
#include "dag_config.h"

static void lm63_dispose(ComponentPtr component);
static void lm63_reset(ComponentPtr component);
static void lm63_default(ComponentPtr component);
static int lm63_post_initialize(ComponentPtr component);
static void smb_write(DagCardPtr card , uint32_t smb_base, unsigned char device, unsigned char address, unsigned char value);
static uint8_t check_temp(ComponentPtr component);
static void lm63_start_copro_fan(ComponentPtr component);



ComponentPtr
lm63_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentHardwareMonitor, card); 
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, lm63_dispose);
        component_set_post_initialize_routine(result, lm63_post_initialize);
        component_set_reset_routine(result, lm63_reset);
        component_set_default_routine(result, lm63_default);
        component_set_name(result, "lm63");
        component_set_description(result, "The lm63 hardware monitor.");
        return result;
    }
    return result;
}

static void
lm63_dispose(ComponentPtr component)
{
}

static void
lm63_reset(ComponentPtr component)
{
}

static void
lm63_default(ComponentPtr component)
{
    /* Start copro fan */
    /*
     * Thermal configuration
     */
    if (check_temp(component))
    {
        lm63_start_copro_fan(component);
    }

}

static int
lm63_post_initialize(ComponentPtr component)
{
    GenericReadWritePtr grw;
    uint32_t address;
    DagCardPtr card = component_get_card(component);
    dag62se_state_t * state = card_get_private_state(card);
    AttributePtr attr;
    uint32_t mask = 0xffffffff;

    address = state->mSMBBase;
    grw = grw_init(card, address, grw_lm63_smb_read, grw_lm63_smb_write);
    attr = attribute_factory_make_attribute(kUint32AttributeTemperature, grw, &mask, 1);
    component_add_attribute(component, attr);    

    return 1;
}
/*
 * check for thermal capabilities
 */
static uint8_t  
check_temp(ComponentPtr component)
{
	uint32_t val = 0;
    uint32_t pp_optics_base = 0;
    DagCardPtr card = component_get_card(component);

    pp_optics_base = card_get_register_address(card, DAG_REG_MSA300_RX, 0);

	if (pp_optics_base )
	{ 
		/* checking for smb_base is only intended for old firmware */
		/* but it won't hurt for new firmware */
        val = card_read_iom(card, pp_optics_base);
		val &= BIT27;
		if(!val)
		{
			return 1;
		}
	}
    return 0;
}

/*
 * Setup temperature parameters on LM63 part.
 * You must ensure the card has this feature
 * before calling this function.
 */
static void
lm63_start_copro_fan(ComponentPtr component)
{
    DagCardPtr card = component_get_card(component);
    dag62se_state_t * state = (dag62se_state_t*) card_get_private_state(card);
    uint32_t smb_base = state->mSMBBase;
	(void) smb_write(card, smb_base, LM63, LM63_PWM_RPM, 0x20);

	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T1, 0x00); /* 0C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P1, 0x00); /* 0% */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T2, 0x30); /* 48C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P2, 0x10); /* 25% */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T3, 0x38); /* 56C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P3, 0x20); /* 50% */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T4, 0x40); /* 64C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P4, 0x30); /* 75% */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T5, 0x50); /* 80C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P5, 0x3f); /* 100% */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T6, 0x60); /* 96C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P6, 0x3f); /* 100% */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T7, 0x70); /* 112C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P7, 0x3f); /* 100% */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_T8, 0x80); /* 128C */
	(void) smb_write(card, smb_base, LM63, LM63_LOOKUP_P8, 0x3f); /* 100% */

	(void) smb_write(card, smb_base, LM63, LM63_PWM_RPM, 0x0);
	(void) smb_write(card, smb_base, LM63, LM63_CONF, 0x01); /* trigger alarm on temperature threshold */
	(void) smb_write(card, smb_base, LM63, LM63_SPINUP, 0x25); /* spin-up at 100% for 0.8s */

	(void) smb_write(card, smb_base, LM63, LM63_LOC_HIGH, 0x55); /* 85C - local high temperature */
	(void) smb_write(card, smb_base, LM63, LM63_REM_HIGH, 0x63); /* 110C - remote high temperature */
	(void) smb_write(card, smb_base, LM63, LM63_ALERT_MASK, 0xAF); /* local and remote high */
}

void
smb_write(DagCardPtr card , uint32_t smb_base, unsigned char device, unsigned char address, unsigned char value)
{
	int safety = SAFETY_ITERATIONS;
	volatile uint32_t reg_value;

	if (smb_base == 0)
	{
		dagutil_warning("No SMB controller\n");
		return;
	}
	card_write_iom(card, smb_base + LM63_SMB_Ctrl, (((uint32_t) value << 16) | ((uint32_t) address << 8) | (uint32_t) device));

    reg_value = card_read_iom(card, smb_base + LM63_SMB_Read) & 0x2;
	while ((0 != safety) && (0 == reg_value))
	{
		dagutil_nanosleep(SAFETY_NANOSECONDS);
		safety--;
        reg_value = card_read_iom(card, smb_base + LM63_SMB_Read) & 0x2;
	}

	if (0 == reg_value)
	{
		dagutil_warning("exceeded safety counter while writing to SMBus\n");
		return;
	}
	
	return;
}

