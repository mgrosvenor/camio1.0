/**********************************************************************
*
* Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: aal_config_msg.c,v 1.25 2009/01/30 01:13:19 wilson.zhu Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         dag_aal5_config_api.c
* DESCRIPTION:	Messaging wrappers around the Embedded Message API
*
*
* HISTORY: 
*   	
*
**********************************************************************/

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include "dagapi.h"
#include "aal_msg_types.h"
#include "aal_config_msg.h"
#include "dagsarapi.h"




/**********************************************************************
 *
 * Constants
 *
 **********************************************************************/

#define DEFAULT_TIMEOUT          2000   /* 2 seconds */


extern int filter_id[];



/**********************************************************************
* FUNCTION:     aal_msg_activate_vc
* DESCRIPTION:  activate a virtual connection on the aal reassembler to
*               start sending all data it sees to the host.
* INPUTS:	
*	dagfd - dag card descriptor
*   iface - interface (used by non 3.7t cards)
*   VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
* OUTPUTS:	None
* RETURNS:	0 if activated
*           1 if could not be activated
**********************************************************************/
uint32_t aal_msg_activate_vc(uint32_t dagfd,
                             uint32_t iface,
                             uint32_t VCI, 
                             uint32_t VPI,
                             uint32_t connection_num)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_activate_vc cmd;
	t_aal_msg_activate_vc_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_connection_num(connection_num) != 0)
		return -8;
	if(aal_msg_check_vpi(VPI) != 0)
		return -9;
	if(aal_msg_check_vci(VCI) != 0)
		return -10;
	if(aal_msg_check_interface(iface) != 0)
		return -11;


	msg_id    = AAL_MSG_ACTIVATE_VC;
	length    = AAL_MSG_ACTIVATE_VC_SIZE;
	cmd.IFace = dagema_htole32(iface);
	cmd.VCI   = dagema_htole32(VCI);
	cmd.VPI   = dagema_htole32(VPI);
	cmd.ConnectionNum = dagema_htole32(connection_num);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
	{
		return -6;
	}
    
	
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
	{
		return -3;
	}
	
	/* confirm the response */
	if (msg_id != AAL_MSG_ACTIVATE_VC_CMPLT || length != AAL_MSG_ACTIVATE_VC_CMPLT_SIZE)
	{
		return -7;
	}
    
	return dagema_le32toh(reply.result);
} /* dag_activate_vc */






/**********************************************************************
* FUNCTION:     aal_msg_remove_vc
* DESCRIPTION:  remove a Virtual Connection from the list
* INPUTS:	
*	dagfd - dag card descriptor
*   iface - interface (used by non 3.7t cards)
*	VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
* OUTPUTS:	None
* RETURNS:	0 if successful, 1 if unsuccessful
**********************************************************************/
uint32_t aal_msg_remove_vc(uint32_t dagfd, 
                           uint32_t iface,
                           uint32_t VCI, 
                           uint32_t VPI,
                           uint32_t connection_num)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_remove_vc cmd;
	t_aal_msg_remove_vc_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_connection_num(connection_num) != 0)
		return -8;
	if(aal_msg_check_vpi(VPI) != 0)
		return -9;
	if(aal_msg_check_vci(VCI) != 0)
		return -10;
	if(aal_msg_check_interface(iface) != 0)
		return -11;

 
    	msg_id    = AAL_MSG_REMOVE_VC;
    	length    = AAL_MSG_REMOVE_VC_SIZE;
	cmd.IFace = dagema_htole32(iface);
	cmd.VCI   = dagema_htole32(VCI);
	cmd.VPI   = dagema_htole32(VPI);
	cmd.ConnectionNum = dagema_htole32(connection_num);

 	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT*20) != 0) 
    	{
        	return -3;
    	}

	/* confirm the response */
	if (msg_id != AAL_MSG_REMOVE_VC_CMPLT || length != AAL_MSG_REMOVE_VC_CMPLT_SIZE)
	{
		return -7;
	}

    return dagema_le32toh(reply.result);
} /* dag_remove_vc */





/**********************************************************************
* FUNCTION:     aal_msg_activate_cid
* DESCRIPTION:  activate a cid on the aal reassembler to
*				start sending all data it sees to the host.
* INPUTS:	
*	dagfd - dag card descriptor
*   iface - interface (used by non 3.7t cards)
*	VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
*   cid - cid to be activated
* OUTPUTS:	None
* RETURNS:	0 if activated
*           1 if could not be activated
**********************************************************************/
uint32_t aal_msg_activate_cid(uint32_t dagfd, 
                              uint32_t iface,
                              uint32_t VCI, 
                              uint32_t VPI,
                              uint32_t connection_num,
                              uint32_t cid)
{
	uint32_t msg_id;
	uint32_t length;
    	t_aal_msg_activate_cid cmd;
    	t_aal_msg_activate_cid_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_connection_num(connection_num) != 0)
		return -8;
	if(aal_msg_check_vpi(VPI) != 0)
		return -9;
	if(aal_msg_check_vci(VCI) != 0)
		return -10;
	if(aal_msg_check_interface(iface) != 0)
		return -11;

	msg_id            = AAL_MSG_ACTIVATE_CID;
	length            = AAL_MSG_ACTIVATE_CID_SIZE;
    	cmd.VCI           = dagema_htole32(VCI);	
	cmd.VPI           = dagema_htole32(VPI);
	cmd.ConnectionNum = dagema_htole32(connection_num);
	cmd.CID           = dagema_htole32(cid);
	cmd.iface         = dagema_htole32(iface);

 	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
   	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_ACTIVATE_CID_CMPLT || length != AAL_MSG_ACTIVATE_CID_CMPLT_SIZE)
	{
		return -7;
	}

    return dagema_le32toh(reply.result);
} /* dag_activate_cid */





