/*
 * Copyright (c) 2006-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagerf.c,v 1.10 2009/06/18 01:54:34 jomi Exp $
 */

/* Routines that are generally useful throughout the programs in the tools directory. 
 * Subject to change at a moment's notice, not supported for customer applications, etc.
 */

/* File header. */
#include "dagerf.h"

/* Endace headers. */
#include "dagapi.h"
#include "dagutil.h"

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: dagerf.c,v 1.10 2009/06/18 01:54:34 jomi Exp $";


const char *
dagerf_type_to_string(uint8_t type, tt_t legacy_type)
{
	switch (type & 0x7F)
	{
		case TYPE_LEGACY:
			switch (legacy_type)
			{
				case TT_ATM:
					return "Legacy ATM";
					
				case TT_POS:
					return "Legacy POS";
					
				case TT_ETH:
					return "Legacy ATM";
				
				case TT_ERROR:
					/* Client has specified that Legacy type is an error. */
					return "Legacy (error)";
				
				default:
					return "Legacy (unknown)";
			}
			break;
		
		case TYPE_HDLC_POS:
			return "ERF PoS";
		
		case TYPE_ETH:
			return "ERF Ethernet";
		
		case TYPE_ATM:
			return "ERF ATM";
		
		case TYPE_AAL5:
			return "ERF AAL5";
		
		case TYPE_AAL2:
			return "ERF AAL2";
		
		case TYPE_MC_HDLC:
			return "ERF Multi Channel HDLC";
		
		case TYPE_MC_RAW:
			return "ERF Multi Channel RAW";
		
		case TYPE_MC_ATM:
			return "ERF Multi Channel ATM";
		
		case TYPE_MC_RAW_CHANNEL:
			return "ERF Multi Channel RAW link data";
		
		case TYPE_MC_AAL5:
			return "ERF Multi Channel AAL5";
		
		case TYPE_MC_AAL2:
			return "ERF Multi Channel AAL2";
		
		case TYPE_COLOR_HDLC_POS:
			return "ERF PoS with Color";
		
		case TYPE_COLOR_HASH_POS:
			return "ERF PoS with Color and Hash";

		case TYPE_COLOR_ETH:
			return "ERF Ethernet with Color";

		case TYPE_COLOR_HASH_ETH:
			return "ERF Ethernet with Color and Hash";

		case TYPE_DSM_COLOR_HDLC_POS:
			return "ERF PoS with DSM Color";
		
		case TYPE_DSM_COLOR_ETH:
			return "ERF Ethernet with DSM Color";
	
		case TYPE_IP_COUNTER:
			return "IP Address Counter";
		
		case TYPE_TCP_FLOW_COUNTER:
			return "TCP Flow Counter";
		
		case TYPE_PAD:
			return "ERF Pad";
        	
		case TYPE_INFINIBAND:
            		return "ERF Infiniband";
        
        case TYPE_INFINIBAND_LINK:
            		return "ERF Infiniband Link";
        
		case TYPE_RAW_LINK:
			return "ERF Raw Link";

		default:
			return "unknown ERF type";
	}
	
	assert(0); /* Should never get here. */
	return "unknown ERF type";
}


const char *
dagerf_record_to_string(uint8_t * erf, tt_t legacy_type)
{
	return dagerf_type_to_string(erf[8], legacy_type);
}


unsigned int
dagerf_get_length(uint8_t * erf)
{
	uint16_t * overlay16 = (uint16_t *) erf;
	
	if (TYPE_LEGACY == erf[8])
	{
		return 64;
	}
	
	return ntohs(overlay16[5]);
}


unsigned int
dagerf_is_known_type(uint8_t * erf)
{
	if ((erf[8] & 0x7F) <= TYPE_MAX)
	{
		return 1;
	}
	
	return 0;
}


unsigned int
dagerf_is_legacy_type(uint8_t * erf)
{
	if (TYPE_LEGACY == erf[8])
	{
		return 1;
	}
	
	return 0;
}


unsigned int
dagerf_is_modern_type(uint8_t * erf)
{
	if ((TYPE_MIN <= (erf[8] & 0x7F)) && ((erf[8] & 0x7F) <= TYPE_MAX))
	{
		return 1;
	}
	
	return 0;
}


unsigned int
dagerf_is_ethernet_type(uint8_t * erf)
{
	switch (erf[8] & 0x7F)
	{
		case TYPE_ETH:
		case TYPE_COLOR_ETH:
		case TYPE_DSM_COLOR_ETH:
			return 1;
	}
	
	return 0;
}


unsigned int
dagerf_is_pos_type(uint8_t * erf)
{
	switch (erf[8] & 0x7F)
	{
		case TYPE_HDLC_POS:
		case TYPE_COLOR_HDLC_POS:
		case TYPE_DSM_COLOR_HDLC_POS:
			return 1;
	}

	return 0;
}


unsigned int
dagerf_is_color_type(uint8_t * erf)
{
	switch (erf[8] & 0x7F)
	{
		case TYPE_COLOR_HDLC_POS:
		case TYPE_DSM_COLOR_HDLC_POS:
		case TYPE_COLOR_ETH:
		case TYPE_DSM_COLOR_ETH:
		case TYPE_COLOR_HASH_POS:
		case TYPE_COLOR_HASH_ETH:
			return 1;
	}

	return 0;
}


unsigned int
dagerf_is_multichannel_type(uint8_t * erf)
{
	switch (erf[8] & 0x7F)
	{
		case TYPE_MC_HDLC:
		case TYPE_MC_RAW:
		case TYPE_MC_ATM:
		case TYPE_MC_RAW_CHANNEL:
		case TYPE_MC_AAL5:
		case TYPE_MC_AAL2:
			return 1;
	}

	return 0;
}


unsigned int
dagerf_ext_header_count(uint8_t * erf, size_t len)
{
	uint32_t hdr_num = 0;
	uint8_t  hdr_type;

	/* basic sanity checks */
	if ( erf == NULL )
		return 0;
	if ( len < 16 )
		return 0;


	/* check if we have any extension headers */
	if ( (erf[8] & 0x80) == 0x00 )
		return 0;

	/* loop over the extension headers */
	do {
	
		/* sanity check we have enough bytes */
		if ( len <= (24 + (hdr_num * 8)) )
			return hdr_num;

		/* get the header type */
		hdr_type = erf[(16 + (hdr_num * 8))];
		hdr_num++;

	} while ( hdr_type & 0x80 );

	return hdr_num;
}

