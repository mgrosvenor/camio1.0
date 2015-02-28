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
#include "include/attribute.h"

/* Public API headers. */

/* Internal project headers. */
#include "include/card_types.h"
#include "include/util/utility.h"
#include "include/modules/latch.h"
#include "include/modules/generic_read_write.h"
#include "include/attribute_factory.h"
#include "include/components/mini_mac.h"
#include "include/components/mini_mac_statistics_component.h"

/* C Standard Library headers. */
#include <assert.h>
#if defined (__linux__)
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dagutil.h"

/* Chosen randomly. */
#define ATTRIBUTE_MAGIC 0x8780c540

#define BUFFER_SIZE 1024

/* Software implementation of an attribute. */
struct AttributeObject
{
    /* Standard protection. */
    uint32_t mMagicNumber;
    uint32_t mChecksum;

    /* Members. */
    void* mCachedValue;  /* Storage allocated by each instance as appropriate.  Can be used as a cache for the value on the card, or any other purpose. */
    void* mDefaultValue; /* Storage allocated by each instance as appropriate. */

/* Everything from here on is checksummed. */
    ComponentPtr mComponent; /* The Component with which this attribute is associated. */
    DagCardPtr mCard;        /* The Card with which this attribute is associated.
                              * It's cached here to save calling component_get_card(attribute_get_component(attribute)).
                              */
    dag_attr_t mValuetype;
    void* mValueArray;
    uint32_t mValueCount; /* number of elemenents in the value array */
    int mValueArraySize; /* allocated array size in bytes */
    dag_attribute_code_t mAttributeCode;
    void* mPrivateState;
    char* mName;
    char* mDescription;
    char* mToString;
    dag_attr_config_status_t mConfigStatus;
    uint32_t mDefaultVal; /* Default value */   

    uint32_t* mMasks; /*mask to apply on value returned from reading/writing*/
    /* Used to read/write values to the card (register). Hides the details of reading/writing and allows for multiple ways to do this. */
    GenericReadWritePtr mGenericReadWrite;
    LatchPtr mLatch;
    
    /* Methods. */
    AttributeDisposeRoutine fDispose;
    AttributePostInitializeRoutine fPostInitialize;    
    AttributeGetValueRoutine fGetValue;
    AttributeSetValueRoutine fSetValue;
    AttributeToStringRoutine fToString; /* return a string representation of the attribute's value. */
    AttributeFromStringRoutine fFromString; /* Set the attribute value using a string representation.
                                            Useful for enumerated types when the user wants to set the value via pretty printing. */
};


/* Internal routines. */
static uint32_t compute_attribute_checksum(AttributePtr attribute);
    /* Default method implementations. */
static void generic_attribute_dispose(AttributePtr attribute);
static void generic_attribute_post_initialize(AttributePtr attribute);
static void* generic_attribute_get_value(AttributePtr attribute);
static void generic_attribute_set_value(AttributePtr attribute, void* value, int length);

/* Implementation of internal routines. */
static uint32_t
compute_attribute_checksum(AttributePtr attribute)
{
    /* Don't check validity here with valid_attribute() because the attribute may not be fully constructed. */
    if (attribute != NULL)
    {
        return compute_checksum(&attribute->mComponent, sizeof(*attribute) - 2 * sizeof(uint32_t) - 2 * sizeof(void*));
    }
    return 0;
}


static void
generic_attribute_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
    }
}


static void
generic_attribute_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented. */
    }
}


static void*
generic_attribute_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {

        /* This is to check if the default routine didn't get set when creating the attribute*/
        /* e.g. if Boolean attribute should use attribute_boolean_get_value */
        /* Easiest to put sefety net here than to go through all attributes on all cards to verify */
        switch(attribute_get_valuetype(attribute)) {
            case kAttributeBoolean:
                return attribute_boolean_get_value(attribute);
            case kAttributeInt32:
                return attribute_int32_get_value(attribute);
            case kAttributeUint32:
                return attribute_uint32_get_value(attribute);
            case kAttributeUint64:
                return attribute_uint64_get_value(attribute);
            default:
                break;
        }

    }
    
    return NULL;
}


static void
generic_attribute_set_value(AttributePtr attribute, void* value, int length)
{
    assert(NULL != value);
    assert(0 != length);
    
    if (1 == valid_attribute(attribute))
    {
        /* This is to check if the default routine didn't get set when creating the attribute*/
        /* e.g. if Boolean attribute should use attribute_boolean_set_value */
        /* Easiest to put sefety net here than to go through all attributes on all cards to verify */
        switch(attribute_get_valuetype(attribute)) {
            case kAttributeBoolean:
                attribute_boolean_set_value(attribute, value, length);
                break;
            case kAttributeInt32:
                attribute_int32_set_value(attribute, value, length);
                break;
            case kAttributeUint32:
                attribute_uint32_set_value(attribute, value, length);
                break;
            case kAttributeUint64:      
                attribute_uint64_set_value(attribute, value, length);
                break;
            default:
                break;
        } 
    
    }
}

