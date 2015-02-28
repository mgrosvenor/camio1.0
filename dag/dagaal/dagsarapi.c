/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagsarapi.c,v 1.25 2009/04/14 02:26:42 wilson.zhu Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:		dagsarapi.c
* DESCRIPTION:	Segmentation and Reassembly library for AAL protocols 
*				on Endace Dag Cards.
*
* HISTORY: 13-6-05 Started for the 3.7t card.
*
**********************************************************************/


#include "dagapi.h"
#include "dag_platform.h"
#include "dagsarapi.h"
#include "dagsarscan.h"
#include "dagutil.h"
#include "aal_msg_types.h"
#include "aal_config_msg.h"
#include <stdbool.h>

int filter_id[MAX_DAGS];          /* holds the filter id when used on the 3.7T*/
extern scan_card_t card_list[MAX_DAGS];


/* declare the required mutex objects to prevent the different threads 
 * from attempting to access the communications with the card at the same
 * time.  This could otherwise cause two threads to attempt to write to 
 * the card at the same time, giving an incorrect message to the card
 */
#if defined (_WIN32)
CRITICAL_SECTION comms_mutex;
#elif defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
pthread_mutex_t comms_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif





/*****Activation of VC ***********************************************/

uint32_t dagsar_vci_activate(uint32_t dagfd, 
							 uint32_t iface,
							 uint32_t channel, 
							 uint32_t vpi, 
							 uint32_t vci)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();

		result = aal_msg_activate_vc(dagfd, 0, vci, vpi, channel);
		
		unlock_mutex();		
			
		return result; 
		break;

	case 0x7100:
		lock_mutex();

		result = aal_msg_activate_vc(dagfd, iface, vci, vpi, channel);
		
		unlock_mutex();		
			
		return result; 
		break;

	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	

	return 1;
}
/*****Deactivation of VC ***********************************************/

uint32_t dagsar_vci_deactivate(uint32_t dagfd, 
							   uint32_t iface,
                               uint32_t channel,
							   uint32_t vpi,
							   uint32_t vci)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();

		result = aal_msg_remove_vc(dagfd, 0, vci, vpi, channel);

		unlock_mutex();

		return result;
		break;
	
	case 0x7100:
		lock_mutex();

		result = aal_msg_remove_vc(dagfd, iface, vci, vpi, channel);

		unlock_mutex();

		return result;
		break;	

	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	
	return 1;

}

/*****Activation of CIDs *********************************************/

uint32_t dagsar_cid_activate(uint32_t dagfd, 
							 uint32_t iface,
							 uint32_t channel, 
							 uint32_t vpi, 
							 uint32_t vci,
							 uint32_t cid)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
	case 0x7100:
		lock_mutex();

		result = aal_msg_activate_cid(dagfd, iface, vci, vpi, channel, cid);
		
		unlock_mutex();		
			
		return result; 
		break;
	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	

	return 1;
}
/*****Deactivation of CID *********************************************/

uint32_t dagsar_cid_deactivate(uint32_t dagfd, 
							   uint32_t iface,
                               uint32_t channel,
							   uint32_t vpi,
							   uint32_t vci,
							   uint32_t cid)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
	case 0x7100:
		lock_mutex();

		result = aal_msg_deactivate_cid(dagfd, iface, vci, vpi, channel, cid);

		unlock_mutex();

		return result;
		break;
	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	
	return 1;

}


/***** Setting SAR Mode  ***********************************************/

uint32_t dagsar_vci_set_sar_mode(uint32_t dagfd, 
								 uint32_t iface,
								 uint32_t channel, 
								 uint32_t vpi,
								 uint32_t vci,
								 sar_mode_t sar_mode)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();

		result = aal_msg_set_sar_mode (dagfd, 0, vci, vpi, channel, sar_mode);

		unlock_mutex();

		return result;
		break;

	case 0x7100:
		lock_mutex();

		result = aal_msg_set_sar_mode (dagfd, iface, vci, vpi, channel, sar_mode);

		unlock_mutex();

		return result;
		break;

	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	
	return 1;
	

}

/***** Get SAR Mode  ***************************************************/

sar_mode_t dagsar_vci_get_sar_mode(uint32_t dagfd, 
								   uint32_t iface,
								   uint32_t channel, 
								   uint32_t vpi,
								   uint32_t vci)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();

		result = aal_msg_get_sar_mode(dagfd, 0, channel, vpi, vci);

		unlock_mutex();

		return result ;
		break;

	case 0x7100:
		lock_mutex();

		result = aal_msg_get_sar_mode(dagfd, iface, channel, vpi, vci);

		unlock_mutex();

		return result ;
		break;

	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	
	return 1;

}

