/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagcrc.c,v 1.6 2007/12/03 05:13:02 jomi Exp $
 */

/* File header. */
#include "dagcrc.h"

/* C Standard Library headers. */
#include <assert.h>
#include <stdio.h>


/* Ethernet CRC table. */
static uint32_t ethernet_crc_table_r[256];

/* Infiniband VCRC table */
static uint16_t infiniband_vcrc_table_r[256];

/* PPP CRC table. */
static uint16_t ppp_fcs16_table[256];
static uint32_t ppp_fcs32_table[256] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


/* ATM AAL5 CRC table. */
static uint32_t aal5_crc_table[256];

/* CRC-8 from
(*) N. M. Abramson, A Class of Systematic Codes for Non-Independent
Errors, IRE Trans. Info. Theory, December 1959.
30/08/04 extended to also include CRC-10 caclulations.
*/
#define HEC_GENERATOR       0x107               /* x^8 + x^2 +  x  + 1  */
#define COSET_LEADER        0x055               /* x^6 + x^4 + x^2 + 1  */
#define CRC10_POLYNOMIAL	0x633				/* x^10 + x^9 + x^5 + x^4 + x + 1 */
#define CRC_TABLE_SIZE		256
#define MASK_ALL_BITS		0xff
static unsigned char crc8_table[CRC_TABLE_SIZE];
static unsigned short crc10_table[CRC_TABLE_SIZE];

/* Function:     generate_crc8_crc10_table()
 * Description:  generate a table of CRC-8 and CRC-10 syndromes for all 
 *				 possible input bytes
 * Outputs:		completed crc 8 and 10 tables
 * Globals:		crc8_table	- table of crc8 syndromes
 *				crc10_table - table of crc10 syndromes
 */
void generate_crc8_crc10_table()
/* generate a table of CRC-8 and CRC-10 syndromes for all possible input bytes */
{
	int i, j, temp;
	unsigned short crc10;
	
	for (i = 0;  i < CRC_TABLE_SIZE;  i++)
	{
		temp = i;
		crc10 = ((unsigned short)i << 2);
		
		for (j = 0;  j < 8;  j++)
		{
			if (temp & 0x80)
			{
				temp = (temp << 1) ^ HEC_GENERATOR;
			}
			else
			{
				temp = (temp << 1);
			}
			
			if ((crc10 <<= 1) & 0x400)
			{
				crc10 ^= CRC10_POLYNOMIAL;
			}
		}
		crc8_table[i] = (unsigned char)temp;
		crc10_table[i] = crc10;
	}
}

/* Function:     insert_hec(frame_count)
 * Description:  calculate CRC-8 remainder over first four bytes of cell 
 *				 header exclusive-or with coset leader & insert into 
 *				 fifth header byte
 * Inputs:       frame_count - position of cells in the list of cells
 * Outputs:      calculated crc8 of the given cells header 
 * Returns:
 * Globals:		 atm_frame	- list of atm cells
 *				 crc8_table	- table of crc8 syndromes
 * Notes: calculate CRC-8 remainder over first four bytes of cell header
 * exclusive-or with coset leader.
 */
uint8_t 
dagcrc_atm_hec8( uint8_t crc_accum, const char *pdata, int len )
{
	int i;
	
	for (i = 0;  i < len;  i++)
		crc_accum = crc8_table[crc_accum ^ pdata[i]];
	
	return crc_accum ^ COSET_LEADER;
}

/* External API. */
void
dagcrc_make_ethernet_table_r(void)
{
	uint32_t c;
	uint32_t n;
	int k;
	
	for (n = 0; n < 256; n++)
	{
		c =  n;
		for (k = 0; k < 8; k++)
		{
			if (c & 1)
			{
				c = 0xedb88320L ^ (c >> 1);
			}
			else
			{
				c = c >> 1;
			}
		}
		ethernet_crc_table_r[n] = c;
	}
}

uint32_t
dagcrc_ethernet_crc32_r(uint32_t crc, const uint8_t* buf, int len)
{
	uint32_t c = crc ^ 0xffffffffL;
	
	while (len--)
	{
		int t = (c ^ (*buf)) & 0xff;
		
		buf++;
		c = ethernet_crc_table_r[t] ^ (c >> 8);
	}
	
	return c ^ 0xffffffffL;
}


