/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagapi.h,v 1.95.2.5 2009/09/24 04:41:38 darren.lim Exp $
 */

/** \defgroup DagAPI DAG API
Interface functions for the DAG packet capture and transmit API.
 */
/*@{*/

#ifndef DAGAPI_H
#define DAGAPI_H

/*****************************************************************************/
/* Includes                                                                  */
/*****************************************************************************/

/* DAG headers. */
#include "dagnew.h"
#include "dagreg.h"

/* Header used for infiniband packet parsing */
#include "infiniband_protocol_headers.h"

#if (defined(__APPLE__) && defined(__ppc__))

#include <IOKit/IOKitLib.h>
#include <ppc_intrinsics.h>

#define dag_writel(val, reg)  __stwbrx((val), (reg), 0)
#define dag_readl(reg)        __lwbrx((reg), 0)

#elif defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__i386__)) || (defined(__SVR4) && defined(__sun)) || defined(_WIN32)

/* Can't rely on autoconf's WORDS_BIGENDIAN here because we can't install config.h with the Endace headers.  
 * So we do our best to detect based on built-in compiler #defines.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(__ia64__) 

/* OK: known little-endian CPU. */
#define dag_writel(val, reg)  *(volatile uint32_t*)((reg)) = (val)
#define dag_readl(reg)        *(volatile uint32_t*)((reg))

#elif defined(__BIG_ENDIAN__) || defined(__ppc__) || defined(__POWERPC__)

/* Error: known big-endian CPU. */
#error Compiling for FreeBSD, Linux, Mac OS X, Solaris or Windows on a big-endian CPU is not supported.  Please contact <support@endace.com> for assistance.
//FIXME: properly for itanium 
//FORCED for linux to recognize as little endian 
#elif defined ( __linux__ )

#else

/* Error: unknown endianness CPU. */
#error Could not detect CPU endianness.  Please contact <support@endace.com> for assistance.

#endif /* big endian check */

#else
#error Compiling on an unsupported platform.  Please contact <support@endace.com> for assistance.
#endif /* Platform-specific code. */


/*****************************************************************************/
/* Macros and constants                                                      */
/*****************************************************************************/

/** \ingroup DagAPI
GPP record type definitions
 */
/*@{*/

/**
GPP record type for legacy records.
 */
#define TYPE_LEGACY             0

/**
GPP record type for HDLC PoS frames.
 */
#define TYPE_HDLC_POS           1

/**
GPP record type for Ethernet frames.
 */
#define TYPE_ETH                2

/**
GPP record type for ATM cells.
 */
#define TYPE_ATM                3

/**
GPP record type for reassembled AAL5 frames.
 */
#define TYPE_AAL5               4

/**
GPP record type for multi-channel HDLC frames.
 */
#define TYPE_MC_HDLC            5

/**
GPP record type for multi-channel Raw Link frames.
 */
#define TYPE_MC_RAW             6

/**
GPP record type for multi-channel ATM cells.
 */
#define TYPE_MC_ATM             7

/**
GPP record type for multi-channel Raw Link frames (unsupported on newer versions). Retained for Dag7.1s firmware image compatibilities.
 */
#define TYPE_MC_RAW_CHANNEL     8

/**
GPP record type for multi-channel AAL5 frames.
 */
#define TYPE_MC_AAL5            9

/**
GPP record type for HDLC PoS frames with the loss counter field reassigned as colour.
 */
#define TYPE_COLOR_HDLC_POS     10

/**
GPP record type for Ethernet frames with the loss counter field reassigned as colour.
 */
#define TYPE_COLOR_ETH          11

/**
GPP record type for multi-channel AAL2 frames.
 */
#define TYPE_MC_AAL2            12

/**
GPP record type for IP counter frames.
 */
#define TYPE_IP_COUNTER         13

/**
GPP record type for TCP flow counter frames.
 */
#define TYPE_TCP_FLOW_COUNTER   14

/**
GPP record type for HDLC PoS frames with the loss counter field reassigned as DSM type colour.
 */
#define TYPE_DSM_COLOR_HDLC_POS 15

/**
GPP record type for Ethernet frames with the loss counter field reassigned as DSM type colour.
 */
#define TYPE_DSM_COLOR_ETH      16

/**
GPP record type for multi-channel HDLC frame with the loss counter field reassigned as colour.
 */
#define TYPE_COLOR_MC_HDLC_POS  17

/**
GPP record type for reassembled AAL2 frames.
 */
#define TYPE_AAL2               18

/**
GPP record type for HDLC PoS frames with the loss counter field reassigned as IPF colour and hash value.
 */
#define TYPE_COLOR_HASH_POS     19

/**
GPP record type for Ethernet frames with the loss counter field reassigned as IPF colour and hash value.
 */
#define TYPE_COLOR_HASH_ETH     20

/**
GPP record type for Infiniband frames.
 */
#define TYPE_INFINIBAND         21

/**
GPP record type for Raw Link frames.
 */
#define TYPE_RAW_LINK		24

/**
GPP record type for Infiniband Link packets.
 */
#define TYPE_INFINIBAND_LINK	25
/**
GPP record type for pad records in DAG-II.
 */
#define TYPE_PAD                48

