/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: ema_types.h,v 1.18 2008/09/01 08:16:20 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         ema_types.h
* DESCRIPTION:  Types for the Embedded Messaging API
*
* HISTORY:      18-11-05 Initial revision
*
**********************************************************************/
#ifndef EMA_TYPES_H
#define EMA_TYPES_H

#if !defined(DAGEMA_H)
	#error "include dagema.h prior to including ema_types.h"
#endif



#include "dag_platform.h"
#include "dagutil.h"





/**********************************************************************
 *
 * Debugging
 *
 */


/* Sets the verbosity output in non-windows versions of the library */
#define EMA_VERBOSE_LEVEL	-1

#if (EMA_VERBOSE_LEVEL > 0) 

	/* Verbose library output, used for debugging */
	#if (defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__)))
		#define EMA_PVERBOSE(level, args...) \
			if (EMA_VERBOSE_LEVEL >= level) { \
			  fprintf(stderr, "%s:%s() line %d -- ", __FILE__, __FUNCTION__, __LINE__); \
			  fprintf(stderr, args); \
			  fflush(stderr); \
			}
	#elif defined(_WIN32)
		static void EMA_PVERBOSE(int level, const char *format, ...)
		{
			va_list marker;
			va_start( marker, level );     /* Initialize variable arguments. */

			if (EMA_VERBOSE_LEVEL >= level) {
				vfprintf(stderr, format, marker);
				fflush(stderr); 
			}
		}
	#endif


	/* dumps the location to stdout */
	#define TRACE \
		do { \
			fprintf(stderr, "%s: %s(): line %d\n", __FILE__, __FUNCTION__, __LINE__); \
			fflush(stderr); \
		} while(0);



	   
#else
	
	#define TRACE

	#if defined(_WIN32)
		static void EMA_PVERBOSE(int level, const char *format, ...) { }
	#else
		#define EMA_PVERBOSE(level, args...)
	#endif
	   
#endif
   




/**********************************************************************
 *
 * Constants
 *
 */

   

#define DAGEMA_USE_SELECT           1
   
   

/* DRB specific constants */
#define DRB_INIT_RECV_BUFFER_SIZE   (EMA_MSG_PAYLOAD_LENGTH + EMA_MSG_HEADER_LENGTH)
#define DRB_RECV_BUFFER_INCR        (EMA_MSG_PAYLOAD_LENGTH + EMA_MSG_HEADER_LENGTH)
#define DRB_COMM_SIGNATURE          0xA60E0DD5

#define DRB_INTERNAL_FIFO_SIZE      1024
#define DRB_FIFO_SIGNATURE          0x57A95F2A




/* socket port for TCP connections */
#define EMA_PORT                    0xEACE






/* define the handle to the drb communications */
typedef void * DRB_COMM_HANDLE;
typedef void * DRB_FIFO_HANDLE;




/* Defined in windows but no other OS */
#if !defined(INVALID_SOCKET)
	#define INVALID_SOCKET    -1
#endif

#if !defined(SOCKET_ERROR)
	#define SOCKET_ERROR      -1
#endif


#if defined(_WIN32)
	typedef SOCKET    socket_t;
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	typedef int       socket_t;
#endif







/**********************************************************************
 *
 * Constants
 *
 */

#define	EMA_MSG_PAYLOAD_LENGTH          2048
#define EMA_MSG_HEADER_LENGTH           8

/**********************************************************************
 *
 * The EMA message structure
 *
 */
 
typedef union _ema_msg_t
{
	struct {
		/* message header */
		uint32_t     message_id;         /* little endian byte ordering */
		uint16_t     length;             /* little endian byte ordering */
		uint8_t      version;
		uint8_t      trans_id;
		
		/* message payload */
		uint8_t payload[EMA_MSG_PAYLOAD_LENGTH];
	} data;
	
	uint8_t datum[EMA_MSG_HEADER_LENGTH + EMA_MSG_PAYLOAD_LENGTH];
	
} ema_msg_t;






/*********************************************************************
 *
 * The structure that stores the sockets
 *
 */
typedef struct _ema_sockets_t
{
	socket_t client_sock;
	socket_t server_sock; 
} ema_sockets_t;


#ifdef DAGEMA_USE_SELECT

typedef struct _ema_poll_set_t
{
	int max_sd;
	fd_set read;
	fd_set write;
	fd_set exception;
	
} ema_poll_set_t;

#endif



typedef void * EMA_DRV_HANDLE;

typedef struct _ema_drv_reg_s
{
	EMA_DRV_HANDLE (*pfunc_drv_init)(int);					/*init function, func(dag_fd)*/
	int (*pfunc_drv_get_byte) (EMA_DRV_HANDLE, uint8_t*);     /*recv function, func(handle,recv_byte_buf)*/
	int (*pfunc_drv_put_byte) (EMA_DRV_HANDLE, uint8_t, int); /*send function, func(handle,send_byte,time_out)*/
	int (*pfunc_drv_put_string) (EMA_DRV_HANDLE, const uint8_t*, int, int); /*send mutil-bytes function, func(handle,send_ptr,len,time_out)*/
	
	int (*pfunc_drv_flush_recv) (EMA_DRV_HANDLE, int); /*recv buf flush function, func(handle,time_out)*/
	void (*pfunc_drv_term) (EMA_DRV_HANDLE);   /*destruct function, func(handle)*/
}ema_drv_cbf_t;


