/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: drb_comm.c,v 1.14 2009/04/14 01:56:12 wilson.zhu Exp $
*
**********************************************************************/

/**********************************************************************
 * FILE:         drb_comm.c
 * DESCRIPTION:  Implements byte send and receive across the two register
 *               DRB interface. The byte send function blocks (with a timeout),
 *               however it also allows asyncronise communication by simultanously
 *               checking the receive stream, and buffering the data until
 *               byte receive is called.
 *
 *               To use, call drb_comm_init first which returns a handle
 *               that should be used in calls to drb_comm_put_byte and
 *               drb_comm_get_byte. To clean up, call drb_comm_term with
 *               the handle returned by drb_comm_init.
 *
 *               drb_comm_init should only be called once per library
 *               instance, however there is no locking to ensure this.
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
#include "dagpci.h"

//#include "dagutil.h"



/**********************************************************************
 *
 * Constants
 *
 */
 
#if defined(DAG_CONSOLE)
#define CONSOLE_BUF_SIZE     2048
#endif



/**********************************************************************
 *
 * Macros
 *
 */







/**********************************************************************
 *
 * Structure that stores the context of a particular card
 *
 */
typedef struct _drb_comm_t
{
	uint32_t           signature;          /* Unique identifies this structure */

	volatile char*     iom;
	volatile uint32_t* pci_in;             /* DRB input */
	volatile uint32_t* pci_out;            /* DRB output */
	uint32_t           pci_out_shadow;     /* Local copy of last sent data - saves PCI bus read access */
	uint8_t            write_count;        /* current byte write count */
	uint8_t            read_count;         /* current byte read count */
	
	uint8_t*           recv_buffer;        /* buffer used to store the received byte(s) during a write */
	uint32_t           recv_buffer_size;   /* the size of the above receive buffer (in bytes) */
	uint32_t           recv_buffer_full;   /* the amount of data in the buffer */
	
#if defined(_WIN32)                         /* mutex used for access to the recv buffer above */
	CRITICAL_SECTION   mutex;
#else
	pthread_mutex_t    mutex;              
#endif

	
	
	
#if defined(DAG_CONSOLE)
	console_putchar_t  console_handler;       /* console character handler */

	uint8_t*           console_buffer;        /* buffer used to store bytes to be send out the console channel */
	uint32_t           console_buffer_full;   /* the amount of data in the console buffer */
	
#if defined(_WIN32)                              /* console mutex used for access to the recv buffer above */
	CRITICAL_SECTION   console_mutex;
#else
	pthread_mutex_t    console_mutex;
#endif

#endif  // defined(DAG_CONSOLE)
	

	
} drb_comm_t;







/**********************************************************************
 *
 * Critical Section
 *
 **********************************************************************/


#if defined(_WIN32)
	#define ENTER_CRITICAL_SECTION(x)	EnterCriticalSection((x))
	#define LEAVE_CRITICAL_SECTION(x)	LeaveCriticalSection((x))
#else
	#define ENTER_CRITICAL_SECTION(x)	pthread_mutex_lock((x))
	#define LEAVE_CRITICAL_SECTION(x)	pthread_mutex_unlock((x))
#endif



/**********************************************************************
 *
 * The last error code returned by internal functions
 *
 **********************************************************************/

extern int g_last_errno;





/**********************************************************************
 *
 * Constants
 *
 **********************************************************************/

/* The possible DRB channels */
#define EMA_MSG_CHANNEL             0x00
#define EMA_TTY_CHANNEL             0x01




/**********************************************************************
 *
 * External Functions
 *
 **********************************************************************/

#if defined(DAG_CONSOLE)
static int drb_comm_37t_send_console_string (DRB_COMM_HANDLE handle, char* str, int len);

#endif


/**********************************************************************
 *
 * FUNCTION:     drb_comm_init
 *
 * DESCRIPTION:  Initialises a handle to be used for DRB communication.  
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *
 * RETURNS:      A handle to the DRB communications, or NULL if there
 *               was an error.
 *
 **********************************************************************/
