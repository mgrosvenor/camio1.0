#include "dagutil.h"
#include "../include/attribute.h"
#include "../include/util/set.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/counter_component.h"
#include "../include/components/counters_interface_component.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"
#include "../include/components/cat_component.h"
#include "../include/dag_attribute_codes.h"
#include "../include/attribute_types.h"
#include "dagcam/idt75k_lib.h"

static void infiniband_classifier_dispose(ComponentPtr component);
static void infiniband_classifier_reset(ComponentPtr component);
static void infiniband_classifier_default(ComponentPtr component);
static int infiniband_classifier_post_initialise(ComponentPtr component);

typedef enum
{
	la1_interface_control_register    = 0x00,
	hmic_control_interface_register   = 0x04,
	sram_miss_value_register          = 0x08
}infiniband_classifier_register_offset;

typedef struct
{
	uint32_t mIndex;
}classifier_state_t;

Attribute_t infiniband_classifier_attr[]=
{
    {
        /* Name */                 "la0_datapath_reset",
        /* Attribute Code */       kBooleanAttributeLA0DataPathReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "LA-0 Data Path Reset.",
        /* Config-Status */        kDagAttrStatus, /*Implemented in FW as Config But For the user only Status.*/
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               la1_interface_control_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT31,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "la1_datapath_reset",
        /* Attribute Code */       kBooleanAttributeLA1DataPathReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "LA-1 Data Path Reset.",
        /* Config-Status */        kDagAttrStatus, /*Implemented in FW as Config But For the user only Status.*/
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               la1_interface_control_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT30,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "la0_firmware_access",
        /* Attribute Code */       kBooleanAttributeLA0AccessEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Controls the FirmWare Access to LA0.",
        /* Config-Status */        kDagAttrConfig, 
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               la1_interface_control_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT25,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "la1_firmware_access",
        /* Attribute Code */       kBooleanAttributeLA1AccessEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Controls the FirmWare Access to LA1.",
        /* Config-Status */        kDagAttrConfig, 
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               la1_interface_control_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT24,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "database_to_use",
        /* Attribute Code */       kUint32AttributeDataBase,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Database to use for searches Can be changed to effect hot-swap.",
        /* Config-Status */        kDagAttrConfig, 
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               hmic_control_interface_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT4 | BIT5 | BIT6 | BIT7,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "sram_miss_value",
        /* Attribute Code */       kUint32AttributeSRamMissValue,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Value that replaces the associated SRAM value if a search misses.",
        /* Config-Status */        kDagAttrConfig, 
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               sram_miss_value_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xffffffff,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "global_mask_register",
        /* Attribute Code */       kUint32Attributegmr,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "The global Mask register .",
        /* Config-Status */        kDagAttrConfig, 
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               hmic_control_interface_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT8 | BIT9 | BIT10 | BIT11 | BIT12,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "enable_classifier",
        /* Attribute Code */       kUint32AttributeClassifierEnable,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Enables the Classifier.",
        /* Config-Status */        kDagAttrConfig, 
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               hmic_control_interface_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0 | BIT1 | BIT2 | BIT3,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "disable_steering",
        /* Attribute Code */       kUint32AttributeDisableSteering,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Pass through if the option is set.",
        /* Config-Status */        kDagAttrConfig, 
        /* Index in register */    0,
        /* Register Address */     DAG_REG_INFINICLASSIFIER,
        /* Offset */               hmic_control_interface_register,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT15 | BIT14 | BIT13,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }
};
#define NB_ELEM (sizeof(infiniband_classifier_attr) / sizeof(Attribute_t))
ComponentPtr
infiniband_classifier_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentInfinibandClassifier,card);
    classifier_state_t* state = NULL;
    if (NULL != result)
    {
        component_set_dispose_routine(result, infiniband_classifier_dispose);
        component_set_post_initialize_routine(result, infiniband_classifier_post_initialise);
        component_set_reset_routine(result, infiniband_classifier_reset);
        component_set_default_routine(result, infiniband_classifier_default);
        component_set_name(result, "Infiniband Classifier.");
        component_set_description(result, "The Infiniband Classification Module.");
        state = (classifier_state_t*)malloc(sizeof(classifier_state_t));
        state->mIndex = index;
        component_set_private_state(result, state);
        return result;
    }
    return result;
}
void infiniband_classifier_dispose(ComponentPtr component)
{
}
void infiniband_classifier_reset(ComponentPtr component)
{
	/*Can be used to issue a phase alignment reset. 
	 * manually by the user if needed.*/

}
void infiniband_classifier_default(ComponentPtr component)
{
	/*Routine to initialise the system properly.*/
	AttributePtr attribute = NULL;
	DagCardPtr card = NULL;
	classifier_state_t *state = NULL;
	bool bool_value = 0;
	uint32_t value;
	void *temp = NULL;
	uintptr_t address1 = 0;
	int res = 0;
	idt75k_dev_t *dev_p_la1;
	idt75k_dev_t *dev_p_la2;
	dev_p_la1 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));
	dev_p_la2 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));
	/*Ensure that LA-0 has ended its data path reset*/	
	attribute = component_get_attribute(component,kBooleanAttributeLA0DataPathReset);
	temp = attribute_boolean_get_value(attribute);
	bool_value = *(bool*)temp;
	/*bool_value should be zero once data path reset is done.*/
	if(!bool_value) 
	{
		dagutil_warning("LA-0 data path not reset.Problem with system initialization \n");
	}
	/*Enable the firmware access to LA-0*/
	attribute = component_get_attribute(component,kBooleanAttributeLA0AccessEnable);
	bool_value = 1;
	attribute_boolean_set_value(attribute,(void*)&bool_value,1);
	/*Call TCAM Initialise to enable LA-1*/
	card = component_get_card(component);
	state = component_get_private_state(component);
	address1 = (card_get_register_address(card, DAG_REG_INFINICAM, state->mIndex) + (uintptr_t)card_get_iom_address(card));
	
	//TODO: change to pass address of LA1-1 or change the tcam initialise interface.
	memset(dev_p_la1,0,sizeof(idt75k_dev_t));
	memset(dev_p_la2,0,sizeof(idt75k_dev_t));
	
	dev_p_la1->mmio_ptr = (uint8_t*)address1;
	dev_p_la1->mmio_size = 64*1024;
	dev_p_la1->device_id = 0;
	dev_p_la2->mmio_ptr = NULL; //only one interface exposed in the new image.LA1-0
	dev_p_la2->mmio_size = 64*1024;
	dev_p_la2->device_id = 0;
	res = tcam_initialise(dev_p_la1,dev_p_la2);
	if(res != 0)
	{
		printf("Settign GMR aborted abnormally with the following error code :%d\n",res);
	}
	/*Here I assume the phase alignment reset will be issued at startup*/
	/*Ensure that LA-1 has ended its data path reset*/
	attribute = component_get_attribute(component,kBooleanAttributeLA1DataPathReset);
	temp = attribute_boolean_get_value(attribute);
	bool_value = *(bool*)temp;
	/*bool_value should be zero once data path reset is done*/
	if(!bool_value)
	{
		dagutil_warning("LA-1 data path not reset.Problem with system initialization \n");
	}
	/*Enable the firmware access to LA-0*/
	attribute = component_get_attribute(component,kBooleanAttributeLA1AccessEnable);
	bool_value = 1;
	attribute_boolean_set_value(attribute,(void*)&bool_value,1);
	/*Flush the SRAM data*/
	idt75k_flush_device(dev_p_la1);
	/*Need to implement Classifier Enable*/
	attribute = component_get_attribute(component,kUint32AttributeClassifierEnable);
	value = 0;/*all 4 bits set to 1.*/
	attribute_uint32_set_value(attribute,(void*)&value,1);

	/*Disable steering attribute should be set to zero*/
	attribute = component_get_attribute(component,kUint32AttributeDisableSteering);
	value = 0;
	attribute_uint32_set_value(attribute,(void*)&value,1);
}
int infiniband_classifier_post_initialise(ComponentPtr component)
{
	 if (1 == valid_component(component))
	    {
	            DagCardPtr card = NULL;
		    classifier_state_t* state = NULL;     
		    GenericReadWritePtr grw = NULL;
		    AttributePtr attribute = NULL;
		    /* Get card reference */
		    card = component_get_card(component);
		    /* Get counter state structure */
		    state = component_get_private_state(component);
		    /* Add attribute of counter */
		    read_attr_array(component, infiniband_classifier_attr,NB_ELEM, state->mIndex);
                   /*should implement negative logic in LA1-0 data path reset.*/
		    attribute = component_get_attribute(component,kBooleanAttributeLA0DataPathReset);
		    grw = attribute_get_generic_read_write_object(attribute);
		    grw_set_on_operation(grw,kGRWClearBit);

                   /*should implement negative logic in LA1-1 data path reset.*/
		    attribute = component_get_attribute(component,kBooleanAttributeLA1DataPathReset);
		    grw = attribute_get_generic_read_write_object(attribute);
		    grw_set_on_operation(grw,kGRWClearBit);
		    return 1;
	    }
	    return kDagErrInvalidParameter;
}
