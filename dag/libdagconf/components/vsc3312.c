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
enum
{
    vsc3312_smbus_address_device_one     = 0x00,
    vsc3312_smbus_address_device_two     = 0x01,
    vsc3312_serial_interface_enable      = 0x79,
    vsc3312_page_register                = 0x7F,
    vsc3312_connection_register          = 0x00,
    vsc3312_input_ise_register           = 0x00,
    vsc3312_input_state_register         = 0x00,
    vsc3312_input_los_register           = 0x00,
    vsc3312_output_pre_long_register     = 0x00,
    vsc3312_output_pre_short_register    = 0x00,
    vsc3312_output_level_register        = 0x00,
    vsc3312_output_state_register        = 0x00,
    vsc3312_status_zero_pin_register     = 0x00,
    vsc3312_status_one_pin_register      = 0x00,
    vsc3312_channel_status_register      = 0x00,
    vsc3312_pin_status_register          = 0x10,
    vsc3312_core_configuration           = 0x75
};
static void vsc3312_dispose(ComponentPtr component);
static void vsc3312_reset(ComponentPtr component);
static void vsc3312_default(ComponentPtr component);
static int vsc3312_post_initialize(ComponentPtr component);
/*raw attribute - address and data.has a mask of 0xff. the user needs to take care.*/
static AttributePtr vsc3312_connection_number_attribute(GenericReadWritePtr grw, const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_raw_data_attribute(GenericReadWritePtr grw, const uint32_t *bit_masks,uint32_t len);
/*page register attributes*/
static AttributePtr vsc3312_page_reg_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr vsc3312_serial_interface_enable_reg_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len);
/*connection register*/
static AttributePtr vsc3312_output_enable_reg_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_set_connection_reg_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*input state*/
static AttributePtr vsc3312_input_state_terminate_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_input_state_off_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_input_state_invert_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*input ISE*/
static AttributePtr vsc3312_input_ise_short_time_constant_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len);
static AttributePtr vsc3312_input_ise_medium_time_constant_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len);
static AttributePtr vsc3312_input_ise_long_time_constant_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len);
/*input LOS threshold*/
static AttributePtr vsc3312_input_los_threshold_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*output pre long*/
static AttributePtr vsc3312_output_prelong_decay_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_output_prelong_level_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Output Pre Short*/
static AttributePtr vsc3312_output_preshort_decay_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_output_preshort_level_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Output Level*/
static AttributePtr vsc3312_output_level_terminate_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_output_level_power_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Output State*/
static AttributePtr vsc3312_output_state_los_forwarding_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_output_state_operation_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Status Zero PIN*/
static AttributePtr vsc3312_status_zero_pin_los_raw_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_status_zero_pin_los_latched_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Status One PIN*/
static AttributePtr vsc3312_status_one_pin_los_raw_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len); 
static AttributePtr vsc3312_status_one_pin_los_latched_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Channel Status */
static AttributePtr vsc3312_channel_status_los_raw_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_channel_status_los_latched_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*PIN STATUS Status 0*/
static AttributePtr vsc3312_pin_status_stat_zero_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_pin_status_stat_one_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Core Configuration*/
static AttributePtr vsc3312_left_core_energise_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_right_core_energise_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_core_low_glitch_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_core_buffer_force_on_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
static AttributePtr vsc3312_core_config_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*Input LOS combinined for all four input channels.*/
static AttributePtr vsc3312_input_los_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len);
/*get value set value routines*/
static void attribute_connection_number_set_value(AttributePtr attribute,void *value,int len);
static void*attribute_connection_number_get_value(AttributePtr attribute);
static void attribute_raw_data_set_value(AttributePtr attribute,void *value,int len);
static void*attribute_raw_data_get_value(AttributePtr attribute);
static void vsc3312_attribute_set_value(AttributePtr attribute,void *value,int len);
static void *vsc3312_attribute_get_value(AttributePtr attribute);
/*input los combined for all four input channels*/
static void* vsc3312_input_los_attribute_get_value(AttributePtr attribute);
typedef struct
{
	uint32_t mIndex;
} vsc3312_state_t;

