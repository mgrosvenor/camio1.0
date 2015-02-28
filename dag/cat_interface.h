/**********************************************************************
 *
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: cat_interface.h,v 1.10.2.1 2009/08/13 23:16:21 wilson.zhu Exp $
 *
 **********************************************************************/
/** \defgroup CAT Color Association Table (CAT) API
 *  The CAT API is used to decouple the classification process from the steering
 *  process. The CAT allows complete re-mapping of the color embedded into each 
 *  ERF record (which contains filtering and hashing result).
 */
/*@{*/

#ifndef CAT_INTERFACE_H
#define CAT_INTERFACE_H

#define BANK_SIZE 0x4000  /**< size of one bank of cat.*/

/**
 * CAT mode 
 */
typedef enum cat_mode
{
	ETH_HDLC_POS = 0, /**< No tcam colour No Hash*/
	COLOR_HDLC_POS_COLOR_ETH = 1, /**< Onnly TCAM colour No Hash*/
	COLOR_HASH_POS_COLOR_HASH_ETH = 2, /**< TCAM colour and Hash form HLB*/
	COLOR_HASH_POS_COLOR_HASH_ETH_SHIFTED = 3 /**< TCAM colour and Hash from HLB shifted.*/
}cat_mode;

typedef enum error_codes
{
	kErrorNone          =  0,
	kInvalidComponent   = -1,
	kInvalidDestination = -2,
	kInvalidAddress     = -3
}cat_error_codes;


/**
 * \defgroup CATinterface The CAT Interface 
 * The CAT interface to be exposed to the user
 */
/*@{*/

/**
 * \brief		Write and entry into the CAT table. The CAT module maps 
 * 			the IPF colour, Hash value from load balancer and interface 
 * 			values to different streams.The CAT has 2 banks. The entry 
 * 			will be written to the inactive bank.To know the maximum  
 * 			values of IPF colour, hash and interface values you can 
 * 			use  the  functions  cat_get_max_color() cat_get_max_hlb_or_hash() 
 * 			cat_get_max_interface().The destination is a one-hot encoded 
 * 			32 bits value with each bit representing one recieve stream.
 *
 * \param[in] component	component reference
 * \param[in] iface	Specified the interface
 * \param[in] color	tcam color
 * \param[in] hlb 	hash value from HLB
 * \param[in] destination the 32 bits one-hot encoded destination stream value 
 *
 * \return 		On success cat_new_set_entry_verify returns the destination 
 * 			written.else it returns the appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 * 			- kInvalidDestination: the destination supplied is invalid.
 */
int32_t cat_set_entry(dag_component_t component_t,uint8_t iface,uint16_t color,uint8_t hlb,int32_t destination);

/**
 * \brief		Write and entry into the CAT table and then read back the value 
 * 			for verification.  
 *
 * \param[in] component	component reference
 * \param[in] iface	Specified the interface
 * \param[in] color	tcam color
 * \param[in] hlb 	hash value from HLB
 * \param[in] destination the 32 bits one-hot encoded destination stream value 
 *
 * \return 		On success cat_new_set_entry_verify returns the destination 
 * 			written.else it returns the appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 * 			- kInvalidDestination: the destination supplied is invalid.
 */
int32_t cat_set_entry_verify(dag_component_t component_t,uint8_t iface,uint16_t color,uint8_t hlb,int32_t destination);

/**
 * \brief		Read and entry into the CAT table.  
 *
 * \param[in] component	component reference
 * \param[in] iface	Specified the interface
 * \param[in] color	tcam color
 * \param[in] hlb 	hash value from HLB
 * \param[in] bank	specified the bank of CAT
 *
 * \return 		On success cat_new_set_entry_verify returns the destination 
 * 			written.else it returns the appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 * 			- kInvalidDestination: the destination supplied is invalid.
 */