DRB_COMM_HANDLE
drb_comm_init (int dagfd)
{
	drb_comm_t* handleP;
	daginf_t*   daginf;
	
	
	/* Ensure we are dealing with a 3.7t or 7.1s */
	daginf = dag_info(dagfd);
	if ( daginf->device_code != PCI_DEVICE_ID_DAG3_70T &&
	     daginf->device_code != PCI_DEVICE_ID_DAG3_7T4 &&	
	     daginf->device_code != PCI_DEVICE_ID_DAG7_10 )
	{
		return NULL;
	}


	/* Allocate a struct and return a pointer to it as a handle */
	handleP = malloc (sizeof(drb_comm_t));
	if ( handleP == NULL )
		return NULL;
	memset (handleP, 0, sizeof(drb_comm_t));
	

	handleP->signature = DRB_COMM_SIGNATURE;
	handleP->iom       = (char*) dag_iom(dagfd);


	/* The drb registers are card specific */
	if ( daginf->device_code == PCI_DEVICE_ID_DAG7_10 )
	{
		handleP->pci_in   = (volatile uint32_t *)(handleP->iom + 0x334);
		handleP->pci_out  = (volatile uint32_t *)(handleP->iom + 0x330);
	}
	
	else if ( daginf->device_code == PCI_DEVICE_ID_DAG3_70T )
	{
		handleP->pci_in   = (volatile uint32_t *)(handleP->iom + 0x414);
		handleP->pci_out  = (volatile uint32_t *)(handleP->iom + 0x410);
	}
	
	else if ( daginf->device_code == PCI_DEVICE_ID_DAG3_7T4 )
	{
		handleP->pci_in   = (volatile uint32_t *)(handleP->iom + 0x414);
                handleP->pci_out  = (volatile uint32_t *)(handleP->iom + 0x410);
	}
	

	/* allocate a 2k buffer to use to store messages */
	handleP->recv_buffer_size = DRB_INIT_RECV_BUFFER_SIZE;
	handleP->recv_buffer      = malloc (handleP->recv_buffer_size);
	handleP->recv_buffer_full = 0;
	
	if ( handleP->recv_buffer == NULL )
	{
		free (handleP);
		return NULL;
	}

	
	
#if defined(DAG_CONSOLE)
	/* standard out is the default for console characters */
	handleP->console_handler = putchar;
#endif	

	
	
	/* initialise the critical section for the buffer */
#if defined(_WIN32)
	InitializeCriticalSection (&handleP->mutex);
#else
	{
		pthread_mutexattr_t mutexattr;

		/* Set the mutex as a recursive mutex */
	#if defined(__linux__)
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
	#else
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	#endif
		
		/* create the mutex with the attributes set */
		pthread_mutex_init(&handleP->mutex, &mutexattr);
		
		/* after initializing the mutex, the thread attribute can be destroyed */
		pthread_mutexattr_destroy(&mutexattr);
	}
#endif

	
	
	/* initialise fields for the console data */
#if defined(DAG_CONSOLE)

	/* allocate a 2k buffer to store console data */
	handleP->console_buffer      = malloc(CONSOLE_BUF_SIZE);
	handleP->console_buffer_full = 0;
	
	/* setup the mutex for access to the console buffer */
	#if defined(_WIN32)
	InitializeCriticalSection (&handleP->console_mutex);
	#else
	{
		pthread_mutexattr_t mutexattr;
	
		/* Set the mutex as a recursive mutex */
	#if defined(__linux__)
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
	#else
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	#endif
			
		/* create the mutex with the attributes set */
		pthread_mutex_init(&handleP->console_mutex, &mutexattr);
			
		/* after initializing the mutex, the thread attribute can be destroyed */
		pthread_mutexattr_destroy(&mutexattr);
	}
	#endif

	dagema_console_drv_reg(drb_comm_37t_send_console_string);

#endif // defined(DAG_CONSOLE) 	
	
	
	
	
	
	
	
	/* let remote know that comm mode is enabled, and that we're ready to receive */
	handleP->pci_out_shadow = *handleP->pci_out;
	handleP->write_count    = (uint8_t)((*handleP->pci_in) >> 16);
	handleP->read_count     = (uint8_t)((handleP->pci_out_shadow) >> 16);
	
	EMA_PVERBOSE (3, "handleP->pci_out_shadow = 0x%08X\n", handleP->pci_out_shadow);
	EMA_PVERBOSE (4, "handleP->read_count = 0x%02X\n", handleP->read_count);
	
	
	return handleP;
}



