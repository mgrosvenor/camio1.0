/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/util/enum_string_table.h"
#include "../include/attribute_factory.h"
#include "../../../include/dag_attribute_codes.h"
#include "../include/create_attribute.h"

#include "dagutil.h"
#include "dagapi.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif


typedef struct
{
    uintptr_t mPbmBase;
    uint32_t mPbmVer;
    int mIndex;
} pbm_state_t;
#if 0 //commenting out to avoid the duplication of the same table in diffrent places
pbm_offsets_t PBM[3] = {
	{ // pbm v0
	/*	globalstatus:*/		0x1c,
	/*	streambase:*/		0x0,
	/*	streamsize:*/		0x60,
	/*	streamstatus:*/		0x1c,
	/*	mem_addr:*/			0x0,
	/*	mem_size:*/			0x04,
	/*	record_ptr:*/		0x18,
	/*	limit_ptr:*/		0x10,
	/*	safetynet_cnt:*/	0x14,
	/*	drop_cnt:*/			0x0c,
	},
	{ //pbm v1
	/*	globalstatus:*/		0x0,
	/*	streambase:*/		0x40,
	/*	streamsize:*/		0x40,
	/*	streamstatus:*/		0x0,
	/*	mem_addr:*/			0x04,
	/*	mem_size:*/			0x08,
	/*	record_ptr:*/		0x0c,
	/*	limit_ptr:*/		0x10,
	/*	safetynet_cnt:*/	0x14,
	/*	drop_cnt:*/			0x18,		
	},
	{ //pbm v2 CSBM
	/*	globalstatus:*/		0x0,
	/*	streambase:*/		0x40,
	/*	streamsize:*/		0x40,
	/*	streamstatus:*/		0x30,
	/*	mem_addr:*/			0x00,
	/*	mem_size:*/			0x08,
	/*	record_ptr:*/		0x18,
	/*	limit_ptr:*/		0x20,
	/*	safetynet_cnt:*/	0x28,
	/*	drop_cnt:*/			0x80000000,	/* Not present in pbm v2!!! */	
	}

};
#endif

/* overlap attribute. */
AttributePtr pbm_get_new_overlap_attribute(void);
static void pbm_overlap_dispose(AttributePtr attribute);
static void* pbm_overlap_get_value(AttributePtr attribute);
static void pbm_overlap_set_value(AttributePtr attribute, void* value, int length);
static void pbm_overlap_post_initialize(AttributePtr attribute);

/* pbm component. */
static void pbm_dispose(ComponentPtr component);
static void pbm_reset(ComponentPtr component);
static void pbm_default(ComponentPtr component);
static int pbm_post_initialize(ComponentPtr component);
static dag_err_t pbm_update_register_base(ComponentPtr component);

/* pci_bus_speed */
AttributePtr pbm_get_new_pci_bus_speed_attribute(void);
static void pbm_pci_bus_speed_dispose(AttributePtr attribute);
static void* pbm_pci_bus_speed_get_value(AttributePtr attribute);
static void pbm_pci_bus_speed_to_string(AttributePtr attribute);
static void pbm_pci_bus_speed_post_initialize(AttributePtr attribute);

/* buffer size */
AttributePtr pbm_get_new_buffer_size_attribute(void);
static void pbm_buffer_size_dispose(AttributePtr attribute);
static void* pbm_buffer_size_get_value(AttributePtr attribute);
static void pbm_buffer_size_post_initialize(AttributePtr attribute);

/* rx stream count */
AttributePtr pbm_get_new_rx_stream_count_attribute(void);
static void pbm_rx_stream_count_dispose(AttributePtr attribute);
static void* pbm_rx_stream_count_get_value(AttributePtr attribute);
static void pbm_rx_stream_count_post_initialize(AttributePtr attribute);

/* tx stream count */
AttributePtr pbm_get_new_tx_stream_count_attribute(void);
static void pbm_tx_stream_count_dispose(AttributePtr attribute);
static void* pbm_tx_stream_count_get_value(AttributePtr attribute);
static void pbm_tx_stream_count_post_initialize(AttributePtr attribute);

/* drop/nodrop. Tells the card where to drop packets and is used with hlb
 * images. If on dropping of packets occurs at the individual stream that has
 * filled up. If off dropping occurs at the gpp. */
