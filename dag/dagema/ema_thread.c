/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: ema_thread.c,v 1.23 2006/10/19 01:22:36 jeff Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         ema_thread.c
*
* DESCRIPTION:  Contains the threads (and supporting functions) for
*               monitoring the DRB registers between the dag card and
*               the host.
*
*               A server socket is used to buffer sent/received messages
*               to/from the card. The client API interface uses a client
*               socket to send and receive messages.
*
*
* HISTORY:
*
**********************************************************************/


/* System headers */
#include "dag_platform.h"

#if defined(_WIN32)
#include <WinSock2.h>
#endif




#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
/* Option to use select instead of poll for socket events */
	#ifdef DAGEMA_USE_SELECT
	#include <sys/select.h>
	#else
	#include <sys/poll.h>
	#endif
	#include <sys/types.h>
	#include <sys/socket.h>
#endif /* !_WIN32 */




/* Endace headers. */
#include "dagapi.h"
//#include "dagutil.h"
#include "dagema.h"
#include "ema_types.h"



/* If the following is defined, transfer rates are displayed on stdout.
 * Used only internally for monitoring transfer algorithms.
 */
 
/*#define PROFILING*/






/**********************************************************************
 *
 * The last error code returned by internal functions
 *
 **********************************************************************/

extern int g_last_errno;









/**********************************************************************
 *
 * FUNCTION:     ema_create_server_sock
 *
 * DESCRIPTION:  Creates a server TCP socket and binds it to the loopback
 *               address on port 0xEACE + dagid. The socket is put in listen
 *               mode before returning.
 *
 * PARAMETERS:   maxClient    IN      Maximum simultaneous connections
 *               dagid        IN      The ID of the dag card, added to the
 *                                    base port address (0xEACE) so multiple
 *                                    server sockets can be used, each for
 *                                    different card.
 *
 * RETURNS:      socket identifier or -1 if there was an error.
 *
 **********************************************************************/
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
static int
#elif defined(_WIN32)
static SOCKET
#else
#error unsopported platform 
#endif

ema_create_server_sock(int maxClient, int dagid)
{
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	int           sock = -1;
	int           reuse = 1;	
#elif defined(_WIN32)
	SOCKET        sock;
	unsigned long argp = 1;
	BOOL          reuse = TRUE;
#endif /* _WIN32 */

	int sockFlags;
	struct sockaddr_in sockAddr;	/* IPv4 Socket address */
	struct linger sock_linger;
	

	
	
	/* Create local listen socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	if (sock < 0) 
	{
		EMA_PVERBOSE (1, "failed to open local listen socket, (errorcode %d)\n", errno);
		return -1;
	}
#elif defined(_WIN32)
	if (sock == INVALID_SOCKET)
	{
		EMA_PVERBOSE (1, "failed to open local listen socket, (errorcode %d)\n", WSAGetLastError());
		return -1;
	}
#else 
#error unsupported platform 
#endif 

	
	/* set the options so the socket closes while lingering */
	sock_linger.l_onoff  = 1;
	sock_linger.l_linger = 5;
	setsockopt (sock, SOL_SOCKET, SO_LINGER, (char*)&sock_linger, sizeof(sock_linger));

#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse, sizeof(int));
#elif defined(_WIN32)
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(BOOL));
#endif



	
	/* Set nonblocking attribute on listen socket */
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	fcntl(sock, F_GETFL, &sockFlags);
	sockFlags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, &sockFlags);
#elif defined(_WIN32)
	/*and now for the windows version....	*/
	sockFlags =  ioctlsocket(sock, FIONBIO, &argp);
	if(sockFlags != 0)
	{
		EMA_PVERBOSE (1, "listen socket could not be changed to non-blocking mode %d\n", WSAGetLastError());
	}

#endif /* _WIN32 */

	
	/* Setup localhost address and port (127.0.0.1:0xEACE) */
	sockAddr.sin_family = AF_INET; 
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY); 

	/* This uses a range of ports, one per dag board number
	 * This should be replaced by a single port and a connect
	 * command to open a new dag board so only one
	 * port address is needed.
	 * TBD when server daemon is implemented
	 */
	sockAddr.sin_port = htons(EMA_PORT + dagid); 

	if (bind(sock, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) != 0)
	{
		EMA_PVERBOSE(1, "bind failed for local listen, (%s)\n", strerror(errno));
		return -1;
	}

	if (listen(sock, maxClient) != 0)
	{
		EMA_PVERBOSE(1, "listen failed for local listen, errno=%d\n", errno);
		return -1;
	}

	/* Success */
	return sock;
}


