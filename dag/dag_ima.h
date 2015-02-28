
/*
 * Copyright (c) 2004-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_ima.h,v 1.6.64.1 2009/08/14 01:56:49 wilson.zhu Exp $
 */
/** \defgroup IMA Inverse Multiplexing ATM (IMA) API
 * Interface functions of Inverse Multiplexing ATM for wire-tap or monitoring
 * on Endace DAG cards. It is therefore not intended to be used to terminate 
 * an IMA connection. However this will be corrected in a future release.\n
 * The IMA implementation is based on the IMA standard, AF-PHY-0086.001. 
 * Where “Section” references are used in this Programming Guide they refer 
 * to sections within that standard.\n This is list of the functionality of the 
 * IMA Host API used on the DAG 3.7T card.
 */
/*@{*/
#ifndef DAG_IMA_CONFIG_API_H
#define DAG_IMA_CONFIG_API_H "dag_ima.h"

//#include "global.h"
//#include "d37t_msg.h"
#include "dagapi.h"
#include "d37t_i2c.h"

#define DRB_CLASS_IMA					0x10020000

/* ------------------------------------------------------- */

#define INVALID_TRANSITION				0xCEEDBABA

#define MAX_GROUPS					16
#define MAX_IMA_LINKS					32

#ifndef MAX_INTERFACE
	#define MAX_INTERFACE				16		/**< num physical interfaces */
#endif

/*	Group */
#define DEF_FRAMESIZE					128
#define DEF_GROUPID					0xBEEFCAAC
#define DEF_GROUPHANDLE					0xABBABABA

/*	Link */
#define DEFAULT_LINK_ID					255		/**< Outside the maximum link ID value of 31 */
#define DEFAULT_FRAMELEN 				128

/*	IESM */
#define LIF_DEFECT_TRANSITION		 		2 		/**< Number of IMA frames to wait until we move back to the Working state */

/*	LSM */
#define INVALID_FRAMEPOS				-1
#define INVALID_FRAMELEN				-1
#define INVALID_CLOCK					255

/*	SCCI field */
#define INVALID_SCCI					-1

#define FRAME_SQN_LENGTH				256

/*	Rx LSM - Error bit positions */
#define RX_ERROR_BTM_LOD				0x00000008 	/**< BTM LOD defect */
#define RX_ERROR_TOP_LOD				0x00000004 	/**< TOP LOD defect */
#define RX_ERROR_LIF					0x00000002 	/**< LIF defect */
#define RX_ERROR_PHY					0x00000001 	/**< Physical line defect */
#define RX_ERROR_DUMMY					0x00000000 	/**< No defect */

/*	TX LSM - Inhibit bit positions */
#define TX_INHIBIT_GROUP				128		/**< 0x00000080 */
#define TX_INHIBIT_SUFFICIENT_LINKS			64		/**< 0x00000040  Inhibited by the group, waiting for sufficient links */
#define TX_INHIBIT_RESTRAIN				16		/**< 0x00000010 */
#define TX_INHIBIT_ICP					8		/**< 0x00000008 Inhibit until we have a new ICP cell */
#define TX_INHIBIT_DUMMY				0		/**< 0x00000000 */

/*	TX LSM - Error bit positions */
#define TX_ERROR_LOD					0x00000001
#define TX_ERROR_DUMMY					0x00000000

/*	Link inhibit positions */
#define LINK_INHIBIT_ICP				8		/**< 0x00000008  Initialize the link with the ICP's ID */

/*	Group LSM - Inhibit Group positions */
#define GRP_INHIBIT_TX_IFCEMPTY 			0x00040000	/**< One of the link's Tx buffers is empty */
#define GRP_INHIBIT_RX_IFCEMPTY				0x00020000 	/**< Set by the group, when attempting to multiplex a group with an  empty IFC */

