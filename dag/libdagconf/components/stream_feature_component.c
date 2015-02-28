/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * This file includes the implementation of stream feature component.
 * At the moment supports only per-stream SNAP_LEN attribute.
 *
* $Id: stream_feature_component.c,v 1.14.2.3 2009/07/16 01:28:18 karthik Exp $
 */
#include "dagutil.h"

#include "../include/attribute.h"
#include "../include/util/set.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/stream_feature_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: stream_feature_component.c,v 1.14.2.3 2009/07/16 01:28:18 karthik Exp $";
static const char* const kRevisionString = "$Revision: 1.14.2.3 $";

typedef enum
{
    /* number of streams and registers per stream */
    kGlobal             = 0x00,
    /* indicates the features present */
    kFeaturePresent     = 0x04,
    /*location of each stream registers */
    kPerStreamRegisters             = 0x08,
} stream_feature_register_offset_t;

/* per stream attributes offset from each stream's base */
typedef enum{
    /* Per-stream snaplength register offset*/
    kOffsetSnapLength = 0x00,
    /* Per-stream drop counter register offset */
    kOffsetDropCounter = 0x04,
    /* Per-stream almost-full counter register offset  */
    kOffsetAlmostFullDrop = 0x08,
	/* Per-stream BFS Morphing.*/
	kOffsetBFSMorphing = 0x0c,
	/*Per-stream Ext Erf Header Strip*/
	//kOffsetExtErfHeaderStrip = 0x10
}stream_attribute_offset_t;

/* function declarations */
static void stream_feature_dispose(ComponentPtr component);
static void stream_feature_reset(ComponentPtr component);
static void stream_feature_default(ComponentPtr component);
static int stream_feature_post_initialize(ComponentPtr component);
static dag_err_t stream_feature_update_register_base(ComponentPtr component);
static AttributePtr get_new_snap_length_attribute(ComponentPtr component, int  number);
static AttributePtr get_new_max_snap_length_attribute(ComponentPtr component, int number);
static uint8_t is_feature_present_check(ComponentPtr component, dag_attribute_code_t code);
static AttributePtr get_new_per_stream_drop_count_attribute(ComponentPtr component, int number);
static AttributePtr get_new_per_stream_almost_full_drop_attribute(ComponentPtr component, int number);
static AttributePtr get_new_morphing_configuration_attribute(ComponentPtr component, int number);
static AttributePtr get_new_extension_header_attribute(ComponentPtr component, int number);