#define TYPE_MIN  1   /* sanity checking */
#define TYPE_MAX  48  /* sanity checking */

/** \ingroup DagAPI
Extension header type defines
 */
/*@{*/

/**
Extension header type for NinjaProbe 40G1 classification units. Retained for backwards compatibility.
 */
#define EXT_HDR_TYPE_NP40G1    		3

/**
Extension header type for classification units.
 */
#define EXT_HDR_TYPE_CLASSIFICATION	3	/* includes Ninjaprobe 40G1, Infiniband and future classification units for carrying steering, colour, user information */

/**
Extension header type for Raw Link frames.
 */
#define EXT_HDR_TYPE_RAW_LINK		5	/* includes raw SONET, SDH, etc */

/**
Extension header type for BFS-based architectures.
 */
#define EXT_HDR_TYPE_BFS		6

/**
Extension header type for reporting SONET/SDH frame layer stats. Note this is an Endace internal header.
 */
#define EXT_HDR_TYPE_SDH_FRAME_INFO	7

/**
Extension header type for reporting SONET/SDH payload layer stats. Note this is an Endace internal header.
 */
#define EXT_HDR_TYPE_SPE_FRAME_INFO	8

/**
Extension header type for reporting single pattern matching from DS3 raw frames and parity bits (CP, P, etc) stats.  Note this is an Endace internal header.
 */
#define EXT_HDR_TYPE_DS3_RAW_LINK	9

/**
Extension header type for reporting DS3 overhead information.
 */
#define EXT_HDR_TYPE_DS3_OVERHEAD	10

/**
Extension header type for reporting DS3 calculated BIP64 information. Note this is an Endace internal header.
 */
#define EXT_HDR_TYPE_DS3_CALCULATED_BIP64	11
/*@}*/

/**
DAG ERF header size in bytes
 */
#define dag_record_size   16


/**
The flag DAGF_NONBLOCK causes dag_offset to be non-blocking.otherwise it blocks till atleast on record is avaliable.
 */
#define DAGF_NONBLOCK     0x01  /* Deprecated, use dag_set_stream_poll(). */

/**
Maximum number of interfaces on a DAG device
 */
#define MAX_INTERFACES 16

/**
FIXME: Needs description!
 */
#define PBM_TX_DEFAULT_SIZE 16
/*@}*/


/*****************************************************************************/
/* Platform-specific headers and macros                                      */
/*****************************************************************************/

#include <inttypes.h>
#include <sys/time.h>

# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}
# define TIMESPEC_TO_TIMEVAL(tv, ts) {                                   \
        (tv)->tv_sec = (ts)->tv_sec;                                    \
        (tv)->tv_usec = (ts)->tv_nsec / 1000;                           \
}

/*****************************************************************************/
/* Data structures                                                           */
/*****************************************************************************/

/** \ingroup DagAPI
 */
/*@{*/

/**
Data structure for HDLC PoS frames, TYPE_HDLC_POS.
 */
typedef struct pos_rec {
	uint32_t          hdlc;
	uint8_t           pload[1];
} pos_rec_t;

/**
Data structure for Ethernet frames, TYPE_ETH.
 */
typedef struct eth_rec {
	uint8_t           offset;
	uint8_t           pad;
	uint8_t           dst[6];
	uint8_t           src[6];
	uint16_t          etype;
	uint8_t           pload[1];
} eth_rec_t;

/**
Data structure for ATM cells, TYPE_ATM.
 */
typedef struct atm_rec {
	uint32_t          header;
	uint8_t           pload[1];
} atm_rec_t;

/**
Data structure for reassembled AAL5 frames, TYPE_AAL5.
 */
typedef struct aal5_rec {
	uint32_t          header;
	uint8_t           pload[1];
} aal5_rec_t;

/**
Data structure for multi-channel HDLC frames, TYPE_MC_HDLC.
 */
typedef struct mc_hdlc_rec {
	uint32_t          mc_header;
	uint8_t           pload[1];
} mc_hdlc_rec_t;

/**
Data structure for multi-channel Raw Link frames, TYPE_MC_RAW.
 */
typedef struct mc_raw_rec {
	uint32_t          mc_header;
	uint8_t           pload[1];
} mc_raw_rec_t;

/**
Data structure for multi-channel ATM cells, TYPE_MC_ATM.
 */
typedef struct mc_atm_rec {
	uint32_t          mc_header;
	uint8_t           pload[1];
} mc_atm_rec_t;

/**
Data structure for multi-channel Raw Link frames, TYPE_MC_RAW_CHANNEL (unsupported on newer versions).
 */
typedef struct mc_raw_channel_rec {
	uint32_t          mc_header;
	uint8_t           pload[1];
} mc_raw_channel_rec_t;

/**
Data structure for multi-channel AAL5 frames, TYPE_MC_AAL5.
 */
typedef struct mc_aal_rec {
	uint32_t          mc_header;
	uint8_t           pload[1];
} mc_aal_rec_t;

/**
Data structure for reassembled AAL2 frames, TYPE_AAL2.
 */
typedef struct aal2_rec {
	uint32_t          ext_header;
	uint32_t          header;
	uint8_t           pload[1];
} aal2_rec_t;