AttributePtr pbm_get_new_drop_attribute(void);
static void pbm_drop_dispose(AttributePtr attribute);
static void* pbm_drop_get_value(AttributePtr attribute);
static void pbm_drop_set_value(AttributePtr attribute, void* val, int len);
static void pbm_drop_post_initialize(AttributePtr attribute);

/* For PCI bus type attribute */
static void* pci_bus_type_get_value(AttributePtr attribute);
static void pci_bus_type_to_string_routine(AttributePtr attribute);

/* new attributes are given as an attribute table */
Attribute_t pbm_attributes[] = 
{
    {
    /* Name */                 "pci_bus_type",
    /* Attribute Code */       kUint32AttributePCIBusType,
    /* Attribute Type */       kAttributeUint32,
    /* Description */          "Module type identifier",
    /* Config-Status */        kDagAttrStatus,
    /* Index in register */    2,/* dont care */
    /* Register Address */     DAG_REG_PBM, /* dont care */
    /* Offset */               0,/* dont care */
    /* Size/length */          1,/* dont care */
    /* Read */                 NULL,/* dont care */
    /* Write */                NULL,/* dont care */
    /* Mask */                 BIT2,/* dont care */
    /* Default Value */        1,/* dont care */
    /* SetValue */             attribute_uint32_set_value,/* Status attribute -funtion not needed*/
    /* GetValue */             pci_bus_type_get_value,
    /* SetToString */          pci_bus_type_to_string_routine,
    /* SetFromString */        attribute_uint32_from_string,/* Status attribute -funtion not needed*/
    /* Dispose */              attribute_dispose,
    /* PostInit */             attribute_post_initialize,
    },
};
#define NB_ELEM_PBM_ATTR (sizeof(pbm_attributes) / sizeof(Attribute_t))
/* pci_bus_speed */
AttributePtr
pbm_get_new_pci_bus_speed_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributePCIBusSpeed); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pbm_pci_bus_speed_dispose);
        attribute_set_post_initialize_routine(result, pbm_pci_bus_speed_post_initialize);
        attribute_set_getvalue_routine(result, pbm_pci_bus_speed_get_value);
        attribute_set_name(result, "pci_bus_speed");
        attribute_set_to_string_routine(result, pbm_pci_bus_speed_to_string);
        attribute_set_description(result, "A number representing the PCI bus speed");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
pbm_pci_bus_speed_dispose(AttributePtr attribute)
{

}

static void
pbm_pci_bus_speed_to_string(AttributePtr attribute)
{
    pci_bus_speed_t speed;
    const char* temp;

    speed = *(pci_bus_speed_t*)attribute_get_value(attribute);
    temp = pci_bus_speed_to_string(speed);
    attribute_set_to_string(attribute, temp);
}

