/* File header. */
#include "dag_platform_freebsd.h"

#if defined(__FreeBSD__)

uint32_t bswap_32(uint32_t x)
{
	return (((x & 0x000000ff) << 24) |
		((x & 0x0000ff00) <<  8) |
		((x & 0x00ff0000) >>  8) |
		((x & 0xff000000) >> 24));
}

/*
 * FIXME: Implementation copied from dag_platform_linux/solaris
 * TODO: move this function to a common place
 */
uint16_t
ip_sum_calc_freebsd(uint8_t* buff)
{
	uint16_t word16;
	uint32_t sum = 0;
	uint16_t i;
	struct ip* ip_hdr = (struct ip*) buff;
	uint16_t header_length = 4 * ip_hdr->ip_hl;
   
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

#endif /* __FreeBSD__ */

