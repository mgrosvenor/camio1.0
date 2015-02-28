/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* File header. */
#include "include/create_attribute.h"

/* Public API headers. */

/* Internal project headers. */
#include "include/card_types.h"
#include "include/util/utility.h"
#include "include/modules/latch.h"
#include "include/modules/generic_read_write.h"
#include "include/attribute_factory.h"
#include "include/cards/common_dagx_constants.h"
#include "include/card.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined (__linux__)
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dagutil.h"

AttributePtr
create_attribute(uint32_t index_tab, ComponentPtr component, Attribute_t tab[], uint32_t num_module)
{
    AttributePtr result = NULL;
    GenericReadWritePtr grw = NULL;  
    DagCardPtr card;
	dag_card_t card_type = 0;
    uintptr_t address;
    uintptr_t base_reg = 0;
	uintptr_t offset = 0;

	card = component_get_card(component);
    base_reg = card_get_register_address(card, tab[index_tab].mReg, num_module);
	offset = tab[index_tab].mOffset;
	
	if(tab[index_tab].mAttributeCode == kBooleanAttributeActive)
	{
		base_reg = card_get_register_address(card, tab[index_tab].mReg, 0);	
		offset = (num_module +1)*SP_OFFSET + SP_CONFIG;
	}
	
	if(tab[index_tab].mAttributeCode == kUint32AttributeDropCount)
	{
		base_reg = card_get_register_address(card, tab[index_tab].mReg, 0);
		offset = (num_module +1)*SP_OFFSET + SP_DROP;
	}

    address = ((uintptr_t)card_get_iom_address(card) + base_reg + offset);
    grw = grw_init(card, address, tab[index_tab].fRead, tab[index_tab].fWrite);
    
	card_type = card_get_card_type(card);
	//Fixed here the ivertion is made at the component level 
//	if ( (card_type == kDag50s) || (card_type == kDag50z) )
//		if((tab[index_tab].mAttributeCode == kBooleanAttributeActive)
//			|| (tab[index_tab].mAttributeCode == kBooleanAttributeEquipmentLoopback)
//			|| (tab[index_tab].mAttributeCode == kBooleanAttributeFacilityLoopback))
//			grw_set_on_operation(grw, kGRWClearBit);
    
    result = attribute_init(tab[index_tab].mAttributeCode);
    grw_set_attribute(grw, result);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result,tab[index_tab].mName);
    attribute_set_description(result, tab[index_tab].mDescription);
    attribute_set_config_status(result,tab[index_tab].mConfigStatus);
    attribute_set_valuetype(result,tab[index_tab].mAttributeType);
    attribute_set_getvalue_routine(result, tab[index_tab].fGetValue);
    attribute_set_setvalue_routine(result, tab[index_tab].fSetValue);
    attribute_set_to_string_routine(result, tab[index_tab].fToString);
    attribute_set_from_string_routine(result, tab[index_tab].fFromString);
    attribute_set_masks(result, &tab[index_tab].mMask, tab[index_tab].mLength);
    //TEMP FIX FOR 64 bit system  
    attribute_set_default_value(result, (void*)(unsigned long)tab[index_tab].mDefaultVal);
    return result;
	
}

void
read_attr_array(ComponentPtr component, Attribute_t array[], int nb_elem, uint32_t num_mod)
{
   int i;
   AttributePtr attr = NULL;
  
   for (i=0; i < nb_elem; i++)
   {
      attr = create_attribute(i, component, array, num_mod);
      component_add_attribute(component, attr);
      dagutil_verbose_level(4,"Added attribute %s to component %s(%p)\n", array[i].mName, component_get_name(component), component);
   }
}
