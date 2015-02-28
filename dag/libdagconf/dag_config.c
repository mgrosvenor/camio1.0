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
#include "dag_config.h"
#include "dag_attribute_codes.h"

/* Endace headers. */
#include "dagapi.h"
#include "dagswid.h"
#include "dagtoken.h"
#include "dag_romutil.h"
#include "dagname.h"
#include "dagutil.h"
#include "dag_component.h"

/* Internal project headers. */
#include "include/card.h"
#include "include/util/utility.h"
#include "include/attribute.h"
#include "include/component.h"
#include "include/components/counters_interface_component.h"
#include "include/attribute_types.h"
#include "include/util/logger.h"
#include "dagcam/idt75k_lib.h"
#include "include/util/enum_string_table.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024
/* Public API routines. */
int
dag_card_count(void)
{
    return 0; /* FIXME: unimplemented routine. */
}


const char*
dag_config_strerror(dag_err_t error_code)
{
    switch (error_code)
    {
        case kDagErrNone: return "no error";
        case kDagErrInvalidCardRef: return "invalid card";
        case kDagErrInvalidParameter: return "invalid parameter";
        case kDagErrNoSuchComponent: return "no such component";
        case kDagErrNoSuchAttribute: return "no such attribute";
        case kDagErrFirmwareVerifyFailed: return "firmware verify failed";
        case kDagErrUnimplemented: return "operation not implemented";
        case kDagErrCardNotSupported: return "card not supported";
        case kDagErrGeneral: return "general error";

        default:
            assert(0);
            return "unknown error";
    }
    
    assert(0);
    return "unknown error";
}


dag_err_t
dag_config_get_last_error(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        dag_err_t code = card_get_last_error(card);
		return code;
    }
    
    return kDagErrInvalidCardRef;
}

dag_err_t
dag_config_set_last_error(dag_card_ref_t card_ref, dag_err_t code)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
		card_set_last_error(card, code);
    }
    
    return kDagErrInvalidCardRef;
}
const char*
dag_config_get_card_type_as_string(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;

    if (1 == valid_card(card))
    {
        return card_get_card_type_as_string(card);
    }
    return "unknown";
}

dag_card_t
dag_config_get_card_type(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;

    if (1 == valid_card(card))
    {
        return card_get_card_type(card);
    }
    return kDagUnknown;
}

dag_card_ref_t
dag_config_init(const char* device_name)
{
    DagCardPtr result;
    daginf_t* card_info;
    dag_reg_t* card_registers;
    dag_card_t card_type;
    volatile uint8_t* card_iom = NULL;
    int dagfd = 0;
	int dagstream = 0;
	char dagname[DAGNAME_BUFSIZE] = "dag0";

    assert(NULL != device_name);
    
	if (-1 == dag_parse_name(device_name, dagname, DAGNAME_BUFSIZE, &dagstream))
	{
		dagutil_panic("dag_parse_name(%s): %s\n", device_name, strerror(errno));
	}
	dagutil_verbose_level(2, "%s\n", dagname);

    dagfd = dag_open((char*) dagname);
    if (dagfd < 0)
    {
        /* Set error code. */
        return NULL;
    }
    
    /* Determine card type. */
    /* maps the drb space into the user 
    and get a pointer to DRB register space into card_iom variable */
    card_iom = dag_iom(dagfd);
    if (NULL == card_iom)
    {
        goto fail_after_open;
    }

    /* gets the basic card_info structure stored into the driver 
    * which stores the physical mem address of the mem hole, the total size of the memory hole
    * the size of the DRB space which usually is 64Kbytes
    * PCI Device code 
    * Since 3.1.0 (after RC5) release the board revision 
    * the BUS ids where the card is plugged 
    */
    card_info = dag_info(dagfd);
    if (NULL == card_info)
    {
        goto fail_after_open;
    }

    card_registers = dag_regs(dagfd);
    if (NULL == card_registers)
    {
        goto fail_after_open;
    }

    card_type = pci_device_to_cardtype(card_info->device_code);
    
    /* Create Card object. */
    result = card_init(card_type);
    if (NULL == result)
    {
        goto fail_after_open;
    }
    
    card_set_card_type(result, card_type);
    /* Set up access to registers. */
    card_set_iom_address(result, card_iom);
    card_set_info(result, card_info);
    card_set_registers(result, card_registers);
    card_set_fd(result, dagfd);
    card_post_initialize(result);

    card_set_rx_stream_count(result, dag_rx_get_stream_count(dagfd));
    card_set_tx_stream_count(result, dag_tx_get_stream_count(dagfd));
    
    return (dag_card_ref_t) result;
    
fail_after_open:
    (void) dag_close(dagfd);
    
    return (dag_card_ref_t) NULL;
}


dag_card_ref_t
dag_config_init_offline(dag_card_t card_type)
{
    DagCardPtr result;

    assert(kFirstDagCard <= card_type);
    assert(card_type <= kLastDagCard);
    
    /* Create Card object. */
    result = card_init(card_type);
    card_post_initialize(result);
    
    return (dag_card_ref_t) result;
}


void
dag_config_dispose(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        dag_close(card_get_fd(card));
        card_dispose(card);
    }
}


dag_err_t
dag_config_reset(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 1 - Output Port Y9*/
    {
        card_reset(card);
	return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_default(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        card_default(card);
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}




int
dag_config_line_count(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return 0;
}


dag_err_t
dag_config_get_indexed_line(dag_card_ref_t card_ref, int line_num, char* buffer, int buflen)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(NULL != buffer);
    assert(0 != buflen);

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return kDagErrUnimplemented;
}

int
dag_config_component_count(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        ComponentPtr root = card_get_root_component(card);
        return component_get_subcomponent_count(root);
    }

    return 0;

}

int
dag_config_attribute_count(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        ComponentPtr root = card_get_root_component(card);
        ComponentPtr component = 0;
        int component_count = component_get_subcomponent_count(root);
        int i = 0;
        int attribute_count = 0;
        for (i = 0; i < component_count; ++i)
        {
            component = component_get_indexed_subcomponent(root, i);
            attribute_count += component_get_attribute_count(component);
        }
        return attribute_count;
    }

    return 0;
}


int
dag_config_get_component_count(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;

    if (1 == valid_card(card))
    {
        ComponentPtr root = card_get_root_component(card);
        return component_get_subcomponent_count(root);
    }
    return 0;
}

int dag_config_get_attribute_count(dag_card_ref_t card_ref, dag_attribute_code_t attr_code)
{
    int comp_count, comp_index, attr_count, attr_index, subcomp_count, subcomp_index;
    int total = 0;
    dag_component_t root, comp, subcomp = NULL;
    attr_uuid_t attr_uuid;

    /* sanity checks */
    if (!valid_card((DagCardPtr)card_ref) || 
        !(root = dag_config_get_root_component(card_ref)))
    {
        return kNullAttributeUuid;
    }

    comp_count = dag_component_get_subcomponent_count(root);
    /* go through all components of the card */
    for (comp_index = 0; comp_index < comp_count; comp_index++)
    {
        comp = dag_component_get_indexed_subcomponent(root, comp_index);

        /* go through all the subcomponents first */
        subcomp_count = dag_component_get_subcomponent_count(comp);
        for(subcomp_index = 0; subcomp_index < subcomp_count; subcomp_index++)
        {
            subcomp = dag_component_get_indexed_subcomponent(comp, subcomp_index);
            /* go through all the attributes of this component */
            attr_count = dag_component_get_attribute_count(subcomp);
            for(attr_index = 0; attr_index < attr_count; attr_index++)
            {
                attr_uuid = dag_component_get_indexed_attribute_uuid(subcomp, attr_index);
                /* see if attributes code matches */
                if((attr_uuid != kNullAttributeUuid)&&
                        (dag_config_get_attribute_code(attr_uuid) == attr_code))
                {
                    total++;
                }
            }

        }

        /* go through all attributes of the component */
        attr_count = dag_component_get_attribute_count(comp);
        for (attr_index = 0; attr_index < attr_count; attr_index++)
        {
            attr_uuid = dag_component_get_indexed_attribute_uuid(comp, attr_index);
            /* see if attribute's code matches */
            if (kNullAttributeUuid != attr_uuid &&
                attr_code == dag_config_get_attribute_code(attr_uuid))
            {
                total++;
            }
        }
    }

    return total;
}

attr_uuid_t dag_config_get_indexed_attribute_uuid(dag_card_ref_t card_ref, dag_attribute_code_t attr_code, int a_index)
{
    int comp_count, comp_index, attr_count, attr_index, subcomp_count, subcomp_index;
    int total = 0;
    dag_component_t root, comp, subcomp = NULL;
    attr_uuid_t attr_uuid;

    /* sanity checks */
    if (!valid_card((DagCardPtr)card_ref) || 
        !(root = dag_config_get_root_component(card_ref)))
    {
        return kNullAttributeUuid;
    }

    comp_count = dag_component_get_subcomponent_count(root);
    /* go through all components of the card */
    for (comp_index = 0; comp_index < comp_count; comp_index++)
    {
        comp = dag_component_get_indexed_subcomponent(root, comp_index);
        /* go through the subcomponents first */
        subcomp_count = dag_component_get_subcomponent_count(comp);
        for(subcomp_index = 0; subcomp_index < subcomp_count; subcomp_index++)
        {
            subcomp = dag_component_get_indexed_subcomponent(comp, subcomp_index);
            /* go through all the attributes of this component */
            attr_count = dag_component_get_attribute_count(subcomp);
            for(attr_index = 0; attr_index < attr_count; attr_index++)
            {
                attr_uuid = dag_component_get_indexed_attribute_uuid(subcomp, attr_index);
                /* see if attributes code matches */
                if((attr_uuid != kNullAttributeUuid) && (dag_config_get_attribute_code(attr_uuid) == attr_code))
                {
                    if(total == a_index) {
                        return attr_uuid;
                    }
                    total++;
                }
            }
        }
        /* if it's not part of hte sub comps check this comp */

        attr_count = dag_component_get_attribute_count(comp);
        /* go through all attributes of the component */
        for (attr_index = 0; attr_index < attr_count; attr_index++)
        {
            attr_uuid = dag_component_get_indexed_attribute_uuid(comp, attr_index);
            /* see if attribute's code matches */
            if (kNullAttributeUuid != attr_uuid &&
                attr_code == dag_config_get_attribute_code(attr_uuid))
            {
                if (total == a_index) {
                    return attr_uuid;
                }
                total++;
            }
        }
    }
	return kNullAttributeUuid; 
}


/** Note this functions is not implemented please use
 * dag_config_get_named_attribute_count and 
 * dag_config_get_indexed_named_attribute_uuid
 * instead of this one
 **/
attr_uuid_t
dag_config_get_named_attribute_uuid(dag_card_ref_t card_ref, const char* name)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(NULL != name);

    if (1 == valid_card(card))
    {
    }

    return kNullAttributeUuid;
}

int dag_config_get_named_attribute_count(dag_card_ref_t card_ref, const char * name)
{
    int comp_count, comp_index, attr_count, attr_index, subcomp_count, subcomp_index;
    int total = 0;
    dag_component_t root, comp, subcomp = NULL;
    attr_uuid_t attr_uuid;

    /* sanity checks */
    if (!valid_card((DagCardPtr)card_ref) || 
        !(root = dag_config_get_root_component(card_ref)))
    {
        return kNullAttributeUuid;
    }

    comp_count = dag_component_get_subcomponent_count(root);
    /* go through all components of the card */
    for (comp_index = 0; comp_index < comp_count; comp_index++)
    {
        comp = dag_component_get_indexed_subcomponent(root, comp_index);

        /* go through all the subcomponents first */
        subcomp_count = dag_component_get_subcomponent_count(comp);
        for(subcomp_index = 0; subcomp_index < subcomp_count; subcomp_index++)
        {
            subcomp = dag_component_get_indexed_subcomponent(comp, subcomp_index);
            /* go through all the attributes of this component */
            attr_count = dag_component_get_attribute_count(subcomp);
            for(attr_index = 0; attr_index < attr_count; attr_index++)
            {
                attr_uuid = dag_component_get_indexed_attribute_uuid(subcomp, attr_index);
                /* see if attributes code matches */
                if((attr_uuid != kNullAttributeUuid)&&
                        (strcmp(dag_config_get_attribute_name(attr_uuid), name) == 0))
                {
                    total++;
                }
            }

        }

        /* go through all attributes of the component */
        attr_count = dag_component_get_attribute_count(comp);
        for (attr_index = 0; attr_index < attr_count; attr_index++)
        {
            attr_uuid = dag_component_get_indexed_attribute_uuid(comp, attr_index);
            /* see if attribute's code matches */
            if (kNullAttributeUuid != attr_uuid &&
                        (strcmp(dag_config_get_attribute_name(attr_uuid), name) == 0))
            {
                total++;
            }
        }
    }

    return total;
}

