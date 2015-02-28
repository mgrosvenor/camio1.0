/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_config.c,v 1.19 2008/04/04 05:17:06 jomi Exp $
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
#include "dagapi.h"
#include "dagpci.h"

/* DSM headers */
#include "dagdsm.h"
#include "dsm_types.h"




/* Uncomment the following line only if debugging, it allows you to open 
 * configuration on any card. However you shouldn't try to download the
 * configuration.
 */
//#define CARD_INSENSITIVE




/*--------------------------------------------------------------------
 
 FUNCTION:      prv_dispose_partial_expr
 
 DESCRIPTION:   Disposer routine for the partial expressions stored in
                a configuration object (dsm_config_t structure). This
                is NOT the disposer for the partial expression lists 
                in a output expression object.
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
prv_dispose_partial_expr (AdtPtr data)
{
	free (data);
}




/*--------------------------------------------------------------------
 
 FUNCTION:      
 
 DESCRIPTION:   
                
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
DsmConfigH
dagdsm_create_configuration(int dagfd)
{
	dsm_config_t     * config_p;
	daginf_t         * daginf;
	uint32_t           if_count = 2;
	layer2_type_t      if_type  = kEthernet;
	uint32_t           i;
	uint32_t		   bit_number,bit_max;
	uint32_t		   temp_value;
	int                res;
#if 0
	double             bits;
	double             int_part;
	double             fract_part;
#endif
	volatile uint8_t * base_reg;
	
	dagdsm_set_last_error (0);


	/* get the card info and confirm it is a supported card */	
	daginf = dag_info(dagfd);
	if (daginf == NULL)
	{
		dagdsm_set_last_error (EBADF);
		return NULL;
	}
	
    if ( -1 == dagdsm_is_dsm_supported( dagfd) )
    {
        dagdsm_set_last_error (ENOENT);
		return NULL;
    }

	
#if !defined(CARD_INSENSITIVE)

	/* get the number of ports on the card */
	if ( (if_count = dagdsm_get_port_count(dagfd)) == -1 )
		return NULL;		
	
	
	/* get the type of the card */
	if ( (res = dagdsm_is_card_ethernet(dagfd)) == -1 )
		return NULL;
	if_type = (res == 1) ? kEthernet : kPoS;
	
	
	/* get the enumeration table and sanity check that the DSM is actually present */
	base_reg = dsm_get_base_reg (dagfd);
	if ( base_reg == NULL )
		return NULL;

#endif
	
	
	/* allocate the memory for the configuration */
	config_p = malloc (sizeof(dsm_config_t));
	if ( config_p == NULL )
	{
		dagdsm_set_last_error(ENOMEM);
		return NULL;
	}
	
	memset (config_p, 0, sizeof(dsm_config_t));
	
	
	
	/* populate the configuration with default values */
	config_p->signature = DSM_SIG_CONFIG;
	
	config_p->dagfd = dagfd;
	config_p->base_reg = base_reg;
	
	config_p->interfaces = if_count;
	
	config_p->device_code = daginf->device_code;
	
	config_p->rx_streams = dagdsm_get_filter_stream_count(dagfd);
#if 0
	//config_p->bits_per_entry = ((int)log2(config_p->rx_streams)) + 1;
	bits = (log(config_p->rx_streams) / log(2));
	fract_part = modf (bits, &int_part);
	if ( fract_part > 0 )  int_part += 1;
	config_p->bits_per_entry = (int)int_part + 1;
#else
	
	temp_value = 0x80000000;
	bit_max = config_p->rx_streams-1;
	for(bit_number=32;bit_number>=1;bit_number--)
	{
		if(bit_max&temp_value)
			break;
		temp_value = temp_value>>1;
	}
	bit_number++;/*the drop bit*/
	temp_value=0x20;
	for(i=6;i>=1;i--)
	{
		if(bit_number&temp_value)
		{
			if(bit_number!=temp_value)
				bit_number = temp_value<<1;
			break;
		}
		temp_value = temp_value>>1;
	}
	config_p->bits_per_entry = bit_number;
