/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagema.h,v 1.12.4.1 2009/08/14 01:56:49 wilson.zhu Exp $
*
**********************************************************************/

/** \defgroup EMAPI Embedded Messaging (EM) API
Interface functions of the Embedded Messaging (EM) API
*/
/*@{*/

/**********************************************************************
* FILE:         dagema.h
* DESCRIPTION:  Embedded messaging API library for the dag3.7t & dag7.1s
*
*
* HISTORY:      18-11-05 Initial revision
*
**********************************************************************/

#ifndef DAGEMA_H
#define DAGEMA_H


#ifdef _WIN32
#include <wintypedefs.h>
#else /* !_WIN32 */
#include <inttypes.h>
#endif /* _WIN32 */




/** \ingroup EMAPI
 */
/*@{*/

/**********************************************************************
 *
 * Universal Message ID types
 *
 * Message addressing classes
 * Allows dispatch of messages to handlers.
 *********************************************************************/

#define EMA_CLASS_CONTROL   0x10000000
#define EMA_CLASS_I2C       0x10010000
#define EMA_CLASS_IMA       0x10020000
#define EMA_CLASS_AAL       0x10030000
#define EMA_CLASS_IXPFILTER 0x10040000
#define EMA_CLASS_DISPATCHER 0x10050000

//0x1006 left intensionaly for other uses 

#define EMA_CLASS_HDLC_FILTER		0x10070000
#define EMA_CLASS_ATM_FILTER		0x10080000
#define EMA_CLASS_POSTAAL_FILTER	0x10090000

/* This class is only used for internal control messaging on the host */
#define EMA_CLASS_INTERNAL  0xFFFE0000

#define EMA_CLASS_MASK      0xFFFF0000


/* Control Messages */
#define EMA_MSG_ERROR                 (EMA_CLASS_CONTROL | 0x0001)

#define EMA_MSG_READ_VERSION          (EMA_CLASS_CONTROL | 0x0005)
#define EMA_MSG_READ_VERSION_CMPLT    (EMA_CLASS_CONTROL | 0x0006)

#define EMA_MSG_PING                  (EMA_CLASS_CONTROL | 0x000A)
#define EMA_MSG_PING_CMPLT            (EMA_CLASS_CONTROL | 0x000B)


/*****************************************************************
 * return error codes 
 *****************************************************************/
#define EMA_ERROR_NONE                 0   /* Success */
#define EMA_ERROR_UNKNOWN_MSG          1   /* Message ID is not recognized */

/******************************************************************
 * Possible error code
 ******************************************************************/

#ifndef ENONE 
#define ENONE 0                  /** the operation was successful */
#endif

#ifndef ETIMEDOUT 
#define ETIMEDOUT 110            /** Connection timed out */
#endif

#ifndef ELOCKED 
#define ELOCKED 0x1000           /** the operation failed, because card is locked */
#endif

#ifndef ECARDNOTSUPPORTED 
#define ECARDNOTSUPPORTED 0x1001 /** request is not supported by the card */
#endif

#ifndef ECARDMEMERROR 
#define ECARDMEMERROR 0x1002     /** there was a problem with the memory for IXP/XScsale on boot */
#endif

#ifndef ERESP
#define ERESP 0x1003             /** no response or invalid response from card */
#endif

/***********************************************************************
 * Embedded Messaging API constants
 **********************************************************************/
#define EMA_RUN_DRAM_MEMORY_TEST             0x00000001   /* for both the d3.7t & d7.1s */
#define EMA_RUN_CPP_DRAM_MEMORY_TEST         0x00000002   /* for d7.1s only             */


#define EMA_CLOSE_NO_FLUSH                   0x00000001
/*@}*/

/** \ingroup EMAPI
 */
/*@{*/

/** 
 * error code return message 
 */
typedef struct {
	uint32_t error_code;
	uint32_t data[8];		/* error-specific diagnostic codes */
} ema_msg_error_t;


/**
 * request the embedded software version 
 */
