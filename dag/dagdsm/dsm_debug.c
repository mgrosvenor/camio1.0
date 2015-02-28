/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_debug.c,v 1.13 2007/08/17 02:52:30 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
* 
* DESCRIPTION:  This file contains utility functions for the DSM loader
*               application. These functions are deliberately undocumented
*               and are subject to change, use these functions ONLY FOR
*               DEBUGGING.
*
* 
*
**********************************************************************/


/* Endace headers */
#include "dag_platform.h"
#include "dagapi.h"
#include "dagcrc.h"
#include "dagutil.h"

/* DSM headers */
#include "dagdsm.h"
#include "dsm_types.h"








/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_display_counters
 
 DESCRIPTION:   User function to display the counter information
 
 PARAMETERS:    config_h        IN      Handle to a configuration
                stream          IN      Output stream to dump the data
                                        to.

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_display_counters (DsmConfigH config_h, FILE * stream)
{
	dsm_config_t * config_p = (dsm_config_t*)config_h;
	uint32_t       value;
	uint16_t       i;
	
	
	/* sanity checking */
	if ( config_p == NULL || config_p->signature != DSM_SIG_CONFIG )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}


	/* read the counters and format */
	for (i=0; i<8; i++)
	{
		value = dsm_read_counter (config_p->base_reg, (DSM_CNT_FILTER0 + i));
		fprintf (stream, "Filter %2d (actual %2d): %10u [%08X]\n", 
			i, config_p->filters[i].actual_filter_num, value, value);
	}

	if(config_p->dsm_version!=DSM_VERSION_1)
	{
		value = dsm_read_counter (config_p->base_reg, DSM_CNT_HLB0);
		fprintf (stream, "HLB (CRC)          : %10u [%08X]\n", value, value);
	
		value = dsm_read_counter (config_p->base_reg, DSM_CNT_HLB1);
		fprintf (stream, "HLB (Parity)       : %10u [%08X]\n", value, value);
	}

	value = dsm_read_counter (config_p->base_reg, DSM_CNT_DROP);
	fprintf (stream, "Drop counter       : %10u [%08X]\n", value, value);
	
	if(config_p->dsm_version!=DSM_VERSION_1)
	{
		for (i=0; i<config_p->rx_streams; i++)
		{
			value = dsm_read_counter (config_p->base_reg, (DSM_CNT_STREAM_BASE + i));
			fprintf (stream, "Stream %2u          : %10u [%08X]\n", (i<<1), value, value);
		}
	}
	
	return 0;
}








