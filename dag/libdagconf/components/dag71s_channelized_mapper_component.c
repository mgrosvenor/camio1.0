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
#include "../include/components/dag71s_channelized_mapper_component.h"
#include "../include/components/dag71s_connection_component.h"
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/util/logger.h"
#include "../include/util/enum_string_table.h"
#include "dagapi.h"

/* DAG API files we rely on being installed in a sys include path somewhere */
#include "dagutil.h"

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

#define MAX_CONNECTIONS 2048
#define MAX_POSSIBLE_CONNECTIONS 672
#define BUFFER_SIZE 1024
#define CONNECTION_NUMBER_MASK 0x000007FF

typedef struct
{
    uint32_t mBase;
    ComponentPtr mConnection[2048];
    uint8_t mIsIndexUsed[MAX_POSSIBLE_CONNECTIONS];

} atm_hdlc_mapper_state_t;

enum
{
    kTimeSlot = 0x8,
    kConnectionConfig = 0x4
};

static int mapper_get_next_free_connection_index(ComponentPtr component);
static void mapper_restore_used_connections_list(ComponentPtr component, DagCardPtr card);
//Because we can't read from this component we have to maintain an
//array of the connections configured
static connection_description_t mapper_connection_description[MAX_CONNECTIONS];
//static int mapper_cd_index = 0;
static uint32_t last_connection_num;

/* demapper component. */
static void dag71s_channelized_mapper_dispose(ComponentPtr component);
static void dag71s_channelized_mapper_reset(ComponentPtr component);
static void dag71s_channelized_mapper_default(ComponentPtr component);
static int dag71s_channelized_mapper_post_initialize(ComponentPtr component);
static dag_err_t dag71s_channelized_mapper_update_register_base(ComponentPtr component);

static AttributePtr dag71s_channelized_mapper_get_new_revision_id(void);
static void dag71s_channelized_mapper_revision_id_dispose(AttributePtr attribute);
static void* dag71s_channelized_mapper_revision_id_get_value(AttributePtr attribute);
static void dag71s_channelized_mapper_revision_id_post_initialize(AttributePtr attribute);
static void dag71s_channelized_mapper_revision_id_to_string_routine(AttributePtr attribute);

static AttributePtr dag71s_channelized_mapper_get_new_add_connection(void);
static void dag71s_channelized_mapper_add_connection_dispose(AttributePtr attribute);
static void dag71s_channelized_mapper_add_connection_set_value(AttributePtr attribute, void* value, int length);
static void* dag71s_channelized_mapper_add_connection_get_value(AttributePtr attribute);

static AttributePtr dag71s_channelized_mapper_get_new_get_connection_number(void);
static void dag71s_channelized_mapper_get_connection_number_dispose(AttributePtr attribute);
static void* dag71s_channelized_mapper_get_connection_number_get_value(AttributePtr attribute);

static AttributePtr dag71s_channelized_mapper_get_new_delete_connection(void);
static void dag71s_channelized_mapper_delete_connection_dispose(AttributePtr attribute);
static void dag71s_channelized_mapper_delete_connection_set_value(AttributePtr attribute, void* value, int length);
static void* dag71s_channelized_mapper_delete_connection_get_value(AttributePtr attribute);

static AttributePtr dag71s_channelized_mapper_get_new_clear_connections(void);
static void dag71s_channelized_mapper_clear_connections_dispose(AttributePtr attribute);
static void dag71s_channelized_mapper_clear_connections_set_value(AttributePtr attribute, void* value, int length);
static void* dag71s_channelized_mapper_clear_connections_get_value(AttributePtr attribute);

static AttributePtr dag71s_channelized_mapper_get_new_restore_connections(void);
static void dag71s_channelized_mapper_restore_connections_dispose(AttributePtr attribute);
static void dag71s_channelized_mapper_restore_connections_set_value(AttributePtr attribute, void* value, int length);
static void* dag71s_channelized_mapper_restore_connections_get_value(AttributePtr attribute);