/**********************************************************************
 *
 * FUNCTION:     ema_destroy_server_sock
 *
 * DESCRIPTION:  Destroys a server socket, previously created by a call
 *               to ema_create_server_sock.
 *
 * PARAMETERS:   sock         IN      Socket returned by ema_create_server_sock()
 *
 * RETURNS:      always returns 0 (no error)
 *
 **********************************************************************/
static int
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
ema_destroy_server_sock(int sock)
#elif defined(_WIN32)
ema_destroy_server_sock(SOCKET sock)
#endif
{
	if ( sock < 0 )
		return 0;
	
	
#if defined(__sun) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	shutdown(sock, SHUT_RDWR);
	close(sock);
#elif defined(_WIN32)
	shutdown(sock, SD_BOTH);
	closesocket(sock);
#endif
	
	
	return 0;
}



/**********************************************************************
 *
 * FUNCTION:     ema_remove_socket_from_pollset
 *
 * DESCRIPTION:  Removes a socket from the pollset and close it.
 *
 * PARAMETERS:   sock          IN      The socket to remove and close.
 *               pollSet       IN/OUT  The pollset that is updated.
 *
 * RETURNS:      void
 *
 **********************************************************************/
#if defined(_WIN32)
static void ema_remove_socket_from_pollset(SOCKET sock, ema_poll_set_t *pollSet)
#else
static void ema_remove_socket_from_pollset(int sock, ema_poll_set_t *pollSet)
#endif
{
	/* Remove the socket from the poll set */
	FD_CLR(sock, &pollSet->read);
	FD_CLR(sock, &pollSet->write);
	FD_CLR(sock, &pollSet->exception);


	/* terminating message socket */
	EMA_PVERBOSE(3, "Client connection closed (%d)\n", sock);
	
	
#if defined(__sun) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	close(sock);
#elif defined(_WIN32)
	closesocket(sock);
#endif

}





/**********************************************************************
 *
 * FUNCTION:     ema_handle_client_events
 *
 * DESCRIPTION:  Check the pollSet for sockets with data ready to be read,
 *               and read and forward one message from each.
 *
 * PARAMETERS:   pollSet       IN/OUT  Last select results
 *               activePollSet IN/OUT  Current poll set
 *               bufp          OUT     Buffer to store the message data in
 *               lenp          IN/OUT  Length of the data received, upon
 *                                     entry should contain the number of
 *                                     bytes that can be copied into the
 *                                     buffer.
 *               psockets      IN      
 *
 * RETURNS:      socket identifier or -1 if there was an error.
 *
 **********************************************************************/
static void
ema_handle_client_events(ema_poll_set_t *pollSet,
                         ema_poll_set_t *activePollSet,
                         uint8_t *bufp,
                         int *lenp,
                         ema_sockets_t *psockets)
{
	int res;
	int max_read;
#if defined(_WIN32)
	int error_code = 0;
#endif /* _WIN32 */

	res = 0;
	
	max_read = *lenp;
	*lenp = 0;


	
	if ( FD_ISSET(psockets->client_sock, &pollSet->read) ) 
	{
		EMA_PVERBOSE(1, "reading client request\n");
		res = recv(psockets->client_sock, bufp, max_read, 0);
		if (res < 0) 
		{
			
#if defined( _WIN32)
			res = 0;
			error_code =  WSAGetLastError();
			if(error_code != WSAECONNRESET)
			{
				EMA_PVERBOSE(1, "recv failed");
				EMA_PVERBOSE(1, "Windows Error Code = %d\n", error_code);
			}
#elif defined(__sun) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
			EMA_PVERBOSE(1, "recv failed");
#endif

		}
		else
		{
			*lenp = res;
			EMA_PVERBOSE(2, "read %d bytes from client\n", res);
		}
	}

	

	if (res < 0) 
	{
		EMA_PVERBOSE(1, "read failure - removing client\n");
		ema_remove_socket_from_pollset(psockets->client_sock, activePollSet);
		psockets->client_sock = 0;

	} 
	else if (FD_ISSET(psockets->client_sock, &pollSet->exception))
	{
		EMA_PVERBOSE(1, "exception for client - removing\n");
		ema_remove_socket_from_pollset(psockets->client_sock, activePollSet);
		psockets->client_sock = 0;

	}
	else if (res == 0)
	{
		EMA_PVERBOSE(1, "zero byte message from client.  Closing connection.\n");
		ema_remove_socket_from_pollset(psockets->client_sock, activePollSet);
		psockets->client_sock = 0;
	}
	
}