attr_uuid_t dag_config_get_indexed_named_attribute_uuid(dag_card_ref_t card_ref, const char * name, int a_index)
{
    int comp_count, comp_index, attr_count, attr_index, subcomp_count, subcomp_index;
    int total = 0;
    dag_component_t root, comp, subcomp = NULL;
    attr_uuid_t attr_uuid;

    /* sanity checks */
    if (!valid_card((DagCardPtr)card_ref) || 
        !(root = dag_config_get_root_component(card_ref)))
    {
        return kNullAttributeUuid;
    }

    comp_count = dag_component_get_subcomponent_count(root);
    /* go through all components of the card */
    for (comp_index = 0; comp_index < comp_count; comp_index++)
    {
        comp = dag_component_get_indexed_subcomponent(root, comp_index);
        /* go through the subcomponents first */
        subcomp_count = dag_component_get_subcomponent_count(comp);
        for(subcomp_index = 0; subcomp_index < subcomp_count; subcomp_index++)
        {
            subcomp = dag_component_get_indexed_subcomponent(comp, subcomp_index);
            /* go through all the attributes of this component */
            attr_count = dag_component_get_attribute_count(subcomp);
            for(attr_index = 0; attr_index < attr_count; attr_index++)
            {
                attr_uuid = dag_component_get_indexed_attribute_uuid(subcomp, attr_index);
                /* see if attributes code matches */
                if((attr_uuid != kNullAttributeUuid) && (strcmp(dag_config_get_attribute_name(attr_uuid), name)== 0))
                {
                    if(total == a_index) {
                        return attr_uuid;
                    }
                    total++;
                }
            }
        }
        /* if it's not part of hte sub comps check this comp */

        attr_count = dag_component_get_attribute_count(comp);
        /* go through all attributes of the component */
        for (attr_index = 0; attr_index < attr_count; attr_index++)
        {
            attr_uuid = dag_component_get_indexed_attribute_uuid(comp, attr_index);
            /* see if attribute's code matches */
            if (kNullAttributeUuid != attr_uuid &&
                    (strcmp(dag_config_get_attribute_name(attr_uuid), name) ==0))
            {
                if (total == a_index) {
                    return attr_uuid;
                }
                total++;
            }
        }
    }
	return kNullAttributeUuid; 
}


const char*
dag_config_get_attribute_name(attr_uuid_t uuid)
{
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_attribute(attribute))
    {
        return attribute_get_name(attribute);
    }

    return "no name";
}

/** 
 * This function is used to return the attribute type 
 * like status attribute , config attribute 
 */

const char*
dag_config_get_attribute_type_to_string(attr_uuid_t uuid)
{
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_attribute(attribute))
    {
        return attribute_get_type_to_string(attribute);
    }

    return "incorrect config type";
}

/** 
 * This function is used to return the attribute value type 
 * like int , ... it returns config and status api enum constant
 */

const char*
dag_config_get_attribute_valuetype_to_string(attr_uuid_t uuid)
{
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_attribute(attribute))
    {
        return attribute_get_valuetype_to_string(attribute);
    }

    return "incorrect value type";
}


const char*
dag_config_get_attribute_description(attr_uuid_t uuid)
{
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_attribute(attribute))
    {
        return attribute_get_description(attribute);
    }

    return "no description";
}

dag_component_code_t
dag_config_get_component_code(dag_component_t component)
{
	ComponentPtr comp = (ComponentPtr)component;
    if (1 == valid_component(comp))
    {
        return component_get_component_code(comp);
    }
    return kComponentInvalid;
	
}

const char*
dag_config_get_component_code_as_string(dag_component_t component)
{
	ComponentPtr comp = (ComponentPtr)component;
    if (1 == valid_component(comp))
    {
        return dag_component_code_to_string(component_get_component_code(comp));
    }
    return "kComponentInvalid";
	
}

const char*
dag_config_get_attribute_code_as_string(attr_uuid_t attr_uuid)
{
//    assert(kNullAttributeUuid != attr_uuid);
    if(attr_uuid != kNullAttributeUuid )
    {
    	return dag_attribute_code_to_string(dag_config_get_attribute_code(attr_uuid));
    }
    
   // this will happended only if the attribute is null
   return "Null Attribute";
	
}


const char*
dag_config_get_component_name(dag_component_t ref)
{
    ComponentPtr component = (ComponentPtr)ref;

    if (1 == valid_component(component))
    {
        return component_get_name(component);
    }

    return "no name";
}


const char*
dag_config_get_component_description(dag_component_t ref)
{
    ComponentPtr component = (ComponentPtr)ref;

    if (1 == valid_component(component))
    {
        return component_get_description(component);
    }

    return "no description";
}

dag_attr_config_status_t
dag_config_get_attribute_config_status(attr_uuid_t uuid)
{
    AttributePtr attribute = (AttributePtr)uuid;

    return attribute_get_config_status(attribute);
}

unsigned int
dag_config_is_attribute_writable(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return 0;
}


dag_attr_t
dag_config_get_attribute_valuetype(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        return attribute_get_valuetype(attribute);
    }

    return kAttributeTypeInvalid;
}

uint8_t
dag_config_get_boolean_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    uint8_t* p = NULL;

    assert(kNullAttributeUuid != uuid);
    if (1 == valid_card(card))
    {
        p = (uint8_t*)attribute_get_value(attribute);
        if (p) return *p;
    }
    return 0;
}


char
dag_config_get_char_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        return *(char*)attribute_get_value(attribute);
    }
    return '-';
}


const char*
dag_config_get_string_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        return attribute_get_value(attribute);
    }
    return "no attribute";
}


int32_t
dag_config_get_int32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    int32_t* p = NULL;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        p = (int32_t*)attribute_get_value(attribute);
        if (p) return *p;
    }
    return 0;
}


uint32_t
dag_config_get_uint32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    uint32_t* p = NULL;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        p = (uint32_t*)attribute_get_value(attribute);
        if (p) return *p;
    }

    return 0;
}


int64_t
dag_config_get_int64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    int64_t* p = NULL;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        p = (int64_t*)attribute_get_value(attribute);
        if (p) return *p;
    }

    return (int64_t) 0;
}


uint64_t
dag_config_get_uint64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    uint64_t* p = NULL;
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        p = (uint64_t*)attribute_get_value(attribute);
        if (p) return *p;
    }

    return (uint64_t) 0;
}


float
dag_config_get_float_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    float* p = NULL;
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        p = (float*)attribute_get_value(attribute);
        if (p) return *p;
    }

    return (float) 0;
}


dag_err_t
dag_config_set_boolean_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, uint8_t value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeBoolean)
            return kDagErrInvalidParameter;
            
        attribute_set_value(attribute, (void*)&value, sizeof(value));
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

dag_err_t
dag_config_set_char_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, char value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);
    assert('\0' != value);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeChar)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, (void*)&value, sizeof(value));
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_set_string_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, const char* value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);
    assert(NULL != value);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeString)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, (void*)value, sizeof(const char*));
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_set_int32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, int32_t value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeInt32)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, (void*)&value, sizeof(value));
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_set_uint32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, uint32_t value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr) uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeUint32)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, (void*)&value, sizeof(value));
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_set_int64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, int64_t value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeInt64)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, (void*)&value, sizeof(value));
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_set_uint64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, uint64_t value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeUint64)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, (void*)&value, sizeof(value));
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_set_float_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, float value)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeFloat)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, (void*)&value, sizeof(value));
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_config_set_struct_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, void* value)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        if(attribute_get_valuetype(attribute) != kAttributeStruct)
            return kDagErrInvalidParameter;
        
        attribute_set_value(attribute, value, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

dag_err_t
dag_config_set_null_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if(attribute_get_config_status(attribute) != kDagAttrConfig)
            return kDagErrStatusAttribute;

        attribute_set_value(attribute, NULL, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

int
dag_config_get_config_attribute_count(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return 0;
}


attr_uuid_t
dag_config_get_indexed_config_attribute_uuid(dag_card_ref_t card_ref, int attr_index)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return kNullAttributeUuid;
}

 
attr_uuid_t
dag_config_get_named_config_attribute_uuid(dag_card_ref_t card_ref, const char* name)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(NULL != name);

    if (1 == valid_card(card))
    {
    }

    return kNullAttributeUuid;
}

const char*
dag_config_get_config_attribute_name(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if (kDagAttrConfig == attribute_get_config_status(attribute))
        {
            return attribute_get_name(attribute);
        }
    }

    return "no name";
}


const char*
dag_config_get_config_attribute_description(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if (kDagAttrConfig == attribute_get_config_status(attribute))
        {
            return attribute_get_description(attribute);
        }
    }

    return "no description";
}

dag_err_t
dag_status_header_line(dag_card_ref_t card_ref, char* buffer, int buflen)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(NULL != buffer);
    assert(0 != buflen);

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return kDagErrUnimplemented;
}


dag_err_t
dag_status_data_line(dag_card_ref_t card_ref, char* buffer, int buflen)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(NULL != buffer);
    assert(0 != buflen);

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return kDagErrUnimplemented;
}


int
dag_config_get_status_attribute_count(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return 0;
}


attr_uuid_t
dag_config_get_indexed_status_attribute_uuid(dag_card_ref_t card_ref, int attr_index)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return kNullAttributeUuid;
}

 
attr_uuid_t
dag_config_get_named_status_attribute_uuid(dag_card_ref_t card_ref, const char* name)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(NULL != name);

    if (1 == valid_card(card))
    {
    }

    return kNullAttributeUuid;
}

const char*
dag_config_get_status_attribute_name(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if (kDagAttrStatus == attribute_get_config_status(attribute))
        {
            return attribute_get_name(attribute);
        }
    }

    return "no name";
}


const char*
dag_config_get_status_attribute_description(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr) card_ref;
    AttributePtr attribute = (AttributePtr)uuid;

    assert(kNullAttributeUuid != uuid);

    if (1 == valid_card(card))
    {
        if (kDagAttrStatus == attribute_get_config_status(attribute))
        {
            return attribute_get_description(attribute);
        }
    }

    return "no description";
}

