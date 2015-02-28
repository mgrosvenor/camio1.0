/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef DAG_CONFIG_H
#define DAG_CONFIG_H

/* C Standard Library headers. */
#include "dag_platform.h"
#include "dagpci.h"
#include "dag_attribute_codes.h"
#include "dag_component_codes.h"

typedef struct dag_card_ dag_card_;
typedef dag_card_* dag_card_ref_t; /* Type-safe opaque reference. */

typedef struct dag_component_ dag_component_;
typedef dag_component_* dag_component_t; /* Type-safe opaque reference. */

/**
\defgroup ConfigAndStatusAPI Configuration and Status API
The Endace Configuration and Status Application Programming Interface (API) enables
developers to configure the varied range of components and associated attributes of a DAG
card.It allows allow third-party developers to perform the following tasks from within their own
application software:

  - Resetting a DAG card.
  - Loading firmware images onto a DAG card.
  - Setting and retrieving the hardware configuration.
  - Retrieving status and statistics information.

Whenever you use the Configuration & Status API you must always include the following
header files:
 - dag_config.h: Contains routines that relate to the card as a whole 
	         e.g. getting an initial reference to the card, loading firmware, 
	         finding a component by name, as well as routines that
	         retrieve and set values on attributes.
 - dag_component.h: Contains routines that operate on components, e.g. getting 
		    the root component, getting subcomponents, getting attributes 
		    of a component.
 - dag_component_codes.h: Contains the codes.
 - dag_attribute_codes.h: Contains the codes

Alternatively you may use the files dag_config_api.h. This is provided simply for
convenience as its only function is to include the four essential files listed above.
*/
/*@{*/

/**
 * \ingroup AttributeAccessors
 * Systems that don't have uintptr_t will need to define
 * attr_uuid_t as an unsigned long
 */
typedef uintptr_t attr_uuid_t; 

/**
 * \ingroup CSMarco
 * Buffer size for error messages returned by dag_config_strerror(). 
 */
#define DAG_STRERROR_BUFLEN 64


/**
 * \ingroup ErrorAccessor
 * Return Error codes 
 */
typedef enum
{
    /**
     * No Error
     */
    kDagErrNone = 0,
    /**
     * Invalid card reference. Indicates that something is wrong with the card object.
     */
    kDagErrInvalidCardRef,
    /** 
     * Invalid component. Indicates that something is wrong with the component object.
     */
    kDagErrInvalidComponent,
    /**
     * Invalid parameter
     */
    kDagErrInvalidParameter,
    /**
     * No such component. Indicates that the requested component was no found.
     */
    kDagErrNoSuchComponent,
    /**
     * No such attribute. Indicates that the requested attribute was not found.
     */
    kDagErrNoSuchAttribute,
    /**
     * Firmware verify failed
     */
    kDagErrFirmwareVerifyFailed,
    /**
     * Firmware load failed
     */
    kDagErrFirmwareLoadFailed,
    /** 
     * Attribute is read only 
     */
    kDagErrStatusAttribute,
    /**
    There was a general error related to the SWID (software id).
    */
    kDagErrSWIDError,
    /**
    Invalid number of bytes specified.
    */
    kDagErrSWIDInvalidBytes,
    /**
    Timeout communicating with the XScale. 
    */
    kDagErrSWIDTimeout,
    /**
    The given key is different from the one stored.
    */
    kDagErrSWIDInvalidKey,
    /**
     * Unimplemented function called
     */
    kDagErrUnimplemented,
    /**
     * Generic Error
     */
    kDagErrGeneral,
    /**
     * Card type not supported
     */
    kDagErrCardNotSupported
} dag_err_t;
/**
 * \defgroup CardType Card Type Codes
 * List of all card types
 * */
