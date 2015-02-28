/*
 * Copyright (c) 2003-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagname.c,v 1.11 2005/07/20 01:33:53 koryn Exp $
 */

/* C Standard Library headers. */
#include <stdio.h>

#ifndef _WIN32
# include	<inttypes.h>
#else
# include	<wintypedefs.h>
#endif /* _WIN32 */

/*
 * We put a lot of paranoia and redundancy into the code to make
 * it foolproof against spurious data.
 */
char**
dag_xrev_parse(uint8_t *xrev, uint32_t xlen, char* argv[5])
{
	int i;
	int rlen;
	char **ap = argv;
	char expect[] = "abcd";

	xrev += 13;
	xlen -= 13;

	for(i = 0 ; i < 4 ; i++) {
		/*
		 * Record tag: a,b,c,d
		 */
		if(xrev[0] != expect[i])
			goto done; /* must be a,b,c,d in that order */
		xrev++;
		xlen--;
		/*
		 * 2 Bytes record length, little endian
		 */
		rlen = (((uint32_t)xrev[0]) << 8) | xrev[1];
		xrev += 2;
		xlen -= 2;
		/*
		 * Actual record
		 */
		if(rlen > (int)xlen)
			goto done; /* record points past buffer */
		*ap++ = (char*) xrev;
		xrev += rlen;
		xlen -= rlen;
	}
	
done:
	while(ap < &argv[5])
		*ap++ = NULL;
	return argv;
}

/*
 * This routine will partially destroy the contents of xrev by
 * writing '\0' markers into the buffer and letting argv point
 * into the buffer.
 */
char*
dag_xrev_name(uint8_t* xrev, uint32_t xlen)
{
	static char retbuf[128];
	char* bp = retbuf;
	char* argv[5];
	char** ap = dag_xrev_parse(xrev, xlen, argv);

	while (*ap != NULL)
	{
		bp += sprintf(bp, "%s ", *ap++);
	}
	
	if (ap != argv)
	{
		*--bp = '\0'; /* remove trailing space */
	}
	else
	{
		*bp = '\0'; /* empty buffer */
	}
	
	return retbuf;
}
