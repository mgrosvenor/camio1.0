/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagema.c,v 1.23 2009/05/07 01:50:34 sfd Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         dagema.c
* DESCRIPTION:  DAG Embedded Messaging API (EMA)
*               Implementation of the dag EMA interface.
*
* HISTORY:
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"


/* Endace headers. */
#include "dagapi.h"
#include "dagpci.h"
#include "dagutil.h"
#include "dag_config.h"
#include "dag_component.h"
#include "dagema.h"
#include "ema_types.h"




/* Make sure that the expected values are defined for the OS'es supported */
#if !(defined(_WIN32) || defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__)))
	#error "Operating System not supported."
#endif






/**********************************************************************
 *
 * The last error code returned by internal functions
 *
 **********************************************************************/

int g_last_errno = 0;



/**********************************************************************
 *
 * Linked list of open EMA sessions (attached to dag cards)
 *
 **********************************************************************/

static ema_context_t * g_ema_contexts = NULL;






/**********************************************************************
 *
 * External functions
 *
 **********************************************************************/


   
   
   
   
/**********************************************************************
 *
 * FUNCTION:     dagema_le16toh
 *               dagema_le32toh
 *               dagema_htole16
 *               dagema_htole32
 *_
 * DESCRIPTION:  Conversion functions, to from little endian to host byte
 *               order and vise-versa.
 *
 * PARAMETERS:   either a little endian or host byte ordered value.
 *
 * RETURNS:      either a little endian or host byte ordered converted value.
 *
 **********************************************************************/
uint16_t
dagema_le16toh(uint16_t little16)
{
	union { long l; char c[sizeof (long)]; } u;
	u.l = 1;
	
	if ( u.c[sizeof (long) - 1] == 1 )
	{
		/* big endian machine */
		return ( ((little16 & 0xFF00) >> 8) | ((little16 & 0x00FF) << 8) );
	}
	else
	{
		/* little endian machine */
		return little16;
	}
}

uint32_t
dagema_le32toh(uint32_t little32)
{
	union { long l; char c[sizeof (long)]; } u;
	u.l = 1;
	
	if ( u.c[sizeof (long) - 1] == 1 )
	{
		/* big endian machine */
		return ( ((little32 & 0xFF000000) >> 24) | ((little32 & 0x00FF0000) >> 8)  |
		         ((little32 & 0x0000FF00) << 8)  | ((little32 & 0x000000FF) << 24) );
	}
	else
	{
		/* little endian machine */
		return little32;
	}
}


uint16_t
dagema_htole16(uint16_t host16)
{
	union { long l; char c[sizeof (long)]; } u;
	u.l = 1;
	
	if ( u.c[sizeof (long) - 1] == 1 )
	{
		/* big endian machine */
		return ( ((host16 & 0xFF00) >> 8) | ((host16 & 0x00FF) << 8) );
	}
	else
	{
		/* little endian machine */
		return host16;
	}
}


uint32_t
dagema_htole32(uint32_t host32)
{
	union { long l; char c[sizeof (long)]; } u;
	u.l = 1;
	
	if ( u.c[sizeof (long) - 1] == 1 )
	{
		/* big endian machine */
		return ( ((host32 & 0xFF000000) >> 24) | ((host32 & 0x00FF0000) >> 8)  |
		         ((host32 & 0x0000FF00) << 8)  | ((host32 & 0x000000FF) << 24) );
	}
	else
	{
		/* little endian machine */
		return host32;
	}
}




/**********************************************************************
 *
 * FUNCTION:     insert_context
 *
 * DESCRIPTION:  Adds a context to the internal global linked list.
 *
 * PARAMETERS:   ctx          IN      Pointer to the context to add
 *
 * RETURNS:      0 if the context was added otherwise -1
 *
 **********************************************************************/
static int
insert_context(ema_context_t* ctx)
{
	ema_context_t* tmp;

	if ( g_ema_contexts == NULL )
	{
		g_ema_contexts = ctx;
		return 0;
	}
	else
	{
		for ( tmp=g_ema_contexts; tmp!=NULL; tmp=tmp->next_ctx )
			if ( tmp->next_ctx == NULL )
			{
				tmp->next_ctx = ctx;
				ctx->next_ctx = NULL;
				return 0;
			}
	}
	
	return -1;
}




/**********************************************************************
 *
 * FUNCTION:     remove_context
 *
 * DESCRIPTION:  Removes a context from the global linked list and frees
 *               the memory allocated to for the context.
 *
 * PARAMETERS:   ctx          IN      Pointer to the context to be removed
 *                                    upon return the context is no longer
 *                                    valid and should not be used.
 *
 * RETURNS:      0 if the conext was removed, otherwise -1
 *
 **********************************************************************/
static int
remove_context(ema_context_t* ctx)
{
	ema_context_t* prev_ctx;
	ema_context_t* tmp;

	prev_ctx = NULL;
	for ( tmp=g_ema_contexts; tmp!=NULL; tmp=tmp->next_ctx )
	{
		if ( tmp == ctx )
		{
			if ( prev_ctx != NULL )
			{
				prev_ctx->next_ctx = tmp->next_ctx;
			}
			else
			{
				g_ema_contexts = tmp->next_ctx;
			}
			
			free (tmp);
			return 0;
		}
		
		prev_ctx = tmp;
	}
	
	return -1;
}