/**
Data structure for Raw Link frames, TYPE_RAW_LINK.
 */
typedef struct raw_link_rec {
	uint8_t           pload[1];
} raw_link_t;

/**
Data structure for the flags field of the ERF header.
 */
typedef struct flags {
	uint8_t           iface:2;
	uint8_t           vlen:1;
	uint8_t           trunc:1;
	uint8_t           rxerror:1;
	uint8_t           dserror:1;
	uint8_t           reserved:1;
	uint8_t           direction:1;
} flags_t;



/**
    Naming the union type used inside dag_record_t (for the member rec ) as erf_payload_t.
    Will be useful while parsing extension headers
*/
typedef union {
		pos_rec_t	pos;
		eth_rec_t	eth;
		atm_rec_t       atm;
		aal5_rec_t      aal5;
		aal2_rec_t      aal2;
		mc_hdlc_rec_t   mc_hdlc;
		mc_raw_rec_t    mc_raw;
		mc_atm_rec_t    mc_atm;
		mc_aal_rec_t    mc_aal5;
		mc_aal_rec_t    mc_aal2;
		mc_raw_channel_rec_t	mc_raw_channel;
	        infiniband_rec_t	infiniband;
		raw_link_t		raw_link;
	} erf_payload_t;

/**
Data structure for a global GPP record type.
 */
typedef struct dag_record {
	uint64_t          ts;
	uint8_t           type;
	flags_t           flags;
	uint16_t          rlen;
	uint16_t          lctr;
	uint16_t          wlen;
	erf_payload_t rec; 
} dag_record_t;
/*@}*/	// ingroup DagAPI

/*****************************************************************************/
/* Parameters set by user                                                    */
/*****************************************************************************/
/**
FIXME: Needs description
 */
#define DAG_FLUSH_RECORDS         0

/*****************************************************************************/
/* Function declarations                                                     */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/** \defgroup DagCardIfc Card Interface Functions
DAG Card Interface Functions
 */

/*@{*/

/**
Obtain a file descriptor for a DAG card. It is passed a string containing the DAG device 
node, “dag0”, for example and returns a valid UNIX file descriptor if successful.

@param dagname The name of the device to open, e.g. "/dev/dag0", "dag0".
@return A UNIX-style file descriptor for the DAG device on success, otherwise -1 with 
	errno set as follows
	- ENOMEM (out of memory)
	- ENFILE (too many open files for process)
	- EIO (fatal internal error)
	- ENODEV (the DAG does not support packet capture)
	Error codes as for ioctl(2), mmap(2).
*/
int dag_open(char * dagname);

/**
Close a DAG card.
@param dagfd A file descriptor for a DAG device.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	Error codes as for close(2)

*/
int dag_close(int dagfd);

/**
Configure a DAG card. This function allows user control over all parameters required 
to configure a card for a measurement session to be carried out.

@param dagfd A file descriptor for a DAG device.
@param params A whitespace-delimited configuration string.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (params contained an invalid token)
	- EIO (fatal internal error)
	- ENODEV (no such device)
	Error codes as for lseek(2), read(2), fork(2), execvp(2), execlp(2), execve(2), wait(2)
@note	At present, only the following options are implemented by dag_configure:
	
	- slen=<N> :	Set packet capture length to N bytes. N will be rounded down to nearest 
			multiple of four. Packets longer than N bytes will be truncated to N.\n
	- varlen :	Set capture record mode to variable length. Packets shorter than slen 
			will produce short records.\n
	- novarlen :	Set capture record mode to fixed length. Packets shorter than slen will 
			be padded to slen.\n
	- fixed	:	Same as novarlen.\n
	- ncells=<N> :	Only for ATM capture mode on the DAG 3.5. N is 0-15, and specifies the 
			number of cells to return from the start of each AAL5 frame. The DAG 3.5 
			is capable of tracking the state of many VPI/VCIs at once. If N is 0 all 
			cells are captured, including OAM and RM cells which are dropped otherwise.\n
	- (no)lcells :	Only for ATM capture mode on the DAG 3.5, where ncells>0. If set, the last 
			cell of the AAL5 frame containing the AAL5 trailer will always be captured.
			For example if ncells=3 and lcell is set, the first, second, third, and 
			seventh cells will be captured from a seven cell AAL5 frame.
*/
int dag_configure(int dagfd, char * params);

/**
This is the mode of operation of a Soft DAG card.
*/
#define DAG_REVERSE_MODE 1
/**
This is the normal mode of operation of a Hardware DAG card.
*/
#define DAG_NORMAL_MODE 0


/**
Sets the mode of RX and TX for usage with softdag
Two modes are possible Normal and Reverse . If normal mode is set the dag api behaves as before and can be used with softdag from the customer side 
Reverse mode - changes the Limit and Record pointer. Thoe with HW dag card is not going to work , but with softdag will emulate the hardware
in the reverse mode allows DAGAPI to behave like daemon for HW emulation
Note this function has to be called before dag_attach_stream was called  
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param mode DAG_REVERSE_MODE (1) or DAG_NORMAL_MODE (0)
@return 0 on success, otherwise -1 with errno set appropriately.
*/
int dag_set_mode(int dagfd, int stream_num, uint32_t mode);