/**********************************************************************
* FUNCTION:     aal_msg_deactivate_cid
* DESCRIPTION:  deactivate a cid on the reassembler
* INPUTS:	
*	dagfd - dag card descriptor
*   iface - interface (used by non 3.7t cards)
*	VCI - VCI identifier of virtual connection
*   VPI - VPI identifier of virtual connection
*   connection_num - connection number identifier of virtual connection
*   cid - cid to be deactivated
* OUTPUTS:	None
* RETURNS:	0 if successful, 1 if unsuccessful
**********************************************************************/
uint32_t aal_msg_deactivate_cid(uint32_t dagfd, 
                                uint32_t iface,
                                uint32_t VCI, 
                                uint32_t VPI,
                                uint32_t connection_num,
                                uint32_t cid)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_deactivate_cid cmd;
	t_aal_msg_deactivate_cid_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_connection_num(connection_num) != 0)
		return -8;
	if(aal_msg_check_vpi(VPI) != 0)
		return -9;
	if(aal_msg_check_vci(VCI) != 0)
		return -10;
	if(aal_msg_check_interface(iface) != 0)
		return -11;

    	msg_id  = AAL_MSG_DEACTIVATE_CID;
    	length  = AAL_MSG_DEACTIVATE_CID_SIZE;
    	cmd.VCI = dagema_htole32(VCI);	
	cmd.VPI = dagema_htole32(VPI);
	cmd.ConnectionNum = dagema_htole32(connection_num);
	cmd.iface = dagema_htole32(iface);
	cmd.CID   = dagema_htole32(cid);

 	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}

	/* confirm the response */
	if (msg_id != AAL_MSG_DEACTIVATE_CID_CMPLT || length != AAL_MSG_DEACTIVATE_CID_CMPLT_SIZE)
	{
		return -7;
	}
    
    return dagema_le32toh(reply.result);
} /* dag_deactivate_cid */





/**********************************************************************
* FUNCTION:     aal_msg_report_dropped_cells
* DESCRIPTION:  get the virtual connections that have dropped cells
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:  The number of dropped cells
**********************************************************************/
uint32_t aal_msg_report_statistics(uint32_t dagfd,
                                   uint32_t statistic)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_read_statistic cmd;
	t_aal_msg_read_statistic_cmplt reply;

	msg_id = AAL_MSG_READ_STATISTIC;
	length = AAL_MSG_READ_STATISTIC_SIZE;
	cmd.Statistic = dagema_htole32(statistic);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
	
	/* confirm the response */
	if (msg_id != AAL_MSG_READ_STATISTIC_CMPLT || length != AAL_MSG_READ_STATISTIC_CMPLT_SIZE)
	{
		return -7;
	}
	
	return dagema_le32toh(reply.statistic);
} /* dag_report_statistic */

/**********************************************************************
* FUNCTION:     aal_msg_set_net_mode
* DESCRIPTION:  set the mode (uni/nni) of the given vc
* INPUTS:	
*	dagfd - dag card descriptor
*	net_mode - net mode to set
*   	iface - interface (used by non 3.7t cards)
*	connection_num, vpi, vci - identifiers to the virtual 
*	channel to be set
*	
* OUTPUTS:	None
* RETURNS:	0 on sucess, non-zero if unsuccessful;
**********************************************************************/
uint32_t aal_msg_set_connection_net_mode(uint32_t dagfd, 
				    		  net_mode_t net_mode, 
						  uint32_t iface,
				    		  uint32_t connection_num)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_net_mode cmd;
    	t_aal_msg_set_net_mode_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_connection_num(connection_num) != 0)
		return -8;

    	msg_id  = AAL_MSG_SET_NET_MODE;
    	length  = AAL_MSG_SET_NET_MODE_SIZE;
	cmd.NetMode = dagema_htole32(net_mode);
	cmd.ConnectionNum = dagema_htole32(connection_num);
	cmd.IFace = dagema_htole32(iface);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_SET_NET_MODE_CMPLT || length != AAL_MSG_SET_NET_MODE_CMPLT_SIZE)
	{
		return -7;
	}

	return dagema_le32toh(reply.result);
} /* dag_set_net_mode */


/**********************************************************************
* FUNCTION:     aal_msg_set_iface_net_mode
* DESCRIPTION:  set the mode (uni/nni) of the given interface
* INPUTS:	
*	dagfd - dag card descriptor
*	net_mode - net mode to set
*   	iface - interface (used by non 3.7t cards)
*	
* OUTPUTS:	None
* RETURNS:	0 on sucess, non-zero if unsuccessful;
**********************************************************************/
uint32_t aal_msg_set_iface_net_mode(uint32_t dagfd, 
				    	     net_mode_t net_mode, 
					     uint32_t iface)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_iface_net_mode cmd;
    	t_aal_msg_set_iface_net_mode_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_interface(iface) != 0)
		return -11;

    	msg_id  = AAL_MSG_SET_IFACE_NET_MODE;
    	length  = AAL_MSG_SET_IFACE_NET_MODE_SIZE;
	cmd.NetMode = dagema_htole32(net_mode);
	cmd.IFace = dagema_htole32(iface);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_SET_IFACE_NET_MODE_CMPLT || length != AAL_MSG_SET_IFACE_NET_MODE_CMPLT_SIZE)
	{
		return -7;
	}

	return dagema_le32toh(reply.result);
} /* dag_set_net_mode */

