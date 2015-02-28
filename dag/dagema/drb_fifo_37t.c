/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: drb_fifo_37t.c,v 1.6 2009/04/14 01:56:12 wilson.zhu Exp $
*
**********************************************************************/

/**********************************************************************
 * FILE:         drb_fifo.c
 * DESCRIPTION:  Implements byte send and receive across the drb using
 *               a 511 entry FIFO.
 *
 *               To use, call drb_fifo_init first which returns a handle
 *               that should be used in calls to drb_fifo_put_byte and
 *               drb_fifo_get_byte. To clean up, call drb_fifo_term with
 *               the handle returned by drb_fifo_init.
 *
 * 				 The fifo register is 32bit, it is used as described below
 *               0-7   bits, data truck 1
 *               8-15  bits, data truck 2
 *               16-23 bits, data truck 3
 *               24    bits, truck 1 valid flag
 *               25    bits, truck 2 valid flag
 *               26    bits, truck 3 valid flag
 *               27-29 bits, data channel flag
 *               30-31 bits, not used 
 *               channel flag: 000 ema_msg, 
 *                             001 console, 
 *                             010 debug ??,
 * 		                       011 - 111 for future usage
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
 
#define DRB_FIFO_37T_SIGNATURE          0x57A95F37
#define DRB_FIFO_37T_EMA_CHANNEL		0x0
#define DRB_FIFO_37T_CON_CHANNEL		0x1



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
typedef void (*DRB_37T_CHANNEL_HANDLER)(int flag,DRB_FIFO_HANDLE handleP, uint8_t rx_byte);
typedef struct _drb_fifo_37t
{
	uint32_t           signature;          /* Unique identifies this structure */
	int				   id;			   /* dagfd*/

	volatile char*     iom;                /* Base addr of fifo*/
	volatile uint32_t* pci_fifo_data;      /* DRB fifo data */
	volatile uint32_t* pci_fifo_ctrl;      /* DRB fifo control */
	volatile uint32_t* pci_fifo_clear;     /* DRB fifo clear */
	
	uint8_t*           ema_buf;      /* buffer used to store the received byte(s) during a write */
	uint32_t           ema_buf_head;     /* the head pointer(index) of the buffer (inserts) */
	uint32_t           ema_buf_tail;     /* the tail pointer(index) of the buffer (removals) */
	uint32_t           ema_buf_cnt;    /* the number of bytes in the internal fifo */

	DRB_37T_CHANNEL_HANDLER  channel_handler[8];

#if defined(DAG_CONSOLE)
	console_putchar_t  console_handler;       /* console character handler */
	uint8_t*           console_buf;        /* buffer used to store bytes to be send out the console channel */
	uint32_t           console_buf_full;   /* the amount of data in the console buffer */
#endif	



#if defined(_WIN32)                         /* mutex used for access to the recv buffer above */
	CRITICAL_SECTION   mutex;
#else 
	pthread_mutex_t    mutex;              
#endif

} drb_fifo_37t_t;



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



int dag_37t_tty_br_fd[2];

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
 *
 * Internal Functions
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)
static int drb_fifo_37t_send_console_string (DRB_COMM_HANDLE handle, char* str, int len);
#endif	

static void drb_fifo_37t_ema_buf_handler(int flag, DRB_FIFO_HANDLE handleP, uint8_t rx_byte);
static void drb_fifo_37t_null_handler(int flag, DRB_FIFO_HANDLE handleP, uint8_t rx_byte);
static void drb_fifo_37t_tty_br_handler(int flag, DRB_FIFO_HANDLE handleP, uint8_t rx_byte);
static void drb_fifo_37t_console_handler(int flag, DRB_FIFO_HANDLE handleP, uint8_t rx_byte);

static void drb_fifo_37t_tty_br_init(DRB_FIFO_HANDLE handle);
static int drb_fifo_37t_write_fifo(drb_fifo_37t_t* handleP, uint32_t channel,  
								const uint8_t *tx_bytes, int length, int timeout);
static int drb_fifo_37t_read_fifo(drb_fifo_37t_t* handleP);