void
generic_attribute_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* This is to check if the default routine didn't get set when creating the attribute*/
        /* e.g. if Boolean attribute should use attribute_boolean_to_string */
        /* Easiest to put sefety net here than to go through all attributes on all cards to verify */
        switch(attribute_get_valuetype(attribute)) {
            case kAttributeBoolean:
                attribute_boolean_to_string(attribute);
                break;
            case kAttributeInt32:
                attribute_int32_to_string(attribute);
                break;
            case kAttributeUint32:
                attribute_uint32_to_string(attribute);
                break;
            case kAttributeUint64:
                attribute_uint64_to_string(attribute);
                break;
            case kAttributeFloat:
                attribute_float_to_string(attribute);
                break;
            default:
                attribute_set_to_string(attribute, "to string conversion not implemented for this attribute");
                break;
        }
    }
}

void
generic_attribute_from_string(AttributePtr attribute, const char * string)
{
    if (1 == valid_attribute(attribute))
    {
        /* This is to check if the default routine didn't get set when creating the attribute*/
        /* e.g. if Boolean attribute should use attribute_boolean_from_string */
        /* Easiest to put sefety net here than to go through all attributes on all cards to verify */

        switch(attribute_get_valuetype(attribute)) {
            case kAttributeBoolean:
                attribute_boolean_from_string(attribute,string);
                break;
            case kAttributeInt32:
                attribute_int32_from_string(attribute, string);
                break;
            case kAttributeUint32:
                attribute_uint32_from_string(attribute, string);
                break;
            case kAttributeUint64:
                attribute_uint64_from_string(attribute, string);
                break;
            case kAttributeFloat:
                attribute_float_from_string(attribute, string);
                break;
            default:
                /* Unimplemented for this attribute */
                break;
        }

    }
}

/* Factory routine. */
AttributePtr
attribute_init(dag_attribute_code_t attribute_code)
{
    AttributePtr result;

    assert(kFirstAttributeCode <= attribute_code);
    assert(attribute_code <= kLastAttributeCode);
    
    result = malloc(sizeof(*result));
    if (NULL != result)
    {
        memset(result, 0, sizeof(*result));
        
        result->mAttributeCode = attribute_code;
        result->fDispose = generic_attribute_dispose;
        result->fPostInitialize = generic_attribute_post_initialize;
        result->fGetValue = generic_attribute_get_value;
        result->fSetValue = generic_attribute_set_value;
        result->fToString = generic_attribute_to_string;
        result->fFromString = generic_attribute_from_string;
        result->mMasks = (uint32_t*)malloc(sizeof(uint32_t));
        assert(result->mMasks);
        result->mMasks[0] = 0xffffffff;
        result->mValueArraySize = -1;
        /* Compute checksum over the object. */
        result->mChecksum = compute_attribute_checksum(result);
        result->mMagicNumber = ATTRIBUTE_MAGIC;
        attribute_set_name(result, "<unnnamed>");
        attribute_set_description(result, "<undescribed>");
        
        (void) valid_attribute(result);
    }

    return result;
}


/* Virtual method implementations. */
void
attribute_dispose(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->fDispose(attribute);
        free(attribute->mName);
        attribute->mName = NULL;
        free(attribute->mDescription);
        attribute->mDescription = NULL;
        free(attribute->mValueArray);
        attribute->mValueArray = NULL;
        attribute->mValueArraySize = -1;
        free(attribute->mToString);
        attribute->mToString = NULL;
        if (attribute->mPrivateState)
            free(attribute->mPrivateState);
        if (attribute->mGenericReadWrite)
        {
            grw_dispose(attribute->mGenericReadWrite);
        }
        if (attribute->mValueArray)
            free(attribute->mValueArray);
        attribute->mValueArray = NULL;
        attribute->mValueArraySize = -1;
        if (attribute->mLatch)
        {
            latch_dispose(attribute->mLatch);
            free(attribute->mLatch);
        }
        if(attribute->mMasks)
            free(attribute->mMasks);

        memset(attribute, 0, sizeof(*attribute));
        free(attribute);
    }
}


void
attribute_post_initialize(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->fPostInitialize(attribute);
    }
}


void*
attribute_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        if (NULL != attribute->fGetValue)
        {
            return attribute->fGetValue(attribute);
        }
    }
    
    return NULL;
}


void
attribute_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        if (NULL != attribute->fSetValue)
        {
            attribute->fSetValue(attribute, value, length);
        }
    }
}


/* Non-virtual method implementations. */
DagCardPtr
attribute_get_card(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        assert(NULL != attribute->mCard);
        
        return attribute->mCard;
    }

    return NULL;
}


ComponentPtr
attribute_get_component(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        assert(NULL != attribute->mComponent);
        
        return attribute->mComponent;
    }

    return NULL;
}