/**********************************************************************
* FUNCTION:     aal_msg_get_sar_mode
* DESCRIPTION:  gets the sar mode for a vc
* INPUTS:	
*	dagfd - dag card descriptor
*   	iface - interface (used by non 3.7t cards)
*	connection_num, vpi, vci - identifiers to the virtual 
*	channel to be get
*	
* OUTPUTS:	None
* RETURNS:	the sar mode for the vc;
**********************************************************************/
uint32_t aal_msg_get_sar_mode(uint32_t dagfd, 
                             uint32_t iface,
                             uint32_t connection_num,
                             uint32_t vpi,
                             uint32_t vci)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_get_sar_mode cmd;
    	t_aal_msg_get_sar_mode_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_connection_num(connection_num) != 0)
		return -8;
	if(aal_msg_check_vpi(vpi) != 0)
		return -9;
	if(aal_msg_check_vci(vci) != 0)
		return -10;
	if(aal_msg_check_interface(iface) != 0)
		return -11;

    	msg_id = AAL_MSG_GET_SAR_MODE;
    	length = AAL_MSG_GET_SAR_MODE_SIZE;

	cmd.ConnectionNum = dagema_htole32(connection_num);
	cmd.IFace = dagema_htole32(iface);
	cmd.VPI   = dagema_htole32(vpi);
	cmd.VCI   = dagema_htole32(vci);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}

	/* confirm the response */
	if (msg_id != AAL_MSG_GET_SAR_MODE_CMPLT || length != AAL_MSG_GET_SAR_MODE_CMPLT_SIZE)
	{
		return -7;
	}
	

	return dagema_le32toh(reply.mode);
} /* d37t_msg_get_vc_mode */


/**********************************************************************
* FUNCTION:     aal_msg_set_sar_mode
* DESCRIPTION:  This is now a direct message to the 3.7t. It creates a 
*				new vc setting using a default value for the max_size
* INPUTS:	
*	dagfd - dag card descriptor
*	connection_num, vpi, vci - identifiers to the virtual 
*				   channel to be set
*	sar_mode - the type of channel to create
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_sar_mode(uint32_t dagfd, 
                              uint32_t iface,
                              uint32_t vci, 
                              uint32_t vpi,
                              uint32_t connection_num,
                              sar_mode_t sar_mode)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_sar_mode cmd;
    	t_aal_msg_set_sar_mode_cmplt reply;

	/* Check user input for valid data */
	if(aal_msg_check_connection_num(connection_num) != 0)
		return -8;
	if(aal_msg_check_vpi(vpi) != 0)
		return -9;
	if(aal_msg_check_vci(vci) != 0)
		return -10;
	if(aal_msg_check_interface(iface) != 0)
		return -11;

    	msg_id = AAL_MSG_SET_SAR_MODE;
    	length = AAL_MSG_SET_SAR_MODE_SIZE;

	cmd.ConnectionNum = dagema_htole32(connection_num);
	cmd.IFace         = dagema_htole32(iface);
	cmd.VPI           = dagema_htole32(vpi);
	cmd.VCI           = dagema_htole32(vci);
	cmd.Mode          = dagema_htole32((uint32_t)sar_mode);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
   	}	
    
	/* confirm the response */
	if (msg_id != AAL_MSG_SET_SAR_MODE_CMPLT || length != AAL_MSG_SET_SAR_MODE_CMPLT_SIZE)
	{
		return -7;
	}
	
	return dagema_le32toh(reply.result);
}

/**********************************************************************
* FUNCTION:     aal_msg_set_buffer_size
* DESCRIPTION:	This sets the internal version of the size of the buffer
*				allocated for reassembling aal5 cells
* INPUTS:	
*	dagfd - dag card descriptor
*	size - the size of the channel to create
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_buffer_size(uint32_t dagfd,
                                 uint32_t size)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_buffer_size cmd;
    	t_aal_msg_set_buffer_size_cmplt reply;

    	msg_id = AAL_MSG_SET_BUFFER_SIZE;
    	length = AAL_MSG_SET_BUFFER_SIZE_SIZE;

	cmd.Size = dagema_htole32(size);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_SET_BUFFER_SIZE_CMPLT || length != AAL_MSG_SET_BUFFER_SIZE_CMPLT_SIZE)
	{
		return -7;
	}
	
	return dagema_le32toh(reply.result);

}

/**********************************************************************
* FUNCTION:     filters_msg_set_filter
* DESCRIPTION:	This sets the filter for accepting or rejecting packets
*				based on the values given by the user.
* INPUTS:	
*	msg_id_o - differnet message ID for different filters AAL or HDLC or POSTALL	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t filters_msg_set_filter(uint32_t msg_id_o, uint32_t dagfd,
                            uint32_t offset,
                            uint32_t mask,
                            uint32_t value,
                            uint32_t operation,
                            uint32_t filter_level,
                            uint32_t level_conf,
                            uint32_t history,
                            uint32_t priority)
{
	uint32_t msg_id;
	uint32_t length;
	t_filters_msg_set_filter cmd;
	t_filters_msg_set_filter_cmplt reply;

	msg_id = msg_id_o;
	length = FILTERS_MSG_SET_FILTER_SIZE;

	cmd.offset       = dagema_htole32(offset);
	cmd.mask         = dagema_htole32(mask);
	cmd.value        = dagema_htole32(value);
	cmd.operation    = dagema_htole32(operation);
	cmd.filter_level = dagema_htole32(filter_level);
	cmd.level_conf   = dagema_htole32(level_conf);
	cmd.history      = dagema_htole32(history);
	cmd.priority     = dagema_htole32(priority);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
	{
		return -6;
	}

	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
	{
		return -3;
	}

	/* confirm the response */
	if (msg_id != (msg_id_o+1) || length != FILTERS_MSG_SET_FILTER_CMPLT_SIZE)
	{
		return -7;
	}

	return dagema_le32toh(reply.result);
}