/* read the software id (swid) into the buffer */
dag_err_t
dag_firmware_read_swid(dag_card_ref_t card_ref, uint8_t* buffer, int length)
{
    int fd;
    DagCardPtr card = (DagCardPtr)card_ref;
    int error;
    
    if (1 == valid_card(card))
    {
        fd = card_get_fd(card);
        error = dag_read_software_id(fd, length, buffer);
        switch (error)
        {
            case -1:
                return kDagErrSWIDInvalidBytes;

            case -2:
                return kDagErrSWIDError;

            case -3:
                return kDagErrSWIDTimeout;

        }
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

dag_err_t
dag_firmware_write_swid(dag_card_ref_t card_ref, uint8_t* buffer, int length, uint32_t key)
{
    int fd;
    DagCardPtr card = (DagCardPtr)card_ref;
    int error;
    
    if (1 == valid_card(card))
    {
        fd = card_get_fd(card);
        error = dag_write_software_id(fd, length, buffer, key);
        switch (error)
        {
            case -1:
                return kDagErrSWIDInvalidBytes;

            case -2:
                return kDagErrSWIDError;

            case -3:
                return kDagErrSWIDTimeout;

            case -4:
                return kDagErrSWIDInvalidKey;
        }
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

dag_err_t
dag_firmware_load_pci_no_card_ref(const char* name, const char* filename)
{
	dag_card_ref_t card;

    NULL_RETURN_WV(name, kDagErrInvalidParameter);
    NULL_RETURN_WV(filename, kDagErrInvalidParameter);
	card = dag_config_init(name);
	if (1 == valid_card((DagCardPtr)card))
	{
		card_load_firmware((DagCardPtr)card, (uint8_t*)filename);
		dag_config_dispose(card);
		return kDagErrNone;
	}
	return kDagErrInvalidCardRef;
}

dag_err_t
dag_firmware_load_pci(const char* name, dag_card_ref_t* card_ref, const char* filename)
{
    DagCardPtr card;
    NULL_RETURN_WV(name, kDagErrInvalidParameter);
    NULL_RETURN_WV(card_ref, kDagErrInvalidParameter);
    NULL_RETURN_WV(filename, kDagErrInvalidParameter);
    card = (DagCardPtr)*card_ref;

    if (1 == valid_card(card))
    {
        card_load_firmware(card, (uint8_t*)filename);
        dag_config_dispose(*card_ref);
        *card_ref = dag_config_init(name);
        assert(*card_ref != NULL);
        return kDagErrNone;
    }

    return kDagErrInvalidCardRef;
}

dag_err_t
dag_firmware_load_embedded(dag_card_ref_t card_ref, const char* filename, dag_embedded_region_t region)
{
    DagCardPtr card;
    NULL_RETURN_WV(card_ref, kDagErrInvalidParameter);
    NULL_RETURN_WV(filename, kDagErrInvalidParameter);
	card = (DagCardPtr) card_ref;
 
    if (1 == valid_card(card))
    {
        return card_load_embedded(card, (uint8_t*)filename, (int)region);
    }

    return kDagErrInvalidCardRef;
}


dag_err_t
dag_firmware_load_pp_no_card_ref(const char* name, const char* filename, int which_pp)
{
    dag_card_ref_t card;
    dag_err_t error = kDagErrNone;
    NULL_RETURN_WV(name, kDagErrInvalidParameter);
    NULL_RETURN_WV(filename, kDagErrInvalidParameter);
    card = dag_config_init(name);

    if (1 == valid_card((DagCardPtr)card))
    {
        error = card_load_pp_image((DagCardPtr)card, filename, which_pp);
		dag_config_dispose(card);
        return error;
    }
    return kDagErrInvalidCardRef;
}

dag_err_t
dag_firmware_load_pp(const char* name, dag_card_ref_t* card_ref, const char* filename, int which_pp)
{
    DagCardPtr card;
    dag_err_t error = kDagErrNone;
    NULL_RETURN_WV(name, kDagErrInvalidParameter);
    NULL_RETURN_WV(card_ref, kDagErrInvalidParameter);
    NULL_RETURN_WV(filename, kDagErrInvalidParameter);
    card = (DagCardPtr)*card_ref;

    if (1 == valid_card(card))
    {
        error = card_load_pp_image(card, filename, which_pp);
        dag_config_dispose(*card_ref);
        *card_ref = dag_config_init(name);
        assert(*card_ref != NULL);
        return error;
    }
    return kDagErrInvalidCardRef;
}


dag_err_t
dag_firmware_load_copro(dag_card_ref_t card_ref, const char* filename, int flags)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(NULL != filename);

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. 
		use load pp function for 3.8 and 7.1s and 4.3 cards 
	    for the new cards use dagrom tool or dagrom_util libary 
	    
	*/
    }

     return kDagErrUnimplemented;
}


dag_firmware_t
dag_firmware_get_active(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    dag_firmware_t ret_val = kFirmwareNone;
    if (1 == valid_card(card))
    {
        romtab_t* rt = NULL;
        int user = 0;
        int firmware = 0;
        rt = dagrom_init(card_get_fd(card), 0, 0);
        NULL_RETURN_WV(rt, kFirmwareNone);
        if(rt->rom_version == 0x2)
        {
            user =	dagrom_reprogram_control_reg(rt);	
            firmware = (user & 0x3);
            /*Check for fallback*/
            if(user & BIT12)
            {
                ret_val =  kFirmwareFactory;
            }
            else 
            {
                switch (firmware)
                {
                    case 0:
                        ret_val =  kFirmwareFactory;
                    break;
                    case 1:
                        ret_val =  kFirmwareUser1;
                    break;
                    case 2:
                        ret_val =  kFirmwareUser2;
                    break;
                    case 3:
                        ret_val =  kFirmwareUser3;
                    break;
                }
            }
        }
        else
        {
            user = ((dagrom_romreg(rt) & BIT30) == BIT30);        
            if(user)
                    ret_val =  kFirmwareUser;
            else
                    ret_val =  kFirmwareFactory;
        }
        dagrom_free(rt);
    }

    return ret_val;
}
int 
dag_firmware_controller_get_version(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    if (1 == valid_card(card))
    {
        romtab_t* rt = NULL;
        rt = dagrom_init(card_get_fd(card), 0, 0);
        NULL_RETURN_WV(rt, kFirmwareNone);
	if(rt->rom_version == 0x2)
	{
		return 2;
	}
	else if(rt->rom_version == 0x1)
	{
		return 1;
	}
	else 
		return 0;
	
    }
    return kDagErrInvalidCardRef;
}
dag_err_t
dag_firmware_make_pci_active(dag_card_ref_t card_ref, dag_firmware_t firmware_type)
{
    DagCardPtr card = (DagCardPtr) card_ref;

    assert(kFirstFirmware <= firmware_type);
    assert(firmware_type <= kLastFirmware);

    if (1 == valid_card(card))
    {
        /* FIXME: unimplemented routine. */
    }

    return kDagErrUnimplemented;
}

dag_attribute_code_t
dag_config_get_attribute_code(attr_uuid_t attr_uuid)
{
    AttributePtr attr = (AttributePtr)attr_uuid;
    if (1 == valid_attribute(attr))
    {
        return attribute_get_attribute_code(attr);
    }
    return kAttributeInvalid;
}


const char*
dag_config_get_attribute_to_string(dag_card_ref_t card_ref, attr_uuid_t uuid)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    if (1 == valid_card(card))
    {
        return attribute_get_to_string(attribute);
    }
    return "Invalid";
}

void
dag_config_set_attribute_from_string(dag_card_ref_t card_ref, attr_uuid_t uuid, const char* string)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    if (1 == valid_card(card))
    {
        attribute_set_from_string(attribute, string);
    }
}

dag_component_t
dag_config_get_root_component(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
	assert(NULL != card_ref);
    if (card_ref)
    {
        return (dag_component_t)card_get_root_component(card);
    }
    else
    {
        return NULL;
    }
}

int
dag_config_get_card_fd(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    if (card_ref)
    {
        return card_get_fd(card);
    }
    return 0;
}

dag_err_t
dag_firmware_read_serial_id(dag_card_ref_t card_ref, int* serial)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    if (1 == valid_card(card))
    {
        romtab_t* rt = NULL;
        uint8_t buf[128];
	uint16_t value_array[8];
	uint8_t* bp;
        uint32_t addr = 0;
	uint32_t size = 0;
	uint32_t index = 0;
	uint32_t sign = 0;

	*serial = 0;


        rt = dagrom_init(card_get_fd(card), 0, 0);
        NULL_RETURN_WV(rt, kDagErrGeneral);
        /* Read the rom into the buffer */
	if(rt->rom_version == 0x2)
	{
		size = (((rt->rblock.sectors[0])*(rt->rblock.size_region[0])) + ((rt->rblock.sectors[1])*(rt->rblock.size_region[1])));
		for(addr = (size - 128),index = 0; addr < (size - 128 + 8);addr+=2,index++)
		{
			value_array[index] = rt->romread(rt,addr);
		}
		sign = ((value_array[1] << 16) | (value_array[0]));	
		if(sign == 0x12345678)
		{
			*serial = ((value_array[3] << 16) | (value_array[2]));
		}
	}
	else
	{	
        	bp = buf;
        	for (addr = rt->tstart; addr < (rt->tstart + sizeof(buf)); addr++)
        	{
            		*bp = rt->romread(rt, addr);
            		bp++;
        	}
        	/* If serial number is present*/
        	if (*(int *) buf == 0x12345678)
        	{
            		*serial = (*(int *) (buf + 4));
        	}
	}
        dagrom_free(rt);
        rt = NULL;

        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}
char *dag_firmware_read_user_firmware_name(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    if (1 == valid_card(card))
    {
        romtab_t* rt = NULL;
        uint8_t buf[128];
	    uint8_t* bp = buf;
        uint32_t addr = 0;
	uint16_t value = 0;

        rt = dagrom_init(card_get_fd(card), 0, 0);
        NULL_RETURN_WV(rt, NULL);
        /* Read the rom into the buffer */
        bp = buf;
       	if(rt->rom_version == 0x2)
	{
		for (addr = (rt->itable.atable[1].start_address + IMAGE_SIGN_FIELD_WIDTH); addr < (rt->itable.atable[1].start_address + sizeof(buf) + IMAGE_SIGN_FIELD_WIDTH); addr+=2)
		{	
			value = rt->romread(rt, addr);
			*(bp + 1) = (value & 0xff00) >> 8;
			*bp =  (value & 0xff);
			bp++;
			bp++;
		}
	} else {
 
		for (addr = rt->bstart; addr < (rt->bstart + sizeof(buf)); addr++)
        	{
			*bp = rt->romread(rt, addr);
			bp++;
        	}
	};
        bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
        
        dagrom_free(rt);
        rt = NULL;

        return (char *)bp;
    }
    return NULL;
    
}
char *dag_firmware_read_current_firmware_name(dag_card_ref_t card_ref)
{
    return dag_firmware_read_user_firmware_name(card_ref);
}

char *dag_firmware_read_factory_firmware_name(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    if (1 == valid_card(card))
    {
        romtab_t* rt = NULL;
        uint8_t buf[128];
	uint8_t* bp = buf;
        uint32_t addr = 0;
	uint16_t value = 0;
        rt = dagrom_init(card_get_fd(card), 0, 0);
        NULL_RETURN_WV(rt, NULL);
        /* Read the rom into the buffer */
        bp = buf;
	if(rt->rom_version == 0x2)
	{
		for (addr = (rt->itable.atable[0].start_address + IMAGE_SIGN_FIELD_WIDTH); addr < (rt->itable.atable[0].start_address + sizeof(buf) + IMAGE_SIGN_FIELD_WIDTH); addr+=2)
		{	
			value = rt->romread(rt, addr);
			*(bp + 1) = (value & 0xff00) >> 8;
			*bp =  (value & 0xff);
			bp++;
			bp++;
		}
	}
	else
	{
        	for (addr = rt->tstart; addr < (rt->tstart + sizeof(buf)); addr++)
        	{
           		*bp = rt->romread(rt, addr);
            		bp++;
		}
        	if(*(int *)buf == 0x12345678)
        	{
            		/* skip over the serial number */
            		bp = buf;
            		for (addr = rt->tstart + DAGSERIAL_SIZE; addr < (rt->tstart + DAGSERIAL_SIZE + sizeof(buf)); addr++)
            		{
                		*bp = rt->romread(rt, addr);
                		bp++;
            		}
        	}
        }
        bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
        
        dagrom_free(rt);
        rt = NULL;

        return (char *)bp;
    }
    return NULL;
    
}
char *
dag_firmware_read_new_user_firmware_name(dag_card_ref_t card_ref,int number)
{
	DagCardPtr card = (DagCardPtr)card_ref;
	if (1 == valid_card(card))
	{
		romtab_t* rp = NULL;
        	uint8_t buf[128];
	   	uint32_t addr = 0;
		uint8_t* bp = buf;
		uint16_t value = 0;
		
        	rp = dagrom_init(card_get_fd(card), 0, 0);
        	NULL_RETURN_WV(rp, NULL);
        	
		for (addr = (rp->itable.atable[number].start_address + IMAGE_SIGN_FIELD_WIDTH); addr < (rp->itable.atable[number].start_address + sizeof(buf) + IMAGE_SIGN_FIELD_WIDTH); addr+=2)
		{	
			value = rp->romread(rp, addr);
			*(bp + 1) = (value & 0xff00) >> 8;
			*bp =  (value & 0xff);
			bp++;
			bp++;
		}
		bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
		return (char*)bp;
	}
	return NULL;
}
char *dag_copro_read_factory_firmware_name(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    if (1 == valid_card(card))
    {
        romtab_t* rt = NULL;
        uint8_t buf[128];
	    uint8_t* bp = buf;
        uint32_t addr = 0;

        rt = dagrom_init(card_get_fd(card), 0, 0);
        NULL_RETURN_WV(rt, NULL);
        if ((rt->mpu_id != DAG_MPU_COPRO_SMALL) && (rt->mpu_id != DAG_MPU_COPRO_BIG))
        {
            dagrom_free(rt);
            return NULL;
        }
        /* Read the rom into the buffer */
        bp = buf;
	   for (addr = COPRO_STABLE_START_BIG; addr < (COPRO_STABLE_START_BIG + sizeof(buf)); addr++)
	   {
		  *bp = rt->romread(rt, addr);
		  bp++;
	   }

	   /* If serial number is present, skip over */
        if (*(int *) buf == 0x12345678)
	   {
	   /* Not sure - Never reach here? */
	    /*int serial = (*(int *) (buf + 4));*/
		  bp = buf;
		  for (addr = rt->tstart + DAGSERIAL_SIZE; addr < (rt->tstart + DAGSERIAL_SIZE + sizeof(buf)); addr++)
		  {
			*bp = rt->romread(rt, addr);
			bp++;
		  }
	   }
	
	   bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
       dagrom_free(rt);
       rt = NULL;

       return (char *)bp;
    }
    return NULL;
}

