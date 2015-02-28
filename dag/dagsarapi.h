/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagsarapi.h,v 1.17.4.2 2009/08/25 22:33:48 wilson.zhu Exp $
*
**********************************************************************/
/** \defgroup SAR Segmentation and Reassembly (SAR) API
 * Interface functions of Segmetation And Reassembly API for AAL 
 * protocols on Endace DAG cards. The SAR API runs on the host 
 * computer and manages the AAL segmentation and reassembly software 
 * running on an embedded processor in a DAG card.\n
 * The AAL reassembler is designed to allow reassembling of AAL5 or 
 * AAL2-SSSAR frames on the card without involving the host in processing. 
 * The reassembler will receive ATM traffic from the lines, this traffic is 
 * then either sent to the host unchanged, dropped or reassembled into AAL5 or 
 * AAL2-SSSAR frames, depending on the configuration used. Different configurations 
 * can be utilized on different virtual connections, allowing only what data is
 * required to be reassembled, other data can be preserved or rejected.\n When 
 * reassembly is used, the frame is constructed from the received data. The frame will
 * end when: The reassembly type is AAL5 and the Payload Type Indication 
 * (PTI) indicates the end of the frame has been received or, the addition 
 * of another ATM cell will cause the buffer allocated to the virtual connection 
 * to overflow. The reassembly type is AAL2-SSSAR and the User to User Indication 
 * (UUI) indicates that the end of the frame has been received or, the addition 
 * of another CPS packet will cause the buffer allocated to the virtual connection 
 * and channel (cid) to overflow. If the frame is AAL5 and has been completed with 
 * a PTI end of message indication the length and CRC contained in the trailer is 
 * checked. On the DAG 3.7T the AAL5 trailer is removed, on the DAG 7.1S the trailer 
 * is kept on the reassembled frame. Errors are indicated in the ERF or Multi-channel 
 * header. If the frame is completed due to a buffer overflow it is assumed the 
 * length is incorrect. The error is indicated and no attempt is made to check or 
 * remove the AAL5 trailer even if the connection is in the AAL5 reassembly mode.
 */
/*@{*/

/**********************************************************************
* FILE:		dagsarapi.h
* DESCRIPTION:	Segmentation and Reassembly library for AAL protocols 
*				on Endace Dag Cards.
*
* HISTORY: 13-6-05 Started for the 3.7t card.
*
**********************************************************************/

#ifndef DAGSARAPI_H
#define DAGSARAPI_H

#ifdef _WIN32
#include <wintypedefs.h>
#else /* !_WIN32 */
#include <inttypes.h>
#endif /* _WIN32 */

#define MAX_VPI_VALUE	0xfff
#define MAX_VCI_VALUE	0xffff

#define MAX_AAL5_SIZE	(64*1024)
#define DAG37T_INTERFACE 0

#if !defined(MAX_DAGS)
#define MAX_DAGS   16
#endif

/**
 * Valid sar mode. sar_error is used to indicate a virtual connection 
 * is in an error mode.
*/
typedef enum  
{
  sar_aal0    = 0,
  sar_aal2    = 1,
  sar_aal5    = 2,
  sar_error   = 3,
  sar_aal0_tx = 4,
  sar_aal2_tx = 5,
  sar_aal5_tx = 6
}sar_mode_t;

/**
 * These modes can be set using the dagsar_set_net_mode() function.
 */
typedef enum 
{
	 nni,
	 uni
}net_mode_t;

/**
 *  available statistics that can be received from the AAL reassembler.
 */
