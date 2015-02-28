/**********************************************************************
*
* Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: aal_config_msg.h,v 1.17 2009/03/16 06:44:42 wilson.zhu Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         aal_config_msg.h
* DESCRIPTION:	Header file for the aal_config.c file that contains the
*               message wrappers for AAL messages.
*
*
* HISTORY: 
*   	18/7/05 - Initial version
*
**********************************************************************/

#include "dagsarapi.h"

#define MAX_PADDING				47
#define AAL5_TRAILER_SIZE		8
#define AAL5_ERF_HEADER_SIZE	24
#define TEN_MIB					10485760

typedef enum 
{
    DAG_EQ = 0,     /* 0 equal */
    DAG_NEQ,        /* 1 not equal */
    DAG_LE,         /* 2 less than or equal */
    DAG_GE,         /* 3 greater than or equal */
    DAG_LT,         /* 4 less than */
    DAG_GT,         /* 5 greater than */
    DAG_AND,        /* 6 bitwise and */
    DAG_OR,         /* 7 bitwise or */
    DAG_XOR,        /* 8 bitwise exlusive-or */
    
    DAG_NUM_FILTER_OPERATIONS    /* this has to be the last in list */
}filter_operations_t;


typedef enum 
{
    DAG_FILTER_LEVEL_BOARD = 0,
    DAG_FILTER_LEVEL_CHANNEL,
    
    DAG_NUM_FILTER_LEVELS   /* this has to be the last in list */
}dag_filter_level_t;

typedef enum
{
	DAG_OR_LIST = 0,
	DAG_AND_LIST

}list_operations_t;

/**********************************************************************
* FUNCTION:     aal_msg_activate_vc
* DESCRIPTION:	activate a vc to return data from the 3.7t
* INPUTS:	
*	dagc - dag device descriptor
*   iface - interface (used by non 3.7t cards)
*	VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
* OUTPUTS:	None
* RETURNS:	0 if created
*           1 if could not be created
**********************************************************************/
uint32_t aal_msg_activate_vc(uint32_t dagfd, 
                             uint32_t iface,
						     uint32_t VCI, 
						     uint32_t VPI,
						     uint32_t connection_num);

/**********************************************************************
* FUNCTION:     aal_msg_remove_vc
* DESCRIPTION:	remove a Virtual Connection from the list
* INPUTS:	
*	dagc - dag device descriptor
*   iface - interface (used by non 3.7t cards)
*	VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
* OUTPUTS:	None
* RETURNS:	0 if successful, 1 if unsuccessful
**********************************************************************/
uint32_t aal_msg_remove_vc(uint32_t dagfd,
                           uint32_t iface,
                           uint32_t VCI, 
                           uint32_t VPI,
						   uint32_t connection_num);

/**********************************************************************
* FUNCTION:     aal_msg_activate_cid
* DESCRIPTION:	activate a cid on the aal reassembler to
*				start sending all data it sees to the host.
* INPUTS:	
*	dagc - dag device descriptor
*   iface - interface (used by non 3.7t cards)
*	VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
*   cid - cid to be activated
* OUTPUTS:	None
* RETURNS:	0 if activated
*           1 if could not be activated
**********************************************************************/
uint32_t aal_msg_activate_cid(uint32_t dagfd,
						      uint32_t iface,
                              uint32_t VCI, 
                              uint32_t VPI,
							  uint32_t connection_num,
							  uint32_t cid);

/**********************************************************************
* FUNCTION:     aal_msg_deactivate_cid
* DESCRIPTION:	deactivate a cid on the reassembler
* INPUTS:	
*	dagc - dag device descriptor
*   iface - interface (used by non 3.7t cards)
*	VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
*   cid - cid to be deactivated
* OUTPUTS:	None
* RETURNS:	0 if successful, 1 if unsuccessful
**********************************************************************/
uint32_t aal_msg_deactivate_cid(uint32_t dagfd,
							    uint32_t iface,
                                uint32_t VCI, 
                                uint32_t VPI,
						    	uint32_t connection_num,
							    uint32_t cid);

/**********************************************************************
* FUNCTION:	aal_msg_report_staistics
* DESCRIPTION:	get the specified statistic from the Xscale
* INPUTS:	
*	dagc - dag device descriptor
*	statistic - specifies the statistic to return
* OUTPUTS:	None
* RETURNS:	the value of the specified statistic
**********************************************************************/
uint32_t aal_msg_report_statistics(uint32_t dagfd, uint32_t statistic);