char *dag_copro_read_user_firmware_name(dag_card_ref_t card_ref)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    if (1 == valid_card(card))
    {
        romtab_t* rt = NULL;
        uint8_t buf[128];
	    uint8_t* bp = buf;
        uint32_t addr = 0;

        rt = dagrom_init(card_get_fd(card), 0, 0);
        NULL_RETURN_WV(rt, NULL);
        if ((rt->mpu_id != DAG_MPU_COPRO_SMALL) && (rt->mpu_id != DAG_MPU_COPRO_BIG))
        {
            dagrom_free(rt);
            return NULL;
        }
        /* Read the rom into the buffer */
        bp = buf;
        for (addr = COPRO_CURRENT_START_BIG; addr < (COPRO_CURRENT_START_BIG + sizeof(buf)); addr++)
	   {
		*bp = rt->romread(rt, addr);
		bp++;
	   }
	
	   bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
       dagrom_free(rt);
       rt = NULL;
       return (char *)bp;
	}
    return NULL;
}

char *dag_firmware_read_stable_firmware_name(dag_card_ref_t card_ref)
{
    return dag_firmware_read_factory_firmware_name(card_ref);
}

dag_err_t
dag_config_get_last_sync_time(dag_card_ref_t card_ref, struct tm* time_param)
{
    duckinf_t duckinf;
    int dagfd = dag_config_get_card_fd(card_ref);
#if defined(_WIN32)
	DWORD BytesTransfered = 0;
#else
    uint32_t error = 0;
#endif


#if defined (_WIN32)
	if(DeviceIoControl(dag_gethandle(dagfd),
		IOCTL_GET_DUCKINFO,
		&duckinf,
		sizeof(duckinf_t),
		&duckinf,
		sizeof(duckinf_t),
		&BytesTransfered,
		NULL) == FALSE)
            return kDagErrGeneral;

#else
	if((error = ioctl(dagfd, DAGIOCDUCK, &duckinf)))
		return kDagErrGeneral;
#endif
    /* copy the members of the struct*/
    *time_param = *localtime(&duckinf.Stat_Start);
    return kDagErrNone;
}

dag_err_t
dag_config_sync_to_host(dag_card_ref_t card_ref)
{
    int dagfd = dag_config_get_card_fd(card_ref);
#if defined(_WIN32)
	DWORD BytesTransfered = 0;

    if(DeviceIoControl(dag_gethandle(dagfd),
               IOCTL_DUCK_RESET,
               NULL,
               0,
               NULL,
               0,
               &BytesTransfered,
               NULL) == FALSE)
        return kDagErrGeneral;


#else
    int magic = DAGRESET_DUCK;
    if(ioctl(dagfd, DAGIOCRESET, &magic) < 0)
        return kDagErrGeneral;
#endif /* Platform-specific code. */
    return kDagErrNone;
}

dag_err_t
dag_config_get_struct_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, void** value)
{
    DagCardPtr card = (DagCardPtr)card_ref;
    AttributePtr attribute = (AttributePtr)uuid;
    if (1 == valid_card(card))
    {
        *value = (void*)attribute_get_value(attribute);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

/* Get number of CSI block(s) */
uint32_t 
dag_config_get_number_block(dag_card_ref_t card_ref)
{
	ComponentPtr root = NULL;	    		
	
	DagCardPtr card = (DagCardPtr)card_ref;
	
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		return(component_get_subcomponent_count_of_type(root, kComponentInterface));
	}
	return 0;
}

/* Get number of counters for a particular block */
uint32_t 
dag_config_get_number_counters(dag_card_ref_t card_ref, dag_block_type_t block_type)
{
	AttributePtr attr = NULL;
	ComponentPtr comp = NULL;
	ComponentPtr root = NULL;
	dag_block_type_t v_type;
	uint32_t count_block = 0;
    int j;
	uint32_t nb_counters = 0;
	
    DagCardPtr card = (DagCardPtr)card_ref;
	
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* get the component and the value of the type */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(comp, kUint32AttributeCSIType);
			v_type = *((dag_block_type_t*)attribute_get_value(attr));
			
			/* if type corresponds to the block desired */
			if (v_type == block_type)
			{
				attr = component_get_attribute(comp, kUint32AttributeNbCounters);
				nb_counters = *((uint32_t*)attribute_get_value(attr));
			}
		}
	}
	return (nb_counters);
}

/* Get number of counters on the card */
uint32_t 
dag_config_get_number_all_counters(dag_card_ref_t card_ref)
{
	AttributePtr attr = NULL;
	ComponentPtr comp = NULL;
	ComponentPtr root = NULL;
	int count_block = 0;
    int j;
	uint32_t nb_counters = 0;
	
    DagCardPtr card = (DagCardPtr)card_ref;
	
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* get the component and the value of the type */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(comp, kUint32AttributeNbCounters);
			nb_counters += *((uint32_t*)attribute_get_value(attr));
		}
	}
	return (nb_counters);
}



/* Get the all counters id for a particular block*/
uint32_t 
dag_config_get_counter_id_subfct(dag_card_ref_t card_ref, dag_block_type_t block_type, dag_counter_value_t counter_id[], uint32_t size)
{
	AttributePtr attr = NULL;
	ComponentPtr block = NULL;
	ComponentPtr counter = NULL;
	ComponentPtr root = NULL;
	dag_block_type_t v_type;
	int count_block = 0;
    int j, i;
	uint32_t nb_counters = 0;
	
    DagCardPtr card = (DagCardPtr)card_ref;
	
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* get the component and the value of the type */
			block = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(block, kUint32AttributeCSIType);
			v_type = *((dag_block_type_t*)attribute_get_value(attr));
			
			/* if type corresponds to the block desired */
			if (v_type == block_type)
			{
				attr = component_get_attribute(block, kUint32AttributeNbCounters);
				nb_counters = *((uint32_t*)attribute_get_value(attr));
				
				for (i = 0; i< nb_counters; i++)
				{
					counter = component_get_subcomponent(block, kComponentCounter, i);
					attr = component_get_attribute(counter, kUint32AttributeCounterID);
					counter_id[i].typeID = *((dag_counter_type_t*)attribute_get_value(attr));
					attr = component_get_attribute(counter, kUint32AttributeSubFunctionType);
					counter_id[i].subfct = *((dag_subfct_type_t*)attribute_get_value(attr));
					attr = component_get_attribute(counter,kUint32AttributeSubFunction);
				}
			}
		}
	}
	return (nb_counters);
}

/* Get the all block ids on the card */
uint32_t
dag_config_get_all_block_id(dag_card_ref_t card_ref, uint32_t block_id[], uint32_t size)
{
	ComponentPtr comp = NULL;
	ComponentPtr root = NULL;
	AttributePtr attr = NULL;
	uint32_t count_block = 0;
    int j;
		
	DagCardPtr card = (DagCardPtr)card_ref;
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{			
			/* save block ID */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(comp, kUint32AttributeCSIType);
			block_id[j] = *((uint32_t*)attribute_get_value(attr));
		}
	}
	return (count_block);
}
	
/* Latch and Clear all counters (<=> Latch and clear all blocks) */
void
dag_config_latch_clear_all(dag_card_ref_t card_ref)
{
	AttributePtr attr = NULL;
	ComponentPtr comp = NULL;
	ComponentPtr root = NULL;
	int count_block = 0;
	int attempts = 0;
    int j;
	int val;
	uint8_t value;
	
    DagCardPtr card = (DagCardPtr)card_ref;
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* set up the latch and clear */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(comp, kBooleanAttributeLatchClear);
			val = 1;
			attribute_set_value(attr, (void*)&val, 1);
			/*FW latch bit description says "When '1' is written to this bit all the latch and clear counters in the CSI block will be latched and cleared.This bit will remain one till latch and clear is complete.Currently no bugs are manifested as the FW obviously latches and clears fast enough.Might be an issue if emulated via SOFTDAG."*/
			value = *(uint8_t*)attribute_get_value(attr);
			while (((value != 0) && (attempts <  20)))
			{
				value = *(uint8_t*)attribute_get_value(attr);
				attempts++;
			}
		}
	}
}
/* Latch and Clear a CSI block */
void
dag_config_latch_clear_block(dag_card_ref_t card_ref, dag_block_type_t block_type)
{
	dag_block_type_t v_type = 0;
	AttributePtr attr = NULL;
	ComponentPtr comp = NULL;
	ComponentPtr root = NULL;
	int count_block = 0;
    int j;
	int val = 0;
	uint8_t value;
	int attempts = 0;
	
    DagCardPtr card = (DagCardPtr)card_ref;
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* get the component and the value of the type */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(comp, kUint32AttributeCSIType);
			v_type = *((dag_block_type_t*)attribute_get_value(attr));

			/* if type corresponds to the block desired */
			if (v_type == block_type)
			{
				/* set up the latch and clear */
				attr = component_get_attribute(comp, kBooleanAttributeLatchClear);
				val = 1;
				attribute_set_value(attr, (void*)&val, 1);
				/*FW latch bit description says "When '1' is written to this bit all the latch and clear counters in the CSI block will be latched and cleared.This bit will remain one till latch and clear is complete.Currently no bugs are manifested as the FW obviously latches and clears fast enough.Might be an issue if emulated via SOFTDAG."*/
				value = *(uint8_t*)attribute_get_value(attr);
				while (((value != 0) && (attempts <  20)))
				{
					value = *(uint8_t*)attribute_get_value(attr);
					attempts++;
				}
			}
		}
	}
}
/* Read and return the value of all counters in a block */
uint32_t
dag_config_read_single_block(dag_card_ref_t card_ref, dag_block_type_t block_type, dag_counter_value_t countersTab[], uint32_t size, int lc)
{
	AttributePtr attr = NULL;
	ComponentPtr comp = NULL, c_counter = NULL;
	ComponentPtr root = NULL;
	int count_block = 0;
	uint32_t count_counter = 0;
    int j, i;
	int val = 0;
	uint8_t value;
	int attempts = 0;
	dag_block_type_t v_type;
    DagCardPtr card = (DagCardPtr)card_ref;
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* get the component and the value of the type */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(comp, kUint32AttributeCSIType);
			v_type = *((dag_block_type_t*)attribute_get_value(attr));

			/* if type corresponds to the block desired */
			if (v_type == block_type)
			{
				/* if latch and clear option is set */
				if (lc == 1)
				{
					attr = component_get_attribute(comp, kBooleanAttributeLatchClear);
					val = 1;
					attribute_set_value(attr, (void*)&val, 1);
					/*FW latch bit description says "When '1' is written to this bit all the latch and clear counters in the CSI block will be latched and cleared.This bit will remain one till latch and clear is complete.Currently no bugs are manifested as the FW obviously latches and clears fast enough.Might be an issue if emulated via SOFTDAG."*/
					value = *(uint8_t*)attribute_get_value(attr);
					while (((value != 0) && (attempts <  20)))
					{
						value = *(uint8_t*)attribute_get_value(attr);
						attempts++;
					}
				}
				/* get the number of counters */
				count_counter = component_get_subcomponent_count_of_type(comp, kComponentCounter);	
				
				/* for each counter */
				for (i = 0; i < count_counter; i++)
				{
					/* get the counter description */
					c_counter = component_get_subcomponent(comp, kComponentCounter, i);
					attr = component_get_attribute(c_counter, kUint32AttributeCounterID);
					countersTab[i].typeID = *((dag_counter_type_t*)attribute_get_value(attr));
					attr = component_get_attribute(c_counter, kUint32AttributeCounterSize);
					countersTab[i].size = *((uint32_t*)attribute_get_value(attr));
					attr = component_get_attribute(c_counter, kBooleanAttributeLatchClear);
					countersTab[i].lc = *((uint32_t*)attribute_get_value(attr));
					attr = component_get_attribute(c_counter, kBooleanAttributeValueType);
					countersTab[i].value_type = *((uint32_t*)attribute_get_value(attr));
					attr = component_get_attribute(c_counter, kUint64AttributeValue);
					countersTab[i].value = *((uint64_t*)attribute_get_value(attr));
					attr = component_get_attribute(c_counter, kUint32AttributeSubFunctionType);
					countersTab[i].subfct = *((uint32_t*)attribute_get_value(attr));
					attr = component_get_attribute(c_counter,kUint32AttributeSubFunction);
					countersTab[i].interface_number = *((uint32_t*)attribute_get_value(attr));
				}
			}
		}
	}
	return (count_counter);

}