/***** Setting Net Mode  ***********************************************/

uint32_t dagsar_channel_set_net_mode(uint32_t dagfd, 
									 uint32_t iface,
										uint32_t channel, 
										net_mode_t net_mode)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
	case 0x7100:
		lock_mutex();

		result = aal_msg_set_connection_net_mode(dagfd, net_mode, iface, channel); 
		
		unlock_mutex();

		return result;
		break;

	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	
	return 1;

}

/***** Setting Net Mode per interface ****************************************/

uint32_t dagsar_iface_set_net_mode (uint32_t dagfd,
									uint32_t iface,
									net_mode_t net_mode)
{
	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x7100:
		lock_mutex();

		result = aal_msg_set_iface_net_mode(dagfd, net_mode, iface); 
		
		unlock_mutex();

		return result;
		break;

	default:
		dagutil_panic("Device could not be configured\n");
		break;
	}
	return 1;
}

/***** Set AAL5 Buffer size ********************************************/

uint32_t dagsar_set_buffer_size(uint32_t dagfd, uint32_t size)
{

 	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();

		result = aal_msg_set_buffer_size(dagfd, size);

		unlock_mutex();

		return  result;
		break;
	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	
	return 1;
}

/***** Get Statistics ***********************************************/

uint32_t dagsar_get_stats(uint32_t dagfd, stats_t statistic)
{

 	daginf_t* daginf;
	int result = 0;
	
	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
	case 0x7100:
		lock_mutex();

		result = aal_msg_report_statistics(dagfd, statistic);
		
		unlock_mutex();

		return result;
		break;
	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;
}

/***** Get Statistics for Interface ****************************************/

uint32_t dagsar_get_interface_stats(uint32_t dagfd, uint32_t iface, stats_t statistic)
{

 	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();
		
		/*there is only one interface on the 3.7T so this is the same as above */
		result = aal_msg_report_statistics(dagfd, statistic);

		unlock_mutex();

		return result;
		break;
	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;
}

/***** Reset Statistics ***********************************************/

uint32_t dagsar_reset_stats(uint32_t dagfd, stats_t statistic)
{

 	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
	case 0x7100:
		lock_mutex();

		result = aal_msg_reset_statistics(dagfd, statistic);

		unlock_mutex();

		return result; 
		break;
	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;
}

/***** Reset All Statistics ***********************************************/

uint32_t dagsar_reset_stats_all(uint32_t dagfd)
{

	daginf_t* daginf;
	int result = 0;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();

		result += aal_msg_reset_statistics(dagfd, dropped_cells);
		result += aal_msg_reset_statistics(dagfd, filtered_cells);
		
		unlock_mutex();

		return result;
		break;

	case 0x7100:
		lock_mutex();

		result = aal_msg_reset_statistics_all(dagfd);

		unlock_mutex();

		return result;
		break;

	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;
}

/***** Set Filter ****************************************************/

uint32_t  dagsar_set_filter_bitmask (uint32_t dagfd, 
									 uint32_t iface, 
									 uint32_t bitmask, 
									 uint32_t match, 
									 filter_action_t filter_action)
{

 	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);
	

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();

		if(filter_id[dagfd] >= 0)
		{
			aal_msg_reset_filter(dagfd, filter_id[dagfd]);
		}
		aal_msg_set_filter_action(dagfd, filter_action);

		filter_id[dagfd] = aal_msg_set_filter(dagfd, 20, bitmask, match, DAG_EQ,
			DAG_FILTER_LEVEL_BOARD, 0, false, 0);
		
		unlock_mutex();
		
		if(filter_id[dagfd] == 0)
			return  -1;
		else if(filter_id[dagfd] == -3)
			return -3;
		else
			return 0;
		break;


	case 0x7100:
		lock_mutex();

		result = aal_msg_set_filter_bitmask (dagfd, iface, bitmask, match, filter_action);

		unlock_mutex();		

		return result;

		break;

	default:
		dagutil_panic("Device could not be configured\n");
		break;

	}	
	return 1;
}

/***** Reset Filter ****************************************************/