#endif	
	
	config_p->swap_filter = (FILTER_COUNT - 1);
	for (i=0; i<FILTER_COUNT; i++)
	{
		config_p->filters[i].signature   = DSM_SIG_FILTER;
		config_p->filters[i].layer2_type = if_type;
		config_p->filters[i].layer3_type = kIPv4;
		config_p->filters[i].enabled     = 0;
		config_p->filters[i].raw_mode    = 0;
		config_p->filters[i].term_depth  = LAST_FILTER_ELEMENT;
		config_p->filters[i].actual_filter_num = i;
	}

	config_p->partial_expr = adt_list_init(kListInterfaceIterator, kListRepresentationDynamic, prv_dispose_partial_expr);
	if ( config_p->partial_expr == NULL )
	{
		dagdsm_set_last_error(ENOMEM);
		free(config_p);
		return NULL;
	}

	for (i=0; i<config_p->rx_streams; i++)
	{
		config_p->output_expr[i].signature = DSM_SIG_OUTPUT_EXPR;

		config_p->output_expr[i].partial_expr     = adt_list_init(kListInterfaceIterator, kListRepresentationArray, adt_null_disposer);
		config_p->output_expr[i].inv_partial_expr = adt_list_init(kListInterfaceIterator, kListRepresentationArray, adt_null_disposer);
	}
	
	
	config_p->dsm_version = DSM_VERSION_0;
	switch (daginf->device_code)
	{
		case PCI_DEVICE_ID_DAG8_20Z:
		case PCI_DEVICE_ID_DAG5_0Z:
		case PCI_DEVICE_ID_DAG4_52Z:
		case PCI_DEVICE_ID_DAG4_52Z8:
			config_p->dsm_version = DSM_VERSION_1;
			break;
	}

	return (DsmConfigH) config_p;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_destroy_configuration
 
 DESCRIPTION:   
                
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_destroy_configuration(DsmConfigH config_h)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;
	uint32_t       i;
	

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* we need to free all the partial expressions in the lists */
	if ( config_p->partial_expr )
		adt_list_dispose (config_p->partial_expr);
	
	for (i=0; i<config_p->rx_streams; i++)
	{
		adt_list_dispose (config_p->output_expr[i].partial_expr);
		adt_list_dispose (config_p->output_expr[i].inv_partial_expr);
	}
	
	
	
	/* free the memory assoiated with configuration */
	free (config_p);
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_filter
 
 DESCRIPTION:   Gets a handle to a filter, the handle can then be modified
                by the caller using the dagdsm_filter_??? functions.
 
 PARAMETERS:    config_h        IN      Handle to the configuration
                filter          IN      The number of the filter to get,
                                        should be in the range 0 to
                                        FILTER_COUNT - 1.

 RETURNS:       returns a handle to the filter, or if there was an error,
                NULL is returned. This function updates the last error code
                using dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
DsmFilterH
dagdsm_get_filter(DsmConfigH config_h, uint32_t filter)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;
	

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	if ( filter >= (FILTER_COUNT - 1) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
	
	return (DsmFilterH) (&config_p->filters[filter]);
}






/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_is_ethernet
 
 DESCRIPTION:   Checks if the configuration is for ethernet layer 2
                type.
 
 PARAMETERS:    config_h         IN      Handle to the configuration

 RETURNS:       Returns 1 if the configuration is for an ethernet, 0
                if not an ethernet configuration and -1 if there was
                an error. This function updates the last error code
                using dagdsm_set_last_error().
 
 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_is_ethernet(DsmConfigH config_h)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( (config_p->filters[0].layer2_type == kEthernet) || (config_p->filters[0].layer2_type == kEthernetVlan) )
		return 1;
	else
		return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_is_sonet
 
 DESCRIPTION:   Checks if the configuration is for sonet layer 2
                type.
 
 PARAMETERS:    config_h         IN      Handle to the configuration

 RETURNS:       Returns 1 if the configuration is for an sonet, 0
                if not an sonet configuration and -1 if there was
                an error. This function updates the last error code
                using dagdsm_set_last_error().
 
 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_is_sonet(DsmConfigH config_h)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( config_p->filters[0].layer2_type == kPoS )
		return 1;
	else
		return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_swap_filter
 
 DESCRIPTION:   Returns a handle to the swap filter, the swap filter
                can then be editied before swapping with an existing
                filter.
 
 PARAMETERS:    config_h         IN      Handle to the configuration

 RETURNS:       A handle to the swap filter, may return NULL if an error
                occurred. This function updates the last error code
                using dagdsm_set_last_error().
 
 HISTORY:       

---------------------------------------------------------------------*/
DsmFilterH
dagdsm_get_swap_filter(DsmConfigH config_h)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;


	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}


	return (DsmFilterH)	(&config_p->filters[DSM_SWAP_FILTER_NUM]);
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_clear_expressions
 
 DESCRIPTION:   Removes all partial expressions and resets the output
                expressions to their default state.
 
 PARAMETERS:    config_h         IN      Handle to the configuration

 RETURNS:       0 if the clear was successiful, otherwise -1. This function
                updates the last error code using dagdsm_set_last_error().
 
 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_clear_expressions(DsmConfigH config_h)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;
	uint32_t       i;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* dispose of all the regular expression pointers stored in the output expressions */
	for (i=0; i<config_p->rx_streams; i++)
	{
		adt_list_make_empty (config_p->output_expr[i].partial_expr);
		adt_list_make_empty (config_p->output_expr[i].inv_partial_expr);
	}
	
	
	/* dispose of all the partial expressions stored in the configuration */
	adt_list_make_empty (config_p->partial_expr);
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_create_partial_expression
 
 DESCRIPTION:   Creates a new partial expression and returns a handle
                to it.
 
 PARAMETERS:    config_h         IN      Handle to the configuration

 RETURNS:       Handle to the new partial expression or if there was an
                error NULL is returned. This function updates the last
                error code using dagdsm_set_last_error().
 
 HISTORY:       

