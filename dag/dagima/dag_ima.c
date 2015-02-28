
/*
 * Copyright (c) 2004-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_ima.c,v 1.9 2006/07/27 20:36:31 cassandra Exp $
 */

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
#include <stdlib.h>
#include <inttypes.h>
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include "dag_ima.h"
#include "dagema.h"

#define IMA_MSG_TIMEOUT              10000

/* ------------------------------------------------------- */

/* IMA Messages */
#define DRB_MSG_ATMRX_SET_MODE								(DRB_CLASS_IMA | 0x0001)
#define DRB_MSG_ATMRX_SET_MODE_CMPLT						(DRB_CLASS_IMA | 0x0002)
#define DRB_MSG_ATMRX_GET_MODE								(DRB_CLASS_IMA | 0x0003)
#define DRB_MSG_ATMRX_GET_MODE_CMPLT						(DRB_CLASS_IMA | 0x0004)
#define DRB_MSG_ATMRX_GET_STATS								(DRB_CLASS_IMA | 0x0005)
#define DRB_MSG_ATMRX_GET_STATS_CMPLT						(DRB_CLASS_IMA | 0x0006)
#define DRB_MSG_ATMRX_FLUSH									(DRB_CLASS_IMA | 0x0007)
#define DRB_MSG_ATMRX_FLUSH_CMPLT							(DRB_CLASS_IMA | 0x0008)
#define DRB_MSG_ATMRX_SET_LOOPBACK							(DRB_CLASS_IMA | 0x0070)
#define DRB_MSG_ATMRX_SET_LOOPBACK_CMPLT					(DRB_CLASS_IMA | 0x0071)
#define DRB_MSG_ATMRX_GET_LOOPBACK							(DRB_CLASS_IMA | 0x0072)
#define DRB_MSG_ATMRX_GET_LOOPBACK_CMPLT					(DRB_CLASS_IMA | 0x0073)

#define DRB_MSG_ADD_TXRX_GROUP								(DRB_CLASS_IMA | 0x0060)
#define DRB_MSG_ADD_TXRX_GROUP_CMPLT						(DRB_CLASS_IMA | 0x0061)
#define DRB_MSG_ADD_RX_GROUP								(DRB_CLASS_IMA | 0x0013)
#define DRB_MSG_ADD_RX_GROUP_CMPLT							(DRB_CLASS_IMA | 0x0014)
#define DRB_MSG_REMOVE_GROUP								(DRB_CLASS_IMA | 0x0015)
#define DRB_MSG_REMOVE_GROUP_CMPLT							(DRB_CLASS_IMA | 0x0016)

#define DRB_MSG_IFC_SET_MODE								(DRB_CLASS_IMA | 0x000b)
#define DRB_MSG_IFC_SET_MODE_CMPLT							(DRB_CLASS_IMA | 0x000c)
#define DRB_MSG_IFC_GET_MODE								(DRB_CLASS_IMA | 0x000d)
#define DRB_MSG_IFC_GET_MODE_CMPLT							(DRB_CLASS_IMA | 0x000e)
#define DRB_MSG_IFC_GET_STATS								(DRB_CLASS_IMA | 0x000f)
#define DRB_MSG_IFC_GET_STATS_CMPLT							(DRB_CLASS_IMA | 0x0010)

#define DRB_MSG_GROUP_GET_STATE								(DRB_CLASS_IMA | 0x0011)
#define DRB_MSG_GROUP_GET_STATE_CMPLT						(DRB_CLASS_IMA | 0x0012)
#define DRB_MSG_GROUP_GET_IFCS								(DRB_CLASS_IMA | 0x0021)
#define DRB_MSG_GROUP_GET_IFCS_CMPLT						(DRB_CLASS_IMA | 0x0022)
#define DRB_MSG_GROUP_GET_EXPECTEDLINKS						(DRB_CLASS_IMA | 0x0015)
#define DRB_MSG_GROUP_GET_EXPECTEDLINKS_CMPLT				(DRB_CLASS_IMA | 0x0016)
#define DRB_MSG_GROUP_GET_SETTINGS							(DRB_CLASS_IMA | 0x0018)
#define DRB_MSG_GROUP_GET_SETTINGS_CMPLT					(DRB_CLASS_IMA | 0x0019)
#define DRB_MSG_GROUP_GET_STATS								(DRB_CLASS_IMA | 0x0023)
#define DRB_MSG_GROUP_GET_STATS_CMPLT						(DRB_CLASS_IMA | 0x0024)
#define DRB_MSG_GROUP_GET_RX_LINKIDS						(DRB_CLASS_IMA | 0x0082)
#define DRB_MSG_GROUP_GET_RX_LINKIDS_CMPLT					(DRB_CLASS_IMA | 0x0083)

#define DRB_MSG_GTSM_GET_STATE								(DRB_CLASS_IMA | 0x0028)
#define DRB_MSG_GTSM_GET_STATE_CMPLT						(DRB_CLASS_IMA | 0x0029)

#define DRB_MSG_AVAIL_GROUPS								(DRB_CLASS_IMA | 0x0025)
#define DRB_MSG_AVAIL_GROUPS_CMPLT							(DRB_CLASS_IMA | 0x0026)

#define DRB_MSG_LINK_GET_NE_ALARMS							(DRB_CLASS_IMA | 0x0030)
#define DRB_MSG_LINK_GET_NE_ALARMS_CMPLT					(DRB_CLASS_IMA | 0x0031)
#define DRB_MSG_LINK_GET_FE_ALARMS							(DRB_CLASS_IMA | 0x0032)
#define DRB_MSG_LINK_GET_FE_ALARMS_CMPLT					(DRB_CLASS_IMA | 0x0033)
#define DRB_MSG_LINK_GET_ICP_STATS							(DRB_CLASS_IMA | 0x0034)
#define DRB_MSG_LINK_GET_ICP_STATS_CMPLT					(DRB_CLASS_IMA | 0x0035)
#define DRB_MSG_LINK_GET_NE_DEFECT_STATS					(DRB_CLASS_IMA | 0x0036)
#define DRB_MSG_LINK_GET_NE_DEFECT_STATS_CMPLT				(DRB_CLASS_IMA | 0x0037)
#define DRB_MSG_LINK_GET_FE_DEFECT_STATS					(DRB_CLASS_IMA | 0x0038)
#define DRB_MSG_LINK_GET_FE_DEFECT_STATS_CMPLT				(DRB_CLASS_IMA | 0x0039)
#define DRB_MSG_LINK_GET_ERRORS								(DRB_CLASS_IMA | 0x0040)
#define DRB_MSG_LINK_GET_ERRORS_CMPLT						(DRB_CLASS_IMA | 0x0041)
#define DRB_MSG_LINK_GET_INHIBITS 							(DRB_CLASS_IMA | 0x0042)
#define DRB_MSG_LINK_GET_INHIBITS_CMPLT						(DRB_CLASS_IMA | 0x0043)
#define DRB_MSG_LINK_GET_STATE								(DRB_CLASS_IMA | 0x0044)
#define DRB_MSG_LINK_GET_STATE_CMPLT						(DRB_CLASS_IMA | 0x0045)
#define DRB_MSG_LINK_GET_POSITION							(DRB_CLASS_IMA | 0x0046)
#define DRB_MSG_LINK_GET_POSITION_CMPLT						(DRB_CLASS_IMA | 0x0047)
#define DRB_MSG_LINK_RXLSM_GET_STATE						(DRB_CLASS_IMA | 0x0062)
#define DRB_MSG_LINK_RXLSM_GET_STATE_CMPLT					(DRB_CLASS_IMA | 0x0063)
#define DRB_MSG_LINK_TXLSM_GET_STATE_CMPLT					(DRB_CLASS_IMA | 0x0064)
#define DRB_MSG_LINK_TXLSM_GET_STATE						(DRB_CLASS_IMA | 0x0065)
#define DRB_MSG_LINK_IFSM_GET_STATE							(DRB_CLASS_IMA | 0x0066)
#define DRB_MSG_LINK_IFSM_GET_STATE_CMPLT					(DRB_CLASS_IMA | 0x0067)
#define DRB_MSG_LINK_IESM_GET_STATE							(DRB_CLASS_IMA | 0x0068)
#define DRB_MSG_LINK_IESM_GET_STATE_CMPLT					(DRB_CLASS_IMA | 0x0069)
#define DRB_MSG_LINK_GET_OIF_TOGGLE							(DRB_CLASS_IMA | 0x0080)
#define DRB_MSG_LINK_GET_OIF_TOGGLE_CMPLT					(DRB_CLASS_IMA | 0x0081)

