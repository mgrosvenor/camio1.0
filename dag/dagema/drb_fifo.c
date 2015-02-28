/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: drb_fifo.c,v 1.9 2008/03/03 01:03:41 usama Exp $
*
**********************************************************************/

/**********************************************************************
 * FILE:         drb_fifo.c
 * DESCRIPTION:  Implements byte send and receive across the drb using
 *               a 512 entry FIFO.
 *
 *               To use, call drb_fifo_init first which returns a handle
 *               that should be used in calls to drb_fifo_put_byte and
 *               drb_fifo_get_byte. To clean up, call drb_fifo_term with
 *               the handle returned by drb_fifo_init.
 *
 *               drb_fifo_init should only be called once per library
 *               instance, however there is no locking to ensure this.
 *
 * NOTES:        
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





/**********************************************************************
 *
 * Constants
 *
 */
 




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
typedef struct _drb_fifo_t
{
	uint32_t           signature;          /* Unique identifies this structure */

	volatile char*     iom;
	volatile uint32_t* pci_fifo_data;      /* DRB fifo data */
	volatile uint32_t* pci_fifo_ctrl;      /* DRB fifo control */
	volatile uint32_t* pci_fifo_clear;     /* DRB fifo clear */
	
	uint8_t*           recv_fifo_buf;      /* buffer used to store the received byte(s) during a write */
	uint32_t           recv_fifo_head;     /* the head pointer(index) of the buffer (inserts) */
	uint32_t           recv_fifo_tail;     /* the tail pointer(index) of the buffer (removals) */
	uint32_t           recv_fifo_count;    /* the number of bytes in the internal fifo */
	
#if defined(_WIN32)                         /* mutex used for access to the recv buffer above */
	CRITICAL_SECTION   mutex;
#else
	pthread_mutex_t    mutex;              
#endif

} drb_fifo_t;







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




/**********************************************************************
 *
 * External Functions
 *
 **********************************************************************/


/**********************************************************************
 * FUNCTION:     drb_fifo_ema_drv_reg
 * DESCRIPTION:  Called by ema library to register 71s fifo to ema lib
 *				 Only register function pointers.
 * PARAMETERS:   ema_context_t  IN/OUT      ema context to register.
 * RETURNS:      0 if success, -1 if error
 **********************************************************************/
int drb_fifo_ema_drv_reg(ema_context_t* ctx)
{
	ema_drv_cbf_t cbf;
	cbf.pfunc_drv_flush_recv = drb_fifo_flush_recv;
	cbf.pfunc_drv_get_byte = drb_fifo_get_byte;
	cbf.pfunc_drv_put_byte = drb_fifo_put_byte;
	cbf.pfunc_drv_put_string = drb_fifo_put_string;
	cbf.pfunc_drv_init = drb_fifo_init;
	cbf.pfunc_drv_term = drb_fifo_term;
	return ema_drv_cbf_reg(ctx,&cbf);
}



/**********************************************************************
 *
 * FUNCTION:     drb_fifo_init
 *
 * DESCRIPTION:  Initialises a handle to be used for DRB communication.
 *               
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *
 * RETURNS:      A handle to the DRB communications, or NULL if there
 *               was an error.
 *
 **********************************************************************/
DRB_FIFO_HANDLE
drb_fifo_init (int dagfd)
{
	drb_fifo_t* handleP;
	daginf_t*   daginf;
	
	
	/* Ensure we are dealing with a 7.1s */
	daginf = dag_info(dagfd);
	if ( daginf->device_code != PCI_DEVICE_ID_DAG7_10 )
	{
		return NULL;
	}


	/* Allocate a struct and return a pointer to it as a handle */
	handleP = malloc (sizeof(drb_fifo_t));
	if ( handleP == NULL )
		return NULL;
	memset (handleP, 0, sizeof(drb_fifo_t));
	

	handleP->signature = DRB_FIFO_SIGNATURE;
	handleP->iom       = (char*) dag_iom(dagfd);


	/* The drb registers are card specific */
	handleP->pci_fifo_data  = (volatile uint32_t *)(handleP->iom + 0x330);
	handleP->pci_fifo_ctrl  = (volatile uint32_t *)(handleP->iom + 0x334);
	handleP->pci_fifo_clear = (volatile uint32_t *)(handleP->iom + 0x33C);
	

	/* allocate a 2k buffer to use to store messages */
	handleP->recv_fifo_buf   = malloc (DRB_INTERNAL_FIFO_SIZE);
	handleP->recv_fifo_head  = 0;
	handleP->recv_fifo_tail  = 0;
	handleP->recv_fifo_count = 0;
	
	if ( handleP->recv_fifo_buf == NULL )
	{
		free (handleP);
		return NULL;
	}

	
	
	
	/* initialise the critical section for the buffer */
#if defined(_WIN32)
	InitializeCriticalSection (&handleP->mutex);

#else
	{
		pthread_mutexattr_t mutexattr;
		pthread_mutexattr_init(&mutexattr);
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


	
	return handleP;
}



/**********************************************************************
 *
 * FUNCTION:     drb_fifo_term
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
drb_fifo_term (DRB_FIFO_HANDLE handle)
{
	drb_fifo_t* handleP = (drb_fifo_t*) handle;

	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_FIFO_SIGNATURE);
	if ( handleP == NULL || handleP->signature != DRB_FIFO_SIGNATURE )
		return;
	handleP->signature = 0;
	
	
	/* destroy the critical section used for buffer access */
#if defined(_WIN32)
	DeleteCriticalSection (&handleP->mutex);
#else 
	pthread_mutex_destroy (&handleP->mutex);
#endif

	
	/* clean up */
	free (handleP->recv_fifo_buf);
	free (handleP);
}