/*@{*/
typedef enum
{
    /**
    * Unknown Card type
    */  
    kDagUnknown = 0,
    /**
    * Dag 3.5E
    */  
    kDag35e,
    /**
    * Dag 3.5
    */  
    kDag35,
    /**
    * Dag 3.6D
    */  
    kDag36d,
    /**
    * Dag 3.6E
    */  
    kDag36e,
    /**
    * Dag 3.6GE
    */  
    kDag36ge,
    /**
    * Dag 3.7D
    */  
    kDag37d,
    /**
    * Dag 3.7GE
    */  
    kDag37ge,
    /**
    * Dag 3.7T
    */  
    kDag37t,
   /**
    * Dag 3.8
    */  
    kDag38,
    /**
    * Dag 4.2GE
    */  
    kDag42ge,
    /**
    * Dag 4.2.3GE
    */  
    kDag423ge,
    /**
    * Dag 4.2
    */  
    kDag42,
    /**
    * Dag 4.2.3
    */  
    kDag423,
    /**
    * Dag 4.3 GE
    */  
    kDag43ge,
    /**
    * Dag 4.3S
    */  
    kDag43s,
    /**
    * Dag 4.5.2E
    */  
    kDag452e,
    /**
    * Dag 4.5.2.GF
    */  
    kDag452gf,
    /**
    * Dag 4.5.2.CF
    */  
    kDag452cf,
    /**
    *Dag 4.5.4E
    */  
    kDag454e,
    /**
    * Dag 4.5.2Z
    */  
    kDag452z,
    /**
    * Dag 5.0s
    */  
    kDag50s,
    /**
    * Dag 5.2x
    */  
    kDag52x,
    /**
    * Dag 5.2SXA
    */  
    kDag52sxa,
    /**
    * Dag 6.0
    */  
    kDag60,
    /**
    * Dag 6.1
    */  
    kDag61,
    /**
    * Dag 6.2
    */  
    kDag62,
    /**
    * Dag 7.0S
    */  
    kDag70s,
    /**
    * Dag 7.0GE
    */  
    kDag70ge,
    /**
    * Dag 7.1S
    */  
    kDag71s,
    /**
    * Dag 8.2X
    */  
    kDag82x,
    /**
    * Dag 8.2X2
    */  
    kDag82x2,
    /**
    * Dag 8.2Z
    */  
    kDag82z,
    /**
    * Dag 8.0.0
    */  
    kDag800,
    /**
    * Dag 8.1.0
    */  
    kDag810,
    /**
    * Dag 5.0Z
    */  
    kDag50z,
    /**
    * Dag 5.0duplicate
    */  
    kDag50dup,
    /**
    * Dag 5.0SG2A
    */  
    kDag50sg2a,
    /**
    * Dag 5.0SG2Aduplicate
    */  
    kDag50sg2adup,
    /**
    * Dag 4.5.2Z8
    */  
    kDag452z8,
    /**
    * Dag 8.4.0
    */  
    kDag840,
    /**
    * Dag 5.4.S.1.2
    */  
    kDag54s12,
    /**
    * Dag 5.4.SG48
    */  
    kDag54sg48,
    /**
    * Dag 5.0SG3A
    */  
    kDag50sg3a,
    /**
    * Dag 5.4GA
    */  
    kDag54ga,
    /**
    * Dag 5.4SA12
    */  
    kDag54sa12,
    /**
    * Dag 5.4SGA48
    */  
    kDag54sga48,
    /**
    * Dag 7.5g2
    */  
    kDag75g2,
    /**
    * Dag 7.5g4
    */  
    kDag75g4,
    /**
    * Dag 7.5be
    */  
    kDag75be,
    /**
    * Dag 7.5ce
    */  
    kDag75ce,
    /**
    * Dag 7.4s
    */  
    kDag74s,
     /**
    * Dag 7.4s48
    */  
    kDag74s48,
    /**
    * Dag 3.7T4
    */  
    kDag37t4,
    /**
    * Dag 8.3x
    */  
    kDag91x2Rx,
    /**
    * Dag 8.3xe this the dumy second core 
    */  
    kDag91x2Tx,
    /*
     * Dag 8.5
     * */
    kDag850,
    /*
     * Dag 9.2x 
     * */
    kDag92x,   
 
    /**
    * First Dag Card Type
    */  
    kFirstDagCard = kDagUnknown,
    /**
    * Last Dag Card Type
    */  
    kLastDagCard = kDag92x,
} dag_card_t;
/*@}*/

/**
 * \ingroup  AttributeValueAccessors
 * Attributes can store different values depending on their type 
 */
typedef enum
{
    /**
     * Invalid Type Attribute
     */ 

    kAttributeTypeInvalid,
    /**
     * Boolean Attribute Type
     */ 
    kAttributeBoolean,
    /**
     * Char Attribute Type
     */ 
    kAttributeChar,
    /**
     * String Attribute Type
     */ 
    kAttributeString,
    /**
     * Int32 Attribute Type
     */ 
    kAttributeInt32,
    /**
     * Uint32AttributeType
     */ 
    kAttributeUint32,
    /**
     * Int64 Attribute Type
     */ 
    kAttributeInt64,
    /**
     * Uint64 Attribute Type
     */ 
    kAttributeUint64,
    /**
     * Struct Attribute Type
     */ 
    kAttributeStruct,
    /**
     * Null Attribute Type
     */ 
    kAttributeNull,
    /**
     * Float Attribute Type
     */ 
    kAttributeFloat
} dag_attr_t;

/**
 * \defgroup Firmware  Firmware and Card Initialization Functions.
 */
/*@{*/
/**
 * \ingroup Firmware
 * Indicates the firmware type  
 */
typedef enum
{
    kFirmwareNone = 0,
    /**
     * Factory-programmed firmware image
     */
    kFirmwareFactory,
    /** 
     * Alias for kFirmwareFactory for backward compatability
     */
    kFirmwareStable = kFirmwareFactory,
    /**
     * User-programmed firmware image
     */
    kFirmwareUser,
    /**
     * Alias for kFirmwareUser for backward compatability
     */
    kFirmwareCurrent = kFirmwareUser,
     /**
      * firmware user 1 ,2,3 newly added for v2 of rom controller.
      */ 
    kFirmwareUser1,

    kFirmwareUser2,

    kFirmwareUser3,

    kFirstFirmware = kFirmwareFactory,
    
    kLastFirmware = kFirmwareUser3

} dag_firmware_t;

/**
 * \ingroup Firmware
 * Firmware flags
 */
typedef enum
{
    kFirmwareFlagsNone = 0,
    kFirmwareFlagsVerify = 1 << 0
    /* Other flags as necessary to allow dagrom to be written on top of this API. */

} dag_firmware_flags_t;

/**
 * LM63 Specific register
 */
typedef enum
{
	/* From SMB. */
	LM63_SMB_Ctrl = 0x0,
	LM63_SMB_Read = 0x4,
}lm63_smb_t;


/**
 * \ingroup Firmware
 * Indicates the embedded image to load
 */
typedef enum
{

    /**
     * Image for the embedded CPU bootloader
     */
    kCpuRegionBootloader = 1,

    /** 
     * Image for the embedded CPU kernel
     */
    kCpuRegionKernel = 2,
    /**
     * Image for the embedded CPU filesystem
     */
    kCpuRegionFilesystem = 3,
    

} dag_embedded_region_t;
/*@}*/
/** 
* \ingroup UCAPI
* list of the different type of csi blocks 
*/
typedef enum
{
	kIDBlockDebug = 0x0,
	kIDBlockEthFramerRx = 0x11,
	kIDBlockEthFramerTx = 0x12,
	kIDBlockEthFramerRxTx = 0x13,
	kIDBlockSonetFramerRx = 0x21,
	kIDBlockSonetFramerTx = 0x22,
	kIDBlockSonetFramerRxTx = 0x23,
	kIDBlockStreamRx = 0x31,
	kIDBlockStreamTx = 0x32,
	kIDBlockStreamRxTx = 0x33,
	kIDBlockStreamDropRx = 0x41,
	kIDBlockStreamDropTx = 0x42,
	kIDBlockStreamDropRxTx = 0x43,
	kIDBlockDropRx = 0x51,
	kIDBlockDropTx = 0x52,
	kIDBlockDropRxTx = 0x53,
	kIDBlockPortDropRx = 0x61,
	kIDBlockPortDropTx = 0x62,
	kIDBlockPortDropRxTx = 0x63,
	kIDBlockFilterRx = 0x71,
	kIDBlockFilterTx = 0x72,
	kIDBlockFilterRxTx = 0x73,
	kIDBlockPatternRx = 0x81,
	kIDBlockPatternTx = 0x82,
	kIDBlockPatternRxTx = 0x83,
        kIDBlockFrontEndFrequencyReferenceRx = 0xa1,
        kIDBlockFrontEndFrequencyReferenceTx = 0xa2,
	kIDBlockFrontEndFrequencyReferenceRxTx = 0xa3
} dag_block_type_t;