ComponentPtr
vsc3312_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentCrossPointSwitch, card);
    vsc3312_state_t* state = NULL;
    index = card_get_register_index_by_version(card,DAG_REG_RAW_SMBBUS,4,index);
    if (NULL != result)
    {
	component_set_dispose_routine(result, vsc3312_dispose);
	component_set_post_initialize_routine(result, vsc3312_post_initialize);
	component_set_reset_routine(result, vsc3312_reset);
	component_set_default_routine(result, vsc3312_default);
	component_set_name(result, "vsc3312");
	component_set_description(result, "The VSC3313 Crosspoint Switch.");
	state = (vsc3312_state_t*)malloc(sizeof(vsc3312_state_t));
	state->mIndex = index;
	component_set_private_state(result, state);
	return result;
    }
    return result;
}
void vsc3312_dispose(ComponentPtr component)
{
}
void vsc3312_reset(ComponentPtr component)
{
}
void vsc3312_default(ComponentPtr component)
{
	/*Serial Interface Enable Register - > 0x02h*/
	/*Core Configuration Register- 
	Energise Lt Core      - On
	Energise Rt Core      - On
	Low Glitch            - Off
	Core Buffer Force On  - Off
	Core Config Attribute - Off
	*/
	uint32_t value = 0;	
	bool value_bool = 0;
	AttributePtr attribute;
	
	value = 2;
	attribute = component_get_attribute(component,kUint32AttributeSerialInterfaceEnable);
	attribute_uint32_set_value(attribute,&value,1);
	
	/*Energise Right Core   - On*/
	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeEnergiseRtCore);
	attribute_boolean_set_value(attribute,&value_bool,1);
	
	/*Energise Left Core.   - On*/
	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeEnergiseLtCore);
	attribute_boolean_set_value(attribute,&value_bool,1);
	
	/*Low Glitch Attribute  - Off*/
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeLowGlitch);
	attribute_boolean_set_value(attribute,&value_bool,1);
	
	/*Core Buffer Force On  - Off*/
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeCoreBufferForceOn);
	attribute_boolean_set_value(attribute,&value_bool,1);
	
	/*Core Config Attribute - Off*/
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeCoreConfig);
	attribute_boolean_set_value(attribute,&value_bool,1);

	/*page register - 7f - 00*/
	/*Set the connection
	 * 02 - 00
	 * 03 - 06
	 * 04 - 07 
	 * 05 - 01*/
	/*Output Port 02 to Input Port 0*/
	value = 2;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 6;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	/*Output Port 03 to Input Port 1*/
	value = 3;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);
	
	value = 7;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Output Port 04 to Input Port 2*/
	value = 4;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 1;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Output Port 05 to Input Port 3*/
	value = 5;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 0;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Setting Output Power Level*/
	/*Page Register Should contain - 0x22*/
	value = 2;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 0xc;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);	

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value = 3;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 0xc;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value = 4;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 0xc;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value = 5;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 0xc;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);	

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/* Set Enable LOS Forwarding*/
	/*Connection Number - 2*/
	value = 2;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);
	
	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Normal Mode of Operation*/	
	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Connection Number - 3*/
	value = 3;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Normal Mode of Operation*/	
	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Connection Number - 4*/
	value = 4;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	/*Normal Mode of Operation*/	
	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Connection Number - 5*/
	value = 5;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);
	
	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	/*Normal Mode of Operation*/

	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Set Input State*/
	/*Page Register Value - 0x11*/
	/*Input State Register has 3 attributes.
	 * Input State Terminate - Off
	 * Input State Off       - Off
	 * Input State Invert    - Off 
	 * The above values for all four Input Conncetions 0,1,6,7*/
	/*Input Connection Number - 0*/
	value = 0;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Input Connection Number - 1*/
	value = 1;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Input Connection Number - 6*/
	value = 6;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Input Connection Number - 7*/
	value = 7;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);



	/*Tx PORT SETTINGS*/
	/* the following are the settting for the tx channel.By default the output off for the output ports in */
	/* transmit which are 0,1,6,7 are turned on(Output Off = 0.).For port mode this should be disabled.*/
	/*page register - 7f - 00*/	
	/*Set the connection
 	 * i/p - o/p 	
	 * 2 - 0
	 * 3 - 1
	 * 4 - 6 
	 * 5 - 7*/
	/*Output Port 0 to Input Port 2*/
	value = 0;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 2;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	/*Output Port 1 to Input Port 3*/
	value = 1;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);
	
	value = 3;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Output Port 6 to Input Port 4*/
	value = 6;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 4;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Output Port 7 to Input Port 5*/
	value = 7;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 5;
	attribute = component_get_attribute(component,kUint32AttributeSetConnection);
	vsc3312_attribute_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Setting Output Power Level*/
	/*Page Register Should contain - 0x22*/
	value = 0;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 8;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);	

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value = 1;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 8;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value = 6;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 8;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value = 7;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value = 8;
	attribute = component_get_attribute(component,kUint32OutputLevelPower);
	vsc3312_attribute_set_value(attribute,&value,1);	

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/* Set Enable LOS Forwarding*/
	/*Connection Number - 12*/
	value = 0;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);
	
	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Normal Mode of Operation*/	
	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Connection Number - 13*/
	value = 1;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Normal Mode of Operation*/	
	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Connection Number - 14*/
	value = 6;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	/*Normal Mode of Operation*/	
	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Connection Number - 15*/
	value = 7;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);
	
	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	/*Normal Mode of Operation*/

	value = 5;
	attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
	vsc3312_attribute_set_value(attribute,&value,1);

	/*Set Input State*/
	/*Page Register Value - 0x11*/
	/*Input State Register has 3 attributes.
	 * Input State Terminate - Off
	 * Input State Off       - Off
	 * Input State Invert    - Off 
	 * The above values for all four Input Conncetions 2,3,4,5*/
	/*Input Connection Number - 2*/
	value = 2;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Input Connection Number - 3*/
	value = 3;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Input Connection Number - 4*/
	value = 4;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);
	
	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 1;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	/*Input Connection Number - 5*/
	value = 5;
	attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	attribute_connection_number_set_value(attribute,&value,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

	value_bool = 0;
	attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
	vsc3312_attribute_set_value(attribute,&value_bool,1);

}
int vsc3312_post_initialize(ComponentPtr component)
{
    GenericReadWritePtr grw = NULL;
    AttributePtr attr = NULL;
    DagCardPtr card = NULL;
    uint32_t address = 0;
    uint32_t component_base_address = 0;
    uint32_t index2 = 0x00;
    uint32_t device_address = 0x00;
    uint32_t mask;
    vsc3312_state_t *state;

    card = component_get_card(component);
    
    state = NULL;
    state = component_get_private_state(component);

    if(state->mIndex == 0x00)
        device_address = 0x00;
    else if(state->mIndex == 0x01)
        device_address = 0x02;
    
    component_base_address = (state->mIndex << 24) | (index2 << 16) | (device_address << 8);
    /*page register */
    mask = 0xff;
    address = component_base_address | vsc3312_page_register ;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_page_reg_attribute(grw, &mask, 1);
    component_add_attribute(component, attr);
    
    /*global register*/
    mask = 0xff;
    address = component_base_address | vsc3312_serial_interface_enable;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_serial_interface_enable_reg_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    /*Core Configuration Register Configuration Register */
    mask = BIT4;
    address = component_base_address | vsc3312_core_configuration; 
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_left_core_energise_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    mask = BIT3;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_right_core_energise_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    mask = BIT2;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_core_low_glitch_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    
    mask = BIT1;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_core_buffer_force_on_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    mask = BIT0;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_core_config_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    /*raw address attribute*/
    mask = 0xff;
    grw = grw_init(card,component_base_address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_connection_number_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    /*raw data attribute*/
    mask = 0xff;
    grw = grw_init(card,component_base_address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_raw_data_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
	
    mask = BIT4;
    /*the connection register will have 0x00 connection number zero while initilization*/
    address = component_base_address | vsc3312_connection_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    /*The attribute - output enable has an inverted logic. 0 - output is enabled. 1- output disabled.So setting GRWClearBit to make it user friendly.*/	
    grw_set_on_operation(grw,kGRWClearBit);	
    attr = vsc3312_output_enable_reg_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    mask = BIT0|BIT1|BIT2|BIT3;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_set_connection_reg_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    /*Input Serial Equlaization Medium Short and Long*/
    mask = BIT4|BIT5;
    address = component_base_address | vsc3312_input_ise_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_ise_short_time_constant_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    mask = BIT2|BIT3;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_ise_medium_time_constant_attribute(grw,&mask,1);
    component_add_attribute(component,attr);


    mask = BIT1|BIT0;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_ise_long_time_constant_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    /*Input State - Terminate , Off and Invert*/
    mask = BIT2;
    address = component_base_address | vsc3312_input_state_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_state_terminate_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    mask = BIT1;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_state_off_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    mask = BIT0;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_state_invert_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    /*Input LOS*/	
    mask = BIT0 | BIT1 | BIT2;
    address = component_base_address | vsc3312_input_los_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_los_threshold_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
   
    /*Output Pre Long*/	 
    mask = BIT0 | BIT1 | BIT2;
    address = component_base_address | vsc3312_output_pre_long_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_output_prelong_decay_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    mask = BIT3 | BIT4 | BIT5 | BIT6;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr =  vsc3312_output_prelong_level_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    /*Output Pre Short*/	
    mask = BIT0 | BIT1 | BIT2;
    address = component_base_address | vsc3312_output_pre_short_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_output_preshort_decay_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    mask = BIT3 | BIT4 | BIT5 | BIT6;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_output_preshort_level_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
   
    /*Output Level*/	
    mask = BIT4;
    address = component_base_address | vsc3312_output_level_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_output_level_terminate_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    mask = BIT0 | BIT1 | BIT2 | BIT3;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr =  vsc3312_output_level_power_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
   
    /*Output State*/ 
    mask = BIT0;
    address = component_base_address | vsc3312_output_state_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_output_state_los_forwarding_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    

    mask = BIT1 | BIT2 | BIT3 | BIT4;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_output_state_operation_mode_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    /*Status 0 PIN*/
    mask = BIT0;
    address = component_base_address | vsc3312_status_zero_pin_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_status_zero_pin_los_raw_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    mask = BIT1;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_status_zero_pin_los_latched_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
        
    /*Status 1 PIN*/
    mask = BIT0;
    address = component_base_address | vsc3312_status_one_pin_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_status_one_pin_los_raw_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    mask = BIT1;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_status_one_pin_los_latched_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    /*Channel Status PIN*/
    mask = BIT0;
    address = component_base_address | vsc3312_channel_status_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_channel_status_los_raw_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    
    mask = BIT1;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_channel_status_los_latched_attribute(grw,&mask,1);
    component_add_attribute(component,attr);

    /*PIN Status*/
    mask = BIT0;
    address = component_base_address | vsc3312_pin_status_register;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_pin_status_stat_zero_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
   
    mask = BIT1;
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_pin_status_stat_one_attribute(grw,&mask,1);
    component_add_attribute(component,attr);
    /*Combined input LOS flag.*/
    mask = 0xff; // does not matter it is a raw attribute.
    grw = grw_init(card,address,grw_raw_smbus_read,grw_raw_smbus_write);
    attr = vsc3312_input_los_attribute(grw,&mask,1);
    component_add_attribute(component,attr);	



    return 1;    
}
/*vsc3312 - crosspoint switch*/
AttributePtr 
vsc3312_connection_number_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
    result = attribute_init(kUint32AttributeCrossPointConnectionNumber);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "crosspoint_connection_number");
    attribute_set_description(result, "VSC3312 Connection Number Attribute.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_connection_number_get_value);
    attribute_set_setvalue_routine(result, attribute_connection_number_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr 
vsc3312_raw_data_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
    result = attribute_init(kUint32AttributeRawData);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "vsc3312_raw_data_attribute");
    attribute_set_description(result, "VSC3312 Raw Data Attribute.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_raw_data_get_value);
    attribute_set_setvalue_routine(result, attribute_raw_data_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_serial_interface_enable_reg_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len)
{
    /*No page Information for this attribute - Global Register*/
    AttributePtr result = NULL;
    result = attribute_init(kUint32AttributeSerialInterfaceEnable);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "serial_interface_enable_register");
    attribute_set_description(result, "Serial Interface Enable Register.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_page_reg_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    /*No page Information for this attribute - Global Register*/
    AttributePtr result = NULL;
    result = attribute_init(kUint32AttributePageRegister);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "page_register");
    attribute_set_description(result, "Page Register for accessing the register pages.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result,attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_left_core_energise_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    /*No page Information for this attribute - Global Register*/
    AttributePtr result = NULL;
    result = attribute_init(kBooleanAttributeEnergiseLtCore);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "energise_left_core");
    attribute_set_description(result, "Energises the Left Core.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result,attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_right_core_energise_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    /*No page Information for this attribute - Global Register*/
    AttributePtr result = NULL;
    result = attribute_init(kBooleanAttributeEnergiseRtCore);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "energise_right_core");
    attribute_set_description(result, "Energises the Right Core.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result,attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}

AttributePtr
vsc3312_core_low_glitch_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    /*No page Information for this attribute - Global Register*/
    AttributePtr result = NULL;
    result = attribute_init(kBooleanAttributeLowGlitch);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "core_low_glitch");
    attribute_set_description(result, "Activates the Low Glitch Mode.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result,attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_core_buffer_force_on_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    /*No page Information for this attribute - Global Register*/
    AttributePtr result = NULL;
    result = attribute_init(kBooleanAttributeCoreBufferForceOn);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "core_buffer_force_on");
    attribute_set_description(result, "Turns On All Core Buffers for faster switching speed.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result,attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_core_config_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    /*No page Information for this attribute - Global Register*/
    AttributePtr result = NULL;
    result = attribute_init(kBooleanAttributeCoreConfig);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "core_config");
    attribute_set_description(result, "Inverts the sense of the Config PIN.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result,attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr 
vsc3312_output_enable_reg_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    
    uint32_t *page_address;
    AttributePtr result = NULL;
    result = attribute_init(kBooleanAttributeOutputEnable);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x00;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_enable_attribute");
    attribute_set_description(result, "Output Enable Register.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr 
vsc3312_set_connection_reg_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{

    AttributePtr result = NULL;
    uint32_t *page_address;
    result = attribute_init(kUint32AttributeSetConnection);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x00;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "set_connection_attribute");
    attribute_set_description(result, "Set Connection Register.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Input ISE Short Medium and Long time constants*/
AttributePtr
vsc3312_input_ise_short_time_constant_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeInputISEShortTimeConstant);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x10;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "ise_short_time_constant");
    attribute_set_description(result, "ISE Short Time Constant.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_input_ise_medium_time_constant_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;

    result = attribute_init(kUint32AttributeInputISEMediumTimeConstant);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x10;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "ise_medium_time_constant");
    attribute_set_description(result, "ISE Medium Time Constant.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result,vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result,vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}

AttributePtr
vsc3312_input_ise_long_time_constant_attribute(GenericReadWritePtr grw,const uint32_t* bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeInputISELongTimeConstant);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x10;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "ise_long_time_constant");
    attribute_set_description(result, "ISE Long Time Constant.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result,vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result,vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Input State - Terminate ,Off and Invert*/
AttributePtr 
vsc3312_input_state_terminate_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kBooleanAttributeInputStateTerminate);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x11;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "input_state_terminate");
    attribute_set_description(result, "Input State Terminate to VDD Register.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr 
vsc3312_input_state_off_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kBooleanAttributeInputStateOff);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x11;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "input_state_off");
    attribute_set_description(result, "Input State Off.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr 
vsc3312_input_state_invert_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kBooleanAttributeInputStateInvert);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x11;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "input_state_invert");
    attribute_set_description(result, "Input State Invert.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Input LOS*/
AttributePtr
vsc3312_input_los_threshold_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeInputLOSThreshold);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x12;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "input_los_threshold");
    attribute_set_description(result, "Input Loss of Signal Threshold.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Output Pre Long*/
AttributePtr
vsc3312_output_prelong_decay_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeOutputPreLongDecay);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x20;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_prelong_decay");
    attribute_set_description(result, "Output Pre Long Decay.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
} 
AttributePtr
vsc3312_output_prelong_level_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeOutputPreLongLevel);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address  = 0x20;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_prelong_level");
    attribute_set_description(result, "Output Pre Long Level.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Output Pre Short*/ 
AttributePtr
vsc3312_output_preshort_decay_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeOutputPreShortDecay);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x21;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_preshort_decay");
    attribute_set_description(result, "Output Pre Short Decay.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
} 
AttributePtr
vsc3312_output_preshort_level_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeOutputPreShortLevel);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x21;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_preshort_level");
    attribute_set_description(result, "Output Pre Short Level.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Output Level*/ 
AttributePtr
vsc3312_output_level_terminate_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kBooleanOutputLevelTerminate);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x22;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_level_terminate"); 
    attribute_set_description(result, "Output Level Terminate.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
} 
AttributePtr
vsc3312_output_level_power_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32OutputLevelPower);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x22;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_level_power");
    attribute_set_description(result, "Output Level Power.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Output State*/ 
AttributePtr
vsc3312_output_state_los_forwarding_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kBooleanOutputStateLOSForwarding);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x23;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_state_los_forwarding");
    attribute_set_description(result, "Output State LOS Forwarding.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
} 
AttributePtr
vsc3312_output_state_operation_mode_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32OutputStateOperationMode);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x23;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "output_state_operation_mode");
    attribute_set_description(result, "Output State Operation Mode.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Status Zero PIN*/
AttributePtr
vsc3312_status_zero_pin_los_raw_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
    uint32_t *page_address;
    result = attribute_init(kUint32AttributeStatusPin0LOS);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x80;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "status_0_pin_los_raw");
    attribute_set_description(result, "Assigns LOS signal from selected input to STAT0 Pin(raw value).");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_status_zero_pin_los_latched_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeStatusPin0LOSLatched);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x80;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "status_0_pin_los_latched");
    attribute_set_description(result, "Assigns LOS signal from selected input to STAT0 Pin(latched value).");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Status One PIN*/
