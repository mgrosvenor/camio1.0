/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: d71s_feature.c,v 1.2 2006/02/27 10:48:32 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         d71s_feature.c
* DESCRIPTION:  Contains the filter feature requests for the dag7.1s.
*
*
* HISTORY:      15-12-05 Initial revision
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"


/* Endace headers */
#include "dagapi.h"
#include "dagema.h"
#include "dagutil.h"
#include "dagixp_filter.h"
#include "df_internal.h"
#include "df_types.h"

#include "d71s_filter.h"










/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_get_ruleset_count
 
 DESCRIPTION:   
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_get_ruleset_count (int dagfd, uint8_t iface)
{
	
	
	
	return 0;
}


	
/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_get_max_filters
 
 DESCRIPTION:   
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_get_max_filters (int dagfd, uint8_t iface)
{
	
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_get_filter_space
 
 DESCRIPTION:   
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_get_filter_space (int dagfd, uint16_t ruleset)
{
	
	
	return 0;
}
	

/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_get_filter_stat
 
 DESCRIPTION:   
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_get_filter_stat (int dagfd, statistic_t stat, uint32_t *stat_low, uint32_t *stat_high)
{
	t_filter_msg_get_statistics       request_msg;
	t_filter_msg_get_statistics_cmplt response_msg;
	uint32_t                          msg_id;
	uint32_t                          msg_len;
	
	
	/* sanity check */
	if ( stat_low == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* populate the message to be sent to the card */
	switch (stat)
	{
		case kPacketsRecv:                   
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_RECV);
			break;
		case kPacketsAccepted:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_ACCEPTED);
			break;
	
		case kPacketsInPlay:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_IN_PLAY);
			break;
		case kPacketsDroppedErfType:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_ERF_TYPE);
			break;
		case kPacketsDroppedHdlc:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_HDLC);
			break;
		case kPacketsDroppedErfSize:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_ERF_SIZE);
			break;
		case kPacketsDroppedNoFree:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_NO_FREE);
			break;
		case kPacketsDroppedRxFull:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_RX_FULL);
			break;
		case kPacketsDroppedTxFull:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_TX_FULL);
			break;
		case kPacketsDroppedIPv6ExtHeader:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_IPV6_EXT_HEADER);
			break;
		case kPacketsDroppedSeqBuffer:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_SEQ_BUFFER);
			break;
		case kPacketsDroppedTooBig:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_TOO_BIG);
			break;
		case kPacketsDroppedMplsOverflow:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_MPLS_OVERFLOW);
			break;
		case kMsfErrUnexpectedSOP:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_UNEXP_SOP);
			break;
		case kMsfErrUnexpectedEOP:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_UNEXP_EOP);
			break;
		case kMsfErrUnexpectedBOP:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_UNEXP_BOP);
			break;
		case kMsfErrNullMPacket:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_NULL_MPACKET);
			break;
		case kMsfErrStatusWord:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_STATUS_WORD);
			break;
	

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
		
	
	
	/* send a request for the statistic */
	if ( dagema_send_msg(dagfd, EMA_MSG_GET_STATISTICS, EMA_MSG_GET_STATISTICS_SIZE, (uint8_t*)&request_msg, NULL) != 0 )
	{
		dagutil_error ("failed to send request for statistics to the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* read the response */
	msg_len = EMA_MSG_GET_STATISTICS_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, DEFAULT_EMA_TIMEOUT) != 0 )
	{
		dagutil_error ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}		
	

	/* check the response */
	if ( (msg_id != EMA_MSG_GET_STATISTICS_CMPLT) || (msg_len != EMA_MSG_GET_STATISTICS_CMPLT_SIZE) )
	{
		dagutil_error ("response to the request for the statistics was not what was expected.\n");
		dagixp_filter_set_last_error (-3);
		return -1;
	}
	
	
	/* check the return code */
	if ( dagema_le32toh(response_msg.result) != 0 )
	{
		dagixp_filter_set_last_error (dagema_le32toh(response_msg.result));
		return -1;
	}
		
	
	
	/* copy the statistic into the supplied arguments */
	*stat_low  = dagema_le32toh(response_msg.stat_lo);
	if (stat_high)
		*stat_high = dagema_le32toh(response_msg.stat_hi);
	
	return 0;
}
	
/*--------------------------------------------------------------------
 
 FUNCTION:      d71s_filter_display_stat
 
 DESCRIPTION:   
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
d71s_filter_reset_filter_stat (int dagfd, statistic_t stat)
{
	t_filter_msg_reset_statistics       request_msg;
	t_filter_msg_reset_statistics_cmplt response_msg;
	uint32_t                            msg_id;
	uint32_t                            msg_len;
	
	

	/* populate the message to be sent to the card */
	switch (stat)
	{
		case kPacketsRecv:                   
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_RECV);
			break;
		case kPacketsAccepted:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_ACCEPTED);
			break;
	
		case kPacketsInPlay:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_IN_PLAY);
			break;
		case kPacketsDroppedErfType:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_ERF_TYPE);
			break;
		case kPacketsDroppedHdlc:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_HDLC);
			break;
		case kPacketsDroppedErfSize:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_ERF_SIZE);
			break;
		case kPacketsDroppedNoFree:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_NO_FREE);
			break;
		case kPacketsDroppedRxFull:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_RX_FULL);
			break;
		case kPacketsDroppedTxFull:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_TX_FULL);
			break;
		case kPacketsDroppedIPv6ExtHeader:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_IPV6_EXT_HEADER);
			break;
		case kPacketsDroppedSeqBuffer:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_SEQ_BUFFER);
			break;
		case kPacketsDroppedTooBig:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_TOO_BIG);
			break;
		case kPacketsDroppedMplsOverflow:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_PACKETS_DROPPED_MPLS_OVERFLOW);
			break;
		case kMsfErrUnexpectedSOP:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_UNEXP_SOP);
			break;
		case kMsfErrUnexpectedEOP:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_UNEXP_EOP);
			break;
		case kMsfErrUnexpectedBOP:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_UNEXP_BOP);
			break;
		case kMsfErrNullMPacket:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_NULL_MPACKET);
			break;
		case kMsfErrStatusWord:
			request_msg.statistic = dagema_htole16((uint16_t)STAT_MSFERR_STATUS_WORD);
			break;
		
		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
		
	
	
	/* send a request for the statistic */
	if ( dagema_send_msg(dagfd, EMA_MSG_RESET_STATISTICS, EMA_MSG_RESET_STATISTICS_SIZE, (uint8_t*)&request_msg, NULL) != 0 )
	{
		dagutil_error ("failed to send request for statistics to the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}
	
	
	/* read the response */
	msg_len = EMA_MSG_RESET_STATISTICS_CMPLT_SIZE;
	if ( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)(&response_msg), NULL, DEFAULT_EMA_TIMEOUT) != 0 )
	{
		dagutil_error ("failed to read the response from the card (error code %d).\n", dagema_get_last_error());
		dagixp_filter_set_last_error (dagema_get_last_error());
		return -1;
	}		
	
	
	/* check the response */
	if ( (msg_id != EMA_MSG_RESET_STATISTICS_CMPLT) || (msg_len != EMA_MSG_RESET_STATISTICS_CMPLT_SIZE) )
	{
		dagutil_error ("response to the request for the statistics was not what was expected.\n");
		dagixp_filter_set_last_error (-3);
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
