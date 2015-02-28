/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: aal_msg_types.h,v 1.12 2009/01/30 01:17:45 wilson.zhu Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         aal_msg_types.h
*
* DESCRIPTION:  Definitions of the messages used to configure the AAL
*               reassembly on the dag3.7t and dag7.1s cards.
*
* HISTORY:      
*
**********************************************************************/

#ifndef AAL_MSG_TYPES_H
#define AAL_MSG_TYPES_H

/*  */
#include "dagema.h"



/*Scanning Maximums */
#define VCS_30      30
#define VCSPACE_30  90




/*   */
#define TEN_MIB            10485760


/* AAL5 Messages */
#define AAL_MSG_SET_SCANNING_MODE              (EMA_CLASS_AAL | 0x0001)
#define AAL_MSG_SET_SCANNING_MODE_CMPLT        (EMA_CLASS_AAL | 0x0002)
#define AAL_MSG_ACTIVATE_VC                    (EMA_CLASS_AAL | 0x0003)
#define AAL_MSG_ACTIVATE_VC_CMPLT              (EMA_CLASS_AAL | 0x0004)
#define AAL_MSG_REMOVE_VC                      (EMA_CLASS_AAL | 0x0005)
#define AAL_MSG_REMOVE_VC_CMPLT                (EMA_CLASS_AAL | 0x0006)
#define AAL_MSG_SET_SAR_MODE                   (EMA_CLASS_AAL | 0x0007)
#define AAL_MSG_SET_SAR_MODE_CMPLT             (EMA_CLASS_AAL | 0x0008)
#define AAL_MSG_GET_SAR_MODE                   (EMA_CLASS_AAL | 0x0009)
#define AAL_MSG_GET_SAR_MODE_CMPLT             (EMA_CLASS_AAL | 0x000a)
#define AAL_MSG_SET_NET_MODE                   (EMA_CLASS_AAL | 0x000b)
#define AAL_MSG_SET_NET_MODE_CMPLT             (EMA_CLASS_AAL | 0x000c)
#define AAL_MSG_READ_VC_LIST                   (EMA_CLASS_AAL | 0x000d)
#define AAL_MSG_READ_VC_LIST_CMPLT             (EMA_CLASS_AAL | 0x000e)
#define AAL_MSG_SET_BUFFER_SIZE                (EMA_CLASS_AAL | 0x000f)
#define AAL_MSG_SET_BUFFER_SIZE_CMPLT          (EMA_CLASS_AAL | 0x0010)
#define AAL_MSG_READ_STATISTIC                 (EMA_CLASS_AAL | 0x0011)
#define AAL_MSG_READ_STATISTIC_CMPLT           (EMA_CLASS_AAL | 0x0012)
#define AAL_MSG_RESET_STATISTICS               (EMA_CLASS_AAL | 0x0013)
#define AAL_MSG_RESET_STATISTICS_CMPLT         (EMA_CLASS_AAL | 0x0014)

//the enumeraions left intentionally the same as old for back words compability 
#define AAL_MSG_SET_FILTER                     (EMA_CLASS_ATM_FILTER | 0x0015)
#define AAL_MSG_SET_FILTER_CMPLT               (EMA_CLASS_ATM_FILTER | 0x0016)
#define AAL_MSG_RESET_FILTER                   (EMA_CLASS_ATM_FILTER | 0x0017)
#define AAL_MSG_RESET_FILTER_CMPLT             (EMA_CLASS_ATM_FILTER | 0x0018)
#define AAL_MSG_RESET_ALL_FILTERS              (EMA_CLASS_ATM_FILTER | 0x0019)
#define AAL_MSG_RESET_ALL_FILTERS_CMPLT        (EMA_CLASS_ATM_FILTER | 0x0020)
#define AAL_MSG_DEFINE_FILTER_ACTION           (EMA_CLASS_ATM_FILTER | 0x0021)
#define AAL_MSG_DEFINE_FILTER_ACTION_CMPLT     (EMA_CLASS_ATM_FILTER | 0x0022)
#define AAL_MSG_SET_SUBFILTER                  (EMA_CLASS_ATM_FILTER | 0x0033)
#define AAL_MSG_SET_SUBFILTER_CMPLT            (EMA_CLASS_ATM_FILTER | 0x0034)