/**********************************************************************
 *
 * FUNCTION:     ema_check_msg_socket
 *
 * DESCRIPTION:  Checks if a new connection is pending or there is a
 *               message from a client. If neither case is satisifed
 *               this function just returns.
 *
 * PARAMETERS:   psockets     IN      Pointer to the server and client
 *                                    socket id. The client
 *               bufp         OUT     Pointer to a buffer to receive the
 *                                    messages from the socket.
 *               lenP         IN/OUT  Upon entry this should contain the
 *                                    number of bytes that can be copied
 *                                    into the buffer. Upon exit the number
 *                                    of bytes copied into the buffer.
 *
 * RETURNS:      socket identifier or -1 if there was an error.
 *
 **********************************************************************/
static void
ema_check_msg_socket (ema_sockets_t *psockets, uint8_t* bufp, int *lenp)
{
	int count;
	struct timeval tv;
	struct sockaddr_un client_addr;
	ema_poll_set_t pollSet;
	ema_poll_set_t pollSetTmp;


	/* Check for socket events */
	FD_ZERO(&pollSet.read);
	FD_ZERO(&pollSet.write);
	FD_ZERO(&pollSet.exception);
	pollSet.max_sd = 0;

	FD_SET(psockets->server_sock, &pollSet.read);
	FD_SET(psockets->server_sock, &pollSet.exception);

	if (psockets->client_sock > 0)
	{
		FD_SET(psockets->client_sock, &pollSet.read);
		FD_SET(psockets->client_sock, &pollSet.exception);
	}

	pollSet.max_sd = (psockets->server_sock > psockets->client_sock) ? (int)psockets->server_sock : (int)psockets->client_sock;

	pollSetTmp = pollSet;
	tv.tv_sec  = 0;
	tv.tv_usec = 0;

	while (((count=select(pollSet.max_sd+1, &pollSetTmp.read,
	   &pollSetTmp.write, &pollSetTmp.exception, &tv)) == -1) && (errno==EINTR)) 
	{
#if defined(_WIN32)
			if(count == SOCKET_ERROR )
			{
				EMA_PVERBOSE(1, "Winsock error occurred %d \n", WSAGetLastError());
				return;
			}
#endif /* _WIN32 */
	}

	
	
	if (count < 0) 
	{
#if defined(_WIN32)
		EMA_PVERBOSE(1, "Winsock error occurred %d \n", WSAGetLastError());
#endif
		EMA_PVERBOSE(1, "check_msg_socket: select failed\n");
		return;
	}

	/* Check for connections */
	if (FD_ISSET(psockets->server_sock, &pollSetTmp.read))
	{
		uint32_t addrLen = sizeof(client_addr);
		EMA_PVERBOSE(3, "check_msg_socket: server socket activity\n");

		/* Set up a new client connection */
		psockets->client_sock = accept(psockets->server_sock, (struct sockaddr*) &client_addr, &addrLen);

		/* Add client to read poll set */
		if ((int)psockets->client_sock > 0)
		{
			EMA_PVERBOSE(3, "check_msg_socket: Client connection accepted (%d)\n", psockets->client_sock);

			FD_SET(psockets->client_sock, &pollSet.read);

			if ((int)psockets->client_sock > pollSet.max_sd) 
				pollSet.max_sd = (int)psockets->client_sock;
		}
		else
		{
			EMA_PVERBOSE(1, "check_msg_socket: Failed to add client connection\n");
		}

		count--;
	}

	/* Check for client activity */
	if (count > 0)
	{
		ema_handle_client_events(&pollSetTmp, &pollSet, bufp, lenp, psockets);
	}
	else
	{
		*lenp = 0;
	}
	
	
}






/**********************************************************************
 *
 * FUNCTION:     ema_process_msg_buffer
 *
 * DESCRIPTION:  Takes the message receive buffer and a value indicating
 *               how much of the receive buffer is full. It then attempts
 *               to find a valid packet in the buffer by reading the first
 *               eight bytes (message header), then determining if we have
 *               'length' bytes available. Because there is no signature
 *               value at the start or a checksum we have to rely on the
 *               communication being reliable and the embedded processor
 *               responding correctly.
 *               If a message is found, the size of the message is returned.
 *
 * PARAMETERS:   pdata        IN      Data buffer used to stored the bytes
 *                                    received from the card.
 *               data_size    IN      The number of bytes in the buffer
 *
 * RETURNS:      the size of the message found in bytes (including the
 *               header) or a negative error code:
 *                   -1 if no message was found
 *                   -2 if a message was found but its length was invalid
 *
 **********************************************************************/
