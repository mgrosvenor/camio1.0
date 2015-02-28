/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_filter.c,v 1.6 2006/09/19 07:14:29 vladimir Exp $
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
 
 FUNCTION:      dagdsm_filter_enable
 
 DESCRIPTION:   Enables or Disables a particular filter. This function
                only sets the internal enabled state within the library,
                this doesn't update the hardware until dagdsm_load_configuration
                is called.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                enable          IN      A non-zero value enables the fitler,
                                        a zero value disables the filter.

 RETURNS:       0 if the filter was enabled/disabled, otherwise -1. 
                This function updates the last error code using
                dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_enable(DsmFilterH filter_h, uint32_t enable)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* find the filter with the virtual filter number */
	if ( enable == 0 )
		filter_p->enabled = 0;
	else
		filter_p->enabled = 1;
		


	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_is_enabled
 
 DESCRIPTION:   Indicates whether the filter is enabled or not. This function
                only sets the internal enabled state within the library,
                this doesn't update the hardware until dagdsm_load_configuration
                is called.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                enabled         IN      A non-zero value indicating the status of the fitler,

 RETURNS:       0 if the filter was enabled/disabled, otherwise -1. 
                This function updates the last error code using
                dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_is_enabled(DsmFilterH filter_h, uint32_t * enabled)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
    *enabled = filter_p->enabled;

	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_early_term_depth
 
 DESCRIPTION:   Sets the byte number to terminate the filter comparsion on.
                The early termination byte tells the hardware what byte
                of the filter to terminate on, packets smaller than the
                early termination size automatically miss the filter. As with
                dagdsm_enable_filter() this function doesn't update the
                hardware until dagdsm_load_configuration() is called.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                depth           IN      The element number to terminate on,
                                        should be in the range 0 to 7.

 RETURNS:       0 if the filter was modified, otherwise -1. 
                This function updates the last error code using
                dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_early_term_depth (DsmFilterH filter_h, uint32_t element)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* set the element to early terminate on */
	if ( element > LAST_FILTER_ELEMENT )
		element = LAST_FILTER_ELEMENT;

	filter_p->term_depth = element;

	
	return 0;
}
/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_early_term_depth
 
 DESCRIPTION:   Returns the current value for the early term depth.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                element         OUT     The element number currently set for early termination,
                                        If no early termination then this is set to DSM_NO_EARLYTERM.

 RETURNS:       0 if the filter was modified, otherwise -1. 
                This function updates the last error code using
                dagdsm_set_last_error().

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_early_term_depth (DsmFilterH filter_h, uint32_t * element)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
    *element = filter_p->term_depth;

	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_clear
 
 DESCRIPTION:   Clears the filter to accept all packets. Resets the mask
                and value pair to all zero's. Also clears the layer3 and
				4 protocols.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
 
 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_clear (DsmFilterH filter_h)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	filter_p->raw_mode    = 0;
	filter_p->layer3_type = kIPv4;
	
	filter_p->term_depth  = LAST_FILTER_ELEMENT;

	
	/* disable VLAN if set */
	if ( filter_p->layer2_type == kEthernetVlan )
		filter_p->layer2_type = kEthernet;
	
	
	/* protocol settings */
	memset (&(filter_p->layer2), 0, sizeof(filter_p->layer2));
	memset (&(filter_p->layer3), 0, sizeof(filter_p->layer3));
	memset (&(filter_p->layer4), 0, sizeof(filter_p->layer4));
	filter_p->layer4_type = 0;
	
	
	/* raw settings */
	memset (&(filter_p->raw), 0, sizeof(filter_p->raw));
	

	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_raw_mode
 
 DESCRIPTION:   Enables the raw filter mode, in this mode the raw filter
                bytes are used for the filter rather than the settings
                set by the user.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least MATCH_DEPTH
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least MATCH_DEPTH
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_raw_mode (DsmFilterH filter_h, uint32_t enable)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* enable the raw mode if asked to */
	if ( enable != 0 )
		filter_p->raw_mode = 1;
	else
		filter_p->raw_mode = 0;
	
	return 0;	
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_raw_mode
 
 DESCRIPTION:   Returns the status of the raw mode
 
 PARAMETERS:    filter_h        IN      Handle to the filter

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_raw_mode (DsmFilterH filter_h)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}


	return (int)filter_p->raw_mode;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_raw_filter
 
 DESCRIPTION:   Sets the raw value and mask bytes used for the filter.
                This clears all previous settings stored for the filter,
                i.e. what protocol type and port settings.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least MATCH_DEPTH
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least MATCH_DEPTH
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_raw_filter (DsmFilterH filter_h, const uint8_t * value, const uint8_t * mask, uint32_t size)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->raw_mode != 1 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	
	/* initial clear the raw values */
	memset (filter_p->raw.value, 0, MATCH_DEPTH);
	memset (filter_p->raw.mask,  0, MATCH_DEPTH);
	
	
	/* copy over the filter */
	size = (size > MATCH_DEPTH) ? MATCH_DEPTH : size;
	memcpy (filter_p->raw.value, value, size);
	memcpy (filter_p->raw.mask,  mask,  size);
	
	
	return 0;
}
/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_raw_filter
 
 DESCRIPTION:   Returns the raw value for filter 
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           OUT     Pointer to a value array, this
                                        array should be at least MATCH_DEPTH
                                        bytes big.
                mask            OUT     Pointer to a mask array, this
                                        array should be at least MATCH_DEPTH
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_raw_filter (DsmFilterH filter_h, uint8_t * value, uint8_t * mask, uint32_t size)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->raw_mode != 1 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	
	/* initial clear the raw values */
	memset (value, 0, MATCH_DEPTH);
	memset (mask,  0, MATCH_DEPTH);
	
	
	/* copy over the filter */
	size = (size > MATCH_DEPTH) ? MATCH_DEPTH : size;
	memcpy (value, filter_p->raw.value, size);
	memcpy (mask, filter_p->raw.mask, size);
	
	
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_layer3_type
 
 DESCRIPTION:   Sets the layer three type, currently the only value allowed
                is kIPv4.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_layer3_type (DsmFilterH filter_h, layer3_type_t type)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	switch (type)
	{
		case kIPv4:
			break;
		
		default:
			dagdsm_set_last_error (EINVAL);
			return -1;
	}
			
	
	
	/* clear the old settings if the type has changed */
	if ( type != filter_p->layer3_type )
	{
		/* protocol settings */
		memset (&(filter_p->layer3), 0, sizeof(filter_p->layer3));
		memset (&(filter_p->layer4), 0, sizeof(filter_p->layer4));
		filter_p->layer4_type = 0;
				
		/* raw settings */
		memset (&(filter_p->raw), 0, sizeof(filter_p->raw));	
		
		/* set the new type */
		filter_p->layer3_type = type;
	}
	

	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_layer3_type
 
 DESCRIPTION:   Gets the layer three type, currently the only value allowed
                is kIPv4.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                type            OUT     Pointer to a layer type

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_layer3_type (DsmFilterH filter_h, layer3_type_t * type)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	*type = filter_p->layer3_type;
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_ip_protocol
 
 DESCRIPTION:   Sets the IP protocol field for the filter. This doesn't
                clear any port filters 
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                protocol        IN      Protocol to filter on

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_ip_protocol (DsmFilterH filter_h, uint8_t protocol)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* check if the protocol has changed and clear all the old values if it has */
	filter_p->layer3.ipv4.protocol = protocol;
	
	switch (protocol)
	{
		case IPPROTO_ICMP:   filter_p->layer4_type = kICMP;     break;
		case IPPROTO_UDP:    filter_p->layer4_type = kUDP;      break;
		case IPPROTO_TCP:    filter_p->layer4_type = kTCP;      break;
	}
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_ip_protocol
 
 DESCRIPTION:   Gets the IP protocol field for the filter. 
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                protocol        OUT     Protocol field for filter

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_ip_protocol (DsmFilterH filter_h, uint8_t * protocol)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* check if the protocol has changed and clear all the old values if it has */
    *protocol = filter_p->layer3.ipv4.protocol;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_icmp_code
 
 DESCRIPTION:   Sets the icmp type code, this has the effect of filtering
                on the ICMP type but also update the layer 4 protocol field.
                This will invalidate any port filters that are installed.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                code            IN      Code value to filter on
                mask            IN      Mask used for the code.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_icmp_code (DsmFilterH filter_h, uint8_t code, uint8_t mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer3_type != kIPv4) || (filter_p->layer4_type != kICMP) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	filter_p->layer4.icmp.icmp_code.code = code;
	filter_p->layer4.icmp.icmp_code.mask = mask;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_icmp_code
 
 DESCRIPTION:   Gets the icmp type code.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                code            OUT     Code value to filter on
                mask            OUT     Mask used for the code.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_icmp_code (DsmFilterH filter_h, uint8_t * code, uint8_t * icmp_mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer3_type != kIPv4) || (filter_p->layer4_type != kICMP) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	*code = filter_p->layer4.icmp.icmp_code.code;
	*icmp_mask = filter_p->layer4.icmp.icmp_code.mask;
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_icmp_type
 
 DESCRIPTION:   Sets the icmp type, this has the effect of filtering
                on the ICMP type but also update the layer 4 protocol field.
                This will invalidate any port filters that are installed.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                type            IN      Type value to filter on
                mask            IN      Mask used for the type.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_icmp_type (DsmFilterH filter_h, uint8_t type, uint8_t mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer3_type != kIPv4) || (filter_p->layer4_type != kICMP) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	filter_p->layer4.icmp.icmp_type.type = type;
	filter_p->layer4.icmp.icmp_type.mask = mask;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_icmp_type
 
 DESCRIPTION:   Gets the icmp type.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                type            OUT     Type value to filter on
                mask            OUT     Mask used for the type.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_icmp_type (DsmFilterH filter_h, uint8_t * type, uint8_t * icmp_mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer3_type != kIPv4) || (filter_p->layer4_type != kICMP) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	*type = filter_p->layer4.icmp.icmp_type.type;
	*icmp_mask = filter_p->layer4.icmp.icmp_type.mask;
	
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_dst_port
 
 DESCRIPTION:   Attempts to set the destination port filter, this function
                will fail if the protocol is not TCP or UDP. To set the
                protocol call dagdsm_filter_set_ip_protocol().
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                port            IN      Port value to filter on
                mask            IN      Mask used for the port

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_dst_port (DsmFilterH filter_h, uint16_t port, uint16_t mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	if (filter_p->layer3_type != kIPv4)
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	switch (filter_p->layer4_type)
	{
		case kTCP:
			filter_p->layer4.tcp.dst_port.port = port;
			filter_p->layer4.tcp.dst_port.mask = mask;
			break;
			
		case kUDP:
			filter_p->layer4.udp.dst_port.port = port;
			filter_p->layer4.udp.dst_port.mask = mask;
			break;
		
		default:
			dagdsm_set_last_error (EINVAL);
			return -1;
	}
	
	
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_dst_port
 
 DESCRIPTION:   Attempts to set the destination port filter, this function
                will fail if the protocol is not TCP or UDP. To set the
                protocol call dagdsm_filter_set_ip_protocol().
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                port            OUT     Port value to filter on
                mask            OUT     Mask used for the port

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_dst_port (DsmFilterH filter_h, uint16_t * port, uint16_t * mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	if (filter_p->layer3_type != kIPv4)
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	switch (filter_p->layer4_type)
	{
		case kTCP:
			*port = filter_p->layer4.tcp.dst_port.port;
			*mask = filter_p->layer4.tcp.dst_port.mask;
			break;
			
		case kUDP:
			*port = filter_p->layer4.udp.dst_port.port;
			*mask = filter_p->layer4.udp.dst_port.mask;
			break;
		
		default:
			dagdsm_set_last_error (EINVAL);
			return -1;
	}
	
	
	
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_src_port
 
 DESCRIPTION:   Attempts to set the source port filter, this function
                will fail if the protocol is not TCP or UDP. To set the
                protocol call dagdsm_filter_set_ip_protocol().
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                port            IN      Port value to filter on
                mask            IN      Mask used for the port

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_src_port (DsmFilterH filter_h, uint16_t port, uint16_t mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	switch (filter_p->layer4_type)
	{
		case kTCP:
			filter_p->layer4.tcp.src_port.port = port;
			filter_p->layer4.tcp.src_port.mask = mask;
			break;
			
		case kUDP:
			filter_p->layer4.udp.src_port.port = port;
			filter_p->layer4.udp.src_port.mask = mask;
			break;
		
		default:
			dagdsm_set_last_error (EINVAL);
			return -1;
	}
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_src_port
 
 DESCRIPTION:   Get the source port filter, this function
                will fail if the protocol is not TCP or UDP. To set the
                protocol call dagdsm_filter_set_ip_protocol().
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                port            OUT     Port value to filter on
                mask            OUT     Mask used for the port

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_src_port (DsmFilterH filter_h, uint16_t * port, uint16_t * mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	switch (filter_p->layer4_type)
	{
		case kTCP:
			*port = filter_p->layer4.tcp.src_port.port;
			*mask = filter_p->layer4.tcp.src_port.mask;
			break;
			
		case kUDP:
			*port = filter_p->layer4.udp.src_port.port;
			*mask = filter_p->layer4.udp.src_port.mask;
			break;
		
		default:
			dagdsm_set_last_error (EINVAL);
			return -1;
	}
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_tcp_flags
 
 DESCRIPTION:   Sets the TCP flags bitmap filter, the protocol must already
                be configured for TCP for this function to succeed.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                flags           IN      The TCP flags value to filter on,
                                        only the lower six bits are used.
                mask            IN      The mask to apply to the filter
                                        before checking the value, only
                                        the lower six bits are used.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_tcp_flags (DsmFilterH filter_h, uint8_t flags, uint8_t mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	if ( (filter_p->layer3_type != kIPv4) || (filter_p->layer4_type != kTCP) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	filter_p->layer4.tcp.tcp_flags.flags = (flags & 0x3F);
	filter_p->layer4.tcp.tcp_flags.mask  = (mask & 0x3F);
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_tcp_flags
 
 DESCRIPTION:   Sets the TCP flags bitmap filter, the protocol must already
                be configured for TCP for this function to succeed.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                flags           OUT     The TCP flags value to filter on,
                                        only the lower six bits are used.
                mask            OUT     The mask to apply to the filter
                                        before checking the value, only
                                        the lower six bits are used.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_tcp_flags (DsmFilterH filter_h, uint8_t * flags, uint8_t * tcp_mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	if ( (filter_p->layer3_type != kIPv4) || (filter_p->layer4_type != kTCP) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
    *flags = filter_p->layer4.tcp.tcp_flags.flags;
    *tcp_mask = filter_p->layer4.tcp.tcp_flags.mask;
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_ip_source
 
 DESCRIPTION:   Sets the IP source filter.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                src             IN      Pointer to a IP address to filter on.
                mask            IN      Pointer to an IP address that is
                                        used for the mask.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_ip_source (DsmFilterH filter_h, const struct in_addr * src, const struct in_addr * mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
#if !defined(_WIN32)	
	uint32_t       in_addr;
	uint32_t       in_mask;
#endif
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( src == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	/* copy over the address and mask */
#if defined(_WIN32)	
	filter_p->layer3.ipv4.src_addr.addr[0] = src->S_un.S_un_b.s_b1;
	filter_p->layer3.ipv4.src_addr.addr[1] = src->S_un.S_un_b.s_b2;
	filter_p->layer3.ipv4.src_addr.addr[2] = src->S_un.S_un_b.s_b3;
	filter_p->layer3.ipv4.src_addr.addr[3] = src->S_un.S_un_b.s_b4;

	filter_p->layer3.ipv4.src_addr.mask[0] = mask->S_un.S_un_b.s_b1;
	filter_p->layer3.ipv4.src_addr.mask[1] = mask->S_un.S_un_b.s_b2;
	filter_p->layer3.ipv4.src_addr.mask[2] = mask->S_un.S_un_b.s_b3;
	filter_p->layer3.ipv4.src_addr.mask[3] = mask->S_un.S_un_b.s_b4;
	
#else 
	in_addr = ntohl(src->s_addr);
	in_mask = ntohl(mask->s_addr);
	
	filter_p->layer3.ipv4.src_addr.addr[0] = (uint8_t) ((in_addr >> 24) & 0xFF);
	filter_p->layer3.ipv4.src_addr.addr[1] = (uint8_t) ((in_addr >> 16) & 0xFF);
	filter_p->layer3.ipv4.src_addr.addr[2] = (uint8_t) ((in_addr >>  8) & 0xFF);
	filter_p->layer3.ipv4.src_addr.addr[3] = (uint8_t) ((in_addr      ) & 0xFF);

	filter_p->layer3.ipv4.src_addr.mask[0] = (uint8_t) ((in_mask >> 24) & 0xFF);
	filter_p->layer3.ipv4.src_addr.mask[1] = (uint8_t) ((in_mask >> 16) & 0xFF);
	filter_p->layer3.ipv4.src_addr.mask[2] = (uint8_t) ((in_mask >>  8) & 0xFF);
	filter_p->layer3.ipv4.src_addr.mask[3] = (uint8_t) ((in_mask      ) & 0xFF);
	
#endif
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_ip_source
 
 DESCRIPTION:   Sets the IP source filter.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                src             OUT     Pointer to a IP address to filter on.
                mask            OUT     Pointer to an IP address that is
                                        used for the mask.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_ip_source (DsmFilterH filter_h, struct in_addr * src, struct in_addr * mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
#if !defined(_WIN32)	
	uint32_t       in_addr;
	uint32_t       in_mask;
#endif
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( src == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	/* copy over the address and mask */
#if defined(_WIN32)	
    src->S_un.S_un_b.s_b1 = filter_p->layer3.ipv4.src_addr.addr[0];
    src->S_un.S_un_b.s_b2 = filter_p->layer3.ipv4.src_addr.addr[1];
    src->S_un.S_un_b.s_b3 = filter_p->layer3.ipv4.src_addr.addr[2];
    src->S_un.S_un_b.s_b4 = filter_p->layer3.ipv4.src_addr.addr[3];

    mask->S_un.S_un_b.s_b1 = filter_p->layer3.ipv4.src_addr.mask[0];
    mask->S_un.S_un_b.s_b2 = filter_p->layer3.ipv4.src_addr.mask[1];
    mask->S_un.S_un_b.s_b3 = filter_p->layer3.ipv4.src_addr.mask[2];
    mask->S_un.S_un_b.s_b4 = filter_p->layer3.ipv4.src_addr.mask[3];

	
#else  
	in_addr = ntohl(src->s_addr);
	in_mask = ntohl(mask->s_addr);
	
    in_addr |= ((filter_p->layer3.ipv4.src_addr.addr[0] << 24) & 0xFF000000);
    in_addr |= ((filter_p->layer3.ipv4.src_addr.addr[1] << 16) & 0xFF0000);
    in_addr |= ((filter_p->layer3.ipv4.src_addr.addr[2] << 8) & 0xFF00);
    in_addr |= (filter_p->layer3.ipv4.src_addr.addr[3] & 0xFF);

    in_mask |= ((filter_p->layer3.ipv4.src_addr.mask[0] << 24) & 0xFF000000);
    in_mask |= ((filter_p->layer3.ipv4.src_addr.mask[1] << 16) & 0xFF0000);
    in_mask |= ((filter_p->layer3.ipv4.src_addr.mask[2] << 8) & 0xFF00);
    in_mask |= (filter_p->layer3.ipv4.src_addr.mask[3] & 0xFF);

    src->s_addr = htonl(in_addr);
    mask->s_addr = htonl(in_mask);
        
            
#endif
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_ip_dest
 
 DESCRIPTION:   Sets the IP destination filter.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                dst             IN      Pointer to a IP address to filter on.
                mask            IN      Pointer to an IP address that is
                                        used for the mask.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_ip_dest (DsmFilterH filter_h, const struct in_addr * dst, const struct in_addr * mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
#if !defined(_WIN32)	
	uint32_t       in_addr;
	uint32_t       in_mask;
#endif
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( dst == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* copy over the address and mask */
#if defined(_WIN32)	
	filter_p->layer3.ipv4.dst_addr.addr[0] = dst->S_un.S_un_b.s_b1;
	filter_p->layer3.ipv4.dst_addr.addr[1] = dst->S_un.S_un_b.s_b2;
	filter_p->layer3.ipv4.dst_addr.addr[2] = dst->S_un.S_un_b.s_b3;
	filter_p->layer3.ipv4.dst_addr.addr[3] = dst->S_un.S_un_b.s_b4;

	filter_p->layer3.ipv4.dst_addr.mask[0] = mask->S_un.S_un_b.s_b1;
	filter_p->layer3.ipv4.dst_addr.mask[1] = mask->S_un.S_un_b.s_b2;
	filter_p->layer3.ipv4.dst_addr.mask[2] = mask->S_un.S_un_b.s_b3;
	filter_p->layer3.ipv4.dst_addr.mask[3] = mask->S_un.S_un_b.s_b4;
	
#else 
	in_addr = ntohl(dst->s_addr);
	in_mask = ntohl(mask->s_addr);

	filter_p->layer3.ipv4.dst_addr.addr[0] = (uint8_t) ((in_addr >> 24) & 0xFF);
	filter_p->layer3.ipv4.dst_addr.addr[1] = (uint8_t) ((in_addr >> 16) & 0xFF);
	filter_p->layer3.ipv4.dst_addr.addr[2] = (uint8_t) ((in_addr >>  8) & 0xFF);
	filter_p->layer3.ipv4.dst_addr.addr[3] = (uint8_t) ((in_addr      ) & 0xFF);

	filter_p->layer3.ipv4.dst_addr.mask[0] = (uint8_t) ((in_mask >> 24) & 0xFF);
	filter_p->layer3.ipv4.dst_addr.mask[1] = (uint8_t) ((in_mask >> 16) & 0xFF);
	filter_p->layer3.ipv4.dst_addr.mask[2] = (uint8_t) ((in_mask >>  8) & 0xFF);
	filter_p->layer3.ipv4.dst_addr.mask[3] = (uint8_t) ((in_mask      ) & 0xFF);
	
#endif
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_ip_dest
 
 DESCRIPTION:   Get the IP destination filter.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                dst             OUT     Pointer to a IP address to filter on.
                mask            OUT     Pointer to an IP address that is
                                        used for the mask.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_ip_dest (DsmFilterH filter_h, struct in_addr * dst, struct in_addr * mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
#if !defined(_WIN32)	
	uint32_t       in_addr = 0;
	uint32_t       in_mask = 0;
#endif
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( dst == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* copy over the address and mask */
#if defined(_WIN32)	
    dst->S_un.S_un_b.s_b1 = filter_p->layer3.ipv4.dst_addr.addr[0];
    dst->S_un.S_un_b.s_b2 = filter_p->layer3.ipv4.dst_addr.addr[1];
    dst->S_un.S_un_b.s_b3 = filter_p->layer3.ipv4.dst_addr.addr[2];
    dst->S_un.S_un_b.s_b4 = filter_p->layer3.ipv4.dst_addr.addr[3];

    mask->S_un.S_un_b.s_b1 = filter_p->layer3.ipv4.dst_addr.mask[0];
    mask->S_un.S_un_b.s_b2 = filter_p->layer3.ipv4.dst_addr.mask[1];
    mask->S_un.S_un_b.s_b3 = filter_p->layer3.ipv4.dst_addr.mask[2];
    mask->S_un.S_un_b.s_b4 = filter_p->layer3.ipv4.dst_addr.mask[3];

	filter_p->layer3.ipv4.dst_addr.addr[0] = dst->S_un.S_un_b.s_b1;
	filter_p->layer3.ipv4.dst_addr.addr[1] = dst->S_un.S_un_b.s_b2;
	filter_p->layer3.ipv4.dst_addr.addr[2] = dst->S_un.S_un_b.s_b3;
	filter_p->layer3.ipv4.dst_addr.addr[3] = dst->S_un.S_un_b.s_b4;

	filter_p->layer3.ipv4.dst_addr.mask[0] = mask->S_un.S_un_b.s_b1;
	filter_p->layer3.ipv4.dst_addr.mask[1] = mask->S_un.S_un_b.s_b2;
	filter_p->layer3.ipv4.dst_addr.mask[2] = mask->S_un.S_un_b.s_b3;
	filter_p->layer3.ipv4.dst_addr.mask[3] = mask->S_un.S_un_b.s_b4;
	
#else

    in_addr |= ((filter_p->layer3.ipv4.dst_addr.addr[0] << 24) & 0xFF000000);
    in_addr |= ((filter_p->layer3.ipv4.dst_addr.addr[1] << 16) & 0xFF0000);
    in_addr |= ((filter_p->layer3.ipv4.dst_addr.addr[2] << 8) & 0xFF00);
    in_addr |= (filter_p->layer3.ipv4.dst_addr.addr[3] & 0xFF);

    in_mask |= ((filter_p->layer3.ipv4.dst_addr.mask[0] << 24) & 0xFF000000);
    in_mask |= ((filter_p->layer3.ipv4.dst_addr.mask[1] << 16) & 0xFF0000);
    in_mask |= ((filter_p->layer3.ipv4.dst_addr.mask[2] << 8) & 0xFF00);
    in_mask |= (filter_p->layer3.ipv4.dst_addr.mask[3] & 0xFF);

    dst->s_addr = htonl(in_addr);
    mask->s_addr = htonl(in_mask);

#endif
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_ip_fragment
 
 DESCRIPTION:   Sets the filter to either accept of discard IP fragments
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                fragment        IN      If a non zero value fragmented
                                        packets are filtered out. If zero
                                        the fragmentation state is ignored.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_ip_fragment (DsmFilterH filter_h, uint32_t enable)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}


	/* set the filter to drop fragments or don't care */	
	if ( enable == 0 )
		filter_p->layer3.ipv4.frag_filter = kFragDontCare;
	else
		filter_p->layer3.ipv4.frag_filter = kFragReject;
		
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_ip_fragment
 
 DESCRIPTION:   Sets the filter to either accept of discard IP fragments
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                enable          OUT     Indicates if ip fragment is enabled 

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_ip_fragment (DsmFilterH filter_h, uint32_t * enable)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}


	/* set the filter to drop fragments or don't care */	
    *enable = filter_p->layer3.ipv4.frag_filter;
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_ip_hdr_length
 
 DESCRIPTION:   Sets the IP header length of the .
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_ip_hdr_length (DsmFilterH filter_h, uint8_t ihl)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( ihl < 5 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		

	
	/* copy in the header length values */	
	filter_p->layer3.ipv4.ihl.ihl  = ihl & 0xF;
	filter_p->layer3.ipv4.ihl.mask = 0xF;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_ip_hdr_length
 
 DESCRIPTION:   Gets the IP header length.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                ihl             OUT     IP Hdr Length 

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_ip_hdr_length (DsmFilterH filter_h, uint8_t * ihl)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer3_type != kIPv4 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		

	
	/* copy in the header length values */	
    *ihl = filter_p->layer3.ipv4.ihl.ihl;
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_hdlc_header
 
 DESCRIPTION:   Sets the raw value and mask bytes used for the filter.
                This clears all previous settings stored for the filter,
                i.e. what protocol type and port settings.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_hdlc_header (DsmFilterH filter_h, uint32_t hdlc_hdr, uint32_t mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer2_type != kPoS )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	

	/* set the hdlc header */
	filter_p->layer2.pos.hdlc.hdlc[0] = (uint8_t) ((hdlc_hdr >> 24) & 0xFF);
	filter_p->layer2.pos.hdlc.mask[0] = (uint8_t) ((mask >> 24)     & 0xFF);
	
	filter_p->layer2.pos.hdlc.hdlc[1] = (uint8_t) ((hdlc_hdr >> 16) & 0xFF);
	filter_p->layer2.pos.hdlc.mask[1] = (uint8_t) ((mask >> 16)     & 0xFF);

	filter_p->layer2.pos.hdlc.hdlc[2] = (uint8_t) ((hdlc_hdr >> 8)  & 0xFF);
	filter_p->layer2.pos.hdlc.mask[2] = (uint8_t) ((mask >> 8)      & 0xFF);

	filter_p->layer2.pos.hdlc.hdlc[3] = (uint8_t) ((hdlc_hdr     ) & 0xFF);
	filter_p->layer2.pos.hdlc.mask[3] = (uint8_t) ((mask     )     & 0xFF);

	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_hdlc_header
 
 DESCRIPTION:   Gets the HDLC header for the filter 
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                hdlc_header     OUT     Pointer to a hdlc header 
                mask            OUT     Pointer to a mask array

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_hdlc_header (DsmFilterH filter_h, uint32_t * hdlc_hdr, uint32_t * mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( filter_p->layer2_type != kPoS )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	

	/* set the hdlc header */
    *hdlc_hdr |= ((filter_p->layer2.pos.hdlc.hdlc[0] << 24) & 0xFF000000);
    *mask |= ((filter_p->layer2.pos.hdlc.mask[0] <<24) & 0xFF000000);

	*hdlc_hdr |= ((filter_p->layer2.pos.hdlc.hdlc[1] << 16) & 0xFF0000);
    *mask |= ((filter_p->layer2.pos.hdlc.mask[1] << 16) & 0xFF0000);

    *hdlc_hdr |= ((filter_p->layer2.pos.hdlc.hdlc[2] << 8) & 0xFF00);
    *mask |= ((filter_p->layer2.pos.hdlc.mask[2] << 8) & 0xFF00);

    *hdlc_hdr |= (filter_p->layer2.pos.hdlc.hdlc[3] & 0xFF);
    *mask |= (filter_p->layer2.pos.hdlc.mask[3] & 0xFF);

	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_mac_src_address
 
 DESCRIPTION:   
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_mac_src_address (DsmFilterH filter_h, const uint8_t src[6], const uint8_t mask[6])
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	uint32_t         i;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( src == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer2_type != kEthernet) && (filter_p->layer2_type != kEthernetVlan) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	

	/* set the mac addresses */
	for (i=0; i<6; i++)
	{
		filter_p->layer2.ethernet.src_mac.mac[i]  = src[i];
		filter_p->layer2.ethernet.src_mac.mask[i] = mask[i];
	}	
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_mac_src_address
 
 DESCRIPTION:   Sets the raw value and mask bytes used for the filter.
                This clears all previous settings stored for the filter,
                i.e. what protocol type and port settings.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                src             OUT     Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            OUT     Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_mac_src_address (DsmFilterH filter_h, uint8_t * src, uint8_t * mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	uint32_t         i;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( src == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer2_type != kEthernet) && (filter_p->layer2_type != kEthernetVlan) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		

	/* set the mac addresses */
	for (i=0; i<6; i++)
	{
		src[i] = filter_p->layer2.ethernet.src_mac.mac[i];
		mask[i] = filter_p->layer2.ethernet.src_mac.mask[i];
	}	
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_mac_dst_address
 
 DESCRIPTION:   
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_mac_dst_address (DsmFilterH filter_h, const uint8_t dst[6], const uint8_t mask[6])
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	uint32_t         i;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( dst == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer2_type != kEthernet) && (filter_p->layer2_type != kEthernetVlan) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	

	/* set the mac addresses */
	for (i=0; i<6; i++)
	{
		filter_p->layer2.ethernet.dst_mac.mac[i]  = dst[i];
		filter_p->layer2.ethernet.dst_mac.mask[i] = mask[i];
	}	
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_mac_dst_address
 
 DESCRIPTION:   
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                dst             OUT     Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            OUT     Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_mac_dst_address (DsmFilterH filter_h, uint8_t * dst, uint8_t * mask)
{
	dsm_filter_t   * filter_p = (dsm_filter_t*) filter_h;
	uint32_t         i;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( dst == NULL || mask == NULL )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer2_type != kEthernet) && (filter_p->layer2_type != kEthernetVlan) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	

	/* set the mac addresses */
	for (i=0; i<6; i++)
	{
		dst[i] = filter_p->layer2.ethernet.dst_mac.mac[i];
		mask[i] = filter_p->layer2.ethernet.dst_mac.mask[i];
	}	
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_ethertype
 
 DESCRIPTION:   
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           IN      Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            IN      Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_ethertype (DsmFilterH filter_h, uint16_t ethertype, uint16_t mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer2_type != kEthernet) && (filter_p->layer2_type != kEthernetVlan) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	

	/* set the mac addresses */
	filter_p->layer2.ethernet.ethertype.type = ethertype;
	filter_p->layer2.ethernet.ethertype.mask = mask;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_ethertype
 
 DESCRIPTION:   
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                value           OUT     Pointer to a value array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.
                mask            OUT      Pointer to a mask array, this
                                        array should be at least FILTER_COUNT
                                        bytes big.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_ethertype (DsmFilterH filter_h, uint16_t * ethertype, uint16_t * mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( (filter_p->layer2_type != kEthernet) && (filter_p->layer2_type != kEthernetVlan) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}	

	*ethertype = filter_p->layer2.ethernet.ethertype.type;
	*mask = filter_p->layer2.ethernet.ethertype.mask;
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_enable_vlan
 
 DESCRIPTION:   Enables/disables VLAN support for the filter, if enabled
                the filter expects the ethertype to be VLAN (0x8100) and
                adjust the rest of the fields (including the ethertype
                field) down by four bytes.
 PARAMETERS:    filter_h        IN      Handle to the filter
                enable          IN      A non-zero value enables the
                                        VLAN support, zero disables VLAN
                                        support.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_enable_vlan (DsmFilterH filter_h, uint32_t enable)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	/* check to make sure VLAN is supported */
	if ( (filter_p->layer2_type != kEthernet) && (filter_p->layer2_type != kEthernetVlan) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	
	if ( enable == 0 )
		filter_p->layer2_type = kEthernet;
	else
		filter_p->layer2_type = kEthernetVlan;
	
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_set_vlan_id
 
 DESCRIPTION:   Sets the VLAN id to filter on, this option is only available
                if the VLAN support has been enabled.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                id              IN      Value to filter on, only the lower
                                        12-bits are used.
                mask            IN      Mask value, only the lower 12-bits
                                        used.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_set_vlan_id (DsmFilterH filter_h, uint16_t id, uint16_t mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	/* check to make sure VLAN is supported */
	if ( filter_p->layer2_type != kEthernetVlan )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	filter_p->layer2.ethernet.vlan_id.id   = (id & 0xFFF);
	filter_p->layer2.ethernet.vlan_id.mask = (mask & 0xFFF);
	
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_vlan_id
 
 DESCRIPTION:   Gets the VLAN id for the filter, this option is only available
                if the VLAN support has been enabled.
 
 PARAMETERS:    filter_h        IN      Handle to the filter
                id              OUT     Value to filter on, only the lower
                                        12-bits are used.
                mask            OUT      Mask value, only the lower 12-bits
                                        used.

 RETURNS:       0 if the filter was changed otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_vlan_id (DsmFilterH filter_h, uint16_t * id, uint16_t * mask)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	/* check to make sure VLAN is supported */
	if ( filter_p->layer2_type != kEthernetVlan )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	*id = (filter_p->layer2.ethernet.vlan_id.id & 0xFFF); 
	*mask = (filter_p->layer2.ethernet.vlan_id.mask  & 0xFFF);
	
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_copy
 
 DESCRIPTION:   Copies the contents of one filter into another. It is
                not nesscary for the two filters to be of the same
                configuration, however they must both be the same network
                type either sonet or ethernet filters.
 
 PARAMETERS:    dst_filter_h    IN      The destination filter.
                src_filter_h    IN      The source filter.
 
 RETURNS:       0 if the filter was copied otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_copy (DsmFilterH dst_filter_h, DsmFilterH src_filter_h)
{
	dsm_filter_t * dst_filter_p = (dsm_filter_t*) dst_filter_h;
	dsm_filter_t * src_filter_p = (dsm_filter_t*) src_filter_h;
	uint32_t       actual_filter_num;

	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (dst_filter_p == NULL) || (dst_filter_p->signature != DSM_SIG_FILTER) ||
		 (src_filter_p == NULL) || (src_filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* check to make sure the two filters are the same network type */
	switch (dst_filter_p->layer2_type)
	{
		case kEthernet:
		case kEthernetVlan:
			if ( (src_filter_p->layer2_type != kEthernet) && (src_filter_p->layer2_type != kEthernetVlan) )
			{
				dagdsm_set_last_error (EINVAL);
				return -1;
			}
			break;
		
		case kPoS:
			if ( src_filter_p->layer2_type != kPoS )
			{
				dagdsm_set_last_error (EINVAL);
				return -1;
			}
			break;
			
		default:
			/* unkown configuration */
			dagdsm_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* save the virtual filter number before replacing */
	actual_filter_num = dst_filter_p->actual_filter_num;


	/* do a straight memory copy and then update the virtual filter number */
	memcpy (dst_filter_p, src_filter_p, sizeof(dsm_filter_t));
	dst_filter_p->actual_filter_num = actual_filter_num;
	
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_filter_get_values
 
 DESCRIPTION:   Generates the filter value and mask arrays and copies
                them into the user supplied buffers.
 
 PARAMETERS:    filter_h        IN      Handle to the filter to get the
                                        values from.
                value           OUT     Array to the comparands in
                mask            OUT     Array to store the masks in
                max_size        IN      The maximum number of bytes that
                                        can be copied into the array.
 
 RETURNS:       0 if the filter was copied otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_filter_get_values (DsmFilterH filter_h, uint8_t * value, uint8_t * mask, uint32_t max_size)
{
	dsm_filter_t * filter_p = (dsm_filter_t*) filter_h;
	uint8_t        temp_value[MATCH_DEPTH];
	uint8_t        temp_mask[MATCH_DEPTH];

	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (filter_p == NULL) || (filter_p->signature != DSM_SIG_FILTER) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	if ( ((value == NULL) && (max_size > 0)) || ((mask == NULL) && (max_size > 0)) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
		
	
	/* generate the value and mask arrays */
	dsm_gen_filter_values (filter_p, temp_value, temp_mask);
	
	
	/* copy to the user supplied arrays */
	max_size = (max_size > MATCH_DEPTH) ? MATCH_DEPTH : max_size;
	memcpy (value, temp_value, max_size);
	memcpy (mask,  temp_mask,  max_size);
	
	
	return max_size;
}