uint32_t dagsar_reset_filter_bitmask (uint32_t dagfd, uint32_t iface)
{
 	daginf_t* daginf;
	int result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		/*check that there is a known filter initialised on this card */
		if(filter_id[dagfd] <= 0)
			return -1;

		filter_id[dagfd] = -1;

		lock_mutex();
		result = aal_msg_reset_filter(dagfd, filter_id[dagfd]); 
		unlock_mutex();

		return result; 
		break;

	case 0x7100:
		lock_mutex();
		
		/* accept everything */
		result = aal_msg_set_filter_bitmask (dagfd, iface, 0x00000000, 0x00000000, sar_accept);

		unlock_mutex();

		return result;
		break;

	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;
}


/***** Initialise Scanning *************************************************/

uint32_t dagsar_init_scanning_mode_37t (uint32_t dagfd)
{
 	daginf_t* daginf;
	connection_list_t *pList, *pNext; /* used for connection list deletion */

	/* if thread already exists (ie this card is already scanning)
	 * do not re initalise scanning - return error
	 */

	if(card_list[dagfd].scanning != 0)
		return -1;

	/* initialise the card_list structure */
	daginf = dag_info(dagfd);

	card_list[dagfd].card_fd = dagfd;
	card_list[dagfd].dev_type = daginf->device_code;

	/* set number of connections for this card to zero*/
	card_list[dagfd].num_connections = 0;

	/* delete any existing vc's for this card*/
	if(card_list[dagfd].pList != NULL)
	{
		pList = card_list[dagfd].pList;
		pNext = card_list[dagfd].pList->pNext;

		while(pList != card_list[dagfd].pEnd)
		{
			free(pList);
			pList = pNext;
			pNext = pNext->pNext;

		}
		free(pList);
		card_list[dagfd].pList = NULL;
		card_list[dagfd].pEnd = NULL;

	}

	return 0;
}

/*****************************************************************************/
uint32_t dagsar_init_scanning_mode (uint32_t dagfd)
{
	daginf_t* daginf;
	uint32_t result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
		result = dagsar_init_scanning_mode_37t (dagfd);
		return result;

		break;

	case 0x7100:
		lock_mutex();
		result = aal_msg_init_scanning_mode (dagfd);
		unlock_mutex();

		return result;
		break;

	default:
		dagutil_panic("Device could not be found\n");
		break;
	}

	return 1;
}

/***** Set Scanning Mode ***********************************************/

/*****************************************************************************/
uint32_t dagsar_set_scanning_mode_37t (uint32_t dagfd, scanning_mode_t scan_mode)
{
	int count = 10000;
#if defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	static pthread_t scan_thread;
#endif 

 	if(scan_mode == scan_on)
	{

		/*allocate memory for the connection list start */


		/*check the card has been initialised at least once*/
		if(card_list[dagfd].dev_type == 0)
			return -4;


		/*create the thread that does the scanning */
#if defined (_WIN32)
		if(CreateThread(NULL, 0, scanning_thread, (LPVOID) &card_list[dagfd], 0, NULL) == NULL)
		{
			return -5;
		}
#elif defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
        if(pthread_create(&scan_thread, NULL, (void *) &scanning_thread, (void *) &card_list[dagfd])!= 0)
			return -5;
#endif

		/*remember that scanning has started */
		card_list[dagfd].scanning = 1;
	}
	else if(scan_mode == scan_off) 
	{
		card_list[dagfd].scanning = 3;
		while(card_list[dagfd].scanning != 4 && count > 0)
		{
			usleep(SCANNING_DELAY);
			count--;
		}
		if(count == 0)
			return -2;

		card_list[dagfd].scanning = 0;

	}
	else
	{

		/*there has been an error */
		return -1;
	}

	return 0;
}

/*****************************************************************************/
uint32_t dagsar_set_scanning_mode (uint32_t dagfd, scanning_mode_t scan_mode)
{
	daginf_t* daginf;
	uint32_t result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
		result = dagsar_set_scanning_mode_37t (dagfd, scan_mode);
		return result;

		break;

	case 0x7100:
		lock_mutex();
		result = aal_msg_set_scanning_mode (dagfd, scan_mode);
		unlock_mutex();

		return result;
		break;

	default:
		dagutil_panic("Device could not be found\n");
		break;
	}

	return 1;
}

/***** Get Scanning Mode *************************************************/

scanning_mode_t dagsar_get_scanning_mode (uint32_t dagfd)
{
 	daginf_t* daginf;
	scanning_mode_t mode;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:

		lock_mutex();
		mode = (scanning_mode_t) aal_msg_get_scanning_mode(dagfd); 
		unlock_mutex();

		return mode; 
		break;

	case 0x7100:
		lock_mutex();
		mode = (scanning_mode_t) aal_msg_get_scanning_mode(dagfd); 
		unlock_mutex();

		return mode; 
		break;

	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;
}