void
attribute_set_component_and_card(AttributePtr attribute, ComponentPtr component, DagCardPtr card)
{
    assert(NULL != component);
    assert(NULL != card);
    
    if (1 == valid_attribute(attribute))
    {
        assert(NULL == attribute->mCard); /* Card should only be set once. */
        assert(NULL == attribute->mComponent); /* Component should only be set once. */
    
        attribute->mCard = card;
        attribute->mComponent = component;
        
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}


void*
attribute_get_private_state(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mPrivateState;
    }
    
    return NULL;
}


void
attribute_set_private_state(AttributePtr attribute, void* state)
{
    if (1 == valid_attribute(attribute))
    {
        /*assert(NULL == attribute->mPrivateState);*/ /* Possible memory leak if not disposed. */
        
        attribute->mPrivateState = state;
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}


dag_attribute_code_t
attribute_get_attribute_code(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mAttributeCode;
    }

    return kAttributeInvalid;
}


void
attribute_set_default(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented. */
    }
}


void*
attribute_get_default_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mDefaultValue;
    }
    
    return NULL;
}


void
attribute_set_default_value(AttributePtr attribute, void* value)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->mDefaultValue = value;
    }
}


const char*
attribute_get_name(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mName;
    }

    return NULL;
}


void
attribute_set_name(AttributePtr attribute, const char* name)
{
    if (1 == valid_attribute(attribute))
    {
        if (attribute->mName)
        {
            free(attribute->mName);
        }
        attribute->mName = (char*)malloc(strlen(name) + 1);
        strcpy(attribute->mName, name);
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}


void
attribute_set_description(AttributePtr attribute, const char* description)
{
    if (1 == valid_attribute(attribute))
    {
        if (attribute->mDescription)
        {
            free(attribute->mDescription);
        }
        attribute->mDescription = (char*)malloc(strlen(description) + 1);
        strcpy(attribute->mDescription, description);
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}


const char*
attribute_get_description(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mDescription;
    }

    return NULL;
}


int
attribute_get_writable(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        /* FIXME: unimplemented. */
    }

    return 0;
}

void
attribute_set_dispose_routine(AttributePtr attribute, AttributeDisposeRoutine routine)
{
    assert(NULL != routine);
    
    if (1 == valid_attribute(attribute))
    {
        attribute->fDispose = routine;
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}


void
attribute_set_post_initialize_routine(AttributePtr attribute, AttributePostInitializeRoutine routine)
{
    assert(NULL != routine);
    
    if (1 == valid_attribute(attribute))
    {
        attribute->fPostInitialize = routine;
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}

void
attribute_set_from_string_routine(AttributePtr attribute, AttributeFromStringRoutine routine)
{
    assert(NULL != routine);
    if (1 == valid_attribute(attribute))
    {
        attribute->fFromString = routine;
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}

void
attribute_set_to_string_routine(AttributePtr attribute, AttributeToStringRoutine routine)
{
    assert(NULL != routine);
    if (1 == valid_attribute(attribute))
    {
        attribute->fToString = routine;
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}

void
attribute_set_getvalue_routine(AttributePtr attribute, AttributeGetValueRoutine routine)
{
    assert(NULL != routine);
    
    if (1 == valid_attribute(attribute))
    {
        attribute->fGetValue = routine;
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}


void
attribute_set_setvalue_routine(AttributePtr attribute, AttributeSetValueRoutine routine)
{
    assert(NULL != routine);
    
    if (1 == valid_attribute(attribute))
    {
        attribute->fSetValue = routine;
        attribute->mChecksum = compute_attribute_checksum(attribute);
        
        /* Make sure nothing has broken. */
        (void) valid_attribute(attribute);
    }
}


void*
attribute_get_cached_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mCachedValue;
    }
    
    return NULL;
}


void
attribute_set_cached_value(AttributePtr attribute, void* value)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->mCachedValue = value;
        
        /* Checksum update is unnecessary since mCachedValue is not included. */
    }
}


/* Verification routine. */
int
valid_attribute(AttributePtr attribute)
{
    uint32_t checksum;

    /* Someone might be querying the component to see if this attribute exists. */
    /* Returning 0 here will cause kNullAttributeUuid to be returned further up */
    if(attribute == NULL)
        return 0;
    
    assert(NULL != attribute);
    assert(ATTRIBUTE_MAGIC == attribute->mMagicNumber);
    
    checksum = compute_attribute_checksum(attribute);
    assert(checksum == attribute->mChecksum);

    assert(NULL != attribute->fDispose);
    assert(NULL != attribute->fPostInitialize);
    assert(NULL != attribute->fGetValue);
    assert(NULL != attribute->fSetValue);
    
    if ((NULL != attribute) && (ATTRIBUTE_MAGIC == attribute->mMagicNumber) && (checksum == attribute->mChecksum))
    {
        return 1;
    }
    
    return 0;
}

void
attribute_set_config_status(AttributePtr attribute, dag_attr_config_status_t code)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->mConfigStatus = code;
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

dag_attr_config_status_t
attribute_get_config_status(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mConfigStatus;
    }
    return kDagAttrErr;
}

int
attribute_get_value_array_count(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mValueCount;
    }
    return 0;
}