/* Read and return the value of all counters  */
uint32_t
dag_config_read_all_counters(dag_card_ref_t card_ref, dag_counter_value_t countersTab[], uint32_t size, int lc)
{
	AttributePtr attr = NULL;
	ComponentPtr comp = NULL, c_counter = NULL;
	ComponentPtr root = NULL;
	int count_block = 0;
	int count_counters = 0;
	uint32_t total_counters = 0;
    int j, i;
	int val = 0;
	int attempts = 0;
	uint8_t value = 0;
	dag_block_type_t v_type;
	
    DagCardPtr card = (DagCardPtr)card_ref;
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* get the component and the value of the type */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			attr = component_get_attribute(comp, kUint32AttributeCSIType);
			v_type = *((dag_block_type_t*)attribute_get_value(attr));

			/* if latch and clear option is set */
			if (lc == 1)
			{
				attr = component_get_attribute(comp, kBooleanAttributeLatchClear);
				val = 1;
				attribute_set_value(attr, (void*)&val, 1);
				/*FW latch bit description says "When '1' is written to this bit all the latch and clear counters in the CSI block will be latched and cleared.This bit will remain one till latch and clear is complete.Currently no bugs are manifested as the FW obviously latches and clears fast enough.Might be an issue if emulated via SOFTDAG."*/
				value = *(uint8_t*)attribute_get_value(attr);
				while (((value != 0) && (attempts <  20)))
				{
					value = *(uint8_t*)attribute_get_value(attr);
					attempts++;
				}
			}
			/* get the number of counters */
			count_counters = component_get_subcomponent_count_of_type(comp, kComponentCounter);	
			
			/* for each counter */
			for (i = 0; i < count_counters; i++)
			{
				/* get the counter description */
				c_counter = component_get_subcomponent(comp, kComponentCounter, i);
				attr = component_get_attribute(c_counter, kUint32AttributeCounterID);
				countersTab[total_counters].typeID = *((dag_counter_type_t*)attribute_get_value(attr));
				attr = component_get_attribute(c_counter, kUint32AttributeCounterSize);
				countersTab[total_counters].size = *((uint32_t*)attribute_get_value(attr));
				attr = component_get_attribute(c_counter, kBooleanAttributeLatchClear);
				countersTab[total_counters].lc = *((uint32_t*)attribute_get_value(attr));
				attr = component_get_attribute(c_counter, kBooleanAttributeValueType);
				countersTab[total_counters].value_type = *((uint32_t*)attribute_get_value(attr));
				attr = component_get_attribute(c_counter, kUint64AttributeValue);
				countersTab[total_counters].value = *((uint64_t*)attribute_get_value(attr));
				attr = component_get_attribute(c_counter, kUint32AttributeSubFunctionType);
                countersTab[i].subfct = *((uint32_t*)attribute_get_value(attr));
                attr = component_get_attribute(c_counter,kUint32AttributeSubFunction);
                countersTab[i].interface_number = *((uint32_t*)attribute_get_value(attr));
				total_counters ++;
			}
		
		}
	}
	return (total_counters);
}