/**\ingroup CSMarco
 */
/*@{*/
#define INDIRECT_ADDR_OFFSET 0x08
#define INDIRECT_DATA_OFFSET 0x0C
/*@}*/

/** 
* \ingroup UCAPI
* list of the different type of ID for counters 
*/
typedef enum
{
	kIDCounterInvalid = 0x0,
	kIDCounterRXFrame = 0x01,
	kIDCounterRXByte= 0x02,
	kIDCounterRXShort = 0x03,
	kIDCounterRXLong = 0x04,
	kIDCounterRXError = 0x05,
	kIDCounterRXFCS = 0x06,
	kIDCounterRXAbort = 0x07,
	kIDCounterTXFrame = 0x08,
	kIDCounterTXByte = 0x09,
	kIDCounterDIP4Error = 0x0A,
	kIDCounterDIP4PlError = 0x0B,
	kIDCounterBurstError = 0x0C,
	kIDCounterPlError = 0x0D,
	kIDCounterDebug = 0x0E,
	kIDCounterFilter = 0x0F,
	kIDCounterB1Error = 0x10,
	kIDCounterB2Error = 0x11,
	kIDCounterB3Error = 0x12,
	kIDCounterRXErr = 0x13,
	kIDCounterSpaceError = 0x14,
	kIDCounterContWdError = 0x15,
	kIDCounterPlContError = 0x16,
	kIDCounterTRDip4Error = 0x17,
	kIDCounterResvWd = 0x18,
	kIDCounterAddrError = 0x19,
	kIDCounterOOFPeriod = 0x1A,
	kIDCounterNbOOF = 0x1B,
	kIDCounterTXOOFPeriod = 0x1C,
	kIDCounterTXNbOOF = 0x1D,
	kIDCounterTXError = 0x1E,
	kIDCounterStatFrError = 0x1F,
	kIDCounterDip2Error = 0x20,
	kIDCounterPatternError = 0x21,
	kIDCounterRXStreamPacket = 0x22,
	kIDCounterRXStreamByte = 0x23,
	kIDCounterTXStreamPacket = 0x24,
	kIDCounterTXStreamByte = 0x25,
	kIDCounterPortDrop = 0x26,
	kIDCounterStreamDrop = 0x27,
	kIDCounterSubStreamDrop = 0x28,
	kIDCounterFilterDrop = 0x29,
	kIDCounterIdleCell = 0x35,
        kIDCounterTxClock = 0x36,
	kIDCounterRxClock = 0x37,
        kIDCounterDuckOverflow = 0x38,
        kIDCounterPhyClockNominal = 0x39
} dag_counter_type_t;

/**
 * \ingroup UCAPI 
 * list of the different type of counter subfunctions 
 */
typedef enum
{
	kIDSubfctPort = 0x00,
	kIDSubfctStream = 0x01,
	kIDSubfctFilter = 0x02,
	kIDSubfctGeneral = 0x03,

} dag_subfct_type_t;

/**
* \ingroup UCAPI
* structure of Direct and Indirect universal counter interface 
*/
typedef struct
{
        dag_counter_type_t typeID;	/**< indicates the type ID */
	int size;			/**< indicates the size of counter */	
	dag_subfct_type_t subfct;	/**< indicates the type of sub-function */
        uint32_t interface_number;	
	int lc;				/**< indicates if there is a latch and clear bit 
					to read the register */
        int value_type; 		/**< indicates whether it is the counter value (0)
					 or the address where the counter value is stored(1).*/ 
	uint64_t value;			/**< indicates the counter value */
	//uint32_t value_lower;
	//uint32_t value_upper;
} dag_counter_value_t;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**
 *General information. 
 */
    /**
     * \defgroup ErrorAccessor	Error Accessor Functions
     */
    /*@{*/
    /**
     * Mapping error codes to human readable strings. 
     * \param error_code error codes
     * \return the human readable strings
     */
    const char* dag_config_strerror(dag_err_t error_code);
    
    /**
     * Retrieve error codes set by routines that don't return dag_err_t. 
     * \param card card reference
     * \return last error code
     */
    dag_err_t dag_config_get_last_error(dag_card_ref_t card);

    /**
     * Set the error code. Normally use it to clear the error code by setting it to kDagErrNone.
     * \param card card reference
     * \return last error code
     */
    dag_err_t dag_config_set_last_error(dag_card_ref_t card, dag_err_t code);
    /*@}*/