static void* sonet_type_get_value(AttributePtr attribute);
static void sonet_type_post_initialize(AttributePtr attribute);
static AttributePtr get_new_sonet_type_attribute(void);
static void sonet_type_dispose(AttributePtr attribute);
static void sonet_type_to_string_routine(AttributePtr attribute);


ComponentPtr
dag71s_get_new_mapper(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentMapper, card); 
    atm_hdlc_mapper_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, dag71s_channelized_mapper_dispose);
        component_set_post_initialize_routine(result, dag71s_channelized_mapper_post_initialize);
        component_set_reset_routine(result, dag71s_channelized_mapper_reset);
        component_set_default_routine(result, dag71s_channelized_mapper_default);
        component_set_update_register_base_routine(result, dag71s_channelized_mapper_update_register_base);
        component_set_name(result, "atm_hdlc_mapper");
        memset(mapper_connection_description, 0, sizeof(mapper_connection_description));
        state = (atm_hdlc_mapper_state_t*)malloc(sizeof(atm_hdlc_mapper_state_t));
        component_set_private_state(result, state);
    }
    
    return result;

}

static void
dag71s_channelized_mapper_dispose(ComponentPtr component)
{
}

static void
dag71s_channelized_mapper_reset(ComponentPtr component)
{
}

static void
dag71s_channelized_mapper_default(ComponentPtr component)
{
}

static int
dag71s_channelized_mapper_post_initialize(ComponentPtr component)
{
    int connection_index;
    AttributePtr revision_id;
    AttributePtr add_connection;
    AttributePtr get_connection_number;
    AttributePtr clear_connections;
    AttributePtr sonet_type;
    AttributePtr delete_connection;
    AttributePtr restore_connections ;
    atm_hdlc_mapper_state_t* state = NULL;
    DagCardPtr card;

    state = component_get_private_state(component);
    card = component_get_card(component);
    state->mBase = card_get_register_address(card, DAG_REG_E1T1_HDLC_MAP, 0);
    assert(state->mBase != 0);
    revision_id = dag71s_channelized_mapper_get_new_revision_id();
    add_connection = dag71s_channelized_mapper_get_new_add_connection();
    get_connection_number = dag71s_channelized_mapper_get_new_get_connection_number();
    clear_connections = dag71s_channelized_mapper_get_new_clear_connections();
    sonet_type = get_new_sonet_type_attribute();
    delete_connection = dag71s_channelized_mapper_get_new_delete_connection();
    restore_connections = dag71s_channelized_mapper_get_new_restore_connections();
    
    component_add_attribute(component, revision_id);
    component_add_attribute(component, add_connection);
    component_add_attribute(component, get_connection_number);
    component_add_attribute(component, clear_connections);
    component_add_attribute(component, restore_connections);
    component_add_attribute(component, sonet_type);
    component_add_attribute(component, delete_connection);

    
      /* Initialize the used connection list as al false (0)*/
    memset(state->mIsIndexUsed, 0, MAX_POSSIBLE_CONNECTIONS);
  
    for(connection_index = 0; connection_index < 2048; connection_index++)
    {
 	    state->mConnection[connection_index] = dag71s_get_new_connection(card, connection_index);
    	component_add_subcomponent(component, state->mConnection[connection_index]); 
        /*for each subcomponent set the parent correctly */
        component_set_parent(state->mConnection[connection_index], component);
        
        	

    }

    return 1;
}

