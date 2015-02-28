/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: ema_reset.c,v 1.16 2007/02/27 01:37:01 cassandra Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         ema_reset.c
* DESCRIPTION:  Contains the functions for reseting the IXP and xScale
*               embedded processors used in the dag3.7t and dag7.1s.
*
* HISTORY:
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"


/* Endace headers. */
#include "dagapi.h"
#include "dagema.h"
#include "ema_types.h"






#define RESET_DELAY         2000    /* 2 msec */
#define STARTUP_DELAY       1000    /* 1 msec */

#if defined(_WIN32)

#define POLL_DELAY          0       /* yield */
#define POLL_TIMEOUT        2000000

#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))

#define POLL_DELAY          100     /* .1 msec */
#define POLL_TIMEOUT        1000    /* 100 msecs */


#endif /* Platform-specific code. */




/**********************************************************************
 *
 * The last error code returned by internal functions
 *
 **********************************************************************/

extern int g_last_errno;




/**********************************************************************
 *
 * FUNCTION:     ema_msleep
 *
 * DESCRIPTION:  Sleeps for a certain amount of milliseconds 
 *
 * PARAMETERS:   usec         IN      The number of milliseconds to sleep
 *                                    for.
 *
 * RETURNS:      nothing
 *
 **********************************************************************/
static void
ema_msleep(int msec)
{
#if defined(_WIN32)

	/* WIN32 has a millisecond Sleep(), convert to msecs */
	Sleep(msec);

#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	int res;
	struct timespec sleep_time = { .tv_sec = 0, .tv_nsec = msec*1000000};

	res = nanosleep(&sleep_time, NULL);
	if (res == -1) 
		EMA_PVERBOSE(2, "nanosleep() failed %s", strerror(errno));

#endif /* Platform-specific code. */
}




/**********************************************************************
 *
 * FUNCTION:     ema_is_dagconsole_running
 *
 * DESCRIPTION:  Checks if dagconsole is running, in which case a non-zero
 *               value is returned. To determine if dagconsole is running
 *               we attempt to connect to the server socket. This is not
 *               fool proof, if the last session was not shutdown hard
 *               we could connect to the server that is lingering.
 *
 * PARAMETERS:   dagfd        IN      The file descriptor used for the card
 *               
 *
 * RETURNS:      socket identifier or -1 if there was an error.
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)

static int
ema_is_dagconsole_running (int dagfd)
{
	return 0;
}

#else //!defined(DAG_CONSOLE)

static int
ema_is_dagconsole_running (int dagfd)
{
	char       sockname[128];
	daginf_t*  daginf;
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	int        client_sock;
#elif defined (_WIN32)
	SOCKET     client_sock;
#endif
	
	/* get the card id number and type */
	daginf = dag_info(dagfd);

	
	/* attempt to open a connection to the server socket exposed by dagconsole */
	snprintf (sockname, 128, "/tmp/dag%d", daginf->id);
	client_sock = ema_connect_to_server(sockname, daginf->id);
	if (client_sock == INVALID_SOCKET)
		return 0;
	
	
	/* close the client socket */
#if defined(_WIN32)
	closesocket(client_sock);
#elif defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	close(client_sock);
#endif
	
	return 1;
}

#endif   //!defined(DAG_CONSOLE)





/**********************************************************************
 *
 * FUNCTION:     ema_reset_dag3_7t
 *
 * DESCRIPTION:  Resets the xScale on the dag3.7t card.
 *
 * PARAMETERS:   dagfd        IN      The unix style file descriptor to
 *                                    the dag card.
 *               iom          IN      The IOM for the card
 *               flags        IN      Bit flagged value that contains the
 *                                    possible options for reset.
 *               handler      IN      Optional callback handler that will
 *                                    be called throught-out the reset
 *                                    processes. If no notification
 *                                    is required this argument can be NULL
 *
 * NOTES:        This function doesn't check the type of card it is 
 *               trying to reset, it is the callers responsibility to make
 *               sure the card is a 3.7t.
 *
 * RETURNS:      0 if processor was reset, otherwise -1
 *
 **********************************************************************/