/**********************************************************************
 *
 * FUNCTION:     ema_connect_to_server
 *
 * DESCRIPTION:  Connects to the server socket created by the messaging
 *               thread.
 *
 * PARAMETERS:   sockname     IN      Name of the socket
 *               dagid        IN      The id of the dag card (also related
 *                                    to the socket identifier).
 *
 * RETURNS:      socket identifier or -1 if there was an error.
 *
 **********************************************************************/
#if defined(_WIN32)
SOCKET
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
int
#endif
ema_connect_to_server(const char *sockname, int dagid)
{
#if defined(_WIN32)
	SOCKET sock = INVALID_SOCKET;
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	int    sock = -1;
#endif
	struct sockaddr_in sockAddr;	/* IPv4 Socket address */

	
	/* Create local listen socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) 
	{
#if defined(_WIN32)
		g_last_errno = WSAGetLastError();
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		g_last_errno = errno;
#endif

		EMA_PVERBOSE (1, "failed to open local listen socket, errno=%d\n", g_last_errno);
		return -1;
	}

	
	
	/* Setup localhost address and port (127.0.0.1:0xEACE) */
	sockAddr.sin_family = AF_INET; 

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	sockAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
#elif defined (_WIN32)
	/* Windows will not use just "any" old address */
	sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
#endif /* _WIN32 */

	
	/* This uses a range of ports, one per dag board number
	 * This should be replaced by a single port and a connect
	 * command to open a new dag board so only one
	 * port address is needed.
	 * TBD when server daemon is implemented
	 */
	sockAddr.sin_port = htons(EMA_PORT + dagid); 

	if (connect(sock, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) != 0)
	{
#if defined(_WIN32)
		g_last_errno = WSAGetLastError();
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		g_last_errno = errno;
#endif

		EMA_PVERBOSE (1, "Failed to connect to socket (error=%d)\n", g_last_errno);
		return -1;
	}

	/* Success */
	return sock;
}





/**********************************************************************
 *
 * FUNCTION:     dagema_get_last_error
 *
 * DESCRIPTION:  Returns the last error code generated by a function
 *               call, use this whenever one of the functions return -1.
 *
 * PARAMETERS:   none
 *
 * RETURNS:      The last error code generated.
 *
 **********************************************************************/
int
dagema_get_last_error (void)
{
	return g_last_errno;
}




/**********************************************************************
 *
 * FUNCTION:     dagema_open_conn
 *
 * DESCRIPTION:  Opens a connection to the dag card, this creates a server
 *               socket and an associated thread to receive and
 *               transmit messages.
 *               An open connection must be closed with a call to 
 *               dagema_close_conn(). Only one connection at a time can
 *               be opened to a dagcard.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open. This file
 *                                    descriptor must stay valid until
 *                                    the connection is closed, failure
 *                                    to do so will result in undefined
 *                                    behaviour.
 *
 * RETURNS:      0 if the connection was opened, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int
dagema_open_conn (int dagfd)
{
	daginf_t*      daginf;
	char           sockname[128];
	ema_context_t* ctx;
	ema_context_t* tmp;


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun))

	struct sched_param sparm;


	/* Set up real-time scheduling to enable high-precision sleeps */
	sparm.sched_priority = ((sched_get_priority_max(SCHED_FIFO) - sched_get_priority_min(SCHED_FIFO))) / 2 + sched_get_priority_min(SCHED_FIFO);
	sched_setscheduler(getpid(), SCHED_RR, &sparm);

#endif	/* Platform-specific code. */

	
	g_last_errno = ENONE;
	
	
	
	/* Get the card id number and type */
	daginf = dag_info(dagfd);
	if ( daginf == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}


    /* Check if this card has already been opened with a call to dagema_open_conn */
    if(g_ema_contexts != NULL)
    {
        for ( tmp=g_ema_contexts; tmp!=NULL; tmp=tmp->next_ctx )
            if ( tmp->dag_id == daginf->id )
            {
                g_last_errno = EEXIST;
                return -1;
            }
    }
	/* Check the card is either a 7.1s or a 3.7t or 3.7t4 */