static dag_err_t
dag71s_channelized_mapper_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card;
        atm_hdlc_mapper_state_t* state;
        
        card = component_get_card(component);
        state = component_get_private_state(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_E1T1_HDLC_MAP, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static AttributePtr
dag71s_channelized_mapper_get_new_revision_id(void)
{
    AttributePtr result = attribute_init(kUint32Attribute71sChannelizedRevisionID); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_channelized_mapper_revision_id_dispose);
        attribute_set_post_initialize_routine(result, dag71s_channelized_mapper_revision_id_post_initialize);
        attribute_set_getvalue_routine(result, dag71s_channelized_mapper_revision_id_get_value);
        attribute_set_to_string_routine(result, dag71s_channelized_mapper_revision_id_to_string_routine);
        attribute_set_name(result, "revision_id");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag71s_channelized_mapper_revision_id_dispose(AttributePtr attribute)
{
}

static void*
dag71s_channelized_mapper_revision_id_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr mapper;
    uint32_t register_value;
    static dag71s_channelized_rev_id_t value;
    atm_hdlc_mapper_state_t* state;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        mapper = attribute_get_component(attribute);
        state = component_get_private_state(mapper);
        register_value = card_read_iom(card, state->mBase);
        switch (register_value & 0xf)
        {
            case 0:
                value = kDag71sRevIdATM;
                break;

            case 1:
                value = kDag71sRevIdATMHDLC;
                break;

            case 2:
                value = kDag71sRevIdATMHDLCRAW;
                break;

            case 3:
                value = kDag71sRevIdHDLC;
                break;

            case 4:
                value = kDag71sRevIdHDLCRAW;
                break;
        }
        return (void*)&value;
    }
    return NULL;
}

static void
dag71s_channelized_mapper_revision_id_post_initialize(AttributePtr attribute)
{
}

static void
dag71s_channelized_mapper_revision_id_to_string_routine(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        dag71s_channelized_rev_id_t id = *(dag71s_channelized_rev_id_t*)attribute_get_value(attribute);
        const char* temp = dag71s_channelized_rev_id_to_string(id);
        attribute_set_to_string(attribute, temp);
    }
}

static AttributePtr
dag71s_channelized_mapper_get_new_add_connection(void)
{
    AttributePtr result = attribute_init(kStructAttributeAddConnection); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_channelized_mapper_add_connection_dispose);
        attribute_set_setvalue_routine(result, dag71s_channelized_mapper_add_connection_set_value);
        attribute_set_getvalue_routine(result, dag71s_channelized_mapper_add_connection_get_value);
        attribute_set_name(result, "add_connection");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeStruct);
        return result;
    }
    return NULL;
}

static void
dag71s_channelized_mapper_add_connection_dispose(AttributePtr attribute)
{
}