typedef enum 
{
	/**
 	 This is the number of cells not returned to the host, either due 
	 to the cells arriving on a Virtual Connection that is deactivated; 
	 or arriving on unconfigured Virtual Connections while in the Scanning mode
 	*/
	dropped_cells,
	/**
 	 This is the number of cells not returned to the host due to a 
	 filter that has determined the cell should be rejected.
 	*/
	filtered_cells,

	/* Standard statistics for reassembly */
	/**
	 Number of cells received by the embedded processor before any filtering on port 0.
 	*/
	dag71s_stats_rx_pkt_0         = 0x100,
	/**
	 Number of cells received by the embedded processor before any filtering on port 1.
 	*/
	dag71s_stats_rx_pkt_1         = 0x101,
	/**
	 Number of cells received by the embedded processor before any filtering on port 2.
 	*/
	dag71s_stats_rx_pkt_2         = 0x102,
	/**
	 Number of cells received by the embedded processor before any filtering on port 3.
 	*/
	dag71s_stats_rx_pkt_3         = 0x103,
	/**
 	 Number of cells dropped by the bitmask filters applied to the ATM headers on port 0.
 	*/
	dag71s_stats_filter_drop_0    = 0x104,
	/**
 	 Number of cells dropped by the bitmask filters applied to the ATM headers on port 1.
 	*/
	dag71s_stats_filter_drop_1    = 0x105,
	/**
 	 Number of cells dropped by the bitmask filters applied to the ATM headers on port 2.
 	*/
	dag71s_stats_filter_drop_2    = 0x106,
	/**
 	 Number of cells dropped by the bitmask filters applied to the ATM headers on port 3.
 	*/
	dag71s_stats_filter_drop_3    = 0x107,
	/**
 	 Number of cells or frames actually sent to the host on port 0
	*/
	dag71s_stats_to_host_0        = 0x108,
	/**
 	 Number of cells or frames actually sent to the host on port 1
	*/
	dag71s_stats_to_host_1        = 0x109,
	/**
 	 Number of cells or frames actually sent to the host on port 2
	*/
	dag71s_stats_to_host_2        = 0x10A,
	/**
 	 Number of cells or frames actually sent to the host on port 3
	*/
	dag71s_stats_to_host_3        = 0x10B,
	dag71s_stats_hash_collisions  = 0x10C,
	dag71s_stats_datapath_resets  = 0x10D,
	dag71s_stats_cmds_exec        = 0x10E,
	/**
 	 Number of CRC errors on AAL5 frames in port 0
 	*/
	dag71s_stats_crc_error_0      = 0x10F,
	/**
 	 Number of CRC errors on AAL5 frames in port 1
 	*/
	dag71s_stats_crc_error_1      = 0x110,
	/**
 	 Number of CRC errors on AAL5 frames in port 2
 	*/
	dag71s_stats_crc_error_2      = 0x111,
	/**
 	 Number of CRC errors on AAL5 frames in port 3
 	*/
	dag71s_stats_crc_error_3      = 0x112,
	/**
 	 Number of packet losses reported by the firmware. This drop counter will increase when
	 the embedded processor cannot process all the incoming packets. The number of packet
	 losses is not accurate, but gives an idea of the performance of the embedded processor.
	 If this statistic counter is greater than zero, it is recommended to decrease the input
	 bandwidth to the embedded processor. This can be achieved through bitmask filters
	 applied to the ATM headers.
	*/
	dag71s_stats_loss_counter     = 0x113,

	/* ERF record sanity checking statistics */
	dag71s_stats_erf_type_error   = 0x114,
	dag71s_stats_msf_error_bop    = 0x115,
	dag71s_stats_msf_error_sop    = 0x116,
	dag71s_stats_msf_error_eop    = 0x117,
	dag71s_stats_rlen_error       = 0x118,
	dag71s_stats_oversized_record = 0x119,
	dag71s_stats_msf_error_state  = 0x11A,
	dag71s_stats_erf_unaligned    = 0x11B,
	dag71s_stats_erf_snapped      = 0x11C,

	/* number of packets received from the host, per interface */
	dag71s_stats_rx_host_0        = 0x120,
	dag71s_stats_rx_host_1        = 0x121,
	dag71s_stats_rx_host_2        = 0x122,
	dag71s_stats_rx_host_3        = 0x123,

	/* number of packets dropped due to conjestion, per interface */
	dag71s_stats_tx_drop_0        = 0x124,
	dag71s_stats_tx_drop_1        = 0x125,
	dag71s_stats_tx_drop_2        = 0x126,
	dag71s_stats_tx_drop_3        = 0x127,

	/* number of packets transmitted to the line, per interface */
	dag71s_stats_tx_line_0        = 0x128,
	dag71s_stats_tx_line_1        = 0x129,
	dag71s_stats_tx_line_2        = 0x12A,
	dag71s_stats_tx_line_3        = 0x12B

}stats_t;

/**
 * The action which can be taken with a cell which is identified as 
 * fitting the set filter requirements. These actions can be set using 
 * the dagsar_set_filter_bitmask() function.
 */
typedef enum  
{
/**
 * Any ATM cells which are the same as the match value after processing 
 * the bitmask should be accepted. This will involve passing them onto 
 * the Virtual Connections list to determine what reassembly action should 
 * be taken. Any ATM cells which are not the same as the match value after 
 * filter processing will be discarded  immediately
*/
  sar_accept = 0,
/**
 * Any ATM cells which are the same as the match value after processing 
 * the bitmask should be rejected regardless of the Virtual Connection 
 * status of the ATM cell. Any ATM cells which are not the same as the 
 * match value after filter processing will be processed by the reassembler 
 * normally.
 */
  sar_reject = 1

}filter_action_t;