/* 	switch (daginf->device_code) */
/* 	{ */
/* 		case PCI_DEVICE_ID_DAG3_70T:        break; */
/* 		case PCI_DEVICE_ID_DAG7_10:         break; */
/* 		case PCI_DEVICE_ID_DAG3_7T4:	    break; */
/* 		default:          */
/* 			g_last_errno = ECARDNOTSUPPORTED; */
/* 			return -1; */
/* 	} */
	
	
	/* Create a EMA context for the dag card  */
	ctx = malloc (sizeof(ema_context_t));
	if ( ctx == NULL )
	{
		g_last_errno = ENOMEM;
		return -1;
	}
	
	memset (ctx, 0, sizeof(ema_context_t));
	
	ctx->dagfd       = dagfd;
	ctx->dag_id      = daginf->id;
	ctx->device_code = daginf->device_code;

	/* set the initial transaction id */
	ctx->current_trans_id = 0x01;
	
	
	/* Slot this new context onto the end of the list */
	if ( insert_context(ctx) != 0 )
	{	
		EMA_PVERBOSE (1, "Failed to insert context\n");
		g_last_errno = ENOMEM;
		free (ctx);
		return -1;
	}

	

	/* Check if another instance of this library has an open connection to this card */
	if ( lock_ema(ctx) != 0 )
	{	
		EMA_PVERBOSE (1, "EMA locked out for dag%d\n", daginf->id);
		g_last_errno = ELOCKED;
		remove_context (ctx);
		return -1;
	}
	
		

	/* Initialise and start the messaging thread */
	if ( ema_initialise_messaging(ctx) == -1 )
	{
		unlock_ema (ctx);
		remove_context (ctx);

		EMA_PVERBOSE (1, "Failed to start messaging thread.\n");
		return -1;
	}
	



	/* Setup a local socket for to connect to the server socket just setup in the messaging thread */
	snprintf (sockname, 128, "/tmp/dag%d", ctx->dag_id);
	ctx->client_sock = ema_connect_to_server(sockname, ctx->dag_id);
	if (ctx->client_sock == INVALID_SOCKET)
	{
		ema_terminate_messaging (ctx, 0);
		unlock_ema (ctx);
		remove_context (ctx);
		
		EMA_PVERBOSE (1, "Failed to connect to EMA socket.\n");
		return -1;
	}
	


	return 0;
}


/**********************************************************************
 *
 * FUNCTION:     dagema_close_conn
 *
 * DESCRIPTION:  Closes the connection to the dag card, shutdowns the
 *               the server socket and kills the monitor thread.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open. This must be
 *                                    the same file descriptor past to
 *                                    dagema_open_conn().
 *               flags        IN      Optional close flags, multiple flags
 *                                    can be OR'ed together. See the notes
 *                                    for possible values.
 *                                    
 *
 * NOTES:        Possible flags (mulitple can be OR'ed together)
 *               EMA_SHUTDOWN_HARD  Closes the link immediately without
 *                                  waiting for queued messages to be
 *                                  transmitted to the xScale. If this
 *                                  flag is used it is recomended that
 *                                  the xScale be reset before opening
 *                                  another connection.
 *
 * RETURNS:      0 if the connection was closed, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int
dagema_close_conn (int dagfd, uint32_t flags)
{
	ema_context_t* ctx;

	g_last_errno = ENONE;
	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
		{
			break;
		}
	
	
	/* check we have a context for the card */
	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}
	
	
	/* close the client socket */
#if defined(_WIN32)
	closesocket(ctx->client_sock);
#elif defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	close(ctx->client_sock);
#endif
	
	
	/* terminating the message (kills the thread and terminates the server socket) */
	EMA_PVERBOSE (1, "terminate_messaging\n");
	ema_terminate_messaging (ctx, flags);

	
	
	/* release the lock for this card */
	unlock_ema (ctx);
	
	
	/* remove the context from the linked list and delete it */
	remove_context (ctx);


	return 0;
}






/**********************************************************************
 *
 * FUNCTION:     dagema_reset_processor
 *
 * DESCRIPTION:  Resets the embedded processor, the reset process may take
 *               upto 2 minutes depending on whether or not memory tests
 *               are performed or not. This function blocks while the reset
 *               is in progress. This function will fail if a EMA connection
 *               is already open.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               flags        IN      Optional flags used to indicate
 *                                    how the boot process should run.
 *                                    Possible values are:
 *                                     - EMA_RUN_SRAM_MEMORY_TEST
 *                                     - EMA_RUN_DRAM_MEMORY_TEST
 *
 * RETURNS:      0 if the reset was successiful, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int
dagema_reset_processor (int dagfd, uint32_t flags)
{
	char            dagname[DAGNAME_BUFSIZE];
	int             dagstream;
	int 		result;
	char            str_dev[48];
	dag_card_ref_t  card_ref; 
	dag_component_t root_component;
	dag_component_t erfmux;
	attr_uuid_t     line_steering_attr;
	attr_uuid_t	ixp_steering_attr;
	attr_uuid_t	host_steering_attr;
	uint32_t 	current_line_steering;
	uint32_t 	current_ixp_steering;
	uint32_t 	current_host_steering;
	daginf_t *      daginf;	
	
	/* get the card info */
	daginf = dag_info(dagfd);
	assert(daginf != NULL);
	
	/* create the filename for the dag card */
	sprintf (str_dev, "%d", daginf->id);
	if (-1 == dag_parse_name(str_dev, dagname, DAGNAME_BUFSIZE, &dagstream))
	{
		EMA_PVERBOSE (1, "dag_parse_name failed\n");
		return -1;
	}

	
	/* open the config API */
	card_ref = dag_config_init (dagname);
	if ( card_ref == NULL )
	{
		EMA_PVERBOSE (1, "dag_config_init failed\n");;
		return -1;
	}
	
	
	/* get a reference to the root component */
	root_component = dag_config_get_root_component (card_ref);
	if ( root_component == NULL )
	{
		dag_config_dispose(card_ref);
		EMA_PVERBOSE (1, "dag_config_get_root_component failed\n");
		return -1;
	}
	
	/* get a reference to the port component */
	erfmux = dag_component_get_subcomponent(root_component, kComponentErfMux, 0);
	if ( erfmux != NULL )
        {

                /* save the current status of erfmux */
                line_steering_attr = dag_component_get_attribute_uuid (erfmux, kUint32AttributeLineSteeringMode);
                host_steering_attr = dag_component_get_attribute_uuid (erfmux, kUint32AttributeHostSteeringMode);
                ixp_steering_attr  = dag_component_get_attribute_uuid (erfmux, kUint32AttributeIXPSteeringMode);

                current_line_steering =  dag_config_get_uint32_attribute(card_ref, line_steering_attr);
                current_host_steering =  dag_config_get_uint32_attribute(card_ref, host_steering_attr);
                current_ixp_steering  = dag_config_get_uint32_attribute(card_ref, ixp_steering_attr);  

                /* Steer packet from line to host, host to line, ixp to ixp */
                dag_config_set_uint32_attribute (card_ref, line_steering_attr, kSteerHost);
                assert (kSteerHost == dag_config_get_uint32_attribute (card_ref, line_steering_attr));

                dag_config_set_uint32_attribute (card_ref, ixp_steering_attr, kSteerIXP);
                assert (kSteerIXP == dag_config_get_uint32_attribute (card_ref, ixp_steering_attr));

                dag_config_set_uint32_attribute (card_ref, host_steering_attr, kSteerLine);
                assert (kSteerLine == dag_config_get_uint32_attribute (card_ref, host_steering_attr));

                /* reset the embedded processor */
                result = dagema_reset_processor_with_cb(dagfd, flags, NULL);
	
                /* restore the status of erfmux */
                dag_config_set_uint32_attribute (card_ref, line_steering_attr, current_line_steering);
                dag_config_set_uint32_attribute (card_ref, host_steering_attr, current_host_steering);
                dag_config_set_uint32_attribute (card_ref, ixp_steering_attr, current_ixp_steering);

                dag_config_dispose (card_ref);

                return result;

        }
        else
        {
                /* reset the embedded processor */
                result = dagema_reset_processor_with_cb(dagfd, flags, NULL);
                return result;
        }

}




