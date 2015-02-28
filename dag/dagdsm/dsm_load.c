/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_load.c,v 1.7 2007/08/15 22:34:31 dlim Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         dsm_load.c
* DESCRIPTION:  
*
*
* HISTORY:      22-01-06 Initial revision
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"
#include "dagapi.h"
#include "dagutil.h"

/* DSM headers */
#include "dagdsm.h"
#include "dsm_types.h"




/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_drb_delay
 
 DESCRIPTION:   Fixed delay used between read and writes of the DRB bus,
                these delays are required for DSM register access.
 
 PARAMETERS:    addr            IN      Address of the register to read from

 RETURNS:       the value in the register

 HISTORY:       

---------------------------------------------------------------------*/
uint32_t
dsm_drb_delay (volatile uint32_t * dsm_csr_reg_p)
{
	uint32_t value = 0;
	uint32_t i;
	
	for (i=0; i<16; i++)
		value += dag_readl (dsm_csr_reg_p);
	
	return value;
}
//void
//dsm_drb_delay (void)
//{
//	dagutil_microsleep (1);
//	
//}






/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_update_filter_byte
 
 DESCRIPTION:   Updates a byte in the array using one of the following
                modes; SET, CLEAR, OR, AND
 
 PARAMETERS:    array_p         IN/OUT  Pointer to array to modify.
                value           IN      The value to apply to the element,
                                        this value is ignored if the mode
                                        is SET.
                offset          IN      The offset into the array
                mode            IN      The update mode

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static inline void
dsm_update_filter_byte(uint8_t array_p[MATCH_DEPTH], uint8_t value, uint32_t offset, uint32_t mode)
{
	if ( offset >= MATCH_DEPTH )
		return;
	
	switch (mode)
	{
		case DSM_SET:    array_p[offset]  = value;		break;
		case DSM_CLR:    array_p[offset]  = 0x00;		break;
		case DSM_OR:     array_p[offset] |= value;		break;
		case DSM_AND:    array_p[offset] &= value;		break;
	}
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_gen_ipv4_filter_values
 
 DESCRIPTION:   Generates the actual mask and value pair to load into
                the filters on the card for an IPv4 filter. This function
                is supplied with the IPv4 header offset and an array
                of bytes to put the filter values and masks into.
 
 PARAMETERS:    filter_p        IN      Pointer to filter to generate the
                                        mask and value set from.
                value           IN/OUT  The value for the filter
                mask            IN/OUT  The mask for the filter
                ipv4_offset     IN      The offset into the array to put
                                        the IPv4 details

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
dsm_gen_ipv4_filter_values (dsm_filter_t * filter_p, uint8_t value[MATCH_DEPTH], uint8_t mask[MATCH_DEPTH], uint32_t ipv4_offset)
{
	uint32_t layer4_offset;
	uint32_t i;
	
	
	/* clear everything from the IPv4 header and down */
	for (i=ipv4_offset; i<MATCH_DEPTH; i++)
		value[i] = mask[i] = 0x00;
	
	
	
	/* set the ip version */
	//dsm_update_filter_byte (value, 0x40, ipv4_offset, DSM_OR);
	//dsm_update_filter_byte (mask,  0xF0, ipv4_offset, DSM_OR);
	
	
	
	/* ip header details */
	for ( i=0; i<4; i++)
	{
		dsm_update_filter_byte (value, filter_p->layer3.ipv4.src_addr.addr[i], ipv4_offset + 12 + i, DSM_SET);
		dsm_update_filter_byte (mask,  filter_p->layer3.ipv4.src_addr.mask[i], ipv4_offset + 12 + i, DSM_SET);
		
		dsm_update_filter_byte (value, filter_p->layer3.ipv4.dst_addr.addr[i], ipv4_offset + 16 + i, DSM_SET);
		dsm_update_filter_byte (mask,  filter_p->layer3.ipv4.dst_addr.mask[i], ipv4_offset + 16 + i, DSM_SET);
	}
		
	/* fragmentation */
	if ( filter_p->layer3.ipv4.frag_filter == kFragReject )
	{
		dsm_update_filter_byte (value, 0xC0, ipv4_offset + 6, DSM_AND);
		dsm_update_filter_byte (value, 0x00, ipv4_offset + 7, DSM_SET);
		dsm_update_filter_byte (mask,  0x3F, ipv4_offset + 6, DSM_OR);
		dsm_update_filter_byte (mask,  0xFF, ipv4_offset + 7, DSM_OR);
	}


	/* ip header length */
	if ( filter_p->layer3.ipv4.ihl.mask == 0 )
		layer4_offset = ipv4_offset + 20;
	else
		layer4_offset = ipv4_offset + (filter_p->layer3.ipv4.ihl.ihl * 4);
	
	dsm_update_filter_byte (value, filter_p->layer3.ipv4.ihl.ihl & 0x0F,  ipv4_offset, DSM_OR);
	dsm_update_filter_byte (mask,  filter_p->layer3.ipv4.ihl.mask & 0x0F, ipv4_offset, DSM_OR);



	/* set the protocol */
	if ( filter_p->layer3.ipv4.protocol != 0 )
	{
		dsm_update_filter_byte (value, filter_p->layer3.ipv4.protocol, ipv4_offset + 9, DSM_SET);
		dsm_update_filter_byte (mask,  0xFF,                           ipv4_offset + 9, DSM_SET);
	}
	
		
	/* add the specialist fields */
	switch (filter_p->layer4_type)
	{
		case kTCP:
				
			/* set the tcp flags */
			dsm_update_filter_byte (value, filter_p->layer4.tcp.tcp_flags.flags & 0x3F, layer4_offset + 13, DSM_OR);
			dsm_update_filter_byte (mask,  filter_p->layer4.tcp.tcp_flags.mask  & 0x3F, layer4_offset + 13, DSM_OR);
				
			/* add the ports */
			dsm_update_filter_byte (value, (filter_p->layer4.tcp.src_port.port & 0xFF00) >> 8, layer4_offset + 0, DSM_SET);
			dsm_update_filter_byte (value, (filter_p->layer4.tcp.src_port.port & 0x00FF),      layer4_offset + 1, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.tcp.src_port.mask & 0xFF00) >> 8, layer4_offset + 0, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.tcp.src_port.mask & 0x00FF),      layer4_offset + 1, DSM_SET);

			dsm_update_filter_byte (value, (filter_p->layer4.tcp.dst_port.port & 0xFF00) >> 8, layer4_offset + 2, DSM_SET);
			dsm_update_filter_byte (value, (filter_p->layer4.tcp.dst_port.port & 0x00FF),      layer4_offset + 3, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.tcp.dst_port.mask & 0xFF00) >> 8, layer4_offset + 2, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.tcp.dst_port.mask & 0x00FF),      layer4_offset + 3, DSM_SET);

			break;
		
		case kUDP:
				
			/* add the ports */
			dsm_update_filter_byte (value, (filter_p->layer4.udp.src_port.port & 0xFF00) >> 8, layer4_offset + 0, DSM_SET);
			dsm_update_filter_byte (value, (filter_p->layer4.udp.src_port.port & 0x00FF),      layer4_offset + 1, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.udp.src_port.mask & 0xFF00) >> 8, layer4_offset + 0, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.udp.src_port.mask & 0x00FF),      layer4_offset + 1, DSM_SET);

			dsm_update_filter_byte (value, (filter_p->layer4.udp.dst_port.port & 0xFF00) >> 8, layer4_offset + 2, DSM_SET);
			dsm_update_filter_byte (value, (filter_p->layer4.udp.dst_port.port & 0x00FF),      layer4_offset + 3, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.udp.dst_port.mask & 0xFF00) >> 8, layer4_offset + 2, DSM_SET);
			dsm_update_filter_byte (mask,  (filter_p->layer4.udp.dst_port.mask & 0x00FF),      layer4_offset + 3, DSM_SET);

			break;
			
		case kICMP:
				
			/* set the type and code */
			dsm_update_filter_byte (value, filter_p->layer4.icmp.icmp_type.type, layer4_offset + 0, DSM_SET);
			dsm_update_filter_byte (mask,  filter_p->layer4.icmp.icmp_type.mask, layer4_offset + 0, DSM_SET);
			dsm_update_filter_byte (value, filter_p->layer4.icmp.icmp_code.code, layer4_offset + 1, DSM_SET);
			dsm_update_filter_byte (mask,  filter_p->layer4.icmp.icmp_code.mask, layer4_offset + 1, DSM_SET);

			break;
			
	}	
	
		
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_gen_filter_values
 
 DESCRIPTION:   Generates the actual mask and value pair to load into
                the filters on the card. This takes into account the
                layer 2,3 and 4 settings before constructing the filter.
 
 PARAMETERS:    filter_p        IN      Pointer to filter to generate the
                                        mask and value set from.
                value           OUT     The value for the filter
                mask            OUT     The mask for the filter

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
void
dsm_gen_filter_values (dsm_filter_t * filter_p, uint8_t value[MATCH_DEPTH], uint8_t mask[MATCH_DEPTH])
{
	uint32_t layer3_offset;
	uint32_t i;
	
	
	/* sanity checking */
	assert(filter_p != NULL);
	assert(value != NULL);
	assert(mask  != NULL);
	
	

	memset (value, 0, MATCH_DEPTH);
	memset (mask,  0, MATCH_DEPTH);

	
	/* check the filter is enabled before populating the filter */
	//if ( !filter_p->enabled )
	//	return;
	
	
	/* check if we should be using the raw bytes  */
	if ( filter_p->raw_mode == 1 )
	{
		memcpy (value, filter_p->raw.value, MATCH_DEPTH);
		memcpy (mask,  filter_p->raw.mask,  MATCH_DEPTH);
		return;
	}
	
		
	
	
	/* calculate the layer 3 offset from the start of the filter */
	switch ( filter_p->layer2_type )
	{
		case kEthernet:     layer3_offset = 16;     break;
		case kPoS:          layer3_offset = 4;      break;
		case kEthernetVlan: layer3_offset = 20;     break;
		default:            assert(0);              return;		
	}		
	


	/* if ethernet update the mac addresses and ethertype */
	if ( filter_p->layer2_type == kEthernet )
	{
		dsm_update_filter_byte (value, ((filter_p->layer2.ethernet.ethertype.type >> 8) & 0x00FF), 14, DSM_SET);
		dsm_update_filter_byte (value, ((filter_p->layer2.ethernet.ethertype.type     ) & 0x00FF), 15, DSM_SET);
		dsm_update_filter_byte (mask,  ((filter_p->layer2.ethernet.ethertype.mask >> 8) & 0x00FF), 14, DSM_SET);
		dsm_update_filter_byte (mask,  ((filter_p->layer2.ethernet.ethertype.mask     ) & 0x00FF), 15, DSM_SET);

		for (i=0; i<6; i++)
		{
			dsm_update_filter_byte (value, filter_p->layer2.ethernet.dst_mac.mac[i],  2 + i, DSM_SET);
			dsm_update_filter_byte (mask,  filter_p->layer2.ethernet.dst_mac.mask[i], 2 + i, DSM_SET);
			
			dsm_update_filter_byte (value, filter_p->layer2.ethernet.src_mac.mac[i],  8 + i, DSM_SET);
			dsm_update_filter_byte (mask,  filter_p->layer2.ethernet.src_mac.mask[i], 8 + i, DSM_SET);
		}
	}
	
	/* if sonet update the hdlc header */
	else if ( filter_p->layer2_type == kPoS )
	{
		for (i=0; i<4; i++)
		{
			dsm_update_filter_byte (value, filter_p->layer2.pos.hdlc.hdlc[i], i, DSM_SET);
			dsm_update_filter_byte (mask,  filter_p->layer2.pos.hdlc.mask[i], i, DSM_SET);
		}
	}
	
	/* if VLAN we should set the VLAN ethertype and move the user supplied ethertype into the VLAN header */
	else if ( filter_p->layer2_type == kEthernetVlan )
	{
		dsm_update_filter_byte (value, 0x81, 14, DSM_SET);
		dsm_update_filter_byte (value, 0x00, 15, DSM_SET);
		dsm_update_filter_byte (mask,  0xFF, 14, DSM_SET);
		dsm_update_filter_byte (mask,  0xFF, 15, DSM_SET);
		
		dsm_update_filter_byte (value, (uint8_t) ((filter_p->layer2.ethernet.vlan_id.id   >> 8) & 0x0F), 16, DSM_SET);
		dsm_update_filter_byte (value, (uint8_t) ((filter_p->layer2.ethernet.vlan_id.id       ) & 0xFF), 17, DSM_SET);
		dsm_update_filter_byte (mask,  (uint8_t) ((filter_p->layer2.ethernet.vlan_id.mask >> 8) & 0x0F), 16, DSM_SET);
		dsm_update_filter_byte (mask,  (uint8_t) ((filter_p->layer2.ethernet.vlan_id.mask     ) & 0xFF), 17, DSM_SET);

		dsm_update_filter_byte (value, (uint8_t) ((filter_p->layer2.ethernet.ethertype.type >> 8) & 0x00FF), 18, DSM_SET);
		dsm_update_filter_byte (value, (uint8_t) ((filter_p->layer2.ethernet.ethertype.type     ) & 0x00FF), 19, DSM_SET);
		dsm_update_filter_byte (mask,  (uint8_t) ((filter_p->layer2.ethernet.ethertype.mask >> 8) & 0x00FF), 18, DSM_SET);
		dsm_update_filter_byte (mask,  (uint8_t) ((filter_p->layer2.ethernet.ethertype.mask     ) & 0x00FF), 19, DSM_SET);
		
		for (i=0; i<6; i++)
		{
			dsm_update_filter_byte (value, filter_p->layer2.ethernet.dst_mac.mac[i],  2 + i, DSM_SET);
			dsm_update_filter_byte (mask,  filter_p->layer2.ethernet.dst_mac.mask[i], 2 + i, DSM_SET);
			
			dsm_update_filter_byte (value, filter_p->layer2.ethernet.src_mac.mac[i],  8 + i, DSM_SET);
			dsm_update_filter_byte (mask,  filter_p->layer2.ethernet.src_mac.mask[i], 8 + i, DSM_SET);
		}
	}
	


	
	/* check if ipv4 and if so add lower level details */
	if ( filter_p->layer3_type == kIPv4 )
		dsm_gen_ipv4_filter_values (filter_p, value, mask, layer3_offset);

	
	
	/* mask out bytes that are beyond the early termination value */
	//for (i=((filter_p->term_depth + 1) * 8); i<MATCH_DEPTH; i++)
	//	mask[i] = 0x00;
	
		
	/* do a final clean to mask out any values that would never occur */
	for (i=0; i<MATCH_DEPTH; i++)
		value[i] &= mask[i];
	
	
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_read_filter
 
 DESCRIPTION:   Reads a filter back from the card.
 
 PARAMETERS:    base_reg        IN      Pointer to the base register for
                                        the DSM.
                filter          IN      The filter number to read back
                term_depth      OUT     Pointer to a varaible that receives
                                        the early termination element, this
                                        is the earliest element in the filter
                                        that has the early termination bit set.
                value & mask    OUT     The actual filter data read from
                                        the card. The size of this buffer
                                        is expected to be MATCH_DEPTH
                                        bytes.

 RETURNS:       0 if the filter was read otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_read_filter (volatile uint8_t * base_reg, uint32_t filter, uint32_t * term_depth, uint8_t value[MATCH_DEPTH], uint8_t mask[MATCH_DEPTH])
{
	uint32_t     cmd;
	uint32_t     data;
	uint32_t     i, j;
	uint32_t     index;
	uint32_t     elements;
	uint32_t     lowest_term;
	
	
	/* sanity checking */
	if ( (filter >= FILTER_COUNT) || (value == NULL) || (mask == NULL) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	memset (value, 0, MATCH_DEPTH);
	memset (mask, 0, MATCH_DEPTH);
	
	
	/* setup */
	elements = MATCH_DEPTH / ELEMENT_SIZE;
	lowest_term = elements;
	
	
	/* read the comparand */
	for (i=0; i<elements; i++)
		for (j=0; j<4; j++)
		{
			cmd  = (filter << 22) | (i << 18) | (j << 16);
			dag_writel (cmd, (base_reg + DSM_REG_FILTER_OFFSET));
			
			dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
			
			data = dag_readl (base_reg + DSM_REG_FILTER_OFFSET);
			index = (i*8) + ((3-j) * 2);
			value[index + 0] = (uint8_t) ((data >> 8) & 0x000000FF);
			value[index + 1] = (uint8_t) ((data     ) & 0x000000FF);

			dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
		}
		

	/* read the mask */
	for (i=0; i<elements; i++)
		for (j=0; j<4; j++)
		{
			cmd  = (filter << 22) | (i << 18) | (j << 16);
			cmd |= BIT25;
			dag_writel (cmd, (base_reg + DSM_REG_FILTER_OFFSET));
			
			dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
			
			data = dag_readl (base_reg + DSM_REG_FILTER_OFFSET);
			index = (i*8) + ((3-j) * 2);
			mask[index + 0] = (uint8_t) ((data >>  8) & 0x000000FF);
			mask[index + 1] = (uint8_t) ((data      ) & 0x000000FF);
			
			if ( (data & BIT30) && (i < lowest_term) )
				lowest_term = i;

			dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
		}


	
	/* check if the user wants the early termination flag */
	if ( term_depth != NULL )
		*term_depth = lowest_term;

	
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_write_filter
 
 DESCRIPTION:   Writes a filter to the card. This function assumes the
                filter is already disabled. If the filter is not disabled
                this function will succeeded, however packets will not
                be filtered correctly.
 
 PARAMETERS:    base_reg        IN      Pointer to the base register for
                                        the DSM.
                filter          IN      The filter number to read back
                term_depth      IN      The element to early terminate, if
                                        this value greater than 7 it is trimmed
                                        back down to zero.
                value & mask    IN      The actual filter data read from
                                        the card. The size of this buffer
                                        is expected to be MATCH_DEPTH
                                        bytes.

 RETURNS:       0 if the filter was read otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_write_filter (volatile uint8_t * base_reg, uint32_t filter, uint32_t term_depth, const uint8_t * value, const uint8_t * mask)
{
	uint32_t     cmd;
	uint32_t     i;
	uint32_t     j;
	uint32_t     elements;
	
	
	/* sanity checking */
	if ( (filter >= FILTER_COUNT) || (value == NULL) || (mask == NULL) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	

	/* calculate the number of 16-bit writes we have to do */
	elements = MATCH_DEPTH / ELEMENT_SIZE;

	
	/* trim the termination depth */
	if ( term_depth > LAST_FILTER_ELEMENT )
		term_depth = LAST_FILTER_ELEMENT;
	

	/* write the comparand */
	for (i=0; i<elements; i++)
		for (j=0; j<4; j++)
		{
			cmd  = (1 << 31) | (filter << 22) | (i << 18) | (j << 16);
			cmd |= (uint32_t)value[(i*8) + ((3-j) * 2) + 0] << 8;
			cmd |= (uint32_t)value[(i*8) + ((3-j) * 2) + 1];

			dag_writel (cmd, (base_reg + DSM_REG_FILTER_OFFSET));
	
			dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
		}


	/* write the mask */
	for (i=0; i<elements; i++)
		for (j=0; j<4; j++)
		{
			cmd  = (1 << 31) | BIT25 | (filter << 22) | (i << 18) | (j << 16);

			if ( i >= term_depth )
				cmd |= BIT30;

			cmd |= (uint32_t)mask[(i*8) + ((3-j) * 2) + 0] << 8;
			cmd |= (uint32_t)mask[(i*8) + ((3-j) * 2) + 1];
	
			dag_writel (cmd, (base_reg + DSM_REG_FILTER_OFFSET));
			
			dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
		}

	
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_enable_filter
 
 DESCRIPTION:   Enables or disables a filter.
 
 PARAMETERS:    base_reg        IN      Pointer to the base register for
                                        the DSM.
                filter          IN      The filter number to read back
                enable          IN      0 to disable or 1 to enable

 RETURNS:       0 if the filter was enabled/disabled otherwise -1

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_enable_filter (volatile uint8_t * base_reg, uint32_t filter, uint32_t enable)
{
	uint32_t  csr_value;
	
	
	/* sanity checking */
	assert(filter < FILTER_COUNT);
	if ( filter >= FILTER_COUNT )
		return -1;


	/* read the CSR and update */
	csr_value = dag_readl (base_reg + DSM_REG_CSR_OFFSET);
		
	dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));

	if ( enable == 0 )
		csr_value &= ~(1 << filter);
	else
		csr_value |=  (1 << filter);
		
	dag_writel (csr_value, (base_reg + DSM_REG_CSR_OFFSET));
	
	dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));

	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_is_filter_enabled
 
 DESCRIPTION:   Indicates whether a filter is currently enabled on the
                card.
 
 PARAMETERS:    base_reg        IN      Pointer to the base register for
                                        the DSM.
                filter          IN      The filter number to check

 RETURNS:       0 if the filter is disabled, 1 if the filter is enabled.
                -1 will be returned if an invalid filter number was supplied.

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_is_filter_enabled (volatile uint8_t * base_reg, uint32_t filter)
{
	uint32_t  csr_value;

	/* sanity checking */
	assert(filter < FILTER_COUNT);
	if ( filter >= FILTER_COUNT )
		return -1;
	
	
	/* read the CSR */
	csr_value = dag_readl (base_reg + DSM_REG_CSR_OFFSET);
		
	dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));

	if ( (csr_value & (1 << filter)) == 0 )
		return 0;
	else
		return 1;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_actual_to_virtual
 
 DESCRIPTION:   Takes a bitmasked value containing the actual filters
                and converts it to another bitmasked value with the
                equivalent virtual values bitmasked to be past through
                to dagdsm_compute_output_expr_value.
 
 PARAMETERS:    config_p        IN      Pointer to the configuration
                                        information.
                filters         IN      Bitmasked actual filter values

 RETURNS:       The virtual filters bitmasked

 HISTORY:       

---------------------------------------------------------------------*/
static uint32_t
dsm_actual_to_virtual (dsm_config_t * config_p, uint32_t filters)
{
	uint32_t output;
	uint32_t i;
	
	assert(config_p != NULL);
	output = 0x00000000;
	
	for (i=0; i<FILTER_COUNT; i++)
		if ( (filters & (1 << config_p->filters[i].actual_filter_num)) != 0 )
			output |= (1 << i);
	
		
	return output;	
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_construct_colur
 
 DESCRIPTION:   Constructs the colour lookup table and puts it in the
                supplied memory buffer.
 
 PARAMETERS:    config_p        IN      Pointer to the configuration to
                                        build the table for.
                table           OUT     Buffer to store the table in.
                size            IN      The size of the buffer in 
                                        halfwords (16-bit).

 RETURNS:       0 if the filter was read otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 NOTES:         The colour lookup table is a map of a 12-bit number (made
                up of 2 HLB bits, 2 IFACE bits and 8 filter bits) to a
                n bit number. Where n is the log2 of the number of receive
                streams plus one.
                
                       : hlb1 hlb0  if1 if0    f7 f6 f5 f4 f3 f2 f1 f0 : bm
                ------------------------------------------------------------
                0x000  :  0    0    0   0      0  0  0  0  0  0  0  0 : 1
                0x001  :  0    0    0   0      0  0  0  0  0  0  0  1 : 0
                ...
                0x221  :  0    0    1   0      0  0  1  0  0  0  0  1 : drop
                
                in the above table if no filters hit and both the interface
                and HLB are zero, then the packet will go to burst
                manager 1 (receive stream 2). If the same situation, but instead
                filter 0 hit, then the packet goes to burst manager 0
                (receive stream 0). In the last example if a packet hits
                on filter 5 & 0 and HLB1 is 1 then the packet is dropped.
                
                The actual table that is sent to the FPGA is constructed
                like this:
                
                         |<--------- 16 bits ---------->|
                         
                        15                              0 
                         +-----+-----+-----+  ... +-----+
                0x0000   | 007 | 006 | 005 |      | 000 | 
                         +-----+-----+-----+  ... +-----+
                0x0002   | 00F | 00E | 00D |      | 008 | 
                         +-----+-----+-----+  ... +-----+
                         .                              
                         .                              
                         .                              
                         +-----+-----+-----+  ... +-----+
                0x03FE   | FFF | FFE | FFD |      | FF8 | 
                         +-----+-----+-----+  ... +-----+
                
                where each entry is n bits wide (in the current case 2 bits wide),
                the numbers in the diagram indicate which table entry they
                match in the table further above. The MSB of each entry is
                set if the packet is to be dropped, the LSB indicates which
                burst manager it goes to 0 or 1 (if not dropped). In the
                above table/diagram there were only 2 burst managers, but if
                there were more, the number of bits to encode each entry 
                will grow to log(n) + 1 were n is the number of burst managers.
                
                
 NOTES:         The actual filter numbers specified by the user are not
                necessarly the same as the filter number in the firmware,
                this is due to the hot-swapping of filters. To compensate
                for this the actual filter numbers are swapped with the
                'virtual' filter numbers when calling dagdsm_compute_output_expr_value()

 SPECIAL		In DSM V2 (special version for dag4.5Z and Dag8.2Z), the 2 hlb bits is 
 NOTES			replaced by stream control bits. The bits are coming from the hlb stream 
 FOR DSM_V2:    control HAT results, 2 bits for 4.5Z and 3 bits for 8.2Z. 
				To keep the input as 12 bits, for 8.2Z the interface bits is reduced to 
				only 1 bit.
				Also this version the size table entry will be 4 bits. So the size of the
				table will be doubled.
                
 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_construct_colur (dsm_config_t * config_p, uint16_t * table, uint32_t size)
{
	int       res = -1;
	uint32_t  range = 0;
	uint32_t  filters;
	uint8_t   iface = 0;
	uint32_t   hlb_hash_value = 0;
	uint8_t   hlb0 = 0, hlb1 = 0;
	uint32_t  stream;
	uint32_t  i;
	uint32_t  j;
	uint16_t  entry;
	uint16_t  row_entry;
	uint32_t  row_offset;
	uint32_t  tbl_index;
	
		
	/* calculate the number of table entries */
	range = (1 << (DSM_FILTER_BITS + DSM_INTERFACE_BITS + DSM_HLB_BITS));

	/* check the size is large enough to hold the complete table */
	if ( (size < ((range * config_p->bits_per_entry) / 16)) || (table == NULL) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* setup */
	row_entry  = 0x0000;
	row_offset = 0;
	tbl_index  = 0;

	
	/* go through all possible combinations of filters, interfaces and HLBs */
	for (i=0; i<range; i++)
	{
		filters = DSM_COLUR_FILTER(i);
		if(config_p->dsm_version==DSM_VERSION_0)
		{
			iface   = (uint8_t)DSM_COLUR_IFACE(i);
			hlb0    = (uint8_t)DSM_COLUR_HLB0(i);
			hlb1    = (uint8_t)DSM_COLUR_HLB1(i);
		}
		else if(config_p->dsm_version==DSM_VERSION_1)
		{
			if(config_p->device_code==PCI_DEVICE_ID_DAG4_52Z)
			{
				iface   = (uint8_t)DSM_COLUR_IFACE_V_1_4(i);
				hlb_hash_value= DSM_COLUR_HLB_HASH_V_1_4(i);
			}
			else if((config_p->device_code==PCI_DEVICE_ID_DAG4_52Z8) || (config_p->device_code==PCI_DEVICE_ID_DAG8_20Z) || (config_p->device_code==PCI_DEVICE_ID_DAG5_0Z ) || (config_p->device_code==PCI_DEVICE_ID_DAG5_00S ))
			{
				iface   = (uint8_t)DSM_COLUR_IFACE_V_1_8(i);
				hlb_hash_value= DSM_COLUR_HLB_HASH_V_1_8(i);
			}
		}		
		/* convert the actual filter numbers to virtual filter numbers */
		filters = dsm_actual_to_virtual (config_p, filters);
		
		
		/* find the first output expression that matches */
		stream = 0xFFFFFFFF;
		for (j=0; j<config_p->rx_streams; j++)
		{
			if(config_p->dsm_version==DSM_VERSION_0)
			{
				res = dagdsm_compute_output_expr_value ((DsmOutputExpH)(&config_p->output_expr[j]), filters, iface, hlb0, hlb1);	
			}
			else if(config_p->dsm_version==DSM_VERSION_1)
			{
				res = dagdsm_compute_output_expr_value_z ((DsmOutputExpH)(&config_p->output_expr[j]), filters, iface, hlb_hash_value);	
			}
			if ( res == 1 )
			{
				stream = j;
				break;
			}
		}
	
		
		/* determine where to steer or just drop */
		if ( stream != 0xFFFFFFFF )
			entry = (uint16_t) (stream & ((1 << (config_p->bits_per_entry - 1)) - 1));
		else
			entry = (uint16_t) (1 << (config_p->bits_per_entry - 1));
		
		
		/* add the entry to the table */
		row_entry  |= (entry << row_offset);
		row_offset += config_p->bits_per_entry;
		
		
		/* if we have filled up a row, copy it to the table */
		if ( row_offset > (16 - config_p->bits_per_entry) )
		{
			table[tbl_index++] = row_entry;
			row_offset = 0;
			row_entry  = 0x0000;
		}
	}
		
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_read_colur
 
 DESCRIPTION:   Reads the colur information from the specfied bank.
 
 PARAMETERS:    config_p        IN      Pointer to the configuration
                                        object.
                base_reg        IN      Pointer to the base register for
                                        the DSM.
                bank            IN      The bank to read from, this must
                                        be either 0 or 1.
                table           OUT     The buffer holding the table
                                        to download to the card.
                size            IN      The size of the table in the
                                        buffer, in 16-bit words.

 RETURNS:       0 if the COLUR was read otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_read_colur (dsm_config_t * config_p, volatile uint8_t * base_reg, uint32_t bank, uint16_t * table, uint32_t size)
{
	uint32_t  i;
	uint32_t  cmd;
	uint32_t  data;
	
	
	/* check to make sure the size is correct */
	if ( (size == 0) || (table == NULL) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/*  */
	memset (table, 0, size);
	
	
	/* make sure the bank is either 0 or 1 */
	bank &= 0x0001;
	if(config_p->dsm_version==DSM_VERSION_0)
		bank = bank<<25;
	else
		bank = bank<<26;
	
	
	/* write the colur data */
	for (i=0; i<size; i++)
	{
		cmd = bank | (i << 16);
		dag_writel (cmd, (base_reg + DSM_REG_COLUR_OFFSET));

		dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
		
		data = dag_readl (base_reg + DSM_REG_COLUR_OFFSET);
		table[i] = (uint16_t)(data & 0xFFFF);
		
		dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
	}
	
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_write_colur
 
 DESCRIPTION:   Writes the colur information into the specfied bank.
 
 PARAMETERS:    config_p        IN      Pointer to the configuration
                                        object.
                base_reg        IN      Pointer to the base register for
                                        the DSM.
                bank            IN      The bank to write to, this must
                                        be either 0 or 1.
                table           IN      The buffer holding the table
                                        to download to the card.
                size            IN      The size of the table in the
                                        buffer, in 16-bit words.

 RETURNS:       0 if the COLUR was written otherwise -1, this function
                updates the last error code using dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dsm_write_colur (dsm_config_t * config_p, volatile uint8_t * base_reg, uint32_t bank, uint16_t * table, uint32_t size)
{
	uint32_t  i;
	uint32_t  cmd;
	
	/* calculate how many rows we should have */
	//rows = ((1 << (DSM_FILTER_BITS + DSM_INTERFACE_BITS + DSM_HLB_BITS)) * config_p->bits_per_entry) / 16;
	//assert (rows == 512);
	
	
	/* check to make sure the size is correct */
	if ( (size == 0) || (table == NULL) )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* make sure the bank is either 0 or 1 */
	bank &= 0x0001;

	if(config_p->dsm_version==DSM_VERSION_0)
		bank = bank<<25;
	else
		bank = bank<<26;
	
	/* write the colur data */
	for (i=0; i<size; i++)
	{
		cmd  = (1 << 31) | bank | (i << 16);
		cmd |= table[i];
		dag_writel (cmd, (base_reg + DSM_REG_COLUR_OFFSET));

		dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
	}
	
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_get_inactive_colur_bank
 
 DESCRIPTION:   Returns the current inactive bank of the colour lookup
                table.
 
 PARAMETERS:    base_reg        IN      Pointer to the base register for
                                        the DSM.

 RETURNS:       the inactive bank, with be either 0 or 1.

 HISTORY:       

---------------------------------------------------------------------*/
uint32_t
dsm_get_inactive_colur_bank (volatile uint8_t * base_reg)
{
	uint32_t   csr_value;
	
	csr_value = dag_readl (base_reg + DSM_REG_CSR_OFFSET);
	
	if ( csr_value & 0x40000000 )
		return 0;
	else
		return 1;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dsm_set_active_colur_bank
 
 DESCRIPTION:   Sets the active bank to the specified one
 
 PARAMETERS:    base_reg        IN      Pointer to the base register for
                                        the DSM.
                bank            IN      The bank to make the active one.

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
void
dsm_set_active_colur_bank (volatile uint8_t * base_reg, uint32_t bank)
{
	uint32_t   csr_value;
	
	csr_value = dag_readl (base_reg + DSM_REG_CSR_OFFSET);
	dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));

	if ( bank == 0 )
		csr_value &= ~BIT30;
	else
		csr_value |=  BIT30;
	
	dag_writel (csr_value, (base_reg + DSM_REG_CSR_OFFSET));
	dsm_drb_delay ((volatile uint32_t*)(base_reg + DSM_REG_FILTER_OFFSET));
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_load_configuration
 
 DESCRIPTION:   Downloads a configuration to a card. It starts by reading
                back the filters from the card and seeing what values
                have changed ... to be completed
 
 PARAMETERS:    config_h        IN      Handle to a configuration

 RETURNS:       0 if the configuration was loaded to the card otherwise -1,
                this function updates the last error code using
                dagdsm_set_last_error()

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_load_configuration (DsmConfigH config_h)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;
	uint16_t         * table_p;
	uint32_t           table_size;
	uint8_t            card_value[MATCH_DEPTH];
	uint8_t            card_mask[MATCH_DEPTH];
	uint8_t            filter_value[MATCH_DEPTH];
	uint8_t            filter_mask[MATCH_DEPTH];
	volatile uint8_t * dsm_base_reg;
	uint32_t           inactive_bank;
	uint32_t           sw_term_depth;
	uint32_t           hw_term_depth;
	int                is_enabled;
	uint32_t           i;
	

	/* sanity checking */
	if ( config_p == NULL || config_p->signature != DSM_SIG_CONFIG )
	{
		dagdsm_set_last_error (EINVAL);
		return -1;
	}


	/* allocate memory for the table */
	/* for dsm v0, v1.4 v1.8, the count of table entris are the same, 12bits, only bits_per_entry is different*/
	table_size = ((1 << (DSM_FILTER_BITS + DSM_INTERFACE_BITS + DSM_HLB_BITS)) * config_p->bits_per_entry) / 16;
	
	table_p = (uint16_t*) malloc(table_size * 2);
	if ( table_p == NULL )
	{
		dagdsm_set_last_error (ENOMEM);
		return -1;
	}
	
	

	/* get a pointer to the base register used for DSM */
	dsm_base_reg = config_p->base_reg;
	
	
	
	/* read the filters and determine which ones have changed, the ones that
	 * have changed are disable, updated and then re-enabled */
	for (i=0; i<FILTER_COUNT; i++)
	{
		
		/* first check if the filter is enabled on the card */
		is_enabled = dsm_is_filter_enabled(dsm_base_reg, i);
		
		
		/* if the filter is enabled on the card but it shouldn't be, disable it now */
		if ( (is_enabled == 1) && (config_p->filters[i].enabled == 0) )
			dsm_enable_filter (dsm_base_reg, i, 0);

		
		/* should be disabled go no further */
		if ( config_p->filters[i].enabled == 0 )
			continue;
		
		
		/* get the early termination depth */
		sw_term_depth = config_p->filters[i].term_depth;
		
		
		/* create the mask and value from the settings in the filter */
		dsm_gen_filter_values (&config_p->filters[i], filter_value, filter_mask);		
		
		/* check if the filter has changed */
		dsm_read_filter (dsm_base_reg, i, &hw_term_depth, card_value, card_mask);
		
		if ( (memcmp(card_value, filter_value, MATCH_DEPTH) != 0) ||
		     (memcmp(card_mask,  filter_mask,  MATCH_DEPTH) != 0) ||
		     (hw_term_depth != sw_term_depth) )
		{

			/* update the filter by disabling it first, change it and then enable */
			
			dsm_enable_filter (dsm_base_reg, i, 0);
			
			dsm_write_filter (dsm_base_reg, i, sw_term_depth, filter_value, filter_mask);
			
			dsm_enable_filter (dsm_base_reg, i, 1);
		}
		
		/* check if the filter is currently disabled when if should be enabled */
		else if ( is_enabled == 0 )
		{
			dsm_enable_filter (dsm_base_reg, i, 1);
		}
			
	}
	
	
	

	/* construct the colour lookup table */
	dsm_construct_colur (config_p, table_p, table_size);

	
	/* get the active bank of the COLUR amd then download the current table to the inactive bank */
	inactive_bank = dsm_get_inactive_colur_bank(dsm_base_reg);
	dsm_write_colur (config_p, dsm_base_reg, inactive_bank, table_p, table_size);

	
	/* switch the banks */
	dsm_set_active_colur_bank (dsm_base_reg, inactive_bank);


	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagdsm_do_swap_filter
 
 DESCRIPTION:   Replaces the filter on the card with the swap filter,
                this function also updates the COLUR at the same time
                so that the output of the filter is consistant.
 
 PARAMETERS:    config_h        IN      Handle to a configuration
                filter          IN      The number of the filter to replace
                                        with the swap filter.

 RETURNS:       0 if the configuration was loaded to the card otherwise -1,
                this function updates the last error code using
                dagdsm_set_last_error()

 NOTES:         The process for doing a filter swap is as follows:

                1. Update the COLUR (construct, download and activate)
                2. Install the swap filter on the card and enable. The
                   COLUR above will have made all the entries that relate
                   to the newly installed swap filter as "don't cares".
                3. Mark the filter we are suppose to be replacing as
                   the new swap filter.
                4. Update the COLUR (construct, download and activate)
                5. Disable the replaced filter on the card (this is the
                   new swap filter).

 HISTORY:       

---------------------------------------------------------------------*/
int
dagdsm_do_swap_filter(DsmConfigH config_h, uint32_t filter)
{
	dsm_config_t     * config_p = (dsm_config_t*)config_h;
	uint16_t         * table_p;
	uint32_t           table_size;
	volatile uint8_t * dsm_base_reg;
	uint32_t           inactive_bank;
	uint32_t           actual_filter_to_replace;
	uint32_t           actual_swap_filter;
	uint32_t           term_depth;
	uint8_t            value[MATCH_DEPTH];
	uint8_t            mask[MATCH_DEPTH];
	
	
	
	
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
	
	
	/* get the actual filter number of the one we are replacing and the actual number of the swap filter */
	actual_swap_filter = config_p->filters[DSM_SWAP_FILTER_NUM].actual_filter_num;
	actual_filter_to_replace = config_p->filters[filter].actual_filter_num;
	
	term_depth = config_p->filters[DSM_SWAP_FILTER_NUM].term_depth;
	
	

	/* Step 1 (this step shouldn't be needed but it won't hurt) */
	
	dsm_construct_colur (config_p, table_p, table_size);

	inactive_bank = dsm_get_inactive_colur_bank(dsm_base_reg);
	dsm_write_colur (config_p, dsm_base_reg, inactive_bank, table_p, table_size);

	dsm_set_active_colur_bank (dsm_base_reg, inactive_bank);

	
	
	/* Step 2 */
	
	dsm_enable_filter (dsm_base_reg, actual_swap_filter, 0);
	
	dsm_gen_filter_values (&(config_p->filters[DSM_SWAP_FILTER_NUM]), value, mask);
	
	dsm_write_filter (dsm_base_reg, actual_swap_filter, term_depth, value, mask);
			
	dsm_enable_filter (dsm_base_reg, actual_swap_filter, 1);




	/* Step 3 */

	config_p->filters[DSM_SWAP_FILTER_NUM].actual_filter_num = actual_filter_to_replace;
	config_p->filters[filter].actual_filter_num = actual_swap_filter;
	
	
	
	/* Step 4 */
	
	dsm_construct_colur (config_p, table_p, table_size);

	inactive_bank = dsm_get_inactive_colur_bank(dsm_base_reg);
	dsm_write_colur (config_p, dsm_base_reg, inactive_bank, table_p, table_size);

	dsm_set_active_colur_bank (dsm_base_reg, inactive_bank);



	/* Step 5 */

	dsm_enable_filter (dsm_base_reg, actual_filter_to_replace, 0);





	/* copy the swap filter values to the filter we just replaced when the user 
	   queries the filter it has the same values as the swap filter.            */
	dagdsm_filter_copy ( (DsmFilterH)(&(config_p->filters[filter])), 
	                     (DsmFilterH)(&(config_p->filters[DSM_SWAP_FILTER_NUM])) );


	return 0;
}
