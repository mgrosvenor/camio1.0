/**********************************************************************
*
* Copyright (c) 2004 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: d71s_filter.h,v 1.3 2006/03/02 02:28:46 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
 * FILE:        d71s_filter.h
 * DESCRIPTION: Defines the functions used for processing the filter api
 *              messages.
 *
 *
 * HISTORY: 
 *      05/10/05 - Initial version
 *
 **********************************************************************/
#ifndef D71S_FILTER_H
#define D71S_FILTER_H



/*
 * 
 */
 
 
/* Set the default timeout to 1 second */
#define DEFAULT_EMA_TIMEOUT               1000



/*
 * Filter Messages 
 */
#define EMA_MSG_CREATE_RULESET                     (EMA_CLASS_IXPFILTER  | 0x0001)
#define EMA_MSG_CREATE_RULESET_CMPLT               (EMA_CLASS_IXPFILTER  | 0x0002)
#define EMA_MSG_DELETE_RULESET                     (EMA_CLASS_IXPFILTER  | 0x0003)
#define EMA_MSG_DELETE_RULESET_CMPLT               (EMA_CLASS_IXPFILTER  | 0x0004)
#define EMA_MSG_RESET_RULESET                      (EMA_CLASS_IXPFILTER  | 0x0005)
#define EMA_MSG_RESET_RULESET_CMPLT                (EMA_CLASS_IXPFILTER  | 0x0006)
#define EMA_MSG_ACTIVATE_RULESET                   (EMA_CLASS_IXPFILTER  | 0x0007)
#define EMA_MSG_ACTIVATE_RULESET_CMPLT             (EMA_CLASS_IXPFILTER  | 0x0008)
#define EMA_MSG_ADD_IP_FILTER                      (EMA_CLASS_IXPFILTER  | 0x0009)
#define EMA_MSG_ADD_IP_FILTER_CMPLT                (EMA_CLASS_IXPFILTER  | 0x000A)
#define EMA_MSG_REMOVE_IP_FILTER                   (EMA_CLASS_IXPFILTER  | 0x000B)
#define EMA_MSG_REMOVE_IP_FILTER_CMPLT             (EMA_CLASS_IXPFILTER  | 0x000C)
#define EMA_MSG_SET_PORT_FILTER_RANGE              (EMA_CLASS_IXPFILTER  | 0x000D)
#define EMA_MSG_SET_PORT_FILTER_RANGE_CMPLT        (EMA_CLASS_IXPFILTER  | 0x000E)
#define EMA_MSG_SET_PORT_FILTER_BITMASK            (EMA_CLASS_IXPFILTER  | 0x000F)
#define EMA_MSG_SET_PORT_FILTER_BITMASK_CMPLT      (EMA_CLASS_IXPFILTER  | 0x0010)
#define EMA_MSG_CLEAR_PORT_FILTERS                 (EMA_CLASS_IXPFILTER  | 0x0011)
#define EMA_MSG_CLEAR_PORT_FILTERS_CMPLT           (EMA_CLASS_IXPFILTER  | 0x0012)
#define EMA_MSG_GET_STATISTICS                     (EMA_CLASS_IXPFILTER  | 0x0013)
#define EMA_MSG_GET_STATISTICS_CMPLT               (EMA_CLASS_IXPFILTER  | 0x0014)
#define EMA_MSG_RESET_STATISTICS                   (EMA_CLASS_IXPFILTER  | 0x0015)
#define EMA_MSG_RESET_STATISTICS_CMPLT             (EMA_CLASS_IXPFILTER  | 0x0016)
#define EMA_MSG_REMOVE_ALL_RULESETS                (EMA_CLASS_IXPFILTER  | 0x0017)
#define EMA_MSG_REMOVE_ALL_RULESETS_CMPLT          (EMA_CLASS_IXPFILTER  | 0x0018)
#define EMA_MSG_DUPLICATE_IP_FILTER                (EMA_CLASS_IXPFILTER  | 0x0019)
#define EMA_MSG_DUPLICATE_IP_FILTER_CMPLT          (EMA_CLASS_IXPFILTER  | 0x001A)