//the enumeraions left intentionally the same as old for back words compability 
#define HDLC_MSG_SET_FILTER                     (EMA_CLASS_HDLC_FILTER | 0x0015)
#define HDLC_MSG_SET_FILTER_CMPLT               (EMA_CLASS_HDLC_FILTER | 0x0016)
#define HDLC_MSG_RESET_FILTER                   (EMA_CLASS_HDLC_FILTER | 0x0017)
#define HDCL_MSG_RESET_FILTER_CMPLT             (EMA_CLASS_HDLC_FILTER | 0x0018)
#define HDLC_MSG_RESET_ALL_FILTERS              (EMA_CLASS_HDLC_FILTER | 0x0019)
#define HDLC_MSG_RESET_ALL_FILTERS_CMPLT        (EMA_CLASS_HDLC_FILTER | 0x0020)
#define HDLC_MSG_DEFINE_FILTER_ACTION           (EMA_CLASS_HDLC_FILTER | 0x0021)
#define HDLC_MSG_DEFINE_FILTER_ACTION_CMPLT     (EMA_CLASS_HDLC_FILTER | 0x0022)
#define HDLC_MSG_SET_SUBFILTER                  (EMA_CLASS_HDLC_FILTER | 0x0033)
#define HDLC_MSG_SET_SUBFILTER_CMPLT            (EMA_CLASS_HDLC_FILTER | 0x0034)



#define AAL_MSG_GET_SCANNING_MODE              (EMA_CLASS_AAL | 0x0023)
#define AAL_MSG_GET_SCANNING_MODE_CMPLT        (EMA_CLASS_AAL | 0x0024)
#define AAL_MSG_SET_BURST_SIZE                 (EMA_CLASS_AAL | 0x0025)
#define AAL_MSG_SET_BURST_SIZE_CMPLT           (EMA_CLASS_AAL | 0x0026)
#define AAL_MSG_FLUSH_BURST_BUFFER             (EMA_CLASS_AAL | 0x0027)
#define AAL_MSG_FLUSH_BURST_BUFFER_CMPLT       (EMA_CLASS_AAL | 0x0028)
#define AAL_MSG_ACTIVATE_CID                   (EMA_CLASS_AAL | 0x0029)
#define AAL_MSG_ACTIVATE_CID_CMPLT             (EMA_CLASS_AAL | 0x0030)
#define AAL_MSG_DEACTIVATE_CID                 (EMA_CLASS_AAL | 0x0031)
#define AAL_MSG_DEACTIVATE_CID_CMPLT           (EMA_CLASS_AAL | 0x0032)

#define AAL_MSG_SET_IFACE_NET_MODE             (EMA_CLASS_AAL | 0x0035)
#define AAL_MSG_SET_IFACE_NET_MODE_CMPLT       (EMA_CLASS_AAL | 0x0036)
#define AAL_MSG_SET_FILTER_BITMASK             (EMA_CLASS_AAL | 0x0037)
#define AAL_MSG_SET_FILTER_BITMASK_CMPLT       (EMA_CLASS_AAL | 0x0038)
#define AAL_MSG_INIT_SCAN_MODE                 (EMA_CLASS_AAL | 0x0039)
#define AAL_MSG_INIT_SCAN_MODE_CMPLT           (EMA_CLASS_AAL | 0x003A)
#define AAL_MSG_GET_SCAN_CONN_NUMBER           (EMA_CLASS_AAL | 0x003B)
#define AAL_MSG_GET_SCAN_CONN_NUMBER_CMPLT     (EMA_CLASS_AAL | 0x003C)
#define AAL_MSG_GET_SCAN_CONN                  (EMA_CLASS_AAL | 0x003D)
#define AAL_MSG_GET_SCAN_CONN_CMPLT            (EMA_CLASS_AAL | 0x003E)
#define AAL_MSG_RESET_STATISTICS_ALL           (EMA_CLASS_AAL | 0x003F)
#define AAL_MSG_RESET_STATISTICS_ALL_CMPLT     (EMA_CLASS_AAL | 0x0040)
#define AAL_MSG_GET_TRAILER_STRIP_MODE         (EMA_CLASS_AAL | 0x0041)
#define AAL_MSG_GET_TRAILER_STRIP_MODE_CMPLT   (EMA_CLASS_AAL | 0x0042)
#define AAL_MSG_SET_TRAILER_STRIP_MODE         (EMA_CLASS_AAL | 0x0043)
#define AAL_MSG_SET_TRAILER_STRIP_MODE_CMPLT   (EMA_CLASS_AAL | 0x0044)
#define AAL_MSG_GET_ATM_FORWARDING_MODE        (EMA_CLASS_AAL | 0x0045)
#define AAL_MSG_GET_ATM_FORWARDING_MODE_CMPLT  (EMA_CLASS_AAL | 0x0046)
#define AAL_MSG_SET_ATM_FORWARDING_MODE        (EMA_CLASS_AAL | 0x0047)
#define AAL_MSG_SET_ATM_FORWARDING_MODE_CMPLT  (EMA_CLASS_AAL | 0x0048)