Attribute_t stream_feature_attr[] = 
{
    {
        /* Name */                 "streams_count",
        /* Attribute Code */       kUint32AttributeNumberOfStreams,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of streams supported",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_STREAM_FTR,
        /* Offset */               kGlobal,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 (0xff),
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }, 
    {
        /* Name */                 "registers_count",
        /* Attribute Code */       kUint32AttributeNumberOfRegisters,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of registers per stream",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    24,
        /* Register Address */     DAG_REG_STREAM_FTR,
        /* Offset */               kGlobal,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 (0x1f << 24),
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "snaplength_present",
        /* Attribute Code */       kBooleanAttributeSnaplengthPresent,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Per stream snap length is present or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_STREAM_FTR,
        /* Offset */               kFeaturePresent,
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
        /* Name */                 "drop_count_present",
        /* Attribute Code */       kBooleanAttributePerStreamDropCounterPresent,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Per stream drop counter is present or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_STREAM_FTR,
        /* Offset */               kFeaturePresent,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT1,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "almost_full_drop_present",
        /* Attribute Code */       kBooleanAttributePerStreamAlmostFullDropPresent,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Per stream counter, which shows the dropped pkts due to buffer full, is present or not",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_STREAM_FTR,
        /* Offset */               kFeaturePresent,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT2,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	{
        /* Name */                 "bfs_morphing",
        /* Attribute Code */       kBooleanAttributeBfsMorphing,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Value of '1' means per stream BFS morphing.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_STREAM_FTR,
        /* Offset */               kFeaturePresent,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT3,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }
};


/* Number of elements in array */
#define NB_ELEM (sizeof(stream_feature_attr) / sizeof(Attribute_t))

ComponentPtr
stream_feature_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentStreamFeatures, card); 
    stream_feature_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, stream_feature_dispose);
        component_set_post_initialize_routine(result, stream_feature_post_initialize);
        component_set_reset_routine(result, stream_feature_reset);
        component_set_default_routine(result, stream_feature_default);
        component_set_update_register_base_routine(result, stream_feature_update_register_base);
        component_set_name(result, "stream_feature");
        state = (stream_feature_state_t*)malloc(sizeof(stream_feature_state_t));
        state->mIndex = index;
        state->mBase = card_get_register_address(card, DAG_REG_STREAM_FTR, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
stream_feature_dispose(ComponentPtr component)
{
}

static dag_err_t
stream_feature_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        stream_feature_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_STREAM_FTR, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
stream_feature_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        AttributePtr cur_attribute = NULL;
        stream_feature_state_t* state = NULL;
        int stream_count = 0;
        int index = 0;
        uint8_t is_snaplength_present = 0;
        uint8_t is_dropcounter_present = 0;
        uint8_t is_almost_fullcounter_present = 0;
		uint8_t is_per_stream_morphing_feature_present = 0;
		uint32_t *ptr_uint = NULL;

        state = component_get_private_state(component);
        read_attr_array(component, stream_feature_attr, NB_ELEM, state->mIndex);

        // get stream_count 
        cur_attribute = component_get_attribute(component, kUint32AttributeNumberOfStreams);
        if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
        {
            stream_count = *ptr_uint;
        }
        is_snaplength_present = is_feature_present_check( component, kBooleanAttributeSnaplengthPresent);
        is_dropcounter_present = is_feature_present_check( component, kBooleanAttributePerStreamDropCounterPresent);
        is_almost_fullcounter_present = is_feature_present_check( component, kBooleanAttributePerStreamAlmostFullDropPresent);
		is_per_stream_morphing_feature_present = is_feature_present_check( component, kBooleanAttributeBfsMorphing);
		for ( index = 0; index < stream_count; index++)
        {
            if ( is_snaplength_present )
            {
                /* create new attribute for max snaplength */
                cur_attribute = get_new_max_snap_length_attribute ( component, index ) ;
                if ( NULL != cur_attribute)
                        component_add_attribute(component, cur_attribute);
                cur_attribute = NULL;
                /* create new attribute for snaplength */
                cur_attribute = get_new_snap_length_attribute ( component, index ) ;
                if ( NULL != cur_attribute)
                        component_add_attribute(component, cur_attribute);
                cur_attribute = NULL;
            }
            if ( is_dropcounter_present)
            {
                /* create new attribute for drop counter */
                cur_attribute = get_new_per_stream_drop_count_attribute ( component, index ) ;
                if ( NULL != cur_attribute)
                        component_add_attribute(component, cur_attribute);
                cur_attribute = NULL;
            }
            if ( is_almost_fullcounter_present)
            {
                /* create new attribute for almost full drop */
                cur_attribute = get_new_per_stream_almost_full_drop_attribute ( component, index ) ;
                if ( NULL != cur_attribute)
                        component_add_attribute(component, cur_attribute);
                cur_attribute = NULL;
            }
	    	if( is_per_stream_morphing_feature_present)
	    	{
			/* create new attribute for almost full drop */
                cur_attribute = get_new_morphing_configuration_attribute( component, index ) ;
                if ( NULL != cur_attribute)
                        component_add_attribute(component, cur_attribute);
                cur_attribute = NULL;
				
				cur_attribute = get_new_extension_header_attribute( component, index ) ;
                if ( NULL != cur_attribute)
                        component_add_attribute(component, cur_attribute);
                cur_attribute = NULL;
			}
	   }
        return 1;
    }
    return kDagErrInvalidParameter;
}

static void
stream_feature_reset(ComponentPtr component)
{   
    if (1 == valid_component(component))
    {
        /* there is no reset bit available */
        stream_feature_default(component);
    }    
}


static void
stream_feature_default(ComponentPtr component)
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