/**********************************************************************
 * FUNCTION:     drb_37t_ema_drv_reg
 * DESCRIPTION:  Called by ema library to register 37t drb or fifo to ema lib
 *				 Only register function pointers.
 *				 The function will detect whether the loaded firmware image has fifo
 *				 or not, to select fifo or drb_com driver to use.
 * PARAMETERS:   ema_context_t  IN/OUT      ema context to register.
 * RETURNS:      0 if success, -1 if error
 **********************************************************************/
int drb_37t_ema_drv_reg(ema_context_t* ctx)
{
	dag_reg_t          regs[DAG_REG_MAX_ENTRIES];
	uint8_t* iom_ptr;
	ema_drv_cbf_t cbf;

	iom_ptr = dag_iom(ctx->dagfd);
	if(iom_ptr==NULL)
		return -1;
	if(dag_reg_find((char*)iom_ptr, DAG_REG_AD2D_BRIDGE, regs)<1)
	{
		/*no fifo, use drb instead*/
		cbf.pfunc_drv_flush_recv = drb_comm_flush_recv;
		cbf.pfunc_drv_get_byte = drb_comm_get_byte;
		cbf.pfunc_drv_put_byte = drb_comm_put_byte;
		cbf.pfunc_drv_put_string = drb_comm_put_string;
		cbf.pfunc_drv_init = drb_comm_init;
		cbf.pfunc_drv_term = drb_comm_term;
	}
	else
	{ /* fio*/
		cbf.pfunc_drv_flush_recv = drb_fifo_37t_flush_recv;
		cbf.pfunc_drv_get_byte = drb_fifo_37t_get_byte;
		cbf.pfunc_drv_put_byte = drb_fifo_37t_put_byte;
		cbf.pfunc_drv_put_string = drb_fifo_37t_put_string;
		cbf.pfunc_drv_init = drb_fifo_37t_init;
		cbf.pfunc_drv_term = drb_fifo_37t_term;
	}
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
 * RETURNS:      A handle to the feDRB communications, or NULL if there
 *               was an error.void
drb_fifo_37t_insert_buf_byte (drb_fifo_37t_t* handleP, uint8_t rx_byte)
 *
 **********************************************************************/
DRB_FIFO_HANDLE
drb_fifo_37t_init (int dagfd)
{
	drb_fifo_37t_t* handleP;
	daginf_t*   daginf;
	
	
	/* Ensure we are dealing with a 7.1s */
	daginf = dag_info(dagfd);
	if ( daginf->device_code != PCI_DEVICE_ID_DAG3_70T && 
	     daginf->device_code != PCI_DEVICE_ID_DAG3_7T4 )
	{
		return NULL;
	}


	/* Allocate a struct and return a pointer to it as a handle */
	handleP = malloc (sizeof(drb_fifo_37t_t));
	if ( handleP == NULL )
		return NULL;
	memset (handleP, 0, sizeof(drb_fifo_37t_t));
	

	handleP->signature = DRB_FIFO_37T_SIGNATURE;
	handleP->id = daginf->id;
	handleP->iom       = (char*) dag_iom(dagfd);


	/* The drb registers are card specific */
	handleP->pci_fifo_data  = (volatile uint32_t *)(handleP->iom + 0x410);
	handleP->pci_fifo_ctrl  = (volatile uint32_t *)(handleP->iom + 0x414);
	handleP->pci_fifo_clear = (volatile uint32_t *)(handleP->iom + 0x418);
	
	handleP->channel_handler[0]=drb_fifo_37t_ema_buf_handler;
    handleP->channel_handler[1]=drb_fifo_37t_console_handler; 
	handleP->channel_handler[2]=drb_fifo_37t_tty_br_handler;
	handleP->channel_handler[3]=drb_fifo_37t_tty_br_handler;
	handleP->channel_handler[4]=drb_fifo_37t_null_handler;
	handleP->channel_handler[5]=drb_fifo_37t_null_handler;
	handleP->channel_handler[6]=drb_fifo_37t_null_handler;
	handleP->channel_handler[7]=drb_fifo_37t_null_handler;

	/* allocate a 2k buffer to use to store messages */
	handleP->ema_buf   = malloc (DRB_INTERNAL_FIFO_SIZE);
	handleP->ema_buf_head  = 0;
	handleP->ema_buf_tail  = 0;
	handleP->ema_buf_cnt = 0;

	if ( handleP->ema_buf == NULL )
	{
		free (handleP);
		return NULL;
	}

#if defined(DAG_CONSOLE)
	/* allocate a 2k buffer to store console data */
	handleP->console_buf      = malloc(2048);
	handleP->console_buf_full = 0;
	handleP->console_handler = putchar;	

	if(handleP->console_buf==NULL)
	{
		free(handleP->ema_buf);
		free(handleP);
		
	}
	dagema_console_drv_reg(drb_fifo_37t_send_console_string);
#endif

	drb_fifo_37t_tty_br_init(handleP);

	
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

	return handleP;
}



/**********************************************************************
 *
 * FUNCTION:     drb_fifo_37t_term
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
drb_fifo_37t_term (DRB_FIFO_HANDLE handle)
{
	drb_fifo_37t_t* handleP = (drb_fifo_37t_t*) handle;

	/* sanity checking */
	assert(handleP != NULL);
	if ( handleP == NULL || handleP->signature != DRB_FIFO_37T_SIGNATURE )
		return;
	handleP->signature = 0;
	

	/* destroy the critical section used for buffer access */
#if defined(_WIN32)
	DeleteCriticalSection (&handleP->mutex);
#else
	pthread_mutex_destroy (&handleP->mutex);
#endif
	
	/* clean up */
	free (handleP->ema_buf); 
#if defined(DAG_CONSOLE)
	free (handleP->console_buf);
#endif
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
void
drb_fifo_37t_ema_buf_handler(int flag, DRB_FIFO_HANDLE handle, uint8_t rx_byte)
{
	drb_fifo_37t_t* handleP = (drb_fifo_37t_t*) handle;
	
	ENTER_CRITICAL_SECTION (&handleP->mutex);
	
	/* check if the internal fifo is full */
	if ( handleP->ema_buf_cnt == (DRB_INTERNAL_FIFO_SIZE - 5) )
	{
		LEAVE_CRITICAL_SECTION (&handleP->mutex);
		return;
	}
	
	/* add the byte to the head */
	handleP->ema_buf[handleP->ema_buf_head++] = rx_byte;
	if ( handleP->ema_buf_head >= DRB_INTERNAL_FIFO_SIZE )
		handleP->ema_buf_head = 0;
	
	handleP->ema_buf_cnt++;
	
	LEAVE_CRITICAL_SECTION (&handleP->mutex);

}

void drb_fifo_37t_null_handler(int flag, DRB_FIFO_HANDLE handle, uint8_t rx_byte)
{
	/* do nothing*/
}

void drb_fifo_37t_tty_br_handler(int flag, DRB_FIFO_HANDLE handle, uint8_t rx_byte)
{
	flag -=2;
	if(flag>2||flag<0)
		return ;
	if(dag_37t_tty_br_fd[flag])
	{
		/* assume write will always success*/
		while(1!=write(dag_37t_tty_br_fd[flag], &rx_byte, 1))
		{
			ema_usleep(100);
		}
	}
}

void drb_fifo_37t_tty_br_init(DRB_FIFO_HANDLE handle)
{
	drb_fifo_37t_t* handleP = (drb_fifo_37t_t*) handle;
	int dag_id;
	int idx;
	int loop_base,loop_max;
	int ret;
	char name_buf[32];

	dag_id = handleP->id;
	
	if(dag_id<0||dag_id>=4)
		return;
	loop_base=dag_id*2;
	loop_max = loop_base+2;
	for(idx = loop_base; idx<loop_max;idx++)
	{
		sprintf(name_buf,"/dev/tty_br%d",idx);
		ret = open(name_buf, O_RDWR);
		if(ret>0)
			dag_37t_tty_br_fd[idx-loop_base] = ret;
		else
			dag_37t_tty_br_fd[idx-loop_base] = 0;
	}
}

void drb_fifo_37t_tty_br_send_process(DRB_FIFO_HANDLE handle)
{
	int idx,ret;
	uint8_t buf[512];
	
	for(idx=0;idx<2;idx++)
	{
		if(dag_37t_tty_br_fd[idx])
		{
			ret = read(dag_37t_tty_br_fd[idx], (char*)buf, 511);
			if(ret<=0)
			{
				return;
			}
			drb_fifo_37t_write_fifo(handle, idx+2,  buf, ret, 0);
			return;
		}
	}
	
	return;
}




void drb_fifo_37t_console_handler(int flag, DRB_FIFO_HANDLE handle, uint8_t rx_byte)
{
#if defined(DAG_CONSOLE)
	dagema_console_char_handler((char)rx_byte);
#endif
}


/**********************************************************************
 *
 * FUNCTION:     drb_fifo_37t_put_byte
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
drb_fifo_37t_put_byte (DRB_FIFO_HANDLE handle, uint8_t tx_byte, int timeout)
{
	drb_fifo_37t_t* handleP = (drb_fifo_37t_t*) handle;
	
	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_FIFO_37T_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_FIFO_37T_SIGNATURE )
		return -1;

	if(1!=drb_fifo_37t_write_fifo(handleP, DRB_FIFO_37T_EMA_CHANNEL,  &tx_byte, 1, timeout))
		return -1;

	return 0;
}	
	
	
	


/**********************************************************************
 *
 * FUNCTION:     drb_fifo_37t_put_string
 *
 * DESCRIPTION:  Writes multiple bytes to the FIFO, this is more efficent
 *               than calling drb_fifo_37t_put_byte repeatedly because it allows
 *               for putting multiple bytes per fifo entry.
 *
 * PARAMETERS:   handle       IN      A handle to the DRB fifo, previously
 *                                    returned by drb_fifo_37t_init.
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
drb_fifo_37t_put_string (DRB_FIFO_HANDLE handle, const uint8_t *tx_bytes, int length, int timeout)
{
	drb_fifo_37t_t*    handleP = (drb_fifo_37t_t*) handle;
	
	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_FIFO_37T_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_FIFO_37T_SIGNATURE )
		return -1;

	assert(!((tx_bytes == NULL) && (length != 0)));
	if ( (tx_bytes == NULL) && (length != 0) )
		return -1;

	if ( length == 0 )
		return 0;
	
	return drb_fifo_37t_write_fifo(handleP, DRB_FIFO_37T_EMA_CHANNEL, tx_bytes, 1, timeout);

	
}	




/**********************************************************************
 *
 * FUNCTION:     drb_fifo_37t_get_byte
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
drb_fifo_37t_get_byte (DRB_FIFO_HANDLE handle, uint8_t* rx_byte)
{
	drb_fifo_37t_t* handleP = (drb_fifo_37t_t*) handle;

	
	/* sanity checking */
	assert(rx_byte != NULL);
	assert(handle  != NULL);
	assert(handleP->signature == DRB_FIFO_37T_SIGNATURE);
	if ( rx_byte == NULL || handle == NULL || handleP->signature != DRB_FIFO_37T_SIGNATURE )
		return -1;

	/*first read any data available to buffer*/
	drb_fifo_37t_read_fifo(handleP);
	
	/* first check if there is any data in the buffer */
	ENTER_CRITICAL_SECTION (&handleP->mutex);

	if ( handleP->ema_buf_cnt > 0 )
	{
		*rx_byte = handleP->ema_buf[handleP->ema_buf_tail++];
		if ( handleP->ema_buf_tail >= DRB_INTERNAL_FIFO_SIZE )
			handleP->ema_buf_tail = 0;
		
		handleP->ema_buf_cnt--;
		
		LEAVE_CRITICAL_SECTION (&handleP->mutex);

		EMA_PVERBOSE (3, "-> read byte from buffer, 0x%02X\n", *rx_byte);
		return 0;
	}
	
	LEAVE_CRITICAL_SECTION (&handleP->mutex);

	drb_fifo_37t_tty_br_send_process(handle);


	
	
	return -1;
}