/**********************************************************************
 *
 * FUNCTION:     drb_comm_term
 *
 * DESCRIPTION:  Terminates and cleans up the drb communication  
 *
 * PARAMETERS:   handle       IN      A handle to the DRB comm, previously
 *                                    returned by drb_comm_init.
 *
 * RETURNS:      nothing
 *
 **********************************************************************/
void
drb_comm_term (DRB_COMM_HANDLE handle)
{
	drb_comm_t* handleP = (drb_comm_t*) handle;

	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_COMM_SIGNATURE);
	if ( handleP == NULL || handleP->signature != DRB_COMM_SIGNATURE )
		return;
	handleP->signature = 0;
	
	
	/* destroy the critical section used for buffer access */
#if defined(_WIN32)
	DeleteCriticalSection (&handleP->mutex);
#else
	pthread_mutex_destroy (&handleP->mutex);
#endif	

	
	/* remove the console fields */
#if defined(DAG_CONSOLE)

	free (handleP->console_buffer);

	#if defined(_WIN32)
	DeleteCriticalSection (&handleP->console_mutex);
	#else
	pthread_mutex_destroy (&handleP->console_mutex);
	#endif
	
#endif // defined(DAG_CONSOLE)

	
	/* clean up */
	free (handleP->recv_buffer);
	free (handleP);
}





/**********************************************************************
 *
 * FUNCTION:     drb_write_console_char
 *
 * DESCRIPTION:  Checks if the internal console buffer has any data in
 *               it and if the DRB registers can accept another byte the
 *               console byte is copied into the DRB register.
 *
 * PARAMETERS:   handle       IN      A handle to the DRB comm, previously
 *                                    returned by drb_comm_init.
 *               ch           IN      Character to send
 *
 * RETURNS:      0 if the write succeeded otherwise -1
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)

void
drb_write_console_char (drb_comm_t* handleP)
{
	uint32_t    in;

	/* check if a byte can be written to the card */
	if ( (uint8_t)((in = *handleP->pci_in) >> 16) != handleP->write_count )
		return;

		
	/* check if we have a character in the console buffer to write */
	ENTER_CRITICAL_SECTION (&handleP->console_mutex);

	if ( handleP->console_buffer_full > 0 )
	{
		
		/* we can now write the new byte to the xScale */
		handleP->write_count++;
		handleP->pci_out_shadow = (handleP->pci_out_shadow & 0x00FF0000) | 
								  (((uint32_t)EMA_TTY_CHANNEL) << 24) | 
								  (((uint32_t)handleP->write_count) << 8) | 
								  handleP->console_buffer[0];
		*handleP->pci_out = handleP->pci_out_shadow;
		
		/* remove the character from the buffer */
		handleP->console_buffer_full--;
		if ( handleP->console_buffer_full != 0 )
			memmove (&(handleP->console_buffer[0]), &(handleP->console_buffer[1]), handleP->console_buffer_full);
	}		
	
	LEAVE_CRITICAL_SECTION (&handleP->console_mutex);
	

}

#endif		// DAG_CONSOLE




int drb_comm_put_string (DRB_COMM_HANDLE handle, const uint8_t *tx_bytes, int length, int timeout)
{
	int loop;
	for(loop=0;loop<length;loop++)
	{
		if(drb_comm_put_byte(handle, tx_bytes[loop], timeout) !=0)
			return -1;
	}
	return 0;
}


