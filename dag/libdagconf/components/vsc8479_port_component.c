/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *
 * This component is used by DAG8.nSX cards
 *
 * $Id: vsc8479_port_component.c,v 1.15.16.4 2009/11/19 05:29:35 jomi.gregory Exp $
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
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"
#include "../include/components/vsc8479_port_component.h"

#define BUFFER_SIZE 1024

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: vsc8479_port_component.c,v 1.15.16.4 2009/11/19 05:29:35 jomi.gregory Exp $";
static const char* const kRevisionString = "$Revision: 1.15.16.4 $";

static void vsc8479_dispose(ComponentPtr component);
static void vsc8479_reset(ComponentPtr component);
static void vsc8479_default(ComponentPtr component);
static int vsc8479_post_initialize(ComponentPtr component);
static dag_err_t vsc8479_update_register_base(ComponentPtr component);
static void negated_bit_set_value(AttributePtr attribute, void* value, int length);
static void* negated_bit_get_value(AttributePtr attribute);
static void* master_slave_get_value(AttributePtr attribute);
static void master_slave_set_value(AttributePtr attribute, void* value, int length);
static void master_slave_to_string_routine(AttributePtr attribute);
static void master_slave_from_string_routine(AttributePtr attribute, const char* string);


static void idelay_set_value(AttributePtr attribute, void* value, int length);

/* new get-value function for lOs attribute - read frm xfp/sfp component and OR it */
static void* vsc8479_los_get_value(AttributePtr attribute);
static uint8_t get_sf_xfp_los_value(DagCardPtr card_ptr, uint32_t index);

typedef enum
{
    kControl            = 0x00,
    kRXControl          = 0x04,
    kTXControl          = 0x08,
    kCLKStatus          = 0x0c,
} vsc8479_register_offset_t;

/**
 * VSC8479 transciever's attribute definition array 
 */
Attribute_t vsc8479_attr[]=
{

   {
        /* Name */                 "eql",
        /* Attribute Code */       kBooleanAttributeEquipmentLoopback,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Equipment loopback",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    9,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kRXControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT9,
        /* Default Value */        0,
        /* SetValue */             negated_bit_set_value,
        /* GetValue */             negated_bit_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "fcl",
        /* Attribute Code */       kBooleanAttributeFacilityLoopback,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Facility loopback",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    5,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kTXControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT5,
        /* Default Value */        0,
	//TODO optional: set up in slave mode as well to be confirmed and tryed before writing a new function 
        /* SetValue */             negated_bit_set_value,
        /* GetValue */             negated_bit_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "master_or_slave",
        /* Attribute Code */       kUint32AttributeMasterSlave,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Master or slave",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    6,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kTXControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT6,
        /* Default Value */        0,
        /* SetValue */             master_slave_set_value,
        /* GetValue */             master_slave_get_value,
        /* SetToString */          master_slave_to_string_routine,
        /* SetFromString */        master_slave_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "RXlockerror",
        /* Attribute Code */       kBooleanAttributeReceiveLockError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "RX lock Error presented on the phy",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    31,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kRXControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT31,
        /* Default Value */        0,
        /* SetValue */             negated_bit_set_value,
        /* GetValue */             negated_bit_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "TXfifoerror",
        /* Attribute Code */       kBooleanAttributeFIFOError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Tx FIFO error",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    30,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kTXControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT30,
        /* Default Value */        0,
        /* SetValue */             negated_bit_set_value,
        /* GetValue */             negated_bit_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "los",
        /* Attribute Code */       kBooleanAttributeLossOfSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "PHY RX Los of signal ",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    30,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kRXControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT30,
        /* Default Value */        0,
        /* SetValue */             negated_bit_set_value,
        /* GetValue */             vsc8479_los_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "TXlockerror",
        /* Attribute Code */       kBooleanAttributeLock,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "TX lock Error presented on the phy",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    31,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kTXControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT31,
        /* Default Value */        0,
        /* SetValue */             negated_bit_set_value,
        /* GetValue */             negated_bit_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "IDELAY_Present",
        /* Attribute Code */       kBooleanAttributeIDELAY_Present,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "IDELAY mechanism to control the IDELAY Tap value is presented",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    23,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kCLKStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT23,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },


};