/**
 * The scanning modes which can be set and received using the
 * dagsar_set_scanning_mode() and dagsar_get_scanning_mode() functions. 
 * Although scan_error is defined, it should not be used as a mode 
 * to be set.
 */
typedef enum  
{
  scan_on    = 0,
  scan_off   = 1,
  scan_error = 2
}scanning_mode_t;

/**
 * These modes can be set using the dagsar_set_trailer_strip_mode() function
 */
typedef enum
{
  trailer_on    = 0,
  trailer_off   = 1,
  trailer_error = 2
}trailer_strip_mode_t;

/**
 * These modes can be set using the dagsar_set_atm_forwarding_mode() function
 */
typedef enum
{
  forwarding_off   = 0,
  forwarding_on    = 1,
  forwarding_error = 2
}atm_forwarding_mode_t;

/**
 * The information which will be returned from a call to the
 * dagsar_get_scanned_connection() function. On the DAG3.7T 
 * card the interface is not a valid identifier. This connection 
 * information can then be used to configure or activate a
 * channel. The channel member of the structure is the same 
 * as the connection number given in the Multi-channel ERF header.
 */
typedef struct  
{
  uint32_t iface;
  uint32_t channel;
  uint32_t vpi;
  uint32_t vci;
}connection_info_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** \defgroup AAL SAR functions  
 * Segmentation and Reassembly functions
 *
 */
/*@{*/

/** \name  Active or deacative
 */
/*@{*/
/*****Activation of VC ************************************************/
/**
 * @brief		Sets a virtual connection to return data. This will include
 * 			allocating memory for use in the reassembly of AAL frames.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specified the interface of the virtual connection to 
 * 			be activated. In the case of the DAG3.7T card, this should
 * 			always be zero
 * @param[in]  channel	Specifies the channel of the virtual connection to be activated.
 * 			This is the same as the connection number given in the 
 * 			Multi-channel ERF header. For concatenated network links this
 * 			should always be zero
 * @param[in]  vpi	Specifies the virtual path identifier of the virtual connection
 * 			to be activated
 * @param[in]  vci	Specifies the virtual channel identifier of the virtual connection
 * 			to be activated
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Memory resources not a available (only DAG3.7T)
 * 			- 2 : Virtual connection is currently activated (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 4 : Configuration could not be altered (only DAG3.7T)
 * 			- 5 : Unsupported sar_mode type (only DAG3.7T)
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 8 : Connection number out of range
 * 			- 9 : VPI out of range
 * 			- 10 : VCI out of range
 * 			- 11 : Interface out of range
 * @note		For DAG3.7T:
 * 			The amount of memory allocated will be either the buffer size and an ERF 
 * 			overhead as previously set by dagsar_set_buffer_size() or 64kB. The total 
 * 			amount of memory available for reassembling is 128MiB. Attempting to open
 * 			more connections when the total memory is used will not succeed.\n\n
 * 			For DAG7.1S:
 * 			The maximum amount of simultaneously activated connections is 8192. Attempting 
 * 			to open more connections will not succeed.
 */
uint32_t dagsar_vci_activate(uint32_t dagfd, 
							 uint32_t iface, 
							 uint32_t channel, 
							 uint32_t vpi, 
							 uint32_t vci);

/*****Deactivation of VC ***********************************************/
/**
 * @brief		Sets a virtual connection to not return any data (all
 * 			ATM cells will be discarded). This function will also 
 * 			deallocate any memory associated with the virtual connection. 
 * 			If this connection has previously been in the AAL2 reassembly 
 * 			mode then all channels (CID) will return to the unconfigured 
 * 			state.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specified the interface of the virtual connection to 
 * 			be activated. In the case of the DAG3.7T card, this should
 * 			always be zero
 * @param[in]  channel	Specifies the channel of the virtual connection to be activated.
 * 			This is the same as the connection number given in the 
 * 			Multi-channel ERF header. For concatenated network links this
 * 			should always be zero
 * @param[in]  vpi	Specifies the virtual path identifier of the virtual connection
 * 			to be activated
 * @param[in]  vci	Specifies the virtual channel identifier of the virtual connection
 * 			to be activated
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Memory resources not a available (only DAG3.7T)
 * 			- 2 : Virtual connection is currently activated (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 4 : Configuration could not be altered (only DAG3.7T)
 * 			- 5 : Unsupported sar_mode type (only DAG3.7T)
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 8 : Connection number out of range
 * 			- 9 : VPI out of range
 * 			- 10 : VCI out of range
 * 			- 11 : Interface out of range
 */
