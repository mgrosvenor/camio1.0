/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagixp_filter.h,v 1.6.86.1 2009/08/14 01:56:49 wilson.zhu Exp $
*
**********************************************************************/
/**
 * \defgroup IXPfilterAPI IXP filtering API
 * The IXP filter is a collection of host and embedded software that 
 * performs Internet Protocol (IPv4/IPv6) and layer 3 filtering on 
 * PoS packets. The IXP Filtering is specific to DAG 7.1S cards, all 
 * the filtering is done internally on the card. A host API library 
 * is supplied to configure and initiate the filtering, however no 
 * host process is required to maintain the filtering once configured 
 * and running.
 */
/*@{*/

/**********************************************************************
* FILE:         dagixp_filter.h
* DESCRIPTION:  Interface functions of the filtering API
*
*
* HISTORY:      14-12-05 Initial revision
*
**********************************************************************/
#ifndef DAGFILTER_H
#define DAGFILTER_H



#if defined(_WIN32)
	/* Required for the in6_addr structure */
	#include <Ws2tcpip.h>
#endif




/*--------------------------------------------------------------------
 
 Types

---------------------------------------------------------------------*/

typedef struct Rule_ Rule_;
typedef Rule_* RuleH;      /* Type-safe opaque type. */

typedef struct Ruleset_ Ruleset_;
typedef Ruleset_* RulesetH;    /* Type-safe opaque type. */


typedef enum
{
	kReject = 0,
	kAccept = 1,

} action_t;


typedef enum
{
	kHost = 0,
	kLine = 1,
	
} steering_t;


typedef enum
{
	kRange   = 0,
	kBitmask = 1,
	
} rule_type_t;


typedef enum
{
	kSourcePort      = 0,
	kDestinationPort = 1,
	
} filter_type_t;


/**
 * Contains the filtering statistic identifiers.
 */
typedef enum
{
	kPacketsRecv                 = 0,  /**< The number of packets that have been received 
						by the IXP, this is a 64-bit number that will roll 
						over to 0 (after 18446744073709551615 packets have 
						been received). */
	kPacketsAccepted             = 1,  /**< The number of packets that have been accepted and 
						sent back out the IXP chip, this statistic doesn't 
						cover the number of packets that have been accepted 
						but were dropped because of buffer overflows or transmit 
						errors. */
	/* dag7.1s specific statistics */
	kPacketsInPlay               = 1000,	/**< Contains the number of packets that are being filtered 
						at that instant, this statistic will contain a value between 
						0 and 8192. When the number of packets in play reaches 8192
						packets will be dropped, if this statistic is regularly up 
						around 4000 you may want to consider reducing the number of 
						filter rules to avoid packets being dropped due to buffer
						overrun. */
	kPacketsDroppedErfType       = 1001,	/**< Number of packets dropped because the ERF type received 
						from the FPGA was not a PoS type record (type 1 or 5). */
	kPacketsDroppedHdlc          = 1002,	/**< Number of packets dropped because the HDLC header of the 
						PoS frame was not recognised, the filtering software only accepts 
						a small number of standard HDLC headers. */

	kPacketsDroppedErfSize       = 1003,	/**< Number of packets dropped because of a mismatch in the record 
						length field of the ERF header and the length of the actual record. */
	kPacketsDroppedNoFree        = 1004,	/**< Number of packets dropped due to the internal packet buffers 
						reaching their limit, the filtering software can buffer up to 8192 
						packets internally when this limit is reached packets are dropped. 
						This statistic will increase if the host machine can't process the 
						packets fast enough, in such cases reverse flow control will be 
						asserted back to the filtering software and packets will be dropped. */
	kPacketsDroppedRxFull        = 1005,	/**< This statistic is reserved for future use and is currently not 
						implemented. */
	kPacketsDroppedTxFull        = 1006,	/**< This statistic is reserved for future use and is currently 
						not implemented. */
	kPacketsDroppedIPv6ExtHeader = 1007,	/**< Number of IPv6 packets dropped because an unknown extension 
						header was found in the packet, see to the table on page 3 for a 
						list of supported IPv6 extension headers. */
	kPacketsDroppedSeqBuffer     = 1008,	/**< Number of packets dropped because more than 10 MPLS shim headers 
						were found in the packet, the IP filtering software supports a maximum 
						of 10 MPLS shims. */
	kPacketsDroppedTooBig        = 1009,	/**< Indicates the number of packets dropped because the filtering 
						process delayed the packet to long, this is caused by complex filter 
						rules that lead to varied packet latencies within the filtering 
						software. */
	kPacketsDroppedMplsOverflow  = 1010,	/**< Number of packets dropped because they are too large to be 
						processed, the maximum packet size supported is 4096 bytes including 
						the ERF header. */
	kMsfErrUnexpectedSOP         = 1011,	/**< Number of packets dropped because of an MSF bus error. */
	kMsfErrUnexpectedEOP         = 1012,	/**< Number of packets dropped because of an MSF bus error. */
	kMsfErrUnexpectedBOP         = 1013,	/**< Number of packets dropped because of an MSF bus error. */
	kMsfErrNullMPacket           = 1014,	/**< Number of packets dropped because of an MSF bus error. */
	kMsfErrStatusWord            = 1015,	/**< Number of packets dropped because of an MSF bus error. */
	
	
	/* can only be used when clearing the statistics */
	kAllStatistics               = 65534,	/**< Constant that allows for all the statistics to be cleared with 
						a single function call. */

} statistic_t;

/**
 * Used to retrieve the details of a port filter entry within a rule.
 */
typedef struct port_entry_s
{
	uint32_t   port_type;	/**< The port type of the filter, this can have one 
				of two possible values; kSourcePort or kDestinationPort.*/
	uint32_t   rule_type;	/**< The rule type of the filter, this can have one of 
				two possible values; kBitmask or kRange. */
	union
	{
		struct {
			uint16_t min_port;	/**< The minimum value of a port range filter, 
						only valid if the rule_type data field is kRange.*/
			uint16_t max_port;	/**< The maximum value of a port range filter, 
						only valid if the rule_type data field is kRange.*/
		} range;
		
		struct {
			uint16_t value;		/**< The comparand value of a port bit-mask filter, 
						only valid if the rule_type data field is kBitmask.*/
			uint16_t mask;		/**< The mask value of a port bit-mask filter, only 
						valid if the rule_type data field is kBitmask. */
		} bitmask;
		
	} data;
	
} port_entry_t;


/**
 * Used to retrieve the details of a ICMP type filter entry within a rule.
 */