/**********************************************************************
* FUNCTION:     hdlc_msg_set_filter
* DESCRIPTION:	This sets the filter for accepting or rejecting packets
*				based on the values given by the user.
* INPUTS:	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_set_filter(uint32_t dagfd,
                            uint32_t offset,
                            uint32_t mask,
                            uint32_t value,
                            uint32_t operation,
                            uint32_t filter_level,
                            uint32_t level_conf,
                            uint32_t history,
                            uint32_t priority)
{	
		return  filters_msg_set_filter(HDLC_MSG_SET_FILTER, dagfd,
			offset,
			mask,
			value,
			operation,
			filter_level,
			level_conf,
			history,
			priority);
};

/**********************************************************************
* FUNCTION:     aal_msg_set_filter
* DESCRIPTION:	This sets the filter for accepting or rejecting packets
*				based on the values given by the user.
* INPUTS:	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_filter(uint32_t dagfd,
                            uint32_t offset,
                            uint32_t mask,
                            uint32_t value,
                            uint32_t operation,
                            uint32_t filter_level,
                            uint32_t level_conf,
                            uint32_t history,
                            uint32_t priority)
{	
	if( 72 > offset)
	{
		return  filters_msg_set_filter(AAL_MSG_SET_FILTER, dagfd,
			offset,
			mask,
			value,
			operation,
			filter_level,
			level_conf,
			history,
			priority);
	}
	else
		return -1;
};

/**********************************************************************
* FUNCTION:     filters_msg_set_subfilter
* DESCRIPTION:	This sets a subfilter for accepting or rejecting packets
*				based on the values given by the user, at a level below the
*				filter as specifed, with the given operation
* INPUTS:
	msg_id_o - message id for which AAL or HDL filter to use	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
*	parent_id - parent filter to base this subfilter from
*	list_operation - specifies if the subfilter list should be OR or AND
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t filters_msg_set_subfilter(uint32_t msg_id_o, uint32_t dagfd,
                            uint32_t offset,
                            uint32_t mask,
                            uint32_t value,
                            uint32_t operation,
                            uint32_t filter_level,
                            uint32_t level_conf,
                            uint32_t history,
                            uint32_t priority,
							uint32_t parent_id,
							list_operations_t list_operation)
{
	uint32_t msg_id;
	uint32_t length;
	t_filters_msg_set_subfilter cmd;
	t_filters_msg_set_subfilter_cmplt reply;

	msg_id = msg_id_o;
	length = FILTERS_MSG_SET_SUBFILTER_SIZE;

	cmd.offset           = dagema_htole32(offset);
	cmd.mask             = dagema_htole32(mask);
	cmd.value            = dagema_htole32(value);
	cmd.operation        = dagema_htole32(operation);
	cmd.filter_level     = dagema_htole32(filter_level);
	cmd.level_conf       = dagema_htole32(level_conf);
	cmd.history          = dagema_htole32(history);
	cmd.priority         = dagema_htole32(priority);
	cmd.parent_id = dagema_htole32(parent_id);
	cmd.list_operation   = dagema_htole32(list_operation);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
	{
		return -6;
	}

	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
	{
		return -3;
	}

	/* confirm the response */
	if (msg_id != (msg_id_o+1) || length != FILTERS_MSG_SET_SUBFILTER_CMPLT_SIZE)
	{
		return -7;
	}

	return dagema_le32toh(reply.result);
}

