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
#include "../include/components/vsc8476_port_component.h"
#include "../include/attribute_factory.h"
#include "../include/components/sr_gpp.h"

#include "dag_platform.h"
#include "dagutil.h"
#include "dagapi.h"

#define BUFFER_SIZE 1024

typedef struct
{
    uint32_t mBase;
    int mIndex;
} port_state_t;


/* port component. */
static void vsc8476_port_dispose(ComponentPtr component);
static void vsc8476_port_reset(ComponentPtr component);
static void vsc8476_port_default(ComponentPtr component);
static int vsc8476_port_post_initialize(ComponentPtr component);
static uint32_t get_base_address(DagCardPtr card);

/* Ethernet mode */
static AttributePtr get_new_ethernet_mode(void);
static void* ethernet_mode_get_value(AttributePtr attribute);
static void ethernet_mode_set_value(AttributePtr attribute, void* value, int length);


static uintptr_t get_drop_count_address(DagCardPtr card,ComponentPtr component);
ComponentPtr
vsc8476_get_new_port(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPort, card);

    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        port_state_t* state = (port_state_t*)malloc(sizeof(port_state_t));
        component_set_dispose_routine(result, vsc8476_port_dispose);
        component_set_reset_routine(result, vsc8476_port_reset);
        component_set_default_routine(result, vsc8476_port_default);
        component_set_post_initialize_routine(result, vsc8476_port_post_initialize);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        state->mBase = get_base_address(card);
        state->mIndex = index;
        component_set_private_state(result, state);
    }

    return result;
}

static uint32_t 
get_base_address(DagCardPtr card)
{
    if (1 == valid_card(card))
    {
        int i = 0;
        uint32_t reg_ver = 0;
        uint32_t register_address = 0;
        do
        {
            register_address = card_get_register_address(card, DAG_REG_VSC8476, i);
            reg_ver = card_get_register_version(card, DAG_REG_VSC8476, i);
            i++;
            if(i >=5)
                break;
        }while (reg_ver != 0);
        return register_address;
    }
    return 0;
}

static void
vsc8476_port_dispose(ComponentPtr component)
{

    

}

static void
vsc8476_port_reset(ComponentPtr component)
{
    /* Reset */ 
    GenericReadWritePtr grw = NULL;
    DagCardPtr card = component_get_card(component);
    uint32_t address = 0;

    address = (CONTROL_1 & 0xffff) | (PMA_DEVICE << 16);
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    grw_write(grw, BIT15);    
    free(grw);
}

static void
vsc8476_port_default(ComponentPtr component)
{
//    AttributePtr attribute = NULL;
    DagCardPtr card = NULL;
    //ethernet_mode_t mode = kEthernetMode10GBase_LR;
    vsc8476_port_reset(component);

    card = component_get_card(component);
/* This should not be done */
    //attribute = component_get_attribute(component, kUint32AttributeEthernetMode);
    //attribute_set_value(attribute, (void *)&mode, sizeof(uint32_t));
    
}

static int
vsc8476_port_post_initialize(ComponentPtr component)
{
    uintptr_t address = 0;
    DagCardPtr card = NULL;
    AttributePtr attr = NULL;
    uint32_t bool_attr_mask = 0;
    GenericReadWritePtr grw = NULL;
    card = component_get_card(component);
    
    /** Config Attributes **/
    address = (VS_CONTROL_PMA & 0xffff)  | (PMA_DEVICE << 16);
    bool_attr_mask = BIT8;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    grw_set_on_operation(grw, kGRWClearBit); /* To enable fcl on this framer we clear the bit */
    attr = attribute_factory_make_attribute(kBooleanAttributeFacilityLoopback, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    address = (CONTROL_1 & 0xffff) | (PMA_DEVICE << 16);
    bool_attr_mask = BIT0;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeEquipmentLoopback, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    

    /** Status Attributes **/
    address = (STATUS_1 & 0xffff) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT2;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLink, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    address = (STATUS_1 & 0xffff) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT7;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeFault, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    address = (STATUS_2 & 0xffff ) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT11;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeTxFault, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    address = (STATUS_2 & 0xffff ) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT10;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeRxFault, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);    

    address = (PCS_STATUS_10GBASE_R_1 & 0xffff ) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT1;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeHighBitErrorRateDetected, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);      

    address = (PCS_STATUS_10GBASE_R_1 & 0xffff ) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT0;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kBooleanAttributeLock, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);      

    address = (PCS_STATUS_10GBASE_R_2 & 0xffff ) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kUint32AttributeBERCounter, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);      

    address = (PCS_STATUS_10GBASE_R_2 & 0xffff ) | (PCS_DEVICE << 16);
    bool_attr_mask = BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7;
    grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
    attr = attribute_factory_make_attribute(kUint32AttributeErrorBlockCounter, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);      

    /* calculate the address depending on GPP or SRGPP*/
    address = get_drop_count_address(card,component);
    bool_attr_mask = -1;
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    attr = attribute_factory_make_attribute(kUint32AttributeDropCount, grw, &bool_attr_mask, 1);
    component_add_attribute(component, attr);

    attr = get_new_ethernet_mode();
    component_add_attribute(component, attr);

    return 1;
}


