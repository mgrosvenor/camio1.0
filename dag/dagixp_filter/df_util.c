/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_util.c,v 1.8 2006/05/12 04:18:52 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         df_util.c
* DESCRIPTION:  dagixp_filter API utility functions.
*
*
* HISTORY:      15-12-05 Initial revision
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"

/* Endace headers */
#include "dagapi.h"
#include "dagixp_filter.h"
#include "dagutil.h"
#include "df_types.h"
#include "df_internal.h"



/*--------------------------------------------------------------------
 
 CONSTANTS

---------------------------------------------------------------------*/

/* IPv6 extension header IDs */
#define HOPOPT_EXT_HEADER       0
#define ROUTE_EXT_HEADER        43
#define FRAG_EXT_HEADER         44
#define AUTH_EXT_HEADER         51
#define ESP_EXT_HEADER          50
#define OPTS_EXT_HEADER         60


/* MPLS End Stack Bit */
#define MPLS_END_STACK_FLAG     0x00000100




/*--------------------------------------------------------------------
 
 CONSTANTS

---------------------------------------------------------------------*/


typedef struct mc_pos_rec_t {
	uint32_t          mc_header;
	uint32_t          hdlc;
	uint8_t           pload[1];
} mc_pos_rec_t;


typedef struct dag_rec_clr {
	uint64_t          ts;
	uint8_t           type;
	flags_t           flags;
	uint16_t          rlen;
	uint16_t          lctr;
	uint16_t          wlen;
	union {
		pos_rec_t       pos;
		mc_pos_rec_t    mc_pos;
	} rec;
	
} dag_rec_clr;




/*--------------------------------------------------------------------
 
 GLOBALS

---------------------------------------------------------------------*/

