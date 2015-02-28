/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_feature.c,v 1.2 2006/03/28 04:23:21 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         df_load.c
* DESCRIPTION:  dagixp_filter API functions used to load and activate a ruleset
*               on a dag card.
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
#include "dagpci.h"
#include "df_types.h"
#include "df_internal.h"

/* Card specific headers */
#include "d71s_filter.h"



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_hw_get_ruleset_count
 
 DESCRIPTION:   Returns the max number of rulesets that can be downloaded
                per the interface specified. This is NOT the number of
                active rulesets (there can only be on active ruleset per
                interface), rather it is the number of rulesets that can
                be download before the memory becomes full.
 
 PARAMETERS:    dagfd      IN       The file descriptor of the card
                interface  IN       The interface to get the maximum
                                    rulesets from.
              

 RETURNS:       positive number of maximum filters, or if there was
                an error -1

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_hw_get_ruleset_count (int dagfd, uint8_t iface)
{
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	

	return 0;
}



	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_hw_get_max_filters
 
 DESCRIPTION:   Returns the max number of filters that can be active on
                a given interface.
 
 PARAMETERS:    dagfd      IN       The file descriptor of the card
                interface  IN       The interface to get the max filter
                                    count from.
              

 RETURNS:       positive number of maximum filters, or if there was
                an error -1

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_hw_get_max_filters (int dagfd, uint8_t iface)
{
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}



	return 0;
}






/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_hw_get_filter_space
 
 DESCRIPTION:   Returns the number of filters that can added to a
                particular ruleset on a particular interface.
 
 PARAMETERS:    dagfd      IN       The file descriptor of the card
                interface  IN       The interface to get the space filter
                                    count from.
                ruleset    IN       The ruleset from which to get the
                                    available size.

 RETURNS:       positive number of maximum filters, or if there was
                an error -1

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_hw_get_filter_space (int dagfd, uint8_t iface, RulesetH ruleset)
{
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}



	return 0;
}


	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_hw_get_filter_stat
 
 DESCRIPTION:   Returns filter statisitics collected so far by the
                dag card. Each statistic is retreived indepently, by
                specifing a stat argument.
 
 PARAMETERS:    dagfd      IN       The file descriptor of the card
                stat       IN       The statistic you want to get
                stat_low   OUT      Pointer to a variable that receives
                                    the lower 32-bits of the statistic
                stat_high  OUT      Pointer to a variable that receives
                                    the upper 32-bits of the statistic,
                                    this argument can be NULL if the
                                    statistic is only a 32-bit value.
              

 RETURNS:       0 if there were no errors, otherwise -1. Use 
                dagixp_filter_get_last_error() to get the error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_hw_get_filter_stat (int dagfd, statistic_t stat, uint32_t *stat_low, uint32_t *stat_high)
{
	daginf_t* daginf;

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* get the dag card type */
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagixp_filter_set_last_error (EBADF);
		return -1;
	}
	
	
	/* pass the processing onto the card specific functions */
	switch (daginf->device_code)
	{
		/* pos filtering on the 7.1s */
		case PCI_DEVICE_ID_DAG7_10:
			return d71s_filter_get_filter_stat (dagfd, stat, stat_low, stat_high);
		
				
		default:
			dagixp_filter_set_last_error (EUNSUPPORTDEV);
			return -1;
	}

	
}
	


/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_hw_reset_filter_stat
 
 DESCRIPTION:   Resets a filtering statistic kept by the dag card.
 
 PARAMETERS:    dagfd      IN       The file descriptor of the card
                stat       IN       The statistic to reset

 RETURNS:       0 if there were no errors, otherwise -1. Use 
                dagixp_filter_get_last_error() to get the error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_hw_reset_filter_stat (int dagfd, statistic_t stat)
{
	daginf_t* daginf;
	int       result;

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* get the dag card type */
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagixp_filter_set_last_error (EBADF);
		return -1;
	}
	



	
	/* pass the processing onto the card specific functions */
	switch (daginf->device_code)
	{
		/* pos filtering on the 7.1s */
		case PCI_DEVICE_ID_DAG7_10:
			if ( stat != kAllStatistics )
				return d71s_filter_reset_filter_stat (dagfd, stat);
			else
			{
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsRecv)) != 0 )                   return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsAccepted)) != 0 )               return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsInPlay)) != 0 )                 return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedErfType)) != 0 )         return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedHdlc)) != 0 )            return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedErfSize)) != 0 )         return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedNoFree)) != 0 )          return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedRxFull)) != 0 )          return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedTxFull)) != 0 )          return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedIPv6ExtHeader)) != 0 )   return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedSeqBuffer)) != 0 )       return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedTooBig)) != 0 )          return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kPacketsDroppedMplsOverflow)) != 0 )    return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kMsfErrUnexpectedSOP)) != 0 )           return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kMsfErrUnexpectedEOP)) != 0 )           return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kMsfErrUnexpectedBOP)) != 0 )           return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kMsfErrNullMPacket)) != 0 )             return result;
				if ( (result = d71s_filter_reset_filter_stat(dagfd, kMsfErrStatusWord)) != 0 )              return result;
				return 0;
			}
				
		default:
			dagixp_filter_set_last_error (EUNSUPPORTDEV);
			return -1;
	}
	

}