#define DRB_MSG_UM_SET_ICPCTRL								(DRB_CLASS_IMA | 0x0048)
#define DRB_MSG_UM_GET_ICPCTRL								(DRB_CLASS_IMA | 0x0049)
#define DRB_MSG_UM_SET_ICPCTRL_CMPLT						(DRB_CLASS_IMA | 0x0050)
#define DRB_MSG_UM_GET_ICPCTRL_CMPLT						(DRB_CLASS_IMA | 0x0051)

#define DRB_MSG_HOLE_GET_BURST_STATS						(DRB_CLASS_IMA | 0x0074)
#define DRB_MSG_HOLE_GET_BURST_STATS_CMPLT					(DRB_CLASS_IMA | 0x0075)
#define DRB_MSG_HOLE_SET_BURST_SIZE							(DRB_CLASS_IMA | 0x0078)
#define DRB_MSG_HOLE_SET_BURST_SIZE_CMPLT					(DRB_CLASS_IMA | 0x0079)
#define DRB_MSG_HOLE_BURST									(DRB_CLASS_IMA | 0x0076)
#define DRB_MSG_HOLE_BURST_CMPLT							(DRB_CLASS_IMA | 0x0077)

/*	Message Sizes */
#define DRB_MSG_ATMRX_SET_MODE_SIZE							sizeof(t_drb_msg_atmrx_mode)
#define DRB_MSG_ATMRX_SET_MODE_CMPLT_SIZE					sizeof(t_drb_msg_atmrx_mode_cmplt)
#define DRB_MSG_ATMRX_GET_MODE_SIZE							sizeof(t_drb_msg_atmrx_mode)
#define DRB_MSG_ATMRX_GET_MODE_CMPLT_SIZE					sizeof(t_drb_msg_atmrx_mode_cmplt)
#define DRB_MSG_ATMRX_GET_STATS_SIZE						sizeof(t_drb_msg_stats_cells)
#define DRB_MSG_ATMRX_GET_STATS_CMPLT_SIZE					sizeof(t_drb_msg_stats_cells_cmplt)
#define DRB_MSG_ATMRX_FLUSH_SIZE							sizeof(t_drb_msg_atmrx_flush)
#define DRB_MSG_ATMRX_FLUSH_CMPLT_SIZE						sizeof(t_drb_msg_atmrx_flush_cmplt)
#define DRB_MSG_ATMRX_SET_LOOPBACK_SIZE						sizeof(t_drb_msg_atmrx_loopback)
#define DRB_MSG_ATMRX_SET_LOOPBACK_CMPLT_SIZE				sizeof(t_drb_msg_atmrx_loopback_cmplt)
#define DRB_MSG_ATMRX_GET_LOOPBACK_SIZE						sizeof(t_drb_msg_atmrx_loopback)
#define DRB_MSG_ATMRX_GET_LOOPBACK_CMPLT_SIZE				sizeof(t_drb_msg_atmrx_loopback_cmplt)

#define DRB_MSG_ADD_TXRX_GROUP_SIZE							sizeof(t_drb_msg_add_txrx_group)
#define DRB_MSG_ADD_TXRX_GROUP_CMPLT_SIZE					sizeof(t_drb_msg_add_txrx_group_cmplt)
#define DRB_MSG_ADD_RX_GROUP_SIZE							sizeof(t_drb_msg_add_rx_group)
#define DRB_MSG_ADD_RX_GROUP_CMPLT_SIZE						sizeof(t_drb_msg_add_rx_group_cmplt)
#define DRB_MSG_REMOVE_GROUP_SIZE							sizeof(t_drb_msg_remove_group)
#define DRB_MSG_REMOVE_GROUP_CMPLT_SIZE						sizeof(t_drb_msg_remove_group_cmplt)

#define DRB_MSG_IFC_SET_MODE_SIZE							sizeof(t_drb_msg_ifc_mode)
#define DRB_MSG_IFC_SET_MODE_CMPLT_SIZE						sizeof(t_drb_msg_ifc_mode_cmplt)
#define DRB_MSG_IFC_GET_MODE_SIZE							sizeof(t_drb_msg_ifc_mode)
#define DRB_MSG_IFC_GET_MODE_CMPLT_SIZE						sizeof(t_drb_msg_ifc_mode_cmplt)
#define DRB_MSG_IFC_GET_STATS_SIZE							sizeof(t_drb_msg_stats_cells)
#define DRB_MSG_IFC_GET_STATS_CMPLT_SIZE					sizeof(t_drb_msg_stats_cells_cmplt)

#define DRB_MSG_GROUP_GET_STATE_SIZE						sizeof(t_drb_msg_group)
#define DRB_MSG_GROUP_GET_STATE_CMPLT_SIZE					sizeof(t_drb_msg_group_get_state_cmplt)
#define DRB_MSG_GROUP_GET_IFCS_SIZE							sizeof(t_drb_msg_group)
#define DRB_MSG_GROUP_GET_IFCS_CMPLT_SIZE					sizeof(t_drb_msg_group_get_ifcs_cmplt)
#define DRB_MSG_GROUP_GET_EXPECTEDLINKS_SIZE				sizeof(t_drb_msg_group)
#define DRB_MSG_GROUP_GET_EXPECTEDLINKS_CMPLT_SIZE			sizeof(t_drb_msg_group_get_expectedlinks_cmplt)
#define DRB_MSG_GROUP_GET_SETTINGS_SIZE						sizeof(t_drb_msg_group)
#define DRB_MSG_GROUP_GET_SETTINGS_CMPLT_SIZE				sizeof(t_drb_msg_group_get_settings_cmplt)
#define DRB_MSG_GROUP_GET_STATS_SIZE						sizeof(t_drb_msg_group)
#define DRB_MSG_GROUP_GET_STATS_CMPLT_SIZE					sizeof(t_drb_msg_stats_cells_cmplt)
#define DRB_MSG_GROUP_GET_RX_LINKIDS_SIZE					sizeof(t_drb_msg_group)
#define DRB_MSG_GROUP_GET_RX_LINKIDS_CMPLT_SIZE				sizeof(t_drb_msg_group_get_rx_linkidmask_cmplt)
#define DRB_MSG_AVAIL_GROUPS_SIZE							sizeof(t_drb_msg_group)
#define DRB_MSG_AVAIL_GROUPS_CMPLT_SIZE						sizeof(t_drb_msg_avail_groups_cmplt)