/**********************************************************************
* FUNCTION:     aal_msg_set_subfilter
* DESCRIPTION:	This sets a subfilter for accepting or rejecting packets
*				based on the values given by the user, at a level below the
*				filter as specifed, with the given operation
* INPUTS:	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
*	parent_id - parent filter to base this subfilter from
*	list_operation - specifies if the subfilter list should be OR or AND
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_subfilter(uint32_t dagfd,
                            uint32_t offset,
                            uint32_t mask,
                            uint32_t value,
                            uint32_t operation,
                            uint32_t filter_level,
                            uint32_t level_conf,
                            uint32_t history,
                            uint32_t priority,
			    uint32_t parent_id,
			    list_operations_t list_operation)
{
return filters_msg_set_subfilter(AAL_MSG_SET_SUBFILTER,
					dagfd,
                         offset,
                         mask,
                         value,
                         operation,
                         filter_level,
                         level_conf,
                         history,
                         priority,
			 parent_id,
			 list_operation);


}

/**********************************************************************
* FUNCTION:     hdlc_msg_set_subfilter
* DESCRIPTION:	This sets a subfilter for accepting or rejecting packets
*				based on the values given by the user, at a level below the
*				filter as specifed, with the given operation
* INPUTS:	
*	dagfd - dag card descriptor
*	offset - the offset from the start of the erf that the filter should 
*			 be applied to
*	mask - gives the bitmask for the filter
*	value - gives the match value for after the bitmask has been applied
*	operation - which logical operator to use for filtering
*	filter_level - whether this filter should be applied to all cells 
*				   or cells from a particular line or channel
*	level_conf - channel ID or line ID, or board ID (not available)
*	history - specifics whether this filter should use the last result
*			  as the next match value
*	priority - priority of this filter
*	parent_id - parent filter to base this subfilter from
*	list_operation - specifies if the subfilter list should be OR or AND
* OUTPUTS:	None
* RETURNS:	id of filter created, zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_set_subfilter(uint32_t dagfd,
                            uint32_t offset,
                            uint32_t mask,
                            uint32_t value,
                            uint32_t operation,
                            uint32_t filter_level,
                            uint32_t level_conf,
                            uint32_t history,
                            uint32_t priority,
			    uint32_t parent_id,
			    list_operations_t list_operation)
{

return filters_msg_set_subfilter(HDLC_MSG_SET_SUBFILTER,
					dagfd,
                         offset,
                         mask,
                         value,
                         operation,
                         filter_level,
                         level_conf,
                         history,
                         priority,
			 parent_id,
			 list_operation);


}

/**********************************************************************
* FUNCTION:     filters_msg_set_filter_action
* DESCRIPTION:	This sets whether the filter will cause packets to be 
*				accepted or rejected
* INPUTS:	
*	dagfd - dag card descriptor
*	action - whether matching packets should be accepted or rejected
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t filters_msg_set_filter_action(uint32_t msg_id_o, uint32_t dagfd,
                                   uint32_t action)
{
	uint32_t msg_id;
	uint32_t length;
	t_filters_msg_define_filter_action cmd;
	t_filters_msg_define_filter_action_cmplt reply;

    	msg_id = msg_id_o;
    	length = FILTERS_MSG_DEFINE_FILTER_ACTION_SIZE;

	cmd.action = dagema_htole32(action);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != (msg_id_o+1) || length != FILTERS_MSG_DEFINE_FILTER_ACTION_CMPLT_SIZE)
	{
		return -7;
	}

	return dagema_le32toh(reply.result);
}

/**********************************************************************
* FUNCTION:     aal_msg_set_filter_action
* DESCRIPTION:	This sets whether the filter will cause packets to be 
*				accepted or rejected
* INPUTS:	
*	dagfd - dag card descriptor
*	action - whether matching packets should be accepted or rejected
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_filter_action(uint32_t dagfd,
                                   uint32_t action)
{
	return filters_msg_set_filter_action( AAL_MSG_DEFINE_FILTER_ACTION, dagfd, action);
}

/**********************************************************************
* FUNCTION:     hdlc_msg_set_filter_action
* DESCRIPTION:	This sets whether the filter will cause packets to be 
*				accepted or rejected
* INPUTS:	
*	dagfd - dag card descriptor
*	action - whether matching packets should be accepted or rejected
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_set_filter_action(uint32_t dagfd,
                                   uint32_t action)
{
	return filters_msg_set_filter_action( HDLC_MSG_DEFINE_FILTER_ACTION, dagfd, action);
}

/**********************************************************************
* FUNCTION:     aal_msg_set_filter
* DESCRIPTION:	This sets the filter for accepting or rejecting packets
*				based on the values given by the user.
* INPUTS:	
*	dagfd - dag card descriptor
*	iface - interface where to apply the filter
*	bitmask - gives the bitmask for the filter
*	match - gives the match value for after the bitmask has been applied
*	filter_action - action to apply to matching values
*
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_set_filter_bitmask (uint32_t dagfd,
				     uint32_t iface,
				     uint32_t bitmask,
				     uint32_t match,
				     uint32_t filter_action)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_filter_bitmask cmd;
	t_aal_msg_set_filter_bitmask_cmplt reply;

	msg_id = AAL_MSG_SET_FILTER_BITMASK;
	length = AAL_MSG_SET_FILTER_BITMASK_SIZE;

	cmd.iface   = dagema_htole32(iface);
	cmd.bitmask = dagema_htole32(bitmask);
	cmd.match   = dagema_htole32(match);
	cmd.action  = dagema_htole32(filter_action);
	
	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
	{
		return -6;
	}

	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
	{
		return -3;
	}

	/* confirm the response */
	if (msg_id != AAL_MSG_SET_FILTER_BITMASK_CMPLT || length != AAL_MSG_SET_FILTER_BITMASK_CMPLT_SIZE)
	{
		return -7;
	}

	return dagema_le32toh(reply.result);
}


/**********************************************************************
* FUNCTION:     filters_msg_reset_filter
* DESCRIPTION:	This stops a filter from filtering
* INPUTS:	
	msg_id_o - MESSAGE id to be used for AAL or HDLC
*	dagfd - dag card descriptor
*	filter_id - id of created filter before for remove 
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t filters_msg_reset_filter(uint32_t msg_id_o, uint32_t dagfd,
                              uint32_t filter_id)
{
	uint32_t msg_id;
	uint32_t length;
	t_filters_msg_reset_filter cmd;
	t_filters_msg_reset_filter_cmplt reply;

    	msg_id = msg_id_o;
    	length = FILTERS_MSG_RESET_FILTER_SIZE;

	cmd.filter_id = dagema_htole32(filter_id);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != (msg_id_o +1) || length != FILTERS_MSG_RESET_FILTER_CMPLT_SIZE)
	{
		return -7;
	}

	return dagema_le32toh(reply.result);
}

/**********************************************************************
* FUNCTION:     aal_msg_reset_filter
* DESCRIPTION:	This stops a filter from filtering
* INPUTS:	
*	dagfd - dag card descriptor
*	filter_id - id of created filter before for remove 
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_filter(uint32_t dagfd,
                              uint32_t filter_id)
{
	return filters_msg_reset_filter(AAL_MSG_RESET_FILTER, 
						dagfd,
                              filter_id);
}

/**********************************************************************
* FUNCTION:     hdlc_msg_reset_filter
* DESCRIPTION:	This stops a filter from filtering
* INPUTS:	
*	dagfd - dag card descriptor
*	filter_id - id of created filter before for remove 
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_reset_filter(uint32_t dagfd,
                              uint32_t filter_id)
{
	return filters_msg_reset_filter(HDLC_MSG_RESET_FILTER, 
						dagfd,
                              filter_id);
}



/**********************************************************************
* FUNCTION:     filters_msg_reset_all_filters
* DESCRIPTION:	This stops filtering from occuring
* INPUTS:	
*	dagfd - dag card descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t filters_msg_reset_all_filters(uint32_t msg_id_o, uint32_t dagfd)
{
	int i;
	uint32_t msg_id;
	uint32_t length;
	t_filters_msg_reset_filter cmd;
	t_filters_msg_reset_filter_cmplt reply;

    	msg_id = msg_id_o;
    	length = FILTERS_MSG_RESET_ALL_FILTERS_SIZE;
	cmd.filter_id = 0;

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != (msg_id_o +1 ) || length != FILTERS_MSG_RESET_ALL_FILTERS_CMPLT_SIZE)
	{
		return -7;
	}

	for(i = 0; i<MAX_DAGS; i++)
		filter_id[i] = -1;
	
	return dagema_le32toh(reply.result);
}

/**********************************************************************
* FUNCTION:     aal_msg_reset_all_filters
* DESCRIPTION:	This stops filtering from occuring
* INPUTS:	
*	dagfd - dag card descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_all_filters(uint32_t dagfd)
{
	return filters_msg_reset_all_filters(AAL_MSG_RESET_ALL_FILTERS, dagfd);
}

/**********************************************************************
* FUNCTION:     hdlc_msg_reset_all_filters
* DESCRIPTION:	This stops filtering from occuring
* INPUTS:	
*	dagfd - dag card descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t hdlc_msg_reset_all_filters(uint32_t dagfd)
{
	return filters_msg_reset_all_filters(HDLC_MSG_RESET_ALL_FILTERS, dagfd);
}

/**********************************************************************
* FUNCTION:     aal_msg_reset_statistics
* DESCRIPTION:	This sets a single statistic to zero
* INPUTS:	
*	dagfd - dag card descriptor
*	statistic - identifiers the statistic to be zeroed
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_statistics(uint32_t dagfd,
                                  uint32_t statistic)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_reset_statistics cmd;
    	t_aal_msg_reset_statistics_cmplt reply;

    	msg_id = AAL_MSG_RESET_STATISTICS;
    	length = AAL_MSG_RESET_STATISTICS_SIZE;

	cmd.statistic = dagema_htole32(statistic);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_RESET_STATISTICS_CMPLT || length != AAL_MSG_RESET_STATISTICS_CMPLT_SIZE)
	{
		return -7;
	}
	
	return dagema_le32toh(reply.result);
}

/**********************************************************************
* FUNCTION:     aal_msg_reset_statistics_all
* DESCRIPTION:	This sets all the statistics to zero
* INPUTS:	
*	dagfd - dag card descriptor
*	
* OUTPUTS:	None
* RETURNS:	zero on success, non-zero if unsuccessful
**********************************************************************/
uint32_t aal_msg_reset_statistics_all(uint32_t dagfd)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_reset_statistics_all cmd;
    	t_aal_msg_reset_statistics_all_cmplt reply;

    	msg_id = AAL_MSG_RESET_STATISTICS_ALL;
    	length = AAL_MSG_RESET_STATISTICS_ALL_SIZE;

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
   	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_RESET_STATISTICS_ALL_CMPLT || length != AAL_MSG_RESET_STATISTICS_ALL_CMPLT_SIZE)
	{
		return -7;
	}
	
	return dagema_le32toh(reply.result);
}