static void*
pbm_pci_bus_speed_get_value(AttributePtr attribute)
{
    ComponentPtr component;
    DagCardPtr card;
    pci_bus_speed_t val = 0;
    uint32_t register_value;
    pbm_offsets_t *PBM = NULL;

    PBM = dagutil_get_pbm_offsets();
    if( NULL == PBM )
    {
        dagutil_panic("Unable to get PBM offset in %s\n",__FUNCTION__);
        return NULL;
    }
    if (1 == valid_attribute(attribute))
    {
        pbm_state_t* state = NULL;
        component = attribute_get_component(attribute);
        state = (pbm_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
		register_value = card_read_iom(card, state->mPbmBase + PBM[state->mPbmVer].globalstatus);
        register_value = (register_value >> 28) & 0xf;
        if (0x1 == register_value)
        {
            val = kPCIBusSpeed33Mhz;
        }
        else if (0x2 == register_value)
        {
            val = kPCIBusSpeed66Mhz;
        }
        else if (0x3 == register_value)
        {
            val = kPCIBusSpeed100Mhz;
        }
        else if (0x4 == register_value)
        {
            val = kPCIBusSpeed133Mhz;
        }
        else if (0xe == register_value)
        {
            val = kPCIBusSpeedUnknown;
        }
        else if (0x9 == register_value)
        {
		val = kPCIEBusSpeed2Gbs;
        }
        else if (0xa == register_value)
        {
		val = kPCIEBusSpeed4Gbs;
        }
        else if (0xb == register_value)
        {
		val = kPCIEBusSpeed8Gbs;
        }
        else if (0xc == register_value)
        {
		val = kPCIEBusSpeed16Gbs;
        }
        else if (0xf == register_value)
        {
            val = kPCIBusSpeedUnstable;
        }
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
pbm_pci_bus_speed_post_initialize(AttributePtr attribute)
{

}

ComponentPtr
pbm_get_new_pbm(DagCardPtr card, int index)
{
    ComponentPtr result = component_init(kComponentPbm, card); 
    
    if (NULL != result)
    {
        pbm_state_t* state = NULL;
        component_set_dispose_routine(result, pbm_dispose);
        component_set_post_initialize_routine(result, pbm_post_initialize);
        component_set_reset_routine(result, pbm_reset);
        component_set_default_routine(result, pbm_default);
        component_set_name(result, "pbm");
		component_set_description(result, "The PCI Burst Manager");
        component_set_update_register_base_routine(result, pbm_update_register_base);
        state = (pbm_state_t*)malloc(sizeof(pbm_state_t));
        memset(state, 0, sizeof(pbm_state_t));
        component_set_private_state(result, state);
        state->mIndex = index;
    }
    
    return result;
}


static void
pbm_dispose(ComponentPtr component)
{
}

static dag_err_t
pbm_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = component_get_card(component);
        pbm_state_t* state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);
        state->mPbmVer = card_get_register_version(card, DAG_REG_PBM, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidCardRef;
}

static int
pbm_post_initialize(ComponentPtr component)
{
    /* Return 1 if standard post_initialize() should continue, 0 if not.
    * "Standard" post_initialize() calls post_initialize() recursively on subcomponents.
    */
    DagCardPtr card = NULL;
    AttributePtr pci_bus_speed;
    AttributePtr buffer_size;
    AttributePtr rx_stream_count;
    AttributePtr tx_stream_count;
    AttributePtr overlap;
    AttributePtr drop;

    pbm_state_t* state = NULL;
    card = component_get_card(component);
    state = component_get_private_state(component);
    state->mPbmBase = card_get_register_address(card, DAG_REG_PBM, 0);
    state->mPbmVer = card_get_register_version(card, DAG_REG_PBM, 0);
    pci_bus_speed = pbm_get_new_pci_bus_speed_attribute();
    component_add_attribute(component, pci_bus_speed);
    buffer_size = pbm_get_new_buffer_size_attribute();
    component_add_attribute(component, buffer_size);
    tx_stream_count = pbm_get_new_tx_stream_count_attribute();
    component_add_attribute(component, tx_stream_count);
    rx_stream_count = pbm_get_new_rx_stream_count_attribute();
    component_add_attribute(component, rx_stream_count);
    overlap = pbm_get_new_overlap_attribute();
    component_add_attribute(component, overlap);
    drop = pbm_get_new_drop_attribute();
    component_add_attribute(component, drop);
    
    /* add the attributes defined in attribute table */
   read_attr_array(component, pbm_attributes, NB_ELEM_PBM_ATTR, state->mIndex);

    return 0;
}

static void
pbm_reset(ComponentPtr component)
{
}


static void
pbm_default(ComponentPtr component)
{
    DagCardPtr card = component_get_card(component);
    daginf_t* daginf = card_get_info(card);
    int dagfd = card_get_fd(card);
    int rxstreams;
    int txstreams;
    int total;
    int used = 0;
    int streammax;
    int retval = 0;
    pbm_offsets_t *PBM = NULL;

    PBM = dagutil_get_pbm_offsets();
    if( NULL == PBM )
    {
        dagutil_panic("Unable to get PBM offset in %s\n",__FUNCTION__);
        return ;
    }
    total = daginf->buf_size / 1024 / 1024;
    rxstreams = dag_rx_get_stream_count(dagfd);
    txstreams = dag_tx_get_stream_count(dagfd);
    streammax = dagutil_max(rxstreams*2 - 1, txstreams*2);

    if (card_lock_all_streams(card))
    {
        retval = EACCES;
    }
    else
    {
        int rxsize = (total - txstreams * PBM_TX_DEFAULT_SIZE) / rxstreams;
        int stream;
        uint32_t pbm_base = card_get_register_address(card, DAG_REG_PBM, 0);
        int pbm_ver = card_get_register_version(card, DAG_REG_PBM, 0);
        
        for (stream = 0; stream < streammax; stream++)
        {
            int val = 0;
            
            if (0 == (stream & 1))
            {
                if (stream < rxstreams*2)
                {
                    val = rxsize;
                }
            }
            else
            {
                /* transmit stream */
                if (stream - 1 < txstreams * 2)
                {
                    val = PBM_TX_DEFAULT_SIZE;
                }
            }
            
            /* assignment checks out, set up pbm stream. */
            card_write_iom(card, pbm_base + PBM[pbm_ver].streambase +
                    stream*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_addr,
                    daginf->phy_addr + used*1024*1024);
                    
            card_write_iom(card, pbm_base + PBM[pbm_ver].streambase +
                    stream*PBM[pbm_ver].streamsize + PBM[pbm_ver].mem_size,
                    val*1024*1024);
                    
            /* update for next loop */
            used += val;
        }
    }

    card_unlock_all_streams(card);

}

/* buffer size */
AttributePtr
pbm_get_new_buffer_size_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeBufferSize); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pbm_buffer_size_dispose);
        attribute_set_post_initialize_routine(result, pbm_buffer_size_post_initialize);
        attribute_set_getvalue_routine(result, pbm_buffer_size_get_value);
        attribute_set_name(result, "buffer_size");
        attribute_set_description(result, "The size of the buffer allocated to the dag card.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
pbm_buffer_size_dispose(AttributePtr attribute)
{

}

static void*
pbm_buffer_size_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    uint32_t val = 0;
    daginf_t* inf = NULL;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        inf = card_get_info(card);
		val = inf->buf_size;
        val /= 1024*1024;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);

    }
    return NULL;
}

