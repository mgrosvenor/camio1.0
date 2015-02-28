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
#include "../include/components/pbm_component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/xgmii_statistics_component.h"
#include "../include/attribute_factory.h"
#include "../include/components/mini_mac_statistics_component.h"

#include "../../../include/dag_platform.h"
#include "../../../include/dagutil.h"

#define BUFFER_SIZE 1024

static void xgmii_statistics_dispose(ComponentPtr component);
static void xgmii_statistics_reset(ComponentPtr component);
static void xgmii_statistics_default(ComponentPtr component);
static int xgmii_statistics_post_initialize(ComponentPtr component);

typedef struct
{
	uint32_t mIndex;
} xgmii_statistics_state_t;

typedef enum
{
	kNCSR = 0x10,
	kRXFrameCount = 0x14,
	kRXByteCountHi = 0x18,
	kRXByteCountLo = 0x1c,
	kCRCCount = 0x20,
	kShortPacketError = 0x24,
	kLongPacketError = 0x2c,
	kTXFrameCount = 0x30,
	kTXByteCountHi = 0x34,
	kTXByteCountLo = 0x38
} xgmii_statistics_offsets_t;

ComponentPtr
xgmii_statistics_get_new_component(DagCardPtr card, uint32_t index)
{
	ComponentPtr result = component_init(kComponentXGMIIStatistics, card);

	if (NULL != result)
	{
		xgmii_statistics_state_t* state = NULL;
		component_set_dispose_routine(result, xgmii_statistics_dispose);
		component_set_reset_routine(result, xgmii_statistics_reset);
		component_set_default_routine(result, xgmii_statistics_default);
		component_set_post_initialize_routine(result, xgmii_statistics_post_initialize);
		component_set_name(result, "xgmii_statistics");
		state = (xgmii_statistics_state_t*)malloc(sizeof(xgmii_statistics_state_t));
		state->mIndex = index;
		component_set_private_state(result, state);
	}

	return result;
}

static void
xgmii_statistics_dispose(ComponentPtr component)
{

	

}

static void
xgmii_statistics_reset(ComponentPtr component)
{
}

static void
xgmii_statistics_default(ComponentPtr component)
{
	
}

static int
xgmii_statistics_post_initialize(ComponentPtr component)
{
    uint32_t mask[2];
	uintptr_t address = 0;
	GenericReadWritePtr grw = NULL;
	DagCardPtr card = NULL;
	AttributePtr attr = NULL;
	xgmii_statistics_state_t* state = component_get_private_state(component);

	assert(state);
	card = component_get_card(component);
	assert(card);
    memset(mask, 0xff, sizeof(mask));
	address = (uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_XGMII, state->mIndex);

	grw = grw_init(card, address + kRXFrameCount, grw_iom_read, grw_iom_write);
	attr = attribute_factory_make_attribute(kUint32AttributeRxFrames, grw, mask, 1);
	component_add_attribute(component, attr);

	grw = grw_init(card, address + kRXByteCountHi, grw_iom_read, grw_iom_write);
	attr = attribute_factory_make_attribute(kUint64AttributeRxBytes, grw, mask, 2);
	component_add_attribute(component, attr);

	grw = grw_init(card, address + kCRCCount, grw_iom_read, grw_iom_write);
	attr = attribute_factory_make_attribute(kUint32AttributeCRCErrors, grw, mask, 1);
	component_add_attribute(component, attr);

	grw = grw_init(card, address + kShortPacketError, grw_iom_read, grw_iom_write);
	attr = attribute_factory_make_attribute(kUint32AttributeShortPacketErrors, grw, mask, 1);
	component_add_attribute(component, attr);

	grw = grw_init(card, address + kLongPacketError, grw_iom_read, grw_iom_write);
	attr = attribute_factory_make_attribute(kUint32AttributeLongPacketErrors, grw, mask, 1);
	component_add_attribute(component, attr);

	grw = grw_init(card, address + kTXFrameCount, grw_iom_read, grw_iom_write);
	attr = attribute_factory_make_attribute(kUint32AttributeTxFrames, grw, mask, 1);
	component_add_attribute(component, attr);

	grw = grw_init(card, address + kTXByteCountHi, grw_iom_read, grw_iom_write);
	attr = attribute_factory_make_attribute(kUint64AttributeTxBytes, grw, mask, 2);
	component_add_attribute(component, attr);

    /* latch attributes */
    mask[0] = BIT31| BIT30;
    grw = grw_init(card,address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCounterLatch, grw, mask, 1);
    component_add_attribute(component, attr);

	return 1;
}