/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_dump_colur
 
 DESCRIPTION:   Debugging routine used to dump the colur table to
                stdout. 
 
 PARAMETERS:    bank            IN      The bank the colur table 
                                        corresponds to.
                rx_streams      IN      The number of receive streams
                                        this directly relates to the
                                        size of each entry.
                colur           IN      A buffer holding the colur.
                size            IN      The size of the colur in 16-bit
                                        words.

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
void
dsm_dump_colur (dsm_config_t * config_p, FILE * stream, uint32_t bank, uint16_t * colur, uint32_t size)
{
	uint32_t  entry;
	uint32_t  addr;
	uint32_t  rows;
	uint32_t  entries;
	uint32_t  entries_per_row;
	uint32_t  entry_mask;
	uint32_t  i;
	uint32_t  j;
    uint32_t  iface,hash_value;
	
	
	
	
	/* calculate the number of entires, rows and entries per row */
	entries = (1 << (DSM_FILTER_BITS + DSM_INTERFACE_BITS + DSM_HLB_BITS));
	entries_per_row = 16 / config_p->bits_per_entry;
	rows = entries / entries_per_row;
	
	assert(size >= rows);
	if ( size < rows )
		return;
	
	/* dump out the table metrics */
	fprintf (stream, " Bits per Entry: %d\n", config_p->bits_per_entry);
	fprintf (stream, "        Entries: %d\n", entries);
	fprintf (stream, "Entries Per Row: %d\n", entries_per_row);
	fprintf (stream, "           Rows: %d\n\n", rows);
	
	
	
	/* calculate the bitmask to use for an entry */
	entry_mask = ((1 << config_p->bits_per_entry) - 1);
	
    if(config_p->dsm_version==DSM_VERSION_0)
    {
    	/* write the initial heading */
    	fprintf (stream,  "       : hlb-par hlb-crc  iface  f7 f6 f5 f4 f3 f2 f1 f0 : bm   | row entry\n");
    	fprintf (stream,  "---------------------------------------------------------------------------\n");
    }
    else if(config_p->dsm_version==DSM_VERSION_1)
    {
    	/* write the initial heading */
    	fprintf (stream,  "       : hlb-hash  iface  f7 f6 f5 f4 f3 f2 f1 f0 : bm   | row entry\n");
    	fprintf (stream,  "---------------------------------------------------------------------------\n");
        
    }

	

	/* loop through the colur and dump to the file stream */	
	for (i=0; i<rows; i++)
		for (j=0; j<entries_per_row; j++)
		{
			entry = (colur[i] >> (j * config_p->bits_per_entry)) & entry_mask;
			addr  = ((i * entries_per_row) + j);
			
            if(config_p->dsm_version==DSM_VERSION_0)
            {
			    fprintf (stream,  "0x%03X  :    %d       %d      %d%d     %d  %d  %d  %d  %d  %d  %d  %d : ",
			                  addr, 
			                  ((addr >> 11) & 0x001), ((addr >> 10) & 0x001), 
			                  ((addr >>  9) & 0x001), ((addr >>  8) & 0x001), 
			                  ((addr >>  7) & 0x001), ((addr >>  6) & 0x001), 
			                  ((addr >>  5) & 0x001), ((addr >>  4) & 0x001), 
			                  ((addr >>  3) & 0x001), ((addr >>  2) & 0x001), 
			                  ((addr >>  1) & 0x001), ((addr >>  0) & 0x001) ); 
            }			                  
            else if(config_p->dsm_version==DSM_VERSION_1)
            {
                hash_value = 0;
                iface = 0;/*to shut gcc*/
                if(config_p->device_code==PCI_DEVICE_ID_DAG4_52Z)
                {
                    hash_value = (addr>>10)&0x003;
                    iface = (addr>>8)&0x003; 
                }
                else if((config_p->device_code==PCI_DEVICE_ID_DAG4_52Z8) || config_p->device_code==PCI_DEVICE_ID_DAG8_20Z || (config_p->device_code== PCI_DEVICE_ID_DAG5_0Z) )// || (config_p->device_code== PCI_DEVICE_ID_DAG5_00S))
                {
                    hash_value = (addr>>9)&0x007;
                    iface = (addr>>8)&0x001;
                }

			    fprintf (stream,  "0x%03X  :     %d       %d     %d  %d  %d  %d  %d  %d  %d  %d : ",
			                  addr, 
			                  hash_value, iface, 
			                  ((addr >>  7) & 0x001), ((addr >>  6) & 0x001), 
			                  ((addr >>  5) & 0x001), ((addr >>  4) & 0x001), 
			                  ((addr >>  3) & 0x001), ((addr >>  2) & 0x001), 
			                  ((addr >>  1) & 0x001), ((addr >>  0) & 0x001) ); 
            }
			if ( entry & (1 << (config_p->bits_per_entry - 1)) )
				fprintf (stream, "drop");
			else
				fprintf (stream, "%2d  ", (entry << 1));
			
			fprintf (stream, " |  0x%04X\n", colur[i]);
		}
	
}