/**********************************************************************
 *
 * FUNCTION:     dagema_reset_processor_with_cb
 *
 * DESCRIPTION:  Resets the embedded processor, same as the dagema_reset_processor
 *               function except with the additional option of installing
 *               a progress handler that receives notifications during
 *               the reset process.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               flags        IN      Optional flags used to indicate
 *                                    how the boot process should run.
 *                                    Possible values are:
 *                                     - EMA_RUN_SRAM_MEMORY_TEST
 *                                     - EMA_RUN_DRAM_MEMORY_TEST
 *
 * RETURNS:      0 if the reset was successiful, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int
dagema_reset_processor_with_cb (int dagfd, uint32_t flags, reset_handler_t handler)
{
	daginf_t*      daginf;
	ema_context_t* ctxP;
	ema_context_t  tmp_ctx;
	char*          iom;
	int            result;

	g_last_errno = ENONE;

	/*  */
	memset (&tmp_ctx, 0, sizeof(ema_context_t));

	/* get the card id number and type */
	daginf = dag_info(dagfd);
	if ( daginf == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}
		

	
	/* check if this card has already been opened with a call to dagema_open_conn */
    if(g_ema_contexts != NULL)
    {
        for ( ctxP=g_ema_contexts; ctxP!=NULL; ctxP=ctxP->next_ctx )
            if ( ctxP->dag_id == daginf->id )
            {
                g_last_errno = EEXIST;
                return -1;
            }
    }

	/* check if someother instance of the EMA has an open connection to the card */
	tmp_ctx.dag_id = daginf->id;
	if ( lock_ema(&tmp_ctx) != 0 )
	{
		g_last_errno = ELOCKED;
		return -1;
	}
	

	
	
	
	/*  */
	iom = (char*) dag_iom(dagfd);
	

	/* start the actual reset process (highly dependent on the type of card) */
	if ( daginf->device_code == PCI_DEVICE_ID_DAG7_10 )
		result = ema_reset_dag7_1s(dagfd, iom, flags, handler);
		
	else if ( daginf->device_code == PCI_DEVICE_ID_DAG3_70T )
		result = ema_reset_dag3_7t(dagfd, iom, flags, handler);
	else if ( daginf->device_code == PCI_DEVICE_ID_DAG3_7T4 )
		result = ema_reset_dag3_7t(dagfd, iom, flags, handler);
	else
	{
		g_last_errno = ECARDNOTSUPPORTED;
		result = -1;
	}
	
	
	/* unlock the EMA */
	unlock_ema(&tmp_ctx);
	
	return result;
}