typedef struct icmp_entry_s
{
	uint32_t   rule_type;	/**< The rule type of the filter, this can have one 
				of two possible values; kBitmask or kRange. */

	union
	{
		struct {
			uint8_t min_type;	/**< The minimum value of a ICMP type range filter, 
						only valid if the rule_type data field is kRange. */
			uint8_t max_type;	/**< The maximum value of a ICMP type range filter, 
						only valid if the rule_type data field is kRange. */

		} range;
		
		struct {
			uint8_t value;		/**< The comparand value of a ICMP type bit-mask filter, 
						only valid if the rule_type data field is kBitmask */
			uint8_t mask;		/**< The mask value of a ICMP type bit-mask filter, 
						only valid if the rule_type data field is kBitmask. */
		} bitmask;
		
	} data;
	
} icmp_entry_t;







/*--------------------------------------------------------------------
 
 Error Codes

---------------------------------------------------------------------*/

#define EUNSUPPORTDEV           2000
#define EALREADYOPEN            2001
#define ENOTOPEN                2002
#define EINVALIDRESP            2003




/*--------------------------------------------------------------------
 
 Constants

---------------------------------------------------------------------*/

#define kAllIntefaces           0xFF

#define kAllProtocols           0x00

#define kIPv4RuleType           4
#define kIPv6RuleType           6