/**********************************************************************
* FUNCTION:     aal_msg_init_scanning_mode 
* DESCRIPTION:	init the AAL Reassembler scanning mode
* INPUTS:	
*	dagfd - dag card descriptor
*	mode  - mode to set
* OUTPUTS:	None
* RETURNS:	0 on success, 1 on failure
**********************************************************************/
uint32_t aal_msg_init_scanning_mode(uint32_t dagfd)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_init_scan_mode cmd;
    	t_aal_msg_init_scan_mode_cmplt reply;

    	msg_id = AAL_MSG_INIT_SCAN_MODE;
    	length = AAL_MSG_INIT_SCAN_MODE_SIZE;

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_INIT_SCAN_MODE_CMPLT || length != AAL_MSG_INIT_SCAN_MODE_CMPLT_SIZE)
	{
		return -7;
	}

    return dagema_le32toh(reply.result);

} /* aal_msg_init_scanning_mode */


/**********************************************************************
* FUNCTION:     aal_msg_set_scanning_mode 
* DESCRIPTION:	set the mode of the AAL Reassembler to scanning or reassembly
* INPUTS:	
*	dagfd - dag card descriptor
*	mode  - mode to set
* OUTPUTS:	None
* RETURNS:	0 on success, 1 on failure
**********************************************************************/
uint32_t aal_msg_set_scanning_mode(uint32_t dagfd,
                                   uint32_t mode)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_scanning_mode cmd;
    	t_aal_msg_set_scanning_mode_cmplt reply;

    	msg_id = AAL_MSG_SET_SCANNING_MODE;
    	length = AAL_MSG_SET_SCANNING_MODE_SIZE;
    	cmd.mode = dagema_htole32(mode);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_SET_SCANNING_MODE_CMPLT || length != AAL_MSG_SET_SCANNING_MODE_CMPLT_SIZE)
	{
		return -7;
	}

    	return dagema_le32toh(reply.result);

} /* dag_set_scanning_mode */


/**********************************************************************
* FUNCTION:	    aal_msg_get_scanning_mode 
* DESCRIPTION:	get the mode of the AAL Reassembler 
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:	mode from 3.7t
**********************************************************************/
uint32_t aal_msg_get_scanning_mode(uint32_t dagfd)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_get_scanning_mode cmd;
	t_aal_msg_get_scanning_mode_cmplt reply;

   	msg_id = AAL_MSG_GET_SCANNING_MODE;
    	length = AAL_MSG_GET_SCANNING_MODE_SIZE;
	cmd.unused = 0;

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_GET_SCANNING_MODE_CMPLT || length != AAL_MSG_GET_SCANNING_MODE_CMPLT_SIZE)
	{
		return -7;
	}

    return dagema_le32toh(reply.mode);

} /* dag_get_scanning_mode */