#define GRP_INHIBIT_MASK				0x0000ffff
#define GRP_INHIBIT_ICP					16		/**<	0x00000010 */
#define GRP_INHIBIT_FE_REJECT				8		/**<	0x00000008 FE group parameter rejection */
#define GRP_INHIBIT_DUMMY				0		/**<	0x00000000 */

/* ------------------------------------------------------- */

typedef enum cellstats_e
{
	DAG_IMA_ALL_CELLS = 0,		/**< Return cumulative statistics for all cell types */
	DAG_IMA_DATA_CELLS,		/**< Return statistics for IMA cells */
	DAG_IMA_IDLE_CELLS,		/**< Return statistics for OAM idle cells */
	DAG_IMA_ICP_CELLS		/**< Return statistics for OAM ICP cells */
} cellstats_t;

typedef enum symconfig_mode_e
{	OPCONFIG_SYM = 0,
	OP_ASYM_CONFIG_SYM = 1,
	OPCONFIG_ASYM = 2,
	OPCONFIG_RESERVED = 3
} symconfig_mode_t;

/*	Group Traffic State machine is defined in Table 14, pg 61 */
typedef enum gtsm_state_e {
	GTSM_UP,
	GTSM_DOWN
} gtsm_state_t;

/* ---------------------------------------------------------*/
/*	IFSM 	
 */
typedef enum ifsm_state_e {
	IFSM_HUNT,				/**< 	We are looking for a ICP cell, cell-by-cell */
	IFSM_PRESYNC,				/**<	We have found an initial ICP cell, waiting for gamma consecutive valid ICP cells */
	IFSM_SYNC,				/**< 	We have located the frame */
} ifsm_state_t;

/* ---------------------------------------------------------*/
/*	IESM 													
 */
typedef enum iesm_state_e {
	IESM_WORKING = 0,			/**<	In the working state */
	IESM_OIF,				/**< 	Out of IMA Frame state, we enter this when we first enter the IMA frame */
	IESM_LIF				/**< 	Loss if IMA Frame deleniation, this is entered when the OIF state persists for some gamma + 2 time */
} iesm_state_t;

/* ---------------------------------------------------------*/

/*	State transitions for the RX LSM */
typedef enum state_rx_e {
	RX_INVALID = -3,
	RX_UNASSIGNED = -2,			/**< unassigned - substate */
	RX_DELETED = -1,			/**< deleted - substate */
	RX_UNUSABLE_1 = 1,			/**< unusable - no reason given */
	RX_UNUSABLE_2 = 2,			/**< unusable - fault */
	RX_UNUSABLE_3 = 3,			/**< unusable - failed */
	RX_UNUSABLE_4 = 4,			/**< unusable - inhibited	*/
	RX_UNUSABLE_5 = 5,			/**< unusable - misconnected */
	RX_USABLE = 6,				/**< usable */
	RX_ACTIVE = 7,				/**< active */
	RX_BLOCKING = 8				/**< blocking - substate of NOT IN GROUP */
} state_rx_t, *pstate_rx_t;

typedef enum state_tx_e {
	TX_INVALID = -3,
	TX_UNASSIGNED = -2,
	TX_DELETED = -1,
	TX_UNUSABLE_1 = 1,			/**< No reason given */
	TX_UNUSABLE_2 = 2,			/**< Fault ( vender specific ) */
	TX_UNUSABLE_3 = 3,			/**< Mis-connected */
	TX_UNUSABLE_4 = 4,			/**< Inhibited ( vendor specific ) */
	TX_UNUSABLE_5 = 5,			/**< Failed ( not currently defined ) */
	TX_USABLE = 6,
	TX_ACTIVE = 7,
} state_tx_t, *pstate_tx_t;

/* ------------------------------------------------------- */
/**
 * structure of available group
 */
typedef struct
{
    uint32_t num_groups;			/**< number of available groups */
    uint32_t group_handles[MAX_INTERFACES];	/**< handle for each avail group */
} dag_avail_groups_t;

/**
 * structure of group setting 
 */