/**********************************************************************
 *
 * FUNCTION:     dagema_halt_processor
 *
 * DESCRIPTION:  Halts the embedded processor, the process remains in
 *               a halted state until dagema_reset_processor is called.
 *               All open EMA connections must be closed prior to calling
 *               this function.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *
 * RETURNS:      0 if the halt was successiful, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int
dagema_halt_processor (int dagfd)
{
	daginf_t*      daginf;
	ema_context_t* ctxP;
	ema_context_t  tmp_ctx;
	char*          iom;
	int            result;

	g_last_errno = ENONE;

	/*  */
	memset (&tmp_ctx, 0, sizeof(ema_context_t));

	/* get the card id number and type */
	daginf = dag_info(dagfd);
	if ( daginf == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}
		

	
	/* check if this card has already been opened with a call to dagema_open_conn */
	for ( ctxP=g_ema_contexts; ctxP!=NULL; ctxP=ctxP->next_ctx )
		if ( ctxP->dag_id == daginf->id )
		{
			g_last_errno = EEXIST;
			return -1;
		}


	/* check if some other instance of the EMA has an open connection to the card */
	tmp_ctx.dag_id = daginf->id;
	if ( lock_ema(&tmp_ctx) != 0 )
	{
		g_last_errno = ELOCKED;
		return -1;
	}
	

	
	
	
	/*  */
	iom = (char*) dag_iom(dagfd);
	

	/* start the actual reset process (highly dependent on the type of card) */
	if ( daginf->device_code == PCI_DEVICE_ID_DAG7_10 )
		result = ema_halt_dag7_1s(dagfd, iom);
		
	else if ( daginf->device_code == PCI_DEVICE_ID_DAG3_70T )
		result = ema_halt_dag3_7t(dagfd, iom);
	
	else if ( daginf->device_code == PCI_DEVICE_ID_DAG3_7T4 )
		result = ema_halt_dag3_7t(dagfd, iom);

	else
	{
		g_last_errno = ECARDNOTSUPPORTED;
		result = -1;
	}
	
	
	/* unlock the EMA */
	unlock_ema(&tmp_ctx);
	
	return result;
}



/**********************************************************************
 *
 * FUNCTION:     dagema_send_msg
 *
 * DESCRIPTION:  Call to send a message to the embedded processor, a
 *               connection should have already been opened prior to
 *               calling this function.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               message_id   IN      The ID of the message to send
 *               length       IN      The number of bytes in the message,
 *                                    sizeof(tx_message)
 *               tx_message   IN      Buffer of at least 'length' size
 *                                    containing the data to send.
 *
 * RETURNS:      0 if message was sent, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 * NOTES:        This functions packages the message into a ema_msg_t which
 *               includes the message_id, length and version, this is the
 *               format sent to the embedded processor. 
 *
 **********************************************************************/
int 
dagema_send_msg (int dagfd, uint32_t message_id, uint32_t length, const uint8_t *tx_message, uint8_t *trans_id)
{
	int               res;
	ema_context_t*    ctx;
	ema_msg_t         msg;
	
	g_last_errno = ENONE;
	
	/* sanity check the size of the message */
	if ( length > EMA_MSG_PAYLOAD_LENGTH )
	{
		g_last_errno = ERANGE;
		return -1;
	}
	if ( (length != 0) && (tx_message == NULL) )
	{
		g_last_errno = EINVAL;
		return -1;
	}
	
	

	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
			break;

	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}
	
	

	
	/* copy over the message and set the header values */
	msg.data.message_id = dagema_htole32(message_id);
	msg.data.length     = dagema_htole16((uint16_t)(length & 0xFFFF));
	msg.data.version    = ctx->msg_version;

	if ( length > 0 )
		memcpy (&(msg.data.payload[0]), tx_message, length);
		
	
	/* set the transaction id if version 1 or greater headers are used */
	msg.data.trans_id   = (ctx->msg_version > 0) ? ctx->current_trans_id : 0x00;
	ctx->current_trans_id = (ctx->current_trans_id == 0xFF) ? 0x01 : (ctx->current_trans_id+1);

	
	EMA_PVERBOSE(2, "-> dagema_send_msg(msgid=0x%x, length=%d)\n", message_id, length);
	
	
	/* write the message */
	res = send(ctx->client_sock, (const char*)&msg, (EMA_MSG_HEADER_LENGTH + length), 0);
	if (res < 0)
	{
#if defined(_WIN32)
		g_last_errno = WSAGetLastError();
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		g_last_errno = errno;
#endif
		return -1;
	}

	EMA_PVERBOSE (2, "<- dagema_send_msg(msgid=0x%x, length=%d) res=%d\n", message_id, length, res);
	
	
	return 0;	
}








/**********************************************************************
 *
 * FUNCTION:     get_msg_from_socket
 *
 * DESCRIPTION:  Reads a message from the socket, this function blocks
 *               until a message can be read from the socket. Either
 *               dagema_recv_msg or dagema_recv_msg_timeout calls this
 *               function to get a message, dagema_recv_msg_timeout first
 *               checks if there is any messages in the socket before
 *               calling this function, whereas dagema_recv_msg doesn't.
 *
 * PARAMETERS:   ctx          IN      Pointer to the EMA context for the
 *                                    card.
 *               message_id   OUT     Pointer to a value that receives
 *                                    the message id.
 *               length       IN/OUT  Pointer to a value that upon entry
 *                                    should contain the maximum size
 *                                    of the rx_message buffer, upon return
 *                                    will contain the number of bytes
 *                                    copied into rx_message.
 *               rx_message   OUT     Buffer to store the message bytes
 *                                    in. If the buffer isn't big enough
 *                                    for all the data, it is trimed to
 *                                    fit and the overflow is discarded.
 *
 * RETURNS:      0 if a packet was retreived otherwise and a negative
 *               error code.
 *
 **********************************************************************/