static void
dag71s_channelized_mapper_add_connection_set_value(AttributePtr attribute, void* value, int length)
{
    uint32_t temp = 0;
    connection_description_t* cd = (connection_description_t*)value;
    DagCardPtr card;
    atm_hdlc_mapper_state_t* state = NULL;
    ComponentPtr component;
    ComponentPtr root_component;
    AttributePtr tributary_unit;
    ComponentPtr sonic;
    uint32_t reg_value;
    uint32_t payloadType;
    int mapper_cd_index = -1;

    if (1 == valid_attribute(attribute))
    { 
       card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        root_component = card_get_root_component(card);
        sonic = component_get_subcomponent(root_component, kComponentSonic, cd->mPortNumber);
        assert(sonic);
        tributary_unit = component_get_attribute(sonic, kUint32AttributeTributaryUnit);
        if (kTU12 == *(tributary_unit_t*)attribute_get_value(tributary_unit))
        {
            /* doing e1 */
            temp |= cd->mTU_ID;
            temp |= cd->mTUG2_ID << 2;
            temp |= cd->mVC_ID << 5;
            temp |= cd->mTUG3_ID << 7;
        } 
        else if (kTU11 == *(tributary_unit_t*)attribute_get_value(tributary_unit))
        {
            /* doing t1 */
            temp |= cd->mTU_ID;
            temp |= cd->mTUG2_ID << 2;
            temp |= cd->mVC_ID << 5;
        }

        cd->mConnectionNumber = temp | (cd->mPortNumber << 9);
        // make sure connection number is only 11 bits otherwise indexes can be outside array
        cd->mConnectionNumber &= CONNECTION_NUMBER_MASK;

        // Read the state and set the port and connection number
        reg_value = card_read_iom(card, state->mBase);
        dagutil_microsleep(1000);
        reg_value &= ~0x07FF0000;
        reg_value = (((uint32_t)cd->mPortNumber << 25) | ((uint32_t)temp << 16));
        card_write_iom(card, state->mBase, reg_value);

        // Wait for the ready indicator
        do {
            dagutil_microsleep(1000);
        } while ( !(card_read_iom(card, state->mBase) & BIT4) );


        // Write the timeslot information
        reg_value = cd->mTimeslotMask;
        card_write_iom(card, state->mBase + kTimeSlot, reg_value);
        dagutil_microsleep(1000);


        // Map the payload types to their actual bit values
        switch (cd->mPayloadType)
        {
            case kPayloadTypeNotConfigured:  payloadType = 0x00;   break;
            case kPayloadTypeATM:            payloadType = 0x01;   break;
            case kPayloadTypeHDLC:           payloadType = 0x02;   break;
//note originally this was ERFtype 8 
//           case kPayloadTypeRAW:            payloadType = 0x04;   break;
//now is changed for ERF type 6 both raws temp fix in the head maybe we need a new constant /
//case kPayloadTypeRAWsync 
            case kPayloadTypeRAW:            payloadType = 0x06;   break;
            default:                         payloadType = 0x00;   break;
				
        }
        /* check for the connection number is already configured*/
        if ( kConnectionTypeNotConfigured == mapper_connection_description[cd->mConnectionNumber].mConnectionType)
        {
            /* get a new index */
            mapper_cd_index = mapper_get_next_free_connection_index(component);
        }
        else 
        {
            /* use the already used index */
            mapper_cd_index = mapper_connection_description[cd->mConnectionNumber].mTableIndex;
        }
	cd->mTableIndex = mapper_cd_index;

        if ( -1 == mapper_cd_index )
        {
             card_set_last_error(card, kDagErrGeneral);
             return;
        }
        // Construct the configuration for this channel
        reg_value  = cd->mConnectionType;
        reg_value |= (payloadType << 4);
        reg_value |= (cd->mScramble << 8);
        reg_value |= (mapper_cd_index << 12);
        reg_value |= BIT31;


        // Write the channel information
        dagutil_verbose("Writing 0x%08X [%s] to the atm/hdlc connection configuration register, with connection number 0x%0X.\n", reg_value, logger_make_bitstring(reg_value), temp);
        dagutil_microsleep(1000);
        card_write_iom(card, state->mBase + kConnectionConfig, reg_value);


        // Wait for the ready indicator
        do {
            dagutil_microsleep(1000);
        } while ( !(card_read_iom(card, state->mBase) & BIT4) );


        dagutil_microsleep(1000);


        if (mapper_cd_index < MAX_POSSIBLE_CONNECTIONS)
        {
            //write this connection to the array of connections
            mapper_connection_description[cd->mConnectionNumber] = *cd;
            last_connection_num = cd->mConnectionNumber;
            state->mIsIndexUsed[mapper_cd_index] = 1;
            //mapper_cd_index++;
        }
    }
}

static AttributePtr
dag71s_channelized_mapper_get_new_get_connection_number(void)
{
    AttributePtr result = attribute_init(kUint32AttributeGetLastConnectionNumber); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_channelized_mapper_get_connection_number_dispose);
        attribute_set_getvalue_routine(result, dag71s_channelized_mapper_get_connection_number_get_value);
        attribute_set_name(result, "get_connection_number");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag71s_channelized_mapper_get_connection_number_dispose(AttributePtr attribute)
{
}

