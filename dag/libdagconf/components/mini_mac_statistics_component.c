/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* 
Component interface to the MiniMac (http://192.168.64.242/phpwiki/MiniMAC%20version%201)
Any card that uses a mini mac can use this card. The mini mac is a soft (i.e. written in VHDL) ethernet framer.
*/

#include "../include/component_types.h"
#include "../include/attribute_types.h"
#include "../include/card_types.h"
#include "../include/component.h"
#include "../include/card.h"
#include "../include/attribute.h"
#include "../include/attribute_factory.h"
#include "../include/components/mini_mac_statistics_component.h"
#include "../include/modules/latch.h"
#include "../include/modules/generic_read_write.h"

/* dag api headers */
#include "dag_platform.h"
#include "dagreg.h"
#include "dagutil.h"

typedef enum
{
    kTXFrameCount = 0x4,
    kTXByteCountHi = 0x8,
    kTXByteCountLow = 0xc,
    kNCSR = 0x10,
    kRXFrameCount = 0x14,
    kRXByteCountHi = 0x18,
    kRXByteCountLo = 0x1c,
    kCRCErrorCount = 0x20,
    kRemoteErrorCount = 0x24,
    kBadSymbolCount = 0x28,
    kConfigSeqCount = 0x2c,
} mini_mac_statistics_offset_t;


static void mini_mac_statistics_dispose(ComponentPtr component);
static void mini_mac_statistics_reset(ComponentPtr component);
static void mini_mac_statistics_default(ComponentPtr component);
static int mini_mac_statistics_post_initialize(ComponentPtr component);

ComponentPtr
mini_mac_statistics_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentMiniMacStatistics, card); 
    mini_mac_statistics_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, mini_mac_statistics_dispose);
        component_set_post_initialize_routine(result, mini_mac_statistics_post_initialize);
        component_set_reset_routine(result, mini_mac_statistics_reset);
        component_set_default_routine(result, mini_mac_statistics_default);
        component_set_name(result, "mini_mac_statistics");
        component_set_description(result, "The MiniMAC statistics component");
        state = (mini_mac_statistics_state_t*)malloc(sizeof(mini_mac_statistics_state_t));
		memset(state, 0, sizeof(*state));
		state->mIndex = index;
        component_set_private_state(result, state);
    }
    return result;
}

static void
mini_mac_statistics_dispose(ComponentPtr component)
{
}

static void
mini_mac_statistics_reset(ComponentPtr component)
{
}

static void
mini_mac_statistics_default(ComponentPtr component)
{
}

static int
mini_mac_statistics_post_initialize(ComponentPtr component)
{
    AttributePtr attr = NULL;
    mini_mac_statistics_state_t* state = component_get_private_state(component);
    uint32_t mask[2];
    DagCardPtr card = component_get_card(component);
    GenericReadWritePtr grw = NULL;
    LatchPtr latch = NULL;
    uintptr_t address = 0;

    state->mMiniMacBase = card_get_register_address(card, DAG_REG_MINIMAC, state->mIndex);    
    address = ((uintptr_t)card_get_iom_address(card)) + state->mMiniMacBase;
    
    memset(mask, 0xff, sizeof(mask));
    grw = grw_init(card, address + kTXFrameCount, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeTxFrames, grw, mask, 1);
    latch = latch_init(card, address + kNCSR, BIT31);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    grw = grw_init(card, address + kTXByteCountHi, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint64AttributeTxBytes, grw, mask, 2);
    latch = latch_init(card, address + kNCSR, BIT31);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    grw = grw_init(card, address + kRXFrameCount, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeRxFrames, grw, mask, 1);
    latch = latch_init(card, address + kNCSR, BIT30);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    grw = grw_init(card, address + kRXByteCountHi, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint64AttributeRxBytes, grw, mask, 2);
    latch = latch_init(card, address + kNCSR, BIT30);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    grw = grw_init(card, address + kCRCErrorCount, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeCRCErrors, grw, mask, 1);
    latch = latch_init(card, address + kNCSR, BIT30);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    grw = grw_init(card, address + kRemoteErrorCount, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeRemoteErrors, grw, mask, 1);
    latch = latch_init(card, address + kNCSR, BIT30);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    grw = grw_init(card, address + kBadSymbolCount, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeBadSymbols, grw, mask, 1);
    latch = latch_init(card, address + kNCSR, BIT30);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    grw = grw_init(card, address + kConfigSeqCount, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeConfigSequences, grw, mask, 1);
    latch = latch_init(card, address + kNCSR, BIT30);
    attribute_set_latch_object(attr, latch);
    component_add_attribute(component, attr);

    /* latch attributes */
    mask[0] = BIT31| BIT30;
    grw = grw_init(card,address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCounterLatch, grw, mask, 1);
    component_add_attribute(component, attr);

    /* attributes of the new config status register (NCSR) */
    memset(mask, 0xff, sizeof(mask));
    grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRefreshCache, grw, mask, 1);
    component_add_attribute(component, attr);
    
    mask[0] = BIT24;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeSFPTxFaultCurrent,NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT25;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeSFPTxFaultEverHi, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT26;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeSFPTxFaultEverLo, NULL, mask, 1);
    component_add_attribute(component, attr);
   
    mask[0] = BIT22;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCRCErrorEverLo, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT21;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeCRCErrorEverHi, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT18;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRemoteErrorEverLo,NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT17;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRemoteErrorEverHi, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT16;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRemoteErrorCurrent, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT15;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeMiniMacLostSync,NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT14;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFramingEverLo, NULL, mask, 1);
    component_add_attribute(component, attr);
       
    mask[0] = BIT13;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFramingEverHi, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT12;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLossOfFramingCurrent, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT10;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeSFPLossOfSignalEverLo, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT9;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeSFPLossOfSignalEverHi,NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT8;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeSFPLossOfSignalCurrent, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT6;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributePeerLinkEverLo, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT5;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributePeerLinkEverHi, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT4;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributePeerLinkCurrent, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT2;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLinkEverLo,NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT1;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLinkEverHi, NULL, mask, 1);
    component_add_attribute(component, attr);

    mask[0] = BIT0;
    //grw = grw_init(card, address + kNCSR, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLinkCurrent, NULL, mask, 1);
    component_add_attribute(component, attr);

    return 1;
}

