/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_counters.c,v 1.2 2006/03/07 09:18:49 ben Exp $
*
**********************************************************************/

/**********************************************************************
* 
* DESCRIPTION:  Source code for the "read counters" functionality of the
*               DSM library.
*
* 
*
**********************************************************************/	

/* System headers */
#include "dag_platform.h"

/* Endace headers */
#include "dagapi.h"
#include "dagcrc.h"
#include "dagutil.h"

/* DSM headers */
#include "dagdsm.h"
#include "dsm_types.h"






/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_read_counter
 
 DESCRIPTION:   Reads the 32-bit counter at a specified counter address.
 
 PARAMETERS:    dagfd           IN      
                counter         IN      

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
uint32_t
dsm_read_counter (volatile uint8_t * base_reg, uint16_t addr)
{
	uint32_t           cmd;
	uint32_t           value_h;
	uint32_t           value_l;


	/* trim to a four bit number */
	addr &= 0xF;


	/* write the address to read from */
	cmd = ((uint32_t)addr << 17);
	dag_writel (cmd, (base_reg + DSM_REG_COUNTER_OFFSET));
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
	value_l = dag_readl (base_reg + DSM_REG_COUNTER_OFFSET);
	value_l &= 0xFFFF;
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));

	cmd = ((uint32_t)addr << 17) | BIT16;
	dag_writel (cmd, (base_reg + DSM_REG_COUNTER_OFFSET));
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));
	value_h = dag_readl (base_reg + DSM_REG_COUNTER_OFFSET);
	value_h &= 0xFFFF;
	dsm_drb_delay ((volatile uint32_t *)(base_reg + DSM_REG_CSR_OFFSET));



	return (value_h << 16) | (value_l);
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_latch_and_clear_counters
 
 DESCRIPTION:   Latches the counters and clears the value
 
 PARAMETERS:    config_h        IN      
                 

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_latch_and_clear_counters (DsmConfigH config_h)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;
	uint32_t       csr_value;


	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	

	csr_value = dag_readl (config_p->base_reg + DSM_REG_CSR_OFFSET);
	dsm_drb_delay ((volatile uint32_t *)(config_p->base_reg + DSM_REG_CSR_OFFSET));

	csr_value |= BIT29;

	dag_writel (csr_value, (config_p->base_reg + DSM_REG_CSR_OFFSET));
	dsm_drb_delay ((volatile uint32_t *)(config_p->base_reg + DSM_REG_CSR_OFFSET));

	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_read_filter_counter
 
 DESCRIPTION:   Reads the specified filter hit counter.
 
 PARAMETERS:    config_h         IN      
                filter_h         IN    
                value_p          IN

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_read_filter_counter (DsmConfigH config_h, DsmFilterH filter_h, uint32_t *value_p)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;
	dsm_filter_t * filter_p = (dsm_filter_t*)filter_h;
	uint16_t       counter;


	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	
	if ( value_p == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	




	/* get the actual filter number */
	switch (filter_p->actual_filter_num)
	{
	case 0:     counter = DSM_CNT_FILTER0;      break;
	case 1:     counter = DSM_CNT_FILTER1;      break;
	case 2:     counter = DSM_CNT_FILTER2;      break;
	case 3:     counter = DSM_CNT_FILTER3;      break;
	case 4:     counter = DSM_CNT_FILTER4;      break;
	case 5:     counter = DSM_CNT_FILTER5;      break;
	case 6:     counter = DSM_CNT_FILTER6;      break;
	case 7:     counter = DSM_CNT_FILTER7;      break;
	default:
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	
	/* read the counter value */
	*value_p = dsm_read_counter (config_p->base_reg, counter);
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_read_hlb_counter
 
 DESCRIPTION:   Reads the HLB steering algorithm counter.
 
 PARAMETERS:    config_h         IN      
                filter_h         IN    
                value_p          IN

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_read_hlb_counter (DsmConfigH config_h, uint32_t *hlb0_p, uint32_t *hlb1_p)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	

	/* read the values */
	if ( hlb0_p != NULL )
		*hlb0_p = dsm_read_counter (config_p->base_reg, DSM_CNT_HLB0);
	
	if ( hlb1_p != NULL )
		*hlb1_p = dsm_read_counter (config_p->base_reg, DSM_CNT_HLB1);

	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_read_drop_counter
 
 DESCRIPTION:   Reads the global drop counter.
 
 PARAMETERS:    config_h         IN      
                filter_h         IN    
                value_p          IN

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_read_drop_counter (DsmConfigH config_h, uint32_t *value_p)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if (value_p == NULL)
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	/* read the values */
	*value_p = dsm_read_counter (config_p->base_reg, DSM_CNT_DROP);
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_read_stream_counter
 
 DESCRIPTION:   Reads the stream accept counter.
 
 PARAMETERS:    config_h         IN      
                filter_h         IN    
                value_p          IN

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_read_stream_counter (DsmConfigH config_h, uint32_t stream, uint32_t *value_p)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;

	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	
	if (value_p == NULL)
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	/* convert the receive stream number (even numbers) to a standard index */
	stream >>= 1;

	/* read the values */
	if ( value_p != NULL )
		*value_p = dsm_read_counter (config_p->base_reg, (uint16_t)(DSM_CNT_STREAM_BASE + stream));
	
	return 0;
}