static int
ema_process_msg_buffer(uint8_t *pdata, uint32_t data_size)
{
	uint32_t         cmplt_size;
	uint16_t         msg_length;
	uint32_t         msg_id;
	ema_msg_t*       msg_p;

	
	/* check if there is enough data in the buffer to constitute a header */
	if ( data_size >= EMA_MSG_HEADER_LENGTH )
	{
		/* check the packet header */
		msg_p = (ema_msg_t*) pdata;
		msg_id     = dagema_le32toh(msg_p->data.message_id);
		msg_length = dagema_le16toh(msg_p->data.length);

		/* check if the size is valid */
		if (msg_length > EMA_MSG_PAYLOAD_LENGTH)
			return -2;
				
		/* calculate the complete message size (header + payload) */
		cmplt_size = msg_length + EMA_MSG_HEADER_LENGTH;
				
		if ( data_size >= cmplt_size )
		{
			EMA_PVERBOSE (3, "[received %d byte message, id:%08X, version:%02X trans:%02X]\n", 
			        msg_length, msg_id, msg_p->data.version, msg_p->data.trans_id);

			return cmplt_size;
		}
	}
	
	return -1;
}






/**********************************************************************
 *
 * FUNCTION:     ema_get_byte
 *
 * DESCRIPTION:  Reads a byte from the lower level communcation functions.
 *               This function doesn't block.
 *
 * PARAMETERS:   ctx          IN      Pointer to the library communication
 *                                    context.
 *               rx_byte      OUT     Pointer to byte that receives the
 *                                    byte read.
 *
 * RETURNS:      Returns 0 if a byte was read and channel & rx_byte were
 *               updated. If no byte to be received then -1 is returned
 *               and rx_byte remains unchanged.
 *
 **********************************************************************/
static int
ema_get_byte (ema_context_t *ctx, uint8_t *rx_byte)
{
	return ctx->drv_cbf.pfunc_drv_get_byte(ctx->drv_handle,rx_byte);
}



/**********************************************************************
 *
 * FUNCTION:     ema_put_bytes
 *
 * DESCRIPTION:  Thread function that waits for a message from a client
 *               API, then process the request. The lower level hardware
 *               communication is also polled to see if there is data
 *               from the card.
 *
 * PARAMETERS:   ctx          IN      Pointer to the library communication
 *                                    context.
 *               tx_bytes     IN      Pointer to buffer containing the
 *                                    bytes to be sent.
 *               length       IN      The number of bytes in the buffer
 *                                    to be sent.
 *
 * RETURNS:      returns -1 if the put was aborted (by the user clearing
 *               the run_thread and setting the abort_all_trans fields in
 *               the contest), otherwise 0 is returned.
 *
 * NOTES:        This function blocks until either all the byte are sent
 *               or the user signals the thread to terminate with all
 *               transactions aborted.
 *
 **********************************************************************/
static int
ema_put_bytes (ema_context_t *ctx, const uint8_t *tx_bytes, int length)
{ 
	int   i;
	int   written;
	
	if(ctx->drv_handle)
	{
		for (i=0; i<length; )
		{
			written = ctx->drv_cbf.pfunc_drv_put_string(ctx->drv_handle, &(tx_bytes[i]), (length - i), 100);
			if ( written > 0 )
				i += written;
			
			if ( ctx->run_thread == 0 && ctx->abort_all_trans == 1 )
				return -1;
		}
	}
	
	return 0;
}



/**********************************************************************
 *
 * FUNCTION:     ema_messaging_thread
 *
 * DESCRIPTION:  Thread function that waits for a message from a client
 *               API, then process the request. The lower level hardware
 *               communication is also polled to see if there is data
 *               from the card.
 *
 * PARAMETERS:   lpParam      IN/OUT  Pointer to the library communication
 *                                    context.
 *
 * RETURNS:      always returns 0
 *
 **********************************************************************/