/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_dump_filter
 
 DESCRIPTION:   Debugging routine used to dump a particular filter to
                stdout. 
 
 PARAMETERS:    filter          IN      The filter number to read back
                value & mask    IN      The filter to display.

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
dsm_dump_filter (FILE * stream, uint32_t filter, uint8_t value[MATCH_DEPTH], uint8_t mask[MATCH_DEPTH], uint32_t term_depth)
{
	char      str[33];
	char      term_char;
	uint32_t  i, j, k;
	
	str[32] = '\0';
	
	for (i=0; i<(MATCH_DEPTH/4); i++)
	{
		for (j=0; j<4; j++)
		{
			for (k=0; k<8; k++)
			{
				if ( (mask[(i*4)+j] & (1<<k)) == 0 )
					str[(j*8)+(7-k)] = '-';
				
				else if ( (value[(i*4)+j] & (1<<k)) == 0 )
					str[(j*8)+(7-k)] = '0';
				
				else
					str[(j*8)+(7-k)] = '1';
			}
		}

		if ( (uint32_t)(i/2) == term_depth )    term_char = '*';
		else                          term_char = ' ';
				
		fprintf (stream, "Filter%d : %02X [%s]  Value[%08X] Mask[%08X] %c\n",
			filter, (i*4), str,
			(((uint32_t)value[(i*4)] << 24) | ((uint32_t)value[(i*4)+1] << 16) | ((uint32_t)value[(i*4)+2] << 8) | ((uint32_t)value[(i*4)+3])),
			(((uint32_t)mask[(i*4)] << 24)  | ((uint32_t)mask[(i*4)+1] << 16)  | ((uint32_t)mask[(i*4)+2] << 8)  | ((uint32_t)mask[(i*4)+3])),
			term_char);
	}
	
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_display_colur
 
 DESCRIPTION:   User function to display the information in the COLUR			
 
 PARAMETERS:    config_h        IN      Handle to a configuration
                stream          IN      Output stream to dump the data
                                        to.
                bank            IN      The bank to read and display

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_display_colur (DsmConfigH config_h, FILE * stream, uint32_t bank, uint32_t flags)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;
	uint16_t         * table_p;
	uint32_t           table_size;
	volatile uint8_t * dsm_base_reg;
	
	/* sanity checking */
	if ( config_p == NULL || config_p->signature != DSM_SIG_CONFIG )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}


	/* allocate memory for the table */
	table_size = ((1 << (DSM_FILTER_BITS + DSM_INTERFACE_BITS + DSM_HLB_BITS)) * config_p->bits_per_entry) / 16;
	
	table_p = (uint16_t*) malloc(table_size * 2);
	if ( table_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}

	
	/* check if we should be using the local COLUR or the one in the card */
	if ( flags & DSM_DBG_INTERNAL_COLUR )
	{
		/* construct the table */
		if ( dsm_construct_colur(config_p, table_p, table_size) == -1 )
		{
			free (table_p);
			return -1;
		}
	}
	
	else 
	{
		/* get a pointer to the base register used for DSM */
		dsm_base_reg = config_p->base_reg;
		
		/* read the table from the card */
		if ( dsm_read_colur(config_p, dsm_base_reg, bank, table_p, table_size) == -1 )
		{
			free (table_p);
			return -1;
		}		
	}
	
	
		
	/* decode and display the table */
	dsm_dump_colur (config_p, stream, bank, table_p, table_size);


	/* free the buffer */
	free (table_p);
	
	return 0;
}