/***** Get Scanned Connection number ***********************************/

uint32_t dagsar_get_scanned_connections_number (uint32_t dagfd)
{
 	daginf_t* daginf;
	uint32_t num_scanned_connections;

	daginf = dag_info(dagfd);
	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
		/* check this card is not currently scanning */
		if(card_list[dagfd].scanning != 0)
			return -1;
		else
		{
			return card_list[dagfd].num_connections;
		}

		break;

	case 0x7100:
		lock_mutex();
		num_scanned_connections = aal_msg_get_scanned_connections_number(dagfd);
		unlock_mutex();
		
		return num_scanned_connections;
		break;

	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;
}


/***** Get Scanned Connections ***********************************/

uint32_t dagsar_get_scanned_connection_37t (uint32_t dagfd, 
									   uint32_t connection_number, 
									   connection_info_t *pConnection_info)
{

	connection_list_t *pList; /* used for connection list deletion */
	int i = 0;

	/* if thread already exists (ie this card is already scanning)
	 * return error
	 */
	if(card_list[dagfd].scanning != 0)
		return -1;

	/* if the requested number is greater than the number available give error */
	if(card_list[dagfd].num_connections <= connection_number)
		return -2;


	/* find the information at the requested connection number */
	pList = card_list[dagfd].pList;

	while(i < (signed)connection_number)
	{
		pList = pList->pNext;
	
		if(NULL == pList)
		{
			/* there is an error, we cannot go further*/
			return -3;
		}
		i++;
	}

	/* copy information to the user supplied structure */
	pConnection_info->channel = pList->channel;
	pConnection_info->iface   = pList->iface;
	pConnection_info->vpi     = pList->vpi;
	pConnection_info->vci     = pList->vci;

	return 0;
}


/*****************************************************************************/
uint32_t dagsar_get_scanned_connection(uint32_t dagfd, 
									   uint32_t connection_number, 
									   connection_info_t *pConnection_info)
{
 	daginf_t* daginf;
	uint32_t result;

	daginf = dag_info(dagfd);
	switch(daginf->device_code)
	{
	case 0x3707:
	case 0x3747:
		return dagsar_get_scanned_connection_37t(dagfd, connection_number, pConnection_info);
		break;

	case 0x7100:
		if (pConnection_info == NULL)
			return -4;

		lock_mutex();
		result = aal_msg_get_scanned_connection(dagfd, connection_number, pConnection_info);
		unlock_mutex();
		
		return result;
		break;

	default:
		dagutil_panic("Device could not be found\n");
		break;

	}	
	return 1;	

}


/*****************************************************************************/
uint32_t dagsar_set_trailer_strip_mode (uint32_t dagfd, trailer_strip_mode_t strip_mode)
{
	daginf_t* daginf;
	uint32_t result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x7100:
		lock_mutex();
		result = aal_msg_set_trailer_strip_mode(dagfd, strip_mode);
		unlock_mutex();

		return result;

	default:
		return 1;
	}
}

/***** Get AAL5 Trailer Strip Mode *************************************************/

trailer_strip_mode_t dagsar_get_trailer_strip_mode (uint32_t dagfd)
{
 	daginf_t* daginf;
	trailer_strip_mode_t mode;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x7100:
		lock_mutex();
		mode = (trailer_strip_mode_t) aal_msg_get_trailer_strip_mode(dagfd); 
		unlock_mutex();

		return mode; 

	default:
		return trailer_error;

	}	
}


/*****************************************************************************/
uint32_t dagsar_set_atm_forwarding_mode (uint32_t dagfd, atm_forwarding_mode_t forwarding_mode)
{
	daginf_t* daginf;
	uint32_t result;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x7100:
		lock_mutex();
		result = aal_msg_set_atm_forwarding_mode(dagfd, forwarding_mode);
		unlock_mutex();

		return result;

	default:
		return 1;
	}
}

/***** Get Unconfigured Connection Forwarding Mode *************************************************/

atm_forwarding_mode_t dagsar_get_atm_forwarding_mode (uint32_t dagfd)
{
 	daginf_t* daginf;
	atm_forwarding_mode_t mode;

	daginf = dag_info(dagfd);

	switch(daginf->device_code)
	{
	case 0x7100:
		lock_mutex();
		mode = (atm_forwarding_mode_t) aal_msg_get_atm_forwarding_mode(dagfd); 
		unlock_mutex();

		return mode; 

	default:
		return forwarding_error;

	}	
}