Attribute_t IDELAY_vsc8479_attr[] =
{
   {
        /* Name */                 "IDELAY",
        /* Attribute Code */       kUint32AttributeIDELAY,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "IDELAY Tap Counter Value",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    24,
        /* Register Address */     DAG_REG_VSC8479,
        /* Offset */               kCLKStatus,
        /* Size/length */          6,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 (0x3F << 24),
        /* Default Value */        14,
        /* SetValue */             idelay_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
};

/* Number of elements in array */
#define NB_ELEM (sizeof(vsc8479_attr) / sizeof(Attribute_t))

ComponentPtr
vsc8479_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentPort, card); 
    vsc8479_state_t* state = NULL;
    
    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        component_set_dispose_routine(result, vsc8479_dispose);
        component_set_post_initialize_routine(result, vsc8479_post_initialize);
        component_set_reset_routine(result, vsc8479_reset);
        component_set_default_routine(result, vsc8479_default);
        component_set_update_register_base_routine(result, vsc8479_update_register_base);
        sprintf(buffer, "port%d", index);
        component_set_name(result, buffer);
        state = (vsc8479_state_t*)malloc(sizeof(vsc8479_state_t));
        state->mIndex = index;
	state->mBase = card_get_register_address(card, DAG_REG_VSC8479, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
vsc8479_dispose(ComponentPtr component)
{
}

static dag_err_t
vsc8479_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        vsc8479_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_VSC8479, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
vsc8479_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        vsc8479_state_t* state = NULL;
	AttributePtr attribute = NULL;
//	AttributePtr result = NULL;
    	GenericReadWritePtr grw = NULL;
	char value;
		
        state = component_get_private_state(component);
        read_attr_array(component, vsc8479_attr, NB_ELEM, state->mIndex);
		

	/* set attribute with inverted boolean
	 * Usually set a bit means "write 1", but in these cases it means "write 0" */
	attribute = component_get_attribute(component, kBooleanAttributeFacilityLoopback);
	grw = attribute_get_generic_read_write_object(attribute);
	grw_set_on_operation(grw, kGRWClearBit);

	attribute = component_get_attribute(component, kBooleanAttributeEquipmentLoopback);
	grw = attribute_get_generic_read_write_object(attribute);
	grw_set_on_operation(grw, kGRWClearBit);
	
	attribute = component_get_attribute(component, kBooleanAttributeIDELAY_Present);
	value = *(char *)attribute_get_value(attribute);
	
	if (value)
	{	
        	state = component_get_private_state(component);	
        	read_attr_array(component, IDELAY_vsc8479_attr, sizeof(IDELAY_vsc8479_attr) / sizeof (Attribute_t), state->mIndex);

		
		
//		result = attribute_init(kBooleanAttributeClockLock);	
			//grw = ;
		//attribute_set_generic_read_write_object(result, grw);
//		attribute_set_name(result, "clock_lock");
//		attribute_set_description(result, "used to autorecover the clock");
//		attribute_set_config_status(result, kDagAttrConfig);
//		attribute_set_valuetype(result, kAttributeBoolean);
//		attribute_set_getvalue_routine(result, attribute_boolean_get_value);
//		attribute_set_setvalue_routine(result, attribute_boolean_set_value);
//		attribute_set_to_string_routine(result, attribute_boolean_to_string);
//		attribute_set_from_string_routine(result, attribute_boolean_from_string);
//		attribute_set_masks(result, bit_masks, len);
		
	}

        return 1;
    }
    return kDagErrInvalidParameter;
}

static void
vsc8479_reset(ComponentPtr component)
{   
    uint8_t val = 0;
    if (1 == valid_component(component))
    {
        AttributePtr attribute = component_get_attribute(component, kBooleanAttributeReset);
        attribute_set_value(attribute, (void*)&val, 1);
        vsc8479_default(component);
    }	
}

