/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
**********************************************************************/

/**********************************************************************
* FILE:		dagmapper.h
* DESCRIPTION:	DAG 3.7T Programming Interface
*		HDLC Transmit API
*
* HISTORY:
*   DNH 16/8/04 - Initial version
**********************************************************************/

#ifndef DAGMAPPER_H
#define DAGMAPPER_H

#define HDLC_ERF_SIZE 56

#define DM_DEBUG_ERF_FILE	0x0001
#define DM_DEBUG_REPEAT		0x0002

/* Mapper channel id's are marked as such by a bit */
//#define USE_MAPPER		/* Have dag37t_api use the mapper for tx channel configuration */
#define DM_CHANNEL_MARKER	0x0008000	/* Flags a channel ID as transmit */

#define DM_CHANID(chanId)	((chanId) & ~DM_CHANNEL_MARKER)
#define DM_CHANID_USER(chanId)	((chanId) | DM_CHANNEL_MARKER)
#define DM_IS_MAPPER_CHANNEL(chanId)	((chanId & DM_CHANNEL_MARKER) == DM_CHANNEL_MARKER)


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/**********************************************************************
* FUNCTION:	dag_mapper_open(dagfd, device, burst_size)
* DESCRIPTION:	Open an HDLC mapper for the specified device.
* INPUTS:	dagfd	   - device to open the mapper on (i.e. previously
*			     openned with dag_open()).
*		device	   - device's ASCII string file name.
*		burst_size - amount of data to send per mapper burst,
*			     set to 0 for default size.
* RETURNS:	0	- success
*		< 0	- failure
**********************************************************************/
int dag_mapper_open(
    int dagfd,
    char *device,
    uint32_t burst_size);


/**********************************************************************
* FUNCTION:	dag_mapper_close(dagfd)
* DESCRIPTION:	Close the HDLC mapper for the specified device.
* INPUTS:	dagfd	   - device to close the mapper on.
* RETURNS:	0	- success
*		< 0	- failure
**********************************************************************/
int dag_mapper_close(int dagfd);


/**********************************************************************
* FUNCTION:	dag_mapper_add_channel(dagfd, type, line, ts, tsConf)
* DESCRIPTION:	Add a channel to a device, return the new channel Id.
* INPUTS:	dagfd	- device to add the channel to
*		type	- type of channel; CT_CHAN, CT_HYPER, CT_SUB.
*		line	- line interface number for channel
*		ts	- timeslot for channel (not used for CT_HYPER)
*		tsConf	- CT_CHAN:  not used
*			  CT_HYPER: 32 bit mask where each bit is a timeslot
*				    to use for the channel.
*			  CT_SUB:   1 to 8 bit mask which defines the 
*				    contiguous bits to use for the subchannel.
*				    Must be 1, 2, or 4 bits.
* RETURNS:	< 0	- failure
* 		else channel Id
**********************************************************************/
//#ifdef USE_MAPPER
int dag_mapper_add_channel(
    int dagfd,
    uint32_t type, 
    uint32_t line, 
    uint32_t ts,
    uint32_t tsConf);

//#else
/*
int dag_mapper_add_channel(int dagfd,
                       int chanId,
                       uint32_t type,
                       uint32_t uline,
                       uint32_t ts,
                       uint32_t tsConf);
#endif
*/
/**********************************************************************
* FUNCTION:	dag_mapper_delete_channel(dagfd, chanId)
* DESCRIPTION:	Deletes a previously added channel.
* INPUTS:	dagfd	- device to delete the channel from
*		chanId	- id of the channel to delete
* RETURNS:	0	- success
*		< 0	- failure
**********************************************************************/
int dag_mapper_delete_channel(
    int dagfd,
    int chanId);


/**********************************************************************
* FUNCTION:	dag_send_frame(dagfd, int chanId, uint8_t *datap, int len, options)
* DESCRIPTION:	Send a block of data as an HDLC frame on the specified channel.
* INPUTS:	dagfd	- device to send frame on
*		chanId	- channel to send frame on
*		datap	- data to send (no flags, no FCS).
*		len	- amount of data to send (number of octets).
*		options	- send options.  Reserved for future use - set to 0.
* RETURNS:	0 - success
*		<0 - failure.
**********************************************************************/
int dag_mapper_send_frame(
    int dagfd,
    int chanId,
    uint8_t *datap,
    int len,
    uint32_t options,
    void (*callback_sent)());