void
attribute_set_value_array(AttributePtr attribute, void* array, int bytes)
{
    if (1 == valid_attribute(attribute))
    {
        assert(bytes > -1);
        
        if (attribute->mValueArray && attribute->mValueArraySize < bytes)
        {
            free(attribute->mValueArray);
            attribute->mValueArray = NULL;
            attribute->mValueArraySize = -1;
        }

        if (!attribute->mValueArray) {
	    attribute->mValueArray = malloc(bytes);
        attribute->mValueArraySize = bytes;
        }
        /* do a copy */
        memcpy(attribute->mValueArray, array, bytes);
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

void
attribute_set_value_array_count(AttributePtr attribute, int count)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->mValueCount = count;
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

void*
attribute_get_value_array(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mValueArray;
    }
    return NULL;
}

void
attribute_set_valuetype(AttributePtr attribute, dag_attr_t type)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->mValuetype = type;
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

/* This function returns the type of the value of the attribute
*/
const char *
attribute_get_valuetype_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
		switch(attribute->mValuetype){
			case kAttributeTypeInvalid:	return "kAttributeTypeInvalid"; 
			case kAttributeBoolean:		return "kAttributeBoolean"; 
			case kAttributeChar:		return "kAttributeChar"; 
			case kAttributeString:		return "kAttributeString"; 
			case kAttributeInt32:		return "kAttributeInt32"; 
			case kAttributeUint32:		return "kAttributeUint32"; 
			case kAttributeInt64:		return "kAttributeInt64"; 
			case kAttributeUint64:		return "kAttributeUint64"; 
			case kAttributeStruct:		return "kAttributeStruct"; 
			case kAttributeNull:		return "kAttributeNull"; 
			case kAttributeFloat:           return "kAttributeFloat";
			default:  return "ValueType Error"; 
		}
		
    }
    return "Attribute Error";
}
/* This function returns the type of the (attribute configuration or status)*/
const char *
attribute_get_type_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
		switch(attribute->mConfigStatus){
				case kDagAttrErr:		return "kDagAttrErr"; 
				case kDagAttrStatus:	return "status"; 
				case kDagAttrConfig:	return "config"; 
				case kDagAttrAll:		return "kDagAttrAll"; 
			
				default:				return "AttributeType Error";
		}

    }
    return "Attribute Error";
}

dag_attr_t
attribute_get_valuetype(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mValuetype;
    }
    return kAttributeTypeInvalid;
}

void
attribute_set_from_string(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute) && string != NULL)
    {
	    attribute->fFromString(attribute, string);
    }
}


const char*
attribute_get_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        if (attribute->fToString)
        {
            attribute->fToString(attribute);
            return attribute->mToString;
        }
    }
    return NULL;
}

void
attribute_set_to_string(AttributePtr attribute, const char* value)
{
    assert(value != NULL);
    if (1 == valid_attribute(attribute) && value != NULL)
    {
        if (attribute->mToString)
            free(attribute->mToString);

	attribute->mToString = (char*)malloc(strlen(value) + 1);
        strcpy(attribute->mToString, value);
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

void
attribute_boolean_to_string(AttributePtr attribute)
{
    uint8_t value; 
    uint8_t *value_ptr = (uint8_t*)attribute->fGetValue(attribute); 
    char buffer[BUFFER_SIZE];

    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeBoolean);
        if( value_ptr == NULL )
        {
            strcpy(buffer,"error");
            attribute_set_to_string(attribute, buffer); 
            return;
        }
        value = *value_ptr;
        if (value == 1)
        {
            strcpy(buffer, "on");
        }
        else if (value == 0)
        {
            strcpy(buffer, "off");
        }
        attribute_set_to_string(attribute, buffer); 
    }
}
/* function is similar to attribute_boolean_to_string 
    But prints 0/1 instead of on/off 
*/ 

void attribute_boolean_to_int_string(AttributePtr attribute)
{
    uint8_t value; 
    uint8_t *value_ptr = (uint8_t*)attribute->fGetValue(attribute); 
    char buffer[BUFFER_SIZE];

    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeBoolean);
        if( value_ptr == NULL )
        {
            strcpy(buffer,"error");
            attribute_set_to_string(attribute, buffer); 
            return;
        }
        value = *value_ptr;
        sprintf(buffer,"%u",value);
        attribute_set_to_string(attribute, buffer); 
    }
}

void
attribute_int32_to_string(AttributePtr attribute)
{
    char buffer[BUFFER_SIZE];
    int32_t value;
    int32_t* value_ptr = (int32_t*)attribute->fGetValue(attribute);
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeInt32);
        if ( value_ptr != NULL )
        {
            value = *value_ptr;
    	    sprintf(buffer, "%d", value);
        } else {
            strcpy(buffer,"error");
	} 
        attribute_set_to_string(attribute, buffer);
    }
}