/* helper function to write to the smbus and the parameter is the index of the component */
static void vsc8479_write_smbus( DagCardPtr card , int index, int vsc_register, uint8_t value) 
{
        GenericReadWritePtr vsc_grw = NULL;
        uint32_t address = 0;

	//SMBUS Address of the VSC chip 
        address = 0x86 << 8;

	// Index of the component 
        address |= (index << 16);
	// Regsiter address to access 	
	address |= vsc_register;

	    vsc_grw = grw_init(card, address, grw_vsc8479_raw_smbus_read, grw_vsc8479_raw_smbus_write);

	assert(vsc_grw);
	
	grw_write(vsc_grw, value);
	
        grw_dispose(vsc_grw);

}

static uint8_t vsc8479_read_smbus( DagCardPtr card , int index, int vsc_register) 
{
        GenericReadWritePtr vsc_grw = NULL;
        uint32_t address = 0;
	int value;

	//SMBUS Address of the VSC chip 
        address = 0x86 << 8;

	// Index of the component 
        address |= (index << 16);
	// Regsiter address to access 	
	address |= vsc_register;

	    vsc_grw = grw_init(card, address, grw_vsc8479_raw_smbus_read, grw_vsc8479_raw_smbus_write);

	assert(vsc_grw);
	
	value = grw_read(vsc_grw);
	
        grw_dispose(vsc_grw);
	return value;
}

const uint8_t vscRegisters[] = 
{
00, /* Register 0 */
00,
00,
00, // TO BE CHECKED 0xX0 /* Register 3 */
00,
00,
00, //Reserved to be verified // Register 6 
00,	/* Register 7 */
0xbb, /* Register 8 */
0xc5, 
0xfc, 
0x07, 
0x00, 
0x05, /* Register 13 */
0x00, //0X TO BE CHECKED Regsiter 14 

0x82, // Not documented Register 15 

0x7F, /* Register 16 */
0x00, 
0x00,
0x59,
0x02,
0x3f,
0x07,
0x00,
0x07, /* Regsiter 24 */
0x00, /* Register 25 */

/* Register 26-Register32 Not documented */
0,0,0,0,0,0,0,

0x00,  /* Register 33 */
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x02 //X2 /* Register 41 */ TO BE CHECKED
};

static void
vsc8479_default(ComponentPtr component)
{
        vsc8479_state_t* state = NULL;
	uint32_t value;
        DagCardPtr card = NULL;
	int i;

    if (1 == valid_component(component))
    {

        int count = component_get_attribute_count(component);
        int index;

       state = component_get_private_state(component);
       card = component_get_card(component);
        
        for (index = 0; index < count; index++)
        {
            AttributePtr attribute = component_get_indexed_attribute(component, index); 
            void* val = attribute_get_default_value(attribute);
            attribute_set_value(attribute, &val, 1);
        }

	//Oner dummy read to the SMBUS to make sure that it is clean and clear 
	value = vsc8479_read_smbus(card,state->mIndex,0);


	//NOTE current assumpton is that we have only one component (VSC8479 chip)
	//If card with more than one shows the 0 needs to be replaced with the component index 

	//Eanble the VSC to acces the settings via programed via SMBUS (SERIALEN) 
	//SoftSMBUS Reset the chip
	vsc8479_write_smbus(card,state->mIndex,0x7f,0xFF);
	usleep(10000);


	value = card_read_iom(card,state->mBase);
	value |= 0x8; 
	card_write_iom(card,state->mBase,value);

	//SoftSMBUS Reset the chip
	vsc8479_write_smbus(card,state->mIndex,0x7f,0xFF);
	usleep(10000);
	//make sure that we are in the correct mode 
	vsc8479_write_smbus(card,state->mIndex,0x7,0);


	for(i=0; i<=41; i++)
	{ 
		//skip undocumented register 15 even after reset shows a value so question ??
		if (i == 15 ) continue;
		//skip resgiters undocumented alweays has been seen as a 0 
		if((i > 25) && (i < 33) ) continue ;

		vsc8479_write_smbus(card,state->mIndex,i,vscRegisters[i]);
	}

	//make sure that we are in the correct mode 
	vsc8479_write_smbus(card,state->mIndex,0x7,0);

//Setting different from the reset defaults and as per datasheet for LOS 
	//We initialize the Threshold to 0 as required per datasheet and fixes a particular cards LOS problem 
	vsc8479_write_smbus(card,state->mIndex,0x14,0);
	// may be needs to be setup this as well according to the VSC datasheet 
	vsc8479_write_smbus(card, state->mIndex,0x13,0x1C);

	
    }	
}