static void*
dag71s_channelized_mapper_get_connection_number_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    { 
        return (void*)&last_connection_num;
    }
    return NULL;
}

static AttributePtr
dag71s_channelized_mapper_get_new_clear_connections(void)
{
    AttributePtr result = attribute_init(kNullAttributeClearConnections); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_channelized_mapper_clear_connections_dispose);
        attribute_set_setvalue_routine(result, dag71s_channelized_mapper_clear_connections_set_value);
        attribute_set_getvalue_routine(result, dag71s_channelized_mapper_clear_connections_get_value);
        attribute_set_name(result, "clear_connections");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag71s_channelized_mapper_clear_connections_dispose(AttributePtr attribute)
{
}

static void
dag71s_channelized_mapper_clear_connections_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr component;
    atm_hdlc_mapper_state_t* state = NULL;

    dagutil_verbose("Clearing connections\n");
    if (1 == valid_attribute(attribute))
    {
        uint32_t conn;
        uint32_t port;
        uint32_t tug3;
        uint32_t vc;
        uint32_t tug2;
        uint32_t tu;
        uint32_t reg_value;

        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);



        // Iterate over all possible combinations of channels and set the fields to zero 
        for (port=0; port<4; port++)
            for (tug3=0; tug3<3; tug3++)
                for (vc=0; vc<4; vc++)
                    for (tug2=0; tug2<7; tug2++)
                        for (tu=0; tu<4; tu++)
                        {
                             conn = (tug3 << 7) | (vc << 5) | (tug2 << 2) | (tu);

                             // Read the state and set the port and connection number
                             reg_value = card_read_iom(card, state->mBase);
                             reg_value &= ~0x07FF0000;
                             reg_value |= (((uint32_t)port << 25) | ((uint32_t)conn << 16));
                             dagutil_microsleep(1000);
                             card_write_iom(card, state->mBase, reg_value);

                             // Wait for the ready indicator
                             do {
                                  dagutil_microsleep(1000);
                             } while ( !(card_read_iom(card, state->mBase) & BIT4) );


                             // Read the current configuration for this channel
                             dagutil_microsleep(1000);
                             reg_value = card_read_iom(card, state->mBase + kConnectionConfig);
                             dagutil_microsleep(1000);

                             // Finally clear the value
                             reg_value &= ~0x77;
                             reg_value |= BIT31;
                             dagutil_microsleep(1000);
                             card_write_iom(card, state->mBase + kConnectionConfig, reg_value);

                             // Wait for the ready indicator
                             do {
                                  dagutil_microsleep(1000);
                             } while ( !(card_read_iom(card, state->mBase) & BIT4) );

                             dagutil_microsleep(1000);
                        }

        //mapper_cd_index = 0;
        memset(state->mIsIndexUsed, 0, MAX_POSSIBLE_CONNECTIONS);
        memset(mapper_connection_description, 0, sizeof(mapper_connection_description));
    }
}

static AttributePtr
dag71s_channelized_mapper_get_new_delete_connection(void)
{
    AttributePtr result = attribute_init(kUint32AttributeDeleteConnection); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_channelized_mapper_delete_connection_dispose);
        attribute_set_setvalue_routine(result, dag71s_channelized_mapper_delete_connection_set_value);
        attribute_set_getvalue_routine(result, dag71s_channelized_mapper_delete_connection_get_value);
        attribute_set_name(result, "delete_connection");
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeUint32);
        return result;
    }
    return NULL;
}

static void
dag71s_channelized_mapper_delete_connection_dispose(AttributePtr attribute)
{
}