int32_t cat_get_entry(dag_component_t component_t,uint8_t iface,uint16_t color,uint8_t hlb,uint8_t bank);

/** 
 * \brief   		Sets the specified destination (one-hot encoded) at the specified 
 * 			address in the CAT table. The user has to calculate the address from 
 * 			interface , tcam colour and hash and also based on the mode.
 *
 * \param[in] component component reference
 * \param[in] address	specified the address of CAT table.
 * \param[in] destination the 32 bits one-hot encoded destination stream value
 *
 * \return 		On success return the destination written. else it returns the 
 * 			appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 *			- kInvalidDestination: the destination supplied is invalid
 */
int32_t cat_set_raw_entry(dag_component_t component ,uint16_t address,int32_t destination);

/**
 * \brief   		Sets the specified destination (one-hot encoded) at the specified 
 * 			address in the CAT table. The user has to calculate the address from 
 * 			interface , tcam colour and hash and also based on the mode.This 
 * 			function also reads back the written entry and verifies them.
 *
 * \param[in] component	component reference
 * \param[in] address	specified the address of CAT table
 * \param[in] destination the 32 bits one-hot encoded destination stream value
 *
 * \return 		On success  returns the destination written.else it returns 
 * 			the appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 * 			- kInvalidDestination: the destination supplied is invalid.
 */
int32_t cat_set_raw_entry_verify(dag_component_t component ,uint16_t address,int32_t destination);

/*
 * \brief   		Reads the specified address in the cat table and returns the one-hot 
 * 			encoded destintaion value
 *
 * \param[in] component	component reference
 * \param[in] address	specified the address of CAT table
 * \param[in] destination the 32 bits one-hot encoded destination stream value
 *
 * \return 		On success  returns the destination written.else it returns 
 * 			the appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 * 			- kInvalidDestination: the destination supplied is invalid.
 */
int32_t cat_get_raw_entry(dag_component_t component_t,uint16_t bank,uint16_t address);

/**
 * \brief		Returns the mask for the hash value after computing the hash width 
 * 			based on the firmware configurations.This function will be used 
 * 			internally by cat_new_set_entry().The user is advised not to use 
 * 			this function.To set an entry to the cat table cat_new_set_entry() 
 * 			can be used.
 * 
 * \param[in] hash_width the hash width is computed base on the firmware configureation
 *
 * \return 		On success return the mask for the hash value.
 *
 */
uint32_t cat_get_hash_mask(uint32_t hash_width);

/*This is the new interface exposed for HSBM to support one hot encoding for up to 64 streams.The old interface is still kept so as to maintain backwards compatibality*/
/**
 * \brief		Write and entry into the CAT table. The CAT module maps 
 * 			the IPF colour, Hash value from load balancer and interface 
 * 			values to different streams.The CAT has 2 banks. The entry 
 * 			will be written to the inactive bank.To know the maximum  
 * 			values of IPF colour, hash and interface values you can 
 * 			use  the  functions  cat_get_max_color() cat_get_max_hlb_or_hash() 
 * 			cat_get_max_interface().The destination is a one-hot encoded 
 * 			value with each bit representing one recieve stream.
 *
 * \param[in] component	component reference
 * \param[in] iface	Specified the interface
 * \param[in] color	tcam color
 * \param[in] hash 	hash value from HLB
 * \param[in] destination one-hot encoded destination stream value 
 *
 * \return 		On success cat_new_set_entry_verify returns the destination 
 * 			written.else it returns the appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 * 			- kInvalidDestination: the destination supplied is invalid.
 */
uint64_t cat_new_set_entry_verify(dag_component_t component,uint8_t iface,uint16_t color,uint8_t hash,uint64_t destination);