/* Read and return the value of a single counters on the card */
uint64_t
dag_config_read_single_counter(dag_card_ref_t card_ref, dag_block_type_t block_type, dag_counter_type_t counter_type, dag_subfct_type_t subfct_type)
{
	dag_block_type_t v_type = 0;
	dag_counter_type_t v_count_id = 0;
	dag_subfct_type_t v_subfct = 0;
	AttributePtr a_type = NULL;
	AttributePtr a_value = NULL;
	AttributePtr attr = NULL;
	ComponentPtr comp = NULL, c_counter = NULL;
	ComponentPtr root = NULL;
	int count_block = 0;
	int count_counter = 0;
    int j, i;
	uint64_t counter_value = 0;
	
    DagCardPtr card = (DagCardPtr)card_ref;
	if (1 == valid_card(card))
	{
		root = card_get_root_component(card);
		count_block = component_get_subcomponent_count_of_type(root, kComponentInterface);
		
		/* for each csi block */
		for (j = 0; j < count_block; j++)
		{
			/* get the component and the value of the type */
			comp = component_get_subcomponent(root, kComponentInterface, j);
			a_type = component_get_attribute(comp, kUint32AttributeCSIType);
			v_type = *((dag_block_type_t*)attribute_get_value(a_type));

			/* if type corresponds to the block desired */
			if (v_type == block_type)
			{
				/* get the number of counters */
				count_counter = component_get_subcomponent_count_of_type(comp, kComponentCounter);	
				
				/* for each counter */
				for (i = 0; i < count_counter; i++)
				{
					/* get the id type and subfct of counter */
					c_counter = component_get_subcomponent(comp, kComponentCounter, i);
					a_type = component_get_attribute(c_counter, kUint32AttributeCounterID);
					v_count_id = *((dag_counter_type_t*)attribute_get_value(a_type));
					attr = component_get_attribute(c_counter, kUint32AttributeSubFunction);
					v_subfct = *((dag_subfct_type_t*)attribute_get_value(attr));
					
					/* check the type of counter */	
					if ((v_count_id == counter_type) && (v_subfct == subfct_type))
					{
						/* get the value of the counter */
						a_value = component_get_attribute(c_counter, kUint64AttributeValue);
						counter_value = *((uint64_t*)attribute_get_value(a_value));
					}
				}
			}
		}
	}
	return (counter_value);

}
/*to set the value of the attribute from the attribute name and value*/
dag_err_t 
dag_config_set_named_attribute_uuid(dag_card_ref_t card, char *name_value,int index)
{
	DagCardPtr card_ref;
	card_ref = (DagCardPtr)card;	
	if(1 == valid_card(card_ref))
	{
		char *saveptr = NULL;
		char *current = name_value;
		char *ptr = name_value;
		int count = 0;
		int count_strip = 0;
		attr_uuid_t attribute;
		char *attribute_name = ptr;
		AttributePtr attr;
		/*stripping off white spaces*/
		while(*current != '\0')
		{
			if(*current == ' ')
				count++;
			else
			{
				name_value[count_strip] = name_value[count];
				count++;
				count_strip++;
			}
			current++;
		}
		name_value[count_strip] = '\0';

		strtok_r(ptr,"=",&saveptr);

		dagutil_verbose_level(2,"Attribute Name = %s\n",attribute_name);
		attribute = dag_config_get_indexed_named_attribute_uuid(card,attribute_name,index);
		if(attribute == kNullAttributeUuid)
		{
			/*Not able to find attribute return -1*/
			printf("Not able to find attribute %s\n",attribute_name);
			return  kDagErrNoSuchAttribute;
		}
		 attr = (AttributePtr)attribute;
		if (attribute_get_config_status(attr) != kDagAttrConfig)
		{
			printf("Trying to Set Status Attribute \n");
			return kDagErrStatusAttribute;
		}
		dagutil_verbose_level(2,"attribute name = %s and value = %s\n",attribute_name,saveptr);	
		dag_config_set_attribute_from_string(card,attribute,saveptr);
		return kDagErrNone;
	}
	return kDagErrInvalidCardRef;
}
const char*
dag_config_get_named_attribute_value(dag_card_ref_t card, char *name,int index)
{
	DagCardPtr card_ref;	
	const char* value; 
	
	card_ref = (DagCardPtr)card;
	if(1 == valid_card(card_ref))
	{
		attr_uuid_t attribute;
		dagutil_verbose_level(2,"attribute name = %s\n",name);
		attribute = dag_config_get_indexed_named_attribute_uuid(card,name,index);
		if(attribute == kNullAttributeUuid)
		{
			printf("Not able to find attribute %s\n",name);
		        dag_config_set_last_error(card,kDagErrNoSuchAttribute);
			return NULL;
		}
		value = dag_config_get_attribute_to_string(card,attribute);
		dagutil_verbose_level(2,"config attribute value = %s\n",value);	
		dag_config_set_last_error(card,kDagErrNone);			
		return value;
	}
	dag_config_set_last_error(card,kDagErrInvalidCardRef);
	return NULL;
}
dag_err_t
dag_config_set_infiniband_activemode(dag_card_ref_t card_ref)
{
	
	ComponentPtr root = NULL;
	ComponentPtr component = NULL;
	AttributePtr attribute = NULL;
	DagCardPtr card = (DagCardPtr)card_ref;
	uint32_t value = 0;
	bool value_bool = 0;
	int loop;
	/*Enable the classifier*/
	void *temp = NULL;
	uintptr_t address1 = 0;
	int res = 0;
	idt75k_dev_t *dev_p_la1;
	idt75k_dev_t *dev_p_la2;
	dev_p_la1 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));
	dev_p_la2 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));

	/*do the settings for Crosspoint switch*/
	root = card_get_root_component(card);
	if(root == NULL)
	{
		return kDagErrNoSuchComponent;
	}

		for(loop = 0;loop < 2;loop++)
		{
		
			/*Switch to be configured in port mode.TRANSMIT SHOULD BE ENABLED.
			 * disable the output ports of transmit configuration.
			 * OutputEnable = 0 for ports 0,1,6 and 7*/
	
			/*page reg - connection reg 0x7F - 0x00*/
			
			/* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  2     0
		 	*  3     1
		 	*  4     6
		 	*  5     7
		 	* */
			component = component_get_subcomponent(root,kComponentCrossPointSwitch,loop);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}

			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeSerialInterfaceEnable);
			attribute_set_value(attribute,&value,1);
	
			/*Energise Right Core   - On*/
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseRtCore);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Energise Left Core.   - On*/
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseLtCore);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Low Glitch Attribute  - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeLowGlitch);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Core Buffer Force On  - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreBufferForceOn);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Core Config Attribute - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreConfig);
			attribute_set_value(attribute,&value_bool,1);


			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value = 0;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
	
			/*Output Port 03 to Input Port 1*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);
	
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 04 to Input Port 6*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 05 to Input Port 7*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			/*Setting Output Power Level*/
			/*Output Power Level Changed from 0x8 -> 0xc*/
			/*Page Register Should contain - 0x22*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			/* Set Enable LOS Forwarding*/
			/*Connection Number - 2*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 3*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 4*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
	
			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Normal Mode of Operation*/

			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

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
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 6*/
			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
	
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
			attribute_set_value(attribute,&value,1);
	
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);
		
			/*Output Port 1 to Input Port 3*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
		
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 6 to Input Port 4*/
			value = 6;	
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 7 to Input Port 5*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);
			/*Connection setting done..*/
			/*Setting Up Long time constant Preemphasis decay and 
			Long time constant Pre-emphasis Level to 8 and 7 respectively.*/

			/*FOR OUTPUT no: 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);

			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			/*FOR OUTPUT no : 6*/


			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);

			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			/*FOR OUTPUT no: 0*/
			value = 0;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);

			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			/*FOR OUTPUT no : 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);

			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);
			
			/*SETTING DONE FOR OUTPUT PRE LONG REGISTER.*/

			/*OUTPUT LEVEL REGISTER FOR OUTPUT 0,1,6,7*/
			/*FOR OUTPUT no : 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);

			/*FOR OUTPUT no : 0*/
			value = 0;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);

	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);

			/*FOR OUTPUT no : 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);

			/* Set Enable LOS Forwarding*/
			/*Connection Number - 0*/
			value = 0;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 6*/
			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);
			/*Normal Mode of Operation*/

			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
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
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 3*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 4*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
		
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
		
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
		}
		
	/*Enable the firmware access to LA-1 . so that the data path is not blocked. when  in tapmode*/
			component = component_get_subcomponent(root,kComponentInfinibandClassifier,0);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			

			attribute = component_get_attribute(component,kBooleanAttributeLA0DataPathReset);
			temp = attribute_boolean_get_value(attribute);
			value_bool = *(bool*)temp;
			/*bool_value should be zero once data path reset is done.*/
			if(!value_bool) 
			{
				dagutil_warning("LA-0 data path not reset.Problem with system initialization \n");
			}
			/*Enable the firmware access to LA-0*/
			attribute = component_get_attribute(component,kBooleanAttributeLA0AccessEnable);
			value_bool = 1;
			attribute_boolean_set_value(attribute,(void*)&value_bool,1);
			/*Call TCAM Initialise to enable LA-1*/
			card = component_get_card(component);
			/*state = component_get_private_state(component);*/
			address1 = (card_get_register_address(card, DAG_REG_INFINICAM, 0) + (uintptr_t)card_get_iom_address(card));
	
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
			value_bool = *(bool*)temp;
			/*bool_value should be zero once data path reset is done*/
			if(!value_bool)
			{
				dagutil_warning("LA-1 data path not reset.Problem with system initialization \n");
			}
			/*Enable the firmware access to LA-0*/
			attribute = component_get_attribute(component,kBooleanAttributeLA1AccessEnable);
			value_bool = 1;
			attribute_boolean_set_value(attribute,(void*)&value_bool,1);
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

	return kDagErrNone;
}
dag_err_t
dag_config_set_infiniband_tapmode(dag_card_ref_t card_ref)
{
	ComponentPtr root = NULL;
	ComponentPtr component = NULL;
	AttributePtr attribute = NULL;
	DagCardPtr card = (DagCardPtr)card_ref;
	uint32_t value = 0;
	bool value_bool = 0;
	int loop;
	/*  classifier_state_t *state = NULL;*/
	void *temp = NULL;
	uintptr_t address1 = 0;
	int res = 0;
	idt75k_dev_t *dev_p_la1;
	idt75k_dev_t *dev_p_la2;
	dev_p_la1 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));
	dev_p_la2 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));
	
	/*do the settings for Crosspoint switch*/
	root = card_get_root_component(card);
	if(root == NULL)
	{
		return kDagErrNoSuchComponent;
	}

		         /*Switch to be configured in TAP mode.- ACTIVE TAP.
			 The customer will link up the HCA'S.The HCA's will link up as actively connected.*/
			/* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  2     0
		 	*  3     1
		 	*  4     6
		 	*  5     7
		 	* */
			/*making the normal monitormode connection. */
			
			for(loop = 0;loop < 2;loop++)
			{
			component = component_get_subcomponent(root,kComponentCrossPointSwitch,loop);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			
			/*Global Regiseter Basic Initializations_Start.*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeSerialInterfaceEnable);
			attribute_set_value(attribute,&value,1);
	
			/*Energise Right Core   - On*/
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseRtCore);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Energise Left Core.   - On*/
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseLtCore);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Low Glitch Attribute  - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeLowGlitch);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Core Buffer Force On  - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreBufferForceOn);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Core Config Attribute - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreConfig);
			attribute_set_value(attribute,&value_bool,1);

			/*Global Regiseter Basic Initializations_End.*/

			/*Actual Monitor Mode Connection Configuration Starts .*/
                        /* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  2     0
		 	*  3     1
		 	*  4     6
		 	*  5     7
		 	* */
			/*Output Port 2 to Input Port 0*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Output Port 03 to Input Port 1*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 04 to Input Port 6*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 05 to Input Port 7*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Setting Output Power Level for the output ports 2,3,4 and 5*/
			/*Changing the output power level from 8 to 0xc -- reason - need to investigate*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			/*Set Enable LOS Forwarding and the Mode of Operation for for Ouptut Connection Number 2,3,4 and 5*/
			/*
				Enable LOS forwarding - On
				Output State Operation Mode - 5 (Normal.)	
			*/
			/*Connection Number - 2*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 3*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 4*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);
			
			/*Normal Mode of Operation*/
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

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
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
		
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Input Connection Number - 6*/
			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
	}

			/*The actual Configuration of the tap bridge.*/
			/*For the first CrossPoint Switch the tap connection is as follows.*/

			/* Input               Output
			 *  0                  Y8 (used as output - need to disable A12 )
			 *  1	               Y9 (used as output - need to disable A13)
                         *  6                  Y10 (used as output - need to disable A14)
                  	 *  7                  Y11 (used as output - need to disable A15)  
			 */
			component = component_get_subcomponent(root,kComponentCrossPointSwitch,0);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 8;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 0;
			attribute_set_value(attribute,&value,1);
			/*Enable the Output Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; 
			attribute_set_value(attribute,&value_bool,1);
			/*Disable the shared Input Port A12*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 12;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
                         * Output LOS forwarding - 1
                         * Ouput State Operation Mode - 5 (Normal)
			*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 8; 
			attribute_set_value(attribute,&value,1);			

#if 0
            value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	
#endif
       
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			/*Newly added attributes to correct the signal form.*/
			/*
                        	output pre long level  = 2; 
                        	output pre long decay = 0;
                       	 	output pre short level = 8;
                        	output pre short decay = 0;
                        	output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			
			/*Here we are changing the output level power to 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
  
			/*General Settings For the Input Port - 0*/
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
             * InputStateInvert      - Off
             */
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 0; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Port 1 - Output Port Y9*/
			/* Input Port 1  - Y9 (used as output - need to disable A13) */
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 9;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 1;
			attribute_set_value(attribute,&value,1);
			/*Need to enable the Output Port - Y9
			Also Need to disable Corrosponding Physical Input Port - A13*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);
			/*Input Port A13 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 13;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
                         * Output LOS forwarding - 1
                         * Ouput State Operation Mode - 5 (Normal)
			*/						
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 9; /*Y9 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Newly added attributes to correct the signal form.. 
                        	output pre long level  = 2; 
                        	output pre long decay = 0;
                        	output pre short level = 8;
                        	output pre short decay = 0;
                        	output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			
			/*Here we are changing the output level power to 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
  			
			/*General Settings For the Input Port - 0*/
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
                         * InputStateInvert      - Off
                         */
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 1; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/* Input Port 6   Y10 (used as output - need to disable A14)*/
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 10;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 6;
			attribute_set_value(attribute,&value,1);
			/*Need to enable the Output Port - Y10
			 Also Need to disable Corrosponding Physical Input Port - A14*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);
			/*Input Port A14 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 14;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
                         * Output LOS forwarding - 1
                         * Ouput State Operation Mode - 5 (Normal)
			*/	
					
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 10; /*Y10 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Newly added attributes to correct the signal form 
                        	output pre long level  = 2; 
                        	output pre long decay = 0;
                        	output pre short level = 8;
                        	output pre short decay = 0;
                        	output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			
			/*Here we are changing the output level power to 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
  
			/*General Configuration for the input port*/
			
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
                         * InputStateInvert      - Off
                         */
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 6; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 7 - Output Port Y11*/	
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 11;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 7;
			attribute_set_value(attribute,&value,1);
			/*Need to enable the Output Port - Y11
			Also Need to disable Corrosponding Physical Input Port - A15*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);
			/*Input Port A15 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 15;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
                         * Output LOS forwarding - 1
                         * Ouput State Operation Mode - 5 (Normal)
			*/	
				
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 11; /*Y11 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Newly added attributes to correct the signal form.
                        	output pre long level  = 2; 
                        	output pre long decay = 0;
                        	output pre short level = 8;
                        	output pre short decay = 0;
                        	output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			
			/*Here we are changing the output level power to 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
  			
			/*General Configuration for the input port*/
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
                         * InputStateInvert      - Off
                         */
			/*input ports - 7*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 7; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
			
			/*Phase 2 Configuring the A8,A9,A10 and A11 as INPUT PORTS - this is for the loop back connection.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 0;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 8;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 12;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 0; /*Y8 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A8 (input enabled - Output turned off)
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 8; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 1 - Input Port A9*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 1;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 9;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 13;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- A9*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 1; 
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 9; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 6 - Input Port A10*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 6;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 10;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 14;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 6; 
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A10
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 10; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			//FINAL CONFIGURATION.
			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 7 - Input Port A11*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 7;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 11;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 15;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 7; 
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 11; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);					
			//NOW THE CONFIGURATION FOR P1 - TAKING OUT FROM THE LOOP

			// FOR CROSSPOINT SWITCH ONE.
			/*Switch to be configured in TAP mode.- ACTIVE TAP.
			The customer will link up the HCA'S.The HCA's will link up as actively connected.*/
			//TODO:Need to add the code to get the component from the root component
			/* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  Y8     0
		 	*  Y9     1
		 	*  Y10    6
		 	*  Y11    7
		 	* */
			/*MAKING THE ACTUAL CONNECTION*/
			
			component = component_get_subcomponent(root,kComponentCrossPointSwitch,1);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 12;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 0;
			attribute_set_value(attribute,&value,1);
			/*Need to enable the Output Port - Y8
			Also Need to disable Corrosponding Physical Input Port - A12*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);
			/*Input Port A12 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 8;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 12; /*Y8 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
                        output pre long level  = 2; 
                        output pre long decay = 0;
                        output pre short level = 8;
                        output pre short decay = 0;
                        output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			



			
			/*CONNECTIONS FOR THE INPUT PORT*/
			/*input ports - 0*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 0; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 1 - Output Port Y9*/
				
			/*MAKING THE ACTUAL CONNECTION*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 13;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 1;
			attribute_set_value(attribute,&value,1);
			/*Need to enable the Output Port - Y8
			Also Need to disable Corrosponding Physical Input Port - A12*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);
			/*Input Port A13 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 9;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 13; /*Y9 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
                        output pre long level  = 2; 
                        output pre long decay = 0;
                        output pre short level = 8;
                        output pre short decay = 0;
                        output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			/*input ports - 1*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 1; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 6 - Output Port Y10*/	

			/*MAKING THE ACTUAL CONNECTION*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 14;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 6;
			attribute_set_value(attribute,&value,1);
			/*Need to enable the Output Port - Y10
			Also Need to disable Corrosponding Physical Input Port - A14*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);
			/*Input Port A14 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 10;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 14; /*Y10 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
                        output pre long level  = 2; 
                        output pre long decay = 0;
                        output pre short level = 8;
                        output pre short decay = 0;
                        output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			/*input ports - 0,1,6,7*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 6; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 7 - Output Port Y11*/	

			/*MAKING THE ACTUAL CONNECTION*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 15;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 7;
			attribute_set_value(attribute,&value,1);
			/*Need to enable the Output Port - Y10
			Also Need to disable Corrosponding Physical Input Port - A15*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);
			/*Input Port A15 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 11;
			attribute_set_value(attribute,&value,1);
				
			value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 15; /*Y11 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
                        output pre long level  = 2; 
                        output pre long decay = 0;
                        output pre short level = 8;
                        output pre short decay = 0;
                        output level power = 5; 
			*/
			value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			attribute_set_value(attribute,&value,1);
			
			value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			attribute_set_value(attribute,&value,1);

			value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			/*input ports - 7*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 7; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
			
			/*Phase 2 Configuring the A12,A13,A14 and A15 as INPUT PORTS - this is for the loop back connection.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 0;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 12;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 8;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 0; /*Y8 -being used as output*/
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A8 (input enabled - Output turned off)
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 12; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 1 - Input Port A9*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 1;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 13;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 9;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- A9*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 1; 
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 13; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 6 - Input Port A10*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 6;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 14;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 10;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 6; 
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A10
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 14; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			//FINAL CONFIGURATION.
			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 7 - Input Port A11*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 7;
			attribute_set_value(attribute,&value,1);	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 15;
			attribute_set_value(attribute,&value,1);
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 11;
			attribute_set_value(attribute,&value,1);
			value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			attribute_set_value(attribute,&value_bool,1);
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 7; 
			attribute_set_value(attribute,&value,1);			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
                	{
                       	 	return kDagErrNoSuchAttribute;
                	}
			value_bool = 1; /*Output to be turned ON*/
			attribute_set_value(attribute,&value_bool,1);	

			value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
                	{
                        	return kDagErrNoSuchAttribute;
                	}
			value = 15; 
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);		

			
			/*Enable the firmware access to LA-1 . so that the data path is not blocked. when  in tapmode*/
			component = component_get_subcomponent(root,kComponentInfinibandClassifier,0);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			

			attribute = component_get_attribute(component,kBooleanAttributeLA0DataPathReset);
			temp = attribute_boolean_get_value(attribute);
			value_bool = *(bool*)temp;
			/*bool_value should be zero once data path reset is done.*/
			if(!value_bool) 
			{
				dagutil_warning("LA-0 data path not reset.Problem with system initialization \n");
			}
			/*Enable the firmware access to LA-0*/
			attribute = component_get_attribute(component,kBooleanAttributeLA0AccessEnable);
			value_bool = 1;
			attribute_boolean_set_value(attribute,(void*)&value_bool,1);
			/*Call TCAM Initialise to enable LA-1*/
			card = component_get_card(component);
			/*state = component_get_private_state(component);*/
			address1 = (card_get_register_address(card, DAG_REG_INFINICAM, 0) + (uintptr_t)card_get_iom_address(card));
	
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
			value_bool = *(bool*)temp;
			/*bool_value should be zero once data path reset is done*/
			if(!value_bool)
			{
				dagutil_warning("LA-1 data path not reset.Problem with system initialization \n");
			}
			/*Enable the firmware access to LA-0*/
			attribute = component_get_attribute(component,kBooleanAttributeLA1AccessEnable);
			value_bool = 1;
			attribute_boolean_set_value(attribute,(void*)&value_bool,1);
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

			return kDagErrNone;
}