int
ema_reset_dag3_7t(int dagfd, char* iom, uint32_t flags, reset_handler_t handler)
{
	uint32_t           timeout;
	volatile uint32_t* pci_reset = NULL;
	volatile uint32_t* pci_out = NULL;
	volatile uint32_t* pci_in = NULL;
	volatile uint32_t* pci_fifo_reset=NULL;
	EMA_DRV_HANDLE     drb_comm;
	int				   using_fifo = 1;


	dag_reg_t          regs[DAG_REG_MAX_ENTRIES];
	uint8_t* iom_ptr;

	iom_ptr = dag_iom(dagfd);
	if(iom_ptr==NULL)
		return -1;
	if(dag_reg_find((char*)iom_ptr, DAG_REG_AD2D_BRIDGE, regs)<1)
	{
		/*no fifo, use drb instead*/
		pci_reset = (volatile uint32_t *)(iom + 0x15c);
		pci_out   = (volatile uint32_t *)(iom + 0x410);
		pci_in    = (volatile uint32_t *)(iom + 0x414);
		using_fifo = 0;
	}
	else
	{ /* fifo*/
		pci_reset = (volatile uint32_t *)(iom + 0x15c);
		pci_out   = (volatile uint32_t *)(iom + 0x418);
		pci_in    = (volatile uint32_t *)(iom + 0x418);
		pci_fifo_reset    = (volatile uint32_t *)(iom + 0x414); /* clean the fifo*/
		using_fifo = 1;
	}
	
	
	/*  */
	*pci_out = 0x00;
	*pci_reset = 0x00;
	if(using_fifo)
		*pci_fifo_reset = 0x80000000;
	
	dagutil_microsleep(RESET_DELAY);
	
	EMA_PVERBOSE(1, "Clearing reset\n");

	/* notify the callback handler */
	if (handler)    if ((g_last_errno = handler(EMA_RST_INIT)) != 0)	    return -1;			
	
	*pci_reset = 0x10;
	
	dagutil_microsleep(RESET_DELAY);
	
	
	/* Wait for the boot code to start */
	timeout = POLL_TIMEOUT;

	while (--timeout && (*pci_in != 0xDEADFEED))
	{
		dagutil_microsleep(STARTUP_DELAY);	/* one millisecond */
	}
	
	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for DEADFEED\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}


	/* Start the boot code */
	*pci_out = 0x01;

	/* notify the callback handler */
	if (handler)    if ((g_last_errno = handler(EMA_RST_BOOTLOADER_STARTED)) != 0)     return -1;


	
	
	/* Wait for the runtime code to be ready */
	timeout = 10;              /* 10 seconds */
	while (--timeout && (*pci_in != 0xFEED0001))
	{
		sleep(1);
	}
	
	if (!timeout)
	{
		/* check for a SODIMM SPD error */
		switch (*pci_in)
		{
			case 0xE9:   g_last_errno = ECARDMEMERROR;     break;
			case 0xE7:   g_last_errno = ECARDMEMERROR;     break;
			case 0xE1:   g_last_errno = ECARDMEMERROR;     break;
			case 0xE2:   g_last_errno = ECARDMEMERROR;     break;
			case 0xE3:   g_last_errno = ECARDMEMERROR;     break;
			case 0xE4:   g_last_errno = ECARDMEMERROR;     break;
			case 0xE5:   g_last_errno = ECARDMEMERROR;     break;
			default:     g_last_errno = ETIMEDOUT;         break;
		}
				
		EMA_PVERBOSE(1, "Timed-out waiting for start\n");
		return -1;
	}


	/* notify the callback handler */
	if (handler)    if ((g_last_errno = handler(EMA_RST_MEMORY_INIT)) != 0)     return -1;

	
	
	/* Start boot loader */
	*pci_out = 0x42;
	timeout  = 5;              /* 5 seconds */
	while (--timeout && (*pci_in != 0xFEED0004))
	{
		sleep(1);
	}
	
	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for boot complete\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}



	/* Start runtime code (with or without memory test) */
	if ( flags & EMA_RUN_DRAM_MEMORY_TEST )
	{
		if (handler)    if ((g_last_errno = handler(EMA_RST_STARTING_MEM_TEST)) != 0)   return -1;
		*pci_out = 0x01;
	}
	else
	{
		*pci_out = 0x00;
	}
	
	EMA_PVERBOSE(1, "Waiting for runtime start\n");



	/* Wait for first communication from runtime code */
	timeout = 45;                       /* 45 seconds */
	while (--timeout && (*pci_in == 0xFEED0004))
	{
		sleep(1);
	}


	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for runtime to start\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}
	else
	{
		*pci_out = 0;
		EMA_PVERBOSE(1, "XScale ready\n\n");
	}
	
	
	/* notify the callback handler */
	if ( flags & EMA_RUN_DRAM_MEMORY_TEST )
		if (handler)    if ((g_last_errno = handler(EMA_RST_FINISHED_MEM_TEST)) != 0)  return -1;
		
	
	/* notify the callback handler */
	if (handler)    if ((g_last_errno = handler(EMA_RST_KERNEL_BOOTED)) != 0)  return -1;

	
	
	/* check if dagconsole is running concurrently and if so just use a fixed delay
	 * to wait for kernel to boot.
	 */
	if ( ema_is_dagconsole_running(dagfd) )
	{
		/* this timeout may need to be changed, dependent on embedded kernel and filesystem */
		sleep (20);
	}
	
	else
	{
		/* open a connection to the drb so that we can poll to wait till
		 * linux is booted before continuing.
		 */
		if(using_fifo)
			drb_comm = drb_fifo_37t_init(dagfd);			
		else
			drb_comm = drb_comm_init(dagfd);

		if (drb_comm == NULL)
		{
			EMA_PVERBOSE(1, "Failed to open connection to drb\n\n");
			return -1;
		}
		
	
		/* flush the console until we get 5 seconds of no data */
		if(using_fifo)
		{
			drb_fifo_37t_flush_recv (drb_comm, 10000);
		}
		else
			drb_comm_flush_recv (drb_comm, 10000);
		
		/* close the drb console handle */
		if(using_fifo)
			drb_fifo_37t_term(drb_comm);
		else
			drb_comm_term (drb_comm);
	}
	
	
	/* notify the callback handler */
	if (handler)    if ((g_last_errno = handler(EMA_RST_COMPLETE)) != 0)        return -1;
	
	
	g_last_errno = ENONE;
	return 0;
}