uint32_t dagsar_vci_deactivate(uint32_t dagfd, 
							   uint32_t iface, 
							   uint32_t channel, 
							   uint32_t vpi,
							   uint32_t vci);

/*@}*/

/** \name  Active or deacative Channels (CID)
 */
/*@{*/

/*****Activation of CID ************************************************/
 /**
 * @brief		Sets an individual AAL2 channel identification (CID) to
 * 			return data. Before this function will succeed the virtual 
 * 			connection that is described by the arguments channel, vpi 
 * 			and vci must be set to reassemble AAL2 frames. An individual 
 * 			CID may not be activated or deactivated for ATM or AAL5 
 * 			connections. If the virtual connection described by the 
 * 			arguments channel, vpi and vci is not currently active it 
 * 			will be set to active by use of this function. This could 
 * 			result in other, currently unconfigured CIDs to also return
 * 			data if there is data present on the line and CID and they 
 * 			have not been previously  deactivated.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specified the interface of the virtual connection to 
 * 			be activated. In the case of the DAG3.7T card, this should
 * 			always be zero
 * @param[in]  channel	Specifies the channel of the virtual connection to be activated.
 * 			This is the same as the connection number given in the 
 * 			Multi-channel ERF header. For concatenated network links this
 * 			should always be zero
 * @param[in]  vpi	Specifies the virtual path identifier of the CID to be activated
 * @param[in]  vci	Specifies the virtual channel identifier of the CID to be activated.
 * @param[in]  cid	Specifies the channel identifier(CID) to be activated
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Memory resources not a available (only DAG3.7T)
 * 			- 2 : Virtual connection is currently activated (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 4 : Configuration could not be altered (only DAG3.7T)
 * 			- 5 : Unsupported sar_mode type (only DAG3.7T)
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 8 : Connection number out of range
 * 			- 9 : VPI out of range
 * 			- 10 : VCI out of range
 * 			- 11 : Interface out of range
 */
uint32_t dagsar_cid_activate(uint32_t dagfd, 
							 uint32_t iface, 
							 uint32_t channel, 
							 uint32_t vpi, 
							 uint32_t vci,
							 uint32_t cid);

/*****Deactivation of VC ***********************************************/
 /**
 * @brief		Sets an individual AAL2 channel identifier (CID) to
 * 			not return any data. All data on the CID will be discarded. 
 * 			This will affect only the CID on the virtual connection 
 * 			specified by the arguments channel, vpi and vci. This will 
 * 			not cause any other CIDs or virtual connections to discard 
 * 			data.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specified the interface of the virtual connection to 
 * 			be activated. In the case of the DAG3.7T card, this should
 * 			always be zero
 * @param[in]  channel	Specifies the channel of the virtual connection to be activated.
 * 			This is the same as the connection number given in the 
 * 			Multi-channel ERF header. For concatenated network links this
 * 			should always be zero
 * @param[in]  vpi	Specifies the virtual path identifier of the CID to be activated
 * @param[in]  vci	Specifies the virtual channel identifier of the CID to be activated.
 * @param[in]  cid	Specifies the channel identifier(CID) to be activated
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Memory resources not a available (only DAG3.7T)
 * 			- 2 : Virtual connection is currently activated (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 4 : Configuration could not be altered (only DAG3.7T)
 * 			- 5 : Unsupported sar_mode type (only DAG3.7T)
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 8 : Connection number out of range
 * 			- 9 : VPI out of range
 * 			- 10 : VCI out of range
 * 			- 11 : Interface out of range
 */
uint32_t dagsar_cid_deactivate(uint32_t dagfd, 
							   uint32_t iface, 
							   uint32_t channel, 
							   uint32_t vpi,
							   uint32_t vci,
							   uint32_t cid);

/*@}*/

/** \name  Configuration Options
 */
/*@{*/

/***** Setting SAR Mode  ***********************************************/
/**
 * @brief		Allows the mode of operation to be changed on a
 * 			virtual connection. The available modes are ATM, 
 * 			AAL2-SSSAR and AAL5 reassembly.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specified the interface of the virtual connection to 
 * 			be activated. In the case of the DAG3.7T card, this should
 * 			always be zero
 * @param[in]  channel	Specifies the channel of the virtual connection to be activated.
 * 			This is the same as the connection number given in the 
 * 			Multi-channel ERF header. For concatenated network links this
 * 			should always be zero
 * @param[in]  vpi	Specifies the virtual path identifier of the virtual connection
 * @param[in]  vci	Specifies the virtual channel identifier of the virtual connection
 * @param[in]  sar_mode Specifies the mode the virtual connection should be chnaged to.
 * 			Possible value:{sar_aal0, sar_aal2, sar_aal5}
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Memory resources not a available (only DAG3.7T)
 * 			- 2 : Virtual connection is currently activated (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 4 : Configuration could not be altered (only DAG3.7T)
 * 			- 5 : Unsupported sar_mode type (only DAG3.7T)
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 8 : Connection number out of range
 * 			- 9 : VPI out of range
 * 			- 10 : VCI out of range
 * 			- 11 : Interface out of range
 */