/**********************************************************************
* FUNCTION:	dag_send_debug_frame(dagfd, chanId, datap, len, error_ind, fcs_error, options)
* DESCRIPTION:	Send a block of data as an HDLC frame on the specified channel.
*		This is the same as dag_send_frame, except you may
*		optionally insert a bit stuffing error at offset error_ind, 
*		and optionally send an invalid fcs for the frame.
* INPUTS:	dagfd		- device to send frame on
*		chanId		- channel to send frame on
*		datap		- data to send (no flags, no FCS).
*		len		- amount of data to send (number of octets).
*		error_ind	-  if non-zero then insert 0xff at this 
*				   offset in the frame
*		fcs_error	-  boolean; if true then send an invalid FCS
*		options		- send options.  Reserved for future use - set to 0.
* RETURNS:	0 - success
*		<0 - failure.
**********************************************************************/
int dag_mapper_send_debug_frame(
    int dagfd,
    int chanId,
    uint8_t *datap,
    int len,
    int error_ind,
    int fcs_error,
    uint32_t options,
    void (*callback_sent)());


/**********************************************************************
* FUNCTION:	dag_send_debug_frame_atm(dagfd, chanId, datap, hec_error, options, callback_sent)
* DESCRIPTION:	Send a block of data as an ATM frame on the specified channel.
*		This is the same as dag_send_frame, except you may
*		optionally insert a HEC error.
*		The datap pointer must point to a buffer containing 48 bytes
* INPUTS:	dagfd		- device to send frame on
*		chanId		- channel to send frame on
*		datap		- data to send (no flags, no FCS).
*		hec_error	-  boolean; if true then send an invalid FCS
*		options		- send options.  Reserved for future use - set to 0.
*		callback_sent	- optional callback handler invoked after the frame
*				  has actually been sent.
* RETURNS:	0 - success
*		<0 - failure.
**********************************************************************/
int dag_mapper_send_debug_frame_atm(
    int dagfd,
    int uchanId,
    uint8_t *datap,
    int hec_error,
    uint32_t options,
    void (*callback_sent)() );


/**********************************************************************
* FUNCTION:	dag_mapper_start(dagfd, burst_count, erf_count, delay)
* DESCRIPTION:	Start the mapper thread running for the specified dag device,
*		start transmitting frames.
* INPUTS:	dagfd	     - device to start the mapper on.
*		burst_count  - the number of frames to send per period
*		erf_count    - exit after sending this many frames
*			       (0 = don't exit).
*		delay	     - burst period - sleep delay microseconds
*			       between bursts.
* RETURNS:	0	- success, mapper thread started.
*		< 0	- mapper not found
**********************************************************************/
int dag_mapper_start(
    int dagfd,
    unsigned int burst_count,
    unsigned int erf_count,
    unsigned int delay);


/**********************************************************************
* FUNCTION:	dag_mapper_stop(dagfd)
* DESCRIPTION:	Stop the mapper thread for the specified dag device.
* INPUTS:	dagfd	- device to stop the mapper on.
* RETURNS:	0	- success
*		< 0	- mapper not found
**********************************************************************/
int dag_mapper_stop(int dagfd);


/**********************************************************************
* FUNCTION:	dag_mapper_set_verbosity(verb_level)
* DESCRIPTION:	Sets the debugging verbosity level.  A level of 0 is no
*		debug output, and each value increment is an additional
*		level of debug output.
* INPUTS:	verb_level	- verbosity level (0 to 4).
* RETURNS:	none.
**********************************************************************/
void dag_mapper_set_verbosity(int verb_level);


/**********************************************************************
* FUNCTION:	dag_mapper_set_debug(debug_bits)
* DESCRIPTION:	Sets the debugging features.  The debug_level is a set
*		of debug bits each of which enables a specific type
*		of debugging.  See the DM_DEBUG_xxx defines for debug
*		options.
* INPUTS:	debug_bits	- debug bitset
* RETURNS:	none.
**********************************************************************/
void dag_mapper_set_debug(uint32_t debug_level);


/**********************************************************************
* FUNCTION:	dag_mapper_get_level(dagfd)
* DESCRIPTION:	Return the total number of HDLC frames waiting to be sent
* 		(or currently being sent) for the specified device.
* INPUTS:	dagfd	     - device to return level of
* RETURNS:	Number of HDLC frames queued to be sent.
**********************************************************************/
uint32_t dag_mapper_get_level(int dagfd);


/**********************************************************************
* FUNCTION:	dag_mapper_get_channel_level(dagfd, chanId)
* DESCRIPTION:	Return the number of HDLC frames waiting to be sent
* 		(or currently being sent) the specified channel.
* INPUTS:	dagfd	     - device to return level of
* RETURNS:	Number of HDLC frames queued to be sent on the channel.
**********************************************************************/
uint32_t dag_mapper_get_channel_level(int dagfd, int chanId);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAGMAPPER_H */