void
attribute_uint32_to_string(AttributePtr attribute)
{
    char buffer[BUFFER_SIZE];
    uint32_t value;
    uint32_t *value_ptr = (uint32_t*)attribute->fGetValue(attribute);
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeUint32);
        if( value_ptr != NULL )
        {
            value = *value_ptr;
            sprintf(buffer, "%u", value);
        } else {
            strcpy(buffer,"error");
	} 
        attribute_set_to_string(attribute, buffer);
    }
}

void
attribute_uint32_to_hex_string(AttributePtr attribute)
{
    char buffer[BUFFER_SIZE];
    uint32_t value;
    uint32_t *value_ptr = (uint32_t*)attribute->fGetValue(attribute);
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeUint32);
        if( value_ptr != NULL )
        {
            value = *value_ptr;
            sprintf(buffer, "0x%x", value);
        } else {
            strcpy(buffer,"error");
	} 
        attribute_set_to_string(attribute, buffer);
    }
}

void
attribute_uint64_to_string(AttributePtr attribute)
{
    char buffer[BUFFER_SIZE];
    uint64_t value;
    uint64_t *value_ptr =  (uint64_t*)attribute->fGetValue(attribute);
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeUint64);
        if ( value_ptr)
        {
            value = *value_ptr;
            sprintf(buffer, "%"PRIu64, value);
        } else {
            strcpy(buffer,"error");
	}
        attribute_set_to_string(attribute, buffer);
    }
}

void
attribute_float_to_string(AttributePtr attribute)
{
    char buffer[BUFFER_SIZE];
    if (1 == valid_attribute(attribute))
    {
        float value = *(float*)attribute->fGetValue(attribute);
        assert(attribute->mValuetype == kAttributeFloat);
        sprintf(buffer, "%f", value);
        attribute_set_to_string(attribute, buffer);
    }
}
void
attribute_boolean_from_string(AttributePtr attribute, const char* string)
{
    uint8_t value;

    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeBoolean);
        if (strcmp(string, "on") == 0 || strcmp(string, "yes") == 0 || strcmp(string, "1") == 0 )
        {
            value = 1;
        }
        else if (strcmp(string, "off") == 0 || strcmp(string, "no") == 0 || strcmp(string, "0") == 0 )
        {
            value = 0;
        }
        attribute->fSetValue(attribute, (void*)&value, sizeof(uint8_t));
    }
}
void
attribute_int32_from_string(AttributePtr attribute, const char* string)
{
    int32_t value;
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeInt32);
        sscanf(string, "%u", &value);
        attribute->fSetValue(attribute, (void*)&value, sizeof(int32_t));
    }
}

void
attribute_uint32_from_string(AttributePtr attribute, const char* string)
{
    uint32_t value;
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeUint32);
        sscanf(string, "%u", &value);
        attribute->fSetValue(attribute, (void*)&value, sizeof(uint32_t));
    }
}

void
attribute_uint32_from_hex_string(AttributePtr attribute, const char* string)
{
    uint32_t value;
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeUint32);
        sscanf(string, "%x", &value);
        attribute->fSetValue(attribute, (void*)&value, sizeof(uint32_t));
    }
}

void
attribute_uint64_from_string(AttributePtr attribute, const char* string)
{
    uint64_t value;
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeUint64);
        sscanf(string, "%"PRIu64, &value);
        attribute->fSetValue(attribute, (void*)&value, sizeof(uint64_t));
    }
}

void
attribute_float_from_string(AttributePtr attribute, const char* string)
{
    float value;
    if (1 == valid_attribute(attribute))
    {
        assert(attribute->mValuetype == kAttributeFloat);
        sscanf(string, "%f", &value);
        attribute->fSetValue(attribute, (void*)&value, sizeof(float));
    }
}

void 
attribute_set_generic_read_write_object(AttributePtr attribute, GenericReadWritePtr grw)
{
    if (1 == valid_attribute(attribute))
    {
        if (attribute->mGenericReadWrite)
        {
            grw_dispose(attribute->mGenericReadWrite);
        }
        attribute->mGenericReadWrite = grw;
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

void 
attribute_set_masks(AttributePtr attribute, const uint32_t* masks, int len)
{
    if (1 == valid_attribute(attribute))
    {
        if (attribute->mMasks)
        {
            free(attribute->mMasks);
        }
        attribute->mMasks = (uint32_t*)malloc(len * sizeof(uint32_t));
        memcpy(attribute->mMasks, masks, len * sizeof(uint32_t));
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

uint32_t*
attribute_get_masks(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mMasks;
    }
    return NULL;
}


GenericReadWritePtr
attribute_get_generic_read_write_object(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mGenericReadWrite;
    }
    return NULL;
}
    
void*
attribute_boolean_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t reg_val = 0;
        uint8_t value = 0;
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        reg_val = grw_read(grw);
        if (attribute->mMasks)
        {
            if (grw_get_on_operation(grw) == kGRWSetBit)
            {
                if ((reg_val & attribute->mMasks[0]) == attribute->mMasks[0])
                {
                    value = 1;
                    attribute_set_value_array(attribute, &value, sizeof(value));
                }
                else
                {
                    value = 0;
                    attribute_set_value_array(attribute, &value, sizeof(value));
                }
            }
            else if (grw_get_on_operation(grw) == kGRWClearBit)
            {
                if ((reg_val & attribute->mMasks[0]) == attribute->mMasks[0])
                {
                    value = 0;
                    attribute_set_value_array(attribute, &value, sizeof(value));
                }
                else
                {
                    value = 1;
                    attribute_set_value_array(attribute, &value, sizeof(value));
                }
            }
           dagutil_verbose_level(4, "%s is %d\n", attribute_get_name(attribute), ((uint8_t*)attribute->mValueArray)[0]);
            return (attribute->mValueArray);
        }
    }
    return NULL;
}

