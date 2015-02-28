/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_init.c,v 1.1 2006/02/27 10:16:16 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         df_init.c
* DESCRIPTION:  Contains the startup and shutdown code for the library.
*
*
* HISTORY:      15-12-05 Initial revision
*
**********************************************************************/


/* System headers */
#include "dag_platform.h"


/* Endace headers */
#include "dagixp_filter.h"
#include "df_internal.h"


/* Card headers */
#include "d71s_filter.h"



static int g_initialised = 0;



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_is_initialised
 
 DESCRIPTION:   Return value is used to indicate whether the library
                has been initialised.
 
 PARAMETERS:    none

 RETURNS:       1 if the library has been initialised otherwise 0

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_is_initialised (void)
{
	return g_initialised;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_startup
 
 DESCRIPTION:   Initialises the API and memory structures used by the
                library.
 
 PARAMETERS:    none

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_startup (void)
{
	dagixp_filter_set_last_error(0);
	
	/* check if already initialised */
	if (g_initialised == 1)
	{
		dagixp_filter_set_last_error(EALREADYOPEN);
		return -1;
	}


	/* initialise the generic api */
	
	
	/* initialise the 7.1s part of the library */
	if ( d71s_filter_startup() != 0 )
		return -1;
	

	g_initialised = 1;
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_shutdown
 
 DESCRIPTION:   Releases the memory used for the dagixp_filter library.
 
 PARAMETERS:    none

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_shutdown (void)
{
	dagixp_filter_set_last_error(0);

	/* check if the library was actually opened */
	if (g_initialised == 0)
	{
		dagixp_filter_set_last_error(ENOTOPEN);
		return -1;
	}
		

	/* TODO: Add garbage collection, users should have freed the rulesets before
	 *       closing the library, but we may want to check and free them if not
	 *       done by the user.
	 */
	
	
	/* cleanup the 7.1s part of the library */
	d71s_filter_shutdown();

	
	g_initialised = 0;
	return 0;
}