static AttributePtr
get_new_snap_length_attribute(ComponentPtr component, int number)
{
    AttributePtr result = NULL;
    AttributePtr cur_attribute = NULL;
    char name[32] ;
    uintptr_t offset = 0;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t register_count = 0 ;
    uint32_t mask = 0xffff;
    uint32_t deflt_value = 10240;
    int length = 1;
    uint32_t num_module = 0;
    uint32_t *ptr_uint = NULL; 
    stream_feature_state_t *state = NULL;

    state = component_get_private_state(component); 
    num_module = state->mIndex;
    
    /* get the register count per stream */
    cur_attribute = component_get_attribute(component, kUint32AttributeNumberOfRegisters);
    if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
    {
        register_count = *ptr_uint;
    }
    cur_attribute = NULL;
    cur_attribute = component_indexed_get_attribute_of_type(component, kUint32AttributeMaxSnapLen, number);
    if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
    {
        deflt_value = *ptr_uint;
    }
    sprintf(name,"stream_snaplength%d",number);
        
    card = component_get_card(component);
    offset =  kPerStreamRegisters + (number * 4 * register_count );
    base_reg = card_get_register_address(card, DAG_REG_STREAM_FTR, num_module);
    address = ((uintptr_t)card_get_iom_address(card) + base_reg  + offset);
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);
    result = attribute_init(kUint32AttributePerStreamSnaplength);
    grw_set_attribute(grw, result);
    attribute_set_generic_read_write_object(result, grw);

    attribute_set_name(result, name);
    attribute_set_description(result, "Snap length for stream");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    //attribute_set_dispose_routine(result, attribute_dispose);
   // attribute_set_post_initialize_routine(result, attribute_post_initialize);
    //TEMP FIX for 64 bit compiler warning- typecasting to unsigned long
    attribute_set_default_value ( result, (void *) (unsigned long) deflt_value);
    attribute_set_masks( result, &mask, length );
    
    return result;
}

static AttributePtr
get_new_max_snap_length_attribute(ComponentPtr component, int number)
{
    AttributePtr result = NULL;
    AttributePtr cur_attribute = NULL;
    char name[32] ;
    uintptr_t offset = 0;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t register_count = 0 ;
    uint32_t mask = 0xffff0000;
    int length = 1;
    uint32_t num_module = 0;
    uint32_t *ptr_uint = NULL;
    stream_feature_state_t *state = NULL;

    state = component_get_private_state(component); 
    num_module = state->mIndex;

    /* get the register count per stream */
    cur_attribute = component_get_attribute(component, kUint32AttributeNumberOfRegisters);
    if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
    {
        register_count = *ptr_uint;
    }
    sprintf(name,"max_stream_snaplength%d",number);
    card = component_get_card(component);
    offset =  kPerStreamRegisters + (number * 4 * register_count );
    base_reg = card_get_register_address(card, DAG_REG_STREAM_FTR, num_module);
    address = ((uintptr_t)card_get_iom_address(card) + base_reg  + offset);
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);

    result = attribute_init(kUint32AttributeMaxSnapLen);

    grw_set_attribute(grw, result);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, name);
    attribute_set_description(result, "Maximum allowed Snap length for stream");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    
    attribute_set_masks( result, &mask, length );
    return result;
}
/*Per Stream BFS Morphing - Morphing Configuration Register.*/
static AttributePtr
get_new_morphing_configuration_attribute(ComponentPtr component, int number)
{
    AttributePtr result = NULL;
    AttributePtr cur_attribute = NULL;
    char name[32] ;
    uintptr_t offset = 0;
    uintptr_t base_reg = 0;
    uintptr_t mBase = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t register_count = 0 ;
    uint32_t mask = 0x007f;
    int length = 1;
    uint32_t num_module = 0;
    uint32_t *ptr_uint = NULL;
    stream_feature_state_t *state = NULL;

    state = component_get_private_state(component); 
    num_module = state->mIndex;

    /* get the register count per stream */
    cur_attribute = component_get_attribute(component, kUint32AttributeNumberOfRegisters);
    if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
    {
        register_count = *ptr_uint;
    }
    sprintf(name,"morphing_configuration%d",number);
    card = component_get_card(component);
    base_reg =  kPerStreamRegisters + (number * 4 * register_count );
	offset = base_reg + kOffsetBFSMorphing;
    mBase = card_get_register_address(card, DAG_REG_STREAM_FTR, num_module);
    address = ((uintptr_t)card_get_iom_address(card) + mBase  + offset);
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);

    result = attribute_init(kUint32AttributeMorphingConfiguration);

    grw_set_attribute(grw, result);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, name);
    attribute_set_description(result, "Set this field to choose the packet type you want to recieve.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    
    attribute_set_masks( result, &mask, length );
    return result;
}

