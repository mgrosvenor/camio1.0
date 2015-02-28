/*
 * * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * * All rights reserved.
 * *
 * * This source code is proprietary to Endace Technology Limited and no part
 * * of it may be redistributed, published or disclosed except as outlined in
 * * the written contract supplied with this product.
 * *
 * */

/*Public API headers. */
/*Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/components/pbm_component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/vsc8486_port_component.h"
#include "../include/attribute_factory.h"
#include "../include/components/sr_gpp.h"
#include "../include/create_attribute.h"


#include "dag_platform.h"
#include "dagutil.h"
#include "dagapi.h"

#define BUFFER_SIZE 1024

typedef struct
{
    uint32_t mBase;
    int mIndex;
} port_state_t;
enum
{
        PMA_DEVICE  = 1,
        WIS_DEVICE  = 2,
        PCS_DEVICE  = 3
};
    

/*port component. */
static void vsc8486_port_dispose(ComponentPtr component);
static void vsc8486_port_reset(ComponentPtr component);
static void vsc8486_port_default(ComponentPtr component);
static int vsc8486_port_post_initialize(ComponentPtr component);
static uint32_t get_base_address(DagCardPtr card);

static AttributePtr vsc8486_pcs_present_attribute(GenericReadWritePtr grw, const uint32_t *bool_attr_mask, uint32_t len);
static AttributePtr vsc8486_wis_present_attribute(GenericReadWritePtr grw, const uint32_t *bool_attr_mask, uint32_t len);
#if 0
static AttributePtr vsc8486_tx_fault_attribute(GenericReadWritePtr grw, const uint32_t *bool_attr_mask, uint32_t len);
static AttributePtr vsc8486_rx_fault_attribute(GenericReadWritePtr grw, const uint32_t *bool_attr_mask, uint32_t len);
#endif
static AttributePtr vsc8486_wan_mode_attribute(GenericReadWritePtr grw, const uint32_t *bool_attr_mask, uint32_t len);
static AttributePtr vsc8486_rx_lock_detect_attribute(GenericReadWritePtr grw,const uint32_t *bool_attr_mask,uint32_t len); 
static AttributePtr vsc8486_tx_lock_detect_attribute(GenericReadWritePtr grw,const uint32_t *bool_attr_mask,uint32_t len);
static AttributePtr vsc8486_loss_of_frame_attribute(GenericReadWritePtr grw,const uint32_t *bool_attr_mask,uint32_t len);
#if 0
static AttributePtr vsc8486_alarm_signal_line_attribute(GenericReadWritePtr grw,const uint32_t *bool_attr_mask,uint32_t len);
#endif
static AttributePtr vsc8486_alarm_signal_path_attribute(GenericReadWritePtr grw,const uint32_t *bool_attr_mask,uint32_t len);
static AttributePtr vsc8486_loss_of_pointer_attribute(GenericReadWritePtr grw,const uint32_t *bool_attr_mask,uint32_t len);
static AttributePtr vsc8486_rx_c2_byte_attribute(GenericReadWritePtr grw,const uint32_t *bool_attr_mask,uint32_t len);
static AttributePtr vsc8486_tx_c2_byte_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_linetime_status_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_linetime_force_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_wan_mode_force_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_wan_override_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_refclk_force_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_refclk_override_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_los_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
#ifdef ENDACE_LABS
static AttributePtr vsc8486_clocka_select_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_clocka_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_clocka_source_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_clockb_select_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_clockb_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
#endif
static AttributePtr vsc8486_wan_master_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_lan_master_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_lan_slave_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_wan_slave_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc8486_network_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);


void* wan_master_mode_read(AttributePtr attribute);
void wan_master_mode_write(AttributePtr attribute, void* value, int len);
void lan_master_mode_write(AttributePtr attribute, void * value, int len);
void *lan_master_mode_read(AttributePtr attribute);
void lan_slave_mode_write(AttributePtr attribute, void * value, int len);
void *lan_slave_mode_read(AttributePtr attribute);
void wan_slave_mode_write(AttributePtr attribute, void * value, int len);
void *wan_slave_mode_read(AttributePtr attribute);
void network_mode_write(AttributePtr attribute,void *value,int len);
void *network_mode_read(AttributePtr attribute);