/*--------------------------------------------------------------------
 
 API Functions

---------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	
	/*****************************************************************
	 *
	 *  INITIALISATION & TERMINATION
	 *
	 *****************************************************************/
	/**
 	* \defgroup grpInitializeIXP IXP Initialization and Termination Functions
 	* Provides two functions to initialise the library prior to use and clean up when
 	* finished with the library.
 	*/
	/*@{*/

	/* initialises the API and memory structures used by the library */
	/**
 	* \brief		Initialises the use of the DAG IXP Filter library.
 	*
 	* \return 		0 if the library was initialised and -1 if an error 
 	* 			occurred. Call dagixp_filter_get_last_error to retrieve 
 	* 			the error code.
 	* 			- EALREADYOPEN : this function has been called previously
 	* 			- ENOMEM : not enough free memory available)
 	*
 	* \note			This dagixp_filter_startup function must be the first IXP 
 	* 			filter API function called by an application or library. 
 	* 			The application or library can only issue further IXP filtering
 	* 			API function calls after successfully calling dagixp_filter_startup.
 	* 			The dagixp_filter_startup function can be called multiple 
 	* 			times, but on subsequent calls -1 will be returned and 
 	* 			dagixp_filter_get_last_error will return EALREADYOPEN.
 	* 			When it has finished using the services of the IXP filtering 
 	* 			library, the application must call dagixp_filter_shutdown 
 	* 			to allow the DAG IXP Filter library to free any resources 
 	* 			allocated for the filtering process.
 	*/
	int
	dagixp_filter_startup (void);
	
	/* releases the memory used for the dagfilter library */

	/**
 	* \brief		Terminates use of the DAG IXP Filter library.
 	*
 	* \return		The return value is zero if the operation was successful. 
 	* 			Otherwise a negative value is returned and dagixp_filter_get_last_error 
 	* 			will return one of the following error codes.
 	* 			- ENOTOPEN : library not initialised
 	* 
 	* \note			An application is required to perform a successful 
 	* 			dagixp_filter_startup call before it can use the IXP filtering 
 	* 			API library services. When it has completed the use of
 	* 			the IXP filtering library, the application must call 
 	* 			dagixp_filter_shutdown to free any resources allocated by 
 	* 			the IXP filtering library on behalf of the application.
 	* 			There is no internal reference count for dagixp_filter_startup 
 	* 			and dagixp_filter_shutdown functions, therefore dagixp_filter_shutdown 
 	* 			will free all resources regardless of the number of calls made to 
 	* 			dagixp_filter_startup. The dagixp_filter_shutdown function doesn't 
 	* 			delete any rulesets that have been created by the application, 
 	* 			it is the callers responsibility to delete the any rulesets prior to
 	* 			calling dagixp_filter_shutdown.
 	*
 	*/
	int
	dagixp_filter_shutdown (void);

	/*@}*/

	

	/*****************************************************************
	 *
	 *  RULE CONSTRUCTION
	 *
	 *****************************************************************/
	/**
 	* \defgroup grpRuleConstructIXP IXP Rule Construction Functions
 	* Provides the functionality to create and modify rulesets as well as individual
 	* IPv4 and IPv6 rules.
 	*/
	/*@{*/

	/* creates a new empty ruleset */
	/**
 	* \brief		Creates a new empty ruleset.
 	*
 	* \return		A handle to a new ruleset is returned if successful. 
 	* 			Otherwise, a NULL value is returned to indicate an error, 
 	* 			and a specific error number can be retrieved by calling
 	* 			dagdsm_get_last_error.
 	* 			- ENOTOPEN : the library hasn't been initialised, 
 	* 			call dagixp_filter_startup
 	* 			- ENOMEM : memory allocation error
 	*
 	* \note			The dagixp_filter_create_ruleset function creates a new 
 	* 			ruleset and returns a handle that can be used in subsequent 
 	* 			IXP filtering library calls that require a ruleses handle.
 	* 			When finished with the ruleset, dagixp_filter_delete_ruleset 
 	* 			must be called to free resources allocated to the ruleset.
 	* 			Rulesets are not automatically deleted when a the IXP filtering 
 	* 			library is shutdown (by calling dagixp_filter_shutdown), it is the
 	* 			callers responsibility to ensure all rulesets are deleted, prior 
 	* 			to the application terminating, otherwise memory leaks will occur.
 	* 			Rulesets are created empty, no filter rules are defined within the 
 	* 			ruleset, an empty ruleset cannot be loaded into a DAG 7.1S card.
 	*/
	RulesetH
	dagixp_filter_create_ruleset (void);
	
	/* deletes an existing ruleset */
	/**
 	* \brief		Deletes an existing ruleset.
 	*
 	* \param[in] ruleset_h	A handle to the ruleset to delete.
 	*
 	* \return 		The return value is zero if the operation was successful. Otherwise 
 	* 			a negative value is returned and dagixp_filter_get_last_error will 
 	* 			return one of the following error codes:
 	* 			- ENOTOPEN : library not initialised
 	*		 	- EINVAL : invalid parameter
 	*
 	* \note			dagixp_filter_delete_ruleset invalidates the specific ruleset handle, 
 	* 			frees the resources allocated and deletes all the filter rules contained 
 	* 			in the ruleset. If the dagixp_filter_delete_ruleset call was successful, 
 	* 			the ruleset_h handle should be discarded, unpredictable behaviour will 
 	* 			result if the ruleset handle is continued to be used.
 	* 			There is no need to free all the individual rules of a ruleset prior to 
 	* 			deleting it, this function will clean up any rules that remain in the 
 	* 			ruleset. For this reason individual rule handles associated with a deleted 
 	* 			ruleset should be discarded, unpredictable behaviour will result if the rule 
 	* 			handle is continued to be used.
 	*
 	*/
	int
	dagixp_filter_delete_ruleset (RulesetH ruleset_h);
	
	/* empties all the filters form a ruleset */
	/**
 	* \brief		Removes all the filter rules from a ruleset.
 	*
 	* \param[in] ruleset_h	A handle to the ruleset to remove all the rules from
 	*
 	* \return 		The return value is zero if the operation was successful. Otherwise 
 	* 			a negative value is returned and dagixp_filter_get_last_error will 
 	* 			return one of the following error codes:
 	* 			- ENOTOPEN : library not initialised
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function removes all the rules from a ruleset and frees all 
 	* 			the resources allocated for them. Any rule handles created for the 
 	* 			ruleset should be discarded, continuing to use them will result in 
 	* 			unpredictable behaviour.
 	*/
	int
	dagixp_filter_empty_ruleset (RulesetH ruleset_h);

	/* gets the number of filters in the ruleset */
	/**
 	* \brief		Returns the number of rules in a ruleset.
 	*
 	* \param[in] ruleset_h	A handle to the ruleset to get the rule count for
 	*
 	* \return 		The return value is a positive number indicating the number of rules 
 	* 			in the ruleset if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one 
 	* 			of the following error codes:
 	* 			- ENOTOPEN : library not initialised
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_ruleset_rule_count (RulesetH ruleset_h);

	/* returns a handle to the filter at a specified index */
	/**
 	* \brief		Returns a handle to a rule at a given index inside a ruleset.
 	*
 	* \param[in] ruleset_h	A handle to the ruleset to get the rule from
 	* \param[in] index	Zero based index of the rule to retrieve
 	*
 	* \return		A handle to the rule at the given index is returned if successful. 
 	* 			Otherwise, a NULL value is returned to indicate an error, and a 
 	* 			specific error number can be retrieved by calling dagdsm_get_last_error.
 	* 			- ENOTOPEN : library not initialised
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			Rules are stored inside the ruleset in priority order, this is 
 	* 			the same order that is used by the filtering module to determine 
 	* 			the best filter hit. Rules with higher priority appear earlier in the 
 	* 			ruleset and therefore have a lower index number. Because of this, when
 	* 			rules are added to a ruleset the index number of the rules may be 
 	* 			rearranged based on the priority of the new rule. It should not be 
 	* 			assumed that the last rule that has been added is at the last index, 
 	* 			unless it is guaranteed that the rule has the lowest priority.
 	* 			Calling dagixp_filter_set_rule_priority will also change the ordering 
 	* 			of the rules inside the ruleset and therefore the indices of the rules. 
 	* 			Use dagixp_filter_set_rule_priority with caution when iterating through 
 	* 			rules inside a ruleset.
 	*/
	RuleH
	dagixp_filter_ruleset_get_rule_at (RulesetH ruleset_h, uint32_t index);

	/* deletes a filter from the ruleset */
	/**
 	* \brief		Deletes a rule from a ruleset.
 	*
 	* \param[in] ruleset_h	A handle to the ruleset to remove the rule from
 	* \param[in] rule_h	Handle to the rule to remove from the ruleset
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a 
 	* 			negative value is returned and dagixp_filter_get_last_error will return 
 	* 			one of the following error codes:
 	* 			- ENOTOPEN : library not initialised
 	* 			- EINVAL : invalid parameter
 	* 			- ENOMEM : memory error
 	*
 	* \note			When a rule is removed from a ruleset, the memory allocated for it is 
 	* 			freed and the ruleset is updated. If dagixp_filter_remove_rule is successful 
 	* 			the rule handle should be discarded, continuing to use the handle will result 
 	* 			in unpredictable behaviour. The rule being removed must belong to the given 
 	* 			ruleset, this function will fail with an error code of EINVAL if the rule 
 	* 			doesn't belong to the ruleset.
 	*/
	int
	dagixp_filter_remove_rule (RulesetH ruleset_h, RuleH rule_h);



	/* creates a blank IPv4 filter */
	/**
 	* \brief		Creates a new rule targeted at Internet Protocol version 4 packets and adds 
 	* 			it to the given ruleset.
 	*
 	* \param[in] ruleset_h	A handle to the ruleset to add the new rule to
 	* \param[in] action	The action to assign to the rule, this can be one of the following constants
 	* \param[in] rule_tag	User defined decimal number that is inserted into the packet record if the 
 	* 			rule hits and the action is kAccept. The upper two bits of this value are 
 	* 			ignored, the range of possible values are 0-4095. Multiple rules can have 
 	* 			the same tag number
 	* \param[in] priority	Decimal number that defines the priority of the rule, the lower the number 
 	* 			the higher the priority, a zero value indicates the rule has the highest priority
 	*
 	*
 	* \return		A handle to a new rule is returned if successful. Otherwise, a NULL value is 
 	* 			returned to indicate an error, and a specific error number can be retrieved by 
 	* 			calling dagdsm_get_last_error. Possible error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- ENOMEM : memory allocation error
 	* 			- EINVAL : invalid parameter
 	*
 	*/
	RuleH
	dagixp_filter_create_ipv4_rule (RulesetH ruleset_h, action_t action, uint16_t rule_tag, uint16_t priority);

	/* creates a blank IPv6 filter */
	/**
 	* \brief		Creates a new rule targeted at Internet Protocol version 6 packets and adds it 
 	* 			to the given ruleset.
 	*
 	* \param[in] ruleset_h	A handle to the ruleset to add the new rule to
 	* \param[in] action	The action to assign to the rule, this can be one of the following constants
 	* \param[in] rule_tag	User defined decimal number that is inserted into the packet record if the rule 
 	* 			hits and the action is kAccept. The upper two bits of this value are ignored, 
 	* 			the range of possible values are 0-4095. Multiple rules can have the same tag number
 	* \param[in] priority	Decimal number that defines the priority of the rule, the lower the number the higher 
 	* 			the priority, a zero value indicates the rule has the highest priority
 	*
 	* \return		A handle to a new rule is returned if successful. Otherwise, a NULL value is returned 
 	* 			to indicate an error, and a specific error number can be retrieved by calling
 	* 			dagdsm_get_last_error. Possible error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- ENOMEM : memory allocation error
 	*			- EINVAL : invalid parameter
 	*
 	* \note			A successful call to dagixp_filter_create_ipv6_rule will create a blank filter 
 	* 			and add it to the given ruleset. The new rule will only target IPv6 packets, 
 	* 			any other packets types will automatically be skipped by the rule.
 	*/
	RuleH
	dagixp_filter_create_ipv6_rule (RulesetH ruleset_h, action_t action, uint16_t rule_tag, uint16_t priority);

	/* returns the type of a rule (either IPv4 or IPv6) */
	/**
 	* \brief		Returns the type of a rule (either IPv4 or IPv6)
 	* 
 	* \param[in] rule_h	Handle to an IPv4 or IPv6 rule.
 	*
 	* \return 		the type of rule
 	*/
	int
	dagixp_filter_get_rule_type (RuleH rule_h);




	/* sets/gets the bitmask to use for the ip source/destination addresses */
	/**
 	* \brief		Sets the IPv4 source address to filter on.
 	*
 	* \param[in] rule_h	Handle to an IPv4 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv4_rule function
 	* \param[out] src	Pointer to an in_addr structure that represents the source address to use 
 	* 			as the comparand
 	* \param[out] mask	Pointer to an in_addr structure that represents the source address mask 
 	* 			to use for the filter
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a 
 	* 			negative value is returned and dagixp_filter_get_last_error will return 
 	* 			one of the following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_set_ipv4_source_field (RuleH rule_h, struct in_addr *src, struct in_addr *mask);
	
	/**
 	* \brief		Gets the IPv4 source address used by the rule.
 	*
 	* \param[in] rule_h	Handle to an IPv4 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv4_rule function
 	* \param[out] src	Pointer to an in_addr structure that receives the comparand part of the source 
 	* 			address field
 	* \param[out] mask	Pointer to an in_addr structure that receives the mask part of the source 
 	* 			address field
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_ipv4_source_field (RuleH rule_h, struct in_addr *src, struct in_addr *mask);
	
	/**
 	* \brief		Sets the IPv4 destination address to filter on.
 	*
 	* \param[in] rule_h	Handle to an IPv4 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv4_rule function.
 	* \param[out] dst	Pointer to an in_addr structure that contains the destination address to use 
 	* 			as the comparand of the filter field.
 	* \param[out] mask	Pointer to an in_addr structure that contains the destination address mask to use 
 	* 			for the filter field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_set_ipv4_dest_field (RuleH rule_h, struct in_addr *dst, struct in_addr *mask);
	
	/**
 	* \brief		Gets the IPv4 destination address used by the rule.
 	*
 	* \param[in] rule_h	Handle to an IPv4 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv4_rule function
 	* \param[out] dst	Pointer to an in_addr structure that receives the comparand part of the 
 	* 			destination address field
 	* \param[out] mask	Pointer to an in_addr structure that receives the mask part of the destination 
 	* 			address field
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_ipv4_dest_field (RuleH rule_h, struct in_addr *dst, struct in_addr *mask);

	/**
 	* \brief		Sets the IPv6 source address to filter on.
 	*
 	* \param[in] rule_h	Handle to an IPv6 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv6_rule function
 	* \param[out] src	Pointer to an in6_addr structure that contains the source address to use 
 	* 			as the comparand of the filter field
 	* \param[out] mask	Pointer to an in6_addr structure that contains the source address mask to use 
 	* 			for the filter field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_set_ipv6_source_field (RuleH rule_h, struct in6_addr *src, struct in6_addr *mask);
	
	/**
 	* \brief		Gets the IPv6 source address used by the rule.
 	*
 	* \param[in] rule_h	Handle to an IPv6 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv6_rule function
 	* \param[out] src	Pointer to an in6_addr structure that receives the comparand part of the 
 	* 			source address field
 	* \param[out] mask	Pointer to an in6_addr structure that receives the mask part of the source 
 	* 			address field
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_ipv6_source_field (RuleH rule_h, struct in6_addr *src, struct in6_addr *mask);

	/**
 	* \brief		Sets the IPv6 destination address to filter on.
 	*
 	* \param[in] rule_h	Handle to an IPv6 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv6_rule function
 	* \param[out] dst	Pointer to an in6_addr structure that contains the destination address to use 
 	* 			as the comparand of the filter field.
 	* \param[out] mask	Pointer to an in6_addr structure that contains the destination address mask to 
 	* 			use for the filter field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_set_ipv6_dest_field (RuleH rule_h, struct in6_addr *dst, struct in6_addr *mask);
	
	/**
 	* \brief		Gets the IPv6 destination address used by the rule.
 	*
 	* \param[in] rule_h	Handle to an IPv6 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv6_rule function.
 	* \param[out] dst	Pointer to an in6_addr structure that receives the comparand part of the 
 	* 			destination address field.
 	* \param[out] mask	Pointer to an in6_addr structure that receives the mask part of the destination 
 	* 			address field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_ipv6_dest_field (RuleH rule_h, struct in6_addr *dst, struct in6_addr *mask);

	
	
	
	/* gets/sets the IPv6 flow bitmask fitler */
	/**
 	* \brief		Sets the IPv6 flow label to filter on for a given rule.
 	*
 	* \param[in] rule_h	Handle to an IPv6 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv6_rule function.
 	* \param[in] flow	The value to use as the comparand of the flow label filter field. Only the 
 	* 			lower 20 bits of this value is used, the upper 12 bits are ignored.
 	* \param[in] mask	The value to use as the mask of the flow label filter field. Only the lower 
 	* 			20 bits of this value is used, the upper 12 bits are ignored.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_set_ipv6_flow_label_field (RuleH rule_h, uint32_t flow, uint32_t mask);
	
	/**
 	* \brief		Gets the IPv6 flow label filter field used by the rule.
 	*
 	* \param[in] rule_h	Handle to an IPv6 rule. The rule should have been created by the
 	* 			dagixp_filter_create_ipv6_rule function.
 	* \param[out] flow	Pointer to a 32-bit value that receives the comparand of the flow label 
 	* 			filter field. Only the lower 20 bits of the returned value are valid the 
 	* 			upper 12 bits will always be zero.
 	* \param[out] mask	Pointer to a 32-bit value that receives the mask of the flow label filter 
 	* 			field. Only the lower 20 bits of the returned value are valid the upper 12 
 	* 			bits will always be zero.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_ipv6_flow_label_field (RuleH rule_h, uint32_t *flow, uint32_t *mask);

	
	

	/* gets/sets the protocol to filter on */
	/**
 	* \brief		Sets the IP protocol field to filter on for a given rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by 
 	* 			either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] protocol	The 8-bit IP protocol number to filter on. Setting this value to either TCP(6), 
 	* 			UDP(17), SCTP(132) or ICMP(1) protocol numbers, will allow addition port or ICMP 
 	* 			type filter fields to be added to the rule, see to the comments below for more 
 	* 			information.
 	* 
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_set_protocol_field (RuleH rule_h, uint8_t protocol);
	
	/**
 	* \brief		Gets the IP protocol filter field used by the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by 
 	* 			either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[out] protocol	Pointer to an 8-bit value that receives the protocol filter field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following
 	* 			error codes:
 	*    			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	*     			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_protocol_field (RuleH rule_h, uint8_t *protocol);
	
	
	
	/* adds a port filter bitmask entry to the rule list */
	/**
 	* \brief		Adds a source port bit-masked filter field to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by 
 	* 			either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] port	The 16-bit value to use as the comparand of the source port filter field.
 	* \param[in] mask	The 16-bit value to use as the mask of the source port filter field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function adds a new bit-masked source port filter to the list of source 
 	* 			port filters associated with the rule. Up to 254 source port filter entries 
 	* 			(of both bit-masked and range types) are allowed per rule.
 	* 			The list of source port filters is only loaded into the card and activated 
 	* 			(by using the dagixp_filter_download_ruleset and dagixp_filter_activate_ruleset
 	* 			functions) when the rule's protocol field is set to either TCP, UDP or SCTP values.
 	* 			Although it is possible to add multiple bit-masked filters to a rule, it is generally 
 	* 			not recommended, because when the ruleset is loaded into the card only one bit-masked 
 	* 			port filter is allowed per rule. The API compensates for this limitation by duplicating 
 	* 			rules that have multiple bit-masked port filters and then allocating one bit-masked 
 	* 			port filter per duplicated rule. The situation is worsened if both source and destination 
 	* 			port filter lists have bit-masked filter entries, because duplicate rules are generated 
 	* 			for every possible combination of source and destination filters.
 	* 			The more rules that are loaded into the card the worse the filter performance. Generally
 	* 			rules should have either a single bit-masked port filter entry or up to 254 port range
 	* 			filters.
 	*/ 
	int
	dagixp_filter_add_source_port_bitmask (RuleH rule_h, uint16_t port, uint16_t mask);
	
	/**
 	* \brief		Adds a destination port bit-masked filter field to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by 
 	* 			either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] port	The 16-bit value to use as the comparand of the source port filter field.
 	* \param[in] mask	The 16-bit value to use as the mask of the destination port filter field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function is equivalent to dagixp_filter_add_sorce_port_bitmask except it
 	* 			adds a destination rather than a source port filter entry.
 	*/
	int
	dagixp_filter_add_dest_port_bitmask (RuleH rule_h, uint16_t port, uint16_t mask);
	

	/* adds a port range bitmask entry to the rule list */
	/**
 	* \brief		Adds a source port range filter field to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] min_port	The 16-bit value to use as the minimum port value of the source port range.
 	* \param[in] max_port	The 16-bit value to use as the maximum port value of the source port range.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			The port range bounded by the min_port and max_port parameters is inclusive,
 	* 			therefore if you want to filter on a single port value rather than a range, 
 	* 			set both the max_port and min_port parameters to the same value.
 	* 			Up to 254 source and destination port range filters can be added per rule. 
 	* 			Port range filters are not limited to one per rule when loaded into the DAG 
 	* 			card like bit-masked port filters are, therefore a number of port range filters 
 	* 			can be added to a single rule with a minimal reduction in overall filtering 
 	* 			performance.
 	* 			Bit-masked and range port filters can be mixed within a single rule, however as
 	* 			commented in the dagixp_filter_add_source_port_bitmask Function section on page 24,
 	* 			this is generally not a good idea.
 	*/
	int
	dagixp_filter_add_source_port_range (RuleH rule_h, uint16_t min_port, uint16_t max_port);
	
	/**
 	* \brief		Adds a destination port range filter field to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by 
 	* 			either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] min_port	The 16-bit value to use as the minimum port value of the source port range.
 	* \param[in] max_port	The 16-bit value to use as the maximum port value of the source port range.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* /note			This function is equivalent to dagixp_filter_add_source_port_range except it
 	* 			adds a destination rather than a source port filter entry. 
 	*/
	int
	dagixp_filter_add_dest_port_range (RuleH rule_h, uint16_t min_port, uint16_t max_port);



	/* gets source and destination port list details */
	/**
 	* \brief		Returns the number of port filter fields that have been added to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* 
 	* \return		The return value is a positive number, indicating how many port filters have 
 	* 			been added to the rule, if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function returns the total number of port filters added to the rule, 
 	* 			including both source & destination, bit-masked and range type filters. The
 	* 			returned value is typically used to iterate over all the port filters per rule.
 	*/
	int
	dagixp_filter_get_port_list_count (RuleH rule_h);
	
	/**
 	* \brief		Returns the details of a port filter at a given index within a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by 
 	* 			either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] index	The index of the port rule to retrieve.
 	* \param[out] entry	 ointer to a port_entry_t structure that is populated by this function.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function retrieves the details of a port filter that has been added to 
 	* 			a rule, the entry parameter should contain a pointer to a port_entry_t structure 
 	* 			that will be populated with the details of the rule.
 	* 			Details for source & destination as well as bit-masked and ranged port filters 
 	* 			can be  retrieved using this function.
 	*/
	int
	dagixp_filter_get_port_list_entry (RuleH rule_h, uint32_t index, port_entry_t * entry);
	
	

	/* sets the ICMP type filter bitmask */
	/**
 	* \brief		Adds an ICMP type bit-masked filter field to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] type	The 8-bit value to use as the comparand for the ICMP type filter field.
 	* \param[in] mask	The 8-bit value to use as the mask of the ICMP type filter field.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	* 
 	* \note			This function adds a new bit-masked ICMP type filter to the list of ICMP 
 	* 			type filters associated with the rule. Up to 254 ICMP type filter entries 
 	* 			(of both bit-masked and range types) are allowed per rule.
 	* 			ICMP type filters, target the type field of ICMP headers. IPv6-ICMP (protocol 
 	* 			number 58) type packets are not supported by these filters.
 	* 			The list of ICMP type filters is only loaded into the card and activated 
 	* 			(by using the dagixp_filter_download_ruleset and dagixp_filter_activate_ruleset
 	* 			functions) when the rule's protocol field is set to the ICMP protocol number.
 	* 			As with port filter lists, it is not recommended to add multiple bit-masked 
 	* 			ICMP type rules to a single rule.
 	*/
	int
	dagixp_filter_add_icmp_type_bitmask (RuleH rule_h, uint8_t type, uint8_t mask);

	/* sets a range of icmp type filters */
	/**
 	* \brief		Adds an ICMP type range filter field to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] min_type	The 8-bit value to use as the minimum ICMP type value of the range.
 	* \param[in] max_type	The 8-bit value to use as the maximum ICMP type value of the range.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes.
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			The ICMP type range bounded by the min_type and max_type parameters is inclusive,
 	* 			therefore if you want to filter on a single ICMP type value rather than a range, 
 	* 			set both the max_type and min_type parameters to the same value.
 	* 			Up to 254 ICMP type range filters can be added per rule. ICMP type range filters 
 	* 			are not limited to one per rule when loaded into the DAG card like bit-masked ICMP 
 	* 			type filters are, therefore a number of ICMP type range filters can be added to a 
 	* 			single rule with a minimal reduction in overall filtering performance.
 	* 			Bit-masked and range ICMP type filters can be mixed within a single rule, however 
 	* 			as commented in the dagixp_filter_add_source_port_bitmask Function section on page 24,
 	* 			this is generally not a good idea.
 	*/
	int
	dagixp_filter_add_icmp_type_range (RuleH rule_h, uint8_t min_type, uint8_t max_type);


	/* gets icmp type list details */
	/**
 	* \brief		Returns the number of ICMP type filter entries that have been added to the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by either 
 	* 			the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule functions.
 	*
 	* \return		The return value is a positive number, indicating how many ICMP type filters have 
 	* 			been added to the rule, if the operation was successful. Otherwise a negative value 
 	* 			is returned and dagixp_filter_get_last_error will return one of the following error 
 	* 			codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_icmp_type_list_count (RuleH rule_h);
	
	/**
 	* \brief		Returns the details of a ICMP type filter entry at a given index within a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by either 
 	* 			the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule functions.
 	* \param[in] index	The index of the port rule to retrieve.
 	* \param[out] entry	Pointer to a icmp_entry_t structure that is populated by this function. See the
 	* 			icmp_entry_t Structure section on page 31 for more information.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function retrieves the details of a ICMP type filter that has been added to 
 	* 			a rule, the entry parameter should contain a pointer to a icmp_entry_t structure 
 	* 			that will be populated with the details of the rule.
 	*/
	int
	dagixp_filter_get_icmp_type_list_entry (RuleH rule_h, uint32_t index, icmp_entry_t * entry);

	/*@}*/


	/*****************************************************************
	 *
	 *  FILTER META DATA
	 *
	 *****************************************************************/
	/**
 	* \defgroup grpFilterMetaDataIXP IXP Filter Meta Data Functions
 	* Contains functions to modify the non-filtering attributes of a particular rule.
 	*/
	/*@{*/
	/* gets/sets the interface that the filter is applied to */
	/**
 	* \brief		Sets the priority value associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have 
 	* 			been created by either the dagixp_filter_create_ipv4_rule or 
 	* 			dagixp_filter_create_ipv6_rule functions.
 	* \param[in] priority	The 16-bit value to use as the priority for the rule, the lower 
 	* 			the priority value the higher the priority of the rule.
 	*
 	* \return		The return value is zero if the operation was successful. 
 	* 			Otherwise a negative value is returned and dagixp_filter_get_last_error 
 	* 			will return one of the following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function sets the priority of a rule within a ruleset, the lower 
 	* 			the priority parameter value the higher the rule priority. Rules that 
 	* 			have higher priority are compared with incoming packets first.
 	* 			Care should be taken when iterating over the rules in a ruleset whilst 
 	* 			changing the priority of the rule, because rules are priority ordered 
 	* 			within a ruleset, changing the priority of a rule will move it's position
 	* 			within a ruleset.
 	*/
	int
	dagixp_filter_set_rule_priority (RuleH rule_h, uint16_t priority);
	
	/**
 	* \brief		Retrieves the priority value associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[out] priority	Pointer to a variable that receives the 16-bit priority associated with 
 	* 			the rule.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_rule_priority (RuleH rule_h, uint16_t *priority);


	/* gets/sets either an accept or reject action for a filter */
	/**
 	* \brief		Sets the action associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] action	The action to assign to the rule, this can be one of the following constants.
 	* 			- kReject: If the rule hits the packet is rejected (dropped).
 	* 			- kAccept: If the rule hits the packet is accepted, and routed to either the host 
 	* 			or the line based on the rule steering attribute and card configuration.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			The action of the rule determines the course of action to take if the rule produces 
 	* 			a hit for a given packet.
 	*/ 
	int
	dagixp_filter_set_rule_action (RuleH rule_h, action_t action);
	
	/**
 	* \brief		Retrieves the action associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[out] action	Pointer to an action_t variable that receives the action associated with the rule.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_rule_action (RuleH rule_h, action_t *action);


	/* gets/sets the snap length of the filter (if accepted) */
	/**
 	* \brief		Sets the snap length value associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] snap_lenght A 16-bit value that contains the number of bytes to snap the packet length to.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function defines the snap length applied to the packet if the rule hits 
 	* 			and the rule action is set to kAccept. The snap length only applies if the 
 	* 			snap_len value is less than the size of the packet record that produced the 
 	* 			rule hit.
 	*    			The snap length should be a multiple of 8 (this ensures 64-bit alignment of packet
 	*     			records), if not then the snap length is rounded down to the nearest multiple. The
 	*      			minimum snap length allowed is 24 bytes.
 	*       		The snap length defined for a rule is independent of the snap length specified when
 	*        		configuring the DAG card for packet reception. The snap length that can be configured
 	*         		via the DAG Configuration and Status API (or the dagconfig program) is applied to the
 	*          		packet records prior to being present to the IXP filtering software.
 	*/
	int
	dagixp_filter_set_rule_snap_length (RuleH rule_h, uint16_t snap_length);
	
	/**
 	* \brief		Retrieves the snap length associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[out] snap_length Pointer to a 16-bit variable that receives the snap length associated with 
 	* 			the rule.
 	* 
 	* \return		The return value is zero if the operation was successful. Otherwise a negative
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_rule_snap_length (RuleH rule_h, uint16_t *snap_length);


	/* gets/sets the target of the filtered packet (line or host) */
	/**
 	* \brief		Sets the steering attribute associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created by 
 	* 			either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[in] steering	The steering to assign to the rule, this can be one of the following constants.
 	* 			- kHost: The packet record is routed to the host if the rule hits and action 
 	* 			attribute is set kAccept.
 	* 			- kLine: The packet record is routed back out the line if the rule hits and 
 	* 			action attribute is set kAccept.
 	* 
 	* \return		The return value is zero if the operation was successful. Otherwise a negative
 	*  			value is returned and dagixp_filter_get_last_error will return one of the following 
 	*  			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			This function defines the target of a packet record if the rule hits and the 
 	* 			rule action is set to kAccept.
 	*   			For this option to work successfully the settings for packet routing within 
 	*   			the DAG 7.1S card should be set to kSteerDirectionBit. See the IXP Filtering 
 	*   			API Typical Usage section on page 9 for more information. If the packet routing 
 	*   			configuration is set to route packet from the IXP to the host (kSteerHost) or 
 	*   			line (kSteerLine) directly then the steering rule attribute is effectively ignored.
 	*/
	int
	dagixp_filter_set_rule_steering (RuleH rule_h, steering_t steering);

	/**
 	* \brief		Retrieves the steering mode associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been created 
 	* 			by either the dagixp_filter_create_ipv4_rule or dagixp_filter_create_ipv6_rule
 	* 			functions.
 	* \param[out] steering	Pointer to a steering_t variable that receives the steering mode associated 
 	* 			with the rule.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following
 	* 			error codes:
 	*			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_rule_steering (RuleH rule_h, steering_t *steering);


	/* gets/sets the filter tag (value copied into the accepted packet erf header) */
	/**
 	* \brief		Sets the tag value associated with the rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have 
 	* 			been created by either the dagixp_filter_create_ipv4_rule or 
 	* 			dagixp_filter_create_ipv6_rule functions.
 	* \param[in] tag	The new tag value to assign to the rule, only the lower 14 
 	* 			bits of this value are used, the upper 2 bits are always ignored.
 	*
 	* \return		The return value is zero if the operation was successful. 
 	* 			Otherwise a negative value is returned and dagixp_filter_get_last_error 
 	* 			will return one of the following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*
 	* \note			The rule tag is a user defined 14-bit value, which is copied into 
 	* 			the color field of the ERF packet record header if the rule produces 
 	* 			a hit. Multiple rules can have the same tag value, there is no limitation 
 	* 			on the tag value except it must fit within 14-bits. Although it is allowed, 
 	* 			users should avoid setting a rule tag to 0x3FFF, as that is the value
 	* 			defined for the color of packets processed when in loopback mode.
 	*/
	int
	dagixp_filter_set_rule_tag (RuleH rule_h, uint16_t tag);

	/**
 	* \brief		Retrieves the tag value associated with a rule.
 	*
 	* \param[in] rule_h	Handle to either an IPv4 or IPv6 rule. The rule should have been 
 	* 			created by either the dagixp_filter_create_ipv4_rule or 
 	* 			dagixp_filter_create_ipv6_rule functions.
 	* \param[out] tag	Pointer to a variable that receives the 14-bit rule tag associated 
 	* 			with the rule.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise 
 	* 			a negative value is returned and dagixp_filter_get_last_error will 
 	* 			return one of the following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	*/
	int
	dagixp_filter_get_rule_tag (RuleH rule_h, uint16_t *tag);
	
	/*@}*/





	/*****************************************************************
	 *
	 *  FILTER LOADING
	 *
	 *****************************************************************/
	/**
 	* \defgroup grpFilterLoadIXP IXP Filter Loading Functions
 	* Contains the necessary functionality to load a ruleset into a DAG 7.1S card and
 	* activate it.
 	*/
	/*@{*/

	/* downloads a ruleset to a card */
	/**
 	* \brief		Loads a ruleset into a DAG card.
 	*
 	* \param[in] dagfd	DAG file descriptor provided by dag_open.
 	* \param[in] iface	This parameter should always contain the kAllInterfaces constant.
 	* \param[in] ruleset	Handle to the ruleset to load into the card.
 	*
 	* \return		The return value is zero if the operation was successful. 
 	* 			Otherwise a negative value is returned and dagixp_filter_get_last_error 
 	* 			will return one of the following error  codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	* 			- EBADF : bad file descriptor, usually caused by a missing EMA connection
 	* 			- ERESP : corrupt message received from the IXP
 	* 			- EUNSUPPORTDEV : the dagfd parameter refers to a non-DAG 7.1S card
 	*
 	* \note			Once a ruleset is loaded into a card, it remains in the card until either
 	* 			dagixp_filter_remove_ruleset or dagixp_filter_clear_iface_rulesets is
 	* 			called, at any time in the future it can be activated by calling
 	* 			dagixp_filter_activate_ruleset. The amount of memory available for storing
 	* 			the rulesets on the card is limited, so avoid maintaining a number of rulesets 
 	* 			on the card, and remove rulesets from the card when they are no longer needed.
 	* 			Internally the IXP filtering library maintains a list of ruleset that have been 
 	* 			loaded into the card, this list is lost when the library is shutdown. Therefore 
 	* 			rulesets that have been downloaded be a previous process can't be reactivated by 
 	* 			a new process, similarly it is also recommended to clear all the rulesets on the 
 	* 			card when starting a new filter session, this will remove all redundant rulesets 
 	* 			left over from a previous process.
 	*/
	int
	dagixp_filter_download_ruleset (int dagfd, uint8_t iface, RulesetH ruleset);
	
	/* activates a downloaded ruleset */
	/**
 	* \brief		Activates a ruleset on a DAG card.
 	*
 	* \param[in] dagfd	DAG file descriptor provided by dag_open.
 	* \param[in] iface	This parameter should always contain the kAllInterfaces constant.
 	* \param[in] ruleset	Handle to the ruleset to activate.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative
 	* 		 	value is returned and dagixp_filter_get_last_error will return one of the following
 	* 		 	error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	* 			- EBADF : bad file descriptor, usually caused by a missing EMA connection
 	* 			- ERESP : corrupt message received from the IXP
 	* 			- EUNSUPPORTDEV : the dagfd parameter refers to a non-DAG 7.1S card
 	*
 	* \note			To activate a ruleset on the DAG card, it should have previously been loaded 
 	* 			into the card by a successful call to dagixp_filter_download_ruleset.
 	* 			When a ruleset is activated, the rules contained within the ruleset are copied into
 	*			microcode (packet processing) memory space and the packet processing programs are
 	* 			started. From now onwards packets are processed based on the rules contained in the
 	* 			ruleset. The filtering process is now running independent of the host, for example 
 	* 			the ruleset can be deleted and the library closed and the filtering will continue.
 	* 			It is possible to download a ruleset to the DAG card while it is active, however any
 	* 			changes in the ruleset won't be effective until dagixp_filter_activate_ruleset is
 	* 			called on the ruleset again. If a different ruleset is currently activated, this 
 	* 			function will deactivate the current one and activate the new one.
 	* 			The process of activating a filter is not an instantaneous one, during the time it 
 	* 			takes, the filtering software is put in loop back mode. See the Modes of Operation 
 	* 			section on page 5 for more information.
 	*/
	int
	dagixp_filter_activate_ruleset (int dagfd, uint8_t iface, RulesetH ruleset);
	
	/* removes a downloaded ruleset from the card */
	/**
 	* \brief		Removes a ruleset from the card.
 	* 
 	* \param[in] dagfd	DAG file descriptor provided by dag_open.
 	* \param[in] iface	This parameter should always contain the kAllInterfaces constant.
 	* \param[in] ruleset	Handle to the ruleset to remove.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the following 
 	* 			error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	* 			- EBADF : bad file descriptor, usually caused by a missing EMA connection
 	* 			- ERESP : corrupt message received from the IXP
 	* 			- EUNSUPPORTDEV : the dagfd parameter refers to a non-DAG 7.1S card
 	*
 	* \note			This function removes a ruleset from the DAG card and frees the memory allocated 
 	* 			for it within the card. This function doesn't destroy the ruleset handle maintained 
 	* 			by the API; it just removes it from the card.
 	* 			Any downloaded ruleset can be removed from the card, including the current active one.
 	* 			If the active ruleset is removed the filtering will continue as before, this function only
 	* 			removes the memory associated with the ruleset on the card, not the actual rules
 	* 			currently used by the packet processing microcode.
 	*/
	int
	dagixp_filter_remove_ruleset (int dagfd, uint8_t iface, RulesetH ruleset);
	
	/* removes all downloaded ruleset from the card */
	/**
 	* \brief		Removes all the rulesets from a DAG card and puts it loop back mode.
 	*
 	* \param[in] dagfd	DAG file descriptor provided by dag_open.
 	* \param[in] iface	This parameter should always contain the kAllInterfaces constant.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	* 			- EBADF : bad file descriptor, usually caused by a missing EMA connection
 	* 			- ERESP : corrupt message received from the IXP
 	* 			- EUNSUPPORTDEV : the dagfd parameter refers to a non-DAG 7.1S card
 	*
 	* \note			This function removes all the ruleset from the DAG card and frees the memory 
 	* 			allocated within the card then puts it in loopback mode. This function is typically 
 	* 			called when the library is first opened to remove any stale rulesets present in the card.
 	*/
	int
	dagixp_filter_clear_iface_rulesets (int dagfd, uint8_t iface);
	
	/*@}*/
	

	/*****************************************************************
	 *
	 *  FEATURE REQUEST
	 *
	 *****************************************************************/
	/**
 	* \defgroup grpFeatureReqIXP IXP Feature Request Functions
 	* Provides the functionality to retrieve the current statistics associated with the
 	* filtering process from the DAG card.
 	*/
	/*@{*/

	/* returns basic filtering statistics */
	/**
 	* \brief		Gets the contents of a filter statistic from a DAG card.
 	*
 	* \param[in] dagfd	DAG file descriptor provided by dag_open.
 	* \param[in] stat	The identifier of a statistic.
 	* \param[out] stat_low	Pointer to a 32-bit variable that receives the lower 32-bits of the statistic.
 	* \param[out] stat_high Pointer to a 32-bit variable that receives the upper 32-bits of the statistic. 
 	* 			This parameter can be NULL if getting a 32-bit statistic counter.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	* 			- EBADF : bad file descriptor, usually caused by a missing EMA connection
 	* 			- ERESP : corrupt message received from the IXP
 	* 			- EUNSUPPORTDEV : the dagfd parameter refers to a non-DAG 7.1S card
 	*/
	int
	dagixp_filter_hw_get_filter_stat (int dagfd, statistic_t stat, uint32_t *stat_low, uint32_t *stat_high);
	
	/* resets a filter statistic stored by the card */
	/**
 	* \brief		Resets a filtering statistic on a DAG card.
 	*
 	* \param[in] dagfd	DAG file descriptor provided by dag_open.
 	* \param[in] stat	The identifier of a statistic.
 	*
 	* \return		The return value is zero if the operation was successful. Otherwise a negative 
 	* 			value is returned and dagixp_filter_get_last_error will return one of the 
 	* 			following error codes:
 	* 			- ENOTOPEN : the library hasn't been initialised, call dagixp_filter_startup
 	* 			- EINVAL : invalid parameter
 	* 			- EBADF : bad file descriptor, usually caused by a missing EMA connection
 	* 			- ERESP : corrupt message received from the IXP
 	* 			- EUNSUPPORTDEV : the dagfd parameter refers to a non-DAG 7.1S card
 	*
 	* \note			The kAllStatistics constant can be past to this function to reset all the statistics in
 	* 			one go, however internally the API resets one statistic at a time and therefore cannot
 	* 			guarantee that the statistics will all be cleared atomically.
 	*/
	int
	dagixp_filter_hw_reset_filter_stat (int dagfd, statistic_t stat);
	
	/*@}*/



	/*****************************************************************
	 *
	 *  UTILITY FUNCTIONS
	 *
	 *****************************************************************/
	/**
 	* \defgroup grpUtiltyIXP IXP Utility Functions
 	* Provides miscellaneous utilities for the IXP Filtering API
 	*/
	/*@{*/

	/* returns the last error code generated */
	/**
 	* \brief		Gets the value of the last error code generated by an IXP filtering API function.
 	*
 	* \return 		The returned value is the last generated error code or 0 if no error was generated.
 	*
 	* \note			When any of the dagixp_filter_functions are called, they internally reset the last
 	* 			error value to 0; therefore the last error code will not persist across multiple IXP 
 	* 			filtering function calls.
 	*/
	int
	dagixp_filter_get_last_error (void);


	/** this is used for debugging, basically it accepts an IP packet and runs it
	 * through the set of filters currently installed for a ruleset, upon a filter
	 * match the filter tag is returned.
	 */
	RuleH
	dagixp_filter_test_ip_packet (RulesetH ruleset_h, uint8_t *packet_buf, uint32_t rlen);

	/*@}*/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif      // DAGFILTER_H
/*@}*/