typedef struct
{
    uint32_t num_ifcs;				/**< number of ifcs in IMA group */
    uint32_t frame_len;				/**< IMA frame length for the group */
} dag_group_settings_t;

/**
 * structure of group RX links
 */
typedef struct
{
	uint32_t num_rxlinks;
	int32_t linkids[MAX_IMA_LINKS];
} dag_group_rxlinks_t;

/**
 * structure of NE link statistics
 */
typedef struct
{
	uint32_t errored_ICP;			/**< Cell with a HEC or CRC error at expected ICP cell position if it is not a Missing cell. */
	uint32_t invalid_ICP;			/**< Cell with good HEC & CRC and CID = ICP at expected frame position with another unexpected field, refer to pg 69 */
	uint32_t valid_ICP;			/**< Cell with the correct fields */
	uint32_t missing_ICP;			/**< Cell which is not where it is supposed to be */
} dag_link_ICP_statistics_t;

/**
 * structure of RX/TX defect statistics
 */
typedef struct
{
	uint32_t AIS;				/**< Rx, Interface Specific Transmission Convergence sub-layer defect */
	uint32_t LCD;				/**< RX, Number of Loss of cell deleniation events */
	uint32_t LIF;				/**< Rx, Number of Loss of IMA frame events */
	uint32_t tx_LODS;			/**< Tx, Number of Link out of delay synchronization defects */
	uint32_t rx_LODS;			/**< Rx, Number of Link out of delay synchronization defects */
} dag_link_ne_defect_statistics_t;

/**
 * structure of FE defect statistics
 */
typedef struct
{
	uint32_t Tx_FC_FE;			/**< FE Tx link failure count */
	uint32_t Rx_FC_FE;			/**< FE Rx link failure count */
} dag_link_fe_defect_statistics_t;

/**
 * structure of RX/TX link alarms
 */
typedef struct
{
	uint32_t LIF;				/**< Rx, Loss of IMA frame */
	uint32_t LODS;				/**< Rx, Loss of deleniation */
	uint32_t RFI_IMA;			/**< Rx, Remove Failure Indicator delected at FE */
	uint8_t tx_mis_connected;		/**< (R-141) Tx link is detected to be misconnected */
	uint8_t rx_mis_connected;		/**< (R-142) Rx link is detected to be misconnected */
} dag_link_ne_alarms_t;

/**
 * structure of RX/TX FE link alarms
 */
typedef struct
{
	uint8_t tx_unusable_FE;			/**< (R-143) Fe reports Tx-Unusable */
	uint8_t rx_unusable_FE;			/**< (R-144) Fe reports Rx-Unusable */
} dag_link_fe_alarms_t;

/**
 * structure of ICP control
 */
typedef struct 
{
	uint8_t icp_drop;			/**< Drop the ICP cells, set to 1 by default*/
	uint8_t icp_send_trl;			/**< Send the ICP cells from the TRL which have changed.
							set to 0 by default, if set to 1, and icp_drop
							is set to 1, ICP cells from the TRL will be received */
	uint8_t filler_drop;			/**< Drop the filler cells set to 1 by default*/
} dag_um_icpctrl;


/** \defgroup gropIMA IMA functions  
 * Inverse Multiplexing ATM (IMA) functions
 */
/*@{*/

/** \name ATM RX module
 */
/*@{*/
typedef enum atmrx_mode_e {

	ATMRX_AUTO = 0,  /**<  Configure Rx groups by checking for ICP cells */

	ATMRX_AUTOBEST = 1, /**< Configure Rx groups by checking for ICP cells 
				 This mode is similar to the ATMRX_AUTO, but divides
				 IFCs into a logical grouping of 8 top and 8 bottom 
				 IFCs, defined by Pod layout, It allows the auto 
				 configuration of 2 groups with the same IMA ID. 
				 This can be obtained by using IFCs 0-7 for 
				 Group 1(ID:0) and IFCs 8-15 for Group 2(ID:0). */

	ATMRX_MANUAL = 2, /**<  Groups must constructed explicitly by the user 
			 	using the appropriate functions, ATM cells will 
				be dropped, while in this mode, if there are no
				groups configured on the appropriate IFCs.  */

	ATMRX_FREEFLOW = 4, /**<  Packets will be forwarded to the host */
} atmrx_mode_t;