typedef struct {
	uint32_t request;
} ema_msg_read_version_t;

/** 
 * response to the request for the software version 
 */
typedef struct {
	uint32_t result;
	uint32_t version;
	uint32_t type;
} ema_msg_read_version_cmplt_t;


typedef struct {
	uint32_t length;        /* The number of bytes being sent */
	uint8_t data[0];        /* Variable length ping bytes */
} ema_msg_ping_t;

typedef struct {
	uint32_t length;        /* The number of bytes being echo'ed */
	uint8_t data[0];        /* Variable length ping bytes echo'ed */
} ema_msg_ping_cmplt_t;

/* Possible reset stages */
typedef enum {
	EMA_RST_INIT = 0x1000,
	EMA_RST_BOOTLOADER_STARTED,
	EMA_RST_MEMORY_INIT,
	EMA_RST_STARTING_MEM_TEST,
	EMA_RST_FINISHED_MEM_TEST,
	EMA_RST_KERNEL_BOOTED,
	EMA_RST_DRIVER_STARTED,
	EMA_RST_COMPLETE,

} DAGEMA_RST_STAGES;
/*@}*/



/**********************************************************************
 *
 * Embedded Messaging API types
 *
 **********************************************************************/
typedef void (*msg_handler_t)(uint32_t message_id, uint8_t *rx_message, uint32_t length, uint8_t trans_id);

typedef int (*reset_handler_t)(uint32_t stage);

typedef void (*ctrl_msg_handler_t)(uint32_t message_id, uint8_t *rx_message, uint32_t length, uint8_t trans_id);



/**********************************************************************
 *
 * Console character handler callback
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)
	typedef int (*console_putchar_t)(int c);
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**********************************************************************
 *
 * Embedded Messaging API functions
 *
 ***********************************************************************/
/** \defgroup EMUF EM Utility Functions
 */
/*@{*/
/** 
 * @brief	Conversion functions, from 16 bits little endian to host byte
 *
 * @param[in] little16 	a 16 bits little endian.
 * @return		a 16 bits little endian converted value.
 * 
 */
extern uint16_t
dagema_le16toh(uint16_t little16);

/** 
 * @brief	Conversion functions, from 32 bits little endian to host byte
 *
 * @param[in] little32 	a 32 bits little endian.
 * @return		a 32 bits little endian converted value.
 * 
 */
extern uint32_t
dagema_le32toh(uint32_t little32);

/** 
 * @brief	Conversion functions, from host byte to a 16 bits 
 * 		little endian value
 *
 * @param[in] little16 	a 16 bits little endian.
 * @return		a 16 bits little endian converted value.
 * 
 */
extern uint16_t
dagema_htole16(uint16_t host16);

/** 
 * @brief	Conversion functions, from host byte to a 32 bits 
 * 		little endian value
 *
 * @param[in] little32 	a 32 bits little endian.
 * @return		a 32 bits little endian converted value.
 * 
 */
extern uint32_t
dagema_htole32(uint32_t host32);	
/*@}*/

/** \defgroup EMEF EM Error Functions 
  */	
/*@{*/

/** 
 * @brief	Returns the last error code generated by a function
 * 		call, use this whenever one of the functions return -1.
 *
 * @return	the last error code generated.
 * 
 */
extern int
dagema_get_last_error (void);
/*@}*/


/** \defgroup EMCF EM Control functions 
*/
/*@{*/

/** 
 * @brief	Opens a connection to the dag card, this creates a 
 * 		server socket and an associated thread to receive and 
 * 		transmit messages.
 * 		An open connection must be closed with a call to 
 * 		dagema_close_conn(). Only one connection at a time can 
 * 		be opened to a dagcard.
 *
 * @param[in] dagfd	A unix style file descriptor returned by 
 * 			dag_open. This file descriptor must stay 
 * 			valid until the connection is closed, failure
 * 			to do so will result in undefined behaviour.
 *
 * @return		0 if the connection was opened, otherwise -1. 
 * 			call dagema_get_last_error() to retrieve the
 * 			the error code.
 * 
 */