#if defined(_WIN32)
DWORD WINAPI ema_messaging_thread(LPVOID lpParam)
#elif defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
int ema_messaging_thread(ema_context_t *ctx)
#endif
{
	uint8_t        rx_byte;
	int            res = 0;
	int            idle= 0;
	int            msg_size;
	int            timeout;
	ema_msg_t*     msgP;
	ema_msg_t*     sock_msgP;
	ema_sockets_t  sockets;
	const uint32_t buf_size = EMA_MSG_PAYLOAD_LENGTH + EMA_MSG_HEADER_LENGTH;

	/* buffer used to store data from the card (channel 0) */
	uint8_t   card_read_buf[EMA_MSG_PAYLOAD_LENGTH + EMA_MSG_HEADER_LENGTH];
	int       card_read_index;

	/* buffer used to store data from the socket (channel 0) */
	uint8_t   sock_read_buf[EMA_MSG_PAYLOAD_LENGTH + EMA_MSG_HEADER_LENGTH];
	int       sock_read_index;
	int       sock_read_count;
	
	

	
#if defined(_WIN32)
	ema_context_t *ctx = (ema_context_t *) lpParam;
#endif

	/* sanity */
	if ( ctx == NULL )
		return 0;



	
	/* indicate the thread is running */
	ctx->thread_running = 1;
	
	
	/* initialise the sockets  */
	sockets.server_sock = ctx->server_sock;
	sockets.client_sock = 0;
	
	
	/* initialise the read counts  */
	sock_read_index = 0;
	card_read_index = 0;
	
	
	
	/* Start the thread loop */
	while (ctx->run_thread)
	{
		idle++;


		/* Check if there are any bytes to be read from the card */
		if ( (res = ema_get_byte(ctx, &rx_byte)) == 0 )
		{
			/* put the byte read in the buffer then process the buffer */
			card_read_buf[card_read_index++] = rx_byte;
			res = ema_process_msg_buffer(card_read_buf, card_read_index);
			if (res > 0)
			{
				msg_size = res;
				
				msgP = (ema_msg_t*) (&card_read_buf[0]);
				EMA_PVERBOSE (3, "Received %d byte message from card (id%08X)\n", msg_size, msgP->data.message_id);

				if ( ctx->message_handler != NULL )
				{
					EMA_PVERBOSE (3, "Sending %d byte message to the callback message handler\n", msg_size);
					ctx->message_handler (msgP->data.message_id, &(msgP->data.payload[0]), msgP->data.length, msgP->data.trans_id);
				}
				else
				{
					EMA_PVERBOSE (3, "Sending %d byte message to the client socket\n", msg_size);
					res = send (sockets.client_sock, (void*)card_read_buf, msg_size, 0);
				}
				
				card_read_index -= msg_size;
				if ( card_read_index > 0 )
				{
					EMA_PVERBOSE (3, "card_read_index:%d msg_size:%d\n", card_read_index, msg_size);
					memmove (&(card_read_buf[0]), &(card_read_buf[msg_size]), card_read_index);
				}
			}
			
			/* if the return value is -2 then there is a message in the buffer that
			 * exceeds the maximum length, so we clear the buffer and reset it.	 */
			else if( res == -2 )
			{
			
				card_read_index = 0;
				//clear the phisycal buffer just in case 
				while ( ema_get_byte(ctx, &rx_byte) == 0 ) 
				   ;
				res = -2;
			}

			idle = 0;
			

			/* go back up and get the next byte from the xScale */
			continue;
		}



		/* transmit to xScale not in progress so check for messages from library clients */
		sock_read_count = (buf_size - sock_read_index);
		ema_check_msg_socket (&sockets, &(sock_read_buf[sock_read_index]), &sock_read_count);
		sock_read_index += sock_read_count;
		
		
		/* process the buffer and check for a valid packet */
		res = ema_process_msg_buffer(sock_read_buf, sock_read_index);
		if (res > 0)
		{
			msg_size = res;
			
			sock_msgP = (ema_msg_t*) (&sock_read_buf[0]);
			EMA_PVERBOSE(1, "Sending %d byte message to the board (id:%08X)\n", msg_size, sock_msgP->data.message_id);


			/* check to see if this is an internal message (one that is not sent to the card) */
			if ( (sock_msgP->data.message_id & EMA_CLASS_MASK) == EMA_CLASS_INTERNAL )
			{
#if defined(DAG_CONSOLE)
				if ( ctx->ctrl_msg_handler )
					ctx->ctrl_msg_handler (sock_msgP->data.message_id, &(sock_msgP->data.payload[0]), sock_msgP->data.length, sock_msgP->data.trans_id);
#endif  // defined(DAG_CONSOLE)
			}
			else
			{
				if ( ema_put_bytes(ctx, sock_read_buf, msg_size) == -1 )
					goto Exit;
			}


			/* remove the message from the socket buffer */
			sock_read_index -= msg_size;
			if ( card_read_index > 0 )
				memmove (&(sock_read_buf[0]), &(sock_read_buf[msg_size]), sock_read_index);
			
			
			idle = 0;
		}

		/* if the return value is -2 then there is a message in the buffer that
		 * exceeds the maximum length, so we clear the buffer and reset it.	 */
		else if ( res == -2 )
		{
			card_read_index = 0;
		}
		
			
		/* Sleep while idle */
		if (idle > 10) 
		{
			ema_usleep(1000);
			idle = 0;
		}
		
	}
	
	

	
	/* if we are not aborting all the transactions then we should check whether
	 * a read from the xScale is half way through, in which case we should wait
	 * till we have finished reading the entire message. Otherwise the next open
	 * library call will get only half a message which will be problem.
	 */
	if (ctx->abort_all_trans == 0)
	{
		timeout = 6000;
		while ((card_read_index > 0) && (--timeout > 0))
		{
			/* Check if there are any bytes to be read from the card */
			if ( (res = ema_get_byte(ctx, &rx_byte)) == 0 )
			{
				/* add the byte to the buffer then process the buffer */
				card_read_buf[card_read_index++] = rx_byte;
				res = ema_process_msg_buffer(card_read_buf, card_read_index);
				
				if (res > 0)
				{
					msg_size = res;
					
					/* Found a message so just dump it */
					card_read_index -= msg_size;
					if ( card_read_index > 0 )
						memmove (&(card_read_buf[0]), &(card_read_buf[msg_size]), card_read_index);
				}
				
				else if (res == -2)
				{
					/* Found an invalid message so ... just give up */
					break;
				}
			}

			/* delay a bit till the xscale can deliver the next byte */
			ema_usleep(100);
		}

		if (timeout == 0)
			EMA_PVERBOSE(0, "Timedout while waiting for last message.\n");
			
	}
		
	
	
	

Exit:	
	/* indicate the thread has stopped */
	ctx->thread_running = 0;

	return 0;
}