/*Extension Header - Set this bit if you want to keep the extension header.*/
static AttributePtr
get_new_extension_header_attribute(ComponentPtr component, int number)
{
    AttributePtr result = NULL;
    AttributePtr cur_attribute = NULL;
    char name[32] ;
    uintptr_t offset = 0;
    uintptr_t base_reg = 0;
	uintptr_t mBase = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t register_count = 0 ;
    uint32_t mask = BIT7;
    int length = 1;
    uint32_t num_module = 0;
    uint32_t *ptr_uint = NULL;
    stream_feature_state_t *state = NULL;

    state = component_get_private_state(component); 
    num_module = state->mIndex;

    /* get the register count per stream */
    cur_attribute = component_get_attribute(component, kUint32AttributeNumberOfRegisters);
    if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
    {
        register_count = *ptr_uint;
    }
    sprintf(name,"extension_header%d",number);
    card = component_get_card(component);
    base_reg =  kPerStreamRegisters + (number * 4 * register_count );
    offset = base_reg + kOffsetBFSMorphing;
    mBase = card_get_register_address(card, DAG_REG_STREAM_FTR, num_module);
    address = ((uintptr_t)card_get_iom_address(card) + mBase  + offset);
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);

    result = attribute_init(kBooleanAttributeExtErfHeaderStripConfigure);

    grw_set_attribute(grw, result);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, name);
    attribute_set_description(result, "Set this field if you want to keep the extension header.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    
    attribute_set_masks( result, &mask, length );
    return result;
}

uint8_t is_feature_present_check(ComponentPtr component, dag_attribute_code_t code)
{
    uint8_t *ptr_bool = NULL;
    AttributePtr cur_attribute = NULL;
    cur_attribute = component_get_attribute(component, code);
    if ( (kNullAttributeUuid == cur_attribute) || (kAttributeBoolean != attribute_get_valuetype(cur_attribute)) )
        return 0;
    if ( NULL != (ptr_bool = (uint8_t*)attribute_boolean_get_value(cur_attribute) ))
    {
        return *ptr_bool;
    }
    return 0;
}
AttributePtr
get_new_per_stream_drop_count_attribute(ComponentPtr component, int number)
{
    AttributePtr result = NULL;
    AttributePtr cur_attribute = NULL;
    char name[32] ;
    uintptr_t offset = 0;
    uintptr_t stream_base = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t register_count = 0 ;
    uint32_t mask = 0xffffffff;
    int length = 1;
    uint32_t num_module = 0;
    uint32_t *ptr_uint = NULL; 
    stream_feature_state_t *state = NULL;

    state = component_get_private_state(component); 
    if (NULL == state )
    {
        return NULL;
    }
    num_module = state->mIndex;
    
    /* get the register count per stream */
    cur_attribute = component_get_attribute(component, kUint32AttributeNumberOfRegisters);
    if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
    {
        register_count = *ptr_uint;
    }
    sprintf(name,"stream_drop_count%d",number);
        
    card = component_get_card(component);
    stream_base =  kPerStreamRegisters + (number * 4 * register_count );
    offset = stream_base + kOffsetDropCounter;
    address = ((uintptr_t)card_get_iom_address(card) + state->mBase  + offset);
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);

    result = attribute_init(kUint32AttributeStreamDropCount);
    grw_set_attribute(grw, result);
    attribute_set_generic_read_write_object(result, grw);

    attribute_set_name(result, name);
    attribute_set_description(result, "drop counter per stream");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks( result, &mask, length );
    return result;
}

AttributePtr
get_new_per_stream_almost_full_drop_attribute(ComponentPtr component, int number)
{
    AttributePtr result = NULL;
    AttributePtr cur_attribute = NULL;
    char name[32] ;
    uintptr_t offset = 0;
    uintptr_t stream_base = 0;
    uintptr_t address = 0;
    DagCardPtr card;
    GenericReadWritePtr grw = NULL;
    uint32_t register_count = 0 ;
    uint32_t mask = 0xffffffff;
    int length = 1;
    uint32_t num_module = 0;
    uint32_t *ptr_uint = NULL; 
    stream_feature_state_t *state = NULL;

    state = component_get_private_state(component); 
    if (NULL == state )
    {
        return NULL;
    }
    num_module = state->mIndex;
    
    /* get the register count per stream */
    cur_attribute = component_get_attribute(component, kUint32AttributeNumberOfRegisters);
    if ( NULL != ( ptr_uint = (uint32_t*)attribute_uint32_get_value( cur_attribute) ) )
    {
        register_count = *ptr_uint;
    }
    sprintf(name,"almost_full_drop%d",number);
        
    card = component_get_card(component);
    stream_base =  kPerStreamRegisters + (number * 4 * register_count );
    offset = stream_base + kOffsetAlmostFullDrop;
    address = ((uintptr_t)card_get_iom_address(card) + state->mBase  + offset);
    grw = grw_init(card, address, grw_iom_read, grw_iom_write);

    result = attribute_init(kUint32AttributeStreamAlmostFullDrop);
    grw_set_attribute(grw, result);
    attribute_set_generic_read_write_object(result, grw);

    attribute_set_name(result, name);
    attribute_set_description(result, "Pkts dropped because the buffer is almost full");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks( result, &mask, length );
    return result;
}