extern int
dagema_open_conn (int dagfd);

/**
 * @brief 	Closes the connection to the dag card, shutdowns the
 * 		server socket and kills the monitor thread.
 *
 * @param[in]	dagfd	A unix style file descriptor returned by dag_open. 
 * 			This must be the same file descriptor past to
 * 			dagema_open_conn().
 * @param[in]	flags	Optional close flags multiple flags can be 
 * 			OR'ed together. See the notes for possible values.
 *
 * @return		0 if the connection was closed, otherwise -1. Call
 *			dagema_get_last_error() to retrieve the error code.
 *
 * @note		Possible flags (mulitple can be OR'ed together)
 * 			EMA_SHUTDOWN_HARD  Closes the link immediately without
 * 			waiting for queued messages to be transmitted to 
 * 			the xScale. If this flag is used it is recomended that
 * 			the xScale be reset before opening another connection.
 *
 */
extern int
dagema_close_conn (int dagfd, uint32_t flags);

/**
 * 
 *
 * @brief  	Resets the embedded processor, the reset process may take
 *              upto 2 minutes depending on whether or not memory tests
 * 		are performed or not. This function blocks while the reset
 *              is in progress. This function will fail if a EMA connection
 *              is already open.
 *
 * @param[in] 	dagfd	A unix style file descriptor returned by dag_open.
 * @param[in] 	flags   Optional flags used to indicate how the boot process 
 * 			should run.
 *                      Possible values are:
 *                      - EMA_RUN_SRAM_MEMORY_TEST
 *                      - EMA_RUN_DRAM_MEMORY_TEST
 *
 * @return		0 if the reset was successiful, otherwise -1. Call
 *              	dagema_get_last_error() to retrieve the error code.
 *
 **/
extern int
dagema_reset_processor (int dagfd, uint32_t flags);

/**
 *
 * @brief     	 Resets the embedded processor, same as the dagema_reset_processor
 *               function except with the additional option of installing
 *               a progress handler that receives notifications during
 *               the reset process.
 *
 * @param[in]	dagfd	A unix style file descriptor returned by dag_open.
 * @param[in]	flags	Optional flags used to indicate
 *                      how the boot process should run.
 *                      Possible values are:
 *                      - EMA_RUN_SRAM_MEMORY_TEST
 *                      - EMA_RUN_DRAM_MEMORY_TEST
 * @param[in]	handler	A hanler that receives noifications during the reset
 * 			process
 *
 * @return      	0 if the reset was successiful, otherwise -1. Call
 *              	dagema_get_last_error() to retrieve the error code.
 *
 **/	
extern int
dagema_reset_processor_with_cb (int dagfd, uint32_t flags, reset_handler_t handler);

/**
 *
 * @brief	 Halts the embedded processor, the process remains in
 *               a halted state until dagema_reset_processor is called.
 *               All open EMA connections must be closed prior to calling
 *               this function.
 *
 * @param[in]	dagfd	A unix style file descriptor returned by dag_open.
 *
 * @return		0 if the halt was successiful, otherwise -1. Call
 *               	dagema_get_last_error() to retrieve the error code.
 *
 **/	
extern int
dagema_halt_processor (int dagfd);
/*@}*/

/** \defgroup EMMC EM Messaging functions 
 */
/*@{*/
/**
 *
 * @brief	Call to send a message to the embedded processor, a
 *              connection should have already been opened prior to
 *              calling this function.
 *
 * @param[in]	dagfd		A unix style file descriptor returned 
 * 				by dag_open.
 * @param[in]	message_id	The ID of the message to send
 * @param[in]	length		The number of bytes in the message,
 * 				sizeof(tx_message)
 * @param[in]	tx_message	Buffer of at least 'length' size 
 * 				containing the data to send.
 *
 * @return			0 if message was sent, otherwise -1. 
 * 				Call dagema_get_last_error() to retrieve 
 * 				the error code.
 *
 * @note	This functions packages the message into a ema_msg_t which
 *              includes the message_id, length and version, this is the
 *              format sent to the embedded processor. 
 *
 */