void
attribute_boolean_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        assert(grw);
        reg_val = grw_read(grw);
        if (*(uint8_t*)value == 1)
        {
           dagutil_verbose_level(4, "Setting %s to 1\n", attribute_get_name(attribute));
            if (grw_get_on_operation(grw) == kGRWClearBit)
            {
                reg_val &= ~attribute->mMasks[0];
            }
            else if (grw_get_on_operation(grw) == kGRWSetBit)
            {
                reg_val |= attribute->mMasks[0];
            }
            grw_write(grw, reg_val);
        }
        else if (*(uint8_t*)value == 0)
        {
           dagutil_verbose_level(4, "Setting %s to 0\n", attribute_get_name(attribute));
            if (grw_get_on_operation(grw) == kGRWClearBit)
            {
                reg_val |= attribute->mMasks[0];
            }
            else if (grw_get_on_operation(grw) == kGRWSetBit)
            {
                reg_val &= ~attribute->mMasks[0];
            }
            grw_write(grw, reg_val);
        }
    }
}

/* this function clears the register on setting the bit. so whatever previous bits were set will
 * in the register will nolonger be there.*/
void
attribute_boolean_set_value_with_clear(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        assert(grw);
        reg_val = grw_read(grw);
        if (*(uint8_t*)value == 1)
        {
           dagutil_verbose_level(4, "Setting %s to 1\n", attribute_get_name(attribute));
            reg_val = 0;
            reg_val |= attribute->mMasks[0];
            grw_write(grw, reg_val);
        }
        else if (*(uint8_t*)value == 0)
        {
           dagutil_verbose_level(4, "Setting %s to 0\n", attribute_get_name(attribute));
            reg_val = 0;
            grw_write(grw, reg_val);
        }
    }
}

/* Specialized for setting the TCP flow reset bit.*/
void
attribute_boolean_set_reset_tcp_flow(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        assert(grw);
        reg_val = grw_read(grw);
        if (*(uint8_t*)value == 1)
        {
           dagutil_verbose_level(4, "Setting %s to 1\n", attribute_get_name(attribute));
            /*preserve all but the lower 4 bits*/
            reg_val &= 0xfffffff0;
            reg_val |= attribute->mMasks[0];
            grw_write(grw, reg_val);
        }
        else if (*(uint8_t*)value == 0)
        {
           dagutil_verbose_level(4, "Setting %s to 0\n", attribute_get_name(attribute));
            reg_val &= 0xfffffff0;
            reg_val &= ~attribute->mMasks[0];
            grw_write(grw, reg_val);
        }
    }
}

void*
attribute_int32_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        int32_t reg_val = 0;
        int32_t mask = attribute->mMasks[0];
        reg_val = grw_read(attribute_get_generic_read_write_object(attribute));
        reg_val &= attribute->mMasks[0];
        while ((mask & BIT0) == 0)
        {
            reg_val = reg_val >> 1;
            mask = mask >> 1;
        }
        attribute_set_value_array(attribute, &reg_val, sizeof(reg_val));
        return (attribute->mValueArray);
    }
    return NULL;
}

void
attribute_int32_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute) && value)
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        int32_t reg_val = 0;
        int32_t temp = 0;
        int32_t mask = attribute->mMasks[0];
        temp = *(int32_t*)value;
        while ((mask & BIT0) == 0)
        {
            mask = mask >> 1;
            temp = temp << 1;
        }
        /* get register value */
        reg_val = grw_read(grw);
        /* clear the specific bits of reg_val */
        reg_val &= ~(attribute->mMasks[0]);
        /* set reg_val to the right value */
        reg_val |= temp;
        /* write the value in the register */
        grw_write(grw, reg_val);
    }
}


void*
attribute_uint32_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t reg_val = 0;
        uint32_t mask = attribute->mMasks[0];
        reg_val = grw_read(attribute_get_generic_read_write_object(attribute));
        reg_val &= attribute->mMasks[0];
        while ((mask & BIT0) == 0)
        {
            reg_val = reg_val >> 1;
            mask = mask >> 1;
        }
        attribute_set_value_array(attribute, &reg_val, sizeof(reg_val));
        return (attribute->mValueArray);
    }
    return NULL;
}