/**********************************************************************
* FUNCTION:	    aal_msg_request_vc_list  
* DESCRIPTION:	Find the virtual connections that scanning has discovered
* INPUTS:	
*	dagfd - dag card descriptor
*	length - maximum length of virtual connection information to receive
* OUTPUTS:	VCList is passed in as a NULL pointer and if successful 
*   returns as an array of VC's. Length can also be made smaller to show
*   the exact number of bytes copied
* RETURNS:	0 if VCList does not equal NULL, otherwise error.
**********************************************************************/
uint32_t aal_msg_request_vc_list(uint32_t  dagfd,
                                 uint32_t* pLength,
                                 uint32_t* pVCList)
{
	uint32_t msg_id;
	uint32_t length;
	uint32_t i =0;
    	t_aal_msg_read_vc_list cmd;
    	t_aal_msg_read_vc_list_cmplt reply;

    
    	msg_id = AAL_MSG_READ_VC_LIST;
    	length = AAL_MSG_READ_VC_LIST_SIZE;
    	cmd.length = dagema_htole32(*pLength);	


	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
       		return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
       		return -3;
    	}

	/* confirm the response */
	if (msg_id != AAL_MSG_READ_VC_LIST_CMPLT )
	{
		return -7;
	}

	
	/* populate VCList structure with VC's */
	*pLength = dagema_le32toh(reply.VCListLength);

	/* VCListLength contains the number of scanned connections seen */
	for(i = 0; i<(*pLength); i++)
	{
		pVCList[i] = dagema_le32toh(reply.VCList[i]);
	}
    
    return 0;
} /* dag_request_vc_list */


/**********************************************************************
* FUNCTION:     aal_msg_get_scanned_connections_number
* DESCRIPTION:	get the number of connections observed in the scan mode
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:	'num of conns' on success, <0 on failure
**********************************************************************/
uint32_t aal_msg_get_scanned_connections_number(uint32_t dagfd)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_get_scanned_connections_number cmd;
    	t_aal_msg_get_scanned_connections_number_cmplt reply;

    	msg_id = AAL_MSG_GET_SCAN_CONN_NUMBER;
    	length = AAL_MSG_GET_SCAN_CONN_NUMBER_SIZE;

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_GET_SCAN_CONN_NUMBER_CMPLT || length != AAL_MSG_GET_SCAN_CONN_NUMBER_CMPLT_SIZE)
	{
		return -7;
	}

    return dagema_le32toh(reply.result);

} /* aal_msg_get_scanned_connections_number */

/**********************************************************************
* FUNCTION:     aal_msg_get_scanned_connection 
* DESCRIPTION:	get a scanned connection
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	
* RETURNS:	
**********************************************************************/
uint32_t aal_msg_get_scanned_connection(uint32_t dagfd,
                                   uint32_t index,
								connection_info_t *pConnection_info)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_get_scanned_connection cmd;
 	t_aal_msg_get_scanned_connection_cmplt reply;

    	msg_id = AAL_MSG_GET_SCAN_CONN;
    	length = AAL_MSG_GET_SCAN_CONN_SIZE;
    	cmd.index = dagema_htole32(index);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_GET_SCAN_CONN_CMPLT || length != AAL_MSG_GET_SCAN_CONN_CMPLT_SIZE)
	{
		return -7;
	}

	pConnection_info->iface   = dagema_le32toh(reply.iface);
	pConnection_info->channel = dagema_le32toh(reply.channel);
	pConnection_info->vpi     = dagema_le32toh(reply.vpi);
	pConnection_info->vci     = dagema_le32toh(reply.vci);

    return dagema_le32toh(reply.result);

} /* aal_msg_get_scanned_connection */



/**********************************************************************
* FUNCTION:	    aal_msg_set_burst_size(dagc, size)
* DESCRIPTION:	Allows the burst size to be altered.  This is the amount
* of data that will be received by the host at once.
* INPUTS:	
*	dagfd - dag card descriptor
*	size - burst size in bytes (72 - 10485760 (10MiB)
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_set_burst_size(uint32_t dagfd,
                                uint32_t size)
{
	uint32_t msg_id;
	uint32_t length;
    	t_aal_msg_set_burst_size cmd;
    	t_aal_msg_set_burst_size_cmplt reply;

	
	/*check new burst size is in range */
	if((size < 72) || (size > (TEN_MIB)))
		return -2;

	/*check new burst size is 64 bit aligned */
	if(size%8 != 0)
		return -4;

    	msg_id   = AAL_MSG_SET_BURST_SIZE;
    	length   = AAL_MSG_SET_BURST_SIZE_SIZE;
	cmd.size = dagema_htole32(size);	

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}

	/* confirm the response */
	if (msg_id != AAL_MSG_SET_BURST_SIZE_CMPLT || length != AAL_MSG_SET_BURST_SIZE_CMPLT_SIZE)
	{
		return -7;
	}

  	return dagema_le32toh(reply.result);
} /* d37t_set_burst_size */





/**********************************************************************
* FUNCTION:	    aal_msg_flush_burst_buffer(dagc)
* DESCRIPTION:	Forces all data ready to be sent to host to be sent
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_flush_burst_buffer(uint32_t dagfd)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_flush_burst_buffer cmd;
    	t_aal_msg_flush_burst_buffer_cmplt reply;

    	msg_id = AAL_MSG_FLUSH_BURST_BUFFER;
   	length = AAL_MSG_FLUSH_BURST_BUFFER_SIZE;
	cmd.unused = 0;
	

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}

	/* confirm the response */
	if (msg_id != AAL_MSG_FLUSH_BURST_BUFFER_CMPLT || length != AAL_MSG_FLUSH_BURST_BUFFER_CMPLT_SIZE)
	{
		return -7;
	}
	
  	return dagema_le32toh(reply.result);
} /* d37t_flush_burst_buffer */