//#define EMA_MSG_CREATE_RULESET_SIZE                sizeof(t_filter_msg_create_rulset)
#define EMA_MSG_CREATE_RULESET_CMPLT_SIZE          sizeof(t_filter_msg_create_rulset_cmplt)
#define EMA_MSG_DELETE_RULESET_SIZE                sizeof(t_filter_msg_delete_rulset)
#define EMA_MSG_DELETE_RULESET_CMPLT_SIZE          sizeof(t_filter_msg_delete_rulset_cmplt)
#define EMA_MSG_RESET_RULESET_SIZE                 sizeof(t_filter_msg_reset_rulset)
#define EMA_MSG_RESET_RULESET_CMPLT_SIZE           sizeof(t_filter_msg_reset_rulset_cmplt)
#define EMA_MSG_ACTIVATE_RULESET_SIZE              sizeof(t_filter_msg_activate_rulset)
#define EMA_MSG_ACTIVATE_RULESET_CMPLT_SIZE        sizeof(t_filter_msg_activate_rulset_cmplt)
#define EMA_MSG_ADD_IP_FILTER_SIZE                 sizeof(t_filter_msg_add_filter)
#define EMA_MSG_ADD_IP_FILTER_CMPLT_SIZE           sizeof(t_filter_msg_add_filter_cmplt)
#define EMA_MSG_REMOVE_IP_FILTER_SIZE              sizeof(t_filter_msg_remove_ip_filter)
#define EMA_MSG_REMOVE_IP_FILTER_CMPLT_SIZE        sizeof(t_filter_msg_remove_ip_filter_cmplt)
#define EMA_MSG_SET_PORT_FILTER_RANGE_SIZE         sizeof(t_filter_msg_set_port_filter_range)
#define EMA_MSG_SET_PORT_FILTER_RANGE_CMPLT_SIZE   sizeof(t_filter_msg_set_port_filter_range_cmplt)
#define EMA_MSG_SET_PORT_FILTER_BITMASK_SIZE       sizeof(t_filter_msg_set_port_filter_bitmask)
#define EMA_MSG_SET_PORT_FILTER_BITMASK_CMPLT_SIZE sizeof(t_filter_msg_set_port_filter_bitmask_cmplt)
#define EMA_MSG_CLEAR_PORT_FILTERS_SIZE            sizeof(t_filter_msg_clear_port_filters)
#define EMA_MSG_CLEAR_PORT_FILTERS_CMPLT_SIZE      sizeof(t_filter_msg_clear_port_filters_cmplt)
#define EMA_MSG_GET_STATISTICS_SIZE                sizeof(t_filter_msg_get_statistics)
#define EMA_MSG_GET_STATISTICS_CMPLT_SIZE          sizeof(t_filter_msg_get_statistics_cmplt)
#define EMA_MSG_RESET_STATISTICS_SIZE              sizeof(t_filter_msg_reset_statistics)
#define EMA_MSG_RESET_STATISTICS_CMPLT_SIZE        sizeof(t_filter_msg_reset_statistics_cmplt)
#define EMA_MSG_REMOVE_ALL_RULESETS_SIZE           sizeof(t_filter_msg_remove_all_rulesets)
#define EMA_MSG_REMOVE_ALL_RULESETS_CMPLT_SIZE     sizeof(t_filter_msg_remove_all_rulesets_cmplt)
#define EMA_MSG_DUPLICATE_IP_FILTER_SIZE           sizeof(t_filter_msg_duplicate_ip_filter)
#define EMA_MSG_DUPLICATE_IP_FILTER_CMPLT_SIZE     sizeof(t_filter_msg_duplicate_ip_filter_cmplt)


/* Possible error codes */
#define ERROR_NONE                          0
#define ERROR_INVALID_RULESET               1001
#define ERROR_NON_UNIQUE_ID                 1002
#define ERROR_INVALID_ARG                   1003
#define ERROR_NO_FILTER                     1004
#define ERROR_NO_MEM                        1005
#define ERROR_TOO_MANY_RULESETS             1006
#define ERROR_WRITE_PORT_FILTERS_FAILED     1007
#define ERROR_CONSTRUCT_TREE_FAILED         1008
#define ERROR_TIMED_OUT                     1009





/* Tells the xScale to create a new ruleset a ruleset */
//typedef struct
//{
//} t_filter_msg_create_rulset;

typedef struct 
{
	uint16_t    ruleset;
	uint16_t    padding;
	uint32_t    result;
} t_filter_msg_create_rulset_cmplt;


/* Tells the xScale to create a new ruleset a ruleset */
typedef struct 
{
	uint16_t    ruleset;
	uint16_t    padding;
} t_filter_msg_delete_rulset;

typedef struct
{
	uint32_t    result;
} t_filter_msg_delete_rulset_cmplt;




/* Tells the xScale to remove all the filters from a ruleset */
typedef struct
{
	uint16_t    ruleset;
	uint16_t    padding;
} t_filter_msg_reset_rulset;