static void
vsc8476_ethernet_mode_to_string(AttributePtr attribute)
{
    ethernet_mode_t value;
    const char* buffer;
    if (1 == valid_attribute(attribute))
    {
        value = *(ethernet_mode_t*)ethernet_mode_get_value(attribute);
        buffer = ethernet_mode_to_string(value);
        if (buffer)
            attribute_set_to_string(attribute, buffer);
    }
}

static void
vsc8476_ethernet_mode_from_string(AttributePtr attribute, const char* string)
{
    ethernet_mode_t value;
    if (1 == valid_attribute(attribute))
    {
        value = string_to_ethernet_mode(string);
        if (value != kEthernetModeInvalid)
            ethernet_mode_set_value(attribute, (void*)&value, sizeof(ethernet_mode_t));
    }
}

static AttributePtr
get_new_ethernet_mode(void)
{
    AttributePtr result = attribute_init(kUint32AttributeEthernetMode);

    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, ethernet_mode_get_value);
        attribute_set_setvalue_routine(result, ethernet_mode_set_value);
        attribute_set_to_string_routine(result, vsc8476_ethernet_mode_to_string);
        attribute_set_from_string_routine(result, vsc8476_ethernet_mode_from_string);
        attribute_set_name(result, "ethernet_mode");
        attribute_set_description(result, "Set the Ethernet mode");
        //attribute_set_config_status(result, kDagAttrConfig);
        // Only status , shouldn't be configurable
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);

    }
    return result;
}

static void*
ethernet_mode_get_value(AttributePtr attribute)
{
    uint32_t value = 0;
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = NULL;
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t address = 0;
        uint32_t register_value = 0;

        address = (CONTROL_2 & 0xffff ) | (PMA_DEVICE << 16);
        grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
        register_value = grw_read(grw);    
        grw_dispose(grw);
        if((register_value &(BIT0|BIT1|BIT2)) == (BIT0|BIT1|BIT2))
        {
               value = kEthernetMode10GBase_SR;
        }
        else if((register_value & (BIT2|BIT1)) == (BIT2|BIT1))
        {
           value = kEthernetMode10GBase_LR;
        }
        else if((register_value & (BIT2|BIT0)) == (BIT2|BIT0))
        {
           value = kEthernetMode10GBase_ER;
        } 
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
ethernet_mode_set_value(AttributePtr attribute, void* value, int len)
{
    ethernet_mode_t eth_mode = *(ethernet_mode_t*)value;

    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = NULL;
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t address = 0;
        uint32_t register_value = 0;

        address = (CONTROL_2 & 0xffff ) | (PMA_DEVICE << 16);
        grw = grw_init(card, address, grw_vsc8476_mdio_read, grw_vsc8476_mdio_write);
        register_value = grw_read(grw);    
        register_value &= ~(BIT2|BIT1|BIT0);
        switch(eth_mode)
        {
            case kEthernetMode10GBase_LR:
                register_value |= (BIT2|BIT1);
                break;
            case kEthernetMode10GBase_ER:
                register_value |= (BIT2|BIT0);
                break;
            case kEthernetMode10GBase_SR:
                register_value |= (BIT2|BIT1|BIT0);
                break;
            default:
                break;
        }
        grw_write(grw, register_value);
        grw_dispose(grw);
    }
}

uintptr_t get_drop_count_address(DagCardPtr card,ComponentPtr component)
{
    int gpp_count = 0;
    uint8_t *iom = 0;
    dag_reg_t    regs[DAG_REG_MAX_ENTRIES];
    bool is_sr_gpp = false;
    uintptr_t address = 0;
    int index = 0;
    ComponentPtr root_comp = NULL;
    uint32_t base_reg = 0;
    uint32_t offset = 0;
    int dagfd = card_get_fd(card);

    iom = dag_iom(dagfd);
    if( NULL == iom)
        return 0;

    gpp_count = dag_reg_find((char*)iom,DAG_REG_GPP,regs);
    if ( gpp_count <= 0 )
    {
        is_sr_gpp = true;
        gpp_count = dag_reg_find((char*)iom,DAG_REG_SRGPP, regs);
    }
    if ( gpp_count <= 0 )
        return 0;
    root_comp = card_get_root_component(card);
    for( index = 0;index < gpp_count; index++)
    {
        if ( component == component_get_subcomponent(root_comp, kComponentPort, index) )
                break;
    }
    if ( index >= gpp_count ) 
        return 0;
    /* now index will have the port number */
    if ( is_sr_gpp)
    {
        base_reg = card_get_register_address(card, DAG_REG_SRGPP, 0/*assuming only one GPP*/);        
        offset =   index * kSRGPPStride + kSRGPPCounter; 
    }
    else
    {
        offset =  (index + 1 ) * SP_OFFSET + SP_DROP; 
        base_reg = card_get_register_address(card, DAG_REG_GPP, 0/*assuming only one GPP*/);
    }    
    address = ((uintptr_t)card_get_iom_address(card) + base_reg  + offset);
    return address;
}
