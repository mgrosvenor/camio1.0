/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_types.h,v 1.3 2006/03/28 03:55:07 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         df_types.h
* DESCRIPTION:  Defines the internal types used by the dagfilter API
*               library.
*
*
* HISTORY:      15-12-05 Initial revision
*
**********************************************************************/

#ifndef DF_TYPES_H
#define DF_TYPES_H


/* ADT headers */
#include "adt_list.h"
#include "adt_array_list.h"




/*--------------------------------------------------------------------
 
 PROTOCOLS
 
 These protocol types allow for extended filter functionality, for
 example if filtering on a TCP protocol filter, port filters can be
 added to the ip filter.

---------------------------------------------------------------------*/

#define DF_PROTO_ICMP          1
#define DF_PROTO_TCP           6
#define DF_PROTO_UDP           17
#define DF_PROTO_SCTP          132








/*--------------------------------------------------------------------
 
 SIGNATURES

---------------------------------------------------------------------*/

#define SIG_RULESET             0xD68FC735

#define SIG_IPV4_RULE           0x3758E847
#define SIG_IPV6_RULE           0xA79B7AEF


#define ISIPRULE_SIGNATURE(x)     (((x) == SIG_IPV4_RULE) ? 1 :    \
                                   ((x) == SIG_IPV6_RULE) ? 1 : 0)


#define ISRULE_SIGNATURE(x)       (((x) == SIG_IPV4_RULE) ? 1 :    \
                                   ((x) == SIG_IPV6_RULE) ? 1 : 0)












/*--------------------------------------------------------------------
 
 RULESET

---------------------------------------------------------------------*/

typedef struct df_ruleset_s
{
	uint32_t signature;         /* signature 0xD68FC735 */
	ListPtr  rule_list;       /* list of filters in the ruleset */
	
} df_ruleset_t;



/*--------------------------------------------------------------------
 
 FILTER RULE META DATA

 All filter data structures should have a filter_meta_t as their first
 data field, this allows the meta-data functions to cast the filter to
 a filter_meta_t type for processing.

---------------------------------------------------------------------*/

typedef struct df_rule_meta_s
{
	uint32_t            signature;               /* signature for the filter type, should be unique for each filter  */
	                                             /* see above for possible filter signatures.                        */
	
	/* ruleset handle */
	RulesetH            ruleset;                 /* handle to the parent ruleset */
	
	/* meta data */
	uint16_t            action   : 1;            /* 1 for accept, 0 for reject */
	uint16_t            steering : 1;            /* 1 for steer to line, 0 for steer to host */
	uint16_t            reserved : 14;           /* reserved for future use */
	uint16_t            snap_len;                /* the snap length of the accepted packets */
	uint16_t            rule_tag;                /* the filter tag set in ERf header, only lower 14-bits are used */
	uint16_t            priority;
	
} df_rule_meta_t;







/*--------------------------------------------------------------------
 
 STANDARD RULE TYPES

---------------------------------------------------------------------*/


typedef struct icmp_type_ruler_s
{
	uint8_t      type;
	uint8_t      mask;
} icmp_type_rule_t;


typedef struct port_rule_s
{
	uint16_t     port;
	uint16_t     mask;
} port_rule_t;


typedef struct ipv4_addr_rule_s
{
	uint8_t     addr[4];
	uint8_t     mask[4];

} ipv4_addr_rule_t;


typedef struct ipv6_addr_rule_s
{
	uint8_t      addr[16];
	uint8_t      mask[16];

} ipv6_addr_rule_t;


typedef struct ipv6_flow_rule_s
{
	uint32_t     flow;
	uint32_t     mask;

} ipv6_flow_rule_t;


typedef struct tcp_flags_rule_s
{
	uint8_t flags;
	uint8_t mask;
} tcp_flags_rule_t;







/*--------------------------------------------------------------------
 
 IPV4 RULE

---------------------------------------------------------------------*/

typedef struct df_ipv4_rule_s
{
	/* meta data */
	df_rule_meta_t      meta;  
	
	/* ip header */
	ipv4_addr_rule_t    source;                  /* source filter */
	ipv4_addr_rule_t    dest;                    /* destination filter */
	
	
	/* level 5/6 */ 
	uint8_t             protocol;                /* protocol to filter on, 0xFF for no protocol filter */
	
	tcp_flags_rule_t    tcp_flags;               /* tcp flags to filter on (only valid if protocol is 6) */
	ArrayListPtr        port_rules;              /* list of port filters (only valid if protocol is 6,17 or 132)*/
	
	ArrayListPtr        icmp_type_rules;         /* list of icmp type filters (only valid if protocol is 1) */
	
} df_ipv4_rule_t;




/*--------------------------------------------------------------------
 
 IPV6 RULE

---------------------------------------------------------------------*/

typedef struct df_ipv6_rule_s
{
	/* meta data */
	df_rule_meta_t      meta;  
	
	
	/* ip header */
	ipv6_addr_rule_t    source;                  /* source filter */
	ipv6_addr_rule_t    dest;                    /* destination filter */
	ipv6_flow_rule_t    flow;                    /* flow label filter */
	
	
	/* level 5/6 */ 
	uint8_t             protocol;                /* protocol to filter on, 0xFF for no protocol filter */
	
	tcp_flags_rule_t    tcp_flags;               /* tcp flags to filter on (only valid if protocol is 6) */
	ArrayListPtr        port_rules;              /* list of port filters (only valid if protocol is 6,17 or 132)*/
	
	ArrayListPtr        icmp_type_rules;         /* list of icmp type filters (only valid if protocol is 1) */
	
	
} df_ipv6_rule_t;





/*--------------------------------------------------------------------
 
 PORT RULE

---------------------------------------------------------------------*/

#define DFFLAG_BITMASK    0x01
#define DFFLAG_RANGE      0x02

#define DFFLAG_SOURCE     0x80
#define DFFLAG_DEST       0x00

typedef struct df_port_rule_s
{
	uint8_t flags;
	
	union {
		struct {
			uint16_t value;
			uint16_t mask;
		} bitmask;
		
		struct {
			uint16_t min;
			uint16_t max;
		} range;
	} rule;
	
} df_port_rule_t;




/*--------------------------------------------------------------------
 
 ICMP TYPE RULE

---------------------------------------------------------------------*/

typedef struct df_icmp_type_rule_s
{
	uint8_t flags;

	union {
		struct {
			uint8_t value;
			uint8_t mask;
		} bitmask;
		
		struct {
			uint8_t min;
			uint8_t max;
		} range;
	} rule;
	
} df_icmp_type_rule_t;




#endif    // DF_TYPES_H