void
attribute_uint32_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute) && value)
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uint32_t temp = 0;
        uint32_t mask = attribute->mMasks[0];
        temp = *(uint32_t*)value;
        while ((mask & BIT0) == 0)
        {
            mask = mask >> 1;
            temp = temp << 1;
        }
        /* get register value */
        reg_val = grw_read(grw);
        /* clear the specific bits of reg_val */
        reg_val &= ~(attribute->mMasks[0]);
        /* set reg_val to the right value */
        reg_val |= temp;
        /* write the value in the register */
        grw_write(grw, reg_val);
    }
}

void*
attribute_uint64_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t reg_val = 0;
        uint64_t val = 0;
        uintptr_t address = 0;
        GenericReadWritePtr grw = NULL;
        
        /* This function assumes that the hi 32 bits are at the 1st address  and the lo 32 bits are at the 1st+1 */
        grw  = attribute_get_generic_read_write_object(attribute);
        reg_val = grw_read(grw);
        val |= (((uint64_t)reg_val) << 32);
        address = grw_get_address(grw);
        grw_set_address(grw, address + 4);
        reg_val = grw_read(grw);
        val |= reg_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        /* reset the address otherwise the next time this is called the address will be wrong */
        grw_set_address(grw, address);
        return attribute->mValueArray;
    }
    return NULL;
}

void
attribute_uint64_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uintptr_t address = grw_get_address(grw);
        uint64_t temp = *(uint64_t*)value;

        reg_val = (uint32_t)(temp & 0xffffffff);
        grw_write(grw, reg_val);
        
        reg_val = (uint32_t)(temp >> 32);
        grw_set_address(grw, address + 4);
        grw_write(grw, reg_val);

        grw_set_address(grw, address);
    }
}


void*
attribute_uint64_get_value_reverse(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t reg_val = 0;
        uint64_t val = 0;
        uintptr_t address = 0;
        GenericReadWritePtr grw = NULL;
        /* This function assumes that the hi 32 bits are at the  address  and the lo 32 bits are at the address - 4 */
        grw  = attribute_get_generic_read_write_object(attribute);
        reg_val = grw_read(grw);
        val |= (((uint64_t)reg_val) << 32);
        address = grw_get_address(grw);
        grw_set_address(grw, address - 4);
        reg_val = grw_read(grw);
        val |= reg_val;
        attribute_set_value_array(attribute, &val, sizeof(val));
        /* reset the address otherwise the next time this is called the address will be wrong */
        grw_set_address(grw, address);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

void
attribute_uint64_set_value_reverse(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uintptr_t address = grw_get_address(grw);
        uint64_t temp = *(uint64_t*)value;

        /* This function assumes that the hi 32 bits are at the  address  and the lo 32 bits are at the address - 4 */
        reg_val = (uint32_t)(temp >> 32);
        grw_write(grw, reg_val);


        reg_val = (uint32_t)(temp & 0xffffffff);
        grw_set_address(grw, address - 4 );
        grw_write(grw, reg_val);
        
        grw_set_address(grw, address);
    }
    return;
}

void
attribute_set_latch_object(AttributePtr attribute, LatchPtr latch)
{
    if (1 == valid_attribute(attribute))
    {
        attribute->mLatch = latch;
        attribute->mChecksum = compute_attribute_checksum(attribute);
    }
}

LatchPtr
attribute_get_latch_object(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        return attribute->mLatch;
    }
    return NULL;
}

bool
is_sfp_fast_enough(GenericReadWritePtr grw)
{
    uint32_t value = grw_read(grw);
    dagutil_verbose_level(4, "SFP Module Bit Rate: %d Mbps\n", value*100);
    if (value * 100 > 1000)
    {
        return true;
    }
    return false;
}

bool
sfp_detect(AttributePtr attribute)
{
    ComponentPtr component = attribute_get_component(attribute);
    AttributePtr detect = component_get_attribute(component, kBooleanAttributeSFPDetect);
    uint8_t* value;
    value = (uint8_t*)attribute_get_value(detect);
    if (value)
        return (*value == 1);
    return false;
}