static int 
get_msg_from_socket (ema_context_t* ctx, uint32_t *message_id, uint32_t *length, uint8_t *rx_message, uint8_t *trans_id)
{
	int        res;
	ema_msg_t  recv_msg;
	int        recv_msg_len;
	int        size_in;
	
	

	/* sanity checking */
	if ( ctx == NULL || message_id == NULL || length == NULL || rx_message == NULL )
	{
		g_last_errno = EINVAL;
		return -1;
	}
	
	
	/* attempt to read the first eight bytes from the socket */	
	res = recv (ctx->client_sock, (char *)&recv_msg, EMA_MSG_HEADER_LENGTH, MSG_PEEK);
	if (res < 0)
	{
#if defined(_WIN32)
		g_last_errno = WSAGetLastError();
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		g_last_errno = errno;
#endif
	
		EMA_PVERBOSE (1, "recv failed");
		return -1;
	}
	
	
	if (res < 8)
	{
#if defined(_WIN32)
		g_last_errno = WSAGetLastError();
#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		g_last_errno = errno;
#endif

		EMA_PVERBOSE (1, "recv to small message ( < 8 )");
		return -1;
	}


	
	/* copy in the message id */
	*message_id   = dagema_le32toh (recv_msg.data.message_id);
	recv_msg_len  = dagema_le16toh (recv_msg.data.length);
	recv_msg_len += EMA_MSG_HEADER_LENGTH;


	/* determine if there is enough data in the socket for the complete message */
	res = recv (ctx->client_sock, (char *)&recv_msg.data.payload, recv_msg_len, MSG_PEEK);
	if ( res < recv_msg_len )
	{
		EMA_PVERBOSE (1, "recv to small message ( < %d )", recv_msg_len);
		g_last_errno = ERESP;
		return -1;
	}
	

	

	/* read the actual message out of the socket (the above calls have just been
	 * peeking at the data).
	 */
	res = recv (ctx->client_sock, (char *)&recv_msg, recv_msg_len, 0);
	if ( res != recv_msg_len )
	{
		EMA_PVERBOSE (1, "failed to receive all the message ( < 8 )");
		g_last_errno = ERESP;
		return -1;
	}
	


	
	
	/* copy the message across to the supplied buffer (trim if nesscery)*/
	size_in = *length;
	*length = (res - EMA_MSG_HEADER_LENGTH);

	if ( res > size_in )
		res = size_in;
	
	memcpy (rx_message, &(recv_msg.data.payload[0]), res);
	
	
	/* copy the transaction id */
	if (trans_id)
		*trans_id = recv_msg.data.trans_id;
	
		
	/* last bit of debugging */
	EMA_PVERBOSE (2, "<- dagema_recv_msg(msgid=0x%x, length=%d)\n", *message_id, recv_msg_len);
	
	return 0;
}



/**********************************************************************
 *
 * FUNCTION:     dagema_recv_msg
 *
 * DESCRIPTION:  Call to poll for messages from the embedded processor,
 *               this function blocks while waiting for a message. You
 *               might want to consider dagema_recv_msg_timeout() or installing
 *               a callback message handler with dagema_set_msg_handler().
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               message_id   OUT     Pointer to a value that receives
 *                                    the message id.
 *               length       IN/OUT  Pointer to a value that upon entry
 *                                    should contain the maximum size
 *                                    of the rx_message buffer, upon return
 *                                    will contain the number of bytes
 *                                    copied into rx_message.
 *               rx_message   OUT     Buffer to store the message bytes
 *                                    in. If the buffer isn't big enough
 *                                    for all the data, it is trimed to
 *                                    fit and the overflow is discarded.
 *               trans_id     OUT     The transaction ID of the received
 *                                    message, this can be NULL if the
 *                                    transaction ID is not used.
 *
 * RETURNS:      0 if message was received, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int 
dagema_recv_msg (int dagfd, uint32_t *message_id, uint32_t *length, uint8_t *rx_message, uint8_t *trans_id)
{
#ifdef DAGEMA_USE_SELECT
	ema_poll_set_t    poll_set;
	ema_poll_set_t    poll_set_tmp;
#else
	struct pollfd     pfd;
#endif
	int               res;
	ema_context_t*    ctx;
	

	g_last_errno = ENONE;

	/* clear the transaction id */
	if (trans_id)	*trans_id = 0;

	
	/* sanity checking */
	if ( message_id == NULL || length == NULL || rx_message == NULL )
	{
		g_last_errno = EINVAL;
		return -1;
	}
	
	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
			break;
		
	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}
	



	/* Read the message header */
	EMA_PVERBOSE (2, "-> dagema_recv_msg()\n");


