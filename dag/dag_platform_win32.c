/* File header. */
#include "dag_platform_win32.h"

#if defined(_WIN32)

uint16_t
ip_sum_calc_win32(uint8_t* buff)
{
	uint16_t word16;
	uint32_t sum = 0;
	uint16_t i;
	uint16_t header_length;
	iphdr * ip_hdr = (iphdr*) buff;

	// NOTE: The header length field of the IP header is from bit4-bit7, but on little endian machines this is swapped with the version field (bit0-bit3)
	header_length = 4 * (ip_hdr->ip_ver_hl & 0x0F);		

	/* make 16 bit words out of every two adjacent 8 bit words 
	 * in the packet and add them up.
	 */
	for (i = 0; i < header_length; i += 2)
	{
		word16 = ((buff[i] << 8) & 0xFF00) + (buff[i+1] & 0xFF);
		sum = sum + (uint32_t) word16;
	}
  
	/* Take only 16 bits out of the 32 bit sum and add up the carries. */
	while (sum >> 16)
	{
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	/* One's complement the result. */
	sum = ~sum;   
  
	return ((uint16_t) sum);
}


void*
reallocf(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

int32_t
mrand48(void)
{
	int32_t value = 0;
	value = (int32_t)rand() << 16;
	value |= (int32_t)rand();
    return value;
}

ULONGLONG bswap_64(ULONGLONG x)
{
	return (((x & 0x00000000000000ff) << 56) |
		((x & 0x000000000000ff00) << 40) |
		((x & 0x0000000000ff0000) << 24) |
		((x & 0x00000000ff000000) <<  8) |
		((x & 0x000000ff00000000) >>  8) |
		((x & 0x0000ff0000000000) >> 24) |
		((x & 0x00ff000000000000) >> 40) |
		((x & 0xff00000000000000) >> 56));
}

UINT32 bswap_32(UINT32 x)
{
	return (((x & 0x000000ff) << 24) |
		((x & 0x0000ff00) <<  8) |
		((x & 0x00ff0000) >>  8) |
		((x & 0xff000000) >> 24));
}

#endif /* _WIN32 */