---------------------------------------------------------------------*/
DsmPartialExpH
dagdsm_create_partial_expr(DsmConfigH config_h)
{
	dsm_config_t       *config_p = (dsm_config_t*)config_h;
	dsm_partial_expr_t *expr_p;
	IteratorPtr         iterator_p;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
	
	/* allocate the memory for the partial expression */
	expr_p = (dsm_partial_expr_t*) malloc( sizeof(dsm_partial_expr_t) );
	if ( expr_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return NULL;
	}
	
	memset (expr_p, 0, sizeof(dsm_partial_expr_t));
	
	expr_p->signature = DSM_SIG_PARTIAL_EXPR;
	expr_p->interfaces = config_p->interfaces;
	
	
	/* add to the list of partial expressions */
	iterator_p = adt_list_get_iterator (config_p->partial_expr);
	if ( iterator_p == NULL )
	{
		free (expr_p);
		dagdsm_set_last_error (ENOMEM);
		return NULL;
	}
	
	adt_iterator_last (iterator_p);
	adt_iterator_add (iterator_p, expr_p);
	
	adt_iterator_dispose (iterator_p);
	return (DsmPartialExpH)expr_p;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_partial_expr_count
 
 DESCRIPTION:   Returns the number of partial expressions that have
                been created for this configuration.
 
 PARAMETERS:    config_h         IN      Handle to the configuration
                

 RETURNS:       Positive number that indicates how many partial expressions
                exist, -1 is returned if there was an error.
 
 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_get_partial_expr_count(DsmConfigH config_h)
{
	dsm_config_t       *config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( config_p->partial_expr == NULL )
		return 0;
		
	return (int)adt_list_items (config_p->partial_expr);
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_partial_expr
 
 DESCRIPTION:   Returns the handle to a partial expression with the given
                index.
 
 PARAMETERS:    config_h         IN      Handle to the configuration
                index            IN      Index of the partial expression
                                         to get.
                

 RETURNS:       Returns a handle to the partial expression or NULL if
                there was an error.
 
 HISTORY:       

---------------------------------------------------------------------*/
DsmPartialExpH
dagdsm_get_partial_expr(DsmConfigH config_h, uint32_t index)
{
	dsm_config_t       *config_p = (dsm_config_t*)config_h;
	DsmPartialExpH      partial_h;


	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	if ( index >= adt_list_items(config_p->partial_expr) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
	

	/* ADT list indices start at 1 and go up, where as the index argument supplied
	   by the user should start and 0 and go up */
	partial_h = (DsmPartialExpH) adt_list_get_indexed_item (config_p->partial_expr, index + 1);

	
	/* return the result */	
	if ( partial_h == NULL )
		dagdsm_set_last_error (EINVAL);
	
	
	return partial_h;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_output_expr_count
 
 DESCRIPTION:   Returns the number of possible stream output expressions
                available in the virtual configuration. This will return
                the exact same value as dagdsm_get_filter_stream_count
                for a given card.
 
 PARAMETERS:    config_h         IN      Handle to the configuration

 RETURNS:       Positive number that indicates how many output expressions
                exist, -1 is returned if there was an error.
 
 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_get_output_expr_count(DsmConfigH config_h)
{
	dsm_config_t       *config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	return config_p->rx_streams;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_get_output_expr
 
 DESCRIPTION:   Returns the handle to an output expression associated
                with a particular receive stream. By convention receive
                stream are given even numbers 0,2,4, ... etc.
 
 PARAMETERS:    config_h         IN      Handle to the configuration
                rx_stream        IN      The stream to get the output
                                         expression from.

 RETURNS:       Handle to the output expression or if there was an
                error NULL is returned. This function updates the last
                error code using dagdsm_set_last_error().
 
 HISTORY:       

---------------------------------------------------------------------*/
DsmOutputExpH
dagdsm_get_output_expr(DsmConfigH config_h, uint32_t rx_stream)
{
	dsm_config_t       *config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
	/* make sure the stream is a even number */
	if ( (rx_stream & 1) == 1 )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
	
	/* divide by 2 to get the stream index */
	rx_stream >>= 1;
	
	if ( rx_stream >= config_p->rx_streams )
	{
		dagdsm_set_last_error (EINVAL);
		return NULL;
	}
	
	
	/* return a handle to the output expression */
	return (DsmOutputExpH)(&config_p->output_expr[rx_stream]);
}