#ifdef DAGEMA_USE_SELECT
	/* Use select to wait for data to be available to read on the socket */
	FD_ZERO(&poll_set.read);
	FD_ZERO(&poll_set.write);
	FD_ZERO(&poll_set.exception);
	poll_set.max_sd = 0;

	FD_SET(ctx->client_sock, &poll_set.read);
	FD_SET(ctx->client_sock, &poll_set.exception);
	poll_set.max_sd = (int)ctx->client_sock;

	poll_set_tmp = poll_set;

	while (((res=select(poll_set.max_sd+1, &poll_set_tmp.read,
		&poll_set_tmp.write, &poll_set_tmp.exception, NULL)) == -1) && (errno==EINTR))
	{
		/* Select modifies the poll parameters, so reset them before trying again */
		poll_set_tmp = poll_set;
	}

	if (res <= 0)
	{
		EMA_PVERBOSE (2, "timeout waiting for message\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}
	
#else
	/* Poll for data so we can timeout */
	pfd.fd = ctx->client_sock;
	pfd.events = POLLIN | POLLPRI;
	pfd.revents = 0;
	res = poll(&pfd, 1, -1);

	if (res <= 0)
	{
		EMA_PVERBOSE (2, "timeout waiting for message\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}
	
#endif



	return get_msg_from_socket (ctx, message_id, length, rx_message, trans_id);
}



/**********************************************************************
 *
 * FUNCTION:     dagema_recv_msg_timeout
 *
 * DESCRIPTION:  The same as dagema_recv_msg() except with an extra timeout
 *               parameter.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               message_id   OUT     Pointer to a value that receives
 *                                    the message id.
 *               length       IN/OUT  Pointer to a value that upon entry
 *                                    should contain the maximum size
 *                                    of the rx_message buffer, upon return
 *                                    will contain the number of bytes
 *                                    copied into rx_message.
 *               rx_message   OUT     Buffer to store the message bytes
 *                                    in. If the buffer isn't big enough
 *                                    for all the data, it is trimed to
 *                                    fit and the overflow is discarded.
 *               trans_id     OUT     The transaction ID of the received
 *                                    message, this can be NULL if the
 *                                    transaction ID is not used.
 *               timeout      IN      Timeout in milliseconds, if timed out
 *                                    -1 will be returned and dagema_get_last_error()
 *                                    will return ETIMEDOUT.
 *
 * RETURNS:      0 if message was received, otherwise -1. Call
 *               dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int
dagema_recv_msg_timeout (int dagfd, uint32_t *message_id, uint32_t *length, uint8_t *rx_message, uint8_t *trans_id, uint32_t timeout)
{
#ifdef DAGEMA_USE_SELECT
	ema_poll_set_t    poll_set;
	ema_poll_set_t    poll_set_tmp;
	struct timeval    tv;
#else
	struct pollfd     pfd;
#endif
	int               res;
	ema_context_t*    ctx;
	
	
	/* clear the last error */
	g_last_errno = ENONE;
	
	
	/* clear the transaction id */
	if (trans_id)	*trans_id = 0;

	
	/* sanity checking */
	if ( message_id == NULL || length == NULL || rx_message == NULL )
	{
		g_last_errno = EINVAL;
		return -1;
	}
	
	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
			break;
		
	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}
	
	

	/* Read the message header */
	EMA_PVERBOSE (2, "-> dagema_recv_msg()\n");


#ifdef DAGEMA_USE_SELECT
	/* Use select to wait for data to be available to read on the socket */
	FD_ZERO(&poll_set.read);
	FD_ZERO(&poll_set.write);
	FD_ZERO(&poll_set.exception);
	poll_set.max_sd = 0;

	FD_SET(ctx->client_sock, &poll_set.read);
	FD_SET(ctx->client_sock, &poll_set.exception);
	poll_set.max_sd = (int)ctx->client_sock;

	poll_set_tmp = poll_set;
	tv.tv_sec    = 0;
	tv.tv_usec   = timeout * 1000;

	while (( (res=select(poll_set.max_sd+1, &poll_set_tmp.read,
		&poll_set_tmp.write, &poll_set_tmp.exception, &tv)) == -1) && (errno == EINTR))
	{
		/* Select modifies the poll parameters, so reset them before trying again */
		poll_set_tmp = poll_set;
	}

	if (res <= 0)
	{
		EMA_PVERBOSE (2, "timeout waiting for message\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}
	
#else
	/* Poll for data so we can timeout */
	pfd.fd = ctx->client_sock;
	pfd.events = POLLIN | POLLPRI;
	pfd.revents = 0;
	res = poll(&pfd, 1, timeout);

	if (res <= 0)
	{
		EMA_PVERBOSE (2, "timeout waiting for message\n");
		g_last_errno = ETIMEDOUT;
		return -1;
	}
	
#endif

	/* read the message from the socket */
	return get_msg_from_socket (ctx, message_id, length, rx_message, trans_id);
}







/**********************************************************************
 *
 * FUNCTION:     dagema_set_msg_handler
 *
 * DESCRIPTION:  Installs a callback handler for the messages from the
 *               xScale. The callback will be called whenever a complete
 *               message has been received from the dag card.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               msg_handler  IN      Callback function pointer
 *
 * RETURNS:      0 if the message hnadler was installed, otherwise -1.
 *               Call dagema_get_last_error() to retrieve the error code.
 *
 **********************************************************************/
int
dagema_set_msg_handler(int dagfd, msg_handler_t msg_handler)
{
	ema_context_t*  ctx;
	
	g_last_errno = ENONE;
	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
			break;
	
	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}

	
	ctx->message_handler = msg_handler;
	
	return 0;
}






/**********************************************************************
 *
 * FUNCTION:     dagema_set_ctrl_msg_handler
 *
 * DESCRIPTION:  Installs a callback handler for internal control messages
 *               that are sent by clients. This function directly
 *               interfaces to the DRB layer, and is only used for 
 *               console access on the 3.7t.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               ctrl_handler IN      Callback function pointer
 *
 * RETURNS:      0 if no error occuired otherwise a negative error code.
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)

int
dagema_set_ctrl_msg_handler(int dagfd, ctrl_msg_handler_t ctrl_handler)
{
	ema_context_t*  ctx;
 	daginf_t*       daginf;
	
	
	g_last_errno = ENONE;
	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
			break;
	
	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}

	
	/* check to make sure the card is a 3.7t or 3.7t4 */
	daginf = dag_info(dagfd);
	if (daginf->device_code != PCI_DEVICE_ID_DAG3_70T && 
	    daginf->device_code != PCI_DEVICE_ID_DAG3_7T4 )
	{
		g_last_errno = ECARDNOTSUPPORTED;
		return -1;		
	}
	

	/* set the callback function */	
	ctx->ctrl_msg_handler = ctrl_handler;
	
	
	return 0;
}

#endif		// DAG_CONSOLE


/**********************************************************************
 *
 * FUNCTION:     dagema_set_console_handler
 *
 * DESCRIPTION:  Installs a callback handler for the console data that
 *               is sent via the DRB registers. This function directly
 *               interfaces to the DRB layer, and is only used for 
 *               console access on the 3.7t.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               char_handler IN      Callback function pointer
 *
 * RETURNS:      0 if no error occuired otherwise a negative error code.
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)
int (*g_console_char_handler)(int c);

int dagema_console_char_handler(int c)
{
	return g_console_char_handler(c);
}

int
dagema_set_console_handler(int dagfd, console_putchar_t char_handler)
{
	ema_context_t*  ctx;
 	daginf_t*       daginf;
	
	
	g_last_errno = ENONE;
	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
			break;
	
	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}

	
	/* check to make sure the card is a 3.7t or 3.7t4*/
	daginf = dag_info(dagfd);
	if (daginf->device_code != PCI_DEVICE_ID_DAG3_70T &&
	    daginf->device_code != PCI_DEVICE_ID_DAG3_7T4 )
	{
		g_last_errno = ECARDNOTSUPPORTED;
		return -1;
	}

	g_console_char_handler = char_handler;
	
#if 0
	if (drb_37t_set_console_handler(ctx->drv_handle, char_handler) == -1)
	{
		g_last_errno = EINVAL;
		return -1;
	}
#endif
	
	return 0;
}