AttributePtr
vsc3312_status_one_pin_los_raw_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeStatusPin1LOS);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x81;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "status_1_pin_los_raw");
    attribute_set_description(result, "Assigns LOS signal from selected input to STAT1 Pin(raw value).");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_status_one_pin_los_latched_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kUint32AttributeStatusPin1LOSLatched);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0x81;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "status_one_pin_los_latched");
    attribute_set_description(result, "Assigns LOS signal from selected input to STAT1 Pin(latched value).");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result,attribute_uint32_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*Channel Status*/
AttributePtr
vsc3312_channel_status_los_raw_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
    uint32_t *page_address;
    result = attribute_init(kBooleanAttributeChannelStatusLOS);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0xF0;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "channel_status_los_raw");
    attribute_set_description(result, "Provides the LOS status for the selected input(raw).");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_channel_status_los_latched_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
    uint32_t *page_address;
    result = attribute_init(kBooleanAttributeChannelStatusLOSLatched);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0xF0;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "channel_status_los_latched");
    attribute_set_description(result, "Provides LOS status for the selected input(latched value).");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, vsc3312_attribute_get_value);
    attribute_set_setvalue_routine(result, vsc3312_attribute_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
/*PIN STATUS*/
AttributePtr
vsc3312_pin_status_stat_zero_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
    uint32_t *page_address;
    result = attribute_init(kBooleanAttributePin0Status);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0xF0;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "pin_status_stat0");
    attribute_set_description(result, "Provides the state of Stat 0 Pin.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_pin_status_stat_one_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
    AttributePtr result = NULL;
	uint32_t *page_address;
    result = attribute_init(kBooleanAttributePin1Status);
    page_address = (uint32_t*)malloc(sizeof(uint32_t));
    *page_address = 0xF0;
    attribute_set_private_state(result,page_address);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "pin_status_stat1");
    attribute_set_description(result, "Provides the state of stat 1 pin.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result,attribute_boolean_from_string);
    attribute_set_masks(result,bit_masks,len);
    return result;
}
AttributePtr
vsc3312_input_los_attribute(GenericReadWritePtr grw,const uint32_t *bit_masks,uint32_t len)
{
	AttributePtr result = NULL;
	uint32_t *page_address;
	result = attribute_init(kBooleanAttributeInputLOS);
	page_address = (uint32_t*)malloc(sizeof(uint32_t));
	*page_address = 0xF0;
	attribute_set_private_state(result,page_address);
	attribute_set_valuetype(result,kAttributeBoolean);
	attribute_set_generic_read_write_object(result,grw);
	attribute_set_name(result,"input_los_attribute");
	attribute_set_description(result,"combined attribute for input los of signal.");
	attribute_set_config_status(result,kDagAttrStatus);
	attribute_set_setvalue_routine(result,attribute_boolean_set_value);/*Read Only attribute.Not to be used.*/
	attribute_set_getvalue_routine (result,vsc3312_input_los_attribute_get_value);
	attribute_set_to_string_routine(result,attribute_boolean_to_string);
	attribute_set_from_string_routine(result,attribute_boolean_from_string);
	attribute_set_masks(result,bit_masks,len);
	return result;
}
void* vsc3312_input_los_attribute_get_value(AttributePtr attribute)
{
	if(1 == valid_attribute(attribute))
	{
		uint32_t connection_number = 0; 
		void *value = NULL;
		bool value1,value2,value3,value4 = false;
		bool final_value;
	        ComponentPtr component = NULL;
		AttributePtr attr = NULL;

		/*connection number from raw address*/
		component = attribute_get_component(attribute);
		
		attr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
		connection_number = 6;
		attribute_connection_number_set_value(attribute,&connection_number,sizeof(connection_number));
		attr = component_get_attribute(component,kBooleanAttributeChannelStatusLOS);
		value = vsc3312_attribute_get_value(attr);
		value1 = *(bool*)value;


		attr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
		connection_number = 7;
		attribute_connection_number_set_value(attribute,&connection_number,sizeof(connection_number));
		attr = component_get_attribute(component,kBooleanAttributeChannelStatusLOS);
		value = vsc3312_attribute_get_value(attr);
		value2 = *(bool*)value;

		
		attr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
		connection_number = 0;
		attribute_connection_number_set_value(attribute,&connection_number,sizeof(connection_number));
		attr = component_get_attribute(component,kBooleanAttributeChannelStatusLOS);
		value = vsc3312_attribute_get_value(attr);
		value3 = *(bool*)value;

					
		attr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
		connection_number = 1;
		attribute_connection_number_set_value(attribute,&connection_number,sizeof(connection_number));
		attr = component_get_attribute(component,kBooleanAttributeChannelStatusLOS);
		value = vsc3312_attribute_get_value(attr);
		value4 = *(bool*)value;

		final_value = value1|value2|value3|value4;
		
		attribute_set_value_array(attribute,&final_value,sizeof(final_value));	
		return (void*)attribute_get_value_array(attribute);		
	}
	return NULL;
}
void attribute_connection_number_set_value(AttributePtr attribute,void*value,int len)
{
    if (1 == valid_attribute(attribute))
    {
	GenericReadWritePtr grw = NULL;
	uint32_t address = *(uint32_t*)value; 
	grw = attribute_get_generic_read_write_object(attribute);
	grw_set_address(grw,address);
    }
}
void *attribute_connection_number_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
	GenericReadWritePtr grw = NULL;
	uint32_t *address = (uint32_t*)malloc(sizeof(uint32_t));
	grw = attribute_get_generic_read_write_object(attribute);
	*address = grw_get_address(grw);
        return address;
    }
    return NULL;
}
/*assumes that page register is already loaded with proper page*/
/*the mask of the raw data register is 8 bits.this has to be kept in mind when writing to and reading from a register*/
void attribute_raw_data_set_value(AttributePtr attribute,void *value,int len)
{
    if(1 == valid_attribute(attribute))
    {
	GenericReadWritePtr grw = NULL;
	uint32_t connection_number = 0; 
	uint32_t destination_address = 0; 
	uint32_t index2 = 0; //always zero
	uint32_t device_address = 0;
	vsc3312_state_t *state = NULL;
	
	/*connection number from raw address*/
        ComponentPtr component = attribute_get_component(attribute);
	AttributePtr raw_addr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
        grw = attribute_get_generic_read_write_object(raw_addr);
	connection_number = grw_get_address(grw);
       
	/*the actual address calculation has to be done here.*/
	state = component_get_private_state(component);
	if(state->mIndex == 0x00)
		device_address = 0x00;
	if(state->mIndex == 0x01)
		device_address = 0x02;

	destination_address = (state->mIndex << 24)| (index2 << 16) | (device_address << 8);

	/*setting the grw addresss of the attribute*/
	grw = attribute_get_generic_read_write_object(attribute);
	destination_address = grw_get_address(grw);
	destination_address = (destination_address&0xffffff00) | connection_number;
	grw_set_address(grw,destination_address);
	dagutil_verbose_level(1,"raw data set address : %x data : %x \n",destination_address,*(uint32_t*)value);
	attribute_uint32_set_value(attribute,value,1);
   }
}
void* attribute_raw_data_get_value(AttributePtr attribute)
{ 
    if(1 == valid_attribute(attribute))
    {
	GenericReadWritePtr grw = NULL;
	uint32_t connection_number = 0; 
	uint32_t destination_address = 0; 
	uint32_t index2 = 0; //always zero
	uint32_t device_address = 0;
	vsc3312_state_t *state = NULL;
	void *value;
        /*connection number from raw address*/
        ComponentPtr component = attribute_get_component(attribute);
	AttributePtr raw_addr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	grw = attribute_get_generic_read_write_object(raw_addr);
        connection_number = grw_get_address(grw);
		
	/*the actual address calculation has to be done here.*/
	state = component_get_private_state(component);
	if(state->mIndex == 0x00)
		device_address = 0x00;
	if(state->mIndex == 0x01)
		device_address = 0x02;

	destination_address = (state->mIndex << 24)| (index2 << 16) | (device_address << 8);

        /*get index to get device address*/
        grw = attribute_get_generic_read_write_object(attribute);
        destination_address = grw_get_address(grw);
	destination_address = (destination_address&0xffffff00) | connection_number;
	grw_set_address(grw,destination_address);
    value = attribute_uint32_get_value(attribute);
	dagutil_verbose_level(1,"raw data get address :%x data : %x \n",destination_address,*(uint32_t*)value);
	return value;
    }
    return NULL;

}
void vsc3312_attribute_set_value(AttributePtr attribute,void *value,int len)
{

	uint32_t connection_number;	
	uint32_t address = 0; 
    	void* page_address;
	ComponentPtr component; 
	AttributePtr attr;
	GenericReadWritePtr grw = NULL;

    	if(1 == valid_attribute(attribute))
    	{
	/*Update the Page Register*/
    	page_address = attribute_get_private_state(attribute);	
    	component = attribute_get_component(attribute);
    	attr = component_get_attribute(component,kUint32AttributePageRegister);
    	//dagutil_verbose_level(1,"vsc3312_attribute_set_value : Loading Page Address : %x\n",*(uint32_t*)page_address);
    	attribute_uint32_set_value(attr,page_address,1);
    	
    	/*Update the Connection Number from Raw Address Attribute*/
    	attr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
    	grw = attribute_get_generic_read_write_object(attr);
    	connection_number = grw_get_address(grw);
    	//dagutil_verbose_level(1,"vsc3312_attribute_set_value : Connection Number : %x\n",connection_number);
        
    	/*grw object of the actual attribute*/
    	grw = attribute_get_generic_read_write_object(attribute);
    	address = grw_get_address(grw);
    	address = (address & 0xffffff00)|connection_number;
    	grw_set_address(grw,address);
        //dagutil_verbose_level(1, "vsc3312_attribute_set_value: raw_smbus_address : %x value : %x\n",address,*(uint32_t*)value);
	switch(attribute_get_valuetype(attribute))
	{
		case kAttributeBoolean:
		     attribute_boolean_set_value(attribute,value,len);
		     break;
	        case kAttributeUint32:
		     attribute_uint32_set_value(attribute,value,len);
		     break;
		default:
		     printf("Not able to set value \n");
		     break;
	}
       
    }
}
void* vsc3312_attribute_get_value(AttributePtr attribute)
{

	uint32_t connection_number;
	void* page_address;
	GenericReadWritePtr grw = NULL;
	uint32_t address = 0;
	void *value = NULL;
	ComponentPtr component;
	AttributePtr attr;

    	if(1 == valid_attribute(attribute))
    	{
		/*load the Page Register from the Private State*/
		page_address = attribute_get_private_state(attribute);	
		component = attribute_get_component(attribute);
		attr = component_get_attribute(component,kUint32AttributePageRegister);
	    	attribute_uint32_set_value(attr,page_address,1);
    
		/*connection number from raw address*/
	    	attr = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
	    	grw = attribute_get_generic_read_write_object(attr);
	    	connection_number = grw_get_address(grw);
    
    
	    	/*grw object of the actual object*/
	    	grw = attribute_get_generic_read_write_object(attribute); 
    		address = grw_get_address(grw);
   		address = (address & 0xffffff00) | connection_number;
    		grw_set_address(grw,address);

		switch (attribute_get_valuetype(attribute))
		{
			case kAttributeBoolean:
		     	value = attribute_boolean_get_value(attribute);
		     	break;
			case kAttributeUint32:
		     	value = attribute_uint32_get_value(attribute);
		     	break;
			default:
		     	printf("Not able to get value - Error  \n");
		     	break;
		}
		return value;
    }
    return NULL;
}
/*vsc3312 - crosspoint switch*/