ComponentPtr
vsc8486_get_new_port(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPort, card);

    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        port_state_t* state = (port_state_t*)malloc(sizeof(port_state_t));
        component_set_dispose_routine(result, vsc8486_port_dispose);
        component_set_reset_routine(result, vsc8486_port_reset);
        component_set_default_routine(result, vsc8486_port_default);
        component_set_post_initialize_routine(result, vsc8486_port_post_initialize);
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
        
        uint32_t register_address = 0;
        
        register_address = card_get_register_address(card, DAG_REG_VSC8486, i);
        
        return register_address;
    }
    return 0;
} 
static void
vsc8486_port_dispose(ComponentPtr component)
{

} 
static void
vsc8486_port_reset(ComponentPtr component)
{
     /* Reset */ 
     GenericReadWritePtr grw = NULL;
     DagCardPtr card = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     uint32_t device_address = 0;
     port_state_t *state = NULL;

     card = component_get_card(component);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;
   	
     /*By default the card will be configured in FCL mode. 1x8000 -> 345f.*/
     address = (VSC8486_CONTROL_1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = 0x8000;
     grw_write(grw, register_value);
     grw_dispose(grw);	


}
static void
vsc8486_port_default(ComponentPtr component)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     uint32_t device_address = 0;
     port_state_t *state = NULL;
     vsc8486_port_reset(component);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x0;
     else if(state->mIndex == 0x01)
       device_address = 0x1;	


     card = component_get_card(component);
     /*By default the card will be configured in FCL mode. 1x8000 -> 345f.*/
    
     address = (VSC8486_VS_CONTROL_PMA & 0xffff ) | (PMA_DEVICE << 16) | (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = 0x355f;
     grw_write(grw, register_value);
     grw_dispose(grw);
     /*By default the card will be in LAN slave mode.We have to select the refsel -1 which selects 156.25Mhz*/	
     
     /*REFSEL_FRC - value to be used if override bit is enabled..*/ 
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16) | (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     register_value |= (BIT6 | BIT7) ;
     grw_write(grw,register_value);
     grw_dispose(grw);
}

static int
vsc8486_port_post_initialize(ComponentPtr component)
{
     uintptr_t address = 0;
     DagCardPtr card = NULL;
     AttributePtr attr = NULL;
     uint32_t bool_attr_mask = 0;
     uint32_t attr_mask = 0;
     GenericReadWritePtr grw = NULL;
     uint32_t device_address = 0;
     port_state_t *state = NULL;
     card = component_get_card(component);

     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	
	
     
     /*PMA BLOCK DEVICE ATTRIBUTES*/
     /** Config Attributes **/
     address = (VSC8486_VS_CONTROL_PMA & 0xffff)  | (PMA_DEVICE << 16) | (device_address << 21);
     bool_attr_mask = BIT8;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     grw_set_on_operation(grw, kGRWClearBit); /* To enable fcl on this framer we clear the bit */
     attr = attribute_factory_make_attribute(kBooleanAttributeFacilityLoopback, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

    #if 0
     address = (VSC8486_VS_CONTROL_PCS & 0xffff)  | (PCS_DEVICE << 16);
     bool_attr_mask = BIT3;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     grw_set_on_operation(grw, kGRWClearBit); /* To enable fcl on this framer we clear the bit */
     attr = attribute_factory_make_attribute(kBooleanAttributeFacilityLoopback, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);     
   #endif 
     
     address = (VSC8486_CONTROL_1 & 0xffff) | (PMA_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT0;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kBooleanAttributeEquipmentLoopback, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);    

     /*PMA BLOCK DEVICE ATTRIBUTES_END*/	
     /** Status Attributes **/
     address = (VSC8486_STATUS_1 & 0xffff) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT2;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kBooleanAttributeLink, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     address = (VSC8486_STATUS_1 & 0xffff) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT7;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kBooleanAttributeFault, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     address = (VSC8486_STATUS_2 & 0xffff ) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT11;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kBooleanAttributeTxFault, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     address = (VSC8486_STATUS_2 & 0xffff ) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT10;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kBooleanAttributeRxFault, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);    

     address = (VSC8486_PCS_STATUS_10GBASE_R_1 & 0xffff ) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT1;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kBooleanAttributeHighBitErrorRateDetected, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);      

     address = (VSC8486_PCS_STATUS_10GBASE_R_1 & 0xffff ) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT0;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kBooleanAttributeLock, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);      

     address = (VSC8486_PCS_STATUS_10GBASE_R_2 & 0xffff ) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT13 | BIT12 | BIT11 | BIT10 | BIT9 | BIT8;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kUint32AttributeBERCounter, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);      

     address = (VSC8486_PCS_STATUS_10GBASE_R_2 & 0xffff ) | (PCS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6 | BIT7;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = attribute_factory_make_attribute(kUint32AttributeErrorBlockCounter, grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);     

     /*PMA BLOCK DEVICE ATTRIBUTES*/
     address = (VSC8486_PMA_DEV_PKG_1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT3;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_pcs_present_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     /*WIS present*/
     address = (VSC8486_PMA_DEV_PKG_1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT2;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_wis_present_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

#if 0      	
     /*Tx Fault - Indicates a fault condition on Tx path.*/ 
     address = (VSC8486_STATUS_2 & 0xffff ) | (PMA_DEVICE << 16);
     bool_attr_mask = BIT11;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_tx_fault_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     /*Rx Fault - Indicates a fault condition on Rx path.*/
     address = (VSC8486_STATUS_2 & 0xffff ) | (PMA_DEVICE << 16);
     bool_attr_mask = BIT10;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_rx_fault_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);
#endif
     /*WAN STATUS -Indicates whether if the WAN Mode is enabled / disables  (0 / 1)*/
     address = (VSC8486_PMA_DEV_STATUS1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT15;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_wan_mode_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     /*Loss of Signal reported from PMA.Indicates that the optics can see the photons.*/
     address = (VSC8486_PMA_STAT_4 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT0;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     grw_set_on_operation(grw, kGRWClearBit); /* To enable fcl on this framer we clear the bit */
     attr = vsc8486_los_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);
     

     /*Rx Lock Error -Recieve Clock Recovery Lock Error status.Indicates that CRU can "lock" on to the incoming data.*/
     address = (VSC8486_PMA_STAT_4 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT1;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_rx_lock_detect_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     /*Tx Lock Error - Tx Clock Multiplier Lock Error status.Indicates the CMU pll can lock to the data.*/
     address = (VSC8486_PMA_STAT_4 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT2;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_tx_lock_detect_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     /*Registers to be added from the WIS block of the device*/
     /*Loss of Frame - Indicates if we have a loss of frame. - only for SONET/WAN - does not make sense for ethernet.*/
     address = (VSC8486_WIS_STATUS_3 & 0xffff ) | (WIS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT7;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_loss_of_frame_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);

     #if 0
     /*Alarm Indication Signal - Line / Path.*/
     address = (VSC8486_WIS_STATUS_3 & 0xffff ) | (WIS_DEVICE << 16);
     bool_attr_mask = BIT4;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_alarm_signal_line_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr); 
     #endif


     /*Alarm Indication Signal - Path.*/
     address = (VSC8486_WIS_STATUS_3 & 0xffff ) | (WIS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT1;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_alarm_signal_path_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr); 

     /*Loss of Pointer.*/
     address = (VSC8486_WIS_STATUS_3 & 0xffff ) | (WIS_DEVICE << 16)| (device_address << 21);
     bool_attr_mask = BIT0;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_loss_of_pointer_attribute(grw, &bool_attr_mask, 1);
     component_add_attribute(component, attr);
   
     /*C2 BYTE. for Receive */
     address = (VSC8486_WIS_MODE_CONTROL & 0xffff ) | (WIS_DEVICE << 16)| (device_address << 21);
     attr_mask = 0xff;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_rx_c2_byte_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);

     /*C2 BYTE for transmit*/	
     address = (VSC8486_WIS_TX_C2_H1 & 0xffff ) | (WIS_DEVICE << 16)| (device_address << 21);
     attr_mask = 0xff00;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_tx_c2_byte_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);
  

     /*CLOCK ADJUSTMETS ATTRIBUTES.*/
     
     /*REFSEL_OVR - enable overriding the input value and instead use the FORCE bit setting.*/ 
     /*
      * 0 - use the REFSEL0 input pin state
      * 1 - override enable.
     */
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT7;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_refclk_override_enable_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);

     /*REFSEL_FRC - value to be used if override bit is enabled..*/ 
     /*
      * 0 -  REFSEL0 = 0
      * 1 -  REFSEL1 = 1
     */
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT6;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_refclk_force_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);
	
     /*WAN OVER */ 	
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT3;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_wan_override_enable_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);

     /*wan mode force - to be used only if override is enabled.*/
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT2;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_wan_mode_force_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);	 	

     /*LINETIME FORCE - 0 - tx data synchronised to REFCLKP / WREFCLKP
      * 1 - tx data synchronised to RECOVERED CLOCK. (operating in slave mode.)
      */
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT4;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_linetime_force_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);
	
     /*LINETIME STATUS - 0 - tx data synchronised to REFCLKP / WREFCLKP
      * 1 - tx data synchronised to RECOVERED CLOCK. (operating in slave mode.)
      */  
     address = (VSC8486_PMA_DEV_STATUS1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT13;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_linetime_status_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);

     address = (WIS_DEVICE << 16)| (device_address << 21);
     attr_mask = 0xff;
     grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
     attr = vsc8486_wan_master_mode_attribute(grw,&attr_mask,1);
     component_add_attribute(component,attr);

     address = (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = 0xff;
     grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
     attr = vsc8486_lan_master_mode_attribute(grw,&attr_mask,1);
     component_add_attribute(component,attr);

     address = (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = 0xff;
     grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
     attr = vsc8486_lan_slave_mode_attribute(grw,&attr_mask,1);
     component_add_attribute(component,attr); 	
     
     address = (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = 0xff;
     grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
     attr = vsc8486_wan_slave_mode_attribute(grw,&attr_mask,1);
     component_add_attribute(component,attr); 	
     	
#ifdef ENDACE_LABS
     /*Attributes in PMA regiseter clocking.*/
     address = (VSC8486_DEV_CONTROL_5 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT9;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_clocka_enable_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);

     address = (VSC8486_DEV_CONTROL_5 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT8 | BIT7;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_clocka_select_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);
    	
     address = (VSC8486_DEV_CONTROL_5 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT6;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_clocka_source_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);

     address = (VSC8486_DEV_CONTROL_5 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT5;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_clockb_enable_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);

     address = (VSC8486_DEV_CONTROL_5 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = BIT4 | BIT3;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     attr = vsc8486_clockb_select_attribute(grw, &attr_mask, 1);
     component_add_attribute(component, attr);
#endif       

     address = (PMA_DEVICE << 16)| (device_address << 21);
     attr_mask = 0xff;
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     vsc8486_network_mode_attribute(grw,&attr_mask,1);
     component_add_attribute(component, attr);
     return 1;
}

AttributePtr 
vsc8486_pcs_present_attribute(GenericReadWritePtr grw, const uint32_t *bit_masks, uint32_t len)
{
	AttributePtr result = NULL;
    	result = attribute_init(kBooleanAttributePCSBlockPresent);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "pcs_block_present");
    	attribute_set_description(result, "Indicates if PCS block is present in the chip.");
    	attribute_set_config_status(result, kDagAttrStatus);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_masks(result,bit_masks,len);
    	return result;
}
AttributePtr
vsc8486_wis_present_attribute(GenericReadWritePtr grw, const uint32_t *bit_masks, uint32_t len)
{
        AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeWISBlockPresent);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "wis_block_present");
        attribute_set_description(result, "Indicates if WIS block is present in the chip.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
#if 0
AttributePtr
vsc8486_tx_fault_attribute(GenericReadWritePtr grw, const uint32_t *bit_masks, uint32_t len)
{
        AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeTxFault);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "tx_path_fault");
        attribute_set_description(result, "Indicates a fault in the transmit path.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
AttributePtr
vsc8486_rx_fault_attribute(GenericReadWritePtr grw, const uint32_t *bit_masks, uint32_t len)
{
        AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeRxFault);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "rx_path_fault");
        attribute_set_description(result, "Indicates a fault in the receive path.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
#endif
AttributePtr
vsc8486_wan_mode_attribute(GenericReadWritePtr grw, const uint32_t *bit_masks, uint32_t len)
{
        AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeWanMode);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "wan_mode");
        attribute_set_description(result, "Indicates if the WAN mode is active.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
AttributePtr 
vsc8486_rx_lock_detect_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeReceiveLockError);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "rx_lock_detect");
        attribute_set_description(result, "rx lock detected.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
AttributePtr 
vsc8486_tx_lock_detect_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeTransmitLockError);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "tx_lock_detect");
        attribute_set_description(result, "tx lock detected.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
AttributePtr
vsc8486_loss_of_frame_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
        AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLossOfFrame);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "loss_of_frame");
        attribute_set_description(result, "loss of frame detected.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
#if 0
AttributePtr 
vsc8486_alarm_signal_line_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLineAlarmIndicationSignal);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "line_alarm_indication_signal");
        attribute_set_description(result, "Line Alarm Indication Signal detected.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
#endif
AttributePtr 
vsc8486_alarm_signal_path_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeAlarmIndicationSignal);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "path_alarm_indication_signal");
        attribute_set_description(result, "Path Alarm Indication Signal detected.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}

AttributePtr
vsc8486_loss_of_pointer_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLossOfPointer);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "loss_of_pointer");
        attribute_set_description(result, "Loss of  Pointer detected.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
        return result;
}
AttributePtr 
vsc8486_rx_c2_byte_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{

	AttributePtr result = NULL;
        result = attribute_init(kUint32AttributeRxC2Byte);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "c2_byte_expected");
        attribute_set_description(result, "Expected C2 Receive octet.If not received PLM-P alarm is generated.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_getvalue_routine(result, attribute_uint32_get_value);
        attribute_set_setvalue_routine(result, attribute_uint32_set_value); 
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_tx_c2_byte_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{

	AttributePtr result = NULL;
        result = attribute_init(kUint32AttributeTxC2Byte);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "c2_byte_transmit");
        attribute_set_description(result, "C2 Byte to be transmitted when TOSI data is inactive.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_getvalue_routine(result, attribute_uint32_get_value);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string);
       	attribute_set_masks(result,bit_masks,len);
	return result;
}
/*Clock Attribute Settings*/
AttributePtr 
vsc8486_refclk_override_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeRefClkOverrideEnable);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "ref_clock_override");
        attribute_set_description(result, "Overrides the Referece Clock PIN Value.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}

AttributePtr 
vsc8486_refclk_force_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeRefClkForce);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "refclock_force_value");
        attribute_set_description(result, "Sets the value of the Referece clock if ref_clock_override is enabled.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}


AttributePtr 
vsc8486_wan_override_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeWANOverrideEnable);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "wan_override_enable");
        attribute_set_description(result, "Enables the overriding the input pin value and instead use the force bit setting.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}

AttributePtr 
vsc8486_wan_mode_force_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeWANModeForce);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "wan_mode_force");
        attribute_set_description(result, "Sets the value of the WAN mode if override bit is eanbled.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_linetime_force_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLineTimeForce);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "line_time_force");
        attribute_set_description(result, "Forces the device into line time mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_linetime_status_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLineTimeStatus);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "line_time_status");
        attribute_set_description(result, "Current status of the line time signal.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}

AttributePtr 
vsc8486_los_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLossOfSignal);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "loss_of_signal");
        attribute_set_description(result, "Receiver Loss Of Signal Status.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_wan_master_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeWANMasterMode);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "wan_master_mode");
        attribute_set_description(result, "Wan in master mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, wan_master_mode_read);
        attribute_set_setvalue_routine(result,wan_master_mode_write);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_lan_master_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLANMasterMode);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "lan_master_mode");
        attribute_set_description(result, "LAN in master mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, lan_master_mode_read);
        attribute_set_setvalue_routine(result, lan_master_mode_write);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string); 
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_lan_slave_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeLANSlaveMode);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "lan_slave_mode");
        attribute_set_description(result, "LAN in Slave mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, lan_slave_mode_read);
        attribute_set_setvalue_routine(result, lan_slave_mode_write);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string); 
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_wan_slave_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeWANSlaveMode);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "wan_slave_mode");
        attribute_set_description(result, "WAN in Slave mode.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, wan_slave_mode_read);
        attribute_set_setvalue_routine(result, wan_slave_mode_write);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string); 
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_network_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kUint32AttributeNetworkMode);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "network_mode_attribute");
        attribute_set_description(result, "POS, ATM, Raw or Ethernet or WAN mode if supported.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_getvalue_routine(result,network_mode_read);
        attribute_set_setvalue_routine(result,network_mode_write);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
        attribute_set_from_string_routine(result, attribute_uint32_from_string); 
        attribute_set_masks(result,bit_masks,len);
	return result;
}
void *
network_mode_read(AttributePtr attribute)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     bool value = 0;
     uint32_t device_address = 0;
     port_state_t * state = NULL;
     component = attribute_get_component(attribute);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	
     card = attribute_get_card(attribute);
     
     address = (VSC8486_PMA_DEV_STATUS1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     if ((((register_value & BIT15) >> 15 )== 0))
     {
		value = kNetworkModeEth; /*It is in Ethernet Mode or LAN mode.*/
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     else
     {
		value = kNetworkModeWAN; /*It is in Wan Mode.*/
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     return attribute_get_value_array(attribute);
}
void 
network_mode_write(AttributePtr attribute,void *value,int len)
{
	printf("Network mode configure not implemented yet. \n");
/*Not Implemented.*/
}
void*
wan_master_mode_read(AttributePtr attribute)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     bool value = 0;
     uint32_t device_address = 0;
     port_state_t * state = NULL;
     
     component = attribute_get_component(attribute);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	
     
     card = attribute_get_card(attribute);	

     address = (VSC8486_PMA_DEV_STATUS1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     if ((((register_value & BIT15) >> 15) == 1) && (((register_value & BIT13) >> 13) == 0))
     {
		value = 1;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     else
     {
		value = 0;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     return attribute_get_value_array(attribute);
}
void 
wan_master_mode_write(AttributePtr attribute, void *value,int len)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     uint32_t device_address = 0;
     port_state_t * state = NULL;
     component = attribute_get_component(attribute);	
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	
     card = attribute_get_card(attribute);

     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     register_value &= ~(BIT6 | BIT7) ;
     grw_write(grw,register_value);
     grw_dispose(grw);		

     
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     
     register_value |= (BIT2 | BIT3 | BIT7);
     register_value &= ~(BIT4 | BIT6 );
     
     grw_write(grw, register_value);
     grw_dispose(grw);
}
void 
lan_master_mode_write(AttributePtr attribute, void * value, int len)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;

     uint32_t device_address = 0;
     port_state_t * state = NULL;

     component = attribute_get_component(attribute);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	
     card = attribute_get_card(attribute);
     
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
	 
     register_value |= (BIT3);
     register_value &= ~(BIT4 | BIT2);
     
     grw_write(grw, register_value);
     grw_dispose(grw);
 
     /*REFSEL_FRC - value to be used if override bit is enabled..*/ 
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     register_value |= (BIT6 | BIT7) ;
     grw_write(grw,register_value);
     grw_dispose(grw);	


}
void *
lan_master_mode_read(AttributePtr attribute)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     bool value = 0;
     uint32_t device_address = 0;
     port_state_t * state = NULL;
     component = attribute_get_component(attribute);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	
      card = attribute_get_card(attribute);
     
     address = (VSC8486_PMA_DEV_STATUS1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     if ((((register_value & BIT15) >> 15 )== 0) && (((register_value & BIT13) >> 13 ) == 0))
     {
		value = 1;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     else
     {
		value = 0;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     return attribute_get_value_array(attribute);
}
#ifdef ENDACE_LABS
AttributePtr 
vsc8486_clocka_select_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeClkASelect);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "clock_a_select");
        attribute_set_description(result, "Select which internal clock to output on CLOCK 64A.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}


AttributePtr 
vsc8486_clocka_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeClkAEnable);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "clock_a_enable");
        attribute_set_description(result, "Enable the CLK64A output signal.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string); 
        attribute_set_masks(result,bit_masks,len);
	return result;

}

AttributePtr 
vsc8486_clocka_source_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeClkASource);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "clock_a_source");
        attribute_set_description(result, "Select the source of CLK64A_EN signal.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string); 
        attribute_set_masks(result,bit_masks,len);
	return result;
	
}