/**********************************************************************
 *
 * FUNCTION:     ema_halt_dag3_7t
 *
 * DESCRIPTION:  Tells the FPGA to hold the XScale in reset. This function
 *               has been added for completeness and unlike the ema_halt_dag7_1s
 *               function, it is not required for programming the flash
 *
 * PARAMETERS:   dagfd        IN      The unix style file descriptor to
 *                                    the dag card.
 *               iom          IN      The IOM for the card
 *
 * NOTES:        This function doesn't check the type of card it is 
 *               trying to reset, it is the callers responsibility to make
 *               sure the card is a 7.1s.
 *
 * RETURNS:      0 if processor was reset, otherwise -1
 *
 **********************************************************************/
int
ema_halt_dag3_7t(int dagfd, char* iom)
{
	volatile uint32_t* pci_reset;
	

	/* Get a pointer to the reset register */
	pci_reset = (volatile uint32_t *)(iom + 0x15c);
	
	/* Hold the XScale in reset */
	*pci_reset = 0x00;
	sleep (1);

	return 0;
}



/**********************************************************************
 *
 * FUNCTION:     ema_reset_dag7_1s
 *
 * DESCRIPTION:  Resets the IXP on the dag7.1s card. The function polls
 *               the general purpose register in the FPGA for the status
 *               of the reset process.
 *
 * PARAMETERS:   dagfd        IN      The unix style file descriptor to
 *                                    the dag card.
 *               iom          IN      The IOM for the card
 *               flags        IN      Bit flagged value that contains the
 *                                    possible options for reset.
 *               handler      IN      Optional callback handler that will
 *                                    be called throught-out the reset
 *                                    processes. If no notification
 *                                    is required this argument can be NULL
 *
 * NOTES:        This function doesn't check the type of card it is 
 *               trying to reset, it is the callers responsibility to make
 *               sure the card is a 7.1s.
 *
 * RETURNS:      0 if processor was reset, otherwise -1
 *
 **********************************************************************/