/**********************************************************************
 *
 * FUNCTION:     drb_comm_put_byte
 *
 * DESCRIPTION:  Waits till the last byte put out the DRB is read by the
 *               xScale and then writes the supplied value. Specify a timeout
 *               to give up after a certain amount of milli seconds.
 *
 * PARAMETERS:   handle       IN      A handle to the DRB comm, previously
 *                                    returned by drb_comm_init.
 *               tx_byte      IN      The data byte to write
 *               timeout      IN      Timeout time in milliseconds, pass
 *                                    a negative value to block until the
 *                                    byte is sent.
 *
 * RETURNS:      0 if the write succeeded otherwise -1
 *
 **********************************************************************/
int
drb_comm_put_byte (DRB_COMM_HANDLE handle, uint8_t tx_byte, int timeout)
{
	drb_comm_t* handleP = (drb_comm_t*) handle;
	uint32_t    in;
	uint8_t*    new_buffer;
	uint64_t    cur_time;
	uint64_t    start_time;
	
	
	
	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_COMM_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_COMM_SIGNATURE )
		return -1;

	

	
	/* write any console data we may have */
#if defined(DAG_CONSOLE)
	drb_write_console_char (handleP);
#endif
	

	/* get the start time for working out the timeout */
	start_time = ema_get_tick_count();



	/* loop until the previous byte is consumed by the xScale */
	while ( (uint8_t)((in = *handleP->pci_in) >> 16) != handleP->write_count )
	{

		/* check if there is a byte to be read, and if so buffer it internally */
		if ( ((uint8_t)(in >> 8) != handleP->read_count) )
		{
			EMA_PVERBOSE (4, "read 0x%02X %02X (wc:%d)\n", (uint8_t)in, (uint8_t)(in >> 24), handleP->write_count);
			

			/* update the read count */
			handleP->read_count = (uint8_t)(in >> 8);
			handleP->pci_out_shadow = (handleP->pci_out_shadow & 0xFF00FFFF) | ((uint32_t)handleP->read_count << 16);
			*handleP->pci_out = handleP->pci_out_shadow;
			
			
			/* determine if a console value */
			if ( (uint8_t)(in >> 24) == EMA_TTY_CHANNEL )
			{
#if defined(DAG_CONSOLE)
				if ( handleP->console_handler != NULL )
					handleP->console_handler((char)(in & 0x00FF));
#endif
			}
			
			else
			{
				ENTER_CRITICAL_SECTION (&handleP->mutex);
				
				/* copy the byte into the buffer */
				handleP->recv_buffer[handleP->recv_buffer_full] = (uint8_t)(in & 0x000000FF);
				handleP->recv_buffer_full++;
					
				/* check if we need to increase the buffer size */
				if ( handleP->recv_buffer_full >= handleP->recv_buffer_size )
				{
					new_buffer = realloc (handleP->recv_buffer, handleP->recv_buffer_size + DRB_RECV_BUFFER_INCR);
					if ( new_buffer == NULL )
						handleP->recv_buffer_full -= 1;
					else
					{
						handleP->recv_buffer_size += DRB_RECV_BUFFER_INCR;
						handleP->recv_buffer = new_buffer;
					}
				}
				
				LEAVE_CRITICAL_SECTION (&handleP->mutex);
			}
			
		}
		
		
		/* get the current tick count */
		cur_time = ema_get_tick_count();
		

		/* check if we have timed-out */
		if ( timeout > 0 )
			if ( (cur_time - start_time) >= timeout )
			{
				TRACE;
				return -1;
			}

		/*  */
		ema_usleep (100);
	}

	
	
	/* we can now write the new byte to the xScale */
	handleP->write_count++;
	
	EMA_PVERBOSE (4, "write 0x%02X\n", (uint8_t)tx_byte);

	handleP->pci_out_shadow = (handleP->pci_out_shadow & 0x00FF0000) | 
	                          (((uint32_t)EMA_MSG_CHANNEL) << 24) | 
	                          (((uint32_t)handleP->write_count) << 8) | 
	                          tx_byte;
	
	*handleP->pci_out = handleP->pci_out_shadow;
		

	return 0;
}	
	
	
	