uint32_t dagsar_vci_set_sar_mode(uint32_t dagfd,
								 uint32_t iface, 
								 uint32_t channel, 
								 uint32_t vpi,
								 uint32_t vci,
								 sar_mode_t sar_mode);


/***** Get SAR Mode  ***************************************************/
/**
 * @brief		Returns the current mode of operation on a virtual connection.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specified the interface of the virtual connection to 
 * 			be activated. In the case of the DAG3.7T card, this should
 * 			always be zero
 * @param[in]  channel	Specifies the channel of the virtual connection to be activated.
 * 			This is the same as the connection number given in the 
 * 			Multi-channel ERF header. For concatenated network links this
 * 			should always be zero
 * @param[in]  vpi	Specifies the virtual path identifier of the virtual connection
 * @param[in]  vci	Specifies the virtual channel identifier of the virtual connection
 *
 * @return 		Valid sar_mode returned if function was successful, Possible 
 * 			value:{sar_aal0, sar_aal2, sar_aal5}. Invalid sar_mode returned
 * 			if function was unsuccessful.
 *	 		Specific Return Codes:	
 * 			- 3 : Timeout communicating with the Xscale
 * 			- sar_aal0 : Virtual connection is configured to return ATM 
 * 				   cell as they are received
 *			- sar_aal2 : Virtual connection will return AAL2-SSSAR frames
 *				   as they are reassembled
 *			- sar_aal5 : Virtual connection will return AAL5 frames as
 *				   they are reassembled
 *			- sar_error : The Virtual Connection has returned an error state 
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 8 : Connection number out of range
 * 			- 9 : VPI out of range
 * 			- 10 : VCI out of range
 * 			- 11 : Interface out of range
 */
sar_mode_t dagsar_vci_get_sar_mode(uint32_t dagfd, 
								   uint32_t iface, 
								   uint32_t channel,
								   uint32_t vpi,
								   uint32_t vci);

/***** Setting Net Mode  ***********************************************/
/**
 * @brief		Sets the net mode of a virtual connection to user 
 * 			to network interface(UNI) or network to network 
 * 			interface (NNI).
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specified the interface of the virtual connection to 
 * 			be activated. In the case of the DAG3.7T card, this should
 * 			always be zero
 * @param[in]  channel	Specifies the channel of the virtual connection to be activated.
 * 			This is the same as the connection number given in the 
 * 			Multi-channel ERF header. For concatenated network links this
 * 			should always be zero
 * @param[in]  net_mode	Specifies the net mode the virtual connection should be changed
 * 			to. Possible values:{uni, nni}
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 *			- 1 : Memory resources not a available (only DAG3.7T)
 * 			- 2 : Virtual connection is currently activated (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 4 : Configuration could not be altered (only DAG3.7T)
 * 			- 5 : Unsupported sar_mode type (only DAG3.7T)
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 8 : Connection number out of range
 * 			- 9 : VPI out of range
 * 			- 10 : VCI out of range
 * 			- 11 : Interface out of range
 */
uint32_t dagsar_channel_set_net_mode(uint32_t dagfd, 
									 uint32_t iface, 
									 uint32_t channel, 
									 net_mode_t net_mode);


/***** Setting Net Mode per interface ****************************************/

uint32_t dagsar_iface_set_net_mode(uint32_t dagfd, 
								   uint32_t iface, 
								   net_mode_t net_mode);


/*@}*/

/** \name  Set buffer size (DAG3.7T only) 
 */
