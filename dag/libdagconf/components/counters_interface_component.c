/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *
 * This component is used by DAG 5.0S (OC-48) and DAG8.0SX (OC-192) as well.
 *
 * $Id: counters_interface_component.c,v 1.6 2009/05/21 01:49:10 dlim Exp $
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
#include "../include/components/counters_interface_component.h"
#include "../include/components/counter_component.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"


/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: counters_interface_component.c,v 1.6 2009/05/21 01:49:10 dlim Exp $";
static const char* const kRevisionString = "$Revision: 1.6 $";

static void counters_interface_dispose(ComponentPtr component);
static void counters_interface_reset(ComponentPtr component);
static void counters_interface_default(ComponentPtr component);
static int counters_interface_post_initialize(ComponentPtr component);
static dag_err_t counters_interface_update_register_base(ComponentPtr component);

/*
static void refresh_cache_set_value(AttributePtr attribute, void* value, int length);
static void* refresh_cache_get_value(AttributePtr attribute);
static void reset_set_value(AttributePtr attribute, void* value, int len);
static void* reset_get_value(AttributePtr attribute);
*/

#define OFFSET_FIELD 0x04
/**
 * Counter Statistics Interface's Attribute definition array 
 */
Attribute_t counters_interface_attr[]=
{

    {
        /* Name */                 "csi_type",
        /* Attribute Code */       kUint32AttributeCSIType,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Counter Statistics Interface type",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    16,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFF0000,
        /* Default Value */        1,
        /* SetValue */             attribute_uint32_set_value,
		/* GetValue */             attribute_uint32_get_value,
		/* SetToString */          attribute_uint32_to_string,
		/* SetFromString */        attribute_uint32_from_string,
		/* Dispose */              attribute_dispose,
		/* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "csi_nb_counters",
        /* Attribute Code */       kUint32AttributeNbCounters,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of counters in CSI",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    8,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFF00,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	
    {
        /* Name */                 "csi_latch_clear",
        /* Attribute Code */       kBooleanAttributeLatchClear,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Latch & Clear set up",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               0,
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
        /* Name */                 "desc_base_add",
        /* Attribute Code */       kUint32AttributeCounterDescBaseAdd,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Counter description base address",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    16,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               OFFSET_FIELD,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFF0000,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
	
    {
        /* Name */                 "value_base_add",
        /* Attribute Code */       kUint32AttributeCounterValueBaseAdd,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Counter value base address",
        /* Config-Status */        kDagAttrStatus,
		/* Index in register */    0,
        /* Register Address */     DAG_REG_COUNT_INTERF,
        /* Offset */               OFFSET_FIELD,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFF,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },	
};

/* Number of elements in array */
#define NB_ELEM (sizeof(counters_interface_attr) / sizeof(Attribute_t))

/* terf component. */
ComponentPtr
counters_interface_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentInterface, card); 
    counters_interface_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, counters_interface_dispose);
        component_set_post_initialize_routine(result, counters_interface_post_initialize);
        component_set_reset_routine(result, counters_interface_reset);
        component_set_default_routine(result, counters_interface_default);
        component_set_update_register_base_routine(result, counters_interface_update_register_base);
        component_set_name(result, "counters_interface");
        state = (counters_interface_state_t*)malloc(sizeof(counters_interface_state_t));
        state->mIndex = index;
		state->mBase = card_get_register_address(card, DAG_REG_COUNT_INTERF, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
counters_interface_dispose(ComponentPtr component)
{
}

static dag_err_t
counters_interface_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        counters_interface_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_COUNT_INTERF, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
counters_interface_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        DagCardPtr card;
		counters_interface_state_t* csi_state = NULL;
		counter_state_t* counter_state = NULL;
		dag_counter_value_t counter;
		
		ComponentPtr c_counter = NULL;
		AttributePtr attr = NULL;
		
		int i = 0; /* indice of counters */
		int n =0; /* indice of 32-bit space in DRB memory base (64-bit counter is 2*32-bit space) */
		int nb_counters = 0;
		int version = 0;
		
		uint32_t descr_base = 0; /* Value of base address for the Counter Description register */
		uint32_t value_base = 0; /* Value of base address for the counter value register */
		uint32_t indirect_add = 0; /* Value of the address for the counter value register (only available with Direct Accessing) */
		
		uint32_t* ind_addr_field =  NULL;
		uint32_t* ind_data_reg =  NULL;
		
		uint32_t* addr_db =  NULL; /* Base address of counter description register */
		uint32_t* addr_vb =  NULL; /* Base address of counter value register */
		uint32_t* addr_cv =  NULL; /* Address of the counter value register (only available with Direct Accessing) */
		uint32_t reg_count_desc =0;
		
        csi_state = component_get_private_state(component);

		card = component_get_card(component);
        /* Add attribute of counters_interface */ 
        read_attr_array(component, counters_interface_attr, NB_ELEM, csi_state->mIndex);
		
		/* Add counter components */
		attr = component_get_attribute(component, kUint32AttributeNbCounters);
		nb_counters = *((int*)(attribute_get_value(attr)));
		
		/* Get the version of the csi component */
		version = card_get_register_version(card, DAG_REG_COUNT_INTERF, csi_state->mIndex);
		
		/* Address of the card, DRB, Indirect addressing field and Indirect data register */
		attr = component_get_attribute(component, kUint32AttributeCounterDescBaseAdd);
		descr_base = *((uint32_t*)attribute_get_value(attr));
		attr = component_get_attribute(component, kUint32AttributeCounterValueBaseAdd); 
		value_base = *((uint32_t*)attribute_get_value(attr));
		ind_addr_field = (uint32_t*)((uintptr_t)card_get_iom_address(card) + card_get_register_address(card,DAG_REG_COUNT_INTERF, csi_state->mIndex) + INDIRECT_ADDR_OFFSET);
		ind_data_reg = (uint32_t*)((uintptr_t)card_get_iom_address(card) + card_get_register_address(card, DAG_REG_COUNT_INTERF, csi_state->mIndex) + INDIRECT_DATA_OFFSET);
		addr_db = (uint32_t*)(card_get_iom_address(card) + descr_base); 
		addr_vb = (uint32_t*)(card_get_iom_address(card) + value_base); 
		
		for(i = 0; i < nb_counters; i++)
		{
			c_counter =  counter_get_new_component(card, i);
			component_set_parent(c_counter, component);
			component_add_subcomponent(component, c_counter);
			
			counter_state = component_get_private_state(c_counter);
			counter_state->mIndex = i;

			/* Get address/offset description and address/offset value */
			switch(version)
			{
				/* Direct block */
				case 0:
				{
					reg_count_desc =  *(addr_db);
					/* Read counter ID, size (32 or 64 bits) and Latch and clear info */
					counter.typeID = (uint8_t)(reg_count_desc >> 16);
					counter.subfct = (uint8_t)(reg_count_desc & 0x60);
					counter.size = (uint8_t)(reg_count_desc & 0x07);
					counter.value_type = (uint8_t)((reg_count_desc & 0x10) >> 4);

					/* Save addresse of counter description in state structure */					
					counter_state->mDescrAddress = addr_db;
					addr_db++;

					/* Check if it is the address (=1) or the value (=0)*/
					if (counter.value_type == 0)
					{
						/* Save address of the counter value in state structure */
						counter_state->mValueAddress =  addr_vb;
						
						/* Check data size 32 or 64 bits */
						if (counter.size == 1)
						{
							addr_vb++;
							addr_vb++;
						}
						else
						{
							addr_vb++;
						}
					}
					else /* address */
					{
						indirect_add = *(addr_vb); 
						addr_vb++;
						addr_cv = (uint32_t*)(card_get_iom_address(card) + indirect_add); 
						
						/* Save address of the counter value in state structure */
						counter_state->mValueAddress =  addr_cv;
						
						/* Check data size 32 or 64 bits */
						/*if (counter.size == 1)
						{
							addr_cv++;
							addr_cv++;
						}
						else
						{
							addr_cv++;
						}*/
					}
					break;
				}
				/* Indirect block */
				case 1:
				{
					/* To retrieve the data from the counter description field
					 * write description base address in the Indirect addressing field */
					*ind_addr_field = descr_base + i;
					/* read data in the indirect data register */
					reg_count_desc = (uint32_t)*(ind_data_reg);
					
					/* Read counter ID, size (32 or 64 bits) and Latch and clear info */
					counter.typeID = (uint8_t)(reg_count_desc >> 16);
					counter.subfct = (uint8_t)(reg_count_desc & 0x60);
					counter.size = (uint8_t)(reg_count_desc & 0x07);
					
					counter_state->mDescrOffset = descr_base + i;
					counter_state->mValueOffset =  value_base + n;
					
					/* Check data size 32 or 64 bits */
					if (counter.size == 1)
					{
						n += 2;
					}
					else
					{
						n++;
					}
					break;
				}
			}
		}
		
		return 1;
    }
    return kDagErrInvalidParameter;
}