int drb_fifo_37t_read_fifo(drb_fifo_37t_t* handleP)
{
	/* read the value on the 'in' register */
	uint32_t rx_word;
	int32_t rd_avail;
	uint32_t channel_flag;
	DRB_37T_CHANNEL_HANDLER handle_func;
	
	rx_word = *(handleP->pci_fifo_ctrl);
	rd_avail = (rx_word & 0x000001FF);
	while(rd_avail>0)
	{
		rx_word = *(handleP->pci_fifo_data);
		
		channel_flag = (rx_word>>27)&0x7; /* truck flag 1*/
		
		if(channel_flag!=1&&channel_flag!=7)
		{
			EMA_PVERBOSE (3, "read 0x%08X from FIFO\n", rx_word);
		}

		handle_func = handleP->channel_handler[channel_flag];
		
		if(rx_word&0x01000000)
			handle_func(channel_flag,(DRB_FIFO_HANDLE)handleP,(unsigned char)(rx_word&0x000000FF));
		if(rx_word&0x02000000)
			handle_func(channel_flag,(DRB_FIFO_HANDLE)handleP,(unsigned char)((rx_word&0x0000FF00)>>8));
		if(rx_word&0x04000000)
			handle_func(channel_flag,(DRB_FIFO_HANDLE)handleP,(unsigned char)((rx_word&0x00FF0000)>>16));

		rd_avail--;
	}
	return 0;
}