#define DRB_MSG_GTSM_GET_STATE_SIZE							sizeof(t_drb_msg_gtsm_get_state)
#define DRB_MSG_GTSM_GET_STATE_CMPLT_SIZE					sizeof(t_drb_msg_gtsm_get_state_cmplt)

#define DRB_MSG_LINK_GET_NE_ALARMS_SIZE						sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_NE_ALARMS_CMPLT_SIZE				sizeof(t_drb_msg_link_get_ne_alarms_cmplt)
#define DRB_MSG_LINK_GET_FE_ALARMS_SIZE						sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_FE_ALARMS_CMPLT_SIZE				sizeof(t_drb_msg_link_get_fe_alarms_cmplt)
#define DRB_MSG_LINK_GET_ICP_STATS_SIZE						sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_ICP_STATS_CMPLT_SIZE				sizeof(t_drb_msg_link_get_ICP_statistics_cmplt)
#define DRB_MSG_LINK_GET_NE_DEFECT_STATS_SIZE				sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_NE_DEFECT_STATS_CMPLT_SIZE			sizeof(t_drb_msg_link_get_ne_defect_statistics_cmplt)
#define DRB_MSG_LINK_GET_FE_DEFECT_STATS_SIZE				sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_FE_DEFECT_STATS_CMPLT_SIZE			sizeof(t_drb_msg_link_get_fe_defect_statistics_cmplt)
#define DRB_MSG_LINK_GET_ERRORS_SIZE						sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_ERRORS_CMPLT_SIZE					sizeof(t_drb_msg_link_get_errors_cmplt)
#define DRB_MSG_LINK_GET_INHIBITS_SIZE						sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_INHIBITS_CMPLT_SIZE				sizeof(t_drb_msg_link_get_inhibits_cmplt)
#define DRB_MSG_LINK_GET_STATE_SIZE							sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_STATE_CMPLT_SIZE					sizeof(t_drb_msg_link_get_state_cmplt)
#define DRB_MSG_LINK_GET_POSITION_SIZE						sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_POSITION_CMPLT_SIZE				sizeof(t_drb_msg_link_get_position_cmplt)
#define DRB_MSG_LINK_RXLSM_GET_STATE_SIZE					sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_RXLSM_GET_STATE_CMPLT_SIZE				sizeof(t_drb_msg_link_rxlsm_get_state_cmplt)
#define DRB_MSG_LINK_TXLSM_GET_STATE_SIZE					sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_TXLSM_GET_STATE_CMPLT_SIZE				sizeof(t_drb_msg_link_txlsm_get_state_cmplt)
#define DRB_MSG_LINK_IFSM_GET_STATE_SIZE					sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_IFSM_GET_STATE_CMPLT_SIZE				sizeof(t_drb_msg_link_ifsm_get_state_cmplt)
#define DRB_MSG_LINK_IESM_GET_STATE_SIZE					sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_IESM_GET_STATE_CMPLT_SIZE				sizeof(t_drb_msg_link_iesm_get_state_cmplt)
#define DRB_MSG_LINK_GET_OIF_TOGGLE_SIZE					sizeof(t_drb_msg_link)
#define DRB_MSG_LINK_GET_OIF_TOGGLE_CMPLT_SIZE				sizeof(t_drb_msg_link_get_OIF_toggle_cmplt)

#define DRB_MSG_UM_SET_ICPCTRL_SIZE							sizeof(t_drb_msg_um_set_icpctrl)
#define DRB_MSG_UM_GET_ICPCTRL_SIZE							sizeof(t_drb_msg_um_get_icpctrl)
#define DRB_MSG_UM_SET_ICPCTRL_CMPLT_SIZE					sizeof(t_drb_msg_um_set_icpctrl_cmplt)
#define DRB_MSG_UM_GET_ICPCTRL_CMPLT_SIZE					sizeof(t_drb_msg_um_get_icpctrl_cmplt)

#define DRB_MSG_HOLE_GET_BURST_STATS_SIZE					sizeof(t_drb_msg_hole_get_burst_stats)
#define DRB_MSG_HOLE_GET_BURST_STATS_CMPLT_SIZE				sizeof(t_drb_msg_hole_get_burst_stats_cmplt)
#define DRB_MSG_HOLE_SET_BURST_SIZE_SIZE					sizeof(t_drb_msg_hole_set_burst_size)
#define DRB_MSG_HOLE_SET_BURST_SIZE_CMPLT_SIZE				sizeof(t_drb_msg_hole_set_burst_size_cmplt)
#define DRB_MSG_HOLE_BURST_SIZE								sizeof(t_drb_msg_hole_burst)
#define DRB_MSG_HOLE_BURST_CMPLT_SIZE						sizeof(t_drb_msg_hole_burst_cmplt)

/* ------------------------------------------------------- */

typedef struct 
{
	uint32_t debug;
} t_drb_msg_set_debug;

typedef struct 
{
	int32_t result;
} t_drb_msg_set_debug_cmplt;

/* Return results */
#define DRB_ERROR_NONE                 0       /* Success */
#define DRB_ERROR_UNKNOWN_MSG          0       /* Message ID is not recognized */

typedef struct 
{
	uint32_t request;
} t_drb_msg_read_version;

typedef struct 
{
	int32_t result;
	uint32_t version;
	uint32_t type;
} t_drb_msg_read_version_cmplt;

typedef struct 
{
	uint32_t length;            /* Number of bytes to read */
} t_drb_msg_read_software_id;

typedef struct 
{
	int32_t result;            /* Result of the operation */
	uint32_t length;            /* length of software ID (bytes) */
	uint8_t software_id[0];     /* Variable length software ID */
} t_drb_msg_read_software_id_cmplt;

typedef struct 
{
	uint32_t length;            /* Number of bytes in the software ID */
	uint32_t key;               /* Security key to enable write access */
	uint8_t software_id[0];     /* Variable length software ID */
} t_drb_msg_write_software_id;

typedef struct 
{
	int32_t result;            /* Result of the write */
} t_drb_msg_write_software_id_cmplt;

typedef struct 
{
	uint32_t sensor_id;         /* Which sensor to read */
} t_drb_msg_read_temperature;

typedef struct 
{
	int32_t result;            /* Result of the operation */
	uint32_t temperature;        /* The temperature read */
} t_drb_msg_read_temperature_cmplt;

/*	Used to get/set the mode of the ATMRX */
typedef struct
{
	uint32_t wMode;
} t_drb_msg_atmrx_mode;

typedef struct
{
	int32_t result;
} t_drb_msg_atmrx_mode_cmplt;

typedef struct
{
	uint32_t ifcmask;
} t_drb_msg_add_rx_group;

