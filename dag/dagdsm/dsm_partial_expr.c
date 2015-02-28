/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_partial_expr.c,v 1.3 2006/09/04 23:15:05 jeff Exp $
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





/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_set_filter
 
 DESCRIPTION:   Adds a filter to the partial expression expression, this
                has the effect of ORing the filter (or the inverse of the
                filter) with the existing partial expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                filter          IN      The number of the filter to add,
                                        should be in the range of 0-6.
                invert          IN      If 1 the filter is inverted before
                                        ORing with the partial expression.

 RETURNS:       0 if the filter was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_set_filter (DsmPartialExpH expr_h, uint32_t filter, uint32_t invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( filter >= (FILTER_COUNT - 1) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	if ( invert == 0 )
		expr_p->filter[filter] = kNonInverted;
	else
		expr_p->filter[filter] = kInverted;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_filter
 
 DESCRIPTION:   Get a filter from the partial expression, this
                has the effect of ORing the filter (or the inverse of the
                filter) with the existing partial expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                filter          OUT     The number of the filter to get,
                                        should be in the range of 0-6.
                invert          OUT     If 1 the filter is inverted before
                                        ORing with the partial expression.

 RETURNS:       0 if the filter was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_filter (DsmPartialExpH expr_h, uint32_t filter, uint32_t * invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( filter >= (FILTER_COUNT - 1) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
    switch(expr_p->filter[filter])
    {
        case kNonInverted:
            *invert = 0;
            break;
        case kInverted:
            *invert = 1;
            break;
        default:
            *invert = -1;
            break;
    }

	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_filter_count
 
 DESCRIPTION:   Gets the maximum number of filters possible for the Partial expression
                has the effect of ORing the filter (or the inverse of the
                filter) with the existing partial expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                count           OUT     The number of filters 

 RETURNS:       0 if the filter was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_filter_count (DsmPartialExpH expr_h, uint32_t * count)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
    *count = FILTER_COUNT;

	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_set_interface
 
 DESCRIPTION:   Adds a interface to the partial expression expression, this
                has the effect of ORing the interface (or the inverse of the
                interface) with the existing partial expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                iface           IN      The number of the interface to add,
                                        should be in the range of 0-3.
                invert          IN      If 1 the interface is inverted before
                                        ORing with the partial expression.

 RETURNS:       0 if the interface was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_set_interface (DsmPartialExpH expr_h, uint32_t iface, uint32_t invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( iface >= expr_p->interfaces )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	if ( invert == 0 )
		expr_p->iface[iface] = kNonInverted;
	else
		expr_p->iface[iface] = kInverted;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_interface
 
 DESCRIPTION:   Gets an interface from the partial expression expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                iface           IN      The number of the interface to get,
                                        should be in the range of 0-3.
                invert          OUT     If 1 the interface is inverted before
                                        ORing with the partial expression.

 RETURNS:       0 if the interface was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_interface (DsmPartialExpH expr_h, uint32_t iface, uint32_t * invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( iface >= expr_p->interfaces )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
    switch(expr_p->iface[iface])
    {
        case kNonInverted:
            *invert = 0;
            break;
        case kInverted:
            *invert = 1;
            break;
        default:
            *invert = -1;
            break;
    }
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_interface_count
 
 DESCRIPTION:   Gets the number of interface from the partial expression expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                count           OUT     The Number of interfaces 

 RETURNS:       0 if the interface was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_interface_count (DsmPartialExpH expr_h, uint32_t * count )
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
    *count = expr_p->interfaces;

    return 0;
}
	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_set_hlb
 
 DESCRIPTION:   Adds a HLB output to the partial expression expression, this
                has the effect of ORing the interface (or the inverse of the
                HLB) with the existing partial expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                hlb             IN      The HLD output to add,
                                        should be either 0 or 1.
                invert          IN      If 1 the HLB output is inverted before
                                        ORing with the partial expression.

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_set_hlb (DsmPartialExpH expr_h, uint32_t hlb, uint32_t invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( hlb >= HLB_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	if ( invert == 0 )
		expr_p->hlb[hlb] = kNonInverted;
	else
		expr_p->hlb[hlb] = kInverted;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_hlb
 
 DESCRIPTION:   Gets a HLB output from the partial expression expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                hlb             IN      The HLB output to get,
                                        should be either 0 or 1.
                invert          OUT     If 1 the HLB output is inverted before
                                        ORing with the partial expression.

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_hlb (DsmPartialExpH expr_h, uint32_t hlb, uint32_t * invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( hlb >= HLB_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
    switch(expr_p->hlb[hlb])
    {
        case kNonInverted:
            *invert = 0;
            break;
        case kInverted:
            *invert = 1;
            break;
        default:
            *invert = -1;
            break;
    }
    return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_set_hlb_hash
 
 DESCRIPTION:   Adds a HLB Hash Hat output to the partial expression expression,
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                hlb_hash_value  IN      The hlb hash value to add,
                                        should be value range in 0-7.
                invert          IN      If 1 the HLB hash value output is inverted before
                                        ORing with the partial expression.

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_set_hlb_hash(DsmPartialExpH expr_h, uint32_t hlb_hash_value, uint32_t invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( hlb_hash_value >= HLB_HASH_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	if ( invert == 0 )
		expr_p->hlb_hash[hlb_hash_value] = kNonInverted;
	else
		expr_p->hlb_hash[hlb_hash_value] = kInverted;
	
	return 0;
}

int
dagdsm_expr_get_hlb_hash(DsmPartialExpH expr_h, uint32_t hlb_hash_value, uint32_t* invert)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( hlb_hash_value >= HLB_HASH_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
    switch(expr_p->hlb_hash[hlb_hash_value])
    {
        case kNonInverted:
            *invert = 0;
            break;
        case kInverted:
            *invert = 1;
            break;
        default:
            *invert = -1;
            break;
    }
    return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_hlb_count
 
 DESCRIPTION:   Gets a HLB count from the partial expression expression.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                count           OUT     HLB count 

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_hlb_count (DsmPartialExpH expr_h, uint32_t * count)
{
	dsm_partial_expr_t   * expr_p = (dsm_partial_expr_t*) expr_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
    *count = HLB_COUNT;

    return 0;
}








/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_compute_partial_expr_value
 
 DESCRIPTION:   Recevies a set of parameters and determines if the
                partial expression will hit.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                filters         IN      Bitmask of the filters to check
                iface           IN      The interface number to check, 0-3
                hlb0            IN      The HLB0 output to check
                hlb1            IN      The HLB1 output to check

 RETURNS:       0 if the expression doesn't match, 1 if the expression
                does match and -1 if there was an error. This function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_compute_partial_expr_value (DsmPartialExpH expr_h, uint32_t filters, uint8_t iface, uint32_t hlb0, uint32_t hlb1)
{
	dsm_partial_expr_t * expr_p = (dsm_partial_expr_t*) expr_h;
	uint32_t             i;
	uint32_t             dont_cares;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	if ( iface >= INTERFACE_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* normalise the HLB arguments */
	if ( hlb0 != 0 )    hlb0 = 1;
	if ( hlb1 != 0 )    hlb1 = 1;
	
	
	
	
	/* check for the special case where all the parameters are 'don't cares' in 
	   in this case the partial expression should always return 'false'. */
	dont_cares = 0;
	
	for (i=0; i<FILTER_COUNT; i++)
		if ( expr_p->filter[i] == kDontCare )
			dont_cares++;
		
	for (i=0; i<HLB_COUNT; i++)
		if ( expr_p->hlb[i] == kDontCare )
			dont_cares++;
	
	for (i=0; i<INTERFACE_COUNT; i++)
		if ( expr_p->iface[i] == kDontCare )
			dont_cares++;
	
	if ( dont_cares == (FILTER_COUNT+HLB_COUNT+INTERFACE_COUNT) )
		return 0;
	
	
	
	
	/* since partial expressions are OR'ed together all we need to do is check for a hit */
	
	
	/* check the filters first */
	for (i=0; i<FILTER_COUNT; i++)
	{
		if ( (expr_p->filter[i] == kNonInverted) && ((filters & (1 << i)) != 0) )
			return 1;
		
		if ( (expr_p->filter[i] == kInverted)    && ((filters & (1 << i)) == 0) )
			return 1;
	}
	
	
	/* check the 2 HLB's */
	if ( (expr_p->hlb[0] == kNonInverted) && (hlb0 == 1) )
		return 1;
	if ( (expr_p->hlb[0] == kInverted)    && (hlb0 == 0) )
		return 1;
	
	if ( (expr_p->hlb[1] == kNonInverted) && (hlb1 == 1) )
		return 1;
	if ( (expr_p->hlb[1] == kInverted)    && (hlb1 == 0) )
		return 1;
	
	
	/* finally check the interface */
	for (i=0; i<INTERFACE_COUNT; i++)
	{
		if ( (i == iface) && (expr_p->iface[i] == kNonInverted) )
			return 1;
		if ( (i != iface) && (expr_p->iface[i] == kInverted) )
			return 1;
	}
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_compute_partial_expr_value_z
 
 DESCRIPTION:   Recevies a set of parameters and determines if the
                partial expression will hit.
 
 PARAMETERS:    expr_h          IN      Handle to the partial expression
                filters         IN      Bitmask of the filters to check
                iface           IN      The interface number to check, 0-3
                hlb0            IN      The HLB0 output to check
                hlb1            IN      The HLB1 output to check

 RETURNS:       0 if the expression doesn't match, 1 if the expression
                does match and -1 if there was an error. This function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_compute_partial_expr_value_z (DsmPartialExpH expr_h, uint32_t filters, uint8_t iface, uint32_t hlb_hash_value)
{
	dsm_partial_expr_t * expr_p = (dsm_partial_expr_t*) expr_h;
	uint32_t             i;
	uint32_t             dont_cares;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (expr_p == NULL) || (expr_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	if ( iface >= INTERFACE_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( hlb_hash_value >= 	HLB_HASH_COUNT)
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* check for the special case where all the parameters are 'don't cares' in 
	   in this case the partial expression should always return 'false'. */
	dont_cares = 0;
	
	for (i=0; i<FILTER_COUNT; i++)
		if ( expr_p->filter[i] == kDontCare )
			dont_cares++;
		
	for (i=0; i<HLB_COUNT; i++)
		if ( expr_p->hlb[i] == kDontCare )
			dont_cares++;
	
	for (i=0; i<INTERFACE_COUNT; i++)
		if ( expr_p->iface[i] == kDontCare )
			dont_cares++;

	for (i=0; i<HLB_HASH_COUNT; i++)
		if ( expr_p->hlb_hash[i] == kDontCare )
			dont_cares++;


	if ( dont_cares == (FILTER_COUNT+HLB_COUNT+INTERFACE_COUNT+HLB_HASH_COUNT) )
		return 0;
	
	
	
	
	/* since partial expressions are OR'ed together all we need to do is check for a hit */
	
	
	/* check the filters first */
	for (i=0; i<FILTER_COUNT; i++)
	{
		if ( (expr_p->filter[i] == kNonInverted) && ((filters & (1 << i)) != 0) )
			return 1;
		
		if ( (expr_p->filter[i] == kInverted)    && ((filters & (1 << i)) == 0) )
			return 1;
	}
	
	
	/* check the interface */
	for (i=0; i<INTERFACE_COUNT; i++)
	{
		if ( (i == iface) && (expr_p->iface[i] == kNonInverted) )
			return 1;
		if ( (i != iface) && (expr_p->iface[i] == kInverted) )
			return 1;
	}
	
	/* finally check the hlb_hash*/
	for(i=0; i<HLB_HASH_COUNT;i++)
	{
		if ( (i == hlb_hash_value) && (expr_p->hlb_hash[i] == kNonInverted) )
			return 1;
		if ( (i != hlb_hash_value) && (expr_p->hlb_hash[i] == kInverted) )
			return 1;

	}
	
	return 0;
}