static void
dag71s_channelized_mapper_delete_connection_set_value(AttributePtr attribute, void* value, int length)
{
    uint32_t connection_num = *(uint32_t*)value;
    uint32_t reg_value;
    DagCardPtr card;
    ComponentPtr component;
    atm_hdlc_mapper_state_t* state = NULL;
    connection_description_t *cd;

    if (1 == valid_attribute(attribute))
    {
        cd = &mapper_connection_description[connection_num];
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);

        // Read the state and set the port and connection number
        reg_value = card_read_iom(card, state->mBase);
        reg_value &= ~0x07FF0000;
        reg_value |= ((uint32_t)connection_num << 16);
        dagutil_microsleep(1000);
        card_write_iom(card, state->mBase, reg_value);

        // Wait for the ready indicator
        do {
            dagutil_microsleep(1000);
        } while ( !(card_read_iom(card, state->mBase) & BIT4) );


        // Read the current configuration for this channel
        dagutil_microsleep(1000);
        reg_value = card_read_iom(card, state->mBase + kConnectionConfig);
        dagutil_microsleep(1000);

        // Finally clear the value
        reg_value &= ~0x77;
        reg_value |= BIT31;
        dagutil_microsleep(1000);
        card_write_iom(card, state->mBase + kConnectionConfig, reg_value);

        // Wait for the ready indicator
        do {
            dagutil_microsleep(1000);
        } while ( !(card_read_iom(card, state->mBase) & BIT4) );

        dagutil_microsleep(1000);

        //mapper_cd_index--;
        state->mIsIndexUsed[(reg_value >> 12) & 0x7FF] =  0;
        memset(&mapper_connection_description[connection_num], 0, sizeof(connection_description_t));
	
    }   
    return;
}

static AttributePtr
dag71s_channelized_mapper_get_new_restore_connections(void)
{
    AttributePtr result = attribute_init(kNullAttributeRestoreConnections); 
    
    if (NULL != result)
    {
        attribute_set_dispose_routine(result, dag71s_channelized_mapper_restore_connections_dispose);
        attribute_set_setvalue_routine(result, dag71s_channelized_mapper_restore_connections_set_value);
        attribute_set_getvalue_routine(result, dag71s_channelized_mapper_restore_connections_get_value);
        attribute_set_name(result, "restore_connections");
        attribute_set_description(result, "To restore already created connections");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeNull);
        return result;
    }
    return NULL;
}

static void
dag71s_channelized_mapper_restore_connections_dispose(AttributePtr attribute)
{
}

static void
dag71s_channelized_mapper_restore_connections_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr component;

    dagutil_verbose("Restoring connections\n");
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        mapper_restore_used_connections_list(component, card);
    }
}