static int g_dagixp_filter_last_error = 0;



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_last_error
 
 DESCRIPTION:   Returns the last error code set, the error code
                is reset to ENONE on every call to a dagixp_filter API
                function.
 
 PARAMETERS:    none
 
 RETURNS:       the last error code set.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_last_error (void)
{
	return g_dagixp_filter_last_error;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_last_error
 
 DESCRIPTION:   Sets the error code returned by dagixp_filter_get_last_error,
                this function should only be used internally to the 
                library.
 
 PARAMETERS:    errorcode     IN      The error code to set as the last
                                      error code.
 
 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
void
dagixp_filter_set_last_error (int errorcode)
{
	g_dagixp_filter_last_error = errorcode;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      DllMain
 
 DESCRIPTION:   Usual windows DLL entry point.
 
 PARAMETERS:    hModule       IN      
                reason        IN      
                lpReserved    IN      
 
 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
#if defined(_WIN32)

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reason, 
                       LPVOID lpReserved
					 )
{


	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

#endif



/*--------------------------------------------------------------------
 
 FUNCTION:      prv_check_port_rules
                prv_check_icmp_type_rules
 
 DESCRIPTION:   Checks if the supplied packet matches the supplied filter,
                if it does 1 is returned for an accept and -1 is returned
                for a reject. If no match 0 is returned.
 
 PARAMETERS:    filter_p      IN      Pointer to the filter to test against
                packet_buf    IN      Buffer containging the IP packet.
                rlen          IN      The size of the packet in the buffer
 
 RETURNS:       1 if both a source and destination port filter match.

 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_check_port_rules (uint16_t src_port, uint16_t dst_port, ArrayListPtr port_list)
{
	uint32_t          items;
	uint32_t          i;
	int               src_port_hit;
	int               src_port_filters;
	int               dst_port_hit;
	int               dst_port_filters;
	df_port_rule_t*   port_rule_p;
	
	
	/* sanity checking */
	if ( port_list == NULL )
		return 0;
	
	
	src_port_hit     = 0;
	src_port_filters = 0;
	dst_port_hit     = 0;
	dst_port_filters = 0;

	items = adt_array_list_items (port_list);

	
	/* check the source port */
	for (i=1; i<=items; i++)
	{
		port_rule_p = (df_port_rule_t*) adt_array_list_get_indexed_item (port_list, i);
		
		if ( (port_rule_p->flags & DFFLAG_SOURCE) != DFFLAG_SOURCE )
			continue;
		src_port_filters++;
		
		if ( (port_rule_p->flags & DFFLAG_BITMASK) == DFFLAG_BITMASK )
		{
			if ( (port_rule_p->rule.bitmask.mask & src_port) == port_rule_p->rule.bitmask.value )
			{
				src_port_hit = 1;
				break;
			}
		}
		
		else
		{
			if ( (src_port >= port_rule_p->rule.range.min) &&
			     (src_port <= port_rule_p->rule.range.max) )
			{
				src_port_hit = 1;
				break;
			}
		}
	}
	
	/* if there were no filters then that is an automatic hit */
	if ( src_port_filters == 0 )
		src_port_hit = 1;
	
	
	/* check the destination port */
	for (i=1; i<=items; i++)
	{
		port_rule_p = (df_port_rule_t*) adt_array_list_get_indexed_item (port_list, i);
		
		if ( (port_rule_p->flags & DFFLAG_SOURCE) == DFFLAG_SOURCE )
			continue;
		dst_port_filters++;
		
		if ( (port_rule_p->flags & DFFLAG_BITMASK) == DFFLAG_BITMASK )
		{
			if ( (port_rule_p->rule.bitmask.mask & dst_port) == port_rule_p->rule.bitmask.value )
			{
				dst_port_hit = 1;
				break;
			}
		}
		
		else
		{
			if ( (dst_port >= port_rule_p->rule.range.min) &&
			     (dst_port <= port_rule_p->rule.range.max) )
			{
				dst_port_hit = 1;
				break;
			}
		}
	}
	
	/* if there were no filters then that is an automatic hit */
	if ( dst_port_filters == 0 )
		dst_port_hit = 1;
	
	
	/* if hits on both the source and destination ports indicate we have a filter match */
	if ( src_port_hit && dst_port_hit )
		return 1;
	else
		return 0;
}


static int
prv_check_icmp_type_rules (uint8_t icmp_type, ArrayListPtr icmp_type_list)
{
	uint32_t               items;
	uint32_t               i;
	df_icmp_type_rule_t*   icmp_rule_p;
	
	
	/* sanity checking */
	if ( icmp_type_list == NULL )
		return 0;
	
	
	
	items = adt_array_list_items (icmp_type_list);
	
	/* check the source port */
	for (i=1; i<=items; i++)
	{
		icmp_rule_p = (df_icmp_type_rule_t*) adt_array_list_get_indexed_item (icmp_type_list, i);
		
		
		if ( (icmp_rule_p->flags & DFFLAG_BITMASK) == DFFLAG_BITMASK )
		{
			if ( (icmp_rule_p->rule.bitmask.mask & icmp_type) == icmp_rule_p->rule.bitmask.value )
				return 1;
		}
		
		else
		{
			if ( (icmp_type >= icmp_rule_p->rule.range.min) &&
			     (icmp_type <= icmp_rule_p->rule.range.max) )
				return 1;
		}
	}	
	
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      prv_check_ipv4_rule
 
 DESCRIPTION:   Checks if the supplied packet matches the supplied rule,
                if it does 1 is returned for an accept and -1 is returned
                for a reject. If no match 0 is returned.
 
 PARAMETERS:    filter_p      IN      Pointer to the filter to test against
                packet_buf    IN      Buffer containging the IP packet.
                rlen          IN      The size of the packet in the buffer
 
 RETURNS:       1 for accept, -1 for reject and 0 for no match

 HISTORY:       

---------------------------------------------------------------------*/
static int 
prv_check_ipv4_rule (df_ipv4_rule_t* rule_p, uint8_t *packet_buf, uint32_t rlen)
{
	uint32_t  i;
	uint32_t  ihl;
	uint16_t  src_port;
	uint16_t  dst_port;
	uint8_t   icmp_type;
	
	
	/* Check we have at least an ip header */
	if ( rlen < 20 )
		return 0;
	
	/* Check if the packet is actual a ipv4 packet */
	ihl = packet_buf[0] & 0x0F;
	if ( (packet_buf[0] & 0xF0) != 0x40 )
		return 0;
	
	

	
	/* Check the source and destination */
	for (i=0; i<4; i++)
	{
		if ( (packet_buf[12+i] & rule_p->source.mask[i]) != (rule_p->source.addr[i] & rule_p->source.mask[i]) )
			return 0;
		
		if ( (packet_buf[16+i] & rule_p->dest.mask[i]) != (rule_p->dest.addr[i] & rule_p->dest.mask[i]) )
			return 0;
	}

	
	
	/* Check the protocol */
	if ( rule_p->protocol == kAllProtocols )
		return (rule_p->meta.action == 1) ? 1 : -1;
	
	else if ( packet_buf[9] != rule_p->protocol )
		return 0;


	/* Check we have at least enough of the packet to perform layer 5 & 6 inspection */
	if ( rlen < ((ihl * 4) + 4) )
		return 0;
	
	
	/* If the protocol is tcp, udp or sctp check the port numbers */
	if ( (rule_p->protocol == DF_PROTO_TCP) || (rule_p->protocol == DF_PROTO_UDP) || (rule_p->protocol == DF_PROTO_SCTP) )
	{
		src_port = ((uint16_t)packet_buf[(ihl * 4) + 0] << 8) | ((uint16_t)packet_buf[(ihl * 4) + 1]);
		dst_port = ((uint16_t)packet_buf[(ihl * 4) + 2] << 8) | ((uint16_t)packet_buf[(ihl * 4) + 3]);
		
		if ( !prv_check_port_rules (src_port, dst_port, rule_p->port_rules) )
			return 0;
	}
	
	/* Else if the protocol is icmp check the type numbers */
	else if ( rule_p->protocol == DF_PROTO_ICMP )
	{
		icmp_type = packet_buf[(ihl * 4)];
		
		if ( !prv_check_icmp_type_rules (icmp_type, rule_p->icmp_type_rules) )
			return 0;
	}
	
	
	/* if we have made it all the way down here then the filter is a match */
	return (rule_p->meta.action == 1) ? 1 : -1;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      prv_check_ipv6_rule
 
 DESCRIPTION:   Checks if the supplied packet matches the supplied rule,
                if it does 1 is returned for an accept and -1 is returned
                for a reject. If no match 0 is returned.
 
 PARAMETERS:    filter_p      IN      Pointer to the filter to test against
                packet_buf    IN      Buffer containging the IP packet.
                rlen          IN      The size of the packet in the buffer
 
 RETURNS:       1 for accept, -1 for reject and 0 for no match

 HISTORY:       

---------------------------------------------------------------------*/
static int 
prv_check_ipv6_rule (df_ipv6_rule_t* rule_p, uint8_t *packet_buf, uint32_t rlen)
{
	int            i;
	uint16_t       src_port;
	uint16_t       dst_port;
	uint8_t        icmp_type;
	uint8_t        next_header;
	uint32_t       hdr_offset;
	uint32_t       flow;

	
	
	
	/* Check if we have enough of the packet left for the IP header inspection */
	if ( rlen < 40 )
		return 0;
	
	
	/* Check if the packet is actual an ipv6 packet */
	if ( (packet_buf[0] & 0xF0) != 0x60 )
		return 0;
	
	

	
	/* Check the source and destination */
	for (i=0; i<16; i++)
	{
		if ( (packet_buf[8+i] & rule_p->source.mask[i]) != (rule_p->source.addr[i] & rule_p->source.mask[i]) )
			return 0;
			
		if ( (packet_buf[24+i] & rule_p->dest.mask[i]) != (rule_p->dest.addr[i] & rule_p->dest.mask[i]) )
			return 0;
	}
	
	
	/* Check the flow label */
	flow = ntohl ( ((uint32_t)(packet_buf[1] & 0x0F) << 16) | ((uint32_t)packet_buf[2] << 8) | (uint32_t)packet_buf[3] );
	if ( (rule_p->flow.mask & flow) != rule_p->flow.flow )
		return 0;
	
	
	
	/* Check the protocol */
	if ( rule_p->protocol == kAllProtocols )
		return (rule_p->meta.action == 1) ? 1 : -1;
	
	
	
	/* Loop through the possible extension headers looking for the layer 5 pdu */
	hdr_offset  = 40;
	next_header = packet_buf[6];
	while (next_header != rule_p->protocol )
	{
		switch (next_header)
		{
			case HOPOPT_EXT_HEADER:
			case ROUTE_EXT_HEADER:
			case OPTS_EXT_HEADER:
				next_header = packet_buf[hdr_offset];
				hdr_offset += (packet_buf[hdr_offset + 1] * 8) + 8;
				break;
				
			case FRAG_EXT_HEADER:
				next_header = packet_buf[hdr_offset];
				hdr_offset += 8;
				break;
				
			case AUTH_EXT_HEADER:
				next_header = packet_buf[hdr_offset];
				hdr_offset += (packet_buf[hdr_offset + 1] * 4) + 8;
				break;
			
			default:
				return 0;
		}
			
		/* again determine we have enough space to read the next header */
		if ( rlen < (hdr_offset + 8) )
			return 0;
			
	}
	
	
	
	
	/* If the protocol is tcp, udp or sctp check the port numbers */
	if ( (rule_p->protocol == DF_PROTO_TCP) || (rule_p->protocol == DF_PROTO_UDP) || (rule_p->protocol == DF_PROTO_SCTP) )
	{
		src_port = ((uint16_t)packet_buf[hdr_offset + 0] << 8) | ((uint16_t)packet_buf[hdr_offset + 1]);
		dst_port = ((uint16_t)packet_buf[hdr_offset + 2] << 8) | ((uint16_t)packet_buf[hdr_offset + 3]);
		
		if ( !prv_check_port_rules (src_port, dst_port, rule_p->port_rules) )
			return 0;
	}
	
	/* Else if the protocol is icmp check the type numbers */
	else if ( rule_p->protocol == DF_PROTO_ICMP )
	{
		icmp_type = packet_buf[hdr_offset];
		
		if ( !prv_check_icmp_type_rules (icmp_type, rule_p->icmp_type_rules) )
			return 0;
	}
	
	
	/* if we have made it all the way down here then the filter is a match */
	return (rule_p->meta.action == 1) ? 1 : -1;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_test_ip_packet
 
 DESCRIPTION:   This function is used for debugging and testing, it expects
                an IP (either version 4 or 6) packet  and then applies the filters
                in the specified ruleset. If a filter matches the filter tag
                is returned, if no match or a match on a reject packet, -1 is
                returned.
                It is the callers responsibility to provide an IP packet with
                enough bytes to filter across, the caller should also strip
                the ERF header and any other encapsulation so just an IP
                packet is supplied.
                
 PARAMETERS:    ruleset       IN      The ruleset to use to check the pacekt
                packet_buf    IN      Buffer containing the ERF record.
                rlen          IN      The size of the packet in the buffer
 
 RETURNS:       A handle to the rule that matched the packet, or NULL
                if there was no match.

 HISTORY:       

---------------------------------------------------------------------*/
RuleH
dagixp_filter_test_ip_packet (RulesetH ruleset_h, uint8_t *packet_buf, uint32_t rlen)
{
	df_ruleset_t*      df_ruleset_p;
	ListPtr            rule_list;
	IteratorPtr        iterator_p;
	df_rule_meta_t*    rule_meta_p;
	int                result;
	uint32_t           ip_version;
	uint32_t           hdlc;
	dag_rec_clr*       drec_p;
	uint8_t*           payload_p;
	uint32_t*          mpls_p;
	uint32_t           mpls_value;
	
	
	
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return NULL;
	}



	/* sanity checking */
	df_ruleset_p = (df_ruleset_t*)ruleset_h;
	if ( (df_ruleset_p == NULL) || (df_ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error(EINVAL);
		return NULL;
	}
	
	rule_list = df_ruleset_p->rule_list;
	
	
	
	
	
	
	/* Do some initial record inspection to get the location of the IP header */
	drec_p = (dag_rec_clr*) packet_buf;
	
	
	/* Make sure the record type is from the filtering */
	if ( drec_p->type == TYPE_COLOR_HDLC_POS || drec_p->type == TYPE_HDLC_POS ) 
	{
		hdlc = ntohl (drec_p->rec.pos.hdlc);
		payload_p = (uint8_t*) &(drec_p->rec.pos.pload[0]);
		rlen -= 20;
	}
	else if ( drec_p->type == TYPE_COLOR_MC_HDLC_POS || drec_p->type == TYPE_MC_HDLC )
	{
		hdlc = ntohl (drec_p->rec.mc_pos.hdlc);
		payload_p = (uint8_t*) &(drec_p->rec.mc_pos.pload[0]);
		rlen -= 24;
	}
	else
	{
		return NULL;
	}

	
	/* Check if the HDLC header indicates the packet is IPv6 and if we have to manage MPLS */
	switch (hdlc)
	{
		/* PPP / HDLC IPv4 */
		case 0xFF030021:
		case 0x0F000800:
			ip_version = 4;
			break;
		
		/* PPP / HDLC IPv6 */
		case 0xFF030057:
		case 0x0F0086DD:
			ip_version = 6;
			break;
		
		/* PPP / HDLC MPLS Unicast and Mulitcast */
		case 0xFF030281:
		case 0xFF030283:
		case 0x0F008847:
		case 0x0F008848:
			
			/* We have to jump over the MPLS headers */
			do 
			{
				mpls_p = (uint32_t*) payload_p;
				mpls_value = *mpls_p;
				mpls_value = ntohl (mpls_value);
				mpls_p++;

				payload_p += 4;
				rlen -= 4;

			} while ( ((mpls_value & MPLS_END_STACK_FLAG) == 0) && (rlen > 4) );
				
			ip_version = ((*payload_p >> 4) & 0x0F);
			
			break;
			
		/* default unknown type */
		default:
			return NULL;			
	}	
	
		
	
	/* get a iterator for the list */
	iterator_p = adt_list_get_iterator(rule_list);
	if (iterator_p == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
	
	
	
	
	/* loop through the list items (rules) */
	result = 0;
	adt_iterator_first (iterator_p);
	while (adt_iterator_is_in_list(iterator_p))
	{
		rule_meta_p = (df_rule_meta_t*) adt_iterator_retrieve(iterator_p);
		
		if ( (rule_meta_p->signature == SIG_IPV4_RULE) && (ip_version == 4) )
			result = prv_check_ipv4_rule ((df_ipv4_rule_t*)rule_meta_p, payload_p, rlen);
		
		else if ( (rule_meta_p->signature == SIG_IPV6_RULE) && (ip_version == 6) )
			result = prv_check_ipv6_rule ((df_ipv6_rule_t*)rule_meta_p, payload_p, rlen);
		
		else
			result = 0;
		
		
		/* -1 is for drop, and 1 is for accept */
		if ( result == -1 )
			break;
		else if ( result == 1 )
		{
			adt_iterator_dispose (iterator_p);
			return (RuleH)rule_meta_p;
		}
		
		adt_iterator_advance(iterator_p);
	}
	
	adt_iterator_dispose (iterator_p);
	
	
	return NULL;
}