typedef struct
{
	uint32_t    result;
} t_filter_msg_reset_rulset_cmplt;



/* Tells the xScale to activate a filter ruleset */
typedef struct
{
	uint16_t    ruleset;
	uint16_t    padding;
} t_filter_msg_activate_rulset;

typedef struct
{
	uint32_t    result;
} t_filter_msg_activate_rulset_cmplt;



/* Tells the xScale to add the filter to a rulset */
typedef struct
{

	/* Filter details */
	
	uint32_t    unique_id;                     // Each filter must have a unique ID, this is
	                                           // used to add port filters.
	uint32_t    priority;                      // Lower numbers have higher priority
	
	/* IP header bitmask filter */
	
	uint32_t    value[10];                     // The value to filter against, this covers the
	                                           // whole of the IPv6 header.
	uint32_t    mask[10];                      // The mask to use for the filter, this covers the
	                                           // whole of the IPv6 header.

	/* filter flags */
	
	uint32_t    flags;                         // Possible flags for the filter, multiple can be bitmasked
	

	/* ruleset */
	
	uint16_t    ruleset;                       // The ruleset the filter belongs to


	/* filter details */

	uint16_t    snap;                          // The snap length to apply by the filter
	uint16_t    filter_id;                     // The classification of the filter (copied into the color field)
	

	uint8_t     iface;                         // The interface to filter on (this is not yet implemented)
	uint8_t     protocol;                      // The protocol type to filter on
	
	
} t_filter_msg_add_filter;



/* Possible filter flags */

#define FILTER_FLAG_ACCEPT         0x00000001    // Set if the filter is an accept filter
#define FILTER_FLAG_IPV6           0x00000002    // Set if the filter is for IPv6
#define FILTER_FLAG_STEER_TO_LINE  0x00000004    // If set accepted packets are steered back out the line, rather than the default which is to the host	
#define FILTER_FLAG_IFACE_SET      0x00000008    // Set to 1 if the interface is to be filtered on (this is not yet implemented)



typedef struct
{
	uint32_t    result;
} t_filter_msg_add_filter_cmplt;




/* Tells the xScale to remove a filter form a ruleset */
typedef struct
{
	uint16_t    ruleset;
	uint16_t    padding;
	uint32_t    unique_id;
} t_filter_msg_remove_ip_filter;

typedef struct
{
	uint32_t    result;
} t_filter_msg_remove_ip_filter_cmplt;



/* Tells the xScale to add a port range to a filter */
typedef struct
{
	uint16_t    ruleset;
	uint16_t    padding;
	
	uint32_t    unique_id;           // The unique filter ID
	uint16_t    flags;               // If bit0 set the port filters are for the source port
	
	uint16_t    range_count;         // The number of port ranges
	uint32_t    port_ranges[0];      // Variable length list of port ranges
} t_filter_msg_set_port_filter_range;

typedef struct
{
	uint32_t    result;
} t_filter_msg_set_port_filter_range_cmplt;



/* Tells the xScale to add a port bitmask to a filter */
typedef struct
{
	uint32_t    unique_id;           // The unique filter ID
	uint16_t    ruleset;
	uint16_t    flags;               // If bit0 set the port filters are for the source port

	uint16_t    value;
	uint16_t    mask;
} t_filter_msg_set_port_filter_bitmask;

typedef struct
{
    uint32_t    result;
} t_filter_msg_set_port_filter_bitmask_cmplt;



/* Tells the xScale to remove all port filter */
typedef struct
{
	uint32_t    unique_id;
	uint16_t    ruleset;
	uint16_t    source   : 1;        // If set the source port filters are cleared
	uint16_t    reserved : 15;       // Reserved set to zero
} t_filter_msg_clear_port_filters;

typedef struct
{
	uint32_t    result;
} t_filter_msg_clear_port_filters_cmplt;



/* Retreives the statistics */
typedef struct
{
	uint16_t    statistic;         // See below for possible statistics
	uint16_t    padding;
} t_filter_msg_get_statistics;

typedef struct
{
	uint32_t    result;
	
	uint32_t    stat_hi;           // The high 32-bits of the statistic
	uint32_t    stat_lo;           // The low 32-bits of the statistic
} t_filter_msg_get_statistics_cmplt;




/* Clears a particular statistic */
typedef struct
{
	uint16_t    statistic;         // See below for possible statistics
	uint16_t    padding;
} t_filter_msg_reset_statistics;

typedef struct
{
	uint32_t    result;
} t_filter_msg_reset_statistics_cmplt;