int
ema_reset_dag7_1s(int dagfd, char* iom, uint32_t flags, reset_handler_t handler)
{
	uint32_t           timeout;
	volatile uint32_t* pci_reset;
	volatile uint32_t* pci_out;
	volatile uint32_t* pci_fifo_reset;
#if (EMA_VERBOSE_LEVEL > 0) 
	volatile uint32_t* pci_fifo_ctrl;
#endif
	uint32_t           mem_tests;
	uint32_t           status;
	

	pci_reset      = (volatile uint32_t *)(iom + 0x170);
	pci_out        = (volatile uint32_t *)(iom + 0x338);
	pci_fifo_reset = (volatile uint32_t *)(iom + 0x33C);
#if (EMA_VERBOSE_LEVEL > 0) 
	pci_fifo_ctrl  = (volatile uint32_t *)(iom + 0x334);
#endif
	
	
	
	/* stop the processor (in case it is already running) */
	*pci_reset = 0x64300b31;
	sleep (1);
	
	
	/* notify the callback handler */
	if (handler)    if ((g_last_errno = handler(EMA_RST_INIT)) != 0)	    return -1;

	
	/* write the memory test options into the DRB register */
	mem_tests = 0x00000000;
	if ( flags & EMA_RUN_DRAM_MEMORY_TEST )
		mem_tests |= 0x80000000;
	if ( flags & EMA_RUN_CPP_DRAM_MEMORY_TEST )
		mem_tests |= 0x40000000;
	
	*pci_out = mem_tests;
	
	
	/* hold the FIFO in reset until the driver starts */
	*pci_fifo_reset = 0x00000001;

	
	/* cycle through the strap options */
	*pci_reset = 0x64302b11;
	sleep (1);

	*pci_reset = 0x64302b10;
	sleep (1);
	
	*pci_reset = 0x64302b02;


	
	
	
	/* wait to the bootloader starts (1 second timeout) */
	status  = *pci_out;
	timeout = 10;
	
	while (--timeout && ((status & 0x00000001) == 0x00000000) )
	{
		ema_msleep(100);
		status  = *pci_out;
	}		
	
	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for bootloader to start\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}

	EMA_PVERBOSE(1, "Bootloader started\n\n");
	if (handler)    if ((g_last_errno = handler(EMA_RST_BOOTLOADER_STARTED)) != 0)	    return -1;

	
	
	
	
	
	/* check that the XSI memory has been initialised (1 second timeout) */
	status  = *pci_out;
	timeout = 10;
	
	while (--timeout && ((status & 0x00000060) == 0x00000000) )
	{
		ema_msleep(100);
		status  = *pci_out;
	}		
	
	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for XSI memory to initialise\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}

	if ( (status & 0x00000060) != 0x00000060 )
	{
		EMA_PVERBOSE(1, "Problem with initialising XSI (0x%02X)\n", ((status & 0x00000060) >> 5));
		g_last_errno = ECARDMEMERROR;
		return -1;
	}
	
	EMA_PVERBOSE(2, "XSI Memory initialised\n");
	if (handler)    if ((g_last_errno = handler(EMA_RST_MEMORY_INIT)) != 0)	      return -1;

	
	
	/* if we started a memory test, we should check the result (5 second timeout) */
	if ( flags & EMA_RUN_DRAM_MEMORY_TEST )
	{
		if (handler)    if ((g_last_errno = handler(EMA_RST_STARTING_MEM_TEST)) != 0) return -1;
	
		status  = *pci_out;
		timeout = 50;
		
		while (--timeout && ((status & 0x00000180) == 0x00000000) )
		{
			ema_msleep(100);
			status  = *pci_out;
		}
		
		if (!timeout)
		{
			EMA_PVERBOSE(1, "Timed-out waiting for XSI memory test to complete.\n");
			g_last_errno = ETIMEDOUT;
			return -1;
		}
	
		if ( (status & 0x00000180) != 0x00000180 )
		{
			EMA_PVERBOSE(1, "Problem with XSI memory test(0x%02X).\n", ((status & 0x00000180) >> 7));
			g_last_errno = ECARDMEMERROR;
			return -1;
		}

		EMA_PVERBOSE(2, "XSI Memory test finished\n");
		if (handler)    if ((g_last_errno = handler(EMA_RST_FINISHED_MEM_TEST)) != 0)	    return -1;
	}




	/* check that the CPP memory has been initialised (1 second timeout) */
	status  = *pci_out;
	timeout = 10;
	
	while (--timeout && ((status & 0x00000006) == 0x00000000) )
	{
		ema_msleep(100);
		status  = *pci_out;
	}		
	
	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for CPP memory to initialise.\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}

	if ( (status & 0x00000006) != 0x00000006 )
	{
		EMA_PVERBOSE(1, "Problem with initialising CPP (0x%02X).\n", ((status & 0x00000006) >> 1));
		g_last_errno = ECARDMEMERROR;
		return -1;
	}
	
	EMA_PVERBOSE(2, "CPP Memory initialised\n");
	if (handler)    if ((g_last_errno = handler(EMA_RST_MEMORY_INIT)) != 0)	      return -1;
	
	
	
	/* if we started a memory test, we should check the result (45 second timeout) */
	if ( flags & EMA_RUN_CPP_DRAM_MEMORY_TEST )
	{
		if (handler)    if ((g_last_errno = handler(EMA_RST_STARTING_MEM_TEST)) != 0) return -1;

		status  = *pci_out;
		timeout = 450;
		
		while (--timeout && ((status & 0x00000018) == 0x00000000) )
		{
			ema_msleep(100);
			status  = *pci_out;
		}
		
		if (!timeout)
		{
			EMA_PVERBOSE(1, "Timed-out waiting for CPP memory test to complete\n");
			g_last_errno = ETIMEDOUT;
			return -1;
		}
	
		if ( (status & 0x00000018) != 0x00000018 )
		{
			EMA_PVERBOSE(1, "Problem with CPP memory test(0x%02X)\n", ((status & 0x00000018) >> 3));
			g_last_errno = ECARDMEMERROR;
			return -1;
		}

		EMA_PVERBOSE(2, "CPP Memory test finished\n");
		if (handler)    if ((g_last_errno = handler(EMA_RST_FINISHED_MEM_TEST)) != 0)	    return -1;
	}



	
	

	/* wait for the kernel to boot (30 second timeout) */
	status  = *pci_out;
	timeout = 300;
	
	while (--timeout && ((status & 0x00000200) == 0) )
	{
		ema_msleep(100);
		status  = *pci_out;
	}
	
	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for kernel to boot.\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}
		
	EMA_PVERBOSE(2, "Kernel booted\n");
	if (handler)    if ((g_last_errno = handler(EMA_RST_KERNEL_BOOTED)) != 0) return -1;



	
	
	/* wait for the driver to start up (30 second timeout) */
	status  = *pci_out;
	timeout = 300;
	
	while (--timeout && ((status & 0x00000400) == 0) )
	{
		ema_msleep(100);
		status  = *pci_out;
	}
	
	if (!timeout)
	{
		EMA_PVERBOSE(1, "Timed-out waiting for driver to start.\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}
		
	EMA_PVERBOSE(1, "Driver started - reset complete\n");
	if (handler)    if ((g_last_errno = handler(EMA_RST_DRIVER_STARTED)) != 0) return -1;
	if (handler)    if ((g_last_errno = handler(EMA_RST_COMPLETE)) != 0) return -1;

	
	/* take the fifo out of reset */
	*pci_fifo_reset = 0x00000000;


	
	
	g_last_errno = ENONE;
	return 0;
}




/**********************************************************************
 *
 * FUNCTION:     ema_halt_dag7_1s
 *
 * DESCRIPTION:  Tells the FPGA to hold the IXP in reset. This allows
 *               the FPGA to program the flash rom.
 *
 * PARAMETERS:   dagfd        IN      The unix style file descriptor to
 *                                    the dag card.
 *               iom          IN      The IOM for the card
 *
 * NOTES:        This function doesn't check the type of card it is 
 *               trying to reset, it is the callers responsibility to make
 *               sure the card is a 7.1s.
 *
 * RETURNS:      0 if processor was reset, otherwise -1
 *
 **********************************************************************/
int
ema_halt_dag7_1s(int dagfd, char* iom)
{
	volatile uint32_t* pci_reset;
	
	
	/* Get a pointer to the IXP control register */
	pci_reset = (volatile uint32_t *)(iom + 0x170);
	
		
	/* hold the IXP in reset and enable flash rom access */
	*pci_reset = 0x64302b31;
	sleep (1);

	return 0;
}