dag_err_t
dag_config_set_infiniband_monitormode(dag_card_ref_t card_ref)
{
	ComponentPtr root = NULL;
	ComponentPtr component = NULL;
	AttributePtr attribute = NULL;
	DagCardPtr card = (DagCardPtr)card_ref;
	uint32_t value = 0;
	bool value_bool = 0;
	int loop;
	/*Enable the calssifier.*/
	void *temp = NULL;
	uintptr_t address1 = 0;
	int res = 0;
	idt75k_dev_t *dev_p_la1;
	idt75k_dev_t *dev_p_la2;
	dev_p_la1 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));
	dev_p_la2 = (idt75k_dev_t*)malloc(sizeof(idt75k_dev_t));
	
	/*do the settings for Crosspoint switch*/
	root = card_get_root_component(card);
	if(root == NULL)
	{
		return kDagErrNoSuchComponent;
	}
	for(loop = 0;loop < 2;loop++)
	{
			component = component_get_subcomponent(root,kComponentCrossPointSwitch,loop);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeSerialInterfaceEnable);
			attribute_set_value(attribute,&value,1);
	
			/*Energise Right Core   - On*/
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseRtCore);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Energise Left Core.   - On*/
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseLtCore);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Low Glitch Attribute  - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeLowGlitch);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Core Buffer Force On  - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreBufferForceOn);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Core Config Attribute - Off*/
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreConfig);
			attribute_set_value(attribute,&value_bool,1);


			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Output Port 03 to Input Port 1*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 04 to Input Port 6*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Output Port 05 to Input Port 7*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			attribute_set_value(attribute,&value_bool,1);

			/*Setting Output Power Level*/
			/*Output power level changed from 0x8 - 0xc ->> exact reasong being investigated.*/
			/*Page Register Should contain - 0x22*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			attribute_set_value(attribute,&value,1);	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			attribute_set_value(attribute,&value_bool,1);

			/* Set Enable LOS Forwarding*/
			/*Connection Number - 2*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 3*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);

			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 4*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Normal Mode of Operation*/	
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

			/*Connection Number - 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
	
			value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			attribute_set_value(attribute,&value_bool,1);
			
			/*Normal Mode of Operation*/
			value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			attribute_set_value(attribute,&value,1);

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
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
		
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
	
			/*Input Connection Number - 6*/
			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);
	
			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);

			/*Input Connection Number - 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			attribute_set_value(attribute,&value_bool,1);

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			attribute_set_value(attribute,&value_bool,1);
	}
	

	/*Enable the firmware access to LA-1 . so that the data path is not blocked. when  in tapmode*/
			component = component_get_subcomponent(root,kComponentInfinibandClassifier,0);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			

			attribute = component_get_attribute(component,kBooleanAttributeLA0DataPathReset);
			temp = attribute_boolean_get_value(attribute);
			value_bool = *(bool*)temp;
			/*bool_value should be zero once data path reset is done.*/
			if(!value_bool) 
			{
				dagutil_warning("LA-0 data path not reset.Problem with system initialization \n");
			}
			/*Enable the firmware access to LA-0*/
			attribute = component_get_attribute(component,kBooleanAttributeLA0AccessEnable);
			value_bool = 1;
			attribute_boolean_set_value(attribute,(void*)&value_bool,1);
			/*Call TCAM Initialise to enable LA-1*/
			card = component_get_card(component);
			/*state = component_get_private_state(component);*/
			address1 = (card_get_register_address(card, DAG_REG_INFINICAM, 0) + (uintptr_t)card_get_iom_address(card));
	
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
			value_bool = *(bool*)temp;
			/*bool_value should be zero once data path reset is done*/
			if(!value_bool)
			{
				dagutil_warning("LA-1 data path not reset.Problem with system initialization \n");
			}
			/*Enable the firmware access to LA-0*/
			attribute = component_get_attribute(component,kBooleanAttributeLA1AccessEnable);
			value_bool = 1;
			attribute_boolean_set_value(attribute,(void*)&value_bool,1);
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



	return kDagErrNone;
}

int dag_config_get_recursive_component_count(dag_card_ref_t card_ref, dag_component_code_t comp_code)
{
    int comp_count, comp_index, subcomp_count, subcomp_index;
    int total = 0;
    dag_component_t root, comp, subcomp = NULL;
    
    /* sanity checks */
    if (!valid_card((DagCardPtr)card_ref) || 
        !(root = dag_config_get_root_component(card_ref)))
    {
        return 0;
    }

    comp_count = dag_component_get_subcomponent_count(root);
    /* go through all components of the card */
    for (comp_index = 0; comp_index < comp_count; comp_index++)
    {
        comp = dag_component_get_indexed_subcomponent(root, comp_index);
        if((NULL != comp)&&
                        (dag_config_get_component_code(comp) == comp_code))
            {
                total++;
                continue;
            }
        /* go through all the subcomponents first */
        subcomp_count = dag_component_get_subcomponent_count(comp);
        for(subcomp_index = 0; subcomp_index < subcomp_count; subcomp_index++)
        {
            subcomp = dag_component_get_indexed_subcomponent(comp, subcomp_index);
            if((NULL != subcomp)&&
                        (dag_config_get_component_code(subcomp) == comp_code))
            {
                total++;
                continue;
            }
        }
    }

    return total;
}

dag_component_t dag_config_get_recursive_indexed_component(dag_card_ref_t card_ref, dag_component_code_t comp_code, int a_index)
{
    int comp_count, comp_index, subcomp_count, subcomp_index;
    int total = 0;
    dag_component_t root, comp, subcomp = NULL;
    
    /* sanity checks */
    if (!valid_card((DagCardPtr)card_ref) || 
        !(root = dag_config_get_root_component(card_ref)))
    {
        return NULL;
    }

    comp_count = dag_component_get_subcomponent_count(root);
    /* go through all components of the card */
    for (comp_index = 0; comp_index < comp_count; comp_index++)
    {
        comp = dag_component_get_indexed_subcomponent(root, comp_index);
        if((NULL != comp)&&
                        (dag_config_get_component_code(comp) == comp_code))
        {
            if( total == a_index ) 
            {
                return comp;
            }
            total++;
            continue;
        }
        /* go through the subcomponents first */
        subcomp_count = dag_component_get_subcomponent_count(comp);
        for(subcomp_index = 0; subcomp_index < subcomp_count; subcomp_index++)
        {
            subcomp = dag_component_get_indexed_subcomponent(comp, subcomp_index);
            if((NULL != subcomp)&&
                        (dag_config_get_component_code(subcomp) == comp_code))
            {
                if( total == a_index ) 
                {
                    return subcomp;
                }   
                total++;
                continue;
            }
        }
    }
    return NULL; 
}

bool
dag_config_get_infiniband_tapmode(dag_card_ref_t card_ref)
{
	ComponentPtr root = NULL;
	ComponentPtr component = NULL;
	AttributePtr attribute = NULL;
	DagCardPtr card = (DagCardPtr)card_ref;
	uint32_t value = 0;
	bool value_bool = 0;
	int loop;
	/*  classifier_state_t *state = NULL;*/
	bool tap_mode = false;	

	/*do the settings for Crosspoint switch*/
	root = card_get_root_component(card);
	if(root == NULL)
	{
		return kDagErrNoSuchComponent;
	}
			/*Switch to be configured in TAP mode.- ACTIVE TAP.
			 The customer will link up the HCA'S.The HCA's will link up as actively connected.*/
			/* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  2     0
		 	*  3     1
		 	*  4     6
		 	*  5     7
		 	* */
			/*making the normal monitormode connection. */
			
			for(loop = 0;loop < 2;loop++)
			{
				component = component_get_subcomponent(root,kComponentCrossPointSwitch,loop);
				if(component == NULL)
				{
					return kDagErrNoSuchComponent;
				}
			
			/*Global Regiseter Basic Initializations_Start.*/
			//value = 2;
			attribute = component_get_attribute(component,kUint32AttributeSerialInterfaceEnable);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	
			/*Energise Right Core   - On*/
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseRtCore);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Energise Left Core.   - On*/
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseLtCore);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			/*Low Glitch Attribute  - Off*/
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeLowGlitch);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Core Buffer Force On  - Off*/
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreBufferForceOn);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	
			/*Core Config Attribute - Off*/
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreConfig);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;	
			}
			/*Global Regiseter Basic Initializations_End.*/
			/*Actual Monitor Mode Connection Configuration Starts .*/
            /* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  2     0
		 	*  3     1
		 	*  4     6
		 	*  5     7
		 	* */
			/*Output Port 2 to Input Port 0*/
			
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 0;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

				
			/*Output Port 03 to Input Port 1*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if (value != 3)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
	
			//value = 1;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Output Port 04 to Input Port 6*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 4)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value = 6;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 6)
			{	
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Output Port 05 to Input Port 7*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value = 7;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*PHASE ONE OF THE IMPLEMENTATION*/


			/*Setting Output Power Level for the output ports 2,3,4 and 5*/
			/*Changing the output power level from 8 to 0xc -- reason - need to investigate*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 3)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 4)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if( value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

	//////////////////////////////////////////////////////////////////////////////////////
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Set Enable LOS Forwarding and the Mode of Operation for for Ouptut Connection Number 2,3,4 and 5*/
			/*
				Enable LOS forwarding - On
				Output State Operation Mode - 5 (Normal.)	
			*/
			/*Connection Number - 2*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Connection Number - 3*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 3)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Connection Number - 4*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	
			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Connection Number - 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
	
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Normal Mode of Operation*/
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

		/*##########################################################################################################################################################################################################################*/		

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
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Input Connection Number - 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
		
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	
			/*Input Connection Number - 6*/
			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
		
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;s
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Input Connection Number - 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)	
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
	}

			/*The actual Configuration of the tap bridge.*/
			/*For the first CrossPoint Switch the tap connection is as follows.*/

			/* Input               Output
			 *  0                  Y8 (used as output - need to disable A12 )
			 *  1	               Y9 (used as output - need to disable A13)
             *  6                  Y10 (used as output - need to disable A14)
             *  7                  Y11 (used as output - need to disable A15)  
			 */
			component = component_get_subcomponent(root,kComponentCrossPointSwitch,0);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                    return kDagErrNoSuchAttribute;
            }
			value = 8;
			attribute_set_value(attribute,&value,1);	
			#if 0
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 0;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Enable the Output Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; 
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Disable the shared Input Port A12*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 12;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 12)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
				
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
             * Output LOS forwarding - 1
             * Ouput State Operation Mode - 5 (Normal)
			 */		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
            	return kDagErrNoSuchAttribute;
            }
			value = 8; 
			attribute_set_value(attribute,&value,1);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}			

#if 0
            //value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
            printf("Value : %d\n",value);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