/* Removes all rulesets from the card and puts it in loopback mode */
typedef struct
{
	uint32_t	iface;            // Only the lower 6 bits are used
} t_filter_msg_remove_all_rulesets;

typedef struct
{
	uint32_t    result;
} t_filter_msg_remove_all_rulesets_cmplt;




/* Copies an existing IP filter and gives the new one a unique id */
typedef struct
{
	uint16_t    ruleset;
	uint16_t    padding;
	uint32_t    unique_id;
	uint32_t    dup_unique_id;     // The ID to give to the duplicated filter
} t_filter_msg_duplicate_ip_filter;

typedef struct
{
	uint32_t    result;
} t_filter_msg_duplicate_ip_filter_cmplt;








/**********************************************************************
 *
 * Possible statistics to read and clear
 *
 */

typedef enum {
	
	// Total number of packets received (64-bit number that rolls over to zero)
	STAT_PACKETS_RECV = 0,

	// The number of packets that have been filtered out (64-bit number that rolls over to zero)
	STAT_PACKETS_ACCEPTED,

	// The number of packets in play, this is the number of non-loopback packets that are awaiting filtering (32-bit number)
	STAT_PACKETS_IN_PLAY,


	// The number of packets dropped becase of invalid ERF type (32-bit inverted number)
	STAT_PACKETS_DROPPED_ERF_TYPE,

	// The number of packets dropped becase of invalid HDLC header (32-bit inverted number)
	STAT_PACKETS_DROPPED_HDLC,

	// The number of packets dropped becase of invalid ERF length (rlen) (32-bit inverted number)
	STAT_PACKETS_DROPPED_ERF_SIZE,


	// The number of packets dropped because not enough free memory slots (32-bit inverted number)
	STAT_PACKETS_DROPPED_NO_FREE,
	
	// The number of packets dropped because receive ring is full (32-bit inverted number)
	STAT_PACKETS_DROPPED_RX_FULL,

	// The number of filtered packets dropped because transmit ring is full (32-bit inverted number)
	STAT_PACKETS_DROPPED_TX_FULL,
	
	// The number of IPv6 packets dropped because they had an unsupported extension header (32-bit inverted number)
	STAT_PACKETS_DROPPED_IPV6_EXT_HEADER,

	// The number of packets dropped because of sequence buffer overflow (32-bit inverted number)
	STAT_PACKETS_DROPPED_SEQ_BUFFER,

	// The number of packets dropped because they were too big for the buffer, > 4096 (32-bit inverted number)
	STAT_PACKETS_DROPPED_TOO_BIG,

	// The number of packets dropped because they had too many MPLS shim headers
	STAT_PACKETS_DROPPED_MPLS_OVERFLOW,


	// The number of times a SOP mPacket was received in the middle of re-assembly (32-bit inverted number)
	STAT_MSFERR_UNEXP_SOP,
	
	// The number of times a EOP mPacket was received after the reassembly had finished or was aborted (32-bit inverted number)
	STAT_MSFERR_UNEXP_EOP,
	
	// The number of times a BOP (body of packet) mPacket was received while not performing reassembly (32-bit inverted number)
	STAT_MSFERR_UNEXP_BOP,
	
	// The number of times a NULL mPacket was received (32-bit inverted number)
	STAT_MSFERR_NULL_MPACKET,
	
	// The number of times a mPacket was received with status word errors (32-bit inverted number)
	STAT_MSFERR_STATUS_WORD,
	
	STAT_LAST

} STATISTICS;







/**********************************************************************
 *
 * Embedded Messaging API functions
 *
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	
	/* initialisation and termination */
	extern int
	d71s_filter_startup (void);

	extern int
	d71s_filter_shutdown (void);

	
	
	/* filter loading */
	extern int
	d71s_filter_download_ruleset (int dagfd, uint8_t iface, RulesetH ruleset);

	extern int
	d71s_filter_activate_ruleset (int dagfd, uint8_t iface, RulesetH ruleset);

	extern int
	d71s_filter_remove_ruleset (int dagfd, uint8_t iface, RulesetH ruleset);

	extern int
	d71s_filter_clear_iface_rulesets (int dagfd, uint8_t iface);
	
	
	
	/* dag7.1s meta data  */
	extern int
	d71s_filter_get_filter_stat (int dagfd, statistic_t stat, uint32_t *stat_low, uint32_t *stat_high);

	extern int
	d71s_filter_reset_filter_stat (int dagfd, statistic_t stat);
	
	
	

#ifdef __cplusplus
}
#endif /* __cplusplus */







#endif	// D71S_FILTER_H
