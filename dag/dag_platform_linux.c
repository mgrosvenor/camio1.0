/* File header. */
#include "dag_platform_linux.h"

#if defined(__linux__)


uint16_t
ip_sum_calc_linux(uint8_t* buff)
{
	uint16_t word16;
	uint32_t sum = 0;
	uint16_t i;
	struct iphdr* ip_hdr = (struct iphdr*) buff;
	uint16_t header_length = 4 * ip_hdr->ihl;
   
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
#endif /* __linux__ */