void
attribute_rocket_io_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        DagCardPtr card = attribute_get_card(attribute);
        uint32_t address = 0;
        ComponentPtr mini_mac = attribute_get_component(attribute);
        int index = mini_mac_get_index(mini_mac);
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        
        /*address has to be packed to contain the the minimac index, device address and the device register*/
        address = 0xa0 << 8;
        address |= 0xc;
        address |= (index << 16);
        sfp_grw = grw_init(card, address, grw_mini_mac_raw_smbus_read, grw_mini_mac_raw_smbus_write);

        assert(grw);
        assert(sfp_grw);
        reg_val = grw_read(grw);
        if (*(uint8_t*)value == 1)
        {
            reg_val |= attribute->mMasks[0];
            if (sfp_detect(attribute) && is_sfp_fast_enough(sfp_grw))
            {
                dagutil_verbose_level(4, "Setting Rocket IO Power\n");
                grw_write(grw, reg_val);
            }
        }
        else if (*(uint8_t*)value == 0)
        {
            reg_val &= ~attribute->mMasks[0];
            grw_write(grw, reg_val);
        }
        grw_dispose(sfp_grw);
    }
}
void 
attribute_refresh_cache_set_value(AttributePtr attribute, void * value, int len)
{
    if(1 == valid_attribute(attribute))
    {
        ComponentPtr mini_mac_stat = attribute_get_component(attribute);
        mini_mac_statistics_state_t * state = component_get_private_state(mini_mac_stat);
        GenericReadWritePtr rlc_grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        assert(rlc_grw);
        reg_val = grw_read(rlc_grw);
        state->mExtCache = reg_val;
    }
}
void*
attribute_refresh_cache_get_value(AttributePtr attribute)
{
    uint8_t value = 0;

    /* get value is not used */
    /* place holder to prevent clearing the register on read */
    if (1 == valid_attribute(attribute))
    {
        attribute_set_value_array(attribute, &value, sizeof(value));
        return  (attribute->mValueArray);
    }
    return NULL;
}

    
void*
attribute_cache_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        ComponentPtr mini_mac_stat = attribute_get_component(attribute);
        mini_mac_statistics_state_t * state = component_get_private_state(mini_mac_stat);
        uint32_t reg_val = state->mExtCache;
        uint8_t value = 0;
        if (attribute->mMasks)
        {
            if ((reg_val & attribute->mMasks[0]) == attribute->mMasks[0])
            {
                value = 1;
                attribute_set_value_array(attribute, &value, sizeof(value));
            }
            else
            {
                value = 0;
                attribute_set_value_array(attribute, &value, sizeof(value));
            }
            return (attribute->mValueArray);
        }
    }
    return NULL;
}
void*
attribute_steer_get_value(AttributePtr attribute)
{
    uint32_t register_value;
    static uint32_t value = 0;
    GenericReadWritePtr steer_grw = NULL;

    if (1 == valid_attribute(attribute))
    {
        steer_grw = attribute_get_generic_read_write_object(attribute);
        assert(steer_grw);
        register_value = grw_read(steer_grw);
        register_value = ((register_value >> 12) & 0x03);
        switch(register_value)
        {
            case 0x00:
                value = kSteerStream0;
                break;
            case 0x01:
                value = kSteerParity;
                break;
            case 0x02:
                value = kSteerCrc;
                break;
            case 0x03:
                value = kSteerIface;
                break;
        }
        return (void *)&value;
     }
    return NULL;
}

void
attribute_steer_set_value(AttributePtr attribute, void* value, int length)
{
    uint32_t register_value = 0;
    GenericReadWritePtr steer_grw = NULL;
    steer_t steer_mode = *(steer_t*)value;

    if (1 == valid_attribute(attribute))
    {
        steer_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(steer_grw);
        register_value &= ~(0x03 << 12);
        switch(steer_mode)
        {
            case kSteerStream0:
                /* nothing */
                break;
			case kSteerColour:
            case kSteerParity:
                register_value |= (0x01 << 12);
                break;
            case kSteerCrc:
                register_value |= (0x02 << 12);
                break;
            case kSteerIface:
                register_value |= (0x03 << 12);
                break;
        }
        grw_write(steer_grw, register_value);
    }
}
void*
attribute_mac_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        static char mac_buf[256];
        uint32_t reg_val = 0;
        uint64_t val = 0;
        uint32_t address = 0;
        GenericReadWritePtr grw = NULL;

        /* This function assumes that the hi 32 bits are at the 1st address  and the lo 32 bits are at the 1st+1 */
        grw  = attribute_get_generic_read_write_object(attribute);
        reg_val = grw_read(grw);
        val |= reg_val;
        address = grw_get_address(grw);
        grw_set_address(grw, address + 4);
        reg_val = grw_read(grw);
        val |= (((uint64_t)reg_val) << 32);
        /* reset the address otherwise the next time this is called the address will be wrong */
        grw_set_address(grw, address);

        memset(mac_buf, 0, 256);
        sprintf(mac_buf, "%02x:%02x:%02x:%02x:%02x:%02x",
                (uint32_t)(val>>40)&0xff,
                (uint32_t)(val>>32)&0xff,
                (uint32_t)(val>>24)&0xff,
                (uint32_t)(val>>16)&0xff,
                (uint32_t)(val>>8)&0xff,
                (uint32_t)val&0xff);

        return (void *)&mac_buf;

     }
    return NULL;
}

void
attribute_snaplen_set_value(AttributePtr attribute, void* value, int len)
{
    if (1 == valid_attribute(attribute) && value)
    {
        ComponentPtr comp = attribute_get_component(attribute);
        GenericReadWritePtr grw = attribute_get_generic_read_write_object(attribute);
        uint32_t reg_val = 0;
        uint32_t mask = attribute->mMasks[0];
        reg_val = *(uint32_t*)value;
        while ((mask & BIT0) == 0)
        {
            mask = mask >> 1;
            reg_val = reg_val << 1;
        }
        grw_write(grw, reg_val);

        /* Perform an L2 reset */
        component_reset(comp);

    }
}