static void
pbm_buffer_size_post_initialize(AttributePtr attribute)
{

}

/* rx_stream_count */
AttributePtr
pbm_get_new_rx_stream_count_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeRxStreamCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pbm_rx_stream_count_dispose);
        attribute_set_post_initialize_routine(result, pbm_rx_stream_count_post_initialize);
        attribute_set_getvalue_routine(result, pbm_rx_stream_count_get_value);
        attribute_set_name(result, "rx_stream_count");
        attribute_set_description(result, "The number of receive streams.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
pbm_rx_stream_count_dispose(AttributePtr attribute)
{

}

static void*
pbm_rx_stream_count_get_value(AttributePtr attribute)
{
    uint32_t val = 0;
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);

        card = attribute_get_card(attribute);
		val = card_get_rx_stream_count(card);
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
pbm_rx_stream_count_post_initialize(AttributePtr attribute)
{

}

/* tx_stream count */
AttributePtr
pbm_get_new_tx_stream_count_attribute(void)
{
    AttributePtr result = attribute_init(kUint32AttributeTxStreamCount); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pbm_tx_stream_count_dispose);
        attribute_set_post_initialize_routine(result, pbm_tx_stream_count_post_initialize);
        attribute_set_getvalue_routine(result, pbm_tx_stream_count_get_value);
        attribute_set_name(result, "tx_stream_count");
        attribute_set_description(result, "The number of transmit streams");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_to_string_routine(result, attribute_uint32_to_string);
    }
    
    return result;
}

static void
pbm_tx_stream_count_dispose(AttributePtr attribute)
{

}

static void*
pbm_tx_stream_count_get_value(AttributePtr attribute)
{
    uint32_t val = 0;
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        card = attribute_get_card(attribute);
		val = card_get_tx_stream_count(card);
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
pbm_tx_stream_count_post_initialize(AttributePtr attribute)
{

}

AttributePtr
pbm_get_new_drop_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeDrop); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pbm_drop_dispose);
        attribute_set_post_initialize_routine(result, pbm_drop_post_initialize);
        attribute_set_getvalue_routine(result, pbm_drop_get_value);
        attribute_set_setvalue_routine(result, pbm_drop_set_value);
        attribute_set_name(result, "drop");
        attribute_set_description(result, "If on dropping of packets occurs at the individual stream that has"
                                  " filled up. If off dropping occurs at the gpp.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
pbm_drop_dispose(AttributePtr attribute)
{

}


static void*
pbm_drop_get_value(AttributePtr attribute)
{
    uint8_t value = 0;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr component = NULL;
        pbm_state_t* state = NULL;
        
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (pbm_state_t*)component_get_private_state(component);
        value = (card_read_iom(card, state->mPbmBase) & BIT15) >> 15;
    }
    attribute_set_value_array(attribute, &value, sizeof(value));
    return (void *)attribute_get_value_array(attribute);

}