/* PPP 16 and 32-bit CRCs */
#define PPP_MAGIC_NUMBER 0x8408
void
dagcrc_make_ppp_fcs16_table(void)
{
	register uint32_t b, v;
	register int i;
	
	for (b = 0; ; )
	{
		v = b;
		for (i = 8; i--; )
		{
			if (0 != (v & 1))
			{
				v = (v >> 1) ^ PPP_MAGIC_NUMBER;
			}
			else
			{
				v = (v >> 1);
			}
		}
		
		ppp_fcs16_table[b] = (uint16_t)( v & 0xffff);

		b++;
		if (b == 256)
			break;
	}
}


uint16_t
dagcrc_ppp_fcs16(uint16_t fcs, const uint8_t* cp, int len)
{
	assert(sizeof (uint16_t) == 2);
	assert(((uint16_t) -1) > 0);
	
	while (len--)
	{
		fcs = (fcs >> 8) ^ ppp_fcs16_table[(fcs ^ (*cp)) & 0xff];
		cp++;
	}
	
	return (fcs);
}


uint32_t
dagcrc_ppp_fcs32(uint32_t fcs, const uint8_t* cp, int len)
{
	assert(sizeof (uint32_t) == 4);
	assert(((uint32_t) -1) > 0);

	while (len--)
	{
		fcs = ((fcs >> 8) ^ ppp_fcs32_table[(fcs ^ (*cp)) & 0xff]);
		cp++;
	}

	return (fcs);
}


/* ATM AAL5 32-bit CRC */
#define POLYNOMIAL 0x04c11db7L
void
dagcrc_make_aal5_table(void)
{
	register int i;
	register int j;
	register uint32_t crc_accum;

	for (i = 0; i < 256; i++)
	{
		crc_accum = ((uint32_t) i << 24);
		for (j = 0; j < 8; j++)
		{
			if (crc_accum & 0x80000000L)
			{		
				crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
			}
			else
			{
				crc_accum = (crc_accum << 1);
			}
		}
		aal5_crc_table[i] = crc_accum;
	}
}


uint32_t
dagcrc_aal5_crc(uint32_t crc_accum, const char *data_blk_ptr, int data_blk_size)
{
	register int i;
	register int j;

	for (j = 0; j < data_blk_size; j++)
	{
		i = ((int) (crc_accum >> 24) ^ (*data_blk_ptr)) & 0xff;
		data_blk_ptr++;
		crc_accum = (crc_accum << 8) ^ aal5_crc_table[i];
	}
	return crc_accum;
}
void
dagcrc_make_infiniband_vcrc_table_r(void)
{
    uint16_t c;
    uint16_t n;
    int k;

    for (n = 0; n < 256; n++)
    {
        c =  n;
        for (k = 0; k < 8; k++)
        {
            if (c & 1)
            {
                c = 0xd008^ (c >> 1);
            }
            else
            {
                c = c >> 1;
            }
        }
        infiniband_vcrc_table_r[n] = c;
    }
}

uint16_t
dagcrc_infiniband_vcrc16_r(uint32_t crc, const uint8_t* buf, int len)
{
    uint16_t c = crc ^ 0xffff;

    while (len--)
    {

        int t = (c   ^ (*buf)) & 0xff;
        buf++;
        c = infiniband_vcrc_table_r[t] ^ (c >> 8);
    }

    return c ^ 0xffff;
}
/* Function:     dagcrc_infiniband_icrc32
 * Description:  calculate ICRC(32 bits ) for the infiniband packet
 * Inputs:       	crc:- Initial value
*			buf1 - pointer to masked buffer (header)
*			len1 - length of the buf1
*			buf2 - pointer rest of the packet
*			len2 - length of the buf2
* Outputs:      calculated crc32 of the given packet 
 */
uint32_t
dagcrc_infiniband_icrc32(uint32_t crc, const uint8_t* buf1, int len1, const uint8_t* buf2, int len2)
{
	uint32_t c = crc ^ 0xffffffffL;
	int i = 0;
	const uint8_t *buf = buf1;
	while (i < (len1+ len2) )
	{
		int t = (c ^ (*buf)) & 0xff;
		c = ethernet_crc_table_r[t] ^ (c >> 8);
		i++;
		buf++;
		if ( i == len1) buf = buf2;
	}
	
	return c ^ 0xffffffffL;
}
