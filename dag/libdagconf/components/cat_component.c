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

static void cat_dispose(ComponentPtr component);
static void cat_reset(ComponentPtr component);
static void cat_default(ComponentPtr component);
static int cat_post_initialize(ComponentPtr component);
static dag_err_t cat_update_register_base(ComponentPtr component);

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: cat_component.c,v 1.13.2.1 2009/11/25 02:15:40 karthik.sharma Exp $";
static const char* const kRevisionString = "$Revision: 1.13.2.1 $";

/* CAT Register offset enum */
typedef enum
{
	kControl              = 0x0000,
	kAddress              = 0x0004,
	kData                 = 0x0008,
	kDeliberateDropCount  = 0x000c
}cat_register_offset_t;

/* CAT Attribute definition Array */
Attribute_t cat_attr[]=
{
    {
        /* Name */                 "write_enable",
        /* Attribute Code */       kBooleanAttributeWriteEnable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Write Enable Pin.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
		/* GetValue */             attribute_boolean_get_value,
		/* SetToString */          attribute_boolean_to_string,
		/* SetFromString */        attribute_boolean_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
    },
	{
        /* Name */                 "hash_width",
        /* Attribute Code */       kUint32AttributeHashSize,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Set the size of the hash to be provided to the CAT.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT1 | BIT2 | BIT3,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
    },
	{
        /* Name */                 "variable_hash_support",
        /* Attribute Code */       kBooleanAttributeVariableHashSupport,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Variable Hash Supported.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    7,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT5,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "interface_overwrite_enable",
        /* Attribute Code */       kBooleanAttributeEnableInterfaceOverwrite,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Interface Overwrite Enable Pin.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    7,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT7,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "forward_destinations",
        /* Attribute Code */       kUint16AttributeForwardDestinations,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Inline Forward Destinations.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    8,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT8|BIT9|BIT10|BIT11,
        /* Default Value */        0,/*NEED TO CHECK*/
     	/* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "bank_select",
        /* Attribute Code */       kBooleanAttributeBankSelect,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "The Bank Select Bit.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    12,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT12,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_int_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "clear_counter",
        /* Attribute Code */       kBooleanAttributeClearCounter,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Clear Counter Bit.",
        /* Config-Status */        kDagAttrConfig,
		/* Index in register */    13,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT13,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "by_pass",
        /* Attribute Code */       kBooleanAttributeByPass,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "All packets to first destination.",
        /* Config-Status */        kDagAttrConfig,
		/* Index in register */    14,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT14,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "output_type",
        /* Attribute Code */       kBooleanAttributeOutputType,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Output type.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    15,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT15,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "number_of_output_bits",
        /* Attribute Code */       kUint32AttributeNumberOfOutputBits,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of Output Bits.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    16,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT16|BIT17|BIT18|BIT19|BIT20|BIT21,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "number_of_input_bits",
        /* Attribute Code */       kUint32AttributeNumberOfInputBits,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of Input Bits.",
		/* Config-Status */        kDagAttrStatus,
		/* Index in register */    24,
        /* Register Address */     DAG_REG_CAT,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT24|BIT25|BIT26|BIT27|BIT28|BIT29,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
		/* Name */                 "deliberate_drop_count",
		/* Attribute Code */       kUint32AttributeDeliberateDropCount,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "The Deliberate Drop Count Register.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_CAT,
		/* Offset */               kDeliberateDropCount,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF,
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
   },
   {
		/* Name */                 "address_register",
		/* Attribute Code */       kUint32AttributeAddressRegister,
		/* Attribute Type */       kAttributeUint32,
		/* Description */          "The Address Register.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_CAT,
		/* Offset */               kAddress,
		/* Size/length */          1,      
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0x7FFF, /*14 bit address`+ 1 bank select bit*/
		/* Default Value */        0,
		/* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
   },
   {
		/* Name */                 "data_register",
		/* Attribute Code */       kInt32AttributeDataRegister,
		/* Attribute Type */       kAttributeInt32,
		/* Description */          "The Data Register.",
		/* Config-Status */        kDagAttrConfig,
		/* Index in register */    0,
		/* Register Address */     DAG_REG_CAT,
		/* Offset */               kData,
		/* Size/length */          1,
		/* Read */                 grw_iom_read,
		/* Write */                grw_iom_write,
		/* Mask */                 0xFFFFFFFF, 
		/* Default Value */        0,
		/* SetValue */             attribute_int32_set_value,
		/* GetValue */             attribute_int32_get_value,
		/* SetToString */          attribute_int32_to_string,
		/* SetFromString */        attribute_int32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
   }
};
/* Number of elements in the array */
#define NB_ELEM (sizeof(cat_attr) / sizeof(Attribute_t))

ComponentPtr
cat_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentCAT, card); 
    cat_state_t* state = NULL;
    if (NULL != result)
    {
        component_set_dispose_routine(result, cat_dispose);
        component_set_post_initialize_routine(result, cat_post_initialize);
        component_set_reset_routine(result, cat_reset);
        component_set_default_routine(result, cat_default);
        component_set_update_register_base_routine(result, cat_update_register_base);
        component_set_name(result, "cat_interface");
        state = (cat_state_t*)malloc(sizeof(cat_state_t));
        state->mIndex = index;
        state->mCatBase = card_get_register_address(card, DAG_REG_CAT, state->mIndex);
        component_set_private_state(result, state);
    }
    return result;
}
static dag_err_t
cat_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        cat_state_t* state = NULL;
        DagCardPtr card;
	state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mCatBase = card_get_register_address(card, DAG_REG_CAT, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}
static int
cat_post_initialize(ComponentPtr component)
{
   if (1 == valid_component(component))
   {
        DagCardPtr card = NULL;
	cat_state_t* state = NULL;	
	/* Get card reference */
	card = component_get_card(component);
        /* Get counter state structure */
	state = component_get_private_state(component);
        /* Add attribute of counter */
        read_attr_array(component, cat_attr, NB_ELEM, state->mIndex);
	/*Need to add the kSturctAttributeRawCATEntry Info here*/
	return 1;
    }
    return kDagErrInvalidParameter;
}
static void
cat_default(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        int count = component_get_attribute_count(component);
        int index;
        for (index = 0; index < count; index++)
        {
            AttributePtr attribute = component_get_indexed_attribute(component, index); 
            void* val = attribute_get_default_value(attribute);
            attribute_set_value(attribute, &val, 1);
        }
    }    
}
static void
cat_dispose(ComponentPtr component)
{
}
static void
cat_reset(ComponentPtr component)
{
}