/**	
 * @brief		Set the configuration mode of the IMA Multiplexer. We 
 * 			specify the mode by. The mode determines how the IMA 
 * 			will handle incoming ATM cells
 *
 * @param[in]  dagfd	handle of the dag card.
 * @param[in]  mode	mode to set
 *
 * @return		0 : success, 1 : failure
 */
int32_t dagima_atmrx_set_mode( int dagfd, atmrx_mode_t mode );

/**	
 * @brief		Retrieve the ATM RX mode
 *
 * @param[in]  dagfd	handle of the dag card.
 *
 * @return		ATM Rx mode.
 */
atmrx_mode_t dagima_atmrx_get_mode( int dagfd );

/**
 * @brief		Return the ATM Rx statistics. These are global
 *			statistics applicable over all the groups/free
 *			IFCs.
 * @param[in]  dagfd	dag file descriptor
 * @param[in] cell_stats which class of cell statistics to return
 *
 * @return		 >=0 - success, statistics for the requested class
 * 			<0 - failure.
 */
int32_t dagima_atmrx_get_stats( int dagfd, cellstats_t cell_stats );
/*@}*/

/** \name IFC module
 */
/*@{*/
typedef enum ifc_mode_e {
	IFC_ERROR = -1,
	IFC_AUTO = 0,		/**< Set IFC simple auto-configure */
	IFC_AUTOBEST = 1,	/**< Set the IFC to auto-configure using IFC top and bottom separation */
	IFC_MANUAL = 2,		/**< Set the IFC */
	IFC_FREEFLOW = 3,	/**< Set the IFC to let ATM cells through */
	IFC_GROUP = 4
} ifc_mode_t;

/**
 * @brief		Set the configuration mode of an individual IFC.
 *
 * @param[in] dagfd 	dag file descriptor
 * @param[in] ifc	interface physical ID
 * @param[in] mode 	mode to set
 *
 * @return		0 - success, -1 failure.
 *
 * @note		You cannot change the mode of an IFC if it is set to 
 * 			IFC_GROUP. The only way to change from IFC_GROUP is to 
 * 			either change the ATM Rx mode, which releases all groups, 
 * 			or release the group holding the IFC explicitly
 *
*/
int32_t dagima_ifc_set_mode( int dagfd, uint32_t ifc, ifc_mode_t mode );

/**
 * @brief		Retreives the current mode of the IFC
 *
 * @param[in] dagfd 	dag file descriptor
 * @param[in] ifc	IFC to retrieve the mode from
 *
 * @return		0 - success, -1 failure.
 */
int32_t dagima_ifc_get_mode( int dagfd, uint32_t ifc );

/**
 * @brief		Return statistics for a particular IFC.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] cell_stats which class of cell statistics to return
 * @param[in] ifc	interface physical ID.
 *
 * @return		>=0 success, statistics/count for the requested class 
 * 			<0 - failure.
*/
int32_t dagima_ifc_get_stats( int dagfd, cellstats_t cell_stats, uint32_t ifc );
/*@}*/

/** \name Group module
 */
/*@{*/

/**
 * @brief		Add a Rx IMA group to the Multiplexer. The missing group
 * 			properties will be supplied by ICP cells.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] ifc_mask	bit mask of the physical interfaces to be included in the group.
 *
 * @return		0 - success, <0 - failure.
 *
 * @note		If the IFC specified by the IFC mask is in use by another group 
 * 			then this function will fail.
*/
int32_t dagima_add_rx_group( int dagfd, uint32_t ifc_mask );

/*
uint32_t dagima_add_txrx_group( uint32_t ID,
								uint32_t txlinks,
								uint32_t rxlinks,
								uint32_t framelen,
								symconfig_mode_t symconfig,
								uint8_t 
								uint32_t ifcmask );
*/