/**********************************************************************
 *
 * FUNCTION:     ema_start_handshaking
 *
 * DESCRIPTION:  Performs the handshaking used to setup the communication
 *               between the host and embedded processor. The handshaking
 *               invloves sending a request for the software version and
 *               reading back the reply. The reply is expected to contain
 *               the message version used by the embeeded software (in the
 *               message header.
 *
 * PARAMETERS:   ctx          IN      Pointer to the open context to perform
 *                                    the handshaking on.
 *
 * RETURNS:      0 if sucessiful otherwise a -1
 *
 **********************************************************************/
static int
ema_start_handshaking (ema_context_t *ctx)
{
#if !defined(DAG_CONSOLE)
	
	ema_msg_t  msg;
	uint16_t   msg_len;
	int        bytes_recv;
	uint8_t    rx_byte;
	int        i;
	uint64_t   cur_time;
	uint64_t   start_time;
	int        idle;
	
	ema_msg_read_version_t*       requestP;
	ema_msg_read_version_cmplt_t* respP;
	
#endif	// !defined(DAG_CONSOLE)
	
	
	/* sanity */
	if ( ctx == NULL || ctx->drv_handle == NULL )
		return -1;

#if !defined(DAG_CONSOLE)
	
	/* request the version of the embedded software with a version 0 message */
	msg_len = (uint16_t)sizeof(ema_msg_read_version_t);
	
	msg.data.message_id = dagema_htole32(EMA_MSG_READ_VERSION);
	msg.data.length     = dagema_htole16(msg_len);
	msg.data.version    = 0;
	msg.data.trans_id   = 0;             /* unused, set to 0 */
	
	requestP = (ema_msg_read_version_t*) (&msg.data.payload[0]);
	requestP->request = 0;               /* unused, set to 0 */
	

	/* send the request */
	for ( i=0; i<(msg_len + EMA_MSG_HEADER_LENGTH); i++ )
	{
		if ( ctx->drv_cbf.pfunc_drv_put_byte(ctx->drv_handle, msg.datum[i], 1000) != 0 )
				return -1;
	}
	
	
	/* poll waiting for the response */
	start_time = ema_get_tick_count();
	bytes_recv = 0;
	idle = 0;
	do
	{
		idle++;

		/* check if we have timed-out (~2 seconds) */
		cur_time = ema_get_tick_count();
		if ( (cur_time - start_time) >= 2000 )
		{
			EMA_PVERBOSE (3, "Timed-out waiting for a reply while handshaking\n");
			return -1;
		}


		/* check if a byte can be read */
		if ( ema_get_byte(ctx, &rx_byte) == 0 )
		{
			msg.datum[bytes_recv++] = rx_byte;
			idle = 0;
		}
		
		/* if idle for a while add a delay */
		if (idle >= 10)
		{
			ema_usleep (1000);
			idle = 0;
		}
			
	} while ( bytes_recv < (sizeof(ema_msg_read_version_cmplt_t) + EMA_MSG_HEADER_LENGTH) );
	
	EMA_PVERBOSE (3, "message_id: 0x%08x, length: 0x%08x, version: 0x%08x, trans_id: 0x%08x\n",
		msg.data.message_id, msg.data.length, msg.data.version, msg.data.trans_id);
	
	/* check the response is correct */
	if ( dagema_le32toh(msg.data.message_id) != EMA_MSG_READ_VERSION_CMPLT ) {
		EMA_PVERBOSE (1, "Wrong message_id (0x%08x) when checking protocol version\n", msg.data.message_id);
		return -1;
	}
	
	if ( dagema_le16toh(msg.data.length) != sizeof(ema_msg_read_version_cmplt_t) ) {
		EMA_PVERBOSE (1, "Wrong data length\n");
		return -1;
	}

	
	
	/* get the version of the received message */
	ctx->msg_version = msg.data.version;
	
	
	/* store the rest of the message in the context (it may be useful
	 * somewhere down the track).
	 */
	respP = (ema_msg_read_version_cmplt_t*) &(msg.data.payload[0]);
	ctx->client_type    = dagema_le32toh(respP->type);
	ctx->client_version = dagema_le32toh(respP->version);
	

#else   // !defined(DAG_CONSOLE)	
	
	ctx->msg_version = 0x00;
	
#endif  // defined(DAG_CONSOLE)	 

	
	return 0;
}