/**********************************************************************
* FUNCTION:	aal_msg_reset_statistics
* DESCRIPTION:	This sets a single statistic to zero
* INPUTS:	
*	dagc - dag device descriptor
*	statistic - identifiers the statistic to be zeroed
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_statistics(uint32_t dagfd, uint32_t statistic);

/**********************************************************************
* FUNCTION:	aal_msg_reset_statistics_all
* DESCRIPTION:	This sets all the statistics to zero
* INPUTS:	
*	dagc - dag device descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_statistics_all(uint32_t dagfd);

/**********************************************************************
* FUNCTION:	aal_msg_set_connection_net_mode
* DESCRIPTION:	set the mode (uni/nni) of the given vc
* INPUTS:	
*	dagc - dag device descriptor
*	net_mode - net mode to set
*	connection_num - identifiers to the virtual channel to be set
*	
* OUTPUTS:	None
* RETURNS:	0 on sucess, non-zero if unsuccessful;
**********************************************************************/
uint32_t aal_msg_set_connection_net_mode(uint32_t dagfd, 
										 net_mode_t net_mode, 
										 uint32_t iface,
										 uint32_t connection_num);

/**********************************************************************
* FUNCTION:     aal_msg_set_iface_net_mode
* DESCRIPTION:  set the mode (uni/nni) of the given interface
* INPUTS:	
*	dagfd - dag card descriptor
*	net_mode - net mode to set
*   iface - interface (used by non 3.7t cards)
*	
* OUTPUTS:	None
* RETURNS:	0 on sucess, non-zero if unsuccessful;
**********************************************************************/
uint32_t aal_msg_set_iface_net_mode(uint32_t dagfd, 
				    				net_mode_t net_mode, 
									uint32_t iface);

/**********************************************************************
* FUNCTION:	aal_msg_get_sar_mode
* DESCRIPTION:	gets the sar mode for a vc
* INPUTS:	
*	dagc - dag device descriptor
*   iface - interface (used by non 3.7t cards)
*	connection_num, vpi, vci - identifiers to the virtual 
*								channel to be get
*	
* OUTPUTS:	None
* RETURNS:	the sar mode for the vc;
**********************************************************************/
uint32_t aal_msg_get_sar_mode(uint32_t dagfd,
                             uint32_t iface,
							 uint32_t connection_num,
							 uint32_t vpi,
							 uint32_t vci);

