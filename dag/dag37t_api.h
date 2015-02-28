/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_api.h,v 1.19 2006/07/27 20:36:31 cassandra Exp $
 */


#ifndef DAG37T_API_H
#define DAG37T_API_H

/**
* Indicates the channel type and/or format for mixed mode firmware
*/


typedef enum
{
	CT_UNDEFINED,   /* undefined channel type */
	CT_CHAN,        /* simple channel type */
	CT_HYPER,       /* hyperchannel type */
	CT_SUB,         /* subchannel type */
	CT_RAW,         /* raw connection type */
	CT_CHAN_RAW,
	CT_HYPER_RAW,
	CT_SUB_RAW,

	/* Add new channel types before this line */

	/* Channel formats go here */
	/* Upper 16 bits hold the format */
	CT_FMT_DEFAULT	= 0x00000000,
	CT_FMT_HDLC		= 0x00010000,
	CT_FMT_ATM		= 0x00020000
} dag_channel_t;

/**
* Indicates the direction of the channel 
*/

typedef enum
{

	DAG_DIR_UNDEFINED=0,
	DAG_DIR_RECEIVE,
	DAG_DIR_TRANSMIT

}dag_direction_t;

#define MAX_CHANNEL 2048


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



	/**
	* Add a channel on the 3.7T card
	* \param dagfd The dag file descriptor returned from dag_open
	* \param direction receive or transmit
	* \param type one of seven channel types such as raw, hyper, sub channel etc.
	* \param uline physical line this connection is to be made on
	* \param tsConf timeslot configuration of hyper channel or bit mask of sub channel
	* \param raw_transmit indicates raw transmit channels
	* \return channel number or -1 on error
	*/

	int dag_add_channel(
		int dagfd, 
		uint32_t direction, 
		uint32_t type, 
		uint32_t uline, 
		uint32_t tsConf,
		uint32_t raw_transmit);

	/**
	* Remove all channels on the 3.7T card
	* \param dagfd The dag file descriptor returned from dag_open
	* \return 0 or -1 on error
	*/

	int dag_delete_board_channels(
		int dagfd);

	/**
	* Remove a channel on the 3.7T card
	* \param dagfd The dag file descriptor returned from dag_open
	* \param channel_id The identification number of the channel previously returned from dag_add_channel
	* \return 0 or -1 on error
	*/

	int dag_delete_channel(
		int dagfd, 
		uint32_t channel_id);

	/**
	* Set a physical interface on the 3.7T card to use or not use ATM cell scrambling 
	* \param dagfd The dag file descriptor returned from dag_open
	* \param iface The interface (0-15) to set the self synchronous cell scrambling on
	* \param option true, set scrambling on, false set scrambling off
	* \return 0 or -1 on error
	*/

	int dag_ifc_cell_scrambling (
		int dagfd, 
		uint32_t iface, 
		uint32_t option);

	/**
	* Set a physical interface on the 3.7T card to use or not use ATM header error control (HEC) correction
	* \param dagfd The dag file descriptor returned from dag_open
	* \param iface The interface (0-15) to set the HEC correction on
	* \param option true, set HEC correction on, false set HEC correction off
	* \return 0 or -1 on error
	*/

	int dag_ifc_hec (
		int dagfd, 
		uint32_t iface, 
		uint32_t option);




	enum 
	{
		DA_DATA_TO_HOST = 0,    /* send data to host */
		DA_DATA_TO_LINE,        /* send data to line */
		DA_DATA_TO_IOP          /* send data to IOP  */
	};


	/**
	* Set the data path (ERF MUX) for the 3.7T card 
	* \param dagfd The dag file descriptor returned from dag_open
	* \param host_mode where to send data coming from the host
	* \param line_mode where to send data coming from the line
	* \param iop_mode  where to send data coming from the IOP (Xscale)
	* \return 0 or -1 on error
	*/

	uint32_t dag_set_mux(
		int dagfd, 
		uint32_t host_mode, 
		uint32_t line_mode, 
		uint32_t iop_mode);


	/* Deprecated Functions */


	/**
	* Deprecated Function  no longer requires a reset for the 3.7T card 
	* \param dagfd The dag file descriptor returned from dag_open
	* \return 0 	
	*/
	int dag_reset_37t(
		int dagfd);

	/**
	* Deprecated Function use dagutil_set_verbosity function from dagutil.h
	* \param verb the level of verbosity required
	* \return void	
	*/
	void dag_channel_verbosity(
		uint32_t verb);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DAG37t_API_H*/