/* TODO: good candidate to be refactored into a common file as it can
 * be used by other components */
static void
negated_bit_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card = NULL;
    uint32_t register_value;
    GenericReadWritePtr bit_grw = NULL;    
    uint8_t val = *(uint8_t*)value;
    uint32_t *masks, mask = 0;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        bit_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(bit_grw);
        masks = attribute_get_masks(attribute);
        mask = masks[0];

        if (val) {
            register_value &= ~(mask); /* negated bit */
        } else {
            register_value |= mask;
        }
        grw_write(bit_grw, register_value);
     }
}

/* TODO: good candidate to be refactored into a common file as it can
 * be used by other components */
static void*
negated_bit_get_value(AttributePtr attribute)
{
   int bit = 0;
   uint32_t register_value = 0;
   GenericReadWritePtr bit_grw = NULL;
   uint32_t *masks, mask;
   
   if (1 == valid_attribute(attribute))
   {
       bit_grw = attribute_get_generic_read_write_object(attribute);
       register_value = grw_read(bit_grw);
       masks = attribute_get_masks(attribute);
       mask = masks[0];
       register_value &= mask;
       if (register_value) {
           bit = 0; /* negated bit */
       } else {
           bit = 1;
       }
       attribute_set_value_array(attribute, &bit, sizeof(bit));
       return (void *)attribute_get_value_array(attribute);
   } 
   return NULL;
}


/* TODO: the following master/slave functions were copied from
 * amcc3485_component.c - they should be refactored eventually,
 * i.e. all the copy-pasted functions moved into a common file
 */
static void* 
master_slave_get_value(AttributePtr attribute)
{
    uint32_t register_value;
    uint32_t value = 0;
    GenericReadWritePtr master_slave_grw = NULL;
    uint32_t *masks;

    if (1 == valid_attribute(attribute))
    {
        master_slave_grw = attribute_get_generic_read_write_object(attribute);
        assert(master_slave_grw);
        register_value = grw_read(master_slave_grw);
        masks = attribute_get_masks(attribute);
        register_value &= masks[0];

        if (!register_value) {
            value = kSlave;
        } else {
            value = kMaster;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
     }
    return NULL;
}

static void
master_slave_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card =NULL;
    uint32_t register_value = 0;
    GenericReadWritePtr master_slave_grw = NULL;    
    master_slave_t master_slave = *(master_slave_t*)value;
    uint32_t *masks;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        master_slave_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(master_slave_grw);
        masks = attribute_get_masks(attribute);
        switch(master_slave)
        {
          case kMasterSlaveInvalid:
               /* nothing */
               break;
          case kMaster:
//               register_value &= ~masks[0]; /* clear */
               register_value |= masks[0]; /* set */
               break;
          case kSlave:
//               register_value |= masks[0]; /* set */
               register_value &= ~masks[0]; /* clear */
               break;
        default:
            assert(0); /* should never get here */
        }
        grw_write(master_slave_grw, register_value);
    }
}

static void
master_slave_to_string_routine(AttributePtr attribute)
{
    master_slave_t value = *(master_slave_t*)master_slave_get_value(attribute);
    const char* temp = master_slave_to_string(value); 
    if (temp)
        attribute_set_to_string(attribute, temp);
}

