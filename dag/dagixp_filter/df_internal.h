/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_internal.h,v 1.1 2006/02/27 10:16:16 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         df_internal.h
* DESCRIPTION:  Header file used internally by the dagixp_filter library.
*
*
* HISTORY:      15-12-05 Initial revision
*
**********************************************************************/
#ifndef DF_INTERNAL_H
#define DF_INTERNAL_H










/*--------------------------------------------------------------------
 
 FUNCTION PROTOTYPES

---------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	
	extern void
	dagixp_filter_set_last_error (int errorcode);
	
	extern int
	dagixp_filter_is_initialised (void);
	
	
	
#ifdef __cplusplus
}
#endif /* __cplusplus */




#endif  // DF_INTERNAL_H
