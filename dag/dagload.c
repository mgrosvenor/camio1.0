/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: dagload.c,v 1.8 2005/02/14 02:34:13 koryn Exp $
 */

/*
 * Deal with loading and executing objects on DAG cards.
 * At present, this is about downloading new FPGA images,
 * and also running ARM executables, on some of the older
 * cards.
 */


/* DAG tree headers. */
#include "dagapi.h"
#include "dagnew.h"
#include "dagutil.h"

/* POSIX headers. */
#include <unistd.h>

/* C Standard Library headers. */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define	BIT28		(1<<28)
#define	MUX(A,D)	(((A)<<8)|(D))

# ifdef TEST

static volatile uint32_t	*dagromcsr;

extern char	*dag_xrev_name(uint8_t *xrev, uint32_t xlen);

static uint32_t
romread(uint32_t addr)
{
	*dagromcsr = (BIT28 | MUX(addr, 0));
	dagutil_nanosleep(500);
	return (*dagromcsr & 0xff);
}

static int dagfd;
static dag_reg_t* regs;
static char dagname[] = "/dev/dag0";


int
main(int argc, char *argv[])
{
	int err, nregs;
	dag_reg_t eprom[DAG_REG_MAX_ENTRIES];
	unsigned char buf[128];
	int i;

	dagutil_set_progname(argv[0]);

	if((dagfd = dag_open(dagname)) < 0)
		dagutil_panic("open %s: %s\n", dagname, strerror(errno));

	regs = dag_regs(dagfd);

	nregs = 0;
	if((err = dag_reg_table_find(regs, 0, DAG_REG_ROM, eprom, &nregs)))
		dagutil_panic("dag_reg_table_find returns %s\n", strerror(err));
	if(nregs != 1)
		dagutil_panic("expecting one ROM register set, found %d\n", nregs);

	printf("0x%x\n", DAG_REG_ADDR(*eprom));
	dagromcsr = (uint32_t *)(dag_iom(dagfd) + DAG_REG_ADDR(*eprom));

	for( i = 0 ; i < sizeof(buf) ; i++ )
		buf[i] = romread(i);

	printf("%s\n", dag_xrev_name(buf, sizeof(buf)));

	return EXIT_SUCCESS;
}

#endif /* TEST */