/**
Reserve a stream on a DAG card and map its memory buffer into the process' address space.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param flags A bitfield that modifies the behaviour of the routine.
@param extra_window_size How much extra address space to allocate.  0 means to allocate twice the 
			 stream size and is recommended for most users.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EACCES (stream is locked by another process)
	- EINVAL (stream_num is invalid or extra_window_size is too large)
	- ENOMEM (stream has no memory allocated)
	- ENODEV (the DAG does not support packet capture)
	- EIO (fatal internal error)
	Error codes as for ioctl(2), malloc(2), mmap(2)
@note	This function performs per-stream locking, and memory mapping functions. The DAG descriptor 
	dagfd is provided by dag_open(). The stream_num is an integer in the range 0 to MAX_INTERFACES
	that indicates which stream to attach to. There are currently no flags defined.\n 	
	To avoid seeing a packet record wrap over from the top of the circular buffer, a portion of
	the circular buffer is mapped into memory twice. This allows a record that would otherwise
	be split across the buffer boundary to appear contiguous. Traditionally the entire buffer is
	mapped twice.\n\n
	For example, a physical 32MB buffer space would be mapped into user-space twice,
	consuming 64MB of that process’s virtual memory. This permits operation of the circular
	buffer with no restriction on how often buffer space must be cleared. When
	extra_window_size has the special value 0, this behavior is maintained. This can consume
	significant quantities of process virtual memory however when large physical buffers are
	used, such as 512MB. The extra_window_size parameter can be used to specify the size of the
	second buffer mapping. For example, with a 32MB physical buffer and extra_window_size
	set to 4MB, only 36MB of process virtual memory is used.\n\n
	When using the dag_advance_stream() access method if extra_window_size is non-zero, the
	user must never process more than extra_window_size before calling dag_advance_stream()
	again. This allows dag_advance_stream() to normalize the buffer pointers into the lower
	buffer mapping before the top of the second buffer mapping is reached.\n\n
	When using the dag_rx_stream_next_record() access method, extra_window_size must be at
	least as large as the maximum size packet possible on the link medium. For efficiency
	extra_window_size should be at least several megabytes, 4MB is a reasonable default.\n\n
	When attaching a stream for transmission, extra_window_size must be set equal to or greater
	than the maximum sized block of data that is to be transmitted at once. For backwards
	compatibility, or if a user is unsure, the extra_window_size is set to zero.\n\n
	Setting the extra_window_size does not affect the physical memory consumption. Setting the
	extra_window_size only conserves the process virtual memory. Endace recommends setting
	the extra_window_size to zero unless process virtual memory is at a premium.

*/
int dag_attach_stream(int dagfd, int stream_num, uint32_t flags, uint32_t extra_window_size);
/**
Reserve a stream on a DAG card and map its memory buffer into the process' address space.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param flags A bitfield that modifies the behaviour of the routine.
@param extra_window_size How much extra address space to allocate.  0 means to allocate twice the stream size and is 
@param protection specify PROT_READ or PROT_READ| PROT_WRITE.allows the user to map a stream as read only.
recommended for most users.
@return 0 on success, otherwise -1 with errno set appropriately.
*/
int
dag_attach_stream_protection(int dagfd, int stream_num, uint32_t flags, uint32_t extra_window_size,int protection);
/**
Release a stream on a DAG card.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is invalid)
	Error codes as for ioctl(2)
@note	The per-stream lock is released, allowing other processes to attach to the stream, and the
	stream buffer is unmapped from user space. It should be called when the stream is no longer
	required by the application.
*/
int dag_detach_stream(int dagfd, int stream_num);

/**
Start transmitting/receiving on a stream.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EBUSY (the stream is already started)
	- ENODEV (the DAG does not support packet capture)
	- EINVAL (stream_num is not attached)
	- EIO (fatal internal error)
	- ETIMEDOUT (communication with card failed)
	Error codes as for ioctl(2)
@note	The stream must be attached before it can be started.
*/
int dag_start_stream(int dagfd, int stream_num);

/**
Start transmitting/receiving on the daemon (REVERSE) side of a softdag stream.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param timeout in units of 10ms
@return 0 on success, otherwise -1 with errno set as follows: 
	- ETIMEDOUT used when client side has not set SYNC_L2R before timeout expires.
*/
int dag_start_stream_softdag(int dagfd, int stream_num, int timeout);

/**
Stop transmitting/receiving on a stream.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached or stream_num is not started)
	- ENODEV (the DAG does not support packet capture)

*/
int dag_stop_stream(int dagfd, int stream_num);