#define AAL_MSG_SET_SCANNING_MODE_SIZE	        sizeof(t_aal_msg_set_scanning_mode)
#define AAL_MSG_SET_SCANNING_MODE_CMPLT_SIZE    sizeof(t_aal_msg_set_scanning_mode_cmplt)
#define AAL_MSG_ACTIVATE_VC_SIZE                sizeof(t_aal_msg_activate_vc)
#define AAL_MSG_ACTIVATE_VC_CMPLT_SIZE  	sizeof(t_aal_msg_activate_vc_cmplt)
#define AAL_MSG_REMOVE_VC_SIZE                  sizeof(t_aal_msg_remove_vc)
#define AAL_MSG_REMOVE_VC_CMPLT_SIZE            sizeof(t_aal_msg_remove_vc_cmplt)
#define AAL_MSG_SET_SAR_MODE_SIZE               sizeof(t_aal_msg_set_sar_mode)			
#define AAL_MSG_SET_SAR_MODE_CMPLT_SIZE         sizeof(t_aal_msg_set_sar_mode_cmplt)			
#define AAL_MSG_GET_SAR_MODE_SIZE               sizeof(t_aal_msg_get_sar_mode)			
#define AAL_MSG_GET_SAR_MODE_CMPLT_SIZE         sizeof(t_aal_msg_get_sar_mode_cmplt)			
#define AAL_MSG_SET_NET_MODE_SIZE               sizeof(t_aal_msg_set_net_mode)			
#define AAL_MSG_SET_NET_MODE_CMPLT_SIZE         sizeof(t_aal_msg_set_net_mode_cmplt)			
#define AAL_MSG_SET_BUFFER_SIZE_SIZE            sizeof(t_aal_msg_set_buffer_size)
#define AAL_MSG_SET_BUFFER_SIZE_CMPLT_SIZE      sizeof(t_aal_msg_set_buffer_size_cmplt)
#define AAL_MSG_READ_VC_LIST_SIZE               sizeof(t_aal_msg_read_vc_list)
#define AAL_MSG_READ_VC_LIST_CMPLT_SIZE         sizeof(t_aal_msg_read_vc_list_cmplt)
#define AAL_MSG_READ_STATISTIC_SIZE             sizeof(t_aal_msg_read_statistic)
#define AAL_MSG_READ_STATISTIC_CMPLT_SIZE       sizeof(t_aal_msg_read_statistic_cmplt)

//filtering structure sizes
#define FILTERS_MSG_SET_FILTER_SIZE                 sizeof(t_filters_msg_set_filter)
#define FILTERS_MSG_SET_FILTER_CMPLT_SIZE           sizeof(t_filters_msg_set_filter_cmplt)
#define FILTERS_MSG_RESET_FILTER_SIZE               sizeof(t_filters_msg_reset_filter)
#define FILTERS_MSG_RESET_FILTER_CMPLT_SIZE         sizeof(t_filters_msg_reset_filter_cmplt)
#define FILTERS_MSG_RESET_ALL_FILTERS_SIZE          sizeof(t_filters_msg_reset_all_filters)
#define FILTERS_MSG_RESET_ALL_FILTERS_CMPLT_SIZE    sizeof(t_filters_msg_reset_all_filters_cmplt)
#define FILTERS_MSG_DEFINE_FILTER_ACTION_SIZE       sizeof(t_filters_msg_define_filter_action)
#define FILTERS_MSG_DEFINE_FILTER_ACTION_CMPLT_SIZE sizeof(t_filters_msg_define_filter_action_cmplt)
#define FILTERS_MSG_SET_SUBFILTER_SIZE              sizeof(t_filters_msg_set_subfilter)
#define FILTERS_MSG_SET_SUBFILTER_CMPLT_SIZE        sizeof(t_filters_msg_set_subfilter_cmplt)


