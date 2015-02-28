/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagsarscan.h,v 1.4 2006/06/26 00:34:40 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:		dagsarscan.h
* DESCRIPTION:	Scanning mechanism for reassembling cards
*               Part of the Segmentation and Reassembly library for AAL 
*               protocols on Endace Dag Cards.
*
* HISTORY: 18-8-05 Started for the 3.7t card.
*
**********************************************************************/

#ifndef DAGSARSCAN_H
#define DAGSARSCAN_H

#include "dag_platform.h"

#define SCANNING_DELAY 1000


typedef struct
{
  uint32_t iface;
  uint32_t channel;
  uint32_t vpi;
  uint32_t vci;

  void *pNext;
}connection_list_t;

typedef struct
{
	uint32_t card_fd;          /* file descriptor for this card */
	uint32_t dev_type;         /* device code (3.7t, 7.1s etc) */
	uint32_t num_connections;  /* number connections available */
	uint32_t scanning;         /* is device scanning? 0-no,1-yes,2-finishing*/
	connection_list_t* pList;  /* start of the connection list */
	connection_list_t* pEnd;   /* end of the connection list */
}scan_card_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void lock_mutex();
void unlock_mutex();
uint32_t get_conns_from_37T(uint32_t dagfd, scan_card_t *pDagcard);


#if defined(_WIN32)
DWORD WINAPI scanning_thread(LPVOID lpParam);
#elif defined(__linux__) || defined(__FreeBSD__) || (defined(__APPLE__) && defined(__ppc__))
int scanning_thread(scan_card_t *pDagcard);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _DAGSARAPI_H_ */