/**
Read the current polling parameters for a stream.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param[out] mindata The minimum amount of data to retrieve in bytes.  
	    The default is 16 (the size of an ERF record header) and 0 
	    puts the stream into non-blocking mode.
@param[out] maxwait The maximum amount of time the receive routines should 
	    wait before returning.  This overrides the mindata parameter.
@param[out] poll In case sufficient data has not arrived and the maximum time 
	    to wait has not expired, this parameter sets the interval at which 
	    the library will check to see if new data has arrived.  The default 
	    is 10 milliseconds.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached)
@note	The DAG drivers avoid interrupts due to the associated overheads, using polling 
	methods instead. The amount of data that must be received before a call to dag_advance_stream() 
	or dag_rx_stream_next_record() will return is given by mindata. This defaults to 
	16 bytes, the size of an ERF record header.\n\n
	If mindata is zero, the receive functions will return immediately if no data is available,
	allowing non-blocking behavior.\n\n
	The maxwait parameter is the maximum amount of time the receive functions should wait
	before returning. This overrides the mindata parameter, so that even if mindata is non-zero,
	the call will return with 0 bytes available after maxwait time. By default the maxwait
	parameter is set to the special value zero which means that it is disabled. This means that the
	receive calls will block indefinitely for mindata bytes.\n\n
	If mindata bytes are not available when the receive function is first called, the library will
	sleep for poll time before checking for more data. This sleep avoids excessive polling traffic
	to the DAG card that may waste bus bandwidth, and frees the CPU for other processes.\n\n
	Each time the library wakes from a poll sleep, the timeout as set by maxwait is checked, and
	the function will return if maxwait is exceeded. The default value of poll is 10ms, implying a
	maximum of 100 polls per second when no data is available. The value of poll should always
	be less than or equal to the value of maxwait, as the minimum sleep time is poll.\n\n
	The poll sleep is implemented in user space using the POSIX.1b nanosleep(2) function. The
	current implementation of this call in Linux is based on the normal kernel timer mechanism,
	which has a resolution of 1/HZ, or 10ms on Linux/i386. This means that values of maxwait
	and poll less than 10ms will result in additional delay up to 10ms.\n\n
	If the application uses a real time scheduler such as SCHED_FIFO or SCHED_RR, then sleep
	values up to 2ms will be performed as busy-waits. This allows for faster and more accurate
	polling, but will lead to high CPU utilization due to busy-waiting rather than releasing the
	CPU to the scheduler.
*/
int dag_get_stream_poll(int dagfd, int stream_num, uint32_t * mindata, struct timeval * maxwait, struct timeval * poll);

/**
Set the polling parameters for an individual stream when the defaults are not sufficient.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param mindata The minimum amount of data to retrieve in bytes.  
	       The default is 16 (the size of an ERF record header) 
	       and 0 puts the stream into non-blocking mode.
@param maxwait The maximum amount of time the receive routines should 
	       wait before returning.  This overrides the mindata parameter.
@param poll In case sufficient data has not arrived and the maximum time to 
	    wait has not expired, this parameter sets the interval at which 
	    the library will check to see if new data has arrived.  The default 
	    is 10 milliseconds.
@return 0 on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached)

*/
int dag_set_stream_poll(int dagfd, int stream_num, uint32_t mindata, struct timeval * maxwait, struct timeval * poll);

/**
Read the size of the stream buffer in bytes.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return The size of the stream buffer on success, otherwise -1 with errno set 
	as follows:
	- EBADF (dagfd is an invalid file descriptor)
*/
int dag_get_stream_buffer_size(int dagfd, int stream_num);

/**
@brief Read the virtual base address of the stream buffer.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return Read the virtual base address of the stream buffer.
*/
void *dag_get_stream_buffer_virt_base_address(int dagfd, int stream_num);

/**
@brief returns the physical address of the stream buffer.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return the physical address of the stream buffer.
*/
int dag_get_stream_phy_buffer_address(int dagfd, int stream_num);

/**
@brief	Read the number of bytes currently outstanding in the stream buffer.  
	For transmit streams this is the number of bytes committed by the 
	user but not yet transmitted by the card. For receive streams this 
	is the number of bytes available to the user for reading.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return The number of bytes outstanding in the stream buffer on success, 
	otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached)
@note	For transmit streams this is the number of bytes that have been committed by 
	the user but have not yet been transmitted. Space allocated using dag_tx_get_stream_space() 
	which has not been committed for transmission is not counted.\n\n
	For receive streams this is the number of bytes of data available to the user for reading. This
	does include bytes that the user may have read but has not yet freed by calling
	dag_advance_stream() or dag_rx_stream_next_record().\n\n
	The dag_rx_stream_next_record() routine may not free buffer space occupied by previously
	read packets immediately for efficiency reasons.\n\n
	The dag_rx_stream_next_record() call reads hardware registers on the DAG card, so each
	call will generate bus transactions. If polled at high rates this could potentially interfere with
	data capture or transmission operations.

*/
int dag_get_stream_buffer_level(int dagfd, int stream_num);

/**
@brief Read the last buffer level.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return
*/
int dag_get_stream_last_buffer_level (int dagfd, int stream_num);

/**
@brief Read the number of receive streams supported by the DAG card with the currently loaded firmware.
@param dagfd A file descriptor for a DAG device.
@return The number of receive streams on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
@note	This does not imply that all of the streams have memory allocated to their buffers. The DAG
	may support a greater or lesser number of streams with different firmware.
*/
int dag_rx_get_stream_count(int dagfd);

/**
@brief Read the number of transmit streams supported by the DAG card with the currently loaded firmware.
@param dagfd A file descriptor for a DAG device.
@return The number of transmit streams on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
@note	This does not imply that all of the streams have memory allocated to their buffers. The DAG
	may support a greater or lesser number of streams with different firmware.
*/
int dag_tx_get_stream_count(int dagfd);