#define AAL_MSG_RESET_STATISTICS_SIZE           sizeof(t_aal_msg_reset_statistics)
#define AAL_MSG_RESET_STATISTICS_CMPLT_SIZE     sizeof(t_aal_msg_reset_statistics_cmplt)
#define AAL_MSG_GET_SCANNING_MODE_SIZE          sizeof(t_aal_msg_get_scanning_mode)
#define AAL_MSG_GET_SCANNING_MODE_CMPLT_SIZE    sizeof(t_aal_msg_get_scanning_mode_cmplt)
#define AAL_MSG_SET_BURST_SIZE_SIZE             sizeof(t_aal_msg_set_burst_size)
#define AAL_MSG_SET_BURST_SIZE_CMPLT_SIZE       sizeof(t_aal_msg_set_burst_size_cmplt)
#define AAL_MSG_FLUSH_BURST_BUFFER_SIZE         sizeof(t_aal_msg_flush_burst_buffer)
#define AAL_MSG_FLUSH_BURST_BUFFER_CMPLT_SIZE   sizeof(t_aal_msg_flush_burst_buffer_cmplt)
#define AAL_MSG_ACTIVATE_CID_SIZE               sizeof(t_aal_msg_activate_cid)
#define AAL_MSG_ACTIVATE_CID_CMPLT_SIZE         sizeof(t_aal_msg_activate_cid_cmplt)
#define AAL_MSG_DEACTIVATE_CID_SIZE             sizeof(t_aal_msg_deactivate_cid)
#define AAL_MSG_DEACTIVATE_CID_CMPLT_SIZE       sizeof(t_aal_msg_deactivate_cid_cmplt)
#define AAL_MSG_SET_IFACE_NET_MODE_SIZE         sizeof(t_aal_msg_set_iface_net_mode)			
#define AAL_MSG_SET_IFACE_NET_MODE_CMPLT_SIZE   sizeof(t_aal_msg_set_iface_net_mode_cmplt)			
#define AAL_MSG_SET_FILTER_BITMASK_SIZE         sizeof(t_aal_msg_set_filter_bitmask)
#define AAL_MSG_SET_FILTER_BITMASK_CMPLT_SIZE   sizeof(t_aal_msg_set_filter_bitmask_cmplt)
#define AAL_MSG_INIT_SCAN_MODE_SIZE             sizeof(t_aal_msg_init_scan_mode)
#define AAL_MSG_INIT_SCAN_MODE_CMPLT_SIZE       sizeof(t_aal_msg_init_scan_mode_cmplt)
#define AAL_MSG_GET_SCAN_CONN_NUMBER_SIZE       sizeof(t_aal_msg_get_scanned_connections_number)
#define AAL_MSG_GET_SCAN_CONN_NUMBER_CMPLT_SIZE sizeof(t_aal_msg_get_scanned_connections_number_cmplt)
#define AAL_MSG_GET_SCAN_CONN_SIZE              sizeof(t_aal_msg_get_scanned_connection)
#define AAL_MSG_GET_SCAN_CONN_CMPLT_SIZE        sizeof(t_aal_msg_get_scanned_connection_cmplt)
#define AAL_MSG_RESET_STATISTICS_ALL_SIZE       sizeof(t_aal_msg_reset_statistics_all)  
#define AAL_MSG_RESET_STATISTICS_ALL_CMPLT_SIZE sizeof(t_aal_msg_reset_statistics_all_cmplt)     
#define AAL_MSG_GET_TRAILER_STRIP_MODE_SIZE	      sizeof(t_aal_msg_get_trailer_strip_mode)
#define AAL_MSG_GET_TRAILER_STRIP_MODE_CMPLT_SIZE sizeof(t_aal_msg_get_trailer_strip_mode_cmplt)
#define AAL_MSG_SET_TRAILER_STRIP_MODE_SIZE	      sizeof(t_aal_msg_set_trailer_strip_mode)
#define AAL_MSG_SET_TRAILER_STRIP_MODE_CMPLT_SIZE sizeof(t_aal_msg_set_trailer_strip_mode_cmplt)
#define AAL_MSG_GET_ATM_FORWARDING_MODE_SIZE       sizeof(t_aal_msg_get_atm_forwarding_mode)
#define AAL_MSG_GET_ATM_FORWARDING_MODE_CMPLT_SIZE sizeof(t_aal_msg_get_atm_forwarding_mode_cmplt)
#define AAL_MSG_SET_ATM_FORWARDING_MODE_SIZE       sizeof(t_aal_msg_set_atm_forwarding_mode)
#define AAL_MSG_SET_ATM_FORWARDING_MODE_CMPLT_SIZE sizeof(t_aal_msg_set_atm_forwarding_mode_cmplt)