static void
counters_interface_reset(ComponentPtr component)
{   
    uint8_t val = 0;
    if (1 == valid_component(component))
    {
		AttributePtr attribute = component_get_attribute(component, kBooleanAttributeReset);
		attribute_set_value(attribute, (void*)&val, 1);
		counters_interface_default(component);
    }	
}


static void
counters_interface_default(ComponentPtr component)
{

    //set all the attribute default values
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

#if 0

static void
refresh_cache_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == 1)
        {
            counters_interface_state_t* state = NULL;
            ComponentPtr comp = attribute_get_component(attribute);
            DagCardPtr card = attribute_get_card(attribute);

            state = component_get_private_state(comp);
            state->mStatusCache = card_read_iom(card, state->mBase + kStatus);

            state->mB1ErrorCounterCache = card_read_iom(card, state->mBase + kB1ErrorCounter);
            state->mB2ErrorCounterCache = card_read_iom(card, state->mBase + kB2ErrorCounter);
            state->mB3ErrorCounterCache = card_read_iom(card, state->mBase + kB3ErrorCounter);

            state->mRXPacketCounterCache = card_read_iom(card, state->mBase + kRXPacketCounter);
            state->mRXByteCounterCache = card_read_iom(card, state->mBase + kRXByteCounter);
            state->mTXPacketCounterCache = card_read_iom(card, state->mBase + kTXPacketCounter);
            state->mTXByteCounterCache = card_read_iom(card, state->mBase + kTXByteCounter);

            state->mRXLongCounterCache = card_read_iom(card, state->mBase + kRXLongCounter);
            state->mRXShortCounterCache = card_read_iom(card, state->mBase + kRXShortCounter);
            state->mRXAbortCounterCache = card_read_iom(card, state->mBase + kRXAbortsCounter);
            state->mRXFCSErrorCounterCache = card_read_iom(card, state->mBase + kRXFCSErrorCounter);
        }
    }
}