/**
@brief	Block/Record transmit allocator - zero copy.  Provides a pointer to the requested number of bytes 
	of available space for the specified stream.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param size The number of bytes requested.
@return A pointer to a block of at least the requested size on success, otherwise NULL with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- ENOTTY (stream_num is not a transmit stream)
	- EINVAL (stream_num is not attached)
	- ENOMEM (stream_num has no memory allocated)
@note	It is necessary to acquire a pointer into the stream buffer at which to write the records to be
	transmitted. When packet transmission is being performed using the zero-copy, the
	dag_tx_get_stream_space function blocks the transmission until the requested space is
	available.\n\n
	While polling for space to become available, it will sleep in poll time increments as set with
	dag_set_stream_poll(), freeing the CPU for other processes.

*/
uint8_t * dag_tx_get_stream_space(int dagfd, int stream_num, uint32_t size);

/**
@brief Block oriented transmit interface - zero copy.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param size The number of bytes to commit (transmit).
@return A pointer to a block of at least the requested size on success, otherwise NULL with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached or size is larger than the stream buffer size)
	- ENOMEM (stream_num has no memory allocated)
	- ENOTTY (stream_num is not a transmit stream)
@note	In order to transmit data the first step for the user is to get a pointer to write to using
	dag_tx_get_stream_space(), then write their data at that location, and finally call
	dag_tx_stream_commit_bytes() to indicate that the data can be sent.\n\n
	No pointer to the bytes to be sent is required, as the API holds this internally. The parameter
	size is the number of bytes that can be sent, this may be less than or equal to the size
	requested in the previous call to dag_tx_get_stream_space(), but must not be greater.\n\n
	This function returns a pointer to the end of the transmitted block, but no data can be
	written at this location until dag_tx_get_stream_space() has been called again to ensure
	buffer space is available.
*/
uint8_t * dag_tx_stream_commit_bytes(int dagfd, int stream_num, uint32_t size);

/**
@brief Block oriented transmit interface - COPIES DATA.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param orig A pointer to the beginning of the first ERF record to be copied.
@param size The number of bytes to copy.
@return The number of bytes written on success, otherwise -1 with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached or size is larger than the stream buffer size)
 	- ENOMEM (stream_num has no memory allocated)
  	- ENOTTY (stream_num is not a transmit stream)
@note	The records are copied from the user buffer into the stream buffer when space is available,
	and committed for transmission. No other functions need be called when using this method,
	but it is less efficient as the packet records must be copied by the CPU.\n\n
	The pointer orig indicates the location of the user buffer to be copies, while size contains
	the number of bytes to be copied and sent. The buffer to be sent does not have to be record
	aligned, but if the buffer contains only the start of a packet record, that packet will not be
	transmitted from the DAG until the remainder of the record is supplied.\n\n
	This call will block until space is available in the transmit stream buffer for all of the supplied
	data to be sent. 

*/
int dag_tx_stream_copy_bytes(int dagfd, int stream_num, uint8_t * orig, uint32_t size);

/**
@brief Record oriented receive interface - zero copy.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@return A pointer to a single ERF record on success, otherwise NULL with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached or size is larger than the stream buffer size)
	- ENOMEM (stream_num has no memory allocated)
	- ENOTTY (stream_num is not a receive stream)
	- EIO (ERF record has an invalid ERF type)
	- EAGAIN (timeouts are enabled and a timeout occurs when no data is available)
@note	This is a simpler approach and may ease porting, but due to the function call per packet the
	overhead may be 10% higher. The two methods should not be mixed on a single stream
	while the stream is started. The function uses the stream poll parameters described under
	dag_get_stream_poll(). These parameters define the blocking or non-blocking behavior, as
	well as the optional timeout and poll period.\n\n
	If not configured with dag_set_stream_poll() the default stream parameters will cause
	dag_rx_stream_next_record() to block when no data is available.
*/
uint8_t * dag_rx_stream_next_record(int dagfd, int stream_num);