#endif		// DAG_CONSOLE



/**********************************************************************
 *
 * FUNCTION:     dagema_write_console_character
 *
 * DESCRIPTION:  Writes a console string of characters out the DRB lines,
 *               this function directly connects to the DRB layer and
 *               is only used for console access on the 3.7t.
 *
 * PARAMETERS:   dagfd        IN      A unix style file descriptor
 *                                    returned by dag_open.
 *               str          IN      Characters to send
 *               len          IN      Number of characters
 *
 * RETURNS:      0 if no error occuired otherwise a negative error code.
 *
 **********************************************************************/
#if defined(DAG_CONSOLE)
int (*g_dagema_console_send) (DRB_COMM_HANDLE, char*, int);
int dagema_console_drv_reg(int (*p_send) (DRB_COMM_HANDLE, char*, int))
{
	if(p_send)
	{
		g_dagema_console_send = p_send;
		return 0;
	}
	return -1;
}

int
dagema_write_console_string(int dagfd, char* str, int len)
{
	ema_context_t*  ctx;
 	daginf_t*       daginf;
	
	
	g_last_errno = ENONE;
	
	/* sanity checking */
	if ( str == NULL || len < 1 )
	{
		g_last_errno = EINVAL;
		return -1;
	}
	
	/* grab the correct context from the global linked list */
	for ( ctx=g_ema_contexts; ctx!=NULL; ctx=ctx->next_ctx )
		if ( ctx->dagfd == dagfd )
			break;
	
	if ( ctx == NULL )
	{
		g_last_errno = EBADF;
		return -1;
	}

	
	/* check to make sure the card is a 3.7t or 3.7t4 */
	daginf = dag_info(dagfd);
	if (daginf->device_code != PCI_DEVICE_ID_DAG3_70T && 
	    daginf->device_code != PCI_DEVICE_ID_DAG3_7T4 )
	{
		g_last_errno = ECARDNOTSUPPORTED;
		return -1;		
	}
	
	/* write the characters into the DRB console buffer */
	if (g_dagema_console_send(ctx->drv_handle, str, len) == -1)
	{
		g_last_errno = EINVAL;
		return -1;		
	}
	
	
	return 0;
}

#endif		// DAG_CONSOLE




/**********************************************************************
 *
 * FUNCTION:     DllMain
 *
 * DESCRIPTION:  This is required only so the Winsocks library can be
 *               identified and started.  An equivalent is not required
 *               on non-Windows platforms.
 *
 * PARAMETERS:   hModule      IN      
 *               reason       IN      
 *               lpReserved   IN
 *
 * RETURNS:      
 *
 **********************************************************************/
#if defined(_WIN32)

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reason, 
                       LPVOID lpReserved
					 )
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:

		wVersionRequested = MAKEWORD( 2, 2 );

		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) {
			/* Tell the user that we could not find a usable */
			/* WinSock DLL.                                  */
			return FALSE;
		}

		break;

	case DLL_PROCESS_DETACH:
		WSACleanup( );

		break;
	}
	return TRUE;
}

#endif