/*AAL 5 message structures */

typedef struct
{
	uint32_t ConnectionNum;
	uint32_t VPI;
	uint32_t VCI;		/*identifiers for the VC to add */
	uint32_t IFace;
}t_aal_msg_activate_vc;

typedef struct
{
	uint32_t result;
}t_aal_msg_activate_vc_cmplt;

typedef struct
{
	uint32_t ConnectionNum;
	uint32_t VPI;
	uint32_t VCI;		/*identifiers for the VC to remove */
	uint32_t IFace;
}t_aal_msg_remove_vc;

typedef struct
{
	uint32_t result;
}t_aal_msg_remove_vc_cmplt;

typedef struct
{
	uint32_t length;
}t_aal_msg_read_vc_list;

typedef struct
{
	uint32_t VCListLength;
	uint32_t VCList[VCSPACE_30];	/*variable length list of VC identifiers seen*/
}t_aal_msg_read_vc_list_cmplt; 

typedef struct
{
	uint32_t Statistic;
}t_aal_msg_read_statistic;

typedef struct
{
	uint32_t statistic;
	uint32_t result;
}t_aal_msg_read_statistic_cmplt;

typedef struct 
{
	uint32_t NetMode;
	uint32_t IFace;
	uint32_t ConnectionNum;
}t_aal_msg_set_net_mode;	

typedef struct 
{
	uint32_t result;
}t_aal_msg_set_net_mode_cmplt;	

typedef struct 
{
	uint32_t NetMode;
	uint32_t IFace;
}t_aal_msg_set_iface_net_mode;	

typedef struct 
{
	uint32_t result;
}t_aal_msg_set_iface_net_mode_cmplt;	

typedef struct 
{
	uint32_t ConnectionNum;
	uint32_t VPI;
	uint32_t VCI;
	uint32_t IFace;
}t_aal_msg_get_sar_mode;	

typedef struct 
{
	uint32_t mode;
}t_aal_msg_get_sar_mode_cmplt;	

typedef struct 
{
	uint32_t ConnectionNum;
	uint32_t VPI;
	uint32_t VCI;
	uint32_t IFace;
	uint32_t Mode;
}t_aal_msg_set_sar_mode;

typedef struct 
{
	uint32_t result;
}t_aal_msg_set_sar_mode_cmplt;	


typedef struct 
{
	uint32_t Size;
}t_aal_msg_set_buffer_size;

typedef struct 
{
	uint32_t result;
}t_aal_msg_set_buffer_size_cmplt;	

typedef struct 
{
	uint32_t offset;
	uint32_t mask;
	uint32_t value;
	uint32_t operation;
	uint32_t filter_level;
	uint32_t level_conf;
	uint32_t history;
	uint32_t priority;
}t_filters_msg_set_filter;

typedef struct 
{
	uint32_t filter_id;
	uint32_t result;
}t_filters_msg_set_filter_cmplt;

typedef struct 
{
	uint32_t offset;
	uint32_t mask;
	uint32_t value;
	uint32_t operation;
	uint32_t filter_level;
	uint32_t level_conf;
	uint32_t history;
	uint32_t priority;
	uint32_t parent_id;
	uint32_t list_operation;
}t_filters_msg_set_subfilter;

typedef struct 
{
	uint32_t filter_id;
	uint32_t result;
}t_filters_msg_set_subfilter_cmplt;

typedef struct
{
	uint32_t iface;
	uint32_t bitmask;
	uint32_t match;
	uint32_t action;
}t_aal_msg_set_filter_bitmask;

