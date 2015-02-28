/* File header. */
#include "adt_adler32.h"

/* C Standard Library headers. */
#include <stdlib.h>


static const uint32_t BASE = 65521L; /* largest prime smaller than 65536 */
static const int NMAX = 5552;
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf,i) {s1 += (uint32_t) buf[i]; s2 += s1;}
#define DO2(buf,i) DO1(buf,i); DO1(buf,i+1);
#define DO4(buf,i) DO2(buf,i); DO2(buf,i+2);
#define DO8(buf,i) DO4(buf,i); DO4(buf,i+4);
#define DO16(buf) DO8(buf,0); DO8(buf,8);


uint32_t
adt_adler32(const char* buf, uint32_t len)
{
	uint32_t s1 = 0;
	uint32_t s2 = 0;
	unsigned int k;

	if (buf == NULL)
	{
		return 1UL;
	}

	while (len > 0)
	{
		k = len < (uint32_t)NMAX ? len : (uint32_t)NMAX;
		len -= k;
		while (k >= 16)
		{
			DO16(buf);
			buf += 16;
			k -= 16;
		}

		if (k != 0)
		{
			do
			{
				s1 += (uint32_t) *buf++;
				s2 += s1;
			} while (--k);
		}
		s1 %= BASE;
		s2 %= BASE;
	}

	return (s2 << 16) | s1;
}