int drb_fifo_37t_write_fifo(drb_fifo_37t_t* handleP, uint32_t channel,  const uint8_t *tx_bytes, int length, int timeout)
{
	uint32_t       tx_word;
	int            bytes_sent;
	int            wr_avail;
	uint32_t       in;
	uint64_t       cur_time = 0;
	uint64_t       start_time = 0;
	uint8_t* 	   chp;
	uint32_t       channel_flag = (channel&0x00000007)<<27;

	/* internal function, no sanity checking */
	
	/* get the start time for working out the timeout */
	if(timeout>0)
		start_time = ema_get_tick_count();
	
	bytes_sent = 0;
	chp = (uint8_t*)tx_bytes;

	ENTER_CRITICAL_SECTION (&handleP->mutex);
	while(1)
	{
		/* loop until the tx FIFO has space for an entry */
		in = *(handleP->pci_fifo_ctrl);
		wr_avail = (in >>9)& 0x1FF;

		while ( wr_avail == 0x000 )
		{
			/* check if we have timed-out */
			if ( timeout > 0 )
			{
				/* get the current tick count */
				cur_time = ema_get_tick_count();
				if ( (cur_time - start_time) >= timeout )
				{
					EMA_PVERBOSE (3, "timed-out before all bytes were sent\n");
					LEAVE_CRITICAL_SECTION (&handleP->mutex);
					return bytes_sent;
				}
			}
			/* sleep then read from the fifo again */
			ema_usleep (100);
			in = *(handleP->pci_fifo_ctrl);
			wr_avail = (in>>9) & 0x1FF;
		}
		while(wr_avail>0)
		{
			/* format the word being written into the FIFO */
			if ( length > 2 )
			{
				tx_word = channel_flag;
				tx_word |= (0x04000000) | ((0x00FF0000) & ((uint32_t)(chp[2]) << 16));
				tx_word |= (0x02000000) | ((0x0000FF00) & ((uint32_t)(chp[1]) << 8));
				tx_word |= (0x01000000) | ((0x000000FF) & ((uint32_t)(chp[0])));
				bytes_sent+=3;
				chp+=3;
				length-=3;
				/* we can now write the new byte(s) into the fifo */
				*(handleP->pci_fifo_data) = tx_word;
				EMA_PVERBOSE (3, "wrote 0x%08X to FIFO\n", tx_word);
				wr_avail--;
			}
			else if ( length ==2 )
			{
				tx_word = channel_flag;
				tx_word |= (0x02000000) | ((0x0000FF00) & ((uint32_t)(chp[1]) << 8));
				tx_word |= (0x01000000) | ((0x000000FF) & ((uint32_t)(chp[0])));
				bytes_sent+=2;
				chp+=2;
				length-=2;
				/* we can now write the new byte(s) into the fifo */
				*(handleP->pci_fifo_data) = tx_word;
				EMA_PVERBOSE (3, "wrote 0x%08X to FIFO\n", tx_word);
				wr_avail--;
			}
			else if ( length ==1 )
			{
				tx_word = channel_flag;
				tx_word |= (0x01000000) | ((0x000000FF) & ((uint32_t)(chp[0])));
				bytes_sent++;
				chp++;
				length--;
				/* we can now write the new byte(s) into the fifo */
				*(handleP->pci_fifo_data) = tx_word;
				wr_avail--;
				EMA_PVERBOSE (3, "wrote 0x%08X to FIFO\n", tx_word);
			}
			if(length==0)
			{
				/*   *(handleP->pci_fifo_data) = 0x38000000;  dump data to push data out, */
				LEAVE_CRITICAL_SECTION (&handleP->mutex);
				return bytes_sent;
			}
		}

	}

/*	*(handleP->pci_fifo_data) = 0x38000000;  dump data to push data out, */
	LEAVE_CRITICAL_SECTION (&handleP->mutex);
	return bytes_sent;
}




