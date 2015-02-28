/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_load.c,v 1.1 2006/02/27 10:16:16 ben Exp $
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
#include "df_internal.h"

/* Card specific headers */
#include "d71s_filter.h"




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_download_ruleset
 
 DESCRIPTION:   
 
 PARAMETERS:    dagfd            IN       Unix style file descriptor to
                                          the dag card to activate the
                                          ruleset on.
                interface        IN       The interface that the ruleset
                                          is applied to.
                ruleset          IN       The ruleset to download to the
                                          card for filtering.

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_download_ruleset (int dagfd, uint8_t iface, RulesetH ruleset)
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
			return d71s_filter_download_ruleset (dagfd, iface, ruleset);
	
				
		default:
			dagixp_filter_set_last_error (EUNSUPPORTDEV);
			return -1;
	}
	
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_activate_ruleset
 
 DESCRIPTION:   Activates a ruleset on a particular interface, the ruleset
                must have already been downloaded to the card (on the same
                interface) prior actually activating it.
 
 PARAMETERS:    dagfd            IN       Unix style file descriptor to
                                          the dag card to activate the
                                          ruleset on.
                interface        IN       The interface that the ruleset
                                          is applied to.
                ruleset          IN       The ruleset to activate on the
                                          card, the ruleset should already
                                          have been downloaded.
 
 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_activate_ruleset (int dagfd, uint8_t iface, RulesetH ruleset)
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
			return d71s_filter_activate_ruleset (dagfd, iface, ruleset);
		
				
		default:
			dagixp_filter_set_last_error (EUNSUPPORTDEV);
			return -1;
	}
	
	return 0;
}






/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_remove_ruleset
 
 DESCRIPTION:   Removes a downloaded ruleset from the card, if the ruleset
                was the active one, all filters are removed and all packets
                are accepted with a color or 0x3FFFF.
 
 PARAMETERS:    dagfd            IN       Unix style file descriptor to
                                          the dag card to activate the
                                          ruleset on.
                interface        IN       The interface that the ruleset
                                          is applied to.
                ruleset          IN       The ruleset to remove from the
                                          card.
 
 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_remove_ruleset (int dagfd, uint8_t iface, RulesetH ruleset)
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
			return d71s_filter_remove_ruleset (dagfd, iface, ruleset);
		
				
		default:
			dagixp_filter_set_last_error (EUNSUPPORTDEV);
			return -1;
	}
	
	return 0;
}


	

/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_clear_iface_rulesets
 
 DESCRIPTION:   Removes all downloaded ruleset from the card
 
 PARAMETERS:    dagfd            IN       Unix style file descriptor to
                                          the dag card to activate the
                                          ruleset on.
                interface        IN       The interface that the ruleset
                                          is applied to.
                ruleset          IN       The ruleset to remove from the
                                          card.
 
 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_clear_iface_rulesets (int dagfd, uint8_t iface)
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
			return d71s_filter_clear_iface_rulesets (dagfd, iface);
		
				
		default:
			dagixp_filter_set_last_error (EUNSUPPORTDEV);
			return -1;
	}
	
	return 0;
}