/**********************************************************************
 *
 * FUNCTION:     drb_comm_get_byte
 *
 * DESCRIPTION:  Checks if there is a character to be read and then returns.
 *               This function does not block.
 *
 * PARAMETERS:   handle       IN      A handle to the DRB comm, previously
 *                                    returned by drb_comm_init.
 *               rx_byte      OUT     Pointer to a byte to receive the data
 *
 * RETURNS:      Returns 0 if a byte was read and channel & rx_byte were
 *               updated. If no byte to be received then -1 is returned
 *               and rx_byte remains unchanged.
 *
 **********************************************************************/
int
drb_comm_get_byte (DRB_COMM_HANDLE handle, uint8_t* rx_byte)
{
	drb_comm_t* handleP = (drb_comm_t*) handle;
	uint32_t    in;
	uint8_t     in_read_count;

	
	/* sanity checking */
	assert(rx_byte != NULL);
	assert(handle  != NULL);
	assert(handleP->signature == DRB_COMM_SIGNATURE);
	if ( rx_byte == NULL || handle == NULL || handleP->signature != DRB_COMM_SIGNATURE )
		return -1;

	
	/* write any console data we may have */
#if defined(DAG_CONSOLE)
	drb_write_console_char (handleP);
#endif


	
	/* first check if there is any data in the buffer */
	ENTER_CRITICAL_SECTION (&handleP->mutex);

	if ( handleP->recv_buffer_full > 0 )
	{
		*rx_byte = handleP->recv_buffer[0];
		
		/* I know this is inefficient (shifting the buffer everytime a byte is removed), however it
		 * shouldn't happen very often and it is the simplest.
		 */		
		handleP->recv_buffer_full--;
		memmove (&(handleP->recv_buffer[0]), &(handleP->recv_buffer[1]), handleP->recv_buffer_full);
		
		
		/* check to see if we have allocate a large buffer and we now only using
		 * less the 20% of it, in which case we should trim it back down to a 
		 * reasonable size.
		 */
		if ( (handleP->recv_buffer_size > DRB_INIT_RECV_BUFFER_SIZE) &&
		     (handleP->recv_buffer_full < (DRB_INIT_RECV_BUFFER_SIZE / 5)) )
		{
			uint8_t *new_buffer = realloc (handleP->recv_buffer, DRB_INIT_RECV_BUFFER_SIZE);
			if ( new_buffer != NULL )
			{
				handleP->recv_buffer_size = DRB_INIT_RECV_BUFFER_SIZE;
				handleP->recv_buffer = new_buffer;
			}
		}
				
		LEAVE_CRITICAL_SECTION (&handleP->mutex);

		/*dagutil_verbose_level (3, "-> read byte from buffer, 0x%02X\n", *rx_byte);*/
		return 0;
	}
	
	LEAVE_CRITICAL_SECTION (&handleP->mutex);



	/* read the value on the 'in' register */
	in = *handleP->pci_in;
	in_read_count  = (uint8_t)(in >> 8);

	/* Keep reading as long as there is output for the console */
	while ( in_read_count != handleP->read_count ) {

		handleP->read_count = in_read_count;

		/* indicate to the xScale that we have read the byte */
		handleP->pci_out_shadow = (handleP->pci_out_shadow & 0xFF00FFFF) | ((in << 8) & 0x00FF0000);
		*handleP->pci_out       =  handleP->pci_out_shadow;
		
		EMA_PVERBOSE (4, "read 0x%02X %s (rc:%02X)\n", (uint8_t)in, (in & 0x01000000) ? "dbg":"", in_read_count);

	
		if ( (uint8_t)(in >> 24) == EMA_TTY_CHANNEL ) {
#if defined(DAG_CONSOLE)
			dagema_console_char_handler((char)(in & 0x00FF));
#endif
			/* If no console handler then discard console output */
		} else {

			/* Got a binary byte */
			/* copy the data into the supplied argument */
			*rx_byte = (uint8_t) (in & 0xFF);
		    return 0;
		}

		/* Get next byte */
		in = *handleP->pci_in;
		in_read_count  = (uint8_t)(in >> 8);
	}

	/* No data */
	return -1;
}