/**********************************************************************
 *
 * FUNCTION:     drb_fifo_37t_flush_recv
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
drb_fifo_37t_flush_recv (DRB_FIFO_HANDLE handle, int timeout)
{
	drb_fifo_37t_t* handleP = (drb_fifo_37t_t*) handle;
	uint32_t    rx_word;
	uint32_t    in;
	int         rd_avail;
	uint64_t    cur_time;
	uint64_t    start_time;
	int         idle;

	

	/* sanity checking */
	assert(handleP != NULL);
	assert(handleP->signature == DRB_FIFO_37T_SIGNATURE);
	if ( handle == NULL || handleP->signature != DRB_FIFO_37T_SIGNATURE )
		return -1;


	/* first check if there is any data in the internal buffer */
	ENTER_CRITICAL_SECTION (&handleP->mutex);

	while ( handleP->ema_buf_cnt > 0 )
	{
		EMA_PVERBOSE (4, "flushed 0x%02X (from internal buffer)\n", handleP->ema_buf[handleP->ema_buf_tail]);
		
		handleP->ema_buf_tail++;
		handleP->ema_buf_cnt--;
		
		if ( handleP->ema_buf_tail >= DRB_INTERNAL_FIFO_SIZE )
			handleP->ema_buf_tail = 0;
		
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
			
			cur_time = ema_get_tick_count();	/* Add the character to the console buffer to send */

			/* read the value on the 'in' register */
			in = *(handleP->pci_fifo_ctrl);
			rd_avail = (in & 0x1FF);
		
			
			/* check the amount available to be read and then read it */
			if ( rd_avail != 0 )
			{
				while(rd_avail>0)
				{
					rx_word = *(handleP->pci_fifo_data);
					EMA_PVERBOSE (4, "flushed 0x%08X (from fifo)\n", rx_word);
					rd_avail--;
				}


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

/**********************************************************************
 *
 * FUNCTION:     drb_fifo_37t_send_console_string
 *
 * DESCRIPTION:  Sends a console character out the 37t fifo
 *
 * PARAMETERS:   handle       IN      A handle to the DRB comm, previously
 *                                    returned by fifo_37t_comm_init.
 *               str          IN      Characters to send
 *               len          IN      Number of characters
 *
 * RETURNS:      0 if the write succeeded otherwise -1
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)
int
drb_fifo_37t_send_console_string (DRB_COMM_HANDLE handle, char* str, int len)
{
	drb_fifo_37t_t* handleP = (drb_fifo_37t_t*) handle;

	/* sanity checking */
	if ( handle == NULL || handleP->signature != DRB_FIFO_37T_SIGNATURE )
		return -1;
		
	drb_fifo_37t_write_fifo(handleP, DRB_FIFO_37T_CON_CHANNEL, (uint8_t*)str, len, -1);
	return 0;
}
#endif	