/*@{*/
/***** Set AAL5 Buffer size ********************************************/
/**
 * @brief		Sets the largest expected size for AAL frames on connections 
 * 			or channels to be activated. This size does not include the 
 * 			extra size required for the ERF header, ATM header, AAL5 Trailer 
 * 			or Padding, when required. By changing this size, virtual connections 
 * 			that have been activated previously will not be altered to have the
 * 			new buffer size. The change will only effect virtual connections or 
 * 			channels that are activated after the buffer size change. To update 
 * 			the buffer size of a previously activated virtual connection or channel, 
 * 			it is necessary to deactivate and re-activate the connection or channel
 * 			after the buffer size has been altered. If a larger AAL frame is received 
 * 			than the size of the buffer, when the buffer is full it will return the 
 * 			first portion of the AAL frame with the length error bit set. The virtual 
 * 			connection or channel will then keep collecting the AAL frame until the 
 * 			current frame is finished, and return the remaining portion, also with 
 * 			the length error bit set.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  size	The size of the largest AAL frame expected on virtual connections
 * 			or channels to be activated
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Buffer size is larger than allowable (64 kilobytes)
 * 			- 2 : Buffer size is less than one ATM cell
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 */
sar_mode_t dagsar_set_buffer_size(uint32_t dagfd, uint32_t size);
/*@}*/

/** \name  Statistics
 */
/*@{*/
/***** Get Statistics *************************************************/
/**
 * @brief		Retrieves the internally held value that corresponds to 
 * 			the requested statistic. The possible statistics currently 
 * 			available for the DAG3.7T are defined by the enumeration 
 * 			stats_t.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  statistic The statistic which should be returned. the currently 
 * 			available options for the statistic are defined in 
 * 			the enumeration stats_t
 *
 * @return 		Any value of the statistic
 *			Specific Return Codes:
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 * @note		For DAG7.1S statistic counters: Statistic counters are 
 * 			unsigned 32-bit integers. When the counter reaches the 
 * 			maximum value (0xFFFFFFFF) it will continue counting from 
 * 			the minimum value (0x00000000).
 *
 */
uint32_t dagsar_get_stats(uint32_t dagfd, stats_t statistic);
/**
 * (DEPRECATED)
 * when used on the DAG3.7T is functionally the same as the 
 * dagsar_get_stats() function, due to the DAG3.7T not having 
 * specified interfaces
 */
uint32_t dagsar_get_interface_stats(uint32_t dagfd, uint32_t iface, stats_t statistic);

/***** Reset Statistics ***********************************************/
/**
 * @brief		Allows a single statistic to be reset to zero without
 * 			affecting any other statistics, which will continue 
 * 			counting from their current position.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  statistic The statistic which should be returned. the currently 
 * 			available options for the statistic are defined in 
 * 			the enumeration stats_t
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 2 : Statistic is not recognised (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 * @note		For DAG7.1S statistic counters: Statistic counters are 
 * 			unsigned 32-bit integers. When the counter reaches the 
 * 			maximum value (0xFFFFFFFF) it will continue counting from 
 * 			the minimum value (0x00000000).
 *
 */
uint32_t dagsar_reset_stats(uint32_t dagfd, stats_t statistic);

/**
 * @brief		Reset all statistics to zero. It is recommended to call this 
 * 			function at the start of a program that will be using the 
 * 			statistics to put all statistics into a known state.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 2 : Statistic is not recognised (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 */
uint32_t dagsar_reset_stats_all(uint32_t dagfd);
/*@}*/

/** \name  Filtering
 */
/*@{*/
/***** Set Filter *****************************************************/
/**
 * @brief		Sets the values of the bitmask, match value and action to be 
 * 			taken for a filter on the card. These values define the filter 
 * 			on the card. Any ATM cells received will have the 32 bits of 
 * 			the ATM header logically AND’d with the bitmask value supplied. 
 * 			The result of this calculation is then compared with the match 
 * 			value, if they are identical, the action defined be filter_action 
 * 			is then taken
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specifies the interface where the bitmask filter is going to be 
 *  			applied. In the case of the DAG3.7T card, this should always be 
 *  			zero
 * @param[in]  bitmask	This is the 32 bit value used by the filter in the mask calculation. 
 * 			This involves performing a logical AND between this value and the 
 * 			ATM header (32 bits not including the HEC)
 * @param[in]  match	The value which the calculation will need to match for the action 
 * 			specified by filter_action to occur. This argument is unused for 
 * 			the DAG3.7T card and should be left at the default
 * @param[in] filter_action The action to be taken when an ATM cell matches the filter value. 
 * 			The contrary to this action is then performed on those ATM cells which 
 * 			do not match the filter value after the bit mask has been applied.
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Filter could not be set (only DAG3.7T)
 * 			- 2 : Statistic is not recognised (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 */
uint32_t  dagsar_set_filter_bitmask (uint32_t dagfd, 
									 uint32_t iface, 
									 uint32_t bitmask, 
									 uint32_t match, 
									 filter_action_t filter_action);

/***** Reset Filter ****************************************************/
/**
 * @brief		Resets the filter on the board so that no further
 * 			cells are rejected based on the previous filter values.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in]  iface	Specifies the interface where the bitmask filter is going to be 
 *  			applied. In the case of the DAG3.7T card, this should always be 
 *  			zero
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Filter could not be set (only DAG3.7T)
 * 			- 2 : Statistic is not recognised (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 */