/**********************************************************************
* FUNCTION:	aal_msg_set_sar_mode
* DESCRIPTION:	This is not a direct message to the 3.7t. It creates a 
*				new vc setting using a default value for the max_size 
*				by calling d37t_add_vc
* INPUTS:	
*	dagc - dag device descriptor
*   iface - interface (used by non 3.7t cards)
*	connection_num, vpi, vci - identifiers to the virtual 
*								channel to be set
*	sar_mode - the type of channel to create
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_sar_mode(uint32_t dagfd, 
                              uint32_t iface,
                              uint32_t vci, 
                              uint32_t vpi,
                              uint32_t channel, 
                              sar_mode_t sar_mode);
							  
/**********************************************************************
* FUNCTION:	aal_msg_set_buffer_size
* DESCRIPTION:	This sets the internal version of the size of the buffer
*				allocated for new vcs to reassemble aal5 cells
* INPUTS:	
*	dagc - dag device descriptor
*	size - the size of the channels to create
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_buffer_size(uint32_t dagfd, uint32_t size);

/**********************************************************************
* FUNCTION:     aal_msg_set_filter
* DESCRIPTION:	This sets the filter for accepting or rejecting packets
*				based on the values given by the user.
* INPUTS:	
*	dagc - dag device descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_filter(uint32_t dagfd, uint32_t offset,
	uint32_t mask, uint32_t value, uint32_t operation,
	uint32_t filter_level, uint32_t level_conf,
	uint32_t history, uint32_t priority);

/**********************************************************************
* FUNCTION:     hdlc_msg_set_filter
* DESCRIPTION:	This sets the filter for accepting or rejecting packets
*				based on the values given by the user.
* INPUTS:	
*	dagc - dag device descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_set_filter(uint32_t dagfd, uint32_t offset,
	uint32_t mask, uint32_t value, uint32_t operation,
	uint32_t filter_level, uint32_t level_conf,
	uint32_t history, uint32_t priority);

/**********************************************************************
* FUNCTION:	aal_msg_set_filter_action
* DESCRIPTION:	This sets whether the filter will cause packets to be 
*				accepted or rejected
* INPUTS:	
*	dagc - dag device descriptor
*	action - whether matching packets should be accepted or rejected
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_filter_action(uint32_t dagfd, uint32_t action);

/**********************************************************************
* FUNCTION:	hdlc_msg_set_filter_action
* DESCRIPTION:	This sets whether the filter will cause packets to be 
*				accepted or rejected
* INPUTS:	
*	dagc - dag device descriptor
*	action - whether matching packets should be accepted or rejected
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_set_filter_action(uint32_t dagfd, uint32_t action);

/**********************************************************************
* FUNCTION:     aal_msg_set_subfilter
* DESCRIPTION:	This sets a subfilter for accepting or rejecting packets
*				based on the values given by the user, at a level below the
*				filter as specifed, with the given operation
* INPUTS:	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
*	parent_id - parent filter to base this subfilter from
*	list_operation - specifies if the subfilter list should be OR or AND
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_subfilter(uint32_t dagfd, uint32_t offset,
	uint32_t mask, uint32_t value, uint32_t operation,
	uint32_t filter_level, uint32_t level_conf, uint32_t history,
	uint32_t priority, uint32_t parent_id, 
	list_operations_t list_operation);

/**********************************************************************
* FUNCTION:     hdlc_msg_set_subfilter
* DESCRIPTION:	This sets a subfilter for accepting or rejecting packets
*				based on the values given by the user, at a level below the
*				filter as specifed, with the given operation
* INPUTS:	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
*	parent_id - parent filter to base this subfilter from
*	list_operation - specifies if the subfilter list should be OR or AND
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_set_subfilter(uint32_t dagfd, uint32_t offset,
	uint32_t mask, uint32_t value, uint32_t operation,
	uint32_t filter_level, uint32_t level_conf, uint32_t history,
	uint32_t priority, uint32_t parent_id, 
	list_operations_t list_operation);

/**********************************************************************
* FUNCTION:     aal_msg_set_filter
* DESCRIPTION:	This sets the filter for accepting or rejecting packets
*				based on the values given by the user.
* INPUTS:	
*	dagfd - dag card descriptor
*	iface - interface where to apply the filter
*	bitmask - gives the bitmask for the filter
*	match - gives the match value for after the bitmask has been applied
*	filter_action - action to apply to matching values
*
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_filter_bitmask (uint32_t dagfd,
									 uint32_t iface,
									 uint32_t bitmask,
									 uint32_t match,
									 uint32_t filter_action);


/**********************************************************************
* FUNCTION:	aal_msg_reset_filter
* DESCRIPTION:	This stops a filter from filtering
* INPUTS:	
*	dagc - dag device descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_filter(uint32_t dagfd, uint32_t filter_id);

/**********************************************************************
* FUNCTION:	hdlc_msg_reset_filter
* DESCRIPTION:	This stops a filter from filtering
* INPUTS:	
*	dagc - dag device descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_reset_filter(uint32_t dagfd, uint32_t filter_id);

/**********************************************************************
* FUNCTION:	aal_msg_reset_all_filters
* DESCRIPTION:	This stops filtering from occuring
* INPUTS:	
*	dagc - dag device descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_all_filters(uint32_t dagfd);

/**********************************************************************
* FUNCTION:	hdlc_msg_reset_all_filters
* DESCRIPTION:	This stops filtering from occuring
* INPUTS:	
*	dagc - dag device descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_reset_all_filters(uint32_t dagfd);

/**********************************************************************
* FUNCTION:	aal_msg_request_vc_list 
* DESCRIPTION:	Find the virtual connections which have data on them
* INPUTS:	
*	dagc - dag device descriptor
*	length - maximum length of virtual connection information to receive
* OUTPUTS:	VCList is passed in as a NULL pointer and if successful 
*   returns as an array of VC's. Length can also be made smaller to show
*   the exact number of bytes copied
* RETURNS:	TRUE if VCList does not equal NULL, FALSE otherwise.
**********************************************************************/
//uint32_t aal_msg_request_vc_list(uint32_t dagfd, uint32_t* length, uint32_t* VCList);

/**********************************************************************
* FUNCTION:     aal_msg_init_scanning_mode 
* DESCRIPTION:	init the AAL Reassembler scanning mode
* INPUTS:	
*	dagfd - dag card descriptor
*	mode  - mode to set
* OUTPUTS:	None
* RETURNS:	0 on success, 1 on failure
**********************************************************************/
uint32_t aal_msg_init_scanning_mode(uint32_t dagfd);

/**********************************************************************
* FUNCTION:	aal_msg_set_scanning_mode 
* DESCRIPTION:	set the mode of the AAL Reassembler to scanning or reassembly
* INPUTS:	
*	dagc - dag device descriptor
*	mode  - mode to set
* OUTPUTS:	None
* RETURNS:	0 on success, 1 on failure
**********************************************************************/
uint32_t aal_msg_set_scanning_mode(uint32_t dagfd, uint32_t mode);