typedef struct
{
	uint32_t handle;
} t_drb_msg_add_rx_group_cmplt;

typedef struct
{
	uint32_t ID;
	uint32_t txlinks;
	uint32_t rxlinks;
	uint32_t framelen;
	symconfig_mode_t symconfig;
	uint8_t clock;
	uint32_t ifcmask;
} t_drb_msg_add_txrx_group;

typedef struct
{
	uint32_t handle;
} t_drb_msg_add_txrx_group_cmplt;

typedef struct
{
	uint32_t handle;
} t_drb_msg_remove_group;

typedef struct
{
	int32_t result;
} t_drb_msg_remove_group_cmplt;

typedef struct
{
	uint32_t group_handle;
} t_drb_msg_group;

typedef struct
{
	uint32_t ifcmask;
	int32_t result;
} t_drb_msg_group_get_ifcs_cmplt;

typedef struct
{
	uint32_t wIfc;
	uint32_t mode;
} t_drb_msg_ifc_mode;

typedef struct
{
	int32_t result;
} t_drb_msg_ifc_mode_cmplt;

typedef struct
{
	uint32_t ifc;
} t_drb_msg_ifc_get_grouphandle;

typedef struct
{
	uint32_t handle;
} t_drb_msg_ifc_get_grouphandle_cmplt;

typedef struct
{
	state_group_t group_state;
	int32_t result;
} t_drb_msg_group_get_state_cmplt;

typedef struct
{
	int32_t num_rxlinks;
	uint32_t rxlinkidmask;
} t_drb_msg_group_get_rx_linkidmask_cmplt;

typedef struct
{
	uint32_t dummy;
} t_drb_msg_avail_groups;

typedef struct
{
	uint32_t numgroups;
	uint32_t groupmask;
} t_drb_msg_avail_groups_cmplt;

typedef struct 
{
	uint32_t wDummy;
} t_drb_msg_groupcount;

typedef struct
{
	uint32_t wGroupCount;
} t_drb_msg_groupcount_cmplt;

typedef struct
{
	uint32_t numifcs;
	uint32_t framelen;
	int32_t result;
} t_drb_msg_group_get_settings_cmplt;

typedef struct
{
	uint32_t cellcmd;							/* tells whether the option is a group or ifc */
	uint32_t ID;
} t_drb_msg_stats_cells;

typedef struct
{
	int32_t result;
	uint32_t cellcount;
} t_drb_msg_stats_cells_cmplt;

typedef struct
{
	uint32_t handle;							/* handle of the group */
} t_drb_msg_gtsm_get_state;

typedef struct
{
	gtsm_state_t gtsm_state;					/* status of the configuration of IMA groups */
} t_drb_msg_gtsm_get_state_cmplt;

/*	Burst control management									*/

/*	The bursthole stats */
typedef struct
{
	uint32_t allocated;				/*	Size of the mem hole allocated. */
	uint32_t empty;					/*	Size of the free space in the allocated memhole */
	int32_t result;
} t_drb_msg_hole_get_burst_stats_cmplt;

/*	Tell the burst hole to spew its memory out */
typedef struct
{
	uint32_t dummy;
} t_drb_msg_hole_burst;

typedef struct
{
	int32_t result;
} t_drb_msg_hole_burst_cmplt;

/*	Retrieve the memory burst hole stats. */
typedef struct 
{	
	uint32_t dummy;
} t_drb_msg_hole_get_burst_stats;

/*	Change the size of the burst buffer */
typedef struct
{
	uint32_t burstsize;
} t_drb_msg_hole_set_burst_size;

typedef struct 
{
	int32_t result;
} t_drb_msg_hole_set_burst_size_cmplt;

/* -------------------------------------------------------- */
/*	Group/link (IFSM/IESM ) state querying 					*/

typedef struct
{
	int32_t group_handle;
	uint32_t linkID;
} t_drb_msg_link;

typedef struct
{
	uint32_t framepos;
	uint32_t framesqn;
	int32_t result;
} t_drb_msg_link_get_position_cmplt;

typedef struct
{
	uint32_t errorfield;
	int32_t result;
} t_drb_msg_link_get_errors_cmplt;

typedef struct
{
	uint32_t inhibitfield;
	int32_t result;
} t_drb_msg_link_get_inhibits_cmplt;

typedef struct
{
	state_rx_t state_rx;
	int32_t result;
} t_drb_msg_link_rxlsm_get_state_cmplt;

typedef struct
{
	state_tx_t state_tx;
	int32_t result;
} t_drb_msg_link_txlsm_get_state_cmplt;

typedef struct
{
	ifsm_state_t ifsm_state;		/*	state of the IFSM */
	int32_t result;
} t_drb_msg_link_ifsm_get_state_cmplt;

typedef struct
{
	int32_t result;
} t_drb_msg_link_get_OIF_toggle_cmplt;

typedef struct
{
	iesm_state_t iesm_state;
	int32_t result;
} t_drb_msg_link_iesm_get_state_cmplt;

/* -------------------------------------------------------- */
/*	We break down the UM flag messages 					*/

/*	Retrieve the UM stats */
typedef struct
{
	uint32_t dummy;
} t_drb_msg_um_buffer_get_ctrl;

/*	Behaviour controlling the link delay and rx buffer size. */
typedef struct
{
	uint32_t rx_smoothbuf_maxsize; 	
	uint32_t tx_smoothbuf_maxsize;	
	uint32_t rx_smoothbuf_default_size;
	uint32_t tx_smoothbuf_default_size;
	uint32_t rx_delay;						/*	Default Rx delay to build the IFC */
	uint32_t tx_delay;						/*	Tx delay ? not sure if this is reuqired at the moment */
	uint8_t result;						
} t_drb_msg_um_buffer_get_ctrl_cmplt;

/*	We only let the user change the size of the rx buffer,
	the tx buffer and the delays for each buffer. */
typedef struct
{
	uint32_t rx_delay;
	uint32_t tx_delay;
	uint32_t rx_smoothbuf_default_size;
	uint32_t tx_smoothbuf_default_size;
} t_drb_msg_um_buffer_set_ctrl;

/* Per group control */
typedef struct
{
	uint32_t rx_delay;
	uint32_t tx_delay;
	uint32_t rx_smoothbuf_size;
	uint32_t tx_smoothbuf_size;
	int32_t result;
} t_drb_msg_group_get_buffer_ctrl;

typedef struct
{
	uint8_t icp_drop;
	uint8_t icp_send_trl;
	uint8_t filler_drop;
	int32_t result;
} t_drb_msg_um_set_icpctrl;

typedef struct
{	
	int32_t result;
} t_drb_msg_um_set_icpctrl_cmplt;

typedef struct
{
	uint32_t dummy;
} t_drb_msg_um_get_icpctrl;

/*	Control ICP flow */
typedef struct 
{
	uint8_t icp_drop;						/*	Drop the ICP cells */
	uint8_t icp_send_trl;					/*	Send the ICP cells from the TRL which have changed */
	uint8_t filler_drop;					/*	Drop the filler cells */
	int32_t result;
} t_drb_msg_um_get_icpctrl_cmplt;

/* -------------------------------------------------------- */
/*	Link statistics 									*/