extern int
dagema_send_msg (int dagfd, uint32_t message_id, uint32_t length, const uint8_t *tx_message, uint8_t *trans_id);

/**
 *  
 * @brief	Call to poll for messages from the embedded processor,
 *              this function blocks while waiting for a message. You
 *              might want to consider dagema_recv_msg_timeout() or installing
 *              a callback message handler with dagema_set_msg_handler().
 *
 * @param[in]	dagfd		A unix style file descriptor returned 
 * 				by dag_open.
 * @param[in]	message_id	Pointer to a value that receives
 *                              the message id.
 * @param[in]	length		Pointer to a value that upon entry
 *				should contain the maximum size
 *                		of the rx_message buffer, upon return
 *                              will contain the number of bytes
 *                              copied into rx_message.
 * @param[out]	rx_message	Buffer to store the message bytes
 *                              in. If the buffer isn't big enough
 *                    		for all the data, it is trimed to
 *                              fit and the overflow is discarded.
 * @param[out]	trans_id	The transaction ID of the received
 *                              message, this can be NULL if the
 *                              transaction ID is not used.
 *
 * @return			0 if message was received, otherwise -1. 
 * 				Call dagema_get_last_error() to retrieve 
 * 				the error code.
 *
 */
extern int 
dagema_recv_msg (int dagfd, uint32_t *message_id, uint32_t *length, uint8_t *rx_message, uint8_t *trans_id);

/**
 *
 * @brief	The same as dagema_recv_msg() except with an extra timeout
 *              parameter.
 *
 * @param[in]	dagfd		A unix style file descriptor returned 
 * 				by dag_open.
 * @param[in]	message_id 	Pointer to a value that receives
 *                              the message id.
 * @param[in/out] length	Pointer to a value that upon entry
 *                              should contain the maximum size
 *                              of the rx_message buffer, upon return
 *                              will contain the number of bytes
 *                              copied into rx_message.
 * @param[out]	rx_message	Buffer to store the message bytes
 *                              in. If the buffer isn't big enough
 *                              for all the data, it is trimed to
 *                              fit and the overflow is discarded.
 * @param[out]	trans_id	The transaction ID of the received
 *                              message, this can be NULL if the
 *                              transaction ID is not used.
 * @param[in]	timeout		Timeout in milliseconds, if timed out
 *                              -1 will be returned and dagema_get_last_error()
 *                               will return ETIMEDOUT.
 *
 * @return			0 if message was received, otherwise -1. 
 * 				Call dagema_get_last_error() to retrieve 
 * 				the error code.
 *
 */
extern int
dagema_recv_msg_timeout (int dagfd, uint32_t *message_id, uint32_t *length, uint8_t *rx_message, uint8_t *trans_id, uint32_t timeout);

/**
 *
 * @brief	Installs a callback handler for the messages from the
 *              xScale. The callback will be called whenever a complete
 *              message has been received from the dag card.
 *
 * @param[in]	dagfd		A unix style file descriptor returned 
 * 				by dag_open.
 * @param[in]	msg_handler	Callback function pointer
 *
 * @return			0 if the message hnadler was installed, 
 * 				otherwise -1. Call dagema_get_last_error() 
 * 				to retrieve the error code.
 *
 */
extern int
dagema_set_msg_handler(int dagfd, msg_handler_t msg_handler);

#if defined(DAG_CONSOLE)
extern int 
dagema_set_console_handler(int dagfd, console_putchar_t char_handler);
	
extern int
dagema_set_ctrl_msg_handler(int dagfd, ctrl_msg_handler_t ctrl_handler);

extern int 
dagema_write_console_string(int dagfd, char* str, int len);

#endif // defined(DAG_CONSOLE)
/*@}*/

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif // DAGEMA_H
/*@}*/