/**
 * @brief		Remove an IMA group given its handle.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of group to remove
 *
 * @return		0 - success, <0 - failure.
 *
 * @note		Fails if the group specified does not exist.
*/
int32_t dagima_remove_group( int dagfd, uint32_t group_handle );

/**
 * @brief		Given the groups handle, return the interfaces 
 * 			associated with that group.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of group to remove
 * @param[out] ifcmask	a initialized 32 bit mask with bit position
 * 			corresponding to physical IFC ID.
 *
 * @return		0 - success, <0 - failure.
*/
int32_t dagima_group_get_ifcmask( int dagfd, uint32_t group_handle, uint32_t* ifcmask );

/*	Group states are defined in Table 11, pg 57 */
typedef enum state_group_e {
	GRP_INVALID = -2,
	GRP_NOT_CONFIGURED = -1,	/**< Group is not configured. This state is 
						provided for completeness. In IMA Mux
						a group that is not configured no longer
						exists. */

	GRP_START_UP = 0,	/**< Group has been configured and is checking FE group parameters */
	GRP_START_UP_ACK = 1,	/**< Group is waiting for GRP_START_UP_ACK from FE to move to 
					GPR_INSUFFICIENT_LINKS */
	GRP_CONFIG_ABORTED_UNSUPPORTED_M = 2,	
	GRP_CONFIG_ABORTED_INCOMPAT_SYM = 3,	/**< Incompatible operation and configuration between 
							the two groups. Refer to the IMA standard for 
							more information */
	GRP_CONFIG_ABORTED_UNSUPPORTED_VERSION = 4,	/**< Unsupported IMA version provided by the FE */
	GRP_CONFIG_ABORTED_ABORTED_1 = 5,	/**< User specific 1 */
	GRP_CONFIG_ABORTED_ABORTED_2 = 6,	/**< User specific 2 */
	GRP_CONFIG_ABORTED_ABORTED_3 = 7,	/**< User specific 3 */
	GRP_INSUFFICIENT_LINKS = 8,	/**< NE group is waiting for some x number of Rx/Tx 
						links to become ACTIVE before moving to GRP_OPERATIONAL */
	GRP_BLOCKED = 9,	/**< The group has been blocked from moving to either the
					GRP_INSUFFICIENT_LINKS or GRP_OPERATIONAL state by the 
					some internal inhibit. */
	GRP_OPERATIONAL = 10	/**< Group is in GPR_OPERATIONAL and is sending ATM cells to the host */
} state_group_t, *pstate_group_t;

/**
 * @brief		Get the current state of the group.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of group to get status of
 * @param[out] pgrpstate state of the group specified.
 *
 * @return		0 - success, <0 - failure.
 *
 * @note		Fails if the group specified does not exist. 
 * 			We distinguish between the two groups at either
 * 			end of a IMA link by using the standard naming 
 * 			convention, FE for the furthest end, i.e the group 
 * 			we are talking to, and NE for the nearest end. 
 * 			More information about group transitions can be 
 * 			found in Section 10.1 Group Operation.
*/
int dagima_group_get_state( int dagfd, uint32_t group_handle, pstate_group_t pgrpstate );

/**
 * @brief		Get the settings of a group
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of group to get settings of
 * @param[out] pgroup_settings	selected seting of the group
 *
 * @return		0 - success, <0 - failure.
*/
int32_t dagima_group_get_settings( int dagfd, uint32_t group_handle, dag_group_settings_t* pgroup_settings );

/**
 * @brief		Returns a 32 bit mask, with each bit indicating the 
 * 			Rx ID of the link in the group, depending on its position 
 * 			within the mask. For example, the link mask 0x03 would 
 * 			correspond to links, with IDs 0 and 1. Step through the 
 * 			mask using a bit mask to easily compute the link IDs.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group to get the settings of
 * @param[out] rxlinkidmask pointer to a 32bit value set by the callee
 *
 * @return		0 - success, <0 - failure
 *
 */