AttributePtr 
vsc8486_clockb_select_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeClkBSelect);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "clock_b_select");
        attribute_set_description(result, "Select which internal clock to output on CLOCK 64B.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;
}
AttributePtr 
vsc8486_clockb_enable_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
        result = attribute_init(kBooleanAttributeClkBSelect);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_name(result, "clock_b_enable");
        attribute_set_description(result, "Enable the CLK64B output signal.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_masks(result,bit_masks,len);
	return result;

}
#endif
void 
lan_slave_mode_write(AttributePtr attribute, void * value, int len)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     int attempts = 0;
     
     uint32_t device_address = 0;
     port_state_t * state = NULL;
     component = attribute_get_component(attribute);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	 	
     card = attribute_get_card(attribute);

     /*REFSEL_FRC - value to be used if override bit is enabled..*/ 
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     register_value |= (BIT6 | BIT7) ;
     grw_write(grw,register_value);
     grw_dispose(grw);	


     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     
     register_value |= (BIT3 | BIT4);
     register_value &= ~( BIT2);
     grw_write(grw,register_value);
     grw_dispose(grw);
	
     /*Configuration of 1xE602 register*/
     /* BIT2 - 1 - Enable the synchronous Ethernet mode.this allows us to set the clock frequencies.*/
     /* BIT5 - 1 - Enable the Clock 64B output.*/
     /* BIT3:BIT4 - 00 - output on CLK64B will be CRU divided by 64.*/	     

      address = (VSC8486_DEV_CONTROL_5 & 0xffff) | (PMA_DEVICE << 16) | (device_address << 21);
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
      register_value = grw_read(grw);
      register_value |= (BIT2 | BIT5);
      register_value &= ~(BIT3 | BIT4);
      grw_write(grw,register_value);
      grw_dispose(grw);
	
      /*Configuration of 1xE604 register*/
      /* BIT 0 -1- CRU clock is divided by 66 (This is the only valid value if using REFCLKP/N)*/  
      /* BIT 2: BIT1 - 01 - CMU clock will be divided by 16.*/
      /* BIT 4: BIT3 - 10 - Clock source to CMU should be internal CRU recovered clock. */
      /* BIT6 : BIT5 - 00 - Clock source to CRU (RREFCLK)should be divided by 64.*/
      /* ?? BIT7 - 1 - In linetime mode Clock Source for CMU is CRU recovered clock DIV by 64*/

      address = (VSC8486_SYNC_ETHERNET_CLK_CTRL & 0xffff) | (PMA_DEVICE << 16) | (device_address << 21);
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
      register_value = grw_read(grw);
      register_value |= (BIT0 | BIT1 | BIT4);
      register_value &= ~(BIT6 | BIT5);
      grw_write(grw,register_value);
      grw_dispose(grw);

	
      address = (VSC8486_WIS_INTERRUPT_STATUS & 0xffff) | (WIS_DEVICE << 16)| (device_address << 21) ; 	
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
     
      register_value = grw_read(grw); 		
      while ( (((register_value & BIT12) >> 12 ) == 1) && (attempts < 50) ) 
      { 
	   attempts++; 
	   register_value = grw_read(grw); 		
      } 
      if (attempts == 50)
		dagutil_warning("Not able to detect Rx lock timed out. \n");

      /*Seems like repetition*/
      

      address = (VSC8486_DEV_CONTROL_5 & 0xffff) | (PMA_DEVICE << 16) | (device_address << 21);
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
      register_value = grw_read(grw);
      register_value |= (BIT5);
      register_value &= ~(BIT4 | BIT3); /*CLK64B_SEL - CRU div by 64.*/
      grw_write(grw,register_value);
      grw_dispose(grw);	

    
      
      address = (VSC8486_SYNC_ETHERNET_CLK_CTRL & 0xffff) | (PMA_DEVICE << 16) | (device_address << 21);
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
      register_value = grw_read(grw);
      register_value |= (BIT4 | BIT1 );
      register_value &= ~(BIT3 | BIT2);
      grw_write(grw,register_value);
      grw_dispose(grw); 
}
void *
lan_slave_mode_read(AttributePtr attribute)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     bool value = 0;
     uint32_t device_address = 0;
     port_state_t * state = NULL;

     component = attribute_get_component(attribute);
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	     
     card = attribute_get_card(attribute);
     
     address = (VSC8486_PMA_DEV_STATUS1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     if ((((register_value & BIT15) >> 15 ) == 0) && (((register_value & BIT13)>> 13)== 1))
     {
        value = 1;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     else
     {
		value = 0;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     return attribute_get_value_array(attribute);
}

void 
wan_slave_mode_write(AttributePtr attribute, void * value, int len)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     int attempts = 0;
     uint32_t device_address = 0;
     port_state_t * state = NULL;
     
     component = attribute_get_component(attribute);

     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	
     card = attribute_get_card(attribute);

     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     register_value &= ~(BIT6 | BIT7) ;
     grw_write(grw,register_value);
     grw_dispose(grw);		
       
     address = (VSC8486_DEV_CTRL_3 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     
     register_value |= (BIT7 |BIT2 |BIT3 | BIT4);
     register_value &= ~(BIT6); 
     grw_write(grw,register_value);
     grw_dispose(grw);
	
     
     /*Configuration of 1xE602 register*/
     /* BIT2 - 1 - Enable the synchronous Ethernet mode.this allows us to set the clock frequencies.*/
     /* BIT5 - 1 - Enable the Clock 64B output.*/
     /* BIT3:BIT4 - 00 - output on CLK64B will be CRU divided by 64.*/	     
 
      address = (VSC8486_DEV_CONTROL_5 & 0xffff) | (PMA_DEVICE << 16) | (device_address << 21);
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
      register_value = grw_read(grw);
      register_value |= (BIT2); /*Enable the synchronous ethernet mode*/
      grw_write(grw,register_value);
      grw_dispose(grw);
	
      /*Configuration of 1xE604 register*/
      /* BIT 0 -1- CRU clock is divided by 66 (This is the only valid value if using REFCLKP/N)*/  
      /* BIT 2: BIT1 - 01 - CMU clock will be divided by 16.*/
      /* BIT 4: BIT3 - 10 - Clock source to CMU should be internal CRU recovered clock. */
      /* BIT6 : BIT5 - 00 - Clock source to CRU (RREFCLK)should be divided by 64.*/
      /* ?? BIT7 - 1 - In linetime mode Clock Source for CMU is CRU recovered clock DIV by 64*/

      address = (VSC8486_SYNC_ETHERNET_CLK_CTRL & 0xffff) | (PMA_DEVICE << 16) | (device_address << 21);
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
      register_value = grw_read(grw);
      register_value |= ( BIT4 | BIT5 | BIT7);
      register_value &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT6 );
      grw_write(grw,register_value);
      grw_dispose(grw);

	
      address = (VSC8486_WIS_INTERRUPT_STATUS & 0xffff) | (WIS_DEVICE << 16) | (device_address << 21); 	
      grw = grw_init(card,address,grw_vsc8486_mdio_read,grw_vsc8486_mdio_write);
     
      register_value = grw_read(grw); 		
      while ( (((register_value & BIT12) >> 12) == 1) && (attempts < 50) ) 
      { 
	   attempts++; 
	   register_value = grw_read(grw); 		
      } 
      if (attempts == 50)
		dagutil_warning("Not able to detect Rx lock timed out. \n");

}
void *
wan_slave_mode_read(AttributePtr attribute)
{
     DagCardPtr card = NULL;
     GenericReadWritePtr grw = NULL;
     ComponentPtr component = NULL;
     uintptr_t address = 0;
     uint32_t register_value = 0;
     bool value = 0;
     uint32_t device_address = 0;
     port_state_t * state = NULL;
     component = attribute_get_component(attribute);
 
     state = component_get_private_state(component);

     if(state->mIndex == 0x00)
       device_address = 0x00;
     else if(state->mIndex == 0x01)
       device_address = 0x01;	      
	
     card = attribute_get_card(attribute);
     
     address = (VSC8486_PMA_DEV_STATUS1 & 0xffff ) | (PMA_DEVICE << 16)| (device_address << 21);
     grw = grw_init(card, address, grw_vsc8486_mdio_read, grw_vsc8486_mdio_write);
     register_value = grw_read(grw);
     if ((((register_value & BIT15) >> 15 ) == 1) && (((register_value & BIT13)>> 13)== 1))
     {
        	value = 1;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     else
     {
		value = 0;
		attribute_set_value_array(attribute, &value, sizeof(value));
     }
     return attribute_get_value_array(attribute);
}