/**********************************************************************
 *
 * FUNCTION:     drb_fifo_insert_buf_byte
 *
 * DESCRIPTION:  Inserts a byte into the internal buffer used for the
 *               receive fifo. This functions grows the size of the buffer
 *               if there is not enough space.
 *
 * PARAMETERS:   handle       IN      A handle to the DRB fifo, previously
 *                                    returned by drb_fifo_init.
 *               tx_byte      IN      The data byte to write
 *
 * RETURNS:      nothing
 *
 **********************************************************************/
static void
drb_fifo_insert_buf_byte (drb_fifo_t* handleP, uint8_t rx_byte)
{
	
	ENTER_CRITICAL_SECTION (&handleP->mutex);
	
	/* check if the internal fifo is full */
	if ( handleP->recv_fifo_count == (DRB_INTERNAL_FIFO_SIZE - 5) )
	{
		LEAVE_CRITICAL_SECTION (&handleP->mutex);
		return;
	}
	
	/* add the byte to the head */
	handleP->recv_fifo_buf[handleP->recv_fifo_head++] = rx_byte;
	if ( handleP->recv_fifo_head >= DRB_INTERNAL_FIFO_SIZE )
		handleP->recv_fifo_head = 0;
	
	handleP->recv_fifo_count++;
	
	LEAVE_CRITICAL_SECTION (&handleP->mutex);
	
}




/**********************************************************************
 *
 * FUNCTION:     drb_fifo_put_byte
 *
 * DESCRIPTION:  Waits till the fifo has space for the byte and then puts
 *               the byte into the fifo. 
 *
 * PARAMETERS:   handle       IN      A handle to the DRB fifo, previously
 *                                    returned by drb_fifo_init.
 *               tx_byte      IN      The data byte to write
 *               timeout      IN      Timeout time in milliseconds, pass
 *                                    a negative value to block until the
 *                                    byte is sent.
 *
 * RETURNS:      0 if the write succeeded otherwise -1
 *
 **********************************************************************/
int
drb_fifo_put_byte (DRB_FIFO_HANDLE handle, uint8_t tx_byte, int timeout)
{
	drb_fifo_t* handleP = (drb_fifo_t*) handle;
	uint32_t    rx_word;
	uint32_t    in;
	uint64_t    cur_time;
	uint64_t    start_time;
	int         rd_avail;
	int         wr_avail;
	
	
	
	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_FIFO_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_FIFO_SIGNATURE )
		return -1;

	

	/* get the start time for working out the timeout */
	start_time = ema_get_tick_count();



	/* loop until the tx FIFO has space for an entry */
	in = *(handleP->pci_fifo_ctrl);
	wr_avail = (in & 0x1FF);
	while ( wr_avail == 0x000 )
	{
		
		/* check if there is a byte(s) to be read, and if so buffer it internally */
		rd_avail = ((in & 0x3FE00) >> 9);
		if ( rd_avail )
		{
			do
			{
				rx_word = *(handleP->pci_fifo_data);
				EMA_PVERBOSE (3, "read 0x%08X from FIFO\n", rx_word);
				
				if ( rx_word & 0x04000000)
					drb_fifo_insert_buf_byte (handleP, (uint8_t)((rx_word & 0x00FF0000) >> 16));
				if ( rx_word & 0x02000000)
					drb_fifo_insert_buf_byte (handleP, (uint8_t)((rx_word & 0x0000FF00) >> 8 ));
				if ( rx_word & 0x01000000)
					drb_fifo_insert_buf_byte (handleP, (uint8_t)(rx_word & 0x000000FF));
				
			} while (--rd_avail);
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

		/* sleep then read from the fifo again */
		ema_usleep (100);
		in = *(handleP->pci_fifo_ctrl);
		wr_avail = (in & 0x1FF);
	}

	
	
	/* we can now write the new byte into the fifo */
	*(handleP->pci_fifo_data) = (0x01000000 | tx_byte);
	EMA_PVERBOSE (3, "wrote 0x%08X to FIFO\n", (0x01000000 | tx_byte));

	return 0;
}	
	
	
	