int32_t dagima_group_get_rx_linkidmask( int dagfd, uint32_t group_handle, uint32_t* rxlinkidmask );

/**
 * @brief		Get a list of handles for each available group, each 
 * 			entry in the group_handles array corresponds to a valid 
 * 			entry, which is a +ve number, or a invalid handle
 * 			which is set to -1.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] groupmask	an initialized mask, each corresponding bit position 
 * 			corresponding to a group handle.
 *
 * @return		Returns: >0 - success, number of groups.
 * 			<0 - failure.
 *
 * @note		The bit mask can be interpreted in the following way:
 * 			for( y = 0; y < MAX_GROUPS; y++ )
 * 			{
 *       			if(( groupmask >> y ) & 1 )
 *             			{
 *                          		printf( “Group Handle: %d\n”, y );
 *                              }
 *                      }
 *                      The constant MAX_GROUPS is defined in the dag_ima.h file
 *
*/
int32_t dagima_avail_groups( int dagfd, uint32_t* groupmask );

int32_t dagima_group_expectedlinks( int dagfd, uint32_t group_handle );

/**
 * (DEPRECIATED)
 * @brief		Retrieve the handle of the group attached to the IFC specified. 
 * 			NOT IMPLEMENTED.
 *
 * @param[in]	dagfd	handle of the dag card.
 * @param[in]	ifc	interface to query.
 *
 * @return		0 - success, <0 - failure
*/
int32_t dagima_ifc_get_grouphandle( int dagfd, uint32_t ifc );
/*@}*/

/** \name UM control													
 */
/*@{*/

/**
 * @brief		Set the UM ICP control flags.
 *
 * @param[in]  dagfd	dag file descriptor
 * @param[in] icp_drop 	drop all ICP cells ( enabled by default, 1 )
 * @param[in] icp_send_trl let ICP cells on the TRL through. ( disabled by default, 0 )
 * @param[in] filler_drop drop all filler cells ( enabled by default, 1 )
 *
 * @return		0 - success, 1 - failure
 *
 * @note		This is useful if the host wishes to independently analyze ICP cells.
*/
int32_t
dagima_um_set_icpctrl( int dagfd, 
					   uint32_t icp_drop,
					   uint32_t icp_send_trl,
					   uint32_t filler_drop  );

/**
 * @brief 		Return the state of the ICP control flags.
 *
 * @param[in]  dagfd	dag file descriptor
 * @param[out] picpctrl reference to a ICP ctrl structure to populate.
 *
 * @return 		0 - success, 1 - failure.
*/
int32_t dagima_um_get_icpctrl( int dagfd, dag_um_icpctrl *picpctrl );
/*@}*/


/** \name Link control functions.									
 */
/*@{*/

/**
 * @brief		Retrieve the current link Rx LSM state.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group 
 * @param[in] linkID	ID of the link
 * @param[out] pstate_tx state of the rx LSM
 *
 * @return		0 - success, 1 - failure
*/
int32_t dagima_link_get_rxlsm_state( int dagfd, uint32_t group_handle, uint32_t linkID, state_rx_t *pstate_rx );

/**
 * @brief		Retrieve the current link Tx LSM state.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group the link is a part of.
 * @param[in] linkID	ID of the link.
 * @param[out] pstate_tx Tx LSM state.
 *
 * @return		0 - success, <0 - failure
*/
int32_t dagima_link_get_txlsm_state( int dagfd, uint32_t group_handle, uint32_t linkID, state_tx_t *pstate_tx );

/**
 * @brief		Retrieve the state of the Rx link's IFSM.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group the link is a part of.
 * @param[in] linkID 	ID of the link
 * @param[out] pifsm_state Rx IFSM state.
 *
 * @return		0 - success, <0 - failure
*/
int32_t dagima_link_get_ifsm_state( int dagfd, uint32_t group_handle, uint32_t linkID, ifsm_state_t* pifsm_state ); 