static void
master_slave_from_string_routine(AttributePtr attribute, const char* string)
{
    master_slave_t value = string_to_master_slave(string);
    if (kMasterSlaveInvalid != value)
        master_slave_set_value(attribute, (void*)&value, sizeof(master_slave_t));
}
static void
idelay_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card =NULL;
    uint32_t old_idelay_tap = 0;
    uint32_t register_value = 0;
    GenericReadWritePtr idelay_grw = NULL;    
    uint32_t new_idelay_tap = *(uint32_t *)value;
    int i;

 //this verification is already done 
 //   	attribute = component_get_attribute(component, kBooleanAttributeIDELAY_Present);
//    	value = *(char *)attribute_get_value(attribute);
	
    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
    	old_idelay_tap = *(char *)attribute_get_value(attribute);	
        idelay_grw = attribute_get_generic_read_write_object(attribute);
	// masks = attribute_get_masks(attribute); 
        new_idelay_tap  &= 0x3f; //manually /* clear */ we can use length to create this
	
	//may be we can create 2 new attributes for incrementing and decrementing but this way works nice
	//with out introducing more attributes
	//Produce a bit for incrementing  which is bit 0x31 of the same register as the IDELAY Tap count 
	for(i=0; i < ((int)new_idelay_tap - (int)old_idelay_tap) ; i++) {
		//hit the increment bit 
        	register_value = grw_read(idelay_grw);
        	grw_write(idelay_grw, register_value | 0x80000000);
		dagutil_nanosleep(200);
	}
	//Produce a bit for decrementing  which is bit 0x30 of the same register as the IDELAY Tap count 
	for(i=0; i < ((int)old_idelay_tap - (int)new_idelay_tap) ; i++) {
		//hit the increment bit 
        	grw_write(idelay_grw, register_value | 0x40000000);
		dagutil_nanosleep(200);
	}
    }
}

static void*
vsc8479_los_get_value(AttributePtr attribute)
{
    uint8_t this_los_value = 0;
    uint8_t sfp_xfp_los_value = 0;
    uint32_t register_value = 0;
    GenericReadWritePtr bit_grw = NULL;
    uint32_t *masks, mask;
    DagCardPtr card_ptr =  NULL;
    vsc8479_state_t* state = NULL;

    if (1 == valid_attribute(attribute))
    {
        /* Get from this component */
        bit_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(bit_grw);
        masks = attribute_get_masks(attribute);
        mask = masks[0];
        register_value &= mask;
        if (register_value) {
            this_los_value = 0; /* negated bit */
        } else {
            this_los_value = 1;
        }
        /* Get the LOS from XPF/SFP module as well */
        card_ptr = attribute_get_card(attribute);
        state =  component_get_private_state( attribute_get_component(attribute));
        if (( NULL != card_ptr) &&  ( NULL != state))
            sfp_xfp_los_value = get_sf_xfp_los_value(card_ptr, state->mIndex);
        /* OR the sfp_xfp_los with this_ los and set it */
        this_los_value |= sfp_xfp_los_value;
        attribute_set_value_array(attribute, &this_los_value, sizeof(this_los_value));
        return (void *)attribute_get_value_array(attribute);
    } 
    return NULL;
}
uint8_t get_sf_xfp_los_value(DagCardPtr card_ptr, uint32_t port_index)
{
    AttributePtr los_attribute = NULL;
    ComponentPtr root_component = NULL;
    ComponentPtr optics_component = NULL;
    uint8_t *ptr_los = NULL;
    uint8_t retval  = 0;
    root_component = card_get_root_component(card_ptr);
    optics_component = component_get_subcomponent(root_component, kComponentOptics, (int) port_index ); 
    if ( NULL == optics_component)
        return 0;
    los_attribute = component_get_attribute(optics_component, kBooleanAttributeLossOfSignal);
    if ( NULL != (ptr_los = (uint8_t*)attribute_get_value(los_attribute) ) )
        retval = *ptr_los;
    return retval;
}
/*
int search_lock(void)
{
	//read the LOS
	if(los_r) return -1; //no signal we can not do the lock 
	//read LOF 	
	if(!lof_r) return 0; //we are good to go 
	
	while()
	{
		inc_idelay();
		dagutil_msleep(2000);
		//check lof
		if(!lof_r){
			//found 1st edge
			break;
		}
	}
	//
	
}
*/