int ema_drv_cbf_reg(ema_context_t* ctx, ema_drv_cbf_t* cbf)
{
	if(cbf->pfunc_drv_init==NULL||
		cbf->pfunc_drv_get_byte==NULL||
		cbf->pfunc_drv_put_byte==NULL||
		cbf->pfunc_drv_put_string==NULL||
		cbf->pfunc_drv_flush_recv==NULL||
		cbf->pfunc_drv_term==NULL)
		return -1;
	memcpy(&(ctx->drv_cbf),cbf,sizeof(ema_drv_cbf_t));
	return 0;
}



/**********************************************************************
 *
 * FUNCTION:     ema_initialise_messaging
 *
 * DESCRIPTION:  Creates the server socket used to wait on messages from
 *               the client APIs. After the server socket is created a
 *               new thread is started that manages the server socket.
 *
 * PARAMETERS:   ctx          IN      Pointer to the context used for
 *                                    the card.
 *
 * RETURNS:      0 if success otherwise a negative error code:
 *                  -2 indicates the socket was already up and running
 *                     (i.e. dagconsole is already running)
 *                  -1 any other error occuired, the errorcode is put
 *                     in g_last_errno.
 *
 **********************************************************************/
int
ema_initialise_messaging(ema_context_t *ctx)
{
#if defined(__sun) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	int     socket_return = -1;
	static pthread_t msg_thread;
#elif defined(_WIN32)
	SOCKET 	socket_return = (SOCKET)-1;
#endif 

	if ( ctx->thread_running == 1 )
	{
		return -2;
	}
	else
	{
		/* There is still a possibility that the server socket has already been 
		 * created by a previous instance of the library on non-Windows platforms.
		 * Find out by trial and error.
		 */
		socket_return = ema_create_server_sock(1, ctx->dag_id);
		if ((int)socket_return < 0)
		{
			ctx->server_sock = -1;

			/* socket thread could not be created. */
			return -2;
		}
			
		/* there was no server socket, it has just been made, continue on with the thread. */
		ctx->server_sock = socket_return;
		
		EMA_PVERBOSE (1, "Established server socket %d\n", ctx->dagfd);
	}
	
	/* Setup access to the DRB */
	if ( ctx->device_code == PCI_DEVICE_ID_DAG7_10 )
	{
		drb_fifo_ema_drv_reg(ctx);
	}
	else /*37t*/
	{
		drb_37t_ema_drv_reg(ctx);
	}

	ctx->drv_handle = ctx->drv_cbf.pfunc_drv_init(ctx->dagfd);
	if ( ctx->drv_handle == NULL )
	{
		ema_destroy_server_sock (ctx->server_sock);
		ctx->server_sock = -1;

		EMA_PVERBOSE (0, "initialise_messaging: Error opening connection via DRB FIFO.\n");
		g_last_errno = ENOMEM;
		return -1;
	}

	
	/* start the handshaking used to determine the version of messaging to use */
	if ( ema_start_handshaking(ctx) != 0 )
	{
		ema_destroy_server_sock (ctx->server_sock);
		ctx->server_sock = -1;

		ctx->drv_cbf.pfunc_drv_term(ctx->drv_handle);
		ctx->drv_handle = NULL;

		EMA_PVERBOSE (0, "initialise_messaging: Handshaking failed.\n");
		g_last_errno = ERESP;
		return -1;
	}
	
	
	/* indicate the thread should be running */
	ctx->run_thread = 1;
	
	
	/*create the thread that does the messaging control (thread implementation is in ema_thread.c) */
#if defined (_WIN32)
	if (CreateThread(NULL, 0, ema_messaging_thread, (LPVOID)ctx, 0, NULL) == NULL)
#elif defined(__sun) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	if (pthread_create(&msg_thread, NULL, (void *) &ema_messaging_thread, (void *)ctx) != 0)
#endif
	{
		ema_destroy_server_sock (ctx->server_sock);
		ctx->server_sock = -1;

		ctx->drv_cbf.pfunc_drv_term(ctx->drv_handle);
		ctx->drv_handle = NULL;

		EMA_PVERBOSE(1, "Error: Failed to create socket monitor thread.\n");
		g_last_errno = ENOMEM;
		return -1;
	}
	

	return 0;
}