/**
 * \brief		Sets the specified 64 bits destination (one-hot encoded) at the specified 
 * 			address in the CAT table. The user has to calculate the address from 
 * 			interface , tcam colour and hash and also based on the mode.
 *
 * \param[in] component	component reference
 * \param[in] address	specified the address of CAT table
 * \param[in] destination one-hot encoded destination stream value 
 *
 * \return 		On success cat_new_set_entry_verify returns the destination 
 * 			written.else it returns the appropriate error value
 *
 * \note		Specified error codes:
 * 			- kInvalidAddress: the address computed from interface hash 
 * 			  and colour is invalid.  
 * 			- kInvalidDestination: the destination supplied is invalid.
 */
uint64_t cat_new_set_raw_entry_verify(dag_component_t component,uint16_t address,uint64_t destination);

/**
 * \brief		returns the currently active bank.
 *
 * \param[in] component_t component reference
 *
 * \return 		On success cat_get_current_bank returns the currently 
 * 			active bank.
 */
int8_t cat_get_current_bank(dag_component_t component_t);

/**
 * \brief		cat_swap_banks swaps the currently active bank.
 * 
 * \param[in] component_t component reference
 *
 * \return 		On success cat_swap_banks swaps the currently active bank.
 */
void cat_swap_banks(dag_component_t component_t);

/**
 * \brief               Returns the number of output bits in the CAT TABLE
 * 
 * \param[in] component_t component reference
 *
 * \return              On success return the number of output bits in the CAT table.
 * 	
 */
uint32_t cat_get_no_of_output_bits(dag_component_t component_t);

/**
 * \brief		returns the number of address bits/input bits in the CAT table.
 *
 * \param[in] component_t component reference
 *
 * \return 		On success return the number of address bits in the CAT table
 */
uint32_t cat_get_no_of_input_bits(dag_component_t component_t);

/**
 * \brief		returns the maximum hlb width or hash width as the case maybe.
 *
 * \return 		On success return the maximum hlb width or hash width.
 */

uint32_t cat_get_max_hlb_or_hash();

/**
 * \brief		Returns the number of interface bits that will be used to program 
 * 			the CAT.The possible values are 0 and 2.You can use the attribute 
 * 			kBooleanAttributeEnableInter‐faceOverwrite to configure this attribute.
 *
 * \param[in] component_t component reference
 *
 * \return 		On success cat_get_max_interface returns the number of interface bits 
 * 			that will be used to program the CAT.
 *
 */
uint32_t cat_get_max_interface(dag_component_t component_t);

/**
 * \brief		Returns  the  number  of  color bits that will be used to program 
 * 			the CAT.This depents on the the number of input bits,number of hash 
 * 			bits and the number of interface bits.
 *
 * \param[in] component_t component reference
 *
 * \return  		On success cat_get_max_color returns the number of color bits that 
 * 			will be used to program the CAT.
 *
 */
uint32_t cat_get_max_color(dag_component_t component_t);

/**
 * \brief		Returns the number of hash bits that will be used to program the CAT.
 * 			This depends on the ERF type in case of FCSBM and on the configured 
 * 			hash width in case of HSBM.
 *
 * \param[in] component component reference
 *
 * \return		On success returns the number of hash width that will be used to 
 * 			program the CAT.
 */
uint32_t get_configured_hash_width(dag_component_t component);

/**
 * \brief		Returns the entry mode in which CAT is configured.
 *
 * \return 		On success return the entry mode in which CAT is configured
 */
uint32_t cat_get_entry_mode();


/**
 * \brief		The mode in which cat is configured, this is based on the version of the 
 * 			firmware modules present in the  cat. see cat_mode enum for details.
 *	
 * \param[in] mode	The CAT mode 
 *
 * \return 		N/A
 */
void cat_set_entry_mode(int mode);

/**
 * \brief		Configures the CAT in bypass mode.This is the default configuration 
 * 			of the CAT module.All the traffic will be directed to stream 0.
 *
 * \param[in] component_t component reference
 *
 * \return 		N/A
 */
void cat_bypass(dag_component_t component_t);
/*@{*/

#endif
/*@{*/