/* Configuration routines.
 * Configuration information is: numeric or text; read-only or read-write.
 */  


  /**
 * \defgroup Firmware  Firmware and Card Initialization Functions.
 */
 /*@{*/

    /**
     * Routine to reset the card 
     * \param card_ref card reference
     * \return error code
     */
    dag_err_t dag_config_reset(dag_card_ref_t card_ref);
    /*@}*/
    
    /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    */
    /*@{*/
     /**
     * Get an attribute by its index. This function is useful for
     * looping through the attributes matching the given attribute
     * code. See \ref dag_config_get_attribute_count function above.
     *
     * \param card card reference
     * \param attribute_code attribute to look for
     * \param attr_index index in the list
     * \return attribute's uuid, or kNullAttributeUuid if attribute is not found
     */
    attr_uuid_t dag_config_get_indexed_attribute_uuid(dag_card_ref_t card_ref, dag_attribute_code_t attr_code, int attr_index);
    /*@}*/
     /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    */
    /*@{*/
    /**
     Get a named config by its name
     \param card_ref card reference  
     \param name attribute name
     \return  An identifier for the attribute if found. If the requested attribute cannot be found kNullAttributeUuid is returned.
     */
    attr_uuid_t dag_config_get_named_attribute_uuid(dag_card_ref_t card_ref, const char* name);
    /*@}*/
     /**
    \defgroup ConfigLayerComponentAccessor 
    */
    /*@{*/
    /**
     * Count the component instances containing the attribute. Only
     * components immediately below the root component are counted.
     *
     * \param card card reference
     * \param name  name of attribute to look for
     * \return number of component instances containing the attribute
     */
    int dag_config_get_named_attribute_count(dag_card_ref_t card_ref, const char * name);
    /*@}*/


    /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    */
    /*@{*/
    /**
     * Get an attribute by its index. This function is useful for
     * looping through the attributes matching the given attribute
     * code. See \ref dag_config_get_named_attribute_count function above.
     *
     * \param card card reference
     * \param name attribute to look for
     * \param attr_index index in the list
     * \return attribute's uuid, or kNullAttributeUuid if attribute is not found
     */
    attr_uuid_t dag_config_get_indexed_named_attribute_uuid(dag_card_ref_t card_ref, const char * name, int attr_index);
    /*@}*/
    /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    */
    /*@{*/
    /**
     * Get the name of the attribute 
     * \param uuid atttribute identifier
     * \return attribute name
     */
    const char* dag_config_get_attribute_name(attr_uuid_t uuid);

    /**
     * Get the description of the attribute
     * \param uuid attribute identifier
     * \return attribute description
     */
    const char* dag_config_get_attribute_description(attr_uuid_t uuid);
    
    /**
     * Get the attribute type
     * \param uuid attribute identifier
     * \return attribute type (configuration or status)
     */
    const char* dag_config_get_attribute_type_to_string(attr_uuid_t uuid);
    
    /**
     * Get the attribute value type
     * \param uuid attribute identifier
     * \return attribute value type (which is printed as the enumeration constants and can be boolean, int or other)
     */
    const char* dag_config_get_attribute_valuetype_to_string(attr_uuid_t uuid);
    /**
     * Get the attribute code value enum  as string as used in the config and status api
     * \param uuid attribute identifier
     * \return attribute value code as const string 
     */
    const char* dag_config_get_attribute_code_as_string(attr_uuid_t uuid);
    /**
     * Count the component instances containing the attribute. Only
     * components immediately below the root component are counted.
     *
     * \param card card reference
     * \param attribute_code attribute to look for
     * \return number of component instances containing the attribute
     */
    int dag_config_get_attribute_count(dag_card_ref_t card_ref, dag_attribute_code_t attr_code);
     /*@}*/
    /**
    \defgroup ComponentAcesssor
    */
     /*@{*/
    /**
    \defgroup ConfigLayerComponentAccessor
    */
     /*@{*/
     
     /**
     * Get the component code for the component. See \ref dag_component_t
     * \param component component reference
     * \return component code \ref dag_component_code_t
     */
    dag_component_code_t dag_config_get_component_code(dag_component_t component);    
    /**
     * Get the component code enum in string form for the component. See \ref dag_component_t
     * \param component component reference
     * \return component code enum constant in string form as to be used in programs \ref dag_component_code_t
     */
    const char* dag_config_get_component_code_as_string(dag_component_t component);
    /*@}*/ 
    /*@}*/
    /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    */
    /*@{*/

     /**
     * Returns whether the attribute is a config or status type attribute. See \ref dag_attr_config_status_t
     * \param uuid attribute reference
     * \return \ref dag_attr_config_status_t
     */
    dag_attr_config_status_t dag_config_get_attribute_config_status(attr_uuid_t uuid);
     /*@}*/

    /**
    \defgroup ComponentAcesssor
    */
     /*@{*/
      /**
    \defgroup ConfigLayerComponentAccessor
    */
     /*@{*/

    
     /**
     * Get the name of the component
     * \param ref component reference
     * \return name
     */
     const char* dag_config_get_component_name(dag_component_t ref);
    /**
     * Get the description of the component
     * \param ref component reference
     * \return description
     */
    const char* dag_config_get_component_description(dag_component_t ref);
    /*@}*/ 
    /*@}*/	
     /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    */
    /*@{*/
    /**
     * Get the valuetype of the attribute. See \ref dag_attr_t
     * \param card_ref card reference
     * \param uuid attribute reference
     * \return \ref dag_attr_t
     */
    dag_attr_t dag_config_get_attribute_valuetype(dag_card_ref_t card_ref, attr_uuid_t uuid);

    /**
     * Get the value of the attribute in string format
     * \param card card reference
     * \param uuid attribute reference
     * \return string representation of value
     */
    const char* dag_config_get_attribute_to_string(dag_card_ref_t card, attr_uuid_t uuid);
    /*@}*/

    /**
    \defgroup AttributeMutatorFunctions AttributeMutatorFunctions.
    */
    /*@{*/
    /**
     * Pass in a string representation of the value to be set on the attribute
     * \param card card reference
     * \param uuid attribute reference
     * \param string string value
     */
    void dag_config_set_attribute_from_string(dag_card_ref_t card, attr_uuid_t uuid, const char* string);
    /*@}*/	
    
    /**
     \defgroup CardAccessor Card Accessor functions.
    */
    /*@{*/
    /**
     * Get the type of card . See \ref dag_card_t
     * \param card card reference
     * \return \ref dag_card_t
     */ 
    dag_card_t dag_config_get_card_type(dag_card_ref_t card);
    /**
     * Get the type of card returned as a string. Do not free the pointer returned by this function.
     * \param card card reference
     * \return \ref const char*  pointer to a string
     */
    const char* dag_config_get_card_type_as_string(dag_card_ref_t card);
    /**
     * Get the file descriptor of the card
     * \param card_ref card reference
     * \return file descriptor
     */
    int dag_config_get_card_fd(dag_card_ref_t card_ref);
    /*@}*/
	
     /**
     * \defgroup Firmware  Firmware and Card Initialization Functions.
     */
     /*@{*/
     /**
    For use with a DAG card in the machine. Initializes the DAG card and returns a reference to the card
    for use with other functions in the API. Once finished with the card use \ref dag_config_dispose to deallocate
    memory used by the card reference.
    \param device_name the name of the card. It can be any of the following forms:
    "/dev/dag0", "dag0", "0".
    \return A reference to the dag card. For use with other configuration functions. NULL is returned on failure.
    */
    dag_card_ref_t dag_config_init(const char* device_name);
    /*@}*/
    /**
     * \ingroup ConfigLayerComponentAccessor
     * Get the number of components in the card
     * \param card_ref a reference to the card.
     * \return the number of components in the card.
     */
    int dag_config_get_component_count(dag_card_ref_t card_ref);

    /**
    \ingroup ConfFunc
    Use to set the card to a default state. The default state varies from card to card.
    \param card_ref A reference to the card. Use dag_config_init to retrieve a reference.
    \return The error code kDagErrInvalidCardRef on failure. On success the code kDagErrNone is returned.
    */   
     /**
    \defgroup ConfigLayerComponentAccessor 
    */
    

   /**
    *\defgroup Firmware  Firmware and Card Initialization Functions.
    */
    /*@{*/
    /**
     * calls the default methods of all the components.Used for making the firmware of the card to defult
     * configurations.
     * \param card card_ref
     * \return kDagErrNone if there is no error. or appropriate errorcode if there is error
     */
   
    dag_err_t dag_config_default(dag_card_ref_t card_ref);
    /*@}*/
    /**
     * \defgroup Firmware  Firmware and Card Initialization Functions.
     */
    /*@{*/

    /**
    Once finished with the card reference use this function to cleanup.
    \param card_ref The card to dispose of.
    */
    void dag_config_dispose(dag_card_ref_t card_ref);
    /*@}*/
   
    /* Configuration attribute value accessors and mutators. */

    /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    All these functions have a similar behaviour. They retrieve the value of
    the attribute specified. The only difference is the return type.
    \param card_ref A reference to a card.
    \param uuid The uuid of the attribute.
    \return The value of the attribute.
    */
    /*@{*/
    /**
    \defgroup AttributeValueAccessors Attribute Value Accessor Functions
    All these functions have a similar behaviour. They retrieve the value of
    the attribute specified. The only difference is the return type.
    \param card_ref A reference to a card.
    \param uuid The uuid of the attribute.
    \return The value of the attribute.
    */
    /*@{*/

    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */
    uint8_t dag_config_get_boolean_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */
    char dag_config_get_char_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */ 
    const char* dag_config_get_string_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */
    int32_t dag_config_get_int32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */
    uint32_t dag_config_get_uint32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */
    int64_t dag_config_get_int64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */ 
    uint64_t dag_config_get_uint64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */
    dag_err_t dag_config_get_struct_attribute(dag_card_ref_t card, attr_uuid_t uuid, void** value);
    /**
    * to get value fo a boolean type attribute.
    * \param card_ref A reference to a card.
    * \param uuid The uuid of the attribute.
    * \return The value of the attriubute
    */
    float dag_config_get_float_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /*@}*/
    /*@}*/

    /**
    \defgroup AttributeMutatorFunctions AttributeMutatorFunctions.
    All these functions behave similarly in that they assign a value to an attribute. The only difference is the type of the value to assigned.
    \param card_ref A reference to the card.
    \param uuid The identifier of the attribute to set.
    \param value The value to assign the attribute. This is not used for dag_config_set_null_attribute.
    \return Returns kDagErrInvalidCardRef if the card refered to is bad. kDagErrNone is returned on success.
    */
    /*@{*/
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_boolean_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, uint8_t value);
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_char_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, char value);
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_string_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, const char* value);
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_int32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, int32_t value);
     /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_uint32_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, uint32_t value);
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_int64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, int64_t value);
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_uint64_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, uint64_t value);
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_null_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid);
     /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_struct_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, void* value);
    /** \param card_ref A reference to a card.
     *  \param uuid the uuid of the attribute.
     *  \param value the value of the attribute to be set.
     *  \return dag_err_t kDagErrNone if No error otherwise the appropriate error code.
     */
    dag_err_t dag_config_set_float_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, float value);
    /*@}*/

    
        /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    All these functions have a similar behaviour. They retrieve the value of
    the attribute specified. The only difference is the return type.
    \param card_ref A reference to a card.
    \param uuid The uuid of the attribute.
    \return The value of the attribute.
    */
    /*@{*/
    /**
     * Get the name of a config attribute
     * \param card_ref card reference
     * \param uuid attribute identifier
     * \return name of attribute
     */
    const char* dag_config_get_config_attribute_name(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
     * Get the description of a config attribute
     * \param card_ref card reference
     * \param uuid attribute identifier
     * \return attribute description
     */
    const char* dag_config_get_config_attribute_description(dag_card_ref_t card_ref, attr_uuid_t uuid);
    
    
    
    /* Status attribute information accessors. */
    /**
     * Get the name of a status attribute
     * \param card_ref card reference
     * \param uuid attribute identifier
     * \return attribute description
     */    
    const char* dag_config_get_status_attribute_name(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /**
     * Get the name of a status attribute
     * \param card_ref card reference
     * \param uuid attribute identifier
     * \return attribute description
     */    
    const char* dag_config_get_status_attribute_description(dag_card_ref_t card_ref, attr_uuid_t uuid);
    /*@}*/


    /**
     * \ingroup Firmware
     * Load the PCI firmware image to the card specified. It does not require a card reference so the
	 * calling function must remember to reinitialize the card (using dag_config_init). A typical 
	 * algorithm for using this function is
	 * \code
	 * dag_config_dispose(card);
	 * dag_firmware_load_pci_no_card_ref("/dev/dag0", "image.bit");
	 * card = dag_config_init("/dev/dag0")
	 * \endcode
     * \param name device name
     * \param filename firmware file name 
     * \return  error code \ref dag_err_t
     */
	dag_err_t dag_firmware_load_pci_no_card_ref(const char* name, const char* filename);

    /**
     * \ingroup Firmware
     * Load the PP (Packet Processor) firmware image to the card specified. It does not require a card reference so the
	 * calling function must remember to reinitialize the card (using dag_config_init). A typical 
	 * algorithm for using this function is
	 * \code
	 * dag_config_dispose(card);
	 * dag_firmware_load_pp_no_card_ref("/dev/dag0", "image.bit", 0);
	 * dag_firmware_load_pp_no_card_ref("/dev/dag0", "image.bit", 1);
	 * card = dag_config_init("/dev/dag0")
	 * \endcode
     * \param name device name
     * \param filename firmware file name 
     * \param which_pp the packet processor on the card to program.
     * \return  error code \ref dag_err_t
     */
	dag_err_t dag_firmware_load_pp_no_card_ref(const char* name, const char* filename, int which_pp);
    /**
     * \ingroup Firmware
     * Load the PCI firmware image to the card specified
     * \param name device name
     * \param card_ref card reference
     * \param filename firmware file name 
     * \return  error code \ref dag_err_t
     */
    dag_err_t dag_firmware_load_pci(const char* name, dag_card_ref_t* card_ref, const char* filename);
    /**
     * \ingroup Firmware
     * Load the PP firmware image to the card specified
     * \param name device name
     * \param card_ref card reference
     * \param filename firmware file name 
     * \param which_pp which pp to load
     * \return  error code \ref dag_err_t
     */
    dag_err_t dag_firmware_load_pp(const char* name, dag_card_ref_t* card_ref, const char* filename, int which_pp);
    /**
     * \ingroup Firmware
     * Load the embedded firmware image to the card specified
     * \param card_ref card reference
     * \param filename firmware file name 
     * \param target_processor_region the processor region to load the file into (bootloader, kernel or filesystem)
     * \return  error code \ref dag_err_t
     */
    dag_err_t dag_firmware_load_embedded(dag_card_ref_t card_ref, const char* filename, dag_embedded_region_t region);
    /**
     * \ingroup Firmware
     * Load the pci firmware image to the card specified
     * \param name device name
     * \param card_ref card reference
     * \param filename firmware file name 
     * \return  error code \ref dag_err_t
     */
    dag_err_t dag_firmware_load_copro(dag_card_ref_t card_ref, const char* filename, int flags);
    /**
     * \ingroup Firmware
     * Write the software id to the card 
     * \param card_ref card reference
     * \param buffer software id
     * \param length number of bytes
     * \param key software key
     * \return  error code \ref dag_err_t
     */
    dag_err_t dag_firmware_write_swid(dag_card_ref_t card_ref, uint8_t* buffer, int length, uint32_t key);
    /**
     * \ingroup Firmware
     * Read the software id from the card
     * \param card_ref card reference
     * \param buffer software id
     * \param length number of bytes
     * \return error code \ref dag_err_t
     */
    dag_err_t dag_firmware_read_swid(dag_card_ref_t card_ref, uint8_t* buffer, int length);
    
    /**
     *\ingroup Firmware
      Read the serial number from the card
      \param card_ref card reference
      \param serial number
      \return error code \ref dag_err_t
    */
    dag_err_t dag_firmware_read_serial_id(dag_card_ref_t card_ref, int* serial);
    /**
     * \ingroup Firmware 
     * Get the currently active firmware \ref dag_firmware_t
     * \param card_ref card reference
     * \return \ref dag_firmware_t
     */
    dag_firmware_t dag_firmware_get_active(dag_card_ref_t card_ref);
    /* Switch to specified image. */
    /**
     * \ingroup Firmware
     * Switch to a specified firmware image
     * \param card_ref card_reference
     * \param firmware_type Firmware type to make active \ref dag_firmware_t
     * \return error code \ref dag_err_t
     */
    dag_err_t dag_firmware_make_pci_active(dag_card_ref_t card_ref, dag_firmware_t firmware_type);

    /**
     *\ingroup Firmware
      Read the user programmed firmware name from the card
      \param card_ref card reference
      \return user firmware name 
    */
     char * dag_firmware_read_user_firmware_name(dag_card_ref_t card_ref);

    /**
     *\ingroup Firmware
      Read the current firmware name from the card
      Alias for \ref dag_firmware_read_user_firmware_name for backward compatability
      \param card_ref card reference
      \return user firmware name 
    */
     char * dag_firmware_read_current_firmware_name(dag_card_ref_t card_ref);

    /**
     *\ingroup Firmware
      Read the factory-programmed firmware name from the card
      \param card_ref card reference
      \return factory firmware name
    */
    char * dag_firmware_read_factory_firmware_name(dag_card_ref_t card_ref);
     /**
      *\ingroup Firmware
       returns the version of the ROM CONTROLLER
       \param card_ref card reference
       \return the version number
      */
    int dag_firmware_controller_get_version(dag_card_ref_t card_ref);
    /**
     *\ingroup Firmware
      read firmware name with the number from 1 - 3 for V2 of rom controller.
      \param card ref card reference
      \param number the number for rom controller
      \return user firmware name
    */
    char *dag_firmware_read_new_user_firmware_name(dag_card_ref_t card_ref,int number); 	    
    /**
     *\ingroup Firmware
      Read the stable firmware name from the card
      Alias for \ref dag_firmware_read_factory_firmware_name for backward compatibility
      \param card_ref card reference
      \return factory firmware name
    */
    char * dag_firmware_read_stable_firmware_name(dag_card_ref_t card_ref);    
    
    /**
    \defgroup AttributeAccessors Attribute Accessor Functions
    */
    /*@{*/
    /**
     * Get the attribute code of an attribute
     * \param attr_uuid attribute identifier
     * \return attribute code \ref dag_attribute_code_t
     */
    dag_attribute_code_t dag_config_get_attribute_code(attr_uuid_t attr_uuid);
    /*@}*/ 

    /** 
    \defgroup TimeSync	Time synchronize Functions
    */
    /*@{*/ 
    /**
     * Get the last time the card was synchronized
     * \param card_ref card reference
     * \param time
     * \return error code \ref dag_err_t
     */
    dag_err_t dag_config_get_last_sync_time(dag_card_ref_t card_ref, struct tm* time_param);
    /**
     * Synchronize the card with host
     * \param card_ref card reference
     * \return error code \ref dag_err_t
     */
    dag_err_t dag_config_sync_to_host(dag_card_ref_t card_ref); 
    /*@}*/ 
    
	/** \defgroup  UCAPI Universal Counter Functions  
 	 * This interface facilitates the reading of the various counters 
 	 * and statistic registers of any type of DAG card
 	 *
 	 */
	/*@{*/
	/** 
         *  \brief		Get number of CSI block(s).
	 *  \param[in] card_ref	card reference
	 *  \return 		Number of block (counter statistic interface) 
	 *  			of the DAG card	
	 */
	uint32_t dag_config_get_number_block(dag_card_ref_t card_ref);
	
	/**
         *  \brief		Get number of counters for a particular block
         *  \param[in] card_ref	card reference,
	 *  \param[in] block_type type of clock.
	 *  \return		return the counter number for a specified block.
	 */
	uint32_t dag_config_get_number_counters(dag_card_ref_t card_ref, dag_block_type_t block_type);
	
	/**
         * \brief		Get total number of counters on the card 
         * \param[in] card_ref	card reference
	 * \return 		return the number of all counters in the card.
	 */
	uint32_t dag_config_get_number_all_counters(dag_card_ref_t card_ref);

	/** 
         * \brief	 	Get the all counter ids and subfct for a particular block,
	 * \param[in] card_ref	card reference,
	 * \param[in] counter_id[] array returned with counter typeid and subfct,
	 * \param[in] size	counter_id array size.
	 * \return 		the number of counter typeids and subfct.
	 */
	uint32_t dag_config_get_counter_id_subfct(dag_card_ref_t card_ref, dag_block_type_t block_type, dag_counter_value_t counter_id[], uint32_t size);

	/** 
         *  \brief		Get the all blocks id on the card,
	 *  \param[in] card_ref	card reference,
	 *  \param[in] block_id[] array returned with block id,
	 *  \param[in] size	block_id array size.
	 *  \return 		 the number of blocks id.
	 */
	uint32_t dag_config_get_all_block_id(dag_card_ref_t card_ref, uint32_t block_id[], uint32_t size);

	/** 
         *  \brief		Latch and Clear all counters (<=> Latch 
         *  			and clear all blocks)
	 *  \param[in] card_ref	card reference
	 *  \return 		N/A
	 *  \note		This function is called by dag_config_read_all_counters, 
	 *  			dag_config_read_counter and print_univ_counter
	 */
	void dag_config_latch_clear_all(dag_card_ref_t card_ref);
	
	/** 
         * \brief		Latch and Clear a CSI block. 
	 * \param[in] card_ref	card reference,
	 * \param[in] block_type type of clock.
	 * \return 		N/A
	 */
	void dag_config_latch_clear_block(dag_card_ref_t card_ref, dag_block_type_t block_type);
	
	/** 
         * \brief		Read and return the value of all counters,
	 * \param[in] card_ref	card reference,
	 * \param[in] countersTab[] array returned with counter structures,
	 * \param[in] size	size of countersTab[],
	 * \param[in] lc	option lacth and Clear. 
	 * \return 		the number of counters.
	 */
	uint32_t dag_config_read_all_counters(dag_card_ref_t card_ref, dag_counter_value_t countersTab[], uint32_t size, int lc);
	
	/** 
         * \brief		Get the value of all counters in a block,
	 * \param[in] card_ref 	card reference,
	 * \param[in] block_type ID type of 
	 * \param[in] countersTab[] array returned with counter structures,
	 * \param[in] size	size of countersTab[],
	 * \param[in] lc	option lacth and Clear.
	 * \return 		the number of counters.
	 */
	uint32_t dag_config_read_single_block(dag_card_ref_t card_ref, dag_block_type_t block_type, dag_counter_value_t countersTab[], uint32_t size, int lc);

	/** 
         * \brief		Get the value of a single counters on the card,
	 * \param[in] card_ref	card reference,
	 * \param[in] block_type ID type of block,
	 * \param[in] counter_type ID type of counter,
	 * \param[in] subfct_type ID type of sub-function.
	 * \return 		the counter value.
	 */
	uint64_t dag_config_read_single_counter(dag_card_ref_t card_ref, dag_block_type_t block_type, dag_counter_type_t counter_type, dag_subfct_type_t subfct_type);
          /*@}*/

	/** 
         *  Read and return one specific counter on the card 
	 * Return 0 if it's not a latch and clear counter, its value is the first element of countersTab
	 * Return N if there are N Latch and Clear counters and their value in countersTab
	 */ 
	 //this function was changed temporary remove for compile usage perp will be back in 1 or 2 days
//	uint32_t dag_config_read_counter(dag_card_ref_t card_ref, dag_counter_type_t type, dag_counter_value_t countersTab[]);
         /**
          \defgroup AttributeMutatorFunctions AttributeMutatorFunctions.
           */
          /*@{*/
         /**
         *sets the attribute value 
         * \param cardref 
         * \param attributename
         * \param indes
         * \return errorcode 
         */
	dag_err_t dag_config_set_named_attribute_uuid(dag_card_ref_t card_ref, char *name_value,int index);
        /*@}*/

	/*This function reads the name and value of an attributes from the command line as a argument of dagconfig -S or --setattribute.
	 * The argument should be of the form attributename=value.it sets the value to the attributes */
          /**
          \defgroup AttributeAccessors Attribute Accessor Functions
           */
          /*@{*/
	  /**
         *gets the attribute value 
         * \param cardref 
         * \param attributename
         * \param indes
         * \return attribute value as string. 
         */
	const char *dag_config_get_named_attribute_value(dag_card_ref_t card, char *name,int index);
         /*@}*/

        /**
 	* \defgroup InfinibandTapmode	Infiniband Card Tapmode Functions
        * sets the infiniband card tapmode. - dummy function not implemented.The functionality included in 
	* default method of crosspoint switch.
        */
        /*@{*/
	/**
 	* \brief		Set infiniband active mode
 	* \param[in] card_ref	card reference
 	* \return 		last error code 
 	*/
	dag_err_t dag_config_set_infiniband_activemode(dag_card_ref_t card_ref);
	/**
	* \brief		Set infiniband tap mode
 	* \param[in] card_ref	card reference
 	* \return 		last error code 
 	*/
	dag_err_t dag_config_set_infiniband_tapmode(dag_card_ref_t card_ref);
	/**	
	* \brief		Set infiniband monitor mode
 	* \param[in] card_ref	card reference
 	* \return 		last error code 
 	*/
	dag_err_t dag_config_set_infiniband_monitormode(dag_card_ref_t card_ref);
	/**	
	* \brief		Get infiniband tap mode
 	* \param[in] card_ref	card reference
 	* \return 		0 : Success, 1 : Failure 
 	*/
	bool dag_config_get_infiniband_tapmode(dag_card_ref_t card_ref);
	/**
	* \brief		Get infiniband monitor mode
 	* \param[in] card_ref	card reference
 	* \return 		0 : Success, 1 : Failure
 	*/
	bool dag_config_get_infiniband_monitormode(dag_card_ref_t card_ref);
        /*@}*/
	
    /**
     * \ingroup ConfigLayerComponentAccessor
     * Count the component instances whose code matches the component_code. Only
     * components immediately below the root component are counted.
     *
     * \param card card reference
     * \param component_code attribute to look for
     * \return number of component instances 
     */
    int dag_config_get_recursive_component_count(dag_card_ref_t card_ref, dag_component_code_t comp_code);

    /**
     * \ingroup ConfigLayerComponentAccessor
     * Get a component by its index. . See \ref dag_config_get_component_count function above.
     *
     * \param card card reference
     * \param component_code attribute to look for
     * \param a_index index in the list
     * \return dag_component_t or NULL if it is not found
     */
    dag_component_t dag_config_get_recursive_indexed_component(dag_card_ref_t card_ref, dag_component_code_t comp_code, int a_index);

    /**
     *\ingroup Firmware
      Read the factory-programmed firmware name from the copro 
      \param card_ref card reference
      \return factory firmware name or NULL if it has no co-pro
    */
    char *dag_copro_read_factory_firmware_name(dag_card_ref_t card_ref);
    /**
     *\ingroup Firmware
      Read the factory-programmed firmware name from the copro 
      \param card_ref card reference
      \return factory firmware name or NULL if it has no co-pro
    */
    char *dag_copro_read_user_firmware_name(dag_card_ref_t card_ref);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/**\defgroup CSMarco Configuration and Status Marco
 */
/*@{*/
/**
 * Return if expression is NULL
 */
#define NULL_RETURN(expression)\
    if ((expression) == NULL)\
    {\
        fprintf(stderr, "NULL_RETURN: %s == NULL, line = %d, file = %s\n", #expression, __LINE__, __FILE__);\
        return;\
    }
/**
 * Null Return With Value
 */
#define NULL_RETURN_WV(expression, value)\
    if ((expression) == NULL)\
    {\
        fprintf(stderr, "NULL_RETURN_WV: %s == NULL, line = %d, file = %s\n", #expression, __LINE__, __FILE__);\
        return value;\
    }

/**
 *  abort on NULL 
 */
#define NULL_ABORT(expression)\
    if ((expression) == NULL)\
    {\
        fprintf(stderr, "NULL_ABORT: %s == NULL, line = %d, file = %s\n", #expression, __LINE__, __FILE__);\
        abort();\
    }

/**
 * Check that exp is not kDagErrNone.
 */
#define DAG_ERR_RETURN(expression)\
    {\
        dag_err_t e = (expression);\
        if ((e) != kDagErrNone)\
        {\
            fprintf(stderr, "DAG_ERR_RETURN: %s != kDagErrNone, line = %d, file = %s\n", #expression, __LINE__, __FILE__);\
            return;\
        }\
    }
/**
 * Check that exp is not kDagErrNone and return the given value
 */
#define DAG_ERR_RETURN_WV(expression, value)\
    {\
        dag_err_t e = (expression);\
        if ((e) != kDagErrNone)\
        {\
            fprintf(stderr, "DAG_ERR_RETURN: %s != kDagErrNone, line = %d, file = %s\n", #expression, __LINE__, __FILE__);\
            return value;\
        }\
    }
/**
 * Abort if expression is not kDagErrNone
 */
#define DAG_ERR_ABORT(expression)\
    {\
        dag_err_t e = (expression);\
        if ((e) != kDagErrNone)\
        {\
            fprintf(stderr, "DAG_ERR_ABORT: %s != kDagErrNone, line = %d, file = %s", #expression, __LINE__, __FILE__);\
            abort();\
        }\
    }

#endif /* DAG_CONFIG_H */
/*@}*/
/*@}*/