typedef struct
{
	uint32_t errored_ICP;	/*	Cell with a HEC or CRC error at expected ICP cell position if it is not a Missing cell. */
	uint32_t invalid_ICP;	/*	Cell with good HEC & CRC and CID = ICP at expected frame position with another unexpected field, refer to pg 69 */
	uint32_t valid_ICP;		/*	Cell with the correct fields */
	uint32_t missing_ICP;	/*	Cell which is not where it is supposed to be */
	int32_t result;
} t_drb_msg_link_get_ICP_statistics_cmplt;

typedef struct
{
	uint32_t AIS;
	uint32_t LCD;
	uint32_t LIF;
	
	uint32_t tx_LODS;
	uint32_t rx_LODS;
	int32_t result;
} t_drb_msg_link_get_ne_defect_statistics_cmplt;

typedef struct
{
	uint32_t Tx_FC_FE;
	uint32_t Rx_FC_FE;
	int32_t result;
} t_drb_msg_link_get_fe_defect_statistics_cmplt;

typedef struct
{
	uint32_t LIF;
	uint32_t LODS;
	uint32_t RFI_IMA;
	
	uint8_t tx_mis_connected;		/*	(R-141) Tx link is detected to be misconnected */
	uint8_t rx_mis_connected;		/*	(R-142) Rx link is detected to be misconnected */
	int32_t result;
} t_drb_msg_link_get_ne_alarms_cmplt;

typedef struct
{
	uint8_t tx_unusable_FE;			/*	(R-143) Fe reports Tx-Unusable */
	uint8_t rx_unusable_FE;			/*	(R-144) Fe reports Rx-Unusable */
	int32_t result;
} t_drb_msg_link_get_fe_alarms_cmplt;

/* -------------------------------------------------------- */

static  
int dagima_open_library( int dagfd )
{
	/* open a connection to the ema library */
	if( dagema_open_conn( dagfd ) == 0 )
	{
		return 1;
	}
	else
	{
		/* check if we failed because the library is already open */
		int res = dagema_get_last_error();
		if ( res != EEXIST )
		{
			/* reset the xscale and try communicating again */
			dagema_reset_processor( dagfd, 0 );
			
			if ( dagema_open_conn( dagfd ) == 0 )
				return 1;
			else return -3;
		}
	}
	
	return 0;
}

/* -------------------------------------------------------- */
/*	UM flags/ICP cell flow										*/

int32_t
dagima_um_set_icpctrl( int dagfd, 
					   uint32_t icp_drop,
					   uint32_t icp_send_trl,
					   uint32_t filler_drop  )
{
	t_drb_msg_um_set_icpctrl cmd;
	t_drb_msg_um_set_icpctrl_cmplt reply ;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	msg_id  = DRB_MSG_UM_SET_ICPCTRL;
	msg_len = DRB_MSG_UM_SET_ICPCTRL_SIZE;
	cmd.icp_drop = icp_drop;
	cmd.icp_send_trl = icp_send_trl;
	cmd.filler_drop = filler_drop;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}
	if( msg_id != DRB_MSG_UM_SET_ICPCTRL_CMPLT || msg_len != DRB_MSG_UM_SET_ICPCTRL_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}

int32_t 
dagima_um_get_icpctrl( int dagfd, dag_um_icpctrl *picpctrl )
{	
	t_drb_msg_um_get_icpctrl cmd;
	t_drb_msg_um_get_icpctrl_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_UM_GET_ICPCTRL;
	msg_len = DRB_MSG_UM_GET_ICPCTRL_SIZE;
	cmd.dummy = 0;
	
	/* send the request */	
	if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof( reply );
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	

	if( msg_id != DRB_MSG_UM_GET_ICPCTRL_CMPLT || msg_len != DRB_MSG_UM_GET_ICPCTRL_CMPLT_SIZE )
	{
		printf( "Invalid\n" );
		res = -7;
		goto Exit;
	}
	
	picpctrl->icp_drop = dagema_le32toh( reply.icp_drop );
	picpctrl->icp_send_trl = dagema_le32toh( reply.icp_send_trl );
	picpctrl->filler_drop = dagema_le32toh( reply.filler_drop);
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

/* -------------------------------------------------------- */
/*	Link handling routines, Rx/Tx state and IFSM/IESM status */

int32_t
dagima_link_get_rxlsm_state( int dagfd, uint32_t group_handle, uint32_t linkID, state_rx_t *pstate_rx )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_rxlsm_get_state_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_RXLSM_GET_STATE;
	msg_len = DRB_MSG_LINK_RXLSM_GET_STATE_SIZE;
	cmd.group_handle = group_handle;
	cmd.linkID = linkID;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}	
	
	if( msg_id != DRB_MSG_LINK_RXLSM_GET_STATE_CMPLT || msg_len != DRB_MSG_LINK_RXLSM_GET_STATE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*pstate_rx = dagema_le32toh( reply.state_rx );
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

int32_t
dagima_link_get_txlsm_state( int dagfd, uint32_t group_handle, uint32_t linkID, state_tx_t *pstate_tx )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_txlsm_get_state_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_TXLSM_GET_STATE;
	msg_len = DRB_MSG_LINK_TXLSM_GET_STATE_SIZE;
	cmd.group_handle = group_handle;
	cmd.linkID = linkID;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0)
	{
		res = -3;
		goto Exit;
	}	
	
	if( msg_id != DRB_MSG_LINK_TXLSM_GET_STATE_CMPLT || msg_len != DRB_MSG_LINK_TXLSM_GET_STATE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*pstate_tx = dagema_le32toh( reply.state_tx );
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

int32_t
dagima_link_get_ifsm_state( int dagfd, uint32_t group_handle, uint32_t linkID, ifsm_state_t* pifsm_state )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_ifsm_get_state_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_IFSM_GET_STATE;
	msg_len = DRB_MSG_LINK_IFSM_GET_STATE_SIZE;
	cmd.group_handle = group_handle;
	cmd.linkID = linkID;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0)
	{
		res = -3;
		goto Exit;
	}	
	
	if( msg_id != DRB_MSG_LINK_IFSM_GET_STATE_CMPLT || msg_len != DRB_MSG_LINK_IFSM_GET_STATE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*pifsm_state = dagema_le32toh( reply.ifsm_state );
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

int32_t
dagima_link_get_iesm_state( int dagfd, uint32_t group_handle, uint32_t linkID, iesm_state_t* piesm_state )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_iesm_get_state_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_IESM_GET_STATE;
	msg_len = DRB_MSG_LINK_IESM_GET_STATE_SIZE;
	cmd.group_handle = group_handle;
	cmd.linkID = linkID;
		
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0)
	{
		res = -3;
		goto Exit;
	}	
	
	if( msg_id != DRB_MSG_LINK_IESM_GET_STATE_CMPLT || msg_len != DRB_MSG_LINK_IESM_GET_STATE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*piesm_state = dagema_le32toh( reply.iesm_state );
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

int32_t 
dagima_link_get_OIF_toggle( int dagfd, uint32_t group_handle, uint32_t linkID )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_get_OIF_toggle_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_OIF_TOGGLE;
	msg_len = DRB_MSG_LINK_GET_OIF_TOGGLE_SIZE;
	cmd.group_handle = group_handle;
	cmd.linkID = linkID;
		
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0)
	{
		res = -3;
		goto Exit;
	}	
	
	if( msg_id != DRB_MSG_LINK_GET_OIF_TOGGLE_CMPLT || msg_len != DRB_MSG_LINK_GET_OIF_TOGGLE_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	res = dagema_le32toh( reply.result );	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return res;
}