static void*
refresh_cache_get_value(AttributePtr attribute)
{
    /* get value is not used */
    /* place holder to prevent clearing the register on read */
    if (1 == valid_attribute(attribute))
    {
        return  0;
    }
    return NULL;
}

/*
 * There is no genereal reset bit on DAG 5.0S, we must reset the
 * framer, deframer and PHY separately
 */
static void
reset_set_value(AttributePtr attribute, void* value, int len)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t sdh_config_register; /* framer reset, deframer reset */
    uint32_t phy_config_register; /* PHY reset (negated!!!) */
    counters_interface_state_t* state = NULL;
    GenericReadWritePtr grw_phy = NULL;
    uintptr_t phy_address = 0;

    if (1 == valid_attribute(attribute))
    {
        /* read SDH config register */
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        sdh_config_register = card_read_iom(card, state->mBase + kConfig);

        /* read PHY control register */
        phy_address = card_get_register_address(card, DAG_REG_AMCC3485, 0); /* PHY A */
        phy_address = (uintptr_t)(card_get_iom_address(card) + phy_address);
        /* phy_address should be 0x0100 (A) or 0x0104 (B) */
        phy_address += (state->mIndex << 2);

        grw_phy = grw_init(card, phy_address, grw_iom_read, grw_iom_write);
        phy_config_register = grw_read(grw_phy);

        if (*(uint8_t*)value == 0)
        {
            sdh_config_register &= ~BIT6; /* framer reset */
            sdh_config_register &= ~BIT7; /* deframer reset */
            phy_config_register |= BIT11; /* PHY reset is active low */
        }
        else
        {
            sdh_config_register |= BIT6;
            sdh_config_register |= BIT7;
            phy_config_register &= ~BIT11;
        }
        card_write_iom(card, state->mBase + kConfig, sdh_config_register);
        grw_write(grw_phy, phy_config_register);
        free(grw_phy);
    }
}


/*
 * Return the reset value of the framer, deframer and PHY
 */
static void*
reset_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t sdh_config_register; /* framer reset, deframer reset */
    uint32_t phy_config_register; /* PHY reset (negated!!!) */
    counters_interface_state_t* state = NULL;
    GenericReadWritePtr grw_phy = NULL;
    uintptr_t phy_address = 0;
    static uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        /* read SDH config register */
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        sdh_config_register = card_read_iom(card, state->mBase + kConfig);

        /* read PHY control register */
        phy_address = card_get_register_address(card, DAG_REG_AMCC3485, 0); /* PHY A */
        phy_address = (uintptr_t)(card_get_iom_address(card) + phy_address);
        /* phy_address should be 0x0100 (A) or 0x0104 (B) */
        phy_address += (state->mIndex << 2);

        grw_phy = grw_init(card, phy_address, grw_iom_read, grw_iom_write);
        phy_config_register = grw_read(grw_phy);

        if ((sdh_config_register & (BIT6|BIT7)) || 
            !(phy_config_register & BIT11)) 
        {
            value = 1;
        }
        return &value;
    }
    return NULL;
}
#endif