/**
@brief Record oriented receive and transmit for inline forwarding.
@param dagfd A file descriptor for a DAG device.
@param rx_stream_num Which receive stream to use.
@param tx_stream_num Which receive stream to use.
@return A pointer to a single ERF record on success, otherwise NULL with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor.)
	- ENOTTY (One of the stream numbers is invalid.)
	- EINVAL (One of the streams is not attached.)
	- EIO (The ERF record has an invalid ERF type. EIO is usually a fatal error and the 
	capture session must be stopped.)
@note	The function can only be used with a DAG that is capable of transmitting packets and
	configured with the overlap option to set up the memory buffers for inline operation:
	- For DAG 3 class cards: dagthree –d dagN default overlap
	- For DAG 4 class series: dagfour –d dagN default overlap\n\n
	The dagfwddemo program included in the tools directory of the DAG software release
	demonstrates the inline capabilities of a DAG card and serves as a fully-functional example
	for programmers.\n\n
	dagfwddemo applies a user-defined BSD Packet Filter [BPF] expression to each packet
	received and only retransmits the packets that pass the filter.\n\n	
	Two streams are attached and started before this routine is used. One stream is for receive
	and the other for transmit. The main loop of an inline packet processing application using
	dag_rx_stream_next_record() will look similar to the following code:
	\code 
	while (keep_processing())
   	{
   		uint8_t* record = dag_rx_stream_next inline(uDagfd, RX_STREAM, TX_STREAM);
   		uint32_t bytes_to_commit;
   		process(record);
   		bytes_to_commit = ntohs(((dag_record_t *)record)->rlen);
   		dag_tx_stream_commit_bytes(uDagfd, TX_STREAM,
   		bytes_to_commit);
   	}
	\endcode 
	The process() routine has up to three functions to perform:
	- Determine action for packet: Make application-specific determination about whether 
	the packet is to be dropped or retransmitted.
	- Set packet to drop : If the packet is to be dropped then the rx error bit in the 
	ERF header flags byte must be set to 1.
	- Adjust iface field : If the packet is to be transmitted out to the opposite interface 
	from which it arrived, then the iface field in the ERF header flags must be adjusted.\n
	Some DAG card firmware has the capability to automatically rewrite the interface field in the
	ERF header so that packets received on interface 0 are transmitted via interface 1 and vice
	versa.\n\n
	If the DAG card has been configured to rewrite the interface field then the software does not
	need to perform Step 3 described above.

*/
uint8_t * dag_rx_stream_next_inline (int dagfd, int rx_stream_num, int tx_stream_num);

/**
@brief Traditional ringbuffer interface - zero copy.  Replaces dag_offset().
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param bottom A pointer to the location in the buffer up to which the application has processed.  Should be NULL on the first call.
@return A pointer to the top of available buffer space on success, otherwise NULL with errno set as follows:
	- EBADF (dagfd is an invalid file descriptor)
	- EINVAL (stream_num is not attached)
	- ENOMEM (stream_num has no memory allocated)
	- EIO (fatal internal error)
@note	Since it can return more than one record to the user at a time, it can be more efficient than
	using dag_rx_stream_next_record(). It operates by returning a pair of pointers into the
	stream buffer, which is mapped into the user process space in the dag_attach_stream() call.
	The bottom parameter is a pointer to a void pointer. On the first call the void pointer can be
	NULL. On subsequent calls, this should contain the address that the user has completed
	processing up to. The function can change the value of the bottom pointer to renormalize the
	circular buffer, so it is doubly referenced. The return value is a pointer to the top of the
	available buffer space. For example:
    	\code
	void *bottom=NULL, *top=NULL;
    	top = dag_advance_stream(dagfd, 0, &bottom);
	\endcode
	Assuming the buffer is mapped into user space at 10000, bottom will now contain 10000, and
	if 10000 bytes were received top would contain 20000. Processing can now begin for ERF
	records, starting at bottom (10000) and continue until you reach top (20000).
	If the first 5000 bytes are processed and it is then decided to call dag_advance_stream() again,
	the call would be:
  	\code
	bottom = bottom + 5000;
    	top = dag_advance_stream(dagfd, 0, &bottom);
	\endcode
	After this call bottom may still contain 15000, but top may be 25000 if a further 5000 bytes
	were received while process the initial 5000 bytes are being processed. If the circular buffer
	needs to be normalized, then bottom can have a lower value after calling dag_advance_stream() 
	than what was passed in. The process is always started from bottom. After calling dag_advance_stream() 
	the top pointer will always have a higher value than the bottom pointer. Further example code 
	is provided below.\n\n
	The function uses the stream poll parameters described under dag_get_stream_poll().
	These parameters define the blocking or non-blocking behavior, as well as the optional
	timeout and poll period. If not configured with dag_set_stream_poll() the default stream
	parameters will cause dag_advance_stream() to block when no data is available.

*/
uint8_t * dag_advance_stream(int dagfd, int stream_num, uint8_t ** bottom);

/**
@brief Receives errno from the DAG API.
@return An error code.
*/
int dag_get_last_error(void);

/**
@brief Sets a general attribute in the DAG API.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use if required otherwise -1.
@param param The parameter to be set.
@param value Pointer to the value the parameter should be given
@return 0 on success otherwise -1 with errno set appropriately.
*/
int dag_set_param(int dagfd, int stream_num, uint32_t param, void *value);

/* Subject to change */
/**
@brief retuns the daginf_t from the herd. 
@param dagfd A file descriptor for a DAG device.
@return retuns the daginf_t from the herd.
 */
daginf_t * dag_info(int dagfd);

/**
@brief returns the IO memory pointer
@param dagfd A file descriptor for a DAG device.
@return returns the IO memory pointer
 */
uint8_t * dag_iom(int dagfd);

/* libpcap helper */
/**\ingroup DagDeprecated
@brief Retrieves the ERF type of a DAG device. Obsoleted by dag_get_erf_types().
@param dagfd A file descriptor for a DAG device.
@return A single ERF type value, otherwise -1 with errno set appropriately.
 */
uint8_t dag_linktype(int dagfd);