#if 0
int32_t 
dagima_link_get_position( int dagfd, int32_t group_handle, uint32_t linkID )
{	
	t_drb_msg_link cmd;
	t_drb_msg_link_get_position_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_POSITION;
	msg_len = DRB_MSG_LINK_GET_POSITION_SIZE;
	cmd.wMode = 0;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_LINK_GET_POSITION_CMPLT || msg_len != DRB_MSG_LINK_GET_POSITION_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}
#endif

int32_t 
dagima_link_get_errors( int dagfd, int32_t group_handle, uint32_t linkID, uint32_t *errorfield )
{	
	t_drb_msg_link cmd;
	t_drb_msg_link_get_errors_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_ERRORS;
	msg_len = DRB_MSG_LINK_GET_ERRORS_SIZE;
	cmd.group_handle = group_handle;
	cmd.linkID = linkID;
			
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_LINK_GET_ERRORS_CMPLT || msg_len != DRB_MSG_LINK_GET_ERRORS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*errorfield = dagema_le32toh( reply.errorfield );
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

int32_t 
dagima_link_get_inhibits( int dagfd, int32_t group_handle, uint32_t linkID, uint32_t *errorfield )
{	
	t_drb_msg_link cmd;
	t_drb_msg_link_get_inhibits_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_INHIBITS;
	msg_len = DRB_MSG_LINK_GET_INHIBITS_SIZE;
	cmd.group_handle = group_handle;
	cmd.linkID = linkID;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_LINK_GET_INHIBITS_CMPLT || msg_len != DRB_MSG_LINK_GET_INHIBITS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

/* -------------------------------------------------------- */
/*	Burst control management									*/

/*	Function : dagima_hole_get_burst_stats
	Retrieve the operating conditions of the Tx memory hole.
	This only returns valid results if the hole is operating
	in burst mode. Otherwise, it will return 0 allocated
	and 0 empty.
*/
int32_t
dagima_hole_get_burst_stats( int dagfd, uint32_t* pallocated, uint32_t* pempty )
{
	t_drb_msg_hole_get_burst_stats cmd;
	t_drb_msg_hole_get_burst_stats_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_HOLE_GET_BURST_STATS;
	msg_len = DRB_MSG_HOLE_GET_BURST_STATS_SIZE;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof( reply );
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_HOLE_GET_BURST_STATS_CMPLT || msg_len != DRB_MSG_HOLE_GET_BURST_STATS_CMPLT )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*pallocated = dagema_le32toh( reply.allocated );
	*pempty = dagema_le32toh( reply.empty );
	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return res;
}

int32_t
dagima_hole_burst( int dagfd )
{
	t_drb_msg_hole_burst cmd;
	t_drb_msg_hole_burst_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_HOLE_BURST;
	msg_len = DRB_MSG_HOLE_BURST_SIZE;
		
	/* send the request */	
    if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof( reply );
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_HOLE_BURST_CMPLT || msg_len != DRB_MSG_HOLE_BURST_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh(reply.result);
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

int32_t
dagima_hole_set_burst_size( int dagfd, uint32_t burstsize )
{
	t_drb_msg_hole_set_burst_size cmd;
	t_drb_msg_hole_set_burst_size_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;

	int      libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_HOLE_SET_BURST_SIZE;
	msg_len = DRB_MSG_HOLE_SET_BURST_SIZE_SIZE;
	cmd.burstsize = burstsize;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_HOLE_SET_BURST_SIZE_CMPLT || msg_len != DRB_MSG_HOLE_SET_BURST_SIZE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

/* -------------------------------------------------------- */

/*	Function : dagima_set_atmrx_mode
	Set the ATM Rx mode.
*/
int32_t
dagima_atmrx_set_mode( int dagfd, atmrx_mode_t mode )
{
	t_drb_msg_atmrx_mode cmd;
	t_drb_msg_atmrx_mode_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_ATMRX_SET_MODE;
	msg_len = DRB_MSG_ATMRX_SET_MODE_SIZE;
	cmd.wMode  = dagema_htole32(mode);
	
	/* send the request */	
    if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_ATMRX_SET_MODE_CMPLT || msg_len != DRB_MSG_ATMRX_SET_MODE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh(reply.result);
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}

atmrx_mode_t
dagima_atmrx_get_mode( int dagfd )
{
	t_drb_msg_atmrx_mode cmd;
	t_drb_msg_atmrx_mode_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_ATMRX_GET_MODE;
	msg_len = DRB_MSG_ATMRX_GET_MODE_SIZE;
	cmd.wMode = 0;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_ATMRX_GET_MODE_CMPLT || msg_len != DRB_MSG_ATMRX_GET_MODE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh(reply.result);

Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn (dagfd, 0);
	}
	
	return res;
}

/*	Function: dagima_atmrx_get_stats
	Retrieve the ATMRx statistics.
*/
int32_t
dagima_atmrx_get_stats( int dagfd, uint32_t cell_cmd )
{
	t_drb_msg_stats_cells cmd;
	t_drb_msg_stats_cells_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id = DRB_MSG_ATMRX_GET_STATS;
	msg_len = DRB_MSG_ATMRX_GET_STATS_SIZE;
	cmd.cellcmd = dagema_htole32(cell_cmd);
	cmd.ID = 0;
	
	/* send the request */	
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}
	if( msg_id != DRB_MSG_ATMRX_GET_STATS_CMPLT || msg_len != DRB_MSG_ATMRX_GET_STATS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */
	res = dagema_le32toh( reply.result );
	if( res == 0 )
	{
		res = dagema_le32toh( reply.cellcount );
	}
	
Exit:
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn (dagfd, 0);
	}
	
	return res;
}

/* ------------------------------------------------------- */

/*	Function:	dagima_add_rx_group
	Add a new group to the IMA UM given the group id and the interfaces
	to add to the group.
*/
int32_t
dagima_add_rx_group( int dagfd, uint32_t ifc_mask )
{
	t_drb_msg_add_rx_group cmd;
	t_drb_msg_add_rx_group_cmplt reply;
	uint32_t	msg_len;
	uint32_t	msg_id;
	int			res = 0;
	int			libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_ADD_RX_GROUP;
	msg_len = DRB_MSG_ADD_RX_GROUP_SIZE;
	cmd.ifcmask  = dagema_htole32(ifc_mask);
	
	/* send the request */	
    if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}

	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_ADD_RX_GROUP_CMPLT || msg_len != DRB_MSG_ADD_RX_GROUP_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.handle );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;

} /* dagima_add_ima_group */

/*	Function: dagima_remove_group
	Remove a group, given its handle group_handle.s
*/
int32_t
dagima_remove_group( int dagfd, uint32_t group_handle )
{
	t_drb_msg_group cmd;
	t_drb_msg_remove_group_cmplt reply;
	uint32_t	msg_len;
	uint32_t	msg_id;
	int			res = 0;
	int			libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_REMOVE_GROUP;
	msg_len = DRB_MSG_REMOVE_GROUP_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	
	/* send the request */	
	if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}
	if( msg_id != DRB_MSG_REMOVE_GROUP_CMPLT || msg_len != DRB_MSG_REMOVE_GROUP_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh(reply.result);
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}

