/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_output_expr.c,v 1.6 2006/09/04 23:15:05 jeff Exp $
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
 
 FUNCTION:      dagdsm_expr_add_partial_expr
 
 DESCRIPTION:   Adds a partial expression to the output expression. This
                has the effect of ANDing the output of the partial
                expression with the existing output expression.
 
 PARAMETERS:    output_h        IN      Handle to the output expression
                partial_h       IN      The partial expression to add
                                        the output expression.
                invert          IN      If 1 the output of the partial
                                        expression is inverted before
                                        ANDing with the rest of the output
                                        expression.

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_add_partial_expr (DsmOutputExpH output_h, DsmPartialExpH partial_h, uint32_t invert)
{
	dsm_output_expr_t   * output_p  = (dsm_output_expr_t*)  output_h;
	dsm_partial_expr_t  * partial_p = (dsm_partial_expr_t*) partial_h;
	IteratorPtr           iterator_p;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (output_p == NULL) || (output_p->signature != DSM_SIG_OUTPUT_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( (partial_p == NULL) || (partial_p->signature != DSM_SIG_PARTIAL_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	/* add the partial expression to the output expression */
	if ( invert == 0 )
		iterator_p = adt_list_get_iterator (output_p->partial_expr);
	else
		iterator_p = adt_list_get_iterator (output_p->inv_partial_expr);
	
	if ( iterator_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	
	
	/* add to the end of the list */
	adt_iterator_first (iterator_p);
	adt_iterator_add (iterator_p, partial_h);
	adt_iterator_dispose (iterator_p);
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_invert_partial_expr_count
 
 DESCRIPTION:   Gets the number of inverted partial expressions 
 
 PARAMETERS:    output_h        IN      Handle to the output expression
                                        the output expression.
                count           OUT     The number of inverted partial expressions

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_inverted_partial_expr_count (DsmOutputExpH output_h, uint32_t * count)
{
	dsm_output_expr_t   * output_p  = (dsm_output_expr_t*)  output_h;
	IteratorPtr           iterator_p;
    uint32_t t_count = 0;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (output_p == NULL) || (output_p->signature != DSM_SIG_OUTPUT_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
    t_count = 0;
    iterator_p = adt_list_get_iterator (output_p->inv_partial_expr);

	if ( iterator_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	    
    adt_iterator_first(iterator_p);
    while(adt_iterator_is_in_list(iterator_p))
    {
        t_count++;
        adt_iterator_advance(iterator_p);
    }
    *count = t_count;
            
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_invert_partial_expr
 
 DESCRIPTION:   Gets the number of inverted partial expressions 
 
 PARAMETERS:    output_h        IN      Handle to the output expression
                                        the output expression.
                index           IN      The index of partial expr
                partial_h       OUT     Partial expression 

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
DsmPartialExpH
dagdsm_expr_get_inverted_partial_expr (DsmOutputExpH output_h, uint32_t index)
{
    DsmPartialExpH  partial_h = NULL;
	dsm_output_expr_t   * output_p  = (dsm_output_expr_t*)  output_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (output_p == NULL) || (output_p->signature != DSM_SIG_OUTPUT_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
	
    partial_h = (DsmPartialExpH )adt_list_get_indexed_item(output_p->inv_partial_expr, index);

	if ( partial_h == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return NULL;
	}
	    
	return partial_h;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_partial_expr_count
 
 DESCRIPTION:   Gets the number of inverted partial expressions 
 
 PARAMETERS:    output_h        IN      Handle to the output expression
                                        the output expression.
                count           OUT     The number of partial expressions

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_expr_get_partial_expr_count (DsmOutputExpH output_h, uint32_t * count)
{
	dsm_output_expr_t   * output_p  = (dsm_output_expr_t*)  output_h;
	IteratorPtr           iterator_p;
    uint32_t t_count = 0;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (output_p == NULL) || (output_p->signature != DSM_SIG_OUTPUT_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
    t_count = 0;
    iterator_p = adt_list_get_iterator (output_p->partial_expr);

	if ( iterator_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	    
    adt_iterator_first(iterator_p);
    while(adt_iterator_is_in_list(iterator_p))
    {
        t_count++;
        adt_iterator_advance(iterator_p);
    }
    *count = t_count;
            
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_expr_get_partial_expr
 
 DESCRIPTION:   Gets the number of inverted partial expressions 
 
 PARAMETERS:    output_h        IN      Handle to the output expression
                                        the output expression.
                index           IN      The index of partial expr
                partial_h       OUT     Partial expression 

 RETURNS:       0 if the HLB output was added otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
DsmPartialExpH
dagdsm_expr_get_partial_expr(DsmOutputExpH output_h, uint32_t index)
{
    DsmPartialExpH partial_h = NULL;
	dsm_output_expr_t   * output_p  = (dsm_output_expr_t*)  output_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (output_p == NULL) || (output_p->signature != DSM_SIG_OUTPUT_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
    partial_h = (DsmPartialExpH )adt_list_get_indexed_item(output_p->partial_expr, index);

	if ( partial_h == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return NULL;
	}
	    
	return partial_h;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_compute_output_expr_value
 
 DESCRIPTION:   Recevies a set of parameters and determines if the
                output expression will hit.
 
 PARAMETERS:    expr_h          IN      Handle to the output expression
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
dagdsm_compute_output_expr_value (DsmOutputExpH output_h, uint32_t filters, uint8_t iface, uint32_t hlb0, uint32_t hlb1)
{
	dsm_output_expr_t   * output_p = (dsm_output_expr_t*)  output_h;
	DsmPartialExpH        expr_h;
	IteratorPtr           iterator_p;
	int                   res;
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (output_p == NULL) || (output_p->signature != DSM_SIG_OUTPUT_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* check for the special case where no partial expressions are set */
	if ( adt_list_is_empty(output_p->partial_expr) && adt_list_is_empty(output_p->inv_partial_expr) )
		return 0;
	
	
	/* because all the output expressions are ANDed together, we only need to
	 * find one that doesn't match to return a miss */
	
	/* check the non-inverted partial expressions first */
	iterator_p = adt_list_get_iterator (output_p->partial_expr);
	if ( iterator_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	
	adt_iterator_first(iterator_p);
	while ( adt_iterator_is_in_list(iterator_p) )
	{
		expr_h = (DsmPartialExpH) adt_iterator_retrieve (iterator_p);
		assert (expr_h != NULL);
		
		res = dagdsm_compute_partial_expr_value(expr_h, filters, iface, hlb0, hlb1);
		assert(res != -1);
		
		if ( res == 0 )
		{
			adt_iterator_dispose (iterator_p);
			return 0;
		}
		
		adt_iterator_advance (iterator_p);
	}
	adt_iterator_dispose (iterator_p);
	
	
	/* check the inverted partial expressions */
	iterator_p = adt_list_get_iterator (output_p->inv_partial_expr);
	if ( iterator_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	
	adt_iterator_first(iterator_p);
	while ( adt_iterator_is_in_list(iterator_p) )
	{
		expr_h = (DsmPartialExpH) adt_iterator_retrieve (iterator_p);
		assert (expr_h != NULL);
		
		res = dagdsm_compute_partial_expr_value(expr_h, filters, iface, hlb0, hlb1);
		assert(res != -1);
		
		if ( res == 1 )
		{
			adt_iterator_dispose (iterator_p);
			return 0;
		}
		
		adt_iterator_advance (iterator_p);
	}
	adt_iterator_dispose (iterator_p);
	
	
	return 1;
}

int
dagdsm_compute_output_expr_value_z (DsmOutputExpH output_h, uint32_t filters, uint8_t iface, uint32_t hlb_hash_value)
{
	dsm_output_expr_t   * output_p = (dsm_output_expr_t*)  output_h;
	DsmPartialExpH        expr_h;
	IteratorPtr           iterator_p;
	int                   res;
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (output_p == NULL) || (output_p->signature != DSM_SIG_OUTPUT_EXPR) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* check for the special case where no partial expressions are set */
	if ( adt_list_is_empty(output_p->partial_expr) && adt_list_is_empty(output_p->inv_partial_expr) )
		return 0;
	
	
	/* because all the output expressions are ANDed together, we only need to
	 * find one that doesn't match to return a miss */
	
	/* check the non-inverted partial expressions first */
	iterator_p = adt_list_get_iterator (output_p->partial_expr);
	if ( iterator_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	
	adt_iterator_first(iterator_p);
	while ( adt_iterator_is_in_list(iterator_p) )
	{
		expr_h = (DsmPartialExpH) adt_iterator_retrieve (iterator_p);
		assert (expr_h != NULL);
		
		res = dagdsm_compute_partial_expr_value_z(expr_h, filters, iface, hlb_hash_value);
		assert(res != -1);
		
		if ( res == 0 )
		{
			adt_iterator_dispose (iterator_p);
			return 0;
		}
		
		adt_iterator_advance (iterator_p);
	}
	adt_iterator_dispose (iterator_p);
	
	
	/* check the inverted partial expressions */
	iterator_p = adt_list_get_iterator (output_p->inv_partial_expr);
	if ( iterator_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	
	adt_iterator_first(iterator_p);
	while ( adt_iterator_is_in_list(iterator_p) )
	{
		expr_h = (DsmPartialExpH) adt_iterator_retrieve (iterator_p);
		assert (expr_h != NULL);
		
		res = dagdsm_compute_partial_expr_value_z(expr_h, filters, iface, hlb_hash_value);
		assert(res != -1);
		
		if ( res == 1 )
		{
			adt_iterator_dispose (iterator_p);
			return 0;
		}
		
		adt_iterator_advance (iterator_p);
	}
	adt_iterator_dispose (iterator_p);
	
	
	return 1;
}