#endif
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Newly added attributes to correct the signal form.*/
			/*
              output pre long level  = 2; 
              output pre long decay = 0;
              output pre short level = 8;
              output pre short decay = 0;
              output level power = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Here we are changing the output level power to 5*/
			//value = 5;

			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*General Settings For the Input Port - 0*/
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
             * InputStateInvert      - Off
             */
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 0; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Input Port 1 - Output Port Y9*/
			/*Input Port 1  - Y9 (used as output - need to disable A13) */
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 9;
			attribute_set_value(attribute,&value,1);
			if(value != 9)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 1;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Need to enable the Output Port - Y9
			Also Need to disable Corrosponding Physical Input Port - A13*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Input Port A13 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 13;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 13)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
				
			//value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
             * Output LOS forwarding - 1
             * Ouput State Operation Mode - 5 (Normal)
			*/						
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 9; /*Y9 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 9)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif			

			//value = 8;
#if 0
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	
#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Newly added attributes to correct the signal form.. 
              output pre long level  = 2; 
              output pre long decay = 0;
              output pre short level = 8;
              output pre short decay = 0;
              output level power = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Here we are changing the output level power to 5*/
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
  			/*General Settings For the Input Port - 0*/
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
             * InputStateInvert      - Off
             */
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                   return kDagErrNoSuchAttribute;
            }
			value = 1; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/* Input Port 6   Y10 (used as output - need to disable A14)*/
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                   return kDagErrNoSuchAttribute;
            }
			value = 10;
			attribute_set_value(attribute,&value,1);
			#if 0	
			if(value != 10)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 6;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Need to enable the Output Port - Y10
			 Also Need to disable Corrosponding Physical Input Port - A14*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Input Port A14 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                   return kDagErrNoSuchAttribute;
            }
			value = 14;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 14)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	
			#endif			
			//value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
             * Output LOS forwarding - 1
             * Ouput State Operation Mode - 5 (Normal)
			*/	
					
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 10; /*Y10 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 10)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 8;
#if 0
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	
#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);	
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Newly added attributes to correct the signal form 
                   output pre long level  = 2; 
                   output pre long decay = 0;
                   output pre short level = 8;
                   output pre short decay = 0;
                   output level power = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Here we are changing the output level power to 5*/
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*General Configuration for the input port*/
			
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
             * InputStateInvert      - Off
             */
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 6; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 7 - Output Port Y11*/	
			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 11;
			attribute_set_value(attribute,&value,1);	
			#if 0
			if(value != 11)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 7;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Need to enable the Output Port - Y11
			Also Need to disable Corrosponding Physical Input Port - A15*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Input Port A15 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 15;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 15)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
	
			//value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/
			/*General Output Port Configurations for Output Port Y8*/
			/*
			 * Ouptut Power Level - 8 	
			 * Output Level Terminate - 0
             * Output LOS forwarding - 1
             * Ouput State Operation Mode - 5 (Normal)
			*/	
				
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                   return kDagErrNoSuchAttribute;
            }
			value = 11; /*Y11 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 11)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif			

			//value = 8;
#if 0
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
#endif
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Newly added attributes to correct the signal form.
               output pre long level  = 2; 
               output pre long decay = 0;
               output pre short level = 8;
               output pre short decay = 0;
               output level power = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Here we are changing the output level power to 5*/
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{	
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
  			
			/*General Configuration for the input port*/
			/* InputStateTerminate   - Off
			 * InputStateOff         - Off	
             * InputStateInvert      - Off
             */
			
			/*input ports - 7*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 7; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Phase 2 Configuring the A8,A9,A10 and A11 as INPUT PORTS - this is for the loop back connection.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 0;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 8;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 12;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 12)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 0; /*Y8 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);	
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A8 (input enabled - Output turned off)
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 8; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 1 - Input Port A9*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 1;
			attribute_set_value(attribute,&value,1);	
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 9;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 9)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 13;
			attribute_set_value(attribute,&value,1);
			if(value != 13)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- A9*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 1; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool == 0)	
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value = 9; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 9)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 6 - Input Port A10*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 6;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 10;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 10)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 14;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 14)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
		
			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 6; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);	
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A10
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 10; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 10)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//FINAL CONFIGURATION.
			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 7 - Input Port A11*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 7;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 11;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 11)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 15;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 15)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 7; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 11; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 11)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}					
			//NOW THE CONFIGURATION FOR P1 - TAKING OUT FROM THE LOOP

			// FOR CROSSPOINT SWITCH ONE.
			/*Switch to be configured in TAP mode.- ACTIVE TAP.
			The customer will link up the HCA'S.The HCA's will link up as actively connected.*/
			//TODO:Need to add the code to get the component from the root component
			/* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  Y8     0
		 	*  Y9     1
		 	*  Y10    6
		 	*  Y11    7
		 	* */
			/*MAKING THE ACTUAL CONNECTION*/
			
			component = component_get_subcomponent(root,kComponentCrossPointSwitch,1);
			if(component == NULL)
			{
				return kDagErrNoSuchComponent;
			}
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 12;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 12)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 0;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Need to enable the Output Port - Y8
			Also Need to disable Corrosponding Physical Input Port - A12*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Input Port A12 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 8;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
				
			//value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
			{
					return kDagErrNoSuchAttribute;
			}
			value = 12; /*Y8 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 12)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
              output pre long level  = 2; 
              output pre long decay = 0;
              output pre short level = 8;
              output pre short decay = 0;
              output level power = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*input ports - 0*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                   return kDagErrNoSuchAttribute;
            }
			value = 0; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 1 - Output Port Y9*/
				
			/*MAKING THE ACTUAL CONNECTION*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                   return kDagErrNoSuchAttribute;
            }
			value = 13;
			attribute_set_value(attribute,&value,1);	
			#if 0
			if(value != 13)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 1;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Need to enable the Output Port - Y8
			Also Need to disable Corrosponding Physical Input Port - A12*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Input Port A13 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 9;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 9)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
				
			//value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 13; /*Y9 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0			
			if(value != 13)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
              	output pre long level  = 2; 
                output pre long decay = 0;
                output pre short level = 8;
                output pre short decay = 0;
                output level power = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			/*input ports - 1*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 1; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 6 - Output Port Y10*/	

			/*MAKING THE ACTUAL CONNECTION*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 14;
			attribute_set_value(attribute,&value,1);
			#if 0	
			if(value != 14)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 6;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Need to enable the Output Port - Y10
			Also Need to disable Corrosponding Physical Input Port - A14*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Input Port A14 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 10;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 10)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
				
			//value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 14; /*Y10 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 14)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool =*(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
                output pre long level  = 2; 
                output pre long decay  = 0;
                output pre short level = 8;
                output pre short decay = 0;
                output level power     = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			/*input ports - 0,1,6,7*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 6; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Input Port 7 - Output Port Y11*/	
			/*MAKING THE ACTUAL CONNECTION*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 15;
			attribute_set_value(attribute,&value,1);
			#if 0	
			if(value != 15)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 7;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Need to enable the Output Port - Y10
			Also Need to disable Corrosponding Physical Input Port - A15*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Input Port A15 - to be disabled.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 11;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 11)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 1;/*INPUT TO BE TURNED OFF - PHYSICAL PORT BEING USED AS OUTPUT - Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 15; /*Y11 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0			
			if(value != 15)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*NEWLY ADDED ATTRIBUTE VALUES TO CORRECT THE SIGNAL FROM. 
              output pre long level  = 2; 
              output pre long decay = 0;
              output pre short level = 8;
              output pre short decay = 0;
              output level power = 5; 
			*/
			//value = 2;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreLongDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			//value = 8;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortLevel);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 0;	
			attribute = component_get_attribute(component,kUint32AttributeOutputPreShortDecay);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			/*input ports - 7*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 7; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)	
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*Phase 2 Configuring the A12,A13,A14 and A15 as INPUT PORTS - this is for the loop back connection.*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 0;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 12;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 12)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 8;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 0; /*Y8 -being used as output*/
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A8 (input enabled - Output turned off)
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                  return kDagErrNoSuchAttribute;
            }
			value = 12; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 12)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 1 - Input Port A9*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 1;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif	
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 13;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 13)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                        	return kDagErrNoSuchAttribute;
            }
			value = 9;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 9)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- A9*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 1; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{	
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{	
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 13; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 13)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 6 - Input Port A10*/
		
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 6;
			attribute_set_value(attribute,&value,1);
			#if 0	
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			//value = 14;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 14)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 10;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 10)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			value = 6; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif			

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A10
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 14; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 14)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//FINAL CONFIGURATION.
			/*FOR THE NEXT PAIR OF INPUT OUTPUT CONNECTIONS.*/
			/*Output Port 7 - Input Port A11*/
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 7;
			attribute_set_value(attribute,&value,1);
#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
#endif		
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			if(attribute == NULL)
			{
				 return kDagErrNoSuchAttribute;
			}
			//value = 15;
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 15)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			//SPECIFIC STUFF - GENERAL CONFIGURATION FOLLOWS
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 11;
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 11)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif
			//value_bool = 0;/*OUTPUT TO BE TURNED OFF for it to function as Input Port- Y8*/
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
			{
				return kDagErrNoSuchAttribute;
			}
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			/*Output Port Configurations - GENERAL*/			
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 7; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			if(attribute == NULL)
            {
                return kDagErrNoSuchAttribute;
            }
			//value_bool = 1; /*Output to be turned ON*/
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}	

			//value = 8;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 8)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);	
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			
			/*CONNECTIONS FOR THE INPUT PORT*/
			//INPUT port - A9
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			if(attribute == NULL)
            {
                 return kDagErrNoSuchAttribute;
            }
			value = 15; 
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 15)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);	
				return tap_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);		
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in tap mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return tap_mode;
			}

		/*coming here*/
		return true;
}

bool
dag_config_get_infiniband_monitormode(dag_card_ref_t card_ref)
{
	ComponentPtr root = NULL;
	ComponentPtr component = NULL;
	AttributePtr attribute = NULL;
	DagCardPtr card = (DagCardPtr)card_ref;
	uint32_t value = 0;
	bool value_bool = 0;
	int loop;
	/*  classifier_state_t *state = NULL;*/
	bool monitor_mode = false;	

	/*do the settings for Crosspoint switch*/
	root = card_get_root_component(card);
	if(root == NULL)
	{
		return kDagErrNoSuchComponent;
	}
			/*Switch to be configured in TAP mode.- ACTIVE TAP.
			 The customer will link up the HCA'S.The HCA's will link up as actively connected.*/
			/* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  2     0
		 	*  3     1
		 	*  4     6
		 	*  5     7
		 	* */
			/*making the normal monitormode connection. */
			
			for(loop = 0;loop < 2;loop++)
			{
				component = component_get_subcomponent(root,kComponentCrossPointSwitch,loop);
				if(component == NULL)
				{
					return kDagErrNoSuchComponent;
				}
			
			/*Global Regiseter Basic Initializations_Start.*/
			//value = 2;
			attribute = component_get_attribute(component,kUint32AttributeSerialInterfaceEnable);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	
			/*Energise Right Core   - On*/
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseRtCore);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			/*Energise Left Core.   - On*/
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeEnergiseLtCore);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}	

			/*Low Glitch Attribute  - Off*/
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeLowGlitch);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			/*Core Buffer Force On  - Off*/
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreBufferForceOn);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}	
			/*Core Config Attribute - Off*/
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeCoreConfig);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;	
			}
			/*Global Regiseter Basic Initializations_End.*/
			/*Actual Monitor Mode Connection Configuration Starts .*/
            /* connection - for the transmit ports is as follows
		 	* 
		 	* o/p  - i/p
		 	*  2     0
		 	*  3     1
		 	*  4     6
		 	*  5     7
		 	* */
			/*Output Port 2 to Input Port 0*/
			
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value = 0;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

				
			/*Output Port 03 to Input Port 1*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if (value != 3)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif
	
			//value = 1;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Output Port 04 to Input Port 6*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 4)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value = 6;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 6)
			{	
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Output Port 05 to Input Port 7*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value = 7;
			attribute = component_get_attribute(component,kUint32AttributeSetConnection);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanAttributeOutputEnable);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*PHASE ONE OF THE IMPLEMENTATION*/


			/*Setting Output Power Level for the output ports 2,3,4 and 5*/
			/*Changing the output power level from 8 to 0xc -- reason - need to investigate*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}	

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 3)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 4)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);
			if( value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value = 0xc;
			attribute = component_get_attribute(component,kUint32OutputLevelPower);
			value = *(uint32_t*)attribute_get_value(attribute);	
			if(value != 0xc)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

	//////////////////////////////////////////////////////////////////////////////////////
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanOutputLevelTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Set Enable LOS Forwarding and the Mode of Operation for for Ouptut Connection Number 2,3,4 and 5*/
			/*
				Enable LOS forwarding - On
				Output State Operation Mode - 5 (Normal.)	
			*/
			/*Connection Number - 2*/
			value = 2;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 2)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif
	
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Connection Number - 3*/
			value = 3;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 3)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			
			/*Connection Number - 4*/
			value = 4;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	
			/*Normal Mode of Operation*/	
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Connection Number - 5*/
			value = 5;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif
	
			//value_bool = 1;
			attribute = component_get_attribute(component,kBooleanOutputStateLOSForwarding);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != true)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			
			/*Normal Mode of Operation*/
			//value = 5;
			attribute = component_get_attribute(component,kUint32OutputStateOperationMode);
			value = *(uint32_t*)attribute_get_value(attribute);
			if(value != 5)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

		/*##########################################################################################################################################################################################################################*/		

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
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 0)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Input Connection Number - 1*/
			value = 1;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 1)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif
			
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
		
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	
			/*Input Connection Number - 6*/
			value = 6;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 6)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
		
	
			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			//value_bool = 0;s
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			/*Input Connection Number - 7*/
			value = 7;
			attribute = component_get_attribute(component,kUint32AttributeCrossPointConnectionNumber);
			attribute_set_value(attribute,&value,1);
			#if 0
			if(value != 7)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
			#endif

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateTerminate);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateOff);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)	
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}

			//value_bool = 0;
			attribute = component_get_attribute(component,kBooleanAttributeInputStateInvert);
			value_bool = *(bool*)attribute_get_value(attribute);
			if(value_bool != false)
			{
				dagutil_verbose_level(2,"attribute value %s is not in monitor mode line no %d\n", attribute_get_name(attribute),__LINE__);
				return monitor_mode;
			}
	}
	return true;
}