/**********************************************************************
 *
 * FUNCTION:     drb_comm_flush_recv
 *
 * DESCRIPTION:  Flushes the receive buffers of any bytes, this effects
 *               all the channels
 *
 * PARAMETERS:   handle       IN      A handle to the DRB comm, previously
 *                                    returned by drb_comm_init.
 *
 * RETURNS:      0 if the write succeeded otherwise -1
 *
 **********************************************************************/
int
drb_comm_flush_recv (DRB_COMM_HANDLE handle, int timeout)
{
	drb_comm_t* handleP = (drb_comm_t*) handle;
	uint32_t    in;
	uint32_t    in_read_count;
	uint64_t    cur_time;
	uint64_t    start_time;
	int         idle;

	

	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_COMM_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_COMM_SIGNATURE )
		return -1;


	/* first check if there is any data in the buffer */
	ENTER_CRITICAL_SECTION (&handleP->mutex);

	if ( handleP->recv_buffer_full > 0 )
	{
		EMA_PVERBOSE (4, "flushed %d bytes from buffer\n", handleP->recv_buffer_full);
		handleP->recv_buffer_full = 0;
	}

	LEAVE_CRITICAL_SECTION (&handleP->mutex);
	

	
	
	/* now poll the receive register and remove everything until we have received
	 * nothing for the timeout period.
	 */
	idle = 0;
	if ( timeout > 0 )
	{
		
		/* get the start time for working out the timeout */
		start_time = ema_get_tick_count();
		
		do
		{			
			idle++;
			
			/* get the current tick count */
			cur_time = ema_get_tick_count();


			/* read the value on the 'in' register */
			in = *handleP->pci_in;
		
			/* check if there is a byte to read */
			in_read_count  = (uint8_t)(in >> 8);
			if ( in_read_count != handleP->read_count )
			{
				handleP->read_count = (uint8_t)in_read_count;
	
				handleP->pci_out_shadow = (handleP->pci_out_shadow & 0xFF00FFFF) | ((in << 8) & 0x00FF0000);
				*handleP->pci_out       =  handleP->pci_out_shadow;
		
				EMA_PVERBOSE (4, "flushed 0x%02X %s (rc:%02X)\n", (uint8_t)in, (in & 0x01000000) ? "dbg":"", in_read_count);
				
				
				/* if a console character handler is installed, dump the flushed console characters to the handler. */
#if defined(DAG_CONSOLE)
				if ( ((uint8_t)(in >> 24) == EMA_TTY_CHANNEL) && (handleP->console_handler != NULL) )
					handleP->console_handler((char)(in & 0x00FF));
#endif
				
				/* calculate the next timeout value */
				start_time = cur_time;

				idle = 0;
			}

			/*   */
			if (idle >= 10)
			{
				ema_usleep (1000);
				idle = 0;
			}
			
		} while ( (cur_time - start_time) < timeout );
	}
			

	return 0;
}

/**********************************************************************
 *
 * FUNCTION:     drb_send_console_char
 *
 * DESCRIPTION:  Sends a console character out the DRB
 *
 * PARAMETERS:   handle       IN      A handle to the DRB comm, previously
 *                                    returned by drb_comm_init.
 *               str          IN      Characters to send
 *               len          IN      Number of characters
 *
 * RETURNS:      0 if the write succeeded otherwise -1
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)

int
drb_comm_37t_send_console_string (DRB_COMM_HANDLE handle, char* str, int len)
{
	drb_comm_t* handleP = (drb_comm_t*) handle;
	int         i;

	/* sanity checking */
	if ( handle == NULL || handleP->signature != DRB_COMM_SIGNATURE )
		return -1;

	
	/* Add the character to the console buffer to send */
	ENTER_CRITICAL_SECTION (&handleP->console_mutex);


	for (i=0; i<len; i++)
	{
		if ( handleP->console_buffer_full >= CONSOLE_BUF_SIZE )
			break;
		else
			handleP->console_buffer[handleP->console_buffer_full++] = str[i];
	}
	
	LEAVE_CRITICAL_SECTION (&handleP->console_mutex);
	
	
	return 0;
}

#endif		// DAG_CONSOLE