static AttributePtr
get_new_sonet_type_attribute(void)
{ 
    AttributePtr result = attribute_init(kUint32AttributeSonetType); 
    
    if (NULL != result)
    {
        attribute_set_getvalue_routine(result, sonet_type_get_value);
        attribute_set_dispose_routine(result, sonet_type_dispose);
        attribute_set_post_initialize_routine(result, sonet_type_post_initialize);
        attribute_set_to_string_routine(result, sonet_type_to_string_routine);
        attribute_set_name(result, "sonet_type");
        attribute_set_description(result, "This component is designed for concatenated or channelized SONET traffic.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void*
sonet_type_get_value(AttributePtr attribute)
{
    static uint32_t return_value = kSonetTypeChannelized;

    if (1 == valid_attribute(attribute))
    {
        return (void*)&return_value;
    }
    return NULL;
}

static void
sonet_type_post_initialize(AttributePtr attribute)
{
}

static void
sonet_type_dispose(AttributePtr attribute)
{
}

static void
sonet_type_to_string_routine(AttributePtr attribute)
{
    sonet_type_t st = *(sonet_type_t*)attribute_get_value(attribute);
    if (st != kSonetTypeInvalid)
    {
      const char* temp = NULL;
      temp = sonet_type_to_string(st);
      attribute_set_to_string(attribute, temp);
    }
}
void mapper_restore_used_connections_list(ComponentPtr component, DagCardPtr card)
{
    uint8_t  * iom;
    uint32_t * iom_demapper1;
    uint32_t * iom_demapper2;
    uint32_t   value;
    dag_reg_t  results[DAG_REG_MAX_ENTRIES];
    int        found;
    unsigned int port;
    unsigned int tug3;
    unsigned int vc;
    unsigned int tug2;
    unsigned int tu;
    uint32_t     conn;
    atm_hdlc_mapper_state_t* state = NULL;
    int dagfd = card_get_fd(card);
    
    state = component_get_private_state(component);
    /* Get a pointer to the register space */
    iom = dag_iom (dagfd);

    /* Find the channelization emuneration entry */
    found = dag_reg_find ((char*)iom, DAG_REG_E1T1_HDLC_MAP, results);
    if ( found == 0 )
    {
            return;
    }

    /* Get pointers to the two registers of interest */
    iom_demapper1 = (uint32_t*)(iom + results[0].addr);
    iom_demapper2 = (uint32_t*)(iom + (results[0].addr + 4));

    /* Loop over all possible channels */
    for (port=0; port<4; port++)
        for (tug3=0; tug3<3; tug3++)
            for (vc=0; vc<4; vc++)
                for (tug2=0; tug2<7; tug2++)
                    for (tu=0; tu<3; tu++)
                                {
                                    conn = (tug3 << 7) | (vc << 5) | (tug2 << 2) |     (tu);
                                    /* set the connection and port number to read */
                                    value  = *iom_demapper1;
                                    value &= ~0x07FF0000;
                                    value |= (((uint32_t)port << 25) | ((uint32_t)conn << 16));
                                    *iom_demapper1 = value;
                
                                    /* wait for the ready indicator */
                                    do {
                                        dagutil_microsleep(1000);   
                                    } while ( (*iom_demapper1 & 0x10) == 0 );
                                   
                                    /* read back */
                                    value = *iom_demapper2;
                                   
                                    conn |= (port << 9);
                                    conn &= CONNECTION_NUMBER_MASK;

                                    if ( value & 0x77)
                                    {
                                        connection_description_t *cd = &mapper_connection_description[conn];
                                        cd->mTUG3_ID = tug3;
                                        cd->mVC_ID = vc;
                                        cd->mTUG2_ID = tug2;
                                        cd->mTU_ID = tu;
                                        cd->mPortNumber = port;
                                        cd->mConnectionType = (connection_type_t) value & 0x07;
                                        cd->mPayloadType = (payload_type_t) (value & 0x70) >> 4 ;
                                        cd->mScramble = ( value & 0x100 )? 1:0;
                                        cd->mHECCorrection = ( value & 0x200 )? 1:0;
                                        cd->mIdleCellMode = ( value & 0x400 )? 1:0;
                                        //cd->mTimeslotMask = ( value & 0x200 )? 1:0;
                                        cd->mConnectionNumber = conn;
                                        cd->mTableIndex = (value >> 12) & 0x000007FF;
                                        state->mIsIndexUsed[(value >> 12) & 0x000007FF] = 1;
                                                        
                                                    }
                                
                    }
    return;
    
}
int mapper_get_next_free_connection_index(ComponentPtr component)
{
    atm_hdlc_mapper_state_t* state = NULL;
    int i = 0;

    state = component_get_private_state(component);
    for( i = 0; i < MAX_POSSIBLE_CONNECTIONS; i++)
    {
        if( ! state->mIsIndexUsed[i] )
            return i;
    }
    return -1;
}

/* dummy get value functions for attributes */
void* dag71s_channelized_mapper_clear_connections_get_value(AttributePtr attribute)
{
    return NULL;
}

void* dag71s_channelized_mapper_restore_connections_get_value(AttributePtr attribute)
{
    return NULL;
}

void* dag71s_channelized_mapper_add_connection_get_value(AttributePtr attribute)
{
    return NULL;
}

void* dag71s_channelized_mapper_delete_connection_get_value(AttributePtr attribute)
{
    return NULL;
}
