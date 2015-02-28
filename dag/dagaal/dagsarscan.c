/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagsarscan.c,v 1.8 2006/09/19 07:14:29 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:		dagsarscan.c
* DESCRIPTION:	Scanning mechanism for reassembling cards
*               Part of the Segmentation and Reassembly library for AAL 
*               protocols on Endace Dag Cards.
*
* HISTORY: 18-8-05 Started for the 3.7t card.
*
**********************************************************************/


#include "dag_platform.h"
#include "dagsarapi.h"
#include "dagsarscan.h"
#include "dagapi.h"
#include "dagutil.h"
#include "aal_msg_types.h"
#include "aal_config_msg.h"

/* declared in dagsarapi.c
 */
#if defined (_WIN32)
extern CRITICAL_SECTION comms_mutex;
#elif defined(_STDC_C99) || defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
extern pthread_mutex_t comms_mutex;
#else
#error dagsarapi.c unsopported platform 
#endif

scan_card_t card_list[MAX_DAGS];

/******** Scanning Thread ***********************************************/

/* thread for scanning, one thread per card */
#if defined(_WIN32)
DWORD WINAPI scanning_thread(LPVOID lpParam)
#elif defined(_STDC_C99) || (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
int scanning_thread(scan_card_t *pDagcard)
#else 
#error DAGSARSCAN.C unsupported platform
#endif

{
	uint32_t total_connections = 0;
#if defined(_WIN32)
    scan_card_t *pDagcard = (scan_card_t *) lpParam;
#endif /* _WIN32 */
	int dagfd = pDagcard->card_fd;

	/* this seems to be quite card dependent */
	switch(pDagcard->dev_type)
	{
	case 0x3707:

		lock_mutex();
		aal_msg_set_scanning_mode(dagfd, scan_on);
		unlock_mutex();

		while(pDagcard->scanning < 2)
		{
			/*give some time for connections to be seen */
			usleep(SCANNING_DELAY);

			total_connections += get_conns_from_37T(dagfd, pDagcard);
		}

		/*scanning mode has been set to off - clean up */
		lock_mutex();
		aal_msg_set_scanning_mode(dagfd, scan_off);
		unlock_mutex();


	break;
	}

	pDagcard->num_connections = total_connections;

	pDagcard->scanning = 4; /*signal end of thread */

	return 0;

}

/******** Set connection found ********************************************/

bool SetNewConnection(scan_card_t* pDagcard, uint32_t connection_num, uint32_t iface, uint32_t vpi, uint32_t vci)
{
	connection_list_t* pThis = NULL;

	/*get space for the new connection, and set it */
	pThis = malloc(sizeof(connection_list_t));

	if(pThis == NULL)
		return false;

	if(connection_num == 0 || vpi == 0 || vci == 0)
	{
		return false;
	}

	pThis->iface   = iface;
	pThis->channel = connection_num;
	pThis->vpi     = vpi;
	pThis->vci     = vci;
	pThis->pNext   = NULL;

	if(pDagcard->pList == NULL)
	{
		pDagcard->pList = pThis;
		pDagcard->pEnd = pDagcard->pList;
	}
	else
	{
		assert(pDagcard->pEnd != NULL);

		/* fit this connection into the list */
		pDagcard->pEnd->pNext = pThis;
		pDagcard->pEnd = pThis;
	}
	return true;

}


/******** 3.7t specific scanning ********************************************/

uint32_t get_conns_from_37T(uint32_t dagfd, scan_card_t *pDagcard)
{
	uint32_t returned_conns = VCSPACE_30;
	int new_connections = 0;
	uint32_t* pVCList = NULL;
	unsigned int i = 0;
			
	pVCList = malloc(VCSPACE_30);
	if(pVCList == NULL)
		return -3;

	lock_mutex();
	if (aal_msg_request_vc_list(dagfd, &returned_conns, pVCList)!= 0)
	{
		unlock_mutex();
		return 0;
	}

	unlock_mutex();

	/*Check returned_conns for errors */
	if(returned_conns < 0 || returned_conns >= VCSPACE_30)
	{
		return 0;
	}

	for(i = 0; i < returned_conns; i+=3)
	{
		if(!SetNewConnection(&card_list[dagfd], pVCList[i], DAG37T_INTERFACE, pVCList[i+1], pVCList[i+2]))
		{
			break;
		}
		new_connections++;
	}

	if(pVCList)
		free(pVCList);

	return new_connections;
}



void lock_mutex()
{
#if defined (_WIN32)
	EnterCriticalSection(&comms_mutex);
#elif defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	pthread_mutex_lock(&comms_mutex);

#endif

}

void unlock_mutex()
{
#if defined (_WIN32)
	LeaveCriticalSection(&comms_mutex);
#elif defined (__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
	pthread_mutex_unlock(&comms_mutex);
#endif

}

/* The dll loader has been added to allow initialisation of the communications
 * mutex in Windows.  There is no equivalent on other platforms at this stage due 
 * to the POSIX mutex object being initialised at declaration.
 */
#if defined(_WIN32)

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reason, 
                       LPVOID lpReserved
					 )
{
    switch (reason)
	{
		case DLL_PROCESS_ATTACH:

			// We use a critical section to protect the communications with the card
			InitializeCriticalSection(&comms_mutex);

			break;

		case DLL_PROCESS_DETACH:
			DeleteCriticalSection(&comms_mutex);
			break;
    }
    return TRUE;
}

#endif