uint32_t dagsar_reset_filter_bitmask (uint32_t dagfd, uint32_t iface);
/*@}*/

/** \name  Scanning
 */
/*@{*/

/***** Scanning Modes **************************************************/
/**
 * @brief		Initializes the internal set of scanned entries, so
 * 			that scanning can be performed correctly. This function 
 * 			will remove any current entries and set the number of 
 * 			scanned connections to zero.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 *  			zero
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Filter could not be set (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 * @note		This function must be called prior to setting the scanning mode 
 * 			to on. Also the scanning mode must be off for the structure to 
 * 			be initialized. On the DAG3.7T an error will result if this function 
 * 			is called while scanning mode is set to on. On the DAG7.1S no action 
 * 			will be performed on that case. This function should also be called 
 * 			after scanning has been completed and scanned connections have been 
 * 			processed to free internal memory.
 */
uint32_t dagsar_init_scanning_mode (uint32_t dagfd);

/**
 * @brief		Allows the scanning mode to be turned on and off. During the scanning 
 * 			mode the system will gather information on the available connections 
 * 			which have data available to them and are currently unconfigured or
 * 			deactivated.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in] scan_mode The scanning mode to be set
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Memory resources not a available (only DAG3.7T)
 * 			- 2 : Virtual connection is currently activated (only DAG3.7T)
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 4 : Configuration could not be altered (only DAG3.7T)
 * 			- 5 : Unsupported sar_mode type (only DAG3.7T)
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 * @note		Before turning the scanning mode on the dagsar_init_scanning_mode() 
 * 			function must be called. When scanning mode is set to on, no other 
 * 			scanning related functions can be called. Scanning places an extra 
 * 			load on the system which can have a detrimental effect on the performance 
 * 			of the AAL reassembler. For this reason it is not recommended to use the
 * 			scanning mode whilst in normal operation. scan_error is not a valid option 
 * 			for the card to be set to.\nn
 * 			After scanning mode has been initialized, turned on and then turned 
 * 			off the scanned connections can be received using the 
 * 			dagsar_get_scanned_connections_number() and
 * 			dagsar_get_scanned_connection() functions.
 *
 */
uint32_t dagsar_set_scanning_mode (uint32_t dagfd, scanning_mode_t scan_mode);

/**
 * @brief		Returns the current status of the scanning mode as set 
 * 			with the dagsar_set_scanning_mode() function.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 *
 * @return		The return enumeration scanning_mode_t.
 * 			Possible value: 
 * 			- scan_on	Scanning mode is on
 * 			- scan_off	Scanning mode is off
 * 			- scan_error	Scanning mode is undifined
 * 			- Invalid	Function was unsuccessful\n 
 *	 		
 *	 		Specific Return Codes:	
 * 			- 3 : Timeout communicating with the Xscale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 *
 */
scanning_mode_t dagsar_get_scanning_mode (uint32_t dagfd);

/***** Scanned Connections *********************************************/
/**
 * @brief		Returns the number of connections that the host has information 
 * 			about after scanning. Theses connections can then be accessed 
 * 			using the function dagsar_get_scanned_connection().
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 *
 * @return 		The number of scanned connections available
 *			Specific Return Codes:
 * 			- 1 : Scanning mode is set to on
 *
 * @note		Scanned connections are only available after scanning has 
 * 			been performed and prior to the dagsar_init_scanning_mode() 
 * 			function being called.
 *
 */
uint32_t dagsar_get_scanned_connections_number (uint32_t dagfd);

/**
 * @brief		Allows information about a connection which was identified during 
 * 			scanning to be retrieved. Connection information is indexed
 * 			from zero to one less than the value returned by 
 * 			the dagsar_get_scanned_connections() function. Connection 
 * 			information returned includes the connection number, VCI, VPI and
 * 			the Interface number. These identifiers can then be used to configure 
 * 			the connection.
 *
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in] connection_number	The index of the connection information to be retrieved
 * @param[in] pConnection_info	Pointer to the structure that will contain the connection 
 * 				information after a successful call to the function. This 
 * 				pointer must already be pointing to a valid connection_info 
 * 				structure, and any memory used must be maintained and 
 * 				deallocated by the user.
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 1 : Scanning mode is set to on
 * 			- 2 : Connection_number requested is out of range
 * 			- 3 : Error attempting to access able entry (only DAG3.7T)
 * 			- 4 : Connection info pointer was not initialized (only DAG7.1S)
 *
 * @note		Scanned connections are only available after scanning has been 
 * 			performed and prior to the dagsar_init_scanning_mode() function 
 * 			being called.
 *
 */
