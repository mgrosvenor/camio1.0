/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_error.c,v 1.1 2006/02/27 06:50:15 ben Exp $
*
**********************************************************************/

/**********************************************************************
* 
* DESCRIPTION:  
*
*
* 
*
**********************************************************************/


/* System headers */
#include "dag_platform.h"

/* DSM headers */
#include "dagdsm.h"
#include "dsm_types.h"




/* Stores the last error code */
int g_dsm_last_error = 0;




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_set_last_error
                dagdsm_get_last_error
 
 DESCRIPTION:   Gets and sets the last error code.
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
void 
dagdsm_set_last_error (int errorcode)
{
	g_dsm_last_error = errorcode;
}

int
dagdsm_get_last_error (void)
{
	return g_dsm_last_error;
}
