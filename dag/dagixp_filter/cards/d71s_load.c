/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: d71s_load.c,v 1.7 2006/04/18 07:21:51 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         d71s_load.c
* DESCRIPTION:  
*
*
* HISTORY:      15-12-05 Initial revision
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"

/* ADT headers */
#include "adt_list.h"
#include "adt_array.h"
#include "adt_array_list.h"

/* Endace headers */
#include "dagapi.h"
#include "dagema.h"
#include "dagutil.h"
#include "dagixp_filter.h"
#include "df_internal.h"
#include "df_types.h"

#include "d71s_filter.h"



/*--------------------------------------------------------------------
  
  Types
  
---------------------------------------------------------------------*/

typedef struct d71s_ruleset_s
{
	RulesetH    ruleset_h;
	uint16_t    ruleset_id;
	
} d71s_ruleset_t;


typedef struct d71s_port_rule_set_s
{
	uint8_t    count;
	
	union {
		struct {
			uint16_t value;
			uint16_t mask;
		} bitmask;
		
		uint32_t     ranges[254];
	} rule;
	
} d71s_port_rule_set_t;



/*--------------------------------------------------------------------
  
  GLOBALS
  
---------------------------------------------------------------------*/


/* This is just an incriment counter that assigns unique IDs to downloaded
 * IP filters. The unique ID functionality is not implemented at the
 * moment. */
static uint32_t g_unique_id_count = 1;



/*--------------------------------------------------------------------
  
  Local mapping array used to convert the ruleset and filter handles 
  to unique numbers for the 7.1s SCale code.
  
---------------------------------------------------------------------*/

static ArrayListPtr g_downloaded_rulesets = NULL;


  
  