/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_display_filter
 
 DESCRIPTION:   User function to display the contents of a filter
 
 PARAMETERS:    config_h        IN      Handle to a configuration
                stream          IN      Output stream to dump the data
                                        to.
                filter          IN      The number of the filter (0-8)

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_display_filter (DsmConfigH config_h, FILE * stream, uint32_t filter, uint32_t flags)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;
	uint8_t            mask[MATCH_DEPTH];
	uint8_t            value[MATCH_DEPTH];
	uint32_t           term_depth;
	int                enabled;
	
	/* sanity checking */
	if ( config_p == NULL || config_p->signature != DSM_SIG_CONFIG )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	if ( filter >= FILTER_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}		
	
	
	/* if an internal filter */	
	if ( flags & DSM_DBG_INTERNAL_FILTER )
	{

		/* find the virtual filter */
		fprintf (stream, "soft-filter, virtual filter %d, actual filter %d, ", filter, config_p->filters[filter].actual_filter_num);
		if ( config_p->filters[filter].enabled )
			fprintf (stream, "enabled");
		else
			fprintf (stream, "disabled");
		
		
		/* get the early termination depth */
		term_depth = config_p->filters[filter].term_depth;
		
		/* generate the filter masks and values */
		dsm_gen_filter_values (&(config_p->filters[filter]), value, mask);
	}
	
	else
	{
		/* get the filter */
		if ( dsm_read_filter(config_p->base_reg, filter, &term_depth, value, mask) == -1 )
			return -1;
		
		/* check if the filter is enabled */
		if ( (enabled = dsm_is_filter_enabled(config_p->base_reg, filter)) == -1 )
			return -1;
		

		/* display enabled/disabled state */		
		fprintf (stream, "hard-filter, ");
		if ( enabled == 1 )
			fprintf (stream, "enabled");
		else
			fprintf (stream, "disabled");


	}	
	

	/* determine if the early termination flag is set */
	if ( term_depth < LAST_FILTER_ELEMENT )
		fprintf (stream, ", early-terminate on element %d\n", term_depth);
	else
		fprintf (stream, "\n");
	
	
	/* display the filter */
	dsm_dump_filter (stream, filter, value, mask, term_depth);

	
	return 0;
}





	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_compare_filter
 
 DESCRIPTION:   
 
 PARAMETERS:    config_h        IN      Handle to a configuration
                

 RETURNS:       If the filters are a match then 1 is returned, if not
                a match 0 is returned, if an error occurred -1 is
                returned.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_compare_filter (DsmConfigH config_h, uint32_t filter, const uint8_t * value, const uint8_t * mask, uint32_t size, uint8_t * read_value, uint8_t * read_mask)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;
	uint8_t            hw_mask[MATCH_DEPTH];
	uint8_t            hw_value[MATCH_DEPTH];
	size_t             compare_len;
	uint32_t           term_depth;
	
	
	/* sanity checking */
	if ( config_p == NULL || config_p->signature != DSM_SIG_CONFIG )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	if ( filter >= FILTER_COUNT )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}			
	
	
	/* get the filter */
	if ( dsm_read_filter(config_p->base_reg, filter, &term_depth, hw_value, hw_mask) == -1 )
		return -1;
	

	/* get the length to compare against */
	compare_len = (size < MATCH_DEPTH) ? size : MATCH_DEPTH;


	
	/* copy over the read values (if the user supplied buffers) */
	if ( read_value != NULL )
		memcpy (read_value, hw_value, compare_len);
	if ( read_mask != NULL )
		memcpy (read_mask, hw_mask, compare_len);


	
	
	/* do the comparision */
	if  ( memcmp(hw_mask, mask, compare_len) != 0 )
		return 0;
	if  ( memcmp(hw_value, value, compare_len) != 0 )
		return 0;
	
	return 1;	
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_compare_colur
 
 DESCRIPTION:   Compares the COLUR installed on the card with the one
                supplied in the buffer. If both are the same 0 is returned,
                otherwise 1 is returned.
 
 PARAMETERS:    config_h        IN      Handle to a configuration
                lut             IN      The COLUR on the card to compare
                                        against.
                data_p          IN      The array used to compare the two
                                        COLUR's.
                size            IN      The size of the buffer to compare

 RETURNS:       1 if the buffer and the COLUR on the card match, 0 if
                they are not a match. -1 is returned if there is an error,
                call dagdsm_get_last_error to reteive the error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_compare_colur (DsmConfigH config_h, uint32_t bank, const uint8_t * data_p, uint32_t size)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;
	uint16_t         * table_p;
	uint32_t           table_size;
	volatile uint8_t * dsm_base_reg;
	size_t             compare_len;
	int                res;
	
	/* sanity checking */
	if ( config_p == NULL || config_p->signature != DSM_SIG_CONFIG )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	
	
	/* allocate memory for the table */
	table_size = ((1 << (DSM_FILTER_BITS + DSM_INTERFACE_BITS + DSM_HLB_BITS)) * config_p->bits_per_entry) / 16;
	assert(table_size == 512);
	
	table_p = (uint16_t*) malloc(table_size * 2);
	if ( table_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}	
	
	

	/* get a pointer to the base register used for DSM */
	dsm_base_reg = config_p->base_reg;
		
	/* read the table from the card */
	if ( dsm_read_colur(config_p,dsm_base_reg, bank, table_p, table_size) == -1 )
	{
		free (table_p);
		return -1;
	}		

	
	/* get the length to compare against */
	compare_len = (size < table_size) ? size : table_size;
	
	/* do the comparision */
	res = memcmp (table_p, data_p, compare_len);
	
	free (table_p);

	return (res == 0) ? 1 : 0;	
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_test_packet_filter
 
 DESCRIPTION:   Accepts a packet from the end of the ERF header and
                checks it against the specified filter.
 
 PARAMETERS:    filter          IN      The filter number to check against
                buf             IN      Buffer that contains the packet
                                        to be filtered on.
                bufLen          IN      The length of the buffer in bytes

 RETURNS:       1 if the filter 'hits', 0 if it misses or -1 if there
                was an error.

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_test_packet_filter (dsm_config_t * config_p, uint32_t filter, const uint8_t * buf, uint32_t bufLen)
{
	uint8_t        value[MATCH_DEPTH];
	uint8_t        mask[MATCH_DEPTH];
	uint32_t       match_depth;
	uint32_t            i;
	dsm_filter_t * filter_p;
	
	
	
	/* sanity checking */
	dagdsm_set_last_error (0);
	if ( (config_p == NULL) || (config_p->signature != DSM_SIG_CONFIG) )
		return -1;
	if ( filter >= FILTER_COUNT )
		return -1;
	
	
	
	
	/* find the filter with the virtual filter number */
	filter_p = &(config_p->filters[filter]);

	
	/* check if the filter is enabled */
	if ( !filter_p->enabled )
		return 0;
	

	/* generate the filter and mask for the filter object */		
	dsm_gen_filter_values (filter_p, value, mask);

	
	/* calculate the require match depth for the filter, if the packet is not big enough */
	match_depth = filter_p->term_depth * 8;
	for (i=7; i>=0; i--)
		if ( mask[match_depth + i] != 0x00 )
		{
			match_depth += i;
			break;
		}
	
		
	/* the filter will fail if the packet is smaller than the filter */
	if ( bufLen < match_depth )
		return 0;		
	

	
	/* do the comparision */
	for (i=0; i<match_depth; i++)
	{
		if ( (buf[i] & mask[i]) != value[i] )
			return 0;	
	}		
			
	return 1;		
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_calc_parity_hlb
 
 DESCRIPTION:   Small utility function to calculate the parity based
                hash load balancing value for a packet.
 
 PARAMETERS:    drec_p          IN      Pointer to a complete erf record

 RETURNS:       1 for HLB hit, otherwise 0.

 HISTORY:       

---------------------------------------------------------------------*/
static uint32_t
dsm_calc_parity_hlb (const dag_record_t * drec_p)
{
	uint32_t     * srcip;
	uint32_t     * destip;
	uint32_t       resxor;
	uint16_t     * r1 = NULL;
	uint32_t       parity = 0;
	
	assert(drec_p != NULL);

	/* extract the source and destination IP addresses */
	switch( drec_p->type ) 
	{
		case TYPE_DSM_COLOR_HDLC_POS:
			r1 = (uint16_t*)drec_p->rec.pos.pload;
			break;
	
		case TYPE_DSM_COLOR_ETH:
			r1 = (uint16_t*)drec_p->rec.eth.pload;
			break;

		default:
			assert(0);
			return 0;
	}
	
	/* xor the destination and source addresses */
	srcip = (uint32_t*)(r1+6);
	destip = (uint32_t*)(r1+8);
	resxor = *destip ^ *srcip;

	
	/* calculate the parity */
	do
	{
		parity += (resxor & 1);
	} while( resxor >>= 1 );

	return (parity & 1);
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_calc_crc_hlb
 
 DESCRIPTION:   Small utility function to calculate the CRC based
                hash load balancing value for a packet.
 
 PARAMETERS:    drec_p          IN      Pointer to a complete erf record

 RETURNS:       1 for HLB hit, otherwise 0.

 HISTORY:       

---------------------------------------------------------------------*/
static uint32_t
dsm_calc_crc_hlb (const dag_record_t * drec_p)
{
	uint32_t     * srcip;
	uint32_t     * destip;
	uint32_t       resxor;
	uint16_t     * r1 = NULL;
	uint32_t       rescrc;
	
	assert(drec_p != NULL);

	/* extract the source and destination IP addresses */
	switch( drec_p->type ) 
	{
		case TYPE_DSM_COLOR_HDLC_POS:
			r1 = (uint16_t*)drec_p->rec.pos.pload;
			break;
	
		case TYPE_DSM_COLOR_ETH:
			r1 = (uint16_t*)drec_p->rec.eth.pload;
			break;

		default:
			assert(0);
			return 0;
	}
	
	/* xor the destination and source addresses */
	srcip = (uint32_t*)(r1+6);
	destip = (uint32_t*)(r1+8);
	resxor = *destip ^ *srcip;
	
	
	/* calculate the crc */
	dagcrc_make_aal5_table ();
	rescrc = dagcrc_aal5_crc( CRC_INIT, (char*)&resxor, 4 );
	
	
	return (rescrc & 0x07) % 2;
	//return (rescrc & 1);
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_compute_packet
 
 DESCRIPTION:   Tests a packet against the current set of filters and
                expression to see which receive stream it should go to,
                if any. This function is only used for debugging, it is
                used to verify the firmware output is correct. The
                interface is assumed to be in the ERF header.
 
 PARAMETERS:    config_h        IN      Handle to a configuration
                pkt_p           IN      Pointer to the ERF record to 
                                        check, the buffer should have
                                        a complete ERF record including
                                        the ERF header.
                rlen            IN      The length of the ERF record.
                lookup          OUT     Optional value that accepts
                                        the 12-bit lookup value.

 RETURNS:       -1 if the packet should be dropped or there was an error,
                otherwise a positive number representing which ERF stream
                the packet should have come from. To determine if -1 was
                returned for an error call dagdsm_get_last_error().
                
 NOTES:         The HLB part of the packet is unknown at the moment, this
                function doesn't generate a HLB value for the packet therefore
                it is not a real test, this needs to be fixed.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_compute_packet (DsmConfigH config_h, uint8_t * pkt_p, uint16_t rlen, uint32_t * lookup)
{
	dsm_config_t      * config_p = (dsm_config_t*)config_h;
	dag_record_t      * drec_p;
	uint8_t             iface;
	uint32_t            filters;
	uint32_t            stream;
	uint32_t            hlb0, hlb1;
	uint32_t            i;

	
	/* sanity checking */
	if ( config_p == NULL || config_p->signature != DSM_SIG_CONFIG )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}

	assert (rlen >= 16);
	if ( rlen < 16 )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	
	/* cast to an ERF header structure and confirm the details */
	drec_p = (dag_record_t*) (pkt_p);
	
		
	
	/* get the interface */
	iface = drec_p->flags.iface;
	

	/* check which filters should hit */
	filters = 0x00;
	
	if ( dsm_test_packet_filter(config_p, 0, &(pkt_p[16]), (rlen - 16)) == 1 )
		filters |= 0x01;
	if ( dsm_test_packet_filter(config_p, 1, &(pkt_p[16]), (rlen - 16)) == 1 )
		filters |= 0x02;
	if ( dsm_test_packet_filter(config_p, 2, &(pkt_p[16]), (rlen - 16)) == 1 )
		filters |= 0x04;
	if ( dsm_test_packet_filter(config_p, 3, &(pkt_p[16]), (rlen - 16)) == 1 )
		filters |= 0x08;
	if ( dsm_test_packet_filter(config_p, 4, &(pkt_p[16]), (rlen - 16)) == 1 )
		filters |= 0x10;
	if ( dsm_test_packet_filter(config_p, 5, &(pkt_p[16]), (rlen - 16)) == 1 )
		filters |= 0x20;
	if ( dsm_test_packet_filter(config_p, 6, &(pkt_p[16]), (rlen - 16)) == 1 )
		filters |= 0x40;
	
	
	/* calculate the two HLB bits for the packet */
	hlb0 = dsm_calc_crc_hlb (drec_p);
	hlb1 = dsm_calc_parity_hlb (drec_p);
	
	
	
	/* check the output expression to see which stream the packet should go to */
	stream = (uint32_t)-1;
	for (i=0; i<config_p->rx_streams; i++)
		if ( dagdsm_compute_output_expr_value((DsmOutputExpH)&(config_p->output_expr[i]), filters, iface, hlb0, hlb1) == 1 )
		{
			stream = i;
			break;
		}
	
	
	/* calculate the 12-bit lookup number for the caller*/
	if ( lookup != NULL )
		*lookup = ((uint32_t)filters) | ((uint32_t)iface << 8) | ((uint32_t)hlb0 << 10) | ((uint32_t)hlb1 << 11);
		
		
	/* return the result */
	if ( stream == (uint32_t)-1 )
		return -1;
	
	else
		return (int)(stream << 1);
}