static void
pbm_drop_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr component = NULL;
        pbm_state_t* state = NULL;
        uint32_t regval = 0;

        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (pbm_state_t*)component_get_private_state(component);
        if (*(uint8_t*)value == 0)
        {
            regval = card_read_iom(card, state->mPbmBase);
            regval &= ~BIT15;
            card_write_iom(card, state->mPbmBase, regval);
        }
        else
        {
            regval = card_read_iom(card, state->mPbmBase);
            regval |= BIT15;
            card_write_iom(card, state->mPbmBase, regval);
        }
    }
}

static void
pbm_drop_post_initialize(AttributePtr attribute)
{

}

AttributePtr
pbm_get_new_overlap_attribute(void)
{
    AttributePtr result = attribute_init(kBooleanAttributeOverlap); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, pbm_overlap_dispose);
        attribute_set_post_initialize_routine(result, pbm_overlap_post_initialize);
        attribute_set_getvalue_routine(result, pbm_overlap_get_value);
        attribute_set_setvalue_routine(result, pbm_overlap_set_value);
        attribute_set_name(result, "overlap");
        attribute_set_description(result, "Share the memory hole between the receive and transmit streams.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
    }
    
    return result;
}

static void
pbm_overlap_dispose(AttributePtr attribute)
{

}


static void*
pbm_overlap_get_value(AttributePtr attribute)
{
    uint8_t overlap = 0;
    DagCardPtr card = attribute_get_card(attribute);

    card_pbm_is_overlapped(card, &overlap);
    attribute_set_value_array(attribute, &overlap, sizeof(overlap));
    return (void *)attribute_get_value_array(attribute);
}

static void
pbm_overlap_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        daginf_t* daginf = NULL;

        card = attribute_get_card(attribute);
        daginf = card_get_info(card);
        
        if (0 == *(uint8_t*)value)
        {
            card_default(card);
        }
        else
        {
            card_pbm_config_overlap(card);
        }
    }
}

static void
pbm_overlap_post_initialize(AttributePtr attribute)
{

}

uint32_t
pbm_get_version(ComponentPtr pbm)
{
    pbm_state_t* state;
    
    state = component_get_private_state(pbm);
    return state->mPbmVer;
}

uint32_t
pbm_get_global_status_register(ComponentPtr pbm)
{
    pbm_state_t* state = NULL;
    DagCardPtr card = NULL;
    pbm_offsets_t *PBM = NULL;

    PBM = dagutil_get_pbm_offsets();
    if( NULL == PBM )
    {
        dagutil_panic("Unable to get PBM offset in %s\n",__FUNCTION__);
        return 1;
    }
    card = component_get_card(pbm);
    state = component_get_private_state(pbm);
    return card_read_iom(card, state->mPbmBase + PBM[state->mPbmVer].globalstatus);
}

void* pci_bus_type_get_value(AttributePtr attribute)
{
    pci_bus_type_t bus_type = kBusTypeUnknown;
    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = NULL;
        ComponentPtr component = NULL;
        AttributePtr attr_pci_speed = NULL;
        pci_bus_speed_t *ptr_speed = NULL;
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        attr_pci_speed = component_get_attribute(component, kUint32AttributePCIBusSpeed);
        ptr_speed = (pci_bus_speed_t *) pbm_pci_bus_speed_get_value (attr_pci_speed);
        if( NULL != ptr_speed  )
        {
            switch (*ptr_speed)
            {
                case kPCIBusSpeed33Mhz:
                case kPCIBusSpeed66Mhz:
                    bus_type = kBusTypePCI;
                    break;
                case kPCIBusSpeed100Mhz:
                case kPCIBusSpeed133Mhz:
                    bus_type = kBusTypePCIX;
                    break;
                case kPCIEBusSpeed2Gbs:
                case kPCIEBusSpeed4Gbs:
                case kPCIEBusSpeed8Gbs:
                case kPCIEBusSpeed16Gbs:
                    bus_type  = kBusTypePCIE;
                    break;
                default:
                /* case kPCIBusSpeedUnknown: case kPCIBusSpeedUnstable: etc TODO verify*/
                    bus_type = kBusTypeUnknown;
            }
        }
        attribute_set_value_array(attribute, &bus_type, sizeof(bus_type));
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
void pci_bus_type_to_string_routine(AttributePtr attribute)
{
   void* temp = attribute_get_value(attribute);
   const char* string = NULL;
   pci_bus_type_t bus_type;
   if (temp)
   {
       bus_type = *(pci_bus_type_t*)temp;
       string = pci_bus_type_to_string(bus_type);
       if (string)
           attribute_set_to_string(attribute, string);
  }
}