/*--------------------------------------------------------------------
 
 FUNCTION:      prv_dispose_d71s_port_rule_set
 
 DESCRIPTION:   Called by the ADT library when the port rule set is
                removed.
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
prv_dispose_d71s_port_rule_set (AdtPtr data)
{
	free(data);	
}



/*--------------------------------------------------------------------
 
 FUNCTION:      prv_dispose_d71s_ruleset
 
 DESCRIPTION:   Called by the ADT library when the ruleset is removed
                from the array (in response to a request for it to be
                removed from the card).
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
prv_dispose_d71s_ruleset (AdtPtr data)
{
	free(data);	
}



/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_startup
 
 DESCRIPTION:   
 
 PARAMETERS:    
 
 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_startup (void)
{
	/* initailise the ruleset array */
	g_downloaded_rulesets = adt_array_list_init(prv_dispose_d71s_ruleset);
	if (g_downloaded_rulesets == NULL)
	{
		dagixp_filter_set_last_error(ENOMEM);
		return -1;
	}
	
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_shutdown
 
 DESCRIPTION:   
 
 PARAMETERS:    
 
 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_shutdown (void)
{
	if (g_downloaded_rulesets)
		adt_array_list_dispose (g_downloaded_rulesets);
		
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      prv_duplicate_ip_rule
 
 DESCRIPTION:   Sends the actual messages to the card, monitors the response
                and returns an error code based on the response (or lack of one).
 
 PARAMETERS:	dagfd          IN      The file descriptor for the card
                ruleset        IN      The ruleset containing the ip
                                       filter.
                orig_id        IN      The unique Id of the filter that
                                       will be duplicated.
                new_id         IN      The unique Id to assign to the new
                                       duplicated IP filter.
                                       

 RETURNS:       -1 if there was an error, otherwise 0. This function
                updates the last_error library variable.
                
 NOTES:         This function sends the message to the card to perform
                the duplication, the card doesn't duplicate the port
                filters assoiated with the IP filter.

 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_duplicate_ip_rule (int dagfd, uint16_t ruleset, uint32_t orig_id, uint32_t new_id)
{
	t_filter_msg_duplicate_ip_filter        request_msg;
	t_filter_msg_duplicate_ip_filter_cmplt  response_msg;
	uint32_t  msg_id;
	uint32_t  msg_len;
	
		
	/* populate the message structure */
	memset (&request_msg, 0, sizeof(t_filter_msg_duplicate_ip_filter));
	
	request_msg.ruleset       = dagema_htole16(ruleset);
	request_msg.unique_id     = dagema_htole32(orig_id);
	request_msg.dup_unique_id = dagema_htole32(new_id);
	

	
	
	/* send a request to set the port filters */
	if ( dagema_send_msg(dagfd, EMA_MSG_DUPLICATE_IP_FILTER, EMA_MSG_DUPLICATE_IP_FILTER_SIZE, (uint8_t*)(&request_msg), NULL) != 0 )
	{
		dagutil_warning ("failed to send request for duplicating an IP filter (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
			
			
	/* read the response */
	msg_len = EMA_MSG_DUPLICATE_IP_FILTER_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, DEFAULT_EMA_TIMEOUT) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}		
			
		
	/* check the response */
	if ( (msg_id != EMA_MSG_DUPLICATE_IP_FILTER_CMPLT) || (msg_len != EMA_MSG_DUPLICATE_IP_FILTER_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for duplicating an ip filter was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
			
			
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}			
	

	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      prv_perform_port_rule_bitmask_download
 
 DESCRIPTION:   Sends the actual messages to the card, monitors the response
                and returns an error code based on the response (or lack of one).
 
 PARAMETERS:    dagfd          IN      The file descriptor for the card
                ruleset        IN      The ruleset containing the ip
                                       filter.
                unique_id      IN      The unique Id of the filter that
                                       will contain the port filters.
                source         IN      Non zero value for source filter
                                       otherwise destination filter.
                mask           IN      The mask value of the filter.
                value          IN      The value of the filter.
                                       

 RETURNS:       -1 if there was an error, otherwise 0. This function
                updates the last_error library variable.

 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_perform_port_rule_bitmask_download (int dagfd, uint16_t ruleset, uint32_t unique_id, uint32_t source, uint16_t value, uint16_t mask)
{
	t_filter_msg_set_port_filter_bitmask        request_msg;
	t_filter_msg_set_port_filter_bitmask_cmplt  response_msg;
	uint32_t  msg_id;
	uint32_t  msg_len;
	

	
	
	/* populate the message structure */
	memset (&request_msg, 0, sizeof(t_filter_msg_set_port_filter_bitmask));
	
	request_msg.ruleset   = dagema_htole16(ruleset);
	request_msg.unique_id = dagema_htole32(unique_id);
		 
	request_msg.flags     = dagema_htole16(source ? 1 : 0);
	
	request_msg.value     = dagema_htole16(value);
	request_msg.mask      = dagema_htole16(mask);
	
	
	
	
	/* send a request to set the port filters */
	if ( dagema_send_msg(dagfd, EMA_MSG_SET_PORT_FILTER_BITMASK, EMA_MSG_SET_PORT_FILTER_BITMASK_SIZE, (uint8_t*)(&request_msg), NULL) != 0 )
	{
		dagutil_warning ("failed to send request for setting the port filter bitmask (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
			
			
	/* read the response */
	msg_len = EMA_MSG_SET_PORT_FILTER_BITMASK_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, DEFAULT_EMA_TIMEOUT) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}		
			
		
	/* check the response */
	if ( (msg_id != EMA_MSG_SET_PORT_FILTER_BITMASK_CMPLT) || (msg_len != EMA_MSG_SET_PORT_FILTER_BITMASK_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for adding a port bitmask filter was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
			
			
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}			
	
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      prv_perform_port_rule_range_download
 
 DESCRIPTION:   Sends the actual messages to the card, monitors the response
                and returns an error code based on the response (or lack of one).
 
 PARAMETERS:	dagfd          IN      The file descriptor for the card
                ruleset        IN      The ruleset containing the ip
                                       filter.
                unique_id      IN      The unique Id of the filter that
                                       will contain the port filters.
                source         IN      Non zero value for source filter
                                       otherwise destination filter.
                ranges         IN      The number of rnage filters.
                rangeP         IN      Pointer to a buffer containing the
                                       range filters, the upper 16 bits
                                       of each entry contain the min port
                                       value, the lower 16 bits contain
                                       the max port value.
                                       

 RETURNS:       -1 if there was an error, otherwise 0. This function
                updates the last_error library variable.

 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_perform_port_rule_range_download (int dagfd, uint16_t ruleset, uint32_t unique_id, uint32_t source, uint16_t ranges, const uint32_t *rangeP)
{
	t_filter_msg_set_port_filter_range*       request_msg_p;
	t_filter_msg_set_port_filter_range_cmplt  response_msg;
	uint8_t   request_buf[2048];
	uint32_t  i;
	uint32_t  msg_id;
	uint32_t  msg_len;
	uint32_t  timeout;
	
	

	
	/* sanity check the number of filters */
	if ( ranges > 254 )
	{
		assert(0);
		return -1;		
	}
	
	if ( ranges <= 0 )
		return 0;
	
	
	
	/* populate the message structure */
	memset (request_buf, 0, 2048);
	request_msg_p = (t_filter_msg_set_port_filter_range*) &(request_buf[0]);
	
	request_msg_p->ruleset     = dagema_htole16(ruleset);
	request_msg_p->unique_id   = dagema_htole32(unique_id);
	request_msg_p->flags       = dagema_htole16(source ? 1 : 0);
	request_msg_p->range_count = dagema_htole16(ranges);
	
	for (i=0; i<ranges; i++)
		request_msg_p->port_ranges[i] = dagema_htole32(rangeP[i]);
	
	msg_len = EMA_MSG_SET_PORT_FILTER_RANGE_SIZE + (ranges * 4);
	
	
	
	/* send a request to set the port filters */
	if ( dagema_send_msg(dagfd, EMA_MSG_SET_PORT_FILTER_RANGE, msg_len, request_buf, NULL) != 0 )
	{
		dagutil_warning ("failed to send request for setting the port filter ranges (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
			
			
	/* read the response */
	msg_len = EMA_MSG_SET_PORT_FILTER_RANGE_CMPLT_SIZE;
	timeout = ((ranges * 4) * 100) + DEFAULT_EMA_TIMEOUT;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, timeout) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}		
			
		
	/* check the response */
	if ( (msg_id != EMA_MSG_SET_PORT_FILTER_RANGE_CMPLT) || (msg_len != EMA_MSG_SET_PORT_FILTER_RANGE_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for adding a port range filter was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
			
			
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}			
	
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      prv_download_port_rules
 
 DESCRIPTION:   Downloads one or more port filters to the card, this function
                doesn't actual do the download (that is done by the 
                prv_perform_port_filter_???_download() function above), it just
                sorts out what port filters need to be downloaded, then
                passes on the parameters to prv_perform_port_filter_???_download().
 
 PARAMETERS:    dagfd          IN      The file descriptor for the card
                ruleset        IN      The ruleset containing the ip
                                       filter.
                port_filters   IN      Array of the port filters to download.
                unique_id      IN      The unique Id of the filter that
                                       will contain the port filters.

 RETURNS:       -1 if there was an error, otherwise 0. This function
                updates the last_error library variable.

 NOTES:         Each IP filter on the 7.1s cards can have upto 254 port
                range filters or a single bitmask filter. However this
                API allows for an arbitary number of port filters (both
                bitmask and range) per IP filter. Therefore if a combination
                of port filters are supplied that cannot fit within a
                single ip filter, the filter is duplicated and the port
                filters are spread across multiple duplicate ip filters.
                To allow for this, the function creates 'sets' of port
                filters that contain either a single bitmask filter or upto
                254 range filters, these 'sets' are then downloaded to the
                card.
                
                To complicate things, source and destination port filters
                need to be checked at the same time, therefore if port
                filters are spread across multiple ip filters, every
                permutation of source and destination filters must be
                installed.
                
                To make efficent ruleset, users should supply only one
                bitmask filter and less than 255 port range filters per
                source/destination. This is only true for the 7.1s, for
                copro filtering, bitmask filters should only be supplied
                rather then range filters.

 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_download_port_rules (int dagfd, uint16_t ruleset, ArrayListPtr port_rules, uint32_t unique_id)
{
	ArrayListPtr      src_set = NULL;
	ArrayListPtr      dst_set = NULL;
	df_port_rule_t*   rule_p;
	uint32_t          count;
	uint32_t          index;
	uint32_t          i, j;
	int               rc = 0;
	uint32_t          src_set_count;
	uint32_t          dst_set_count;
	
	const uint32_t    max_range_filters = 250;
	
	d71s_port_rule_set_t* bitmask_p;
	d71s_port_rule_set_t* range_p;

	d71s_port_rule_set_t* source_p;
	d71s_port_rule_set_t* dest_p;

	
	
	
	/* sanity checking */
	if ( port_rules == NULL )
	{
		dagixp_filter_set_last_error(EINVAL);
		return -1;
	}
	
	
	/* initialise the sets that will contain the filters */
	src_set = adt_array_list_init(prv_dispose_d71s_port_rule_set);
	dst_set = adt_array_list_init(prv_dispose_d71s_port_rule_set);
	
	
	/* get the number of port filters */
	count = adt_array_list_items(port_rules);
	
	
	/* first we manually create mini port filter sets, a set can contain a
	 * single bitmask filter or upto 254 range filters. */
	
	
	
	/*
	 * Source Port Filters 
	 */
	range_p = NULL;
	for (index=1; index<=count; index++)
	{
		rule_p = (df_port_rule_t*) adt_array_list_get_indexed_item(port_rules, index);
		if ( rule_p == NULL )
			continue;

		
		/* get either the source or destination specific sets */
		if ( (rule_p->flags & DFFLAG_SOURCE) == 0 )
			continue;
		
		
		/* bitmask filter so add a new entry to the set */
		if ( rule_p->flags & DFFLAG_BITMASK )
		{
			bitmask_p = (d71s_port_rule_set_t*) malloc (sizeof(d71s_port_rule_set_t));
			bitmask_p->count = 0xFF;
			bitmask_p->rule.bitmask.mask  = rule_p->rule.bitmask.mask;
			bitmask_p->rule.bitmask.value = rule_p->rule.bitmask.value;
			
			adt_array_list_add_last (src_set, bitmask_p);
		}
		
		/* range filter so add to the current active collection, if the collection
		 * is full, add to the set.
		 */
		else if ( rule_p->flags & DFFLAG_RANGE )
		{
			if ( range_p == NULL )
			{
				range_p = (d71s_port_rule_set_t*) malloc (sizeof(d71s_port_rule_set_t));
				memset (range_p, 0, sizeof(d71s_port_rule_set_t));
			}
			
			range_p->rule.ranges[range_p->count] = ((uint32_t)rule_p->rule.range.min << 16) | (uint32_t)rule_p->rule.range.max;
			range_p->count++;
			
			if ( range_p->count == max_range_filters )
			{
				adt_array_list_add_last (src_set, range_p);
				range_p = NULL;
			}
		}
	}
	

	if ( range_p != NULL )
	{
		assert(range_p->count > 0);
		adt_array_list_add_last (src_set, range_p);
	}
	
	
	
	
	

	/*
	 * Destination Port Filters 
	 */
	range_p = NULL;
	for (index=1; index<=count; index++)
	{		
		rule_p = (df_port_rule_t*) adt_array_list_get_indexed_item(port_rules, index);
		if ( rule_p == NULL )
			continue;
		
		/* get either the source or destination specific sets */
		if ( (rule_p->flags & DFFLAG_SOURCE) != 0 )
			continue;
		
		
		/* bitmask filter so add a new entry to the set */
		if ( rule_p->flags & DFFLAG_BITMASK )
		{
			bitmask_p = (d71s_port_rule_set_t*) malloc (sizeof(d71s_port_rule_set_t));
			bitmask_p->count = 0xFF;
			bitmask_p->rule.bitmask.mask  = rule_p->rule.bitmask.mask;
			bitmask_p->rule.bitmask.value = rule_p->rule.bitmask.value;

			adt_array_list_add_last (dst_set, bitmask_p);
		}
		
		/* range filter so add to the current active collection, if the collection
		 * is full, add to the set.
		 */
		else if ( rule_p->flags & DFFLAG_RANGE )
		{
			if ( range_p == NULL )
			{
				range_p = (d71s_port_rule_set_t*) malloc (sizeof(d71s_port_rule_set_t));
				memset (range_p, 0, sizeof(d71s_port_rule_set_t));
			}
			
			range_p->rule.ranges[range_p->count] = ((uint32_t)rule_p->rule.range.min << 16) | (uint32_t)rule_p->rule.range.max;
			range_p->count++;
			
			if ( range_p->count == max_range_filters )
			{
				adt_array_list_add_last (dst_set, range_p);
				range_p = NULL;
			}
		}
	}	

	

	if ( range_p != NULL )
	{
		assert(range_p->count > 0);
		adt_array_list_add_last (dst_set, range_p);
	}


	

	/*
	 * At this point we have two filter sets, one for destination and one
	 * for source.
	 */
	src_set_count = adt_array_list_items(src_set);
	dst_set_count = adt_array_list_items(dst_set);
	
	
	/* if no source filter just add the destination filter(s) */
	if ( src_set_count == 0 )
	{
		for (j=1; j<=dst_set_count; j++)
		{
	
			/* set the destination port filter */
			dest_p = adt_array_list_get_indexed_item (dst_set, j);
			if ( dest_p->count == 255 )
				rc = prv_perform_port_rule_bitmask_download (dagfd, ruleset, unique_id, 0, dest_p->rule.bitmask.value, dest_p->rule.bitmask.mask);
			else
				rc = prv_perform_port_rule_range_download (dagfd, ruleset, unique_id, 0, dest_p->count, dest_p->rule.ranges);
			if ( rc != 0 )   goto Exit;


			/* if there is another destination filter set we should add another ip filter */
			if ( j < dst_set_count )
			{
				rc = prv_duplicate_ip_rule (dagfd, ruleset, unique_id, ++g_unique_id_count);
				if ( rc != 0 )   goto Exit;

				unique_id = g_unique_id_count;
			}
		}
	}
	
	/* if no destination filter just add the source filter(s) */
	else if ( dst_set_count == 0 )
	{
		for (i=1; i<=src_set_count; i++)
		{
	
			/* set the destination port filter */
			source_p = adt_array_list_get_indexed_item (src_set, i);
			if ( source_p->count == 255 )
				rc = prv_perform_port_rule_bitmask_download (dagfd, ruleset, unique_id, 1, source_p->rule.bitmask.value, source_p->rule.bitmask.mask);
			else
				rc = prv_perform_port_rule_range_download (dagfd, ruleset, unique_id, 1, source_p->count, source_p->rule.ranges);
			if ( rc != 0 )   goto Exit;


			/* if there is another source filter set we should add another ip filter */
			if ( i < src_set_count )
			{
				rc = prv_duplicate_ip_rule (dagfd, ruleset, unique_id, ++g_unique_id_count);
				if ( rc != 0 )   goto Exit;

				unique_id = g_unique_id_count;
			}
		}
	}		

	/* if a combination of source and destination filters, we have to cover all permutations */	
	else
	{
		for (i=1; i<=src_set_count; i++)
			for (j=1; j<=dst_set_count; j++)
			{
	
				/* set the source port filter */
				source_p = adt_array_list_get_indexed_item (src_set, i);
				if ( source_p->count == 255 )
					rc = prv_perform_port_rule_bitmask_download (dagfd, ruleset, unique_id, 1, source_p->rule.bitmask.value, source_p->rule.bitmask.mask);
				else
					rc = prv_perform_port_rule_range_download (dagfd, ruleset, unique_id, 1, source_p->count, source_p->rule.ranges);
				if ( rc != 0 )   goto Exit;
				
				/* set the destination port filter */
				dest_p = adt_array_list_get_indexed_item (dst_set, j);
				if ( dest_p->count == 255 )
					rc = prv_perform_port_rule_bitmask_download (dagfd, ruleset, unique_id, 0, dest_p->rule.bitmask.value, dest_p->rule.bitmask.mask);
				else
					rc = prv_perform_port_rule_range_download (dagfd, ruleset, unique_id, 0, dest_p->count, dest_p->rule.ranges);
				if ( rc != 0 )   goto Exit;
	
	
				/* if there is another destination filter set we should add another ip filter */
				if ( (i < src_set_count) || (j < dst_set_count) )
				{
					rc = prv_duplicate_ip_rule (dagfd, ruleset, unique_id, ++g_unique_id_count);
					if ( rc != 0 )   goto Exit;
	
					unique_id = g_unique_id_count;
				}
			}
	}
		
	
		
Exit:
	
	/* free the sets */
	adt_array_list_dispose(src_set);
	adt_array_list_dispose(dst_set);

	
	return rc;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      prv_download_icmp_type_rules
 
 DESCRIPTION:   Downloads one or more port filters to the card, this function
                doesn't actual do the download (that is done by the 
                prv_perform_port_filter_???_download() function above), it just
                sorts out what port filters need to be downloaded, then
                passes on the parameters to prv_perform_port_filter_???_download().
 
 PARAMETERS:    dagfd          IN      The file descriptor for the card
                ruleset        IN      The ruleset containing the ip
                                       filter.
                filter_p       IN      Pointer to the filter to download
                unique_id      IN      The unique Id of the filter that
                                       will contain the port filters.

 RETURNS:       -1 if there was an error, otherwise 0. This function
                updates the last_error library variable.

 NOTES:         
 
 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_download_icmp_type_rules (int dagfd, uint16_t ruleset, ArrayListPtr icmp_type_rules, uint32_t unique_id)
{
	ArrayListPtr            icmp_type_set = NULL;
	df_icmp_type_rule_t*    rule_p;
	uint32_t                count;
	uint32_t                index;
	uint32_t                i;
	int                     rc = 0;
	uint32_t                icmp_type_set_count;
	d71s_port_rule_set_t*   set_p;
	d71s_port_rule_set_t*   bitmask_p;
	d71s_port_rule_set_t*   range_p;

	
	
	
	/* sanity checking */
	if ( icmp_type_rules == NULL )
	{
		dagixp_filter_set_last_error(EINVAL);
		return -1;
	}
	
	
	/* initialise the sets that will contain the filters */
	icmp_type_set = adt_array_list_init(prv_dispose_d71s_port_rule_set);
	
	
	/* get the number of port filters */
	count = adt_array_list_items(icmp_type_rules);
	
	
	/* first we manually create mini icmp type filter sets, a set can contain a
	 * single bitmask filter or upto 254 range filters. */
	
	range_p = NULL;
	for (index=1; index<=count; index++)
	{
		rule_p = (df_icmp_type_rule_t*) adt_array_list_get_indexed_item(icmp_type_rules, index);
		
		
		/* bitmask filter so add a new entry to the set */
		if ( rule_p->flags & DFFLAG_BITMASK )
		{
			bitmask_p = (d71s_port_rule_set_t*) malloc (sizeof(d71s_port_rule_set_t));
			bitmask_p->count = 0xFF;
			bitmask_p->rule.bitmask.mask  = rule_p->rule.bitmask.mask;
			bitmask_p->rule.bitmask.value = rule_p->rule.bitmask.value;

			adt_array_list_add_last (icmp_type_set, bitmask_p);
		}
		
		/* range filter so add to the current active collection, if the collection
		 * is full, add to the set.
		 */
		else if ( rule_p->flags & DFFLAG_RANGE )
		{
			if ( range_p == NULL )
			{
				range_p = (d71s_port_rule_set_t*) malloc (sizeof(d71s_port_rule_set_t));
				memset (range_p, 0, sizeof(d71s_port_rule_set_t));
			}
			
			range_p->rule.ranges[range_p->count] = ((uint32_t)rule_p->rule.range.min << 16) | (uint32_t)rule_p->rule.range.max;
			range_p->count++;
			
			if ( range_p->count == 254 )
			{
				adt_array_list_add_last (icmp_type_set, range_p);
				range_p = NULL;
			}
		}
	}	

	if ( range_p != NULL )
	{
		assert(range_p->count > 0);
		adt_array_list_add_last (icmp_type_set, range_p);
	}



	/*
	 * At this point we have two filter sets, one for destination and one
	 * for source.
	 */
	icmp_type_set_count = adt_array_list_items(icmp_type_set);
		
	for (i=1; i<=icmp_type_set_count; i++)
	{
		/* set the icmp type filter */
		set_p = adt_array_list_get_indexed_item (icmp_type_set, i);
		if ( set_p->count == 255 )
			rc = prv_perform_port_rule_bitmask_download (dagfd, ruleset, unique_id, 1, set_p->rule.bitmask.value, set_p->rule.bitmask.mask);
		else
			rc = prv_perform_port_rule_range_download (dagfd, ruleset, unique_id, 1, set_p->count, set_p->rule.ranges);
		if ( rc != 0 )   goto Exit;


		/* if there is another destination filter set we should add another ip filter */
		if ( i < (icmp_type_set_count-1) )
		{
			rc = prv_duplicate_ip_rule (dagfd, ruleset, unique_id, ++g_unique_id_count);
			if ( rc != 0 )   goto Exit;

			unique_id = g_unique_id_count;
		}
	}

		
		
Exit:
	
	/* free the sets */
	adt_array_list_dispose(icmp_type_set);

	
	return rc;	
}




/*--------------------------------------------------------------------
 
 FUNCTION:      prv_download_ipv4_rule
 
 DESCRIPTION:   Downloads an IPv4 rule to the card
 
 PARAMETERS:    ruleset        IN      The ruleset to download the IP
                                       rule for.
                filter_p       IN      Pointer to the rule to download

 RETURNS:       -1 if there was an error, otherwise 0. This function
                updates the last_error library variable.

 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_download_ipv4_rule (int dagfd, uint16_t ruleset, df_ipv4_rule_t *rule_p)
{
	t_filter_msg_add_filter        request_msg;
	t_filter_msg_add_filter_cmplt  response_msg;
	uint32_t                       timeout;
	uint32_t                       msg_id;
	uint32_t                       msg_len;
	
	
	/* sanity checking */
	if ( rule_p == NULL || rule_p->meta.signature != SIG_IPV4_RULE )
	{
		dagixp_filter_set_last_error(EINVAL);
		return -1;
	}


	
	/* populate the message structure */
	memset (&request_msg, 0, sizeof(t_filter_msg_add_filter));
	
	request_msg.ruleset   = dagema_htole16(ruleset);
	request_msg.unique_id = dagema_htole32(g_unique_id_count);
	request_msg.priority  = dagema_htole32(rule_p->meta.priority);
	
	request_msg.snap      = dagema_htole16(rule_p->meta.snap_len);
	request_msg.filter_id = dagema_htole16(rule_p->meta.rule_tag);

	request_msg.flags    |= (rule_p->meta.action == 1)   ? FILTER_FLAG_ACCEPT : 0;
	request_msg.flags    |= (rule_p->meta.steering == 1) ? FILTER_FLAG_STEER_TO_LINE : 0;
	request_msg.flags     = dagema_htole32(request_msg.flags);
	

	request_msg.iface = 0;                                           /* interface filtering no yet supported on the 7.1s */


	request_msg.value[0]  = dagema_htole32(0x40000000);              /* IP version */
	request_msg.mask[0]   = dagema_htole32(0xF0000000);

	                                                                 /* source address */
	request_msg.value[3]  = dagema_htole32(((uint32_t)rule_p->source.addr[0] << 24) |
	                                       ((uint32_t)rule_p->source.addr[1] << 16) |
	                                       ((uint32_t)rule_p->source.addr[2] <<  8) |
	                                       ((uint32_t)rule_p->source.addr[3]      ));
	request_msg.mask[3]   = dagema_htole32(((uint32_t)rule_p->source.mask[0] << 24) |
	                                       ((uint32_t)rule_p->source.mask[1] << 16) |
	                                       ((uint32_t)rule_p->source.mask[2] <<  8) |
	                                       ((uint32_t)rule_p->source.mask[3]      ));

	                                                                 /* destination address */
	request_msg.value[4]  = dagema_htole32(((uint32_t)rule_p->dest.addr[0] << 24) |
	                                       ((uint32_t)rule_p->dest.addr[1] << 16) |
	                                       ((uint32_t)rule_p->dest.addr[2] <<  8) |
	                                       ((uint32_t)rule_p->dest.addr[3]      ));
	request_msg.mask[4]   = dagema_htole32(((uint32_t)rule_p->dest.mask[0] << 24) |
	                                       ((uint32_t)rule_p->dest.mask[1] << 16) |
	                                       ((uint32_t)rule_p->dest.mask[2] <<  8) |
	                                       ((uint32_t)rule_p->dest.mask[3]      ));
	
	
	request_msg.protocol  = rule_p->protocol;
	if ( rule_p->protocol != kAllProtocols )                       /* protocol */
	{
		request_msg.value[2] = dagema_htole32((uint32_t)rule_p->protocol << 16);  
		request_msg.mask[2]  = dagema_htole32(0x00FF0000);
	}
	
	
	
	
	
	/* send a request to add an IP filter */
	if ( dagema_send_msg(dagfd, EMA_MSG_ADD_IP_FILTER, EMA_MSG_ADD_IP_FILTER_SIZE, (uint8_t*)&request_msg, NULL) != 0 )
	{
		dagutil_warning ("failed to send request for adding an IP filter (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* read the response */
	msg_len = EMA_MSG_ADD_IP_FILTER_CMPLT_SIZE;
	timeout = (EMA_MSG_ADD_IP_FILTER_SIZE * 10) + DEFAULT_EMA_TIMEOUT;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, timeout) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* check the response */
	if ( (msg_id != EMA_MSG_ADD_IP_FILTER_CMPLT) || (msg_len != EMA_MSG_ADD_IP_FILTER_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for adding an IP filter was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
	
	
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}
	
	
	
	
	/* now we should add any port filters that are attached to this filter */
	if ( ((rule_p->protocol == DF_PROTO_TCP) || (rule_p->protocol == DF_PROTO_UDP) || (rule_p->protocol == DF_PROTO_SCTP))
	   && (adt_array_list_items(rule_p->port_rules) > 0) )
	{
		prv_download_port_rules (dagfd, ruleset, rule_p->port_rules, g_unique_id_count);
	}
	
	else if ( (rule_p->protocol == DF_PROTO_ICMP) && (adt_array_list_items(rule_p->icmp_type_rules) > 0) )
	{
		prv_download_icmp_type_rules (dagfd, ruleset, rule_p->icmp_type_rules, g_unique_id_count);
	}
	

	/* update the unique id count */
	g_unique_id_count++;


	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      prv_download_ipv6_rule
 
 DESCRIPTION:   Downloads an IPv6 rule to the card.
 
 PARAMETERS:    ruleset        IN      The ruleset to download the IP
                                       filter for.
                filter_p       IN      Pointer to the filter to download

 RETURNS:       -1 if there was an error, otherwise 0. This function
                updates the last_error library variable.

 HISTORY:       

---------------------------------------------------------------------*/
static int
prv_download_ipv6_rule (int dagfd, uint16_t ruleset, df_ipv6_rule_t *rule_p)
{
	t_filter_msg_add_filter        request_msg;
	t_filter_msg_add_filter_cmplt  response_msg;
	uint32_t                       msg_id;
	uint32_t                       msg_len;
	uint32_t                       i, j;
	uint32_t                       timeout;
	

	/* sanity checking */
	if ( rule_p == NULL || rule_p->meta.signature != SIG_IPV6_RULE )
	{
		dagixp_filter_set_last_error(EINVAL);
		return -1;
	}
	
	
	/* populate the message structure */
	memset (&request_msg, 0, sizeof(t_filter_msg_add_filter));
	
	request_msg.ruleset   = dagema_htole16(ruleset);
	request_msg.unique_id = dagema_htole32(g_unique_id_count);
	request_msg.priority  = dagema_htole32(rule_p->meta.priority);

	request_msg.snap      = dagema_htole16(rule_p->meta.snap_len);
	request_msg.filter_id = dagema_htole16(rule_p->meta.rule_tag);
	
	request_msg.flags    |= FILTER_FLAG_IPV6;
	request_msg.flags    |= (rule_p->meta.action == 1)   ? FILTER_FLAG_ACCEPT : 0;
	request_msg.flags    |= (rule_p->meta.steering == 1) ? FILTER_FLAG_STEER_TO_LINE : 0;
	request_msg.flags     = dagema_htole32(request_msg.flags);

	request_msg.iface     = 0;                           /* interface filtering no yet supported on the 7.1s */

	request_msg.protocol  = rule_p->protocol;            /* protocol */

	request_msg.value[0]  = dagema_htole32(0x60000000);  /* IP version */
	request_msg.mask[0]   = dagema_htole32(0xF0000000);

	                                                     /* flow label */
	request_msg.value[0] |= dagema_htole32(0x000FFFFF & rule_p->flow.flow);
	request_msg.mask[0]  |= dagema_htole32(0x000FFFFF & rule_p->flow.mask);

	for (i=0, j=2; i<16; i+=4, j++)                      /* source address */
	{
		request_msg.value[j]  = dagema_htole32(((uint32_t)rule_p->source.addr[i+0] << 24) |
		                                       ((uint32_t)rule_p->source.addr[i+1] << 16) |
		                                       ((uint32_t)rule_p->source.addr[i+2] <<  8) |
		                                       ((uint32_t)rule_p->source.addr[i+3]      ));
		request_msg.mask[j]   = dagema_htole32(((uint32_t)rule_p->source.mask[i+0] << 24) |
		                                       ((uint32_t)rule_p->source.mask[i+1] << 16) |
		                                       ((uint32_t)rule_p->source.mask[i+2] <<  8) |
		                                       ((uint32_t)rule_p->source.mask[i+3]      ));
	}
	
	for (i=0, j=6; i<16; i+=4, j++)                      /* destination address */
	{
		request_msg.value[j]  = dagema_htole32(((uint32_t)rule_p->dest.addr[i+0] << 24) |
		                                       ((uint32_t)rule_p->dest.addr[i+1] << 16) |
		                                       ((uint32_t)rule_p->dest.addr[i+2] <<  8) |
		                                       ((uint32_t)rule_p->dest.addr[i+3]      ));
		request_msg.mask[j]   = dagema_htole32(((uint32_t)rule_p->dest.mask[i+0] << 24) |
		                                       ((uint32_t)rule_p->dest.mask[i+1] << 16) |
		                                       ((uint32_t)rule_p->dest.mask[i+2] <<  8) |
		                                       ((uint32_t)rule_p->dest.mask[i+3]      ));
	}
	


	
	
	
	/* send a request to add an IP filter */
	if ( dagema_send_msg(dagfd, EMA_MSG_ADD_IP_FILTER, EMA_MSG_ADD_IP_FILTER_SIZE, (uint8_t*)&request_msg, NULL) != 0 )
	{
		dagutil_warning ("failed to send request for adding an IP filter (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* read the response */
	msg_len = EMA_MSG_ADD_IP_FILTER_CMPLT_SIZE;
	timeout = (EMA_MSG_ADD_IP_FILTER_SIZE * 10) + DEFAULT_EMA_TIMEOUT;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, timeout) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* check the response */
	if ( (msg_id != EMA_MSG_ADD_IP_FILTER_CMPLT) || (msg_len != EMA_MSG_ADD_IP_FILTER_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for adding an IP filter was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
	
	
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}
	
	

	
	/* now we should add any port filters that are attached to this filter */
	if ( ((rule_p->protocol == DF_PROTO_TCP) || (rule_p->protocol == DF_PROTO_UDP) || (rule_p->protocol == DF_PROTO_SCTP))
	   && (adt_array_list_items(rule_p->port_rules) > 0) )
	{
		prv_download_port_rules (dagfd, ruleset, rule_p->port_rules, g_unique_id_count);
	}

	else if ( (rule_p->protocol == DF_PROTO_ICMP) && (adt_array_list_items(rule_p->icmp_type_rules) > 0) )
	{
		prv_download_icmp_type_rules (dagfd, ruleset, rule_p->icmp_type_rules, g_unique_id_count);
	}
	
	

	/* update the unique id count */
	g_unique_id_count++;
	
	
	return 0;
}







/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_download_ruleset
 
 DESCRIPTION:   
 
 PARAMETERS:    dagfd            IN       
                interface        IN       
                ruleset          IN       
 
 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_download_ruleset (int dagfd, uint8_t iface, RulesetH ruleset_h)
{
	df_ruleset_t*                     df_ruleset_p;
	d71s_ruleset_t*                   d71s_ruleset_p;
	ListPtr                           rule_list;
	IteratorPtr                       iterator_p;
	df_rule_meta_t  *                 rule_meta_p;
	uint32_t                          index;
	uint32_t                          items;
	int                               result;
	
	t_filter_msg_create_rulset_cmplt  response_msg;
	uint32_t                          msg_id;
	uint32_t                          msg_len;
	
	
	/* sanity checking */
	if (g_downloaded_rulesets == NULL)
	{
		dagixp_filter_set_last_error(ENOMEM);
		return -1;
	}
	
	/* more checking */
	df_ruleset_p = (df_ruleset_t*)ruleset_h;
	if ( (df_ruleset_p == NULL) || (df_ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error(EINVAL);
		return -1;
	}
	
	rule_list = df_ruleset_p->rule_list;
	
	


	/* 
	 * first check if we have already downloaded the ruleset, if we have
	 * then we need to delete it before downloading the ruleset again.
	 */
	items = adt_array_list_items (g_downloaded_rulesets);
	for (index=1; index<=items; index++)
	{
		d71s_ruleset_p = adt_array_list_get_indexed_item(g_downloaded_rulesets, index);
		if ( (d71s_ruleset_p) && (d71s_ruleset_p->ruleset_h == ruleset_h) )
		{
			if (d71s_filter_remove_ruleset(dagfd, iface, ruleset_h) == -1)
				return -1;
			
			break;
		}
	}	
	
	



	/* send a request to create a new ruleset */
	if ( dagema_send_msg(dagfd, EMA_MSG_CREATE_RULESET, 0, NULL, NULL) != 0 )
	{
		dagutil_warning ("failed to send request for creating a new ruleset (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}

	
	/* read the response */
	msg_len = EMA_MSG_CREATE_RULESET_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, DEFAULT_EMA_TIMEOUT) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* check the response */
	if ( (msg_id != EMA_MSG_CREATE_RULESET_CMPLT) || (msg_len != EMA_MSG_CREATE_RULESET_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for creating a new ruleset was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
	
	
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}
	
	
	
	/* add the ruleset handle to the local array */
	d71s_ruleset_p = malloc(sizeof(d71s_ruleset_t));
	d71s_ruleset_p->ruleset_h  = ruleset_h;
	d71s_ruleset_p->ruleset_id = dagema_le16toh(response_msg.ruleset);
	
	adt_array_list_add_first (g_downloaded_rulesets, d71s_ruleset_p);
	
	
	/* now add the IP filters to the ruleset just created */
	iterator_p = adt_list_get_iterator(rule_list);
	if (iterator_p == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	
	/* loop through the list items */
	result = 0;
	adt_iterator_first (iterator_p);
	while (adt_iterator_is_in_list(iterator_p))
	{
		rule_meta_p = (df_rule_meta_t*) adt_iterator_retrieve(iterator_p);
		result = 0;
		
		if (rule_meta_p->signature == SIG_IPV4_RULE)
			result = prv_download_ipv4_rule (dagfd, d71s_ruleset_p->ruleset_id, (df_ipv4_rule_t*)rule_meta_p);
		
		else if (rule_meta_p->signature == SIG_IPV6_RULE)
			result = prv_download_ipv6_rule (dagfd, d71s_ruleset_p->ruleset_id, (df_ipv6_rule_t*)rule_meta_p);
		
		if (result == -1)
			break;
		
		adt_iterator_advance(iterator_p);
	}
	
	adt_iterator_dispose (iterator_p);
	
	
	return result;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_activate_ruleset
 
 DESCRIPTION:   
 
 PARAMETERS:    dagfd            IN       
                interface        IN       
                ruleset          IN       
 
 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_activate_ruleset (int dagfd, uint8_t iface, RulesetH ruleset_h)
{
	df_ruleset_t*                      df_ruleset_p;
	d71s_ruleset_t*                    d71s_ruleset_p;
	uint32_t                           index;
	uint32_t                           items;
	uint32_t                           timeout;
	uint16_t                           ruleset_id = 0xFFFF;
	t_filter_msg_activate_rulset       request_msg;
	t_filter_msg_activate_rulset_cmplt response_msg;
	uint32_t                           msg_id;
	uint32_t                           msg_len;




	/* sanity checking */
	if (g_downloaded_rulesets == NULL)
	{
		dagixp_filter_set_last_error(ENOMEM);
		return -1;
	}
	
	/* more checking */
	df_ruleset_p = (df_ruleset_t*)ruleset_h;
	if ( (df_ruleset_p == NULL) || (df_ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error(EINVAL);
		return -1;
	}


	/* first check to make sure the ruleset has been downloaded and then
	 * retreive the ruleset id.	 */
	items = adt_array_list_items (g_downloaded_rulesets);
	for (index=1; index<=items; index++)
	{
		d71s_ruleset_p = adt_array_list_get_indexed_item(g_downloaded_rulesets, index);
		if ( (d71s_ruleset_p) && (d71s_ruleset_p->ruleset_h == ruleset_h) )
		{
			ruleset_id = d71s_ruleset_p->ruleset_id;
			break;
		}
	}
	
	/* check to make sure the ruleset is in our local array of downloaded rulesets */
	if (index > items)
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}	
	

	
	/* populate the activate request */
	request_msg.ruleset = dagema_htole16(ruleset_id);
	
	
	/* send a request to activate a ruleset */
	if ( dagema_send_msg(dagfd, EMA_MSG_ACTIVATE_RULESET, EMA_MSG_ACTIVATE_RULESET_SIZE, (uint8_t*)(&request_msg), NULL) != 0 )
	{
		dagutil_warning ("failed to send request for activating a ruleset (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}

	
	/* read the response */

	/* this may take some time depending on the size of the ruleset, therefore the
	 * timeout is adjust to take into account the number of filters */
	timeout = DEFAULT_EMA_TIMEOUT;
	if ( df_ruleset_p->rule_list )
		timeout += ( adt_list_items(df_ruleset_p->rule_list) * 100 );

	msg_len = EMA_MSG_ACTIVATE_RULESET_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, timeout) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* check the response */
	if ( (msg_id != EMA_MSG_ACTIVATE_RULESET_CMPLT) || (msg_len != EMA_MSG_ACTIVATE_RULESET_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for activating a ruleset was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
	
	
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_remove_ruleset
 
 DESCRIPTION:	
 
 PARAMETERS:	dagfd            IN       
                interface        IN       
                ruleset          IN       
 
 RETURNS:		

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_remove_ruleset (int dagfd, uint8_t iface, RulesetH ruleset_h)
{
	df_ruleset_t*                     df_ruleset_p;
	d71s_ruleset_t*                   d71s_ruleset_p;
	uint32_t                          index;
	uint32_t                          items;
	uint16_t                          ruleset_id = 0xFFFF;

	t_filter_msg_delete_rulset        request_msg;
	t_filter_msg_delete_rulset_cmplt  response_msg;
	uint32_t                          msg_id;
	uint32_t                          msg_len;

	
	
	/* sanity checking */
	if (g_downloaded_rulesets == NULL)
	{
		dagixp_filter_set_last_error(ENOMEM);
		return -1;
	}
	
	/* more checking */
	df_ruleset_p = (df_ruleset_t*)ruleset_h;
	if ( (df_ruleset_p == NULL) || (df_ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error(EINVAL);
		return -1;
	}
	


	/* first check to make sure the ruleset has been downloaded and then
	 * retreive the ruleset id.	 */
	items = adt_array_list_items (g_downloaded_rulesets);
	for (index=1; index<=items; index++)
	{
		d71s_ruleset_p = adt_array_list_get_indexed_item(g_downloaded_rulesets, index);
		if ( (d71s_ruleset_p) && (d71s_ruleset_p->ruleset_h == ruleset_h) )
		{
			ruleset_id = d71s_ruleset_p->ruleset_id;
			break;
		}
	}
	
	/* check to make sure the ruleset is in our local array of downloaded rulesets */
	if (index > items)
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	
	
	/* populate the delete request */
	request_msg.ruleset = dagema_htole16(ruleset_id);
	
	
	/* send a request to delete a ruleset */
	if ( dagema_send_msg(dagfd, EMA_MSG_DELETE_RULESET, EMA_MSG_DELETE_RULESET_SIZE, (uint8_t*)(&request_msg), NULL) != 0 )
	{
		dagutil_warning ("failed to send request for creating a new ruleset (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}

	
	/* read the response */
	msg_len = EMA_MSG_ADD_IP_FILTER_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, DEFAULT_EMA_TIMEOUT) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* check the response */
	if ( (msg_id != EMA_MSG_ADD_IP_FILTER_CMPLT) || (msg_len != EMA_MSG_ADD_IP_FILTER_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for creating an new ruleset was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
	
	
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagfilter_clear_iface_rulesets
 
 DESCRIPTION:   
 
 PARAMETERS:    dagfd            IN       
                interface        IN       
 
 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_clear_iface_rulesets (int dagfd, uint8_t iface)
{
	uint32_t           msg_id;
	uint32_t           msg_len;

	t_filter_msg_remove_all_rulesets        request_msg;
	t_filter_msg_remove_all_rulesets_cmplt  response_msg;

	
	/* set the request parameters */
	request_msg.iface = dagema_htole32(iface);
	
	
	/* send a request to delete a ruleset */
	if ( dagema_send_msg(dagfd, EMA_MSG_REMOVE_ALL_RULESETS, EMA_MSG_REMOVE_ALL_RULESETS_SIZE, (uint8_t*)(&request_msg), NULL) != 0 )
	{
		dagutil_warning ("failed to send request for creating a new ruleset (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}

	
	/* read the response */
	msg_len = EMA_MSG_REMOVE_ALL_RULESETS_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, DEFAULT_EMA_TIMEOUT) != 0 )
	{
		dagutil_warning ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}		
			
		
	/* check the response */
	if ( (msg_id != EMA_MSG_REMOVE_ALL_RULESETS_CMPLT) || (msg_len != EMA_MSG_REMOVE_ALL_RULESETS_CMPLT_SIZE) )
	{
		dagutil_warning ("response to the request for removing all rulesets was not what was expected.\n");
		dagixp_filter_set_last_error (EINVALIDRESP);
		return -1;
	}
			
			
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}				
	
	return 0;	

}