uint32_t dagsar_get_scanned_connection(uint32_t dagfd, 
					uint32_t connection_number, 
					connection_info_t *pConnection_info);

/*@}*/

/** \name  Trailer Strip (DAG7.1S only)
 */
/*@{*/
/***** AAL5 Trailer Strip Mode *************************************************/
/**
 * ONLY DAG7.1S
 * @brief		Sets the current AAL5 trailer stripping mode, by default the trailer 
 * 			and padding of AAL5 frames are removed by the reassembler prior to
 * 			being presented to the host. Using this function the behavior can be 
 * 			changed to the preserve the padding bytes and the trailer on the end 
 * 			of the reassembled frame.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in] strip_mode The trailer strip mode to set, possible values are trailer_on or
 *            		 strip_mode trailer_off. If trailer_on is specified then the AAL5 
 *            		 trailer and padding bytes remain on the reassembled frames. 
 *            		 If trailer_off is specified then the padding and trailer bytes 
 *            		 are stripped from the reassembled frame, this is the default.
 * 
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 3 : Timeout communicating with the XScale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 15 : Communication timed-out when connecting to the packet 
 * 			       processing engines
 * 			-18 : A generic error occurred inside the packet processing 
 * 			      engines
 *
 */
uint32_t dagsar_set_trailer_strip_mode (uint32_t dagfd, trailer_strip_mode_t strip_mode);

/**
 * ONLY DAG7.1S
 * @brief		Retrieves the current AAL5 trailer strip mode
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * 
 * @return		The return enumeration trailer_strip_mode_t.
 * 			Possible value: 
 * 			- trailer_on	The trailer and padding bytes remain on the 
 * 					reassembled AAL5 frames
 * 			- trailer_off	The trailer and padding bytes are stripped 
 * 					from the reassembled AAL5 frames
 * 			- trailer_error	A generic error occured while trying to 
 * 					retrieve the current AAL5 trailer strip
 * 					mode
 */
trailer_strip_mode_t dagsar_get_trailer_strip_mode (uint32_t dagfd);

/*@}*/

/** \name  ATM forwarding (DAG7.1S only)
 */
/*@{*/

/***** ATM Forwarding Mode *************************************************/
/**
 * ONLY DAG7.1S
 * @brief		Sets the circuit to forwarding ATM cell mode, by default the 
 * 			unconfigured circuit forwarding feature is disabled.Using this 
 * 			function the behavior of IXP enable the forwarding unconfigured
 * 			circuits(VPI/VCI/Interface) to the host.
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * @param[in] forwarding_mode The atm forwarding mode to set, the possible value are
 * 			      forwarding_off or forwarding_on. If forwarding_on is specified
 * 			      then atm cell from line are forwarded onto the host as if they
 * 			      were a configured AAL0 circuit. If forwarding_off is specified 
 * 			      then atm cells from the line are not forward onto the host. 
 *
 * @return 		0 is returned to indicate success, -1 is returned to indicate
 *			an error. Use dagdsm_get_last_error() to retrieve the error code
 *			Specific Return Codes:
 * 			- 3 : Timeout communicating with the XScale
 * 			- 6 : Message not transmitted
 * 			- 7 : Message not responded to correctly
 * 			- 15 : Communication timed-out when connecting to the packet 
 * 			       processing engines
 * 			-18 : A generic error occurred inside the packet processing 
 * 			      engines
 */
uint32_t dagsar_set_atm_forwarding_mode (uint32_t dagfd, atm_forwarding_mode_t forwarding_mode);

/**
 * ONLY DAG7.1S
 * @brief		Retrieves the current atm forwarding mode
 *
 * @param[in]  dagfd	The file descriptor for the DAG card as returned from
 * 			dag_open(). This card also should have been initialized 
 * 			via dagema_open_conn()
 * 
 * @return		The return enumeration atm_forwarding_mode_t.
 * 			Possible value: 
 * 			- forwarding_on	The atm cells from line are forwarded onto
 * 					the host if they were a configured AAL0 
 * 					circuit
 * 			- trailer_off	The atm cells from line are not forwarded 
 * 					onto the host.
 * 			- trailer_error	A generic error occured while trying to 
 * 					retrieve the current atm forwarding mode
 */
atm_forwarding_mode_t dagsar_get_atm_forwarding_mode (uint32_t dagfd);

/*@}*/
/*@}*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DAGSARAPI_H_ */

/*@}*/