typedef struct
{
	uint32_t result;
}t_aal_msg_set_filter_bitmask_cmplt;

typedef struct 
{
	uint32_t filter_id; 
}t_filters_msg_reset_filter;

typedef struct 
{
	uint32_t result;
}t_filters_msg_reset_filter_cmplt;

typedef struct 
{
	uint32_t dummy_value; /* not needed*/
}t_filters_msg_reset_all_filters;

typedef struct 
{
	uint32_t result;
}t_filters_msg_reset_all_filters_cmplt;

typedef struct 
{
	uint32_t action; 
}t_filters_msg_define_filter_action;

typedef struct 
{
	uint32_t result;
}t_filters_msg_define_filter_action_cmplt;

typedef struct 
{
	uint32_t statistic; 
}t_aal_msg_reset_statistics;

typedef struct 
{
	uint32_t result;
}t_aal_msg_reset_statistics_cmplt;

typedef struct
{
	uint32_t unused;
}t_aal_msg_reset_statistics_all;

typedef struct
{
	uint32_t result;
}t_aal_msg_reset_statistics_all_cmplt;

typedef struct
{
	uint32_t mode;		/*mode to set (vc reporting)*/	
}t_aal_msg_set_scanning_mode;

typedef struct
{
	uint32_t result;		
}t_aal_msg_set_scanning_mode_cmplt;

typedef struct
{
	uint32_t unused;		
}t_aal_msg_get_scanning_mode;

typedef struct
{
	uint32_t mode;		
}t_aal_msg_get_scanning_mode_cmplt;

typedef struct
{
	uint32_t size;
}t_aal_msg_set_burst_size;

typedef struct
{
	uint32_t result;
}t_aal_msg_set_burst_size_cmplt;

typedef struct
{
	uint32_t unused;
}t_aal_msg_flush_burst_buffer;

typedef struct
{
	uint32_t result;
}t_aal_msg_flush_burst_buffer_cmplt;

typedef struct
{
	uint32_t ConnectionNum;
	uint32_t VPI;
	uint32_t VCI;
	uint32_t iface;
	uint32_t CID;
}t_aal_msg_activate_cid;

typedef struct
{
	uint32_t result;
}t_aal_msg_activate_cid_cmplt;

typedef struct
{
	uint32_t ConnectionNum;
	uint32_t VPI;
	uint32_t VCI;
	uint32_t iface;
	uint32_t CID;
}t_aal_msg_deactivate_cid;

typedef struct
{
	uint32_t result;
}t_aal_msg_deactivate_cid_cmplt;

typedef struct
{
	uint32_t unused;
} t_aal_msg_init_scan_mode;

typedef struct
{
	uint32_t result;
} t_aal_msg_init_scan_mode_cmplt;

typedef struct
{
	uint32_t unused;
} t_aal_msg_get_scanned_connections_number;

typedef struct
{
	uint32_t result;
} t_aal_msg_get_scanned_connections_number_cmplt;

typedef struct
{
	uint32_t index;
} t_aal_msg_get_scanned_connection;

typedef struct
{
	uint32_t result;
	uint32_t iface;
	uint32_t channel;
	uint32_t vpi;
	uint32_t vci;
} t_aal_msg_get_scanned_connection_cmplt;


typedef struct 
{
	uint32_t unused;
}t_aal_msg_get_trailer_strip_mode;

typedef struct 
{
	uint32_t result;
	uint32_t mode;
}t_aal_msg_get_trailer_strip_mode_cmplt;	

typedef struct 
{
	uint32_t mode;
}t_aal_msg_set_trailer_strip_mode;

typedef struct 
{
	uint32_t result;
}t_aal_msg_set_trailer_strip_mode_cmplt;


typedef struct 
{
	uint32_t unused;
}t_aal_msg_get_atm_forwarding_mode;

typedef struct 
{
	uint32_t result;
	uint32_t mode;
}t_aal_msg_get_atm_forwarding_mode_cmplt;	

typedef struct 
{
	uint32_t mode;
}t_aal_msg_set_atm_forwarding_mode;

typedef struct 
{
	uint32_t result;
}t_aal_msg_set_atm_forwarding_mode_cmplt;


#endif		// AAL_MSG_TYPES_H