int32_t
dagima_group_get_rx_linkidmask( int dagfd, uint32_t group_handle, uint32_t* rxlinkidmask )
{
	t_drb_msg_group cmd;
	t_drb_msg_group_get_rx_linkidmask_cmplt reply;
	uint32_t	msg_len;
	uint32_t	msg_id;
	int			res = 0;
	int			libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_GROUP_GET_RX_LINKIDS;
	msg_len = DRB_MSG_GROUP_GET_RX_LINKIDS_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_GROUP_GET_RX_LINKIDS_CMPLT || msg_len != DRB_MSG_GROUP_GET_RX_LINKIDS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.num_rxlinks );
	
	/*	For the moment, if the number of links is less then 0, then
		we have an error, so return it as the result. */
	if( res > 0 )
	{
		*rxlinkidmask = dagema_le32toh( reply.rxlinkidmask );
	}
	
Exit:
	/* if the library was opened close it now */
	if( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return res;
}

/* ------------------------------------------------------- */

/*	Function : dagima_ifc_get_mode
	Retrieve the mode of the IFC.
*/
ifc_mode_t 
dagima_ifc_get_mode( int dagfd, uint32_t ifc )
{
	t_drb_msg_ifc_mode cmd;
	t_drb_msg_ifc_mode_cmplt reply;
	uint32_t	msg_len;
	uint32_t	msg_id;
	int			res = 0;
	int			libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_IFC_GET_MODE;
	msg_len = DRB_MSG_IFC_GET_MODE_SIZE;
	cmd.wIfc  = dagema_htole32(ifc);
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_IFC_GET_MODE_CMPLT || msg_len != DRB_MSG_IFC_GET_MODE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return (ifc_mode_t) res;
}

/*	Function : dagima_ifc_get_mode
	Retrieve the mode of the IFC.
*/
int32_t
dagima_ifc_set_mode( int dagfd, uint32_t ifc, ifc_mode_t mode )
{
	t_drb_msg_ifc_mode cmd;
	t_drb_msg_ifc_mode_cmplt reply;
	uint32_t	msg_len;
	uint32_t	msg_id;
	int			res = 0;
	int			libema_opened;
	
	/* open a connection to the ema library */
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_IFC_SET_MODE;
	msg_len = DRB_MSG_IFC_SET_MODE_SIZE;
	cmd.wIfc  = dagema_htole32(ifc);
	
	/* send the request */	
    if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_IFC_SET_MODE_CMPLT || msg_len != DRB_MSG_IFC_SET_MODE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return (ifc_mode_t) res;
}

#if 0
/*	Function: dagima_ifc_grouphandle
	Return the grouphandle of the IFC requested.
*/	
int32_t
dagima_ifc_get_grouphandle( int dagfd, uint32_t ifc )
{
	t_drb_msg_ifc_get_grouphandle cmd;
	t_drb_msg_ifc_get_grouphandle_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_IFC_GET_GROUPHANDLE;
	msg_len = DRB_MSG_IFC_GET_GROUPHANDLE_SIZE;
	cmd.wIfc  = dagema_htole32(ifc);
	
	/* send the request */	
    if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}	
	if (msg_id != DRB_MSG_IFC_GET_GROUPHANDLE_CMPLT || msg_len != DRB_MSG_IFC_GET_GROUPHANDLE_CMPLT_SIZE)
	{
		res = -7;
		goto Exit;
	}

	
	/* positive response */	
	res = dagema_le32toh(reply.result);
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;

} /* dagima_ifc_grouphandle */
#endif

/*	Function: dagima_ifc_get_stats
	Retrieve the IFC statistics.
*/
int32_t 
dagima_ifc_get_stats( int dagfd, cellstats_t cell_cmd, uint32_t ifc )
{
	t_drb_msg_stats_cells cmd;
	t_drb_msg_stats_cells_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_IFC_GET_STATS;
	msg_len = DRB_MSG_IFC_GET_STATS_SIZE;
	cmd.cellcmd  = dagema_htole32(cell_cmd);
	cmd.ID  = dagema_htole32(ifc);
	
	/* send the request */	
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_IFC_GET_STATS_CMPLT || msg_len != DRB_MSG_IFC_GET_STATS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */
	res = dagema_le32toh( reply.result );
	if( res == 0 )
	{
		res = dagema_le32toh( reply.cellcount );
	}
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn (dagfd, 0);
	}
	
	return res;
}

/* ------------------------------------------------------- */

/*	Function  : dagima_group_get_ifcmask
	Return the IFC's that are part of the group specified.
*/
int32_t
dagima_group_get_ifcmask(int dagfd, uint32_t group_handle, uint32_t* ifcmask )
{
	t_drb_msg_group cmd;
	t_drb_msg_group_get_ifcs_cmplt reply;
	int32_t res;
	uint32_t msg_len;
	uint32_t msg_id;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_GROUP_GET_IFCS;
	msg_len = DRB_MSG_GROUP_GET_IFCS_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	
	/* send the request */	
    if (dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0)
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_GROUP_GET_IFCS_CMPLT || msg_len != DRB_MSG_GROUP_GET_IFCS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*ifcmask = dagema_le32toh( reply.ifcmask );
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
 
}