/**
 * @brief		Returns the current state of the Rx IESM.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group the link is a part of
 * @param[in] linkID	ID of the link
 * @param[out] piesm_state Rx IESM state
 *
 * @return		 0 - success, <0 - failure
*/
int32_t dagima_link_get_iesm_state( int dagfd, uint32_t group_handle, uint32_t linkID, iesm_state_t* piesm_state );

/**
 * @brief		Provides a quick way to determine if there was a OIF event, 
 * 			i.e. the link lost SYNCH from the last time this was queried. 
 * 			Note, querying the state of the OIF toggle clears the state. 
 * 			Querying it again will allow you to determine if a OIF event 
 * 			has occurred since the last time. Note that you can also 
 * 			determine if a OIF event has occurred by quering the links 
 * 			NE defect statistics.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group the link is a part of
 * @param[in] linkID	ID of the link
 *
 * @return		1 - OIF, 0 - No OIF, <0 - failure
 *
 * @note		The OIF toggle is a convenience function to set the Provides 
 * 			a quick way to determine if there was an OIF event, i.e. the 
 * 			link lost SYNCH from the last time this was queried.
 *			Querying the state of the OIF toggle clears the state.\n
 *			Querying it again will allow you to determine if a OIF event 
 *			has occurred since the last time. You can also determine if 
 *			an OIF event has occurred by querying the links NE defect
 *			statistics.
 *
*/
int32_t dagima_link_get_OIF_toggle( int dagfd, uint32_t group_handle, uint32_t linkID );

/**
 * @brief		Retrieve the NE alarms for the link.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group the link is attached to.
 * @param[in] linkID	ID of the link.
 * @param[out] palarms	pointer to an NE alarms structure (in/out) parameter
 *
 * @return		0 - success, <0 - failure.
*/
int32_t dagima_link_get_ne_alarms( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_ne_alarms_t* palarms );

/**
 * @brief		Retrieve the FE alarms for the link.
 *
 * @param[in] dagfd	dag file descriptor
 * @param[in] group_handle handle of the group the link is attached to.
 * @param[in] linkID	ID of the link.
 * @param[out] palarms	pointer to an FE alarms structure (in/out) parameter
 *
 * @return		0 - success, <0 - failure
*/
int32_t dagima_link_get_fe_alarms( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_fe_alarms_t* palarms );

/**
 * @brief		Retrieve the ICP statistics for the link.
 *
 * @param[in] dagfd	file descriptor
 * @param[in] group_handle handle of the group the link is attached to.
 * @param[in] linkID	ID of the link.
 * @param[out] pstatistics pointer to an ICP statistics structure (in/out) parameter
 *
 * @return		0 - success, <0 - failure
*/
int32_t dagima_link_get_ICP_statistics( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_ICP_statistics_t* pstatistics );

/**
 * @brief		Retrieve the NE defect statistics for the link.
 *
 * @param[in] dagfd	file descriptor
 * @param[in] group_handle handle of the group the link is attached to.
 * @param[in] linkID	ID of the link.
 * @param[out] pdefects pointer to a NE defect structure (in/out) parameter
 *
 * @return		0 - success, <0 - failure
*/
int32_t dagima_link_get_ne_defect_statistics( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_ne_defect_statistics_t* pdefects );

/**
 * @brief		Retrieve the FE defect statistics for the link
 *
 * @param[in] dagfd	file descriptor
 * @param[in] group_handle handle of the group the link is attached to.
 * @param[in] linkID	ID of the link.
 * @param[out] pdefects	pointer to a FE defect structure (in/out) parameter
 *
 * @return		 0 - success, <0 - failure
*/
int32_t dagima_link_get_fe_defect_statistics( int dagfd, uint32_t group_handle, uint32_t linkID, dag_link_fe_defect_statistics_t* pdefects );
/*@}*/

#endif
/*@}*/
/*@}*/