/**********************************************************************
* FUNCTION:     aal_msg_get_scanning_mode 
* DESCRIPTION:	get the mode of the AAL Reassembler 
* INPUTS:	
*	dagc - dag device descriptor
* OUTPUTS:	None
* RETURNS:	mode from 3.7t
**********************************************************************/
uint32_t aal_msg_get_scanning_mode(uint32_t dagfd);

/**********************************************************************
* FUNCTION:     aal_msg_request_vc_list 
* DESCRIPTION:	Find the virtual connections that scanning has discovered
* INPUTS:	
*	dagc - dag device descriptor
*	length - maximum length of virtual connection information to receive
* OUTPUTS:	VCList is passed in as a NULL pointer and if successful 
*   returns as an array of VC's. Length can also be made smaller to show
*   the exact number of bytes copied
* RETURNS:	0 if VCList does not equal NULL, otherwise error.
**********************************************************************/
uint32_t aal_msg_request_vc_list(uint32_t dagfd, uint32_t* length, uint32_t* pVCList);

/**********************************************************************
* FUNCTION:     aal_msg_get_scanned_connections_number
* DESCRIPTION:	get the number of connections observed in the scan mode
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:	'num of conns' on success, <0 on failure
**********************************************************************/
uint32_t aal_msg_get_scanned_connections_number(uint32_t dagfd);

/**********************************************************************
* FUNCTION:     aal_msg_get_scanned_connection 
* DESCRIPTION:	get a scanned connection
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	
* RETURNS:	
**********************************************************************/
uint32_t aal_msg_get_scanned_connection(uint32_t dagfd,
								uint32_t index,
								connection_info_t *pConnection_info);

/**********************************************************************
* FUNCTION:     aal_msg_set_burst_size(dagc, size)
* DESCRIPTION:	Allows the burst size to be altered.  This is the amount
* of data that will be received by the host at once.
* INPUTS:	
*	dagc - dag device descriptor
*	size - burst size in bytes (72 - 10485760 (10MiB))
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_set_burst_size(uint32_t dagfd, uint32_t size);

/**********************************************************************
* FUNCTION:     aal_msg_flush_burst_buffer(dagc)
* DESCRIPTION:	Forces all data ready to be sent to host to be sent
* INPUTS:	
*	dagc - dag device descriptor
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_flush_burst_buffer(uint32_t dagfd);

/**********************************************************************
* FUNCTION:	    aal_msg_check_connection_num(connection_num)
* DESCRIPTION:	Checks the connection num is within the valid range (9 bit value)
* INPUTS:	
*	connection_num - input connection_num to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_connection_num(uint32_t connection_num);

/**********************************************************************
* FUNCTION:	    aal_msg_check_vpi(VPI)
* DESCRIPTION:	Checks the VPI is within the valid range (12 bit value)
* INPUTS:	
*	VPI - input VPI to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_vpi(uint32_t VPI);

/**********************************************************************
* FUNCTION:	    aal_msg_check_vci(VCI)
* DESCRIPTION:	Checks the VCI is within the valid range (16bit value)
* INPUTS:	
*	VPI - input VPI to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_vci(uint32_t VCI);

/**********************************************************************
* FUNCTION:	    aal_msg_check_interface(Iface)
* DESCRIPTION:	Checks the Interface is within the valid range (2 bit value)
* INPUTS:	
*	Iface - input Iface to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_interface(uint32_t Iface);

/**********************************************************************
* FUNCTION:     aal_msg_set_trailer_strip_mode 
* DESCRIPTION:	set the AAL5 trailer strip mode for the AAL5 reassembler
* INPUTS:	
*	dagfd - dag card descriptor
*	mode  - mode to set
* OUTPUTS:	None
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_set_trailer_strip_mode(uint32_t dagfd, uint32_t mode);

/**********************************************************************
* FUNCTION:	    aal_msg_get_trailer_strip_mode 
* DESCRIPTION:	gets the AAL5 trailer strip mode from the AAL5 reassembler
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_get_trailer_strip_mode(uint32_t dagfd);

/**********************************************************************
* FUNCTION:	    aal_msg_set_atm_forwarding_mode 
* DESCRIPTION:	When enabled all unconfigured ATM connections will forward
*               the raw ATM cells onto the FPGA. By default this option
*               is disabled.
* INPUTS:	
*	dagfd - dag card descriptor
*   mode  - 
* OUTPUTS:	None
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_set_atm_forwarding_mode (uint32_t dagfd, uint32_t mode);

/**********************************************************************
* FUNCTION:	    aal_msg_get_atm_forwarding_mode 
* DESCRIPTION:	Gets the current unconfigured ATM forwarding mode
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	The current forwarding mode
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_get_atm_forwarding_mode (uint32_t dagfd);