/**********************************************************************
* FUNCTION:	    aal_msg_check_connection_num(connection_num)
* DESCRIPTION:	Checks the connection num is within the valid range (9 bit value)
* INPUTS:	
*	connection_num - input connection_num to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_connection_num(uint32_t connection_num)
{

	if ((connection_num & 0xFFFFFE00) > 0)
		return -1;

	return 0;
}

/**********************************************************************
* FUNCTION:	    aal_msg_check_vpi(VPI)
* DESCRIPTION:	Checks the VPI is within the valid range (12 bit value)
* INPUTS:	
*	VPI - input VPI to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_vpi(uint32_t VPI)
{

	if ((VPI & 0xFFFFF000) > 0)
		return -1;

	return 0;
}

/**********************************************************************
* FUNCTION:	    aal_msg_check_vci(VCI)
* DESCRIPTION:	Checks the VCI is within the valid range (16bit value)
* INPUTS:	
*	VPI - input VPI to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_vci(uint32_t VCI)
{

	if ((VCI & 0xFFFF0000) > 0)
		return -1;

	return 0;
}

/**********************************************************************
* FUNCTION:	    aal_msg_check_interface(Iface)
* DESCRIPTION:	Checks the Interface is within the valid range (2 bit value)
* INPUTS:	
*	Iface - input Iface to check
* OUTPUTS:	None
* RETURNS:	0 if successful, otherwise error.
**********************************************************************/
uint32_t aal_msg_check_interface(uint32_t Iface)
{

	if ((Iface & 0xFFFFFFFC) > 0)
		return -1;

	return 0;
}



/**********************************************************************
* FUNCTION:     aal_msg_set_trailer_strip_mode 
* DESCRIPTION:	set the AAL5 trailer strip mode for the AAL5 reassembler
* INPUTS:	
*	dagfd - dag card descriptor
*	mode  - mode to set
* OUTPUTS:	None
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_set_trailer_strip_mode(uint32_t dagfd,
                                        uint32_t mode)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_trailer_strip_mode cmd;
    	t_aal_msg_set_trailer_strip_mode_cmplt reply;

    	msg_id = AAL_MSG_SET_TRAILER_STRIP_MODE;
    	length = AAL_MSG_SET_TRAILER_STRIP_MODE_SIZE;
    	cmd.mode = dagema_htole32(mode);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
 	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_SET_TRAILER_STRIP_MODE_CMPLT || length != AAL_MSG_SET_TRAILER_STRIP_MODE_CMPLT_SIZE)
	{
		return -7;
	}

    return dagema_le32toh(reply.result);

} /* aal_msg_set_trailer_strip_mode */





/**********************************************************************
* FUNCTION:	    aal_msg_get_trailer_strip_mode 
* DESCRIPTION:	gets the AAL5 trailer strip mode from the AAL5 reassembler
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_get_trailer_strip_mode(uint32_t dagfd)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_get_trailer_strip_mode cmd;
	t_aal_msg_get_trailer_strip_mode_cmplt reply;

    	msg_id = AAL_MSG_GET_TRAILER_STRIP_MODE;
    	length = AAL_MSG_GET_TRAILER_STRIP_MODE_SIZE;
	cmd.unused = 0;

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_GET_TRAILER_STRIP_MODE_CMPLT || length != AAL_MSG_GET_TRAILER_STRIP_MODE_CMPLT_SIZE)
	{
		return -7;
	}

	if ( reply.result != 0 )
		return (uint32_t)trailer_error;
	else
		return dagema_le32toh(reply.mode);

} /* aal_msg_get_trailer_strip_mode */



/**********************************************************************
* FUNCTION:	    aal_msg_set_atm_forwarding_mode 
* DESCRIPTION:	When enabled all unconfigured ATM connections will forward
*               the raw ATM cells onto the FPGA. By default this option
*               is disabled.
* INPUTS:	
*	dagfd - dag card descriptor
*   mode  - the mode to set
* OUTPUTS:	None
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_set_atm_forwarding_mode (uint32_t dagfd,
                                      uint32_t mode)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_set_atm_forwarding_mode cmd;
	t_aal_msg_set_atm_forwarding_mode_cmplt reply;
	
	if ((mode != forwarding_off) && (mode != forwarding_on))
	{
		return -5;
	}

    	msg_id = AAL_MSG_SET_ATM_FORWARDING_MODE;
    	length = AAL_MSG_SET_ATM_FORWARDING_MODE_SIZE;
    	cmd.mode = dagema_htole32(mode);

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_SET_ATM_FORWARDING_MODE_CMPLT || length != AAL_MSG_SET_ATM_FORWARDING_MODE_CMPLT_SIZE)
	{
		return -7;
	}

    	return dagema_le32toh(reply.result);
	
} /* aal_set_atm_forwarding_mode */



/**********************************************************************
* FUNCTION:	    aal_msg_get_atm_forwarding_mode 
* DESCRIPTION:	Gets the current unconfigured ATM forwarding mode
* INPUTS:	
*	dagfd - dag card descriptor
* OUTPUTS:	None
* RETURNS:	result code returned by from the IXP
**********************************************************************/
uint32_t aal_msg_get_atm_forwarding_mode (uint32_t dagfd)
{
	uint32_t msg_id;
	uint32_t length;
	t_aal_msg_get_atm_forwarding_mode cmd;
	t_aal_msg_get_atm_forwarding_mode_cmplt reply;

    	msg_id = AAL_MSG_GET_ATM_FORWARDING_MODE;
    	length = AAL_MSG_GET_ATM_FORWARDING_MODE_SIZE;
	cmd.unused = 0;

	/* send the command */
	if (dagema_send_msg(dagfd, msg_id, length, (uint8_t*)&cmd, NULL) != 0)
    	{
        	return -6;
    	}
    
	/* read the response */
	length = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &length, (uint8_t*)&reply, NULL, DEFAULT_TIMEOUT) != 0) 
    	{
        	return -3;
    	}
    
	/* confirm the response */
	if (msg_id != AAL_MSG_GET_ATM_FORWARDING_MODE_CMPLT || length != AAL_MSG_GET_ATM_FORWARDING_MODE_CMPLT_SIZE)
	{
		return -7;
	}

	if ( reply.result != 0 )
		return (uint32_t)forwarding_error;
	else
		return dagema_le32toh(reply.mode);

} /* aal_msg_get_atm_forwarding_mode */