/*	Function: dagima_group_get_state
	Retrieve the state of the group specified.
*/
int32_t
dagima_group_get_state( int dagfd, uint32_t group_handle, state_group_t* pgrpstate  )
{
	t_drb_msg_group cmd;
	t_drb_msg_group_get_state_cmplt reply;
	uint32_t	msg_len;
	uint32_t	msg_id;
	int			res = 0;
	int			libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_GROUP_GET_STATE;
	msg_len = DRB_MSG_GROUP_GET_STATE_SIZE;
	cmd.group_handle = dagema_htole32(group_handle);
	
	/* send the request */	
    if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	if( msg_id != DRB_MSG_GROUP_GET_STATE_CMPLT || msg_len != DRB_MSG_GROUP_GET_STATE_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	*pgrpstate = dagema_le32toh( reply.group_state );
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;

} /* dagima_group_status */

#if 0
int32_t
dagima_group_expectedlinks( int dagfd, int32_t group_handle )
{
	t_drb_msg_group cmd;
	t_drb_msg_group_get_expectedlinks_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int		 res;
	int		 libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_GROUP_EXPECTEDLINKS;
	msg_len = DRB_MSG_GROUP_EXPECTEDLINKS_SIZE;
	cmd.wHandle  = dagema_htole32(group_handle);
	
	/* send the request */	
	if (dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0)
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if (dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0)
	{
		res = -3;
		goto Exit;
	}
	
	if (msg_id != DRB_MSG_GROUP_EXPECTEDLINKS || msg_len != DRB_MSG_GROUP_EXPECTEDLINKS_SIZE)
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh(reply.result);
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn (dagfd, 0);
	
	return res;
}

#endif

/*	Function : dagima_avail_groups
	Return a list of available groups, with their handles.
*/
int32_t
dagima_avail_groups( int dagfd, uint32_t* groupmask )
{
	t_drb_msg_group cmd;
	t_drb_msg_avail_groups_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_AVAIL_GROUPS;
	msg_len = DRB_MSG_AVAIL_GROUPS_SIZE;
	
	/* send the request */	
	if( dagema_send_msg( dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof( reply );
	if( dagema_recv_msg_timeout( dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT ) != 0 )
	{
		res = -3;
		goto Exit;
	}	
	
	if( msg_id != DRB_MSG_AVAIL_GROUPS_CMPLT || msg_len != DRB_MSG_AVAIL_GROUPS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.numgroups );
	(*groupmask) = dagema_le32toh( reply.groupmask );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

/*	Function: dagima_group_get_settings
	Retrieve the settings for the group.
*/
int32_t
dagima_group_get_settings( int dagfd, uint32_t group_handle, dag_group_settings_t* pgroup_settings )
{
	t_drb_msg_group cmd;
	t_drb_msg_group_get_settings_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int		 res = 0;
	int		 libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_GROUP_GET_SETTINGS;
	msg_len = DRB_MSG_GROUP_GET_SETTINGS_SIZE;
	cmd.group_handle = dagema_htole32( group_handle );
	
	/* send the request */	
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
	if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}
	if( msg_id != DRB_MSG_GROUP_GET_SETTINGS_CMPLT || msg_len != DRB_MSG_GROUP_GET_SETTINGS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	pgroup_settings->num_ifcs = dagema_le32toh( reply.numifcs );
	pgroup_settings->frame_len = dagema_le32toh( reply.framelen );

Exit:
	
	/* if the library was opened close it now */
	if( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	if( res != 0 )
	{
		pgroup_settings->frame_len = res;
	}
	
	return res;
}

/* ------------------------------------------------------- */

int32_t
dagima_link_get_ne_alarms( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_ne_alarms_t* palarms )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_get_ne_alarms_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_NE_ALARMS;
	msg_len = DRB_MSG_LINK_GET_NE_ALARMS_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	cmd.linkID = dagema_htole32( linkID );
	
	/* send the request */
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL ) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_LINK_GET_NE_ALARMS_CMPLT || msg_len != DRB_MSG_LINK_GET_NE_ALARMS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	palarms->LIF = dagema_le32toh( reply.LIF );
	palarms->LODS = dagema_le32toh( reply.LODS );
	palarms->RFI_IMA = dagema_le32toh( reply.RFI_IMA );
	palarms->tx_mis_connected = dagema_le32toh( reply.tx_mis_connected );
	palarms->rx_mis_connected = dagema_le32toh( reply.rx_mis_connected );
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return 0;
}

int32_t
dagima_link_get_fe_alarms( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_fe_alarms_t* palarms )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_get_fe_alarms_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_NE_ALARMS;
	msg_len = DRB_MSG_LINK_GET_NE_ALARMS_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	cmd.linkID = dagema_htole32( linkID );
	
	/* send the request */	
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_LINK_GET_NE_ALARMS_CMPLT || msg_len != DRB_MSG_LINK_GET_NE_ALARMS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	palarms->tx_unusable_FE = dagema_le32toh( reply.tx_unusable_FE );
	palarms->rx_unusable_FE = dagema_le32toh( reply.rx_unusable_FE );
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return res;
}

/*	Return failure alarms for various link and group properties */
int32_t
dagima_link_get_ICP_statistics( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_ICP_statistics_t* pstatistics )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_get_ICP_statistics_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_ICP_STATS;
	msg_len = DRB_MSG_LINK_GET_ICP_STATS_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	cmd.linkID = dagema_htole32( linkID );
	
	/* send the request */
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		printf( "FAILEd -3" );
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_LINK_GET_ICP_STATS_CMPLT || msg_len != DRB_MSG_LINK_GET_ICP_STATS_CMPLT_SIZE )
	{
		printf( "FAILEd -7" );
		res = -7;
		goto Exit;
	}
	
	pstatistics->errored_ICP = dagema_le32toh( reply.errored_ICP );
	pstatistics->invalid_ICP = dagema_le32toh( reply.invalid_ICP );
	pstatistics->valid_ICP = dagema_le32toh( reply.valid_ICP );
	pstatistics->missing_ICP = dagema_le32toh( reply.missing_ICP );
	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
		dagema_close_conn( dagfd, 0 );
	
	return res;
}

int32_t
dagima_link_get_ne_defect_statistics( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_ne_defect_statistics_t* pdefects )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_get_ne_defect_statistics_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_NE_DEFECT_STATS;
	msg_len = DRB_MSG_LINK_GET_NE_DEFECT_STATS_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	cmd.linkID = dagema_htole32( linkID );
	
	/* send the request */	
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_LINK_GET_NE_DEFECT_STATS_CMPLT || msg_len != DRB_MSG_LINK_GET_NE_DEFECT_STATS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	pdefects->AIS = dagema_le32toh( reply.AIS );
	pdefects->LCD = dagema_le32toh( reply.LCD );
	pdefects->LIF = dagema_le32toh( reply.LIF );
	pdefects->tx_LODS = dagema_le32toh( reply.tx_LODS );
	pdefects->rx_LODS = dagema_le32toh( reply.rx_LODS );
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return res;
}

int32_t
dagima_link_get_fe_defect_statistics( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_fe_defect_statistics_t* pdefects )
{
	t_drb_msg_link cmd;
	t_drb_msg_link_get_fe_defect_statistics_cmplt reply;
	uint32_t msg_len;
	uint32_t msg_id;
	int      res = 0;
	int      libema_opened;
	
	if(( libema_opened = dagima_open_library( dagfd )) < 0 )
	{
		return libema_opened;
	}
	
	/* populate the message */
	msg_id  = DRB_MSG_LINK_GET_NE_DEFECT_STATS;
	msg_len = DRB_MSG_LINK_GET_NE_DEFECT_STATS_SIZE;
	cmd.group_handle  = dagema_htole32( group_handle );
	cmd.linkID = dagema_htole32( linkID );
	
	/* send the request */	
	if( dagema_send_msg(dagfd, msg_id, msg_len, (uint8_t*)&cmd, NULL) != 0 )
	{
		res = -6;
		goto Exit;
	}
	
	/* read the response */
	msg_len = sizeof(reply);
    if( dagema_recv_msg_timeout(dagfd, &msg_id, &msg_len, (uint8_t*)&reply, NULL, IMA_MSG_TIMEOUT) != 0 )
	{
		res = -3;
		goto Exit;
	}
	
	if( msg_id != DRB_MSG_LINK_GET_NE_DEFECT_STATS_CMPLT || msg_len != DRB_MSG_LINK_GET_NE_DEFECT_STATS_CMPLT_SIZE )
	{
		res = -7;
		goto Exit;
	}
	
	pdefects->Tx_FC_FE = dagema_le32toh( reply.Tx_FC_FE );
	pdefects->Rx_FC_FE = dagema_le32toh( reply.Rx_FC_FE );
	
	/* positive response */	
	res = dagema_le32toh( reply.result );
	
Exit:
	
	/* if the library was opened close it now */
	if ( libema_opened )
	{
		dagema_close_conn( dagfd, 0 );
	}
	
	return res;
}