/**********************************************************************
 *
 * Context storing information about an open EMA dag card
 *
 */
typedef struct _ema_context_t ema_context_t;
struct _ema_context_t
{
	int                dagfd;             /* the card file descriptor */
	uint32_t           dag_id;            /* the id of the dag card (returned by dag_info) */
	uint32_t           device_code;       /* holds the device code of the dag card (returned by dag_info) */

	EMA_DRV_HANDLE	   drv_handle;

	ema_drv_cbf_t	   drv_cbf;

	int                abort_all_trans;   /* if run_thread = 0 this field specifies if all transactions should be aborted */
	int                run_thread;        /* controls the state of the thread 1= running 0= not running */
	int                thread_running;    /* holds the status of the thread 1= running 0= not running */

	socket_t           server_sock;       /* socket for dagconsole communication mode */
	socket_t           client_sock;
	struct sockaddr_un sock_addr;
	int                msg_version;

	msg_handler_t      message_handler;   /* callback function to use for received messages */
#if defined(DAG_CONSOLE)
	ctrl_msg_handler_t ctrl_msg_handler; /* callback function used to process internal control messages */
#endif
	
	uint32_t           client_type;       /* embedded client application type */
	uint32_t           client_version;    /* embedded client application version */
	
	/* locking handles */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	int                lock_file;
#else
	HANDLE             hLockMutex;
#endif	
	
	uint8_t            current_trans_id; /* the current transaction id, incrimented on each sent packet */
	
	/* next item list pointer */
	struct _ema_context_t * next_ctx;
};






/**********************************************************************
 *
 * Internal functions
 *
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	/* dagema.c */
	#if defined(_WIN32)
	extern SOCKET ema_connect_to_server(const char *sockname, int dagid);
	#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	extern int ema_connect_to_server(const char *sockname, int dagid);
	#endif
	
	
	
	/* ema_lock.c */
	extern int lock_ema(ema_context_t* ctx);
	extern int unlock_ema(ema_context_t* ctx);
	
	
	/* ema_thread.c */
	extern int ema_initialise_messaging(ema_context_t *ctx);
	extern int ema_terminate_messaging(ema_context_t *ctx, uint32_t flags);
	
	
	/* ema_reset.c */
	extern int ema_reset_dag3_7t(int dagfd, char* iom, uint32_t flags, reset_handler_t handler);
	extern int ema_reset_dag7_1s(int dagfd, char* iom, uint32_t flags, reset_handler_t handler);
	extern int ema_halt_dag3_7t(int dagfd, char* iom);
	extern int ema_halt_dag7_1s(int dagfd, char* iom);
	
	/* drb_comm.c */
	extern DRB_COMM_HANDLE drb_comm_init (int dagfd);
	extern void drb_comm_term (DRB_COMM_HANDLE handle);
	extern int drb_comm_get_byte (DRB_COMM_HANDLE handle, uint8_t* rx_byte);
	extern int drb_comm_put_byte (DRB_COMM_HANDLE handle, uint8_t rx_byte, int timeout);
	extern int drb_comm_put_string (DRB_COMM_HANDLE handle, const uint8_t *tx_bytes, int length, int timeout);
	extern int drb_comm_flush_recv (DRB_COMM_HANDLE handle, int timeout);
	extern void ema_usleep(int usec);
	extern uint64_t ema_get_tick_count(void);
	
	
	extern int ema_drv_cbf_reg(ema_context_t* ctx, ema_drv_cbf_t* cbf);
	extern int drb_fifo_ema_drv_reg(ema_context_t* ctx);
	extern int drb_37t_ema_drv_reg(ema_context_t* ctx);
	

	
	/* drb_fifo.c */
	extern DRB_FIFO_HANDLE drb_fifo_init (int dagfd);
	extern void drb_fifo_term (DRB_FIFO_HANDLE handle);
	extern int drb_fifo_get_byte (DRB_FIFO_HANDLE handle, uint8_t* rx_byte);
	extern int drb_fifo_put_byte (DRB_FIFO_HANDLE handle, uint8_t rx_byte, int timeout);
	extern int drb_fifo_put_string (DRB_FIFO_HANDLE handle, const uint8_t *tx_bytes, int length, int timeout);
	extern int drb_fifo_flush_recv (DRB_FIFO_HANDLE handle, int timeout);
	
	/* drb_fifo_37t.c*/
	extern DRB_FIFO_HANDLE drb_fifo_37t_init (int dagfd);
	extern void drb_fifo_37t_term (DRB_FIFO_HANDLE handle);
	extern int drb_fifo_37t_get_byte (DRB_FIFO_HANDLE handle, uint8_t* rx_byte);
	extern int drb_fifo_37t_put_byte (DRB_FIFO_HANDLE handle, uint8_t rx_byte, int timeout);
	extern int drb_fifo_37t_put_string (DRB_FIFO_HANDLE handle, const uint8_t *tx_bytes, int length, int timeout);
	extern int drb_fifo_37t_flush_recv (DRB_FIFO_HANDLE handle, int timeout);
	#if defined(DAG_CONSOLE)
	extern int dagema_console_drv_reg(int (*p_send) (DRB_COMM_HANDLE, char*, int));
	extern int dagema_console_char_handler(int c);
	#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */




#endif // EMA_TYPES_H