/**********************************************************************
 *
 * FUNCTION:     drb_fifo_put_string
 *
 * DESCRIPTION:  Writes multiple bytes to the FIFO, this is more efficent
 *               than calling drb_fifo_put_byte repeatedly because it allows
 *               for putting multiple bytes per fifo entry.
 *
 * PARAMETERS:   handle       IN      A handle to the DRB fifo, previously
 *                                    returned by drb_fifo_init.
 *               tx_bytes     IN      The data buffer to write to.
 *               length       IN      The number of bytes in the buffer
 *                                    to be sent.
 *               timeout      IN      Timeout time in milliseconds, pass
 *                                    a negative value to block until the
 *                                    byte is sent.
 *
 * RETURNS:      returns the number of bytes sent, which may be less than
 *               the amount requested if there was a timeout.
 *
 **********************************************************************/
int
drb_fifo_put_string (DRB_FIFO_HANDLE handle, const uint8_t *tx_bytes, int length, int timeout)
{
	drb_fifo_t*    handleP = (drb_fifo_t*) handle;
	uint32_t       rx_word;
	uint32_t       tx_word;
	int            bytes_sent;
	int            wr_avail;
	int            rd_avail;
	uint32_t       in;
	uint64_t       cur_time;
	uint64_t       start_time;
	const uint8_t* chp;
	
	
	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_FIFO_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_FIFO_SIGNATURE )
		return -1;

	assert(!((tx_bytes == NULL) && (length != 0)));
	if ( (tx_bytes == NULL) && (length != 0) )
		return -1;

	if ( length == 0 )
		return 0;
	

	
	
	/* get the start time for working out the timeout */
	start_time = ema_get_tick_count();
	
	bytes_sent = 0;
	chp = tx_bytes;

	do
	{

		/* loop until the tx FIFO has space for an entry */
		in = *(handleP->pci_fifo_ctrl);
		wr_avail = (in & 0x1FF);
		while ( wr_avail == 0x000 )
		{
			
			/* check if there is a byte(s) to be read, and if so buffer it internally */
			rd_avail = ((in & 0x3FE00) >> 9);
			if ( rd_avail )
			{
				do
				{
					rx_word = *(handleP->pci_fifo_data);
					EMA_PVERBOSE (3, "read 0x%08X from FIFO\n", rx_word);
					
					if ( rx_word & 0x04000000)
						drb_fifo_insert_buf_byte (handleP, (uint8_t)((rx_word & 0x00FF0000) >> 16));
					if ( rx_word & 0x02000000)
						drb_fifo_insert_buf_byte (handleP, (uint8_t)((rx_word & 0x0000FF00) >> 8 ));
					if ( rx_word & 0x01000000)
						drb_fifo_insert_buf_byte (handleP, (uint8_t)((rx_word & 0x000000FF)));
					
				} while (--rd_avail);
			}
	
	
			/* get the current tick count */
			cur_time = ema_get_tick_count();
			
			/* check if we have timed-out */
			if ( timeout > 0 )
				if ( (cur_time - start_time) >= timeout )
				{
					EMA_PVERBOSE (3, "timed-out before all bytes were sent\n");
					return bytes_sent;
				}
	
			/* sleep then read from the fifo again */
			ema_usleep (100);
			in = *(handleP->pci_fifo_ctrl);
			wr_avail = (in & 0x1FF);
		}

	
		
		/* format the word being written into the FIFO */
		tx_word = 0;
		if ( length > 2 )
		{
			tx_word |= (0x04000000) | ((0x00FF0000) & ((uint32_t)*chp++ << 16));
			bytes_sent++;
			length--;
		}
		if ( length > 1 )
		{
			tx_word |= (0x02000000) | ((0x0000FF00) & ((uint32_t)*chp++ << 8));
			bytes_sent++;
			length--;
		}
		if ( length > 0 )
		{
			tx_word |= (0x01000000) | ((0x000000FF) & ((uint32_t)*chp++));
			bytes_sent++;
			length--;
		}

		
		/* we can now write the new byte(s) into the fifo */
		*(handleP->pci_fifo_data) = tx_word;
		EMA_PVERBOSE (3, "wrote 0x%08X to FIFO\n", tx_word);
		

	} while (length > 0);
	

	return bytes_sent;
}	




/**********************************************************************
 *
 * FUNCTION:     drb_fifo_get_byte
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
 *               and channel / rx_byte are left unchanged.
 *
 **********************************************************************/