/**********************************************************************
 *
 * FUNCTION:     ema_terminate_messaging
 *
 * DESCRIPTION:  Terminates the messaging thread and server socket for
 *               the card specified by its context. This function has
 *               a timeout of 3 seconds, if the thread doesn't terminate
 *               by then the function simply returns.
 *
 * PARAMETERS:   ctx          IN      Pointer to the context used for
 *                                    the card.
 *               flags        IN      Possible shutdown flags, can be
 *                                    OR'ed together.
 *                                    EMA_SHUTDOWN_HARD = abort any
 *                                    transactions that may be pending,
 *                                    this is not recommended and will
 *                                    likely require a card reset when
 *                                    opening a new EMA connection.
 *
 * RETURNS:      0 if success otherwise a negative error code.
 *
 **********************************************************************/
int
ema_terminate_messaging(ema_context_t *ctx, uint32_t flags)
{
	int timeout;
	
	/* Process the flags */
	if ( flags & EMA_CLOSE_NO_FLUSH )
		ctx->abort_all_trans = 1;
	
	
	/* Tell the thread to stop and wait while it terminates (timeout after ~3 seconds) */
	timeout = 1000;
	ctx->run_thread = 0;
	while (ctx->thread_running && --timeout)
		ema_usleep (1000);

	
	/* Debugging */	
	if (timeout == 0)
		EMA_PVERBOSE(0, "Timedout while waiting for thread to finish.\n");

	
	/* Kill the server socket */
	ema_destroy_server_sock (ctx->server_sock);
	ctx->server_sock = -1;


	/* close the drb connection */
	if ( ctx->drv_handle )
	{
		ctx->drv_cbf.pfunc_drv_term(ctx->drv_handle);
		ctx->drv_handle = NULL;
	}

		
	return 0;
}


/* moved here from drb_comm.c*/
/**********************************************************************
 *
 * FUNCTION:     ema_get_tick_count
 *
 * DESCRIPTION:  Returns the number of milliseconds that have ellapsed.
 *               On a windows machine this is the number of milliseconds
 *               since the system started. On Unix based OS'es this is the
 *               time in milliseconds since the Epoch.
 *
 * PARAMETERS:   none
 *
 * RETURNS:      returns the number of ellapsed milliseconds
 *
 **********************************************************************/
uint64_t
ema_get_tick_count(void)
{
#if defined(_WIN32)
	
	return GetTickCount();
	
#elif defined(__sun) || defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))

	struct timeval tv;
	gettimeofday (&tv, NULL);
	return ((uint64_t)tv.tv_sec * 1000) + (tv.tv_usec / 1000);

#endif	
}





/**********************************************************************
 *
 * FUNCTION:     ema_usleep
 *
 * DESCRIPTION:  Sleeps for a certain amount of microseconds 
 *
 * PARAMETERS:   usec         IN      The number of microseconds to sleep
 *                                    for. Only Windows the best resolution
 *                                    is 1000us, a value less than that
 *                                    will default to 1000us.
 *
 * RETURNS:      nothing
 *
 **********************************************************************/
void
ema_usleep(int usec)
{
#if defined(_WIN32)

	/* WIN32 has a millisecond Sleep(), convert to msecs */
	usec = usec/1000;
	if(usec<1)
		usec = 1;
	Sleep(usec);

#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	int res;
	struct timespec sleep_time = { .tv_sec = 0, .tv_nsec = usec*1000};

	res = nanosleep(&sleep_time, NULL);
	if (res == -1) 
		EMA_PVERBOSE(2, "nanosleep() failed %s", strerror(errno));

#endif /* Platform-specific code. */
}