/**
@brief Retrieves the number of ERF types associated with a DAG device.
@param dagfd A file descriptor for a DAG device.
@param erfs An array of bytes supplied by the user.
@param size Must be at least TYPE_MAX, as this allows all ERF types to be announced. Recommend users set size=255.
@return Number of types detected, otherwise -1 with errno set appropriately.
 */
int dag_get_erf_types(int dagfd, uint8_t * erfs, int size);
/**
@brief Get a 0 terminated list of ERF types that can be received from a stream
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param pointer to array of bytes to hold results
@param size of result list in bytes
@return min(size, number of types found)
 */
int dag_get_stream_erf_types(int dagfd, int stream_num, uint8_t * erfs, int size);
/**
@brief Provide a 0 terminated list of ERF types that may be sent to a transmit stream.
	Would only be needed on virtual streams, returns error on physical streams.
@param dagfd A file descriptor for a DAG device.
@param stream_num Which stream to use.
@param pointer to array of bytes holding types to set. 0 terminated.
@return number of types set on stream
 */
int dag_set_stream_erf_types(int dagfd, int stream_num, uint8_t * erfs);

/**
@brief Retrieves the number of interfaces on a card
@param dagfd A file descriptor for a DAG device.
@return Number of interfaces on a card, otherwise -1 with errno set appropriately.
 */
uint8_t dag_get_interface_count(int dagfd);


/**
@brief General helper function - here temporarily, will eventually be put in dagutil.c
 	name is the user-supplied string specifying a DAG device and stream.
        e.g. "/dev/dag0:2", "dag0:2", "0:2", "/dev/dag0", "dag0", "0", ""
 	The canonical version of the name ("/dev/dag0") and the stream number are 
        returned in buffer (which has length buflen) and stream_number respectively.
        Returns -1 and sets errno on failure (most likely because name couldn't be
        matched against any of the above templates).
@param name     name is the user-supplied string specifying a DAG device and stream. 
@param buffer   The canonical version of the name ("/dev/dag0") returned 
@param buflen   the length of the buffer.
@param stream_number the stream number returned.
@return 0 on success otherwise -1 with errno set appropriately.
 */
int dag_parse_name(const char * name, char * buffer, int buflen, int * stream_number);

/**
Retrieves the name of a DAG device
@param dagfd A file descriptor for a DAG device.
@return Name of the device as a character string
 */
char* dag_getname(int dagfd);

/*@}*/



/** \defgroup DagDeprecated Deprecated Functions
Deprecated Function declarations - provided for code compatibility only
 */
/*@{*/
/**
@brief Starts a measurement session on the nominated DAG. Obsoleted by dag_start_stream().
@param dagfd A file descriptor for a DAG device.
@return 0 on success otherwise -1 with errno set appropriately.
*/
int dag_start(int dagfd);

/**
@brief Stops a measurement session on the nominated DAG. Obsoleted by dag_stop_stream().
@param dagfd A file descriptor for a DAG device.
@return 0 on success otherwise -1 with errno set appropriately.
*/
int dag_stop(int dagfd);

/**
@brief  Returns an address in user space which corresponds to the base of the circular packet buffer as utilized by the DAG card. Obsoleted by dag_attach_stream().
@param dagfd A file descriptor for a DAG device.
@return returns and address in the user space which corrosponds to the base of the packet buffer
*/
void * dag_mmap(int dagfd);

/**
Returns the first address beyond the most recently written packet record in the circular buffer. Obsoleted by dag_advance_stream().
@param oldoffset Address of the previous offset as returned by the card or any other address the application wishes to be acknowledged as having completed the processing at.
@param flags The flag DAGF_NONBLOCK causes dag_offset to be non-blocking, otherwise the function blocks until at least one record is available.
@return 0 on success otherwise -1 with errno set appropriately. 
*/
int dag_offset (int dagfd, int * oldoffset, int flags);

/**
Reads the polling parameters in use. Obsoleted by dag_get_stream_poll().
@param mindata  The amount of data that must be received before a call to
		dag_advance_stream() or dag_rx_stream_next_record() will return is
		given by mindata.

@param maxwait  The maxwait parameter is the maximum amount of time the receive functions
	        should wait before returning.

@param poll     time for each polling.
*/
void dag_getpollparams(int * mindatap, struct timeval * maxwait, struct timeval * poll);

/**
Sets the polling parameters in use. Obsoleted by dag_set_stream_poll().
@param mindata  The amount of data that must be received before a call to
		dag_advance_stream() or dag_rx_stream_next_record() will return is
		given by mindata.

@param maxwait  The maxwait parameter is the maximum amount of time the receive functions
	        should wait before returning.

@param poll     time for each polling.
*/
void dag_setpollparams(int mindata, struct timeval * maxwait, struct timeval * poll);

/*
FIXME: Needs description!
@param dagfd A file descriptor for a DAG device.
@param minor FIXME: Needs description!
@return 0 on success otherwise -1 with errno set appropriately. FIXME?
*/
int dag_clone(int dagfd, int minor);
/*@}*/	// DagDeprecated


/** \ingroup DagAPI
Buffer size for use with dag_parse_name().
 */
#define DAGNAME_BUFSIZE 128

//void* GetMmapRegion(int iDagFd, int iStream);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DAGAPI_H */

/*@}*/	// defgroup DagAPI