int
drb_fifo_get_byte (DRB_FIFO_HANDLE handle, uint8_t* rx_byte)
{
	drb_fifo_t* handleP = (drb_fifo_t*) handle;
	uint32_t    in;
	uint32_t    rx_word;
	int         first;
	int         rd_avail;

	
	/* sanity checking */
	assert(rx_byte != NULL);
	assert(handle  != NULL);
	assert(handleP->signature == DRB_FIFO_SIGNATURE);
	if ( rx_byte == NULL || handle == NULL || handleP->signature != DRB_FIFO_SIGNATURE )
		return -1;

	
	
	
	

	
	/* first check if there is any data in the buffer */
	ENTER_CRITICAL_SECTION (&handleP->mutex);

	if ( handleP->recv_fifo_count > 0 )
	{
		*rx_byte = handleP->recv_fifo_buf[handleP->recv_fifo_tail++];
		if ( handleP->recv_fifo_tail >= DRB_INTERNAL_FIFO_SIZE )
			handleP->recv_fifo_tail = 0;
		
		handleP->recv_fifo_count--;
		
		LEAVE_CRITICAL_SECTION (&handleP->mutex);

		EMA_PVERBOSE (3, "-> read byte from buffer, 0x%02X\n", *rx_byte);
		return 0;
	}
	
	LEAVE_CRITICAL_SECTION (&handleP->mutex);



	/* read the value on the 'in' register */
	in = *(handleP->pci_fifo_ctrl);
	rd_avail = ((in & 0x3FE00) >> 9);
	
	
	/* check if there is a byte(s) to be read */
	if ( rd_avail > 0 )
	{
		rx_word = *(handleP->pci_fifo_data);
		EMA_PVERBOSE (3, "read 0x%08X from FIFO\n", rx_word);
		
		/* the first byte we received is returned, the rest are buffered internally */
		first = 1;
		
		if ( rx_word & 0x04000000)
		{
			if ( !first )
				drb_fifo_insert_buf_byte (handleP, (uint8_t)((rx_word & 0x00FF0000) >> 16));
			else
			{
				*rx_byte = (uint8_t)((rx_word & 0x00FF0000) >> 16);
				first = 0;
			}
		}
		
		if ( rx_word & 0x02000000)
		{
			if ( !first )
				drb_fifo_insert_buf_byte (handleP, (uint8_t)((rx_word & 0x0000FF00) >> 8));
			else
			{
				*rx_byte = (uint8_t)((rx_word & 0x0000FF00) >> 8);
				first = 0;
			}
		}

		if ( rx_word & 0x01000000)
		{
			if ( !first )
				drb_fifo_insert_buf_byte (handleP, (uint8_t)(rx_word & 0x000000FF));
			else
			{
				*rx_byte = (uint8_t)((rx_word & 0x000000FF));
				first = 0;
			}
		}
		
		return 0;
	}
	
	
	return -1;
}







/**********************************************************************
 *
 * FUNCTION:     drb_fifo_flush_recv
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
drb_fifo_flush_recv (DRB_FIFO_HANDLE handle, int timeout)
{
	drb_fifo_t* handleP = (drb_fifo_t*) handle;
	uint32_t    rx_word;
	uint32_t    in;
	int         rd_avail;
	uint64_t    cur_time;
	uint64_t    start_time;
	int         idle;

	

	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_FIFO_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_FIFO_SIGNATURE )
		return -1;


	/* first check if there is any data in the internal buffer */
	ENTER_CRITICAL_SECTION (&handleP->mutex);

	while ( handleP->recv_fifo_count > 0 )
	{
		EMA_PVERBOSE (4, "flushed 0x%02X (from internal buffer)\n", handleP->recv_fifo_buf[handleP->recv_fifo_tail]);
		
		handleP->recv_fifo_tail++;
		handleP->recv_fifo_count--;
		
		if ( handleP->recv_fifo_tail >= DRB_INTERNAL_FIFO_SIZE )
			handleP->recv_fifo_tail = 0;
		
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
			in = *(handleP->pci_fifo_ctrl);
			rd_avail = ((in & 0x3FE00) >> 9);
		
			
			/* check the amount available to be read and then read it */
			if ( rd_avail != 0 )
			{
				do
				{
					rx_word = *(handleP->pci_fifo_data);
					EMA_PVERBOSE (4, "flushed 0x%08X (from fifo)\n", rx_word);
					
				} while (--rd_avail);


				/* calculate the next timeout value */
				start_time = cur_time;

				idle = 0;
			}
			

			/* sleep while idle */
			if (idle >= 10)
			{
				ema_usleep (1000);
				idle = 0;
			}
			
		} while ( (cur_time - start_time) < timeout );
	}
			

	return 0;
}
