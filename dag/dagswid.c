/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagswid.c,v 1.7 2009/04/02 03:13:55 wilson.zhu Exp $
 */

/* File header. */
#include "dagswid.h"

/* Endace headers. */
#include "dagapi.h"
#include "d37t_i2c.h"
#include "dag37t_api.h"
#include "dag_romutil.h"
#include "dagutil.h"




/* dag_write_software_id : Writes a software ID to a dag card. 
 * Args :   dagfd - valid dag file desciptor, returned from dag_open().
 *          num_bytes - number of bytes to be written to the dag card.
 *          *datap - pointer to the byte array holding the dag SWID.
 *          key - software key to check the write process against.
 * Returns  0   - Function was successfull
 *          -1  - Invalid number of bytes specified 
 *          -2  - Firmware error writing to the dag card.
 *          -3  - Timeout communicating with the XScale
 *          -4  - Wrong key.
 */
int
dag_write_software_id(int dagfd, uint32_t num_bytes, uint8_t* datap, uint32_t iswidkey)
{
	daginf_t* dinf;
	int device_code;
	romtab_t* rp; /* For talking to the dagrom SWID functions */

	dinf = (daginf_t *) dag_info(dagfd);
	device_code = dinf->device_code;

	switch (device_code)
	{
		case 0x430e: /* DAG4.3ge */
		case 0x378e: /* DAG3.7g */
		case 0x3707: /* DAG3.7T */
		case 0x3747: /* DAG3.7T4 */
		case 0x7100: /* DAG7.1s */
			if (num_bytes > DAGSERIAL_SIZE)
			{
				return -1;
			}
			
			rp = (romtab_t*) dagrom_init(dagfd, 0, 0);
			
			/* Check if we have a valid SWID, if we do then perform a SWID key check. 
			 * If this fails, then the operation is deemed invalid. Otherwise write 
			 * the SWID to the dag card.
			 */
			if (dagswid_isswid(rp) < 0)
			{
				dagswid_erase(rp);
				
				/* dagswid_write will write the supplied SWID key to the ROM */
				if (dagswid_write(rp, datap, num_bytes, iswidkey) < 0)
				{
					return -2;
				}
			}
			else
			{
				if (dagswid_checkkey(rp, iswidkey) == 0)
				{
					dagswid_erase(rp);
					if (dagswid_write(rp, datap, num_bytes, iswidkey) < 0)
					{
						return -2;
					}
				}
				else
				{
					return -4;
				}
			}
		
			dagrom_free(rp);
			break;

		
		default:
			return -2;
	}
	
	return 0;
}


/* dag_read_software_id : Reads a software ID from the dag card.
 * Args :   dagfd - valid dag file descriptor, returned from dag_open().
 *          num_bytes - number of bytes to be written to the dag card.
 *          *datap - pointer the byte array holding the SWID.
 * Returns  0   - Function was successfull
 *          -1  - Invalid number of bytes specified
 *          -2  - Firmware error reading from the dag card.
 *          -3  - Timeout communicating with the XScale.
 */
int
dag_read_software_id(int dagfd, uint32_t num_bytes, uint8_t * datap)
{
	daginf_t *dinf;
	int device_code;
	romtab_t* rp; /* For talking to the dagrom SWID functions */

	dinf = (daginf_t *) dag_info(dagfd);
	device_code = dinf->device_code;

	switch (device_code)
	{
		case 0x430e: /* DAG4.3ge */
		case 0x378e: /* DAG3.7g */
		case 0x3707: /* DAG3.7t */
		case 0x3747: /* DAG3.7t4 */
		case 0x7100: /* DAG7.1s */
			if (num_bytes > DAGSERIAL_SIZE)
			{
				return -1;
			}
			
			rp = (romtab_t*) dagrom_init(dagfd, 0, 0);
			if (dagswid_read(rp, datap, num_bytes) < 0)
			{
				dagutil_verbose("SWID dagswid_read : failed to read ROM");
				return -2;
			}
			
			dagrom_free(rp);
			break;
		




			
		default:
			return -2;
	}
	
	return 0;
}
