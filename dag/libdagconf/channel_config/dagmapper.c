/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dagmapper.c,v 1.4 2009/06/23 06:55:23 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         dagmapper.c
* DESCRIPTION:  This module implements an hdlc mapper api for the DAG3.7T.
*		It provides API functions to define channels and send frames,
*		and maps the frames into the raw transmit model according to the
*		channel definition for the frame, bit stuffing and 
*		calculating the FCS as needed.
*		The mapper is run as a thread to ensure that valid data is 
*		constantly transmitted on the defined channels.
*
* HISTORY:
*       19-07-04 DNH 1.0  Initial version derived from hdlcgen.c
*
**********************************************************************/

#include <stdio.h>
#if defined(__FreeBSD__) || defined(__linux__) || defined(__NetBSD__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <pthread.h>

#endif /* !_WIN32 */

#include "dagapi.h"
#include "dagmapper.h"
#include "dag37t_api.h"

#define DEBUG_IDLE_ERROR        /* actively detect 0xff errors in idle streams */

#define ULL     unsigned long long

/* Channel states */
#define CS_OK           0
#define CS_EOD          1       /* No more data for channel */
#define CS_REPEAT       2       /* Channel is repeating output data */

#define MAX_CHANNEL_PER_MODULE 512
#define MAX_LINES   16
#define MAX_DAGS    16

#define BURST_MAX	0x100000        /* 1 MB                              */

#define UNUSED_TIMESLOT_FILLER 0xFF

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif
#define BIN_ARGS(data) \
	((data) & (1 << 7)) != 0, \
	((data) & (1 << 6)) != 0, \
	((data) & (1 << 5)) != 0, \
	((data) & (1 << 4)) != 0, \
	((data) & (1 << 3)) != 0, \
	((data) & (1 << 2)) != 0, \
	((data) & (1 << 1)) != 0, \
	((data) & (1 << 0)) != 0

/* Channel types */
#if 0
#define CT_UNDEFINED    0       /* Undefined */
#define CT_CHAN         1       /* One timeslot */
#define CT_HYPER        2       /* Multiple timeslots */
#define CT_SUB          3       /* sub-timeslot */
#endif

/* Timeslots */
#define MAX_TIMESLOT    32      /* Max timeslots per line */
#define MAX_TS_CHAN     8       /* Max number of channels per timeslot */
#define MIN_LINE        0       /* minimum line id */

/* Need a per line solution here to support channelised vrs unchannelised E1,
   where timeslot 0 is only used for unchannelised
   e.g. num_timeslots[MAX_LINES]
*/
static int num_timeslots[MAX_LINES];

typedef struct s_framebuf
{
    uint16_t size;
    uint16_t offset;            /* Final bit offset after stuffing */
    struct s_framebuf *next;
    int error_ind;
    int fcs_error;
    void (*callback_sent) ();   /* Call back handler for sent frames */
    uint8_t data[7];
} t_framebuf;

#define FBUF_HEADER_SIZE 20

/* configuration and management info for each channel */
typedef struct s_channel
{
    int id;                     /* Channel id */
    uint32_t line;
    uint32_t timeslot;
    uint32_t format;            /* hdlc or ATM */
    uint32_t type;              /* channel, hyperchannel, or subchannel */
    uint32_t mask;              /* subchannel bitmask, or hyperchannel timeslot map */
    uint32_t ind;               /* current byte index into hdlc data stream */
    uint16_t size;              /* Size of subchannel */
    uint16_t state;             /* Channel state; active or out of data */
    uint8_t data_offset;        /* channel data offset, 0 to 7 bit right shift */
    uint8_t first_bit;          /* offset of first bit in subchannel mask */
    uint8_t cur_offset;         /* current subchannel bit offset in the current hdlc byte */
    uint8_t ones_count;         /* number of contiguous ones last sent */
    uint16_t bits;              /* bits to feed to subchannel */
    uint16_t bit_count;         /* number of bits available in bits field */
    uint32_t send_count;        /* number of frames sent */
    uint32_t frame_count;       /* number of frames in frame list */
    uint8_t last_byte;          /* last byte sent from channel */
    uint32_t cur_output_frame;
    uint16_t next_free_id;      /* For free channel list */
    struct s_channel *next;
    t_framebuf *head_framep;
    t_framebuf *tail_framep;
} t_channel;

/*
 * Timeslot to channel map.
 * Allows up to eight channels per timeslot.
 */

typedef struct
{
    int chan[8];                /* Channel list for the timeslot */
    int count;
} t_ts_chan_map;

/* packed erf frame matching file data format */
typedef struct {
    ULL timestamp;
    uint8_t type;
    uint8_t flags;
    uint16_t rlen;              /* Record Length */
    uint16_t loss_counter;
    uint16_t wlen;              /* Wire Length */
    uint16_t hdlc_flags;
    uint16_t hdlc_chan;
    uint8_t data[36];           /* padded to be 64-bit aligned */
} __attribute__ ((__packed__)) t_packed_mc_raw_erf;

typedef struct {
    ULL timestamp;
    uint8_t type;
    uint8_t flags;
    uint16_t rlen;              /* Record Length */
    uint16_t loss_counter;
    uint16_t wlen;              /* Wire Length */
    uint16_t hdlc_flags;
    uint16_t hdlc_chan;
    uint8_t data[28];           /* padded to be 64-bit aligned */
} __attribute__ ((__packed__)) t_packed_mc_raw_t1_erf;


/*
 * HDLC frame generation definitions
 */

#define HDLC_MAX_IDLE   20
#define HDLC_FCS_SIZE   2
#define HDLC_FLAG       0x7E
#define HDLC_MAX_SIZE   4096
#define HDLC_MIN_SIZE 10
#define HDLC_MAX_FRAME_COUNT 9500

/* Data generation schemes */
#define CT_PAY_INCREMENT 1
#define CT_PAY_RANDOM    2
#define CT_PAY_LFSR      3

#define LFSR_ONE  0x40
#define LFSR_TWO  0x01
#define LFSR_SEED 0xFF


/*
 * FCS calculation from RFC1662 
 */

/*
* FCS lookup table as calculated by the table generator.
*/
static uint16_t fcstab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,

    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

/* Number of contiguous ones in 5 bits, right to left, from bit 4 */
static uint8_t right_ones_total[] = {
    0,                          /* 0x00 00000 */
    0,                          /* 0x01 00001 */
    0,                          /* 0x02 00010 */
    0,                          /* 0x03 00011 */
    0,                          /* 0x04 00100 */
    0,                          /* 0x05 00101 */
    0,                          /* 0x06 00110 */
    0,                          /* 0x07 00111 */
    0,                          /* 0x08 01000 */
    0,                          /* 0x09 01001 */
    0,                          /* 0x0A 01010 */
    0,                          /* 0x0B 01011 */
    0,                          /* 0x0C 01100 */
    0,                          /* 0x0D 01101 */
    0,                          /* 0x0E 01110 */
    0,                          /* 0x0F 01111 */
    1,                          /* 0x10 10000 */
    1,                          /* 0x11 10001 */
    1,                          /* 0x12 10010 */
    1,                          /* 0x13 10011 */
    1,                          /* 0x14 10100 */
    1,                          /* 0x15 10101 */
    1,                          /* 0x16 10110 */
    1,                          /* 0x17 10111 */
    2,                          /* 0x18 11000 */
    2,                          /* 0x19 11001 */
    2,                          /* 0x1A 11010 */
    2,                          /* 0x1B 11011 */
    3,                          /* 0x1C 11100 */
    3,                          /* 0x1D 11101 */
    4,                          /* 0x1E 11110 */
    5,                          /* 0x1F 11111 */
};

typedef struct
{
    uint8_t data;
    uint8_t stuff_count;
} t_stuffed_byte[256][6];

/* Table of byte stuffings based on byte value and the number
   of previous set bits 
*/
static t_stuffed_byte right_stuffed_byte;
#define STUFFED_BYTE right_stuffed_byte
#define ONES_TOTAL(data) (right_ones_total[((data) >> 3)])

/* Mapping from user interface numbers to physical interface numbers */
static int ste_physical[MAX_INTERFACES] = {
    11,
    14,
    12,
    9,
    4,
    1,
    7,
    2,
    10,
    15,
    13,
    8,
    5,
    0,
    6,
    3
};

/* Need these per line.... */
static uint32_t min_timeslot = 1;
static uint32_t max_timeslot = 31;

static int verbose = 2;         /* verbose output */
static uint32_t debug = 0;

static uint32_t global_erf_number = 0x42000000;	/* Debug tracking map between erf and raw erf */


#define MAX_DAG_NAME 128
/* DAG management structures */
typedef struct
{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	pthread_mutex_t mutex;      /* Protection for channel access, etc */
    pthread_t thread;
    pthread_attr_t thread_attr;
#elif defined (_WIN32)
    HANDLE thread;
    DWORD thread_id;
    CRITICAL_SECTION mutex;
#endif                          /* _WIN32 */
    int thread_error;
    int dagfd;
    char device[MAX_DAG_NAME];
    uint32_t burst_size;
    uint32_t chancount;
    uint32_t burst_count;
    uint32_t erf_count;
    uint32_t delay;
    int num_timeslots;	/* 24, 31, 32 */
    int raw_erf_size;
    int run;
    int running;
    uint32_t frame_level;       /* Number of frames pending to be sent */
    uint16_t free_chan_id;      /* Free list for channel allocation */
    t_channel *head_chanp;
    t_channel *tail_chanp;
    t_ts_chan_map tcmap[MAX_LINES][MAX_TIMESLOT];
    t_channel channel[MAX_CHANNEL_PER_MODULE];     /* Channel management for all channels */
} t_dag_mapper;

static t_dag_mapper dm_dag[MAX_DAGS];

static int dm_repeat_mode = 0;
static int dm_dag_count = -1;

static FILE *erffile = NULL;

/* Table to reverse bit order of a byte */
static uint8_t reversed_byte[256];

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define STATE_FLAGS     1
#define STATE_IN_FRAME  2
#define STATE_EOF       3

/* CRC-8 from
(*) N. M. Abramson, A Class of Systematic Codes for Non-Independent
Errors, IRE Trans. Info. Theory, December 1959.
30/08/04 extended to also include CRC-10 caclulations.
*/
#define HEC_GENERATOR		0x107		/* x^8 + x^2 +  x  + 1  */
#define COSET_LEADER		0x055		/* x^6 + x^4 + x^2 + 1  */
#define CRC10_POLYNOMIAL	0x633		/* x^10 + x^9 + x^5 + x^4 + x + 1 */
#define CRC_TABLE_SIZE		256
#define MASK_ALL_BITS		0xff
static unsigned char crc8_table[CRC_TABLE_SIZE];
static unsigned short crc10_table[CRC_TABLE_SIZE];

#define ATM_CELL_SIZE 53
#define ATM_HEADER_SIZE 5

static void generate_crc_table()
/* generate a table of CRC-8 and CRC-10 syndromes for all possible input bytes */
{
    int i, j, temp;
    unsigned short crc10;

    for (i = 0;  i < CRC_TABLE_SIZE;  i++)
    {
	temp = i;
	crc10 = ((unsigned short)i << 2);

	for (j = 0;  j < 8;  j++)
	{
	    if (temp & 0x80)
	    {
		temp = (temp << 1) ^ HEC_GENERATOR;
	    }
	    else
	    {
		temp = (temp << 1);
	    }

	    if ((crc10 <<= 1) & 0x400)
	    {
		    crc10 ^= CRC10_POLYNOMIAL;
	    }
	}
	crc8_table[i] = (unsigned char)temp;
	crc10_table[i] = crc10;
    }
}


#if 0
static unsigned short generate_crc10_check(uint8_t *framep)
{
/* calculate the CRC-10 remainder over the whole payload
Credit to Charles Michael Heard of heard@vvnet.com for crc-10
calculation example and polynomial*/

    unsigned short crc10_accum = 0;
    int i;

    /* ensure bytes to hold the CRC are zero before calculation*/
    framep[ATM_CELL_SIZE-1] = framep[ATM_CELL_SIZE-2] = 0;

	for ( i = ATM_HEADER_SIZE;  i < (ATM_CELL_SIZE);  i++ )
    {
		crc10_accum = ((crc10_accum << 8) & 0x3ff)
                      ^ crc10_table[( crc10_accum >> 2) & MASK_ALL_BITS]
                      ^ framep[i];
    }
    return crc10_accum;
}
#endif


/* calculate CRC-8 remainder over first four bytes of cell header   */
/* exclusive-or with coset leader & insert into fifth header byte   */
static void atm_insert_hec(uint8_t *framep)
{
    unsigned char hec_accum = 0;
    int i;
    
    for (i = 0;  i < 4;  i++)
    {
        hec_accum = crc8_table[hec_accum ^ framep[i]];
    }
    framep[4] = hec_accum ^ COSET_LEADER;
}


static void atm_initialise()
{
    generate_crc_table();
}
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
/**********************************************************************
* FUNCTION:     dm_lock(dmap)
* DESCRIPTION:  Obtain the mutex lock on the specified dag map structure.
*		Used to protect thread access to the various tables and lists.
* INPUTS:       dmap	- mapper reference to lock
* RETURNS:	0 - ok, current thread has the lock
*		-1 - lock failed.  See pthread_mutex_lock().
**********************************************************************/
static int
dm_lock(t_dag_mapper * dmap)
{
    int res;

    res = pthread_mutex_lock(&dmap->mutex);
    if (res)
    {
        fprintf(stderr, "dm_lock: pthread_mutext_lock failed, err=%d\n", res);
    }
    return res;
}


/**********************************************************************
* FUNCTION:     dm_unlock(dmap)
* DESCRIPTION:  Release the mutex lock on the specified dag map structure.
*		Used to protect thread access to the various tables and lists.
* INPUTS:       dmap	- mapper reference to lock
* RETURNS:	0 - ok
*		-1 - lock failed.  See pthread_mutex_lock().
**********************************************************************/
static int
dm_unlock(t_dag_mapper * dmap)
{
    int res;

    res = pthread_mutex_unlock(&dmap->mutex);
    if (res)
    {
        fprintf(stderr, "dm_unlock: pthread_mutext_lock failed, err=%d\n", res);
    }
    return res;
}
#elif defined (_WIN32)
/* Use critical sections for Windows mutex */
#define dm_lock(dmap)	0; EnterCriticalSection(&((dmap)->mutex))
#define dm_unlock(dmap)	0; LeaveCriticalSection(&((dmap)->mutex))
#endif /* _WIN32 */


/**********************************************************************
* FUNCTION:     dm_getNextChanByte(dmap, chanp, active)
* DESCRIPTION:  Get the next 8 bits of data from the specified channel.
*		If there are no pending frames for the channel then 
*		HDLC_IDLE is returned with the current channel offset.
* INPUTS:       dmap	- mapper reference
*               chanp	- channel to return data from
* OUTPUTS:	*active - set to 1 if the channel has data, unchanged if not
* RETURNS:	next 8 bits of data from the channel
**********************************************************************/
static uint8_t
dm_getNextChanByte(t_dag_mapper * dmap, t_channel * chanp, int *active)
{
    uint8_t data = 0;
    t_framebuf *framep;
    static int out_ind = 0;

    /* protect frame list access before getting the next frame */
    framep = chanp->head_framep;

    /* Got data to send? */
    if (framep) {
	*active = 1;
	if ((verbose > 1) && (chanp->ind == 0)) {
	    printf("chan %d starting new frame %d of %d\n", chanp->id, chanp->send_count, chanp->frame_count);
	}
        if (verbose > 1) {
            if (chanp->state == CS_EOD) {                   /* XXX DEBUG */
                chanp->state = CS_OK;
                printf("chan %d new frame: ind=%d, data_offset=%d\n",
                       chanp->id, chanp->ind, chanp->data_offset);
            }
        }
        data = chanp->last_byte | (framep->data[chanp->ind] << chanp->data_offset);
        if (verbose > 2) {
            printf("%d: chan[%d][%d.%d](data=0x%02x->%02x, last=0x%02x) -> 0x%02x",
                 out_ind++, chanp->id, chanp->ind, chanp->data_offset,
                 framep->data[chanp->ind], chanp->last_byte,
                 (framep->data[chanp->ind] >> (8 - chanp->data_offset)) & 0xFF, data);
            printf(" %d%d%d%d %d%d%d%d\n", BIN_ARGS(data));

        }

        chanp->last_byte = framep->data[chanp->ind] >> (8 - chanp->data_offset);
        chanp->ind++;

        /* Get next data */
        /* Q. At end of frame? */
        if (((signed) chanp->ind >= (framep->size - 1)) &&
            !((framep->offset == 0) && (chanp->ind == (framep->size - 1))) &&
            !((chanp->ind == (framep->size - 1)) && ((chanp->data_offset + framep->offset) >= 8))) {

            if (framep->offset) {
                /* Save remaining bits for next byte */
                if ((chanp->data_offset + framep->offset) < 8)
                    chanp->last_byte |= (framep->data[chanp->ind] << chanp->data_offset);

                /* Update channel offset to deal with next frame */
                chanp->data_offset = (chanp->data_offset + framep->offset) & 0x07;
            }
            if (verbose > 2)
                printf("chan[%d][%d.%d] eof last=0x%02x->0x%02x, offset +%d (frame %d)\n",
                     chanp->id, chanp->ind, chanp->data_offset,
                     framep->data[chanp->ind], chanp->last_byte, framep->offset, chanp->send_count);

            /* Reset data read index for next frame */
            chanp->ind = 0;

            dm_lock(dmap);

            /* Get next frame */
            chanp->head_framep = framep->next;
            chanp->send_count++;
	    global_erf_number = chanp->send_count;

	    if (verbose > 2) printf("chan %d: Frame %d\n", chanp->id, chanp->send_count);

            /* Keep repeating the frames? */
            if (dm_repeat_mode) {
                if (verbose > 2)
                    printf("Repeating frame on channel %d\n", chanp->id);
                chanp->tail_framep->next = framep;
                chanp->tail_framep = framep;
                framep->next = NULL;
            }

            if (chanp->head_framep == NULL) {
                chanp->tail_framep = NULL;
                chanp->state = CS_EOD;
                if (verbose > 2)
                    printf("chan[%d] end of data\n", chanp->id);
            }

            if (!dm_repeat_mode) {
                /* Done with frame */
                chanp->frame_count--;
                if (framep->callback_sent)
                    framep->callback_sent();
                free(framep);
                dmap->frame_level--;    /* track level for all channels */
                dm_unlock(dmap);
            } else {
                dm_unlock(dmap);
                if (framep->callback_sent)
                    framep->callback_sent();
            }

        }
    } else {
	/* Insert HDLC flag for HDLC channels */
	if (chanp->format == CT_FMT_HDLC) {
	    data = (HDLC_FLAG << chanp->data_offset) | chanp->last_byte;
	    chanp->last_byte = HDLC_FLAG >> (8 - chanp->data_offset);
	    if (verbose > 5)
		printf("chan[%d][0.%d] idle sending flag 0x%02x last=0x%02x\n",
		       chanp->id, chanp->data_offset, data, chanp->last_byte);
#ifdef DEBUG_IDLE_ERROR
	    if (data == 0xFF)
	    {
		printf
		    ("ERROR: chan[%d][0.%d] idle sending flag 0x%02x last=0x%02x\n",
		     chanp->id, chanp->data_offset, data, chanp->last_byte);
	    }
#endif
	}
    }

#ifdef DEBUG_IDLE_ERROR
    if ((chanp->format == CT_FMT_HDLC) && 
	    (chanp->type == CT_CHAN || chanp->type == CT_HYPER || chanp->type == CT_SUB)) {
        if (data == 0xFF) {
            printf("ERROR: chan[%d][0.%d] sending abort! 0x%02x last=0x%02x\n",
                   chanp->id, chanp->data_offset, data, chanp->last_byte);
        }
    }
#endif
    return data;
}


/**********************************************************************
* FUNCTION:     int dm_getTimeslotData(ts, map, active)
* DESCRIPTION:  Uses the channel map to obtain 8 bits of data for the
*               timeslot.  As a side effect the HDLC data feed for the
*               channel or channels is incremented.  If there is no more
*               data to feed the timeslot then -1 is returned.
* INPUTS:       ts  - timeslot to return data for
*               map - timeslot to channel map to obtain data from
* OUTPUTS:      *active - boolean true if there is unrepeated data available
*                         for the timeslot
* RETURNS: >= 0 - 8-bits of timeslot data
*          <0   - no more HDLC data for the timeslot
**********************************************************************/
static int dm_getTimeslotData(
    t_dag_mapper *dmap,
    uint32_t ts,
    t_ts_chan_map *map,
    int *active)
{
    uint8_t data = 0;
    t_channel *chanp;
    int ind;
    int data_valid = 0;
    int chan_active=0;

    if (verbose > 4) {
	printf("getTimeslotData: ts=%d\n", ts);
    }

    /* Fill in the data from each channel */
    for (ind = 0; ind < map->count; ind++)
    {
        chanp = &dmap->channel[map->chan[ind]];
        /* we're active if at least one channel is still processing its first round of data */
        /* (With the CS_REPEAT mode we'll always be active here, i.e. always return valid data) */
        data_valid = 1;

	if (verbose > 4) {
	    printf("getTimeslotData: ts=%d chan=%d\n", ts,chanp->id);
	}

        switch (chanp->type)
        {

        case CT_HYPER_RAW:
        case CT_CHAN_RAW:
        case CT_HYPER:
        case CT_CHAN:
            data = dm_getNextChanByte(dmap, chanp, &chan_active);
            if (verbose > 1)
                printf("\tchan[%d:%d] -> 0x%02x (active=%d)\n",
                     chanp->id, chanp->size, data, chan_active);

            break;

        case CT_SUB:
        case CT_SUB_RAW:
            /* Do we have enough data available? */
            if (chanp->bit_count < chanp->size)
            {
                chanp->bits |= dm_getNextChanByte(dmap, chanp, &chan_active) << chanp->bit_count;
                chanp->bit_count += 8;
            }

            /* Add the subchannel data to the current timeslot data */
            data |= (chanp->bits & chanp->mask) << chanp->first_bit;

            if (verbose > 1)
                printf("\tsubchan[%d:%d<<%d] -> 0x%02x (bits %04x.%d, mask 0x%02x, active=%d)\n",
                     chanp->id, chanp->size, chanp->first_bit, data,
                     chanp->bits, chanp->bit_count, chanp->mask, chan_active);

            chanp->bits >>= chanp->size;
            chanp->bit_count -= chanp->size;
            break;

        default:
            fprintf(stderr,
                    "Error got invalid channel type for line %d ts %d\n",
                    chanp->line, chanp->timeslot);
            exit(1);
        }

	/* Timeslot is active if at least one channel is active */
	if (chan_active) {
	    *active = 1;
	} else {
	    if (verbose > 1) {
		printf("chan[%d] is inactive\n",chanp->id);
	    }
	}
    }

    if (data_valid)
        return data;
    else
        return -1;              /* no channels active for the timeslot */
}

#if defined (_WIN32)
#define htons(data)	((((uint16_t)(data)) >> 8) | (((uint16_t)(data)) << 8))
#endif /* _WIN32 */

/**********************************************************************
* FUNCTION:     send_erfs(dmap, buffer, count, lines, timeslots) 
* DESCRIPTION:  Send a burst of count raw erfs to the board.
*		The erfs are built directly in the memory hole beginning
*		at the location pointed to by buffer.
* INPUTS:       dmap	  - mapper to send erfs via.
*		buffer	  - start of buffer in the transmit memory hole.
*		count	  - number of raw erfs to send.
*			    Note: count must be >= lines
*		lines	  - total number of lines to generate erfs for
*		timeslots - number of timeslots to fill in the erf.
* RETURNS:      1 	- still sending active data
*		0	- no active data sent, all channels generating idle
**********************************************************************/
static int
send_erfs(t_dag_mapper * dmap, void *buffer, uint32_t count, uint32_t lines, uint32_t timeslots)
{
    uint32_t ts;
    int channel_active = 0;
    uint32_t erf_count = 0;     /* Number of erfs sent */

    static int line = 0;
    t_packed_mc_raw_erf *erf;		/* Note:  actual wlen and rlen may be smaller (e.g. in T1 mode) */

    if (count < lines)
        count = lines;

    if (verbose)
        printf("Writing %d erfs\n", count);

    /* Keep going as long as there are channels with data */
    erf = (t_packed_mc_raw_erf *) buffer;
    for (erf_count = 0; erf_count < count; erf_count++, erf++, line = (line + 1) % lines)
    {
        erf->timestamp = 0;
        erf->type = 6;
        erf->flags = 4;         /* vlen = 1 */
        erf->rlen = htons(dmap->raw_erf_size);
        erf->loss_counter = 0;
        erf->wlen = htons(timeslots);
        erf->hdlc_flags = 0;
        erf->hdlc_chan = htons(line);

        for (ts = min_timeslot; ts <= max_timeslot; ts++)
        {
            /* Get 8 bits of data for the timeslot */
            if (dmap->tcmap[line][ts].count)
            {                   /* Q. Any channels assigned to this timeslot? */
                int data;
                data = dm_getTimeslotData(dmap, ts, &dmap->tcmap[line][ts], &channel_active);
                if (data < 0)
                {
                    /* Channel went inactive */
                    if (verbose > 1)
                        printf("%d:%d inactive\n", line, ts);
                    printf("%d:%d inactive\n", line, ts);
                    // dmap->tcmap[line][ts].count = 0;
                    data = 0;
                    erf->data[ts - min_timeslot] = UNUSED_TIMESLOT_FILLER;
                    continue;
                }
                erf->data[ts - min_timeslot] = data;

            }
            else
            {
                /* Timeslot unprovisioned */
                erf->data[ts - min_timeslot] = UNUSED_TIMESLOT_FILLER;
            }
        }

	/* debug - stamp the current erf frame number in the raw frame */
	*(uint32_t *)&erf->data[32] = global_erf_number; 
    }

    return channel_active;
}


/**********************************************************************
* FUNCTION:     dm_mapChannel(dmap, chanp)
* DESCRIPTION:  Update the timeslot map with the specified channel.
*		The timeslot map is a table specifying the list of
*		channels feeding data to each timeslot.
* INPUTS:       dmap	  - mapper to map the channel on.
*		chanp	  - the channel to map.
* RETURNS:      0  - success
*		-1 - invalid channel type
*		-2 - hyperchannel mapped onto already mapped timeslot
*		-3 - too many channels assigned to timeslot (max=8).
**********************************************************************/
static int
dm_mapChannel(t_dag_mapper * dmap, t_channel * chanp)
{
    t_ts_chan_map *tcp;

    /*
     * Organise channels by timeslots
     */

    if (verbose)
        printf("mapping chan %d line %d timeslot %d\n", chanp->id, chanp->line, chanp->timeslot);

    if (dmap->tcmap[chanp->line][chanp->timeslot].count == 8)
    {
        /* Arg! */
        fprintf(stderr,
                "Error: too many channels assigned to line %d ts %d\n",
                chanp->line, chanp->timeslot);
        return -3;
    }
    if (verbose)
        printf("chan %d: type=%d line=%d ts=%d count=%d\n",
               chanp->id, chanp->type, chanp->line, chanp->timeslot,
               dmap->tcmap[chanp->line][chanp->timeslot].count);

    switch (chanp->type)
    {
    case CT_SUB:
    case CT_CHAN:
    case CT_CHAN_RAW:
    case CT_SUB_RAW:
        /* One-to-one mapping between channel and timeslot */
        if (verbose > 1)
            printf("ts[%d]: Adding subchan %d (count %d)\n",
                   chanp->timeslot, chanp->id, dmap->tcmap[chanp->line][chanp->timeslot].count);
        tcp = &dmap->tcmap[chanp->line][chanp->timeslot];
        tcp->chan[tcp->count++] = chanp->id;
        break;

    case CT_HYPER:
    case CT_HYPER_RAW:
        /* One-to-many mapping between channel and timeslot */
        {
            int ts;
            /* Map the channel to each timeslot */
            for (ts = 0; ts < MAX_TIMESLOT; ts++)
            {
                tcp = &dmap->tcmap[chanp->line][ts];
                if (chanp->mask & (1 << ts))
                {
                    if (verbose > 1)
                    {
                        printf("ts[%d]: Adding hyperchan %d (count %d)\n",
                               ts, chanp->id, tcp->count);
                    }
                    /* Sanity check */
                    if (tcp->count != 0)
                    {
                        fprintf(stderr,
                                "Error ts %d:%d count at %d for hyperchannel\n",
                                chanp->line, ts + 1,
                                dmap->tcmap[chanp->line][chanp->timeslot].count);
                        return -2;
                    }
                    tcp->chan[tcp->count++] = chanp->id;
                }
            }
        }
        break;

    default:
        fprintf(stderr, "Invalid channel type %d, channel %d\n", chanp->type, chanp->id);
        return -1;
    }

    return 0;
}


/**********************************************************************
* FUNCTION:     dm_unmapChannel(dmap, chanp)
* DESCRIPTION:  Remove a channel from the timeslot map.
*		The timeslot map is a table specifying the list of
*		channels feeding data to each timeslot.
* INPUTS:       dmap	  - mapper to remove the channel from.
*		chanp	  - the channel to remove.
* RETURNS:      0  - success
*		-1 - invalid channel type
*		-2 - channel not mapped
**********************************************************************/
static int
dm_unmapChannel(t_dag_mapper * dmap, t_channel * chanp)
{
    t_ts_chan_map *tcp;
    int ind;

    /*
     * Organise channels by timeslots
     */

    printf("unmapping chan %d line %d timeslot %d\n", chanp->id, chanp->line, chanp->timeslot);

    if (verbose)
        printf("chan %d: type=%d line=%d ts=%d count=%d\n",
               chanp->id, chanp->type, chanp->line, chanp->timeslot,
               dmap->tcmap[chanp->line][chanp->timeslot].count);

    switch (chanp->type)
    {
    case CT_SUB:
    case CT_CHAN:
    case CT_CHAN_RAW:
    case CT_SUB_RAW:
        /* One-to-one mapping between channel and timeslot */
        if (verbose > 1)
            printf("ts[%d]: removing subchan %d (count %d)\n",
                   chanp->timeslot, chanp->id, dmap->tcmap[chanp->line][chanp->timeslot].count);

        tcp = &dmap->tcmap[chanp->line][chanp->timeslot];

        for (ind = 0; ind < tcp->count; ind++)
        {
            if (tcp->chan[ind] == chanp->id)
            {
                if (ind < 7)
                {
                    /* fill in the channel with the last active channel */
                    tcp->chan[ind] = tcp->chan[tcp->count - 1];
                }
                tcp->count--;
                return 0;
            }
        }

        /* Error - channel not found */
        fprintf(stderr, "dm_unmapChannel() error: channel %d not found\n", chanp->id);
        return -2;
        break;

    case CT_HYPER:
    case CT_HYPER_RAW:
        /* One-to-many mapping between channel and timeslot */
        {
            int ts;
            /* unmap the channel to each timeslot */
            for (ts = 0; ts < MAX_TIMESLOT; ts++)
            {
                tcp = &dmap->tcmap[chanp->line][ts];
                if (chanp->mask & (1 << ts))
                {
                    if ((tcp->count != 1) || (tcp->chan[0] != chanp->id))
                    {
                        fprintf(stderr,
                                "dm_unmapChannel() error: channel %d not found\n", chanp->id);
                        return -2;
                    }
                    if (verbose > 1)
                    {
                        printf("ts[%d]: Removing hyperchan %d (count %d)\n",
                               ts, chanp->id, tcp->count);
                    }
                    tcp->count--;
                }
            }
        }
        break;

    default:
        fprintf(stderr, "Invalid channel type %d, channel %d\n", chanp->type, chanp->id);
        return -1;
    }

    return 0;
}


/**********************************************************************
* FUNCTION:     dm_add_frame(dmap, chanp, framep)
* DESCRIPTION:  Queue an HDLC frame to be sent on the specified channel.
* INPUTS:       dmap	  - mapper to send the frame on.
*		chanp	  - channel to send the frame on.
*		framep	  - the frame to send.
* RETURNS:      0  - success
*		-1 - failure
**********************************************************************/
static int
dm_add_frame(t_dag_mapper * dmap, t_channel * chanp, t_framebuf * framep)
{
    int res;
    static uint32_t total_frame_count = 1;

    if (verbose > 2)
        printf("Adding frame %d\n", total_frame_count++);

    /* Confirm channel exists */
    if (chanp->type == CT_UNDEFINED)
    {
        fprintf(stderr, "add_frame() error: channel %d not defined\n", chanp->id);
        return -1;
    }

    /* Protect access to the channel frame lists */
    res = dm_lock(dmap);
    if (res)
    {
        fprintf(stderr, "dm_add_frame: pthread_mutext_lock failed, err=%d\n", res);
        return -1;
    }

    if (chanp->tail_framep == NULL)
    {
        chanp->head_framep = framep;
        chanp->frame_count++;
    }
    else
    {
        chanp->tail_framep->next = framep;
        chanp->frame_count++;
    }
    chanp->tail_framep = framep;
    dmap->frame_level++;        /* track level for all channels */

    /* Unprotect access to the channel frame lists */
    res = dm_unlock(dmap);
    if (res)
    {
        fprintf(stderr, "dm_add_frame: pthread_mutext_unlock failed, err=%d\n", res);
        return -1;
    }


    if (verbose > 2)
        printf("added frame for channel %d\n", chanp->id);

    return 0;
}


/**********************************************************************
* FUNCTION:     stuff_byte(in_byte, last_data, ones_count, increment, offset)
* DESCRIPTION:  Bit-stuff the in_byte based on the previous number of 
*		contiguous ones seen and the number of contiguous ones
*		in the byte.  The byte is shifted to match the current
*		offset in the HDLC bitsream, or'd with the left over bits
*		from the previous byte (last_data), and finally bit stuffed.
*		The HDLC protocol requires a zero bit to be stuffed
*		after any sequence of five ones.
*		Any new left over bits are returned in last data.
* INPUTS:       in_byte	   - the byte to stuff
*		last_data  - The remaining bits from the previous bit stuffing
*		ones_count - The number of contiguous ones in the bitstream
*			     (sent ahead of this byte).
*	        offset	   - current bit offset in the HDLC bit stream
* OUTPUTS:	last_data  - Updated with the left over bits from the in_byte
*		 	     based on the bit offset.
*		ones_count - Updated with the current octets ones count in 
*			     the returned byte.
* 		increment  - used to manage bit offset overflow.  If the
*			     offset increments beyond 7 bits then the source
*		 	     data index of the caller should not be incremented.
*		             0 = don't increment source data index.
*			     1 = increment source data index.
*	        offset	   - incremented once per bit stuffed to reflect
*			     the new offset for the next byte.
* RETURNS:      the next 8 bits to send in the hdlc stream.
**********************************************************************/
static uint8_t
stuff_byte(uint8_t in_byte, uint8_t * last_data, int *ones_count, int *increment, int *offset)
{
    uint8_t data, mask;
    uint16_t sc;

    /* read in a byte, shifted by the channel data offset */
    mask = 0xff >> *offset;
    data = ((in_byte & mask) << *offset) | *last_data;

    /* Bit stuff it */
    sc = STUFFED_BYTE[data][*ones_count].stuff_count;
    *offset += sc;

    if (*offset <= 8) {
	*last_data = in_byte >> (8 - *offset);      /* Save data for next round */
    } else {
	/* Preserve the extra bits in last data before stuffing it - need it for the next time */
	*last_data = data >> (7 - (*offset - 8));
    }

    data = STUFFED_BYTE[data][*ones_count].data;

    *ones_count = ONES_TOTAL(data);

    if (*offset > 7)
    {
        *offset -= 8;
        *increment = 0;
    }
    else
    {
        *increment = 1;
    }

    return (data);
}


/**********************************************************************
* FUNCTION:	dm_get_mapper(dagfd)
* DESCRIPTION:	Returns a pointer to the mapper management data for the 
*		specified dag device.
* INPUTS:	dagfd	- device to find mapper for.
* RETURNS:	NULL	- mapper not found
*		else pointer to the mapper for the device.
**********************************************************************/
/* XXX make this thread-safe for searching */
static t_dag_mapper *
dm_get_mapper(int dagfd)
{
    int ind, count;

    /* basically hashed into the list and then searched sequencially */
    for (ind = (dagfd % MAX_DAGS), count = MAX_DAGS;
         count && (dm_dag[ind].dagfd != dagfd); count--, ind = (ind + 1) % MAX_DAGS);

    if (dm_dag[ind].dagfd != dagfd)
    {
        return NULL;
    }

    return &dm_dag[ind];
}


/**********************************************************************
* FUNCTION:	dag_send_debug_frame(dagfd, chanId, datap, len, error_ind, fcs_error, options, callback_sent)
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
*		callback_sent	- optional callback handler invoked after the frame
*				  has actually been sent.
* RETURNS:	0 - success
*		<0 - failure.
**********************************************************************/
int dag_mapper_send_debug_frame(
    int dagfd,
    int uchanId,
    uint8_t * datap,
    int len,
    int error_ind,
    int fcs_error,
    uint32_t options,
    void (*callback_sent)() )
{
    uint16_t chanId = DM_CHANID(uchanId);
    t_framebuf *framep;
    int offset;
    int rc = 0;
    uint8_t last_data;
    int ind, find;
    uint32_t dind = 0;
    int ones_count;
    uint8_t fcs_data[2];
    uint16_t fcs;
    int increment;
    uint32_t flag_count = 6;
    uint32_t max_frame_size;
    t_dag_mapper *dmap = dm_get_mapper(dagfd);
    int do_bit_stuffing = 0;

    if (verbose > 2)
        printf("send_frame(chan=%d, len=%d, err=%d, fcs=%d)\n", chanId, len, error_ind, fcs_error);

    /* Confirm channel exists */
    if (dmap->channel[chanId].type == CT_UNDEFINED)
    {
        fprintf(stderr, "send_frame() error: channel %d not defined\n", chanId);
        return -1;
    }

    if (dmap->channel[chanId].type == CT_CHAN_RAW ||
        dmap->channel[chanId].type == CT_HYPER_RAW || dmap->channel[chanId].type == CT_SUB_RAW)
    {
        do_bit_stuffing = 0;
    }
    else
    {
        do_bit_stuffing = 1;
    }

    /* Q. Should we limit the number of frames allowed to pend per channel? */

    /* Get space for the frame, including max stuffing scenario (all 0xFF) */
    /* (len/5) approximates max extra bits per byte */
    max_frame_size = FBUF_HEADER_SIZE + len + (len / 4) + 5 + flag_count;
    framep = (t_framebuf *) malloc(sizeof(t_framebuf) + max_frame_size);

    if (framep == NULL)
    {
        fprintf(stderr, "Failed to allocated %d bytes for frame\n", max_frame_size);
        perror("Malloc failed: ");
        return (-1);
    }

    framep->next = NULL;
    framep->error_ind = error_ind;
    framep->fcs_error = fcs_error;

    /* Treat the data as being sent out LSB first for stuffing.  */

    for (dind = 0; dind < flag_count; dind++)
    {
        framep->data[dind] = HDLC_FLAG; /* Flag at start of frame */
    }

    if (dmap->channel[chanId].head_framep == NULL)
    {
        /* Debug - add an extra flag before first frame to match hdlcgen output */
        framep->data[dind++] = HDLC_FLAG;
    }

    /* Do the bit stuffing here (could change to stuff on the fly in dm_getTimeslotData()) */
    last_data = 0;              /* Left over bits from last round - none to start */
    ones_count = 0;
    offset = 0;
    fcs = 0xFFFF;
    increment = 1;
    for (ind = 0; ind < len; dind++, ind += increment)
    {
        if (do_bit_stuffing)
        {
            framep->data[dind] =
                stuff_byte(datap[ind], &last_data, &ones_count, &increment, &offset);
        }
        else
        {
            framep->data[dind] = datap[ind];
        }

        if (increment)
            fcs = (fcs >> 8) ^ fcstab[(fcs ^ datap[ind]) & 0xff];

        if (verbose > 2)
        {
            printf(" frame[%d.0] = 0x%02x -> [%d.%d]=0x%02x (0x%02x) (oc %d) ld 0x%02x",
                 ind, datap[ind], dind, offset, framep->data[dind],
                 datap[ind], ones_count, last_data);
            printf(" %d%d%d%d %d%d%d%d -> %d%d%d%d %d%d%d%d\n",
                   BIN_ARGS(datap[ind]), BIN_ARGS(framep->data[dind]));
        }

    }

    /* Sanity check */
    if (dind > max_frame_size)
    {
        /* Arg */
        fprintf(stderr, "Fatal error: wrote %d bytes into %d byte frame\n", dind, max_frame_size);
        exit(0);
    }

    /* fcs = fcs16( 0xffff, &hdlc_buf[ind], frame_size); */
    fcs ^= 0xffff;              /* complement */

    /* Generate an FCS error? */
    if (framep->fcs_error)
    {
        fcs += 0x4242;
    }

    /* Add on FCS */
    fcs_data[0] = (uint8_t) (fcs & 0xFF);       /* low */
    fcs_data[1] = (uint8_t) (fcs >> 8); /* high */

    for (find = 0; find < 2; dind++, find += increment)
    {
        if (do_bit_stuffing)
        {
            framep->data[dind] =
                stuff_byte(fcs_data[find], &last_data, &ones_count, &increment, &offset);
        }
        else
        {
            framep->data[dind] = fcs_data[find];
        }
        if (verbose > 2)
        {
            printf
                (" frame[%d.0] = 0x%02x -> [%d.%d]=0x%02x (0x%02x) (oc %d) ld 0x%02x",
                 ind + find, fcs_data[find], dind, offset,
                 framep->data[dind], datap[ind], ones_count, last_data);
            printf(" %d%d%d%d %d%d%d%d -> %d%d%d%d %d%d%d%d\n",
                   BIN_ARGS(fcs_data[find]), BIN_ARGS(framep->data[dind]));
        }
    }

    if (offset || (ones_count == 5))
    {
        if (do_bit_stuffing)
        {
            framep->data[dind++] = stuff_byte(0, &last_data, &ones_count, &increment, &offset);
        }
        else
        {
            framep->data[dind++] = 0;
        }
        if (verbose > 2)
        {
            printf(" frame[eof.0]         [%d.%d]=0x%02x (oc %d) ld 0x%02x",
                   dind - 1, offset, framep->data[dind - 1], ONES_TOTAL(last_data), last_data);
            printf(" ---- ---- -> %d%d%d%d %d%d%d%d\n", BIN_ARGS(framep->data[dind - 1]));
        }
        /* any more data?
         * if the data index didn't incremenent AND we have bits left over (offset)
         * OR we have 5 ones
         */
        if (((increment == 0) && offset) || (ones_count == 5))
        {
            if (do_bit_stuffing)
            {
                framep->data[dind++] = stuff_byte(0, &last_data, &ones_count, &increment, &offset);
            }
            else
            {
                dind++;
            }
            if (verbose > 2)
            {
                printf
                    (" frame[eof.0]         [%d.%d]=0x%02x (oc %d) ld 0x%02x",
                     dind - 1, offset, framep->data[dind - 1], ONES_TOTAL(last_data), last_data);
                printf(" ---- ---- -> %d%d%d%d %d%d%d%d\n", BIN_ARGS(framep->data[dind - 1]));
            }
        }
    }

    framep->size = (uint16_t) dind;
    framep->offset = offset;
    framep->callback_sent = callback_sent;

    if (framep->error_ind)
    {
        /* Add a multiple ones error */
        printf("Adding multiple ones error at %d\n", error_ind);
        framep->data[framep->error_ind + flag_count] = 0xff;
    }

    framep->data[dind + 1] = 0;

    /* Queue the frame for the channel */
    rc = dm_add_frame(dmap, &dmap->channel[chanId], framep);

    return rc;
}


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
    void (*callback_sent)() )
{
    uint16_t chanId = DM_CHANID(uchanId);
    t_framebuf *framep;
    int rc = 0;
    uint32_t dind = 0;
    uint32_t max_frame_size;
    t_dag_mapper *dmap = dm_get_mapper(dagfd);

    if (verbose > 2)
        printf("send_frame_atm(chan=%d, hec_error=%d)\n", chanId, hec_error);

    /* Confirm channel exists */
    if (dmap->channel[chanId].type == CT_UNDEFINED)
    {
        fprintf(stderr, "send_frame_atm() error: channel %d not defined\n", chanId);
        return -1;
    }

    /* Q. Should we limit the number of frames allowed to pend per channel? */

    /* Get space for the cell and its header - 53 bytes i*/
    max_frame_size = 128;
    framep = (t_framebuf *) malloc(sizeof(t_framebuf) + max_frame_size);

    if (framep == NULL)
    {
        fprintf(stderr, "Failed to allocated %d bytes for frame\n", max_frame_size);
        perror("Malloc failed: ");
        return (-1);
    }

    framep->next = NULL;

    /* simple - just copy the full ATM record */
#if 0
    memcpy(&framep->data[0], &datap[0], 53);
    /* Set the hec */
    atm_insert_hec(&framep->data[0]);
#else
    /* Set the hec */
    atm_insert_hec(&datap[0]);

    for (dind=0; dind<53; dind++) {
	framep->data[dind] = reversed_byte[datap[dind]];
    }
#endif

    if (verbose > 2) printf("atm hec: %02x %02x %02x %02x -> %02x\n",
	    		framep->data[0], framep->data[1], framep->data[2], 
	    		framep->data[3], framep->data[4]);

    /* Sanity check */
    if (dind > max_frame_size)
    {
        /* Arg */
        fprintf(stderr, "Fatal error: wrote %d bytes into %d byte frame\n", dind, max_frame_size);
        exit(0);
    }

    framep->size = (uint16_t) 53;
    framep->offset = 0;
    framep->callback_sent = callback_sent;

    /* Queue the frame for the channel */
    rc = dm_add_frame(dmap, &dmap->channel[chanId], framep);

    return rc;
}


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
int
dag_mapper_send_frame(int dagfd,
                      int chanId,
                      uint8_t * datap, int len, uint32_t options, void (*callback_sent) ())
{
    return dag_mapper_send_debug_frame(dagfd, chanId, datap, len, 0, 0, options, callback_sent);
}


/**********************************************************************
* FUNCTION:	dm_initialise()
* DESCRIPTION:	Initialise the mapper api.
*		Generate the bit stuffing table and the bit reversing table.
* INPUTS:	none.
* RETURNS:	none.
**********************************************************************/
/* setup all the tables required for hdlc mapping */
/* One for all dags */
static void dm_initialise()
{
    int ones_count, ind;

    for (ind = 0; ind < MAX_LINES; ind++)
    {
        num_timeslots[ind] = MAX_TIMESLOT - 1;  /* Channelised E1 default */
    }


    /* Generate a table to accelerate bit stuffing */
    for (ones_count = 0; ones_count < 6; ones_count++)
    {
        for (ind = 0; ind < 0x100; ind++)
        {
            int stuff_count;
            uint16_t data;
            uint16_t mask;
            int bit;

            stuff_count = 0;
            data = ind << 8;
            mask = 0x00F8;
            data |= ((0x1F00 >> ones_count) & 0x00FF);  /* Include extra ones in data */
            for (bit = 0; bit < 8; bit++, mask <<= 1)
            {
                if ((data & mask) == mask)
                {               /* Hit 5 ones */
                    /* Stuff a zero in the bit, shift rest of the bits left */
                    data = ((data << 1) & (0xFE00 << bit)) | (data & (0x00FF << bit));
                    //data = ((data >> 1) & (0x7F00 >> bit)) | (data & (0xFF00 << (7-bit)));
                    stuff_count++;
                }
            }

            right_stuffed_byte[ind][ones_count].data = (uint8_t) ((data >> 8) & 0xFF);
            right_stuffed_byte[ind][ones_count].stuff_count = stuff_count;
        }
    }

    /* Generate table to accelerate bit order reversal */
    for (ind = 0; ind < 0x100; ind++)
    {
        reversed_byte[ind] =
            ((ind & 0x80) >> 7) |
            ((ind & 0x40) >> 5) |
            ((ind & 0x20) >> 3) |
            ((ind & 0x10) >> 1) |
            ((ind & 0x08) << 1) | ((ind & 0x04) << 3) | ((ind & 0x02) << 5) | ((ind & 0x01) << 7);
    }

    if (verbose > 3)
    {
        printf("data: sent ones count ");
        for (ind = 0; ind < 0x100; ind++)
        {
            printf
                ("0x%02x: %02x.%d %02x.%d %02x.%d %02x.%d %02x.%d %02x.%d\n",
                 ind, STUFFED_BYTE[ind][0].data,
                 STUFFED_BYTE[ind][0].stuff_count, STUFFED_BYTE[ind][1].data,
                 STUFFED_BYTE[ind][1].stuff_count, STUFFED_BYTE[ind][2].data,
                 STUFFED_BYTE[ind][2].stuff_count, STUFFED_BYTE[ind][3].data,
                 STUFFED_BYTE[ind][3].stuff_count, STUFFED_BYTE[ind][4].data,
                 STUFFED_BYTE[ind][4].stuff_count, STUFFED_BYTE[ind][5].data,
                 STUFFED_BYTE[ind][5].stuff_count);
        }
    }
}


/**********************************************************************
* FUNCTION:	dag_mapper_set_verbosity(verb_level)
* DESCRIPTION:	Sets the debugging verbosity level.  A level of 0 is no
*		debug output, and each value increment is an additional
*		level of debug output.
* INPUTS:	verb_level	- verbosity level (0 to 4).
* RETURNS:	none.
**********************************************************************/
void
dag_mapper_set_verbosity(int verb_level)
{
    verbose = verb_level;
}


/**********************************************************************
* FUNCTION:	dag_mapper_set_debug(debug_bits)
* DESCRIPTION:	Sets the debugging features.  The debug_level is a set
*		of debug bits each of which enables a specific type
*		of debugging.  See the DM_DEBUG_xxx defines for debug
*		options.
* INPUTS:	debug_bits	- debug bitset
* RETURNS:	none.
**********************************************************************/
void
dag_mapper_set_debug(uint32_t debug_bits)
{
    debug = debug_bits;
}


/**********************************************************************
* FUNCTION:	dag_mapper_delete_channel(dagfd, chanId)
* DESCRIPTION:	Deletes a previously added channel.
* INPUTS:	dagfd	- device to delete the channel from
*		chanId	- id of the channel to delete
* RETURNS:	0	- success
*		< 0	- failure
**********************************************************************/
int
dag_mapper_delete_channel(int dagfd, int uchanId)
{
    uint16_t chanId = DM_CHANID(uchanId);
    t_dag_mapper *dmap = dm_get_mapper(dagfd);
    t_channel *chanp = dmap->head_chanp;
    t_channel *lchanp = dmap->head_chanp;

    if (dmap == NULL)
    {
        /* unknown device */
        fprintf(stderr, "dag_mapper_delete_channel() error: unknown device %d\n", dagfd);
        return -1;
    }

    while (chanp && (chanp->id != chanId))
    {
        lchanp = chanp;
        chanp = chanp->next;
    }

    if (chanp)
    {
        dm_unmapChannel(dmap, chanp);

        /* Is this the first record? */
        if (chanp == dmap->head_chanp)
        {
            dmap->head_chanp = chanp->next;
        }

        /* Is this the last record? (could be first as well) */
        if (chanp == dmap->tail_chanp)
        {
            lchanp->next = chanp->next;
            if (lchanp == dmap->tail_chanp)
            {
                /* List is empty - no channels defined */
                dmap->tail_chanp = NULL;
            }
            else
            {
                /* Tail is the last channel */
                dmap->tail_chanp = lchanp;
            }
        }

        /* Add channel to head of free list */
        chanp->next_free_id = dmap->free_chan_id;
        dmap->free_chan_id = chanp->id;

        chanp->id = -1;
        chanp->type = CT_UNDEFINED;
        dmap->chancount--;

    }
    else
    {
        fprintf(stderr, "dag_remove_tx_channel() error: channel %d not defined\n", uchanId);
        return -1;
    }

    return 0;
}


//#ifdef USE_MAPPER
/**********************************************************************
* FUNCTION:	dag_mapper_add_channel(dagfd, chanId, type, line, ts, tsConf)
* DESCRIPTION:	Add a channel to a device.
* INPUTS:	dagfd	- device to add the channel to
*		chanId	- id of the channel to add (!?)
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
* 		else, channel Id
**********************************************************************/
int
dag_mapper_add_channel(int dagfd, uint32_t type, uint32_t uline, uint32_t ts, uint32_t tsConf)
{
    uint32_t chanId;
    t_channel *chanp;
    uint16_t sub_size = 0;
    uint8_t sub_first_bit = 0;  /* First bit for subchannel mask */
    uint32_t line = ste_physical[uline];        /* Map from user to physical */
    t_dag_mapper *dmap = dm_get_mapper(dagfd);
    uint32_t format;

    /* Extract ERF format from channel type (upper 16 bits) */
    format = type & 0xFFFF0000;
    type &= 0xFFFF;

    /* The default mapper format is HDLC */
    if (format == CT_FMT_DEFAULT) {
	format = CT_FMT_HDLC;
    }


    if (dmap == NULL)
    {
        /* unknown device */
        fprintf(stderr, "dag_mapper_add_channel() error: unknown device %d\n", dagfd);
        return -1;
    }

    if (dmap->chancount >= MAX_CHANNEL_PER_MODULE)
    {
        fprintf(stderr,
                "dag_mapper_add_channel() error: too many channels defined for %s\n", dmap->device);
        return -1;
    }

    if (dmap->free_chan_id == MAX_CHANNEL_PER_MODULE)
    {
        fprintf(stderr,
                "dag_mapper_add_channel() error: no free channels available for %s\n",
                dmap->device);
        return -1;
    }

    /* Allocate a channel */
    chanId = dmap->free_chan_id;
    chanp = &dmap->channel[chanId];

    /* Make sure the channel is not in use? Or just allow redefine? */
    if (chanp->type != CT_UNDEFINED)
    {
        /* channel already in use */
        fprintf(stderr,
                "dag_mapper_add_channel() error: channel %d already defined\n",
                DM_CHANID_USER(chanId));
        return -1;
    }

    /* Remove channel from free list */
    dmap->free_chan_id = chanp->next_free_id;
    chanp->next_free_id = MAX_CHANNEL_PER_MODULE;

    if (verbose > 1)
	printf("dag_mapper_add_channel(): adding chan %d type %d, format %d\n",
		chanp->id, type, format>>16);


    if (type == CT_SUB || type == CT_SUB_RAW) {
        int bit;
        /* determine size of subchannel and location of the first bit from the subchannel mask */
        for (bit = 0; (bit < 7) && !(tsConf & (1 << bit)); bit++);
        sub_first_bit = bit;
        /* Get last bit location */
        for (bit = 7; (bit > 0) && !(tsConf & (1 << bit)); bit--);
        sub_size = bit + 1 - sub_first_bit;
        if (verbose)
            printf
                ("define_channel(%d, %d, %d, %d, 0x%04x) [size %d, first %d]\n",
                 chanp->id, type, line, ts, tsConf, sub_size, sub_first_bit);
        tsConf >>= sub_first_bit;
    } else {
        if (verbose)
            printf("define_channel(%d, %08x, %d, %d, %d, 0x%04x)\n", chanp->id, format, type, line, ts, tsConf);
    }


    memset(chanp, 0, sizeof(t_channel));

    /* Fill in channel entry */
    chanp->id = chanId;
    chanp->line = line;
    chanp->timeslot = ts;
    chanp->format = format;
    chanp->type = type;
    chanp->mask = tsConf;
    chanp->state = CS_OK;
    chanp->first_bit = sub_first_bit;
    chanp->size = sub_size;
    chanp->cur_offset = 0;      /* subchannel bit offset in current hdlc byte */
    chanp->data_offset = 0;     /* not used */
    chanp->bits = 0;
    chanp->bit_count = 0;
    chanp->frame_count = 0;
    chanp->send_count = 0;
    chanp->last_byte = 0;
    chanp->ind = 0;
    chanp->next = NULL;

    /* append to channel control list */
    if (dmap->tail_chanp != NULL) {
        dmap->tail_chanp->next = chanp;
    } else {
        dmap->head_chanp = chanp;
    }
    dmap->tail_chanp = chanp;

    dmap->chancount++;

    dm_mapChannel(dmap, chanp);

    return DM_CHANID_USER(chanp->id);
}

#if 0
int
dag_mapper_add_channel(int dagfd,
                       int chanId, uint32_t type, uint32_t uline, uint32_t ts, uint32_t tsConf)
{
    uint16_t sub_size = 0;
    uint8_t sub_first_bit = 0;  /* First bit for subchannel mask */
    uint32_t line = ste_physical[uline];        /* Map from user to physical */
    t_dag_mapper *dmap = dm_get_mapper(dagfd);

    if (dmap == NULL)
    {
        /* unknown device */
        fprintf(stderr, "dag_define_tx_channel() error: unknown device %d\n", dagfd);
        return -1;
    }

    /* Make sure the channel is not in use? Or just allow redefine? */
    if (dmap->channel[chanId].type != CT_UNDEFINED)
    {
        /* channel already in use */
        fprintf(stderr, "dag_define_tx_channel() error: channel %d already defined\n", chanId);
        return -1;
    }

    memset(&dmap->channel[chanId], 0, sizeof(t_channel));

    if (type == CT_SUB || type == CT_SUB_RAW)
    {
        int bit;
        /* determine size of subchannel and location of the first bit from the subchannel mask */
        for (bit = 0; (bit < 7) && !(tsConf & (1 << bit)); bit++);
        sub_first_bit = bit;
        for (sub_size = 1; (bit < 7) && (tsConf & (1 << bit)); bit++, sub_size++);
        if (verbose)
            printf
                ("define_channel(%d, %d, %d, %d, 0x%04x) [size %d, first %d]\n",
                 chanId, type, line, ts, tsConf, sub_size, sub_first_bit);
        tsConf >>= sub_first_bit;
    }
    else
    {
        if (verbose)
            printf("define_channel(%d, %d, %d, %d, 0x%04x)\n", chanId, type, line, ts, tsConf);
    }


    /* Fill in channel entry */
    dmap->channel[chanId].id = chanId;
    dmap->channel[chanId].line = line;
    dmap->channel[chanId].timeslot = ts;
    dmap->channel[chanId].type = type;
    dmap->channel[chanId].mask = tsConf;
    dmap->channel[chanId].state = CS_OK;
    dmap->channel[chanId].first_bit = sub_first_bit;
    dmap->channel[chanId].size = sub_size;
    dmap->channel[chanId].cur_offset = 0;       /* subchannel bit offset in current hdlc byte */
    dmap->channel[chanId].data_offset = 0;      /* not used */
    dmap->channel[chanId].bits = 0;
    dmap->channel[chanId].bit_count = 0;
    dmap->channel[chanId].frame_count = 0;
    dmap->channel[chanId].send_count = 0;
    dmap->channel[chanId].last_byte = 0;
    dmap->channel[chanId].ind = 0;
    dmap->channel[chanId].next = NULL;

    /* append to channel control list */
    if (dmap->tail_chanp != NULL)
    {
        dmap->tail_chanp->next = &dmap->channel[chanId];
    }
    else
    {
        dmap->head_chanp = &dmap->channel[chanId];
    }
    dmap->tail_chanp = &dmap->channel[chanId];

    dmap->chancount++;

    dm_mapChannel(dmap, &dmap->channel[chanId]);

    return 0;
}
#endif

#undef SIMULATE_TX

#ifdef SIMULATE_TX
#define MAX_FILENAME_LEN 1024
char tmp_filename[MAX_FILENAME_LEN];
void *tx_memory_buffer = NULL;
uint32_t tx_memory_size = 0;

int dag_get_stream_buffer_level(int dagfd, int stream_num)
{
    /* Buffer is always empty since we commit it */
    return 0;
}

uint8_t *dag_tx_get_stream_space(int dagfd, int streamId, uint32_t size)
{
    if (tx_memory_buffer && (tx_memory_size != size))
    {
        free(tx_memory_buffer);
        tx_memory_buffer = NULL;
    }
    if (tx_memory_buffer == NULL)
    {
        tx_memory_buffer = malloc(size);
        tx_memory_size = size;
        printf("Allocated %d bytes for memory hole\n", size);
    }
    return tx_memory_buffer;
}

uint8_t *dag_tx_stream_commit_bytes(int dagfd, int streamId, uint32_t size)
{
    int res;
    /* Output raw erfs */
    res = fwrite(tx_memory_buffer, size, 1, erffile);
    if (res != 1)
    {
        fprintf(stderr, "Error writing erf file, res=%d\n", res);
        perror("Write failed");
        exit(0);
    }

    return tx_memory_buffer;
}

int dag_open(char * dagname)
{
    snprintf(tmp_filename, MAX_FILENAME_LEN, "%s.erf", dagname);
    if ((erffile = fopen(tmp_filename, "wb")) == NULL)
    {
        perror("Failed to open erf output file");
        exit(0);
    }
    printf("Generating output to %s\n", tmp_filename);

    return 0;

}

int dag_close(int dagfd)
{
    fclose(erffile);

    return 0;
}


/* Open a simulated tx mapper - writes to a file instead of a transmit stream */
int dag_mapper_open(int dagfd, char *device, uint32_t burst_count)
{
    int ind, count;
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	int res;
#endif /* _WIN32 */

    /* Default burst size? */
    if (burst_count == 0)
        burst_count = 40000;

    if (dm_dag_count == -1)
    {
        /* Adjust max burst size to an integer number of HDLC erfs */
        dm_initialise();
        memset(dm_dag, 0, sizeof(t_dag_mapper) * MAX_DAGS);
        for (ind = 0; (ind < MAX_DAGS); ind++)
        {
            dm_dag[ind].dagfd = -1;
        }
        dm_dag_count = 0;

	atm_initialise();
    }

    /* Get a dag mapper entry for the new device */
    for (ind = (dagfd % MAX_DAGS), count = MAX_DAGS;
         count && (dm_dag[ind].dagfd != -1); count--, ind = (ind + 1) % MAX_DAGS);

    if (dm_dag[ind].dagfd != -1)
    {
        fprintf(stderr,
                "dag_mapper_open %s: failed to allocate mapper - %d devices active\n",
                device, dm_dag_count);
        return -1;
    }

    /* fill in mapper info for new device */
    dm_dag[ind].dagfd = dagfd;
    dm_dag[ind].burst_count = burst_count;
    dm_dag[ind].num_timeslots = 31;		/* XXX set this based on configuration */
    dm_dag[ind].raw_erf_size = (dm_dag[ind].num_timeslots == 24) ? sizeof(t_packed_mc_raw_t1_erf) : sizeof(t_packed_mc_raw_erf);
    dm_dag[ind].burst_size = burst_count * dm_dag[ind].raw_erf_size;
    dm_dag[ind].erf_count = 0;
    dm_dag[ind].delay = 10000;  /* Default 10msec delay */
    strncpy(dm_dag[ind].device, device, MAX_DAG_NAME);

    memset(dm_dag[ind].tcmap, 0, sizeof(dm_dag[ind].tcmap));

    memset(dm_dag[ind].channel, 0, sizeof(dm_dag[ind].channel));
    dm_dag[ind].head_chanp = NULL;
    dm_dag[ind].tail_chanp = NULL;

    /* Initialize channels and free channel list */
    for (count = 0; count < MAX_CHANNEL_PER_MODULE; count++)
    {
        dm_dag[ind].channel[count].id = -1;
        dm_dag[ind].channel[count].next_free_id = count + 1;
    }
    dm_dag[ind].free_chan_id = 16;      /* Start at 16 - skip raw channel id's */
    dm_dag[ind].channel[MAX_CHANNEL_PER_MODULE - 1].next_free_id = MAX_CHANNEL;

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	res = pthread_mutex_init(&dm_dag[ind].mutex, NULL);
    if (res)
    {
        fprintf(stderr, "dag_mapper_open: pthread_mutext_init failed, err=%d\n", res);
        return -1;
    }
#elif defined (_WIN32)
    InitializeCriticalSection(&dm_dag[ind].mutex);
#endif /* _WIN32 */

    dm_dag_count++;

    if (debug & DM_DEBUG_ERF_FILE)
    {
        if ((erffile = fopen("dagmap.erf", "wb")) == NULL)
        {
            perror("Failed to open erf output file");
            exit(0);
        }
        printf("Generating output to dagmap.erf\n");
    }

    return dagfd;
}


/**********************************************************************
* FUNCTION:	dag_mapper_close(dagfd)
* DESCRIPTION:	Close the HDLC mapper for the specified device.
* INPUTS:	dagfd	   - device to close the mapper on.
* RETURNS:	0	- success
*		< 0	- failure
**********************************************************************/
int
dag_mapper_close(int dagfd)
{
    t_dag_mapper *dmap;

    dmap = dm_get_mapper(dagfd);

    if (dmap == NULL)
    {
        fprintf(stderr, "dag_close_hdlc_tx(%d) : failed.  Invalid file handle.\n", dagfd);
        return -1;
    }

    if (debug & DM_DEBUG_ERF_FILE)
    {
        fclose(erffile);
    }

    dmap->dagfd = -1;
    dm_dag_count--;

    return 0;

}
#else	/* SIMULATE_TX */

/**********************************************************************
* FUNCTION:	dag_mapper_open(dagfd, device, burst_count)
* DESCRIPTION:	Open an HDLC mapper for the specified device.
* INPUTS:	dagfd	    - device to open the mapper on (i.e. previously
*			      openned with dag_open()).
*		device	    - device's ASCII string file name.
*		burst_count - number of erfs to send per burst
*			      set to 0 for default size.
* RETURNS:	0	- success
*		< 0	- failure
**********************************************************************/
int dag_mapper_open(int dagfd, char *device, uint32_t burst_count)
{
    int ind, count;
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	int res;
#endif /* _WIN32 */

    /* Default burst size? */
    if (burst_count == 0)
        burst_count = 3000;

    if (dm_dag_count == -1)
    {
        /* Adjust max burst size to an integer number of HDLC erfs */
	atm_initialise();
        dm_initialise();
        memset(dm_dag, 0, sizeof(t_dag_mapper) * MAX_DAGS);
        for (ind = 0; (ind < MAX_DAGS); ind++)
        {
            dm_dag[ind].dagfd = -1;
        }
        dm_dag_count = 0;
    }

    /* Get a dag mapper entry for the new device */
    for (ind = (dagfd % MAX_DAGS), count = MAX_DAGS;
         count && (dm_dag[ind].dagfd != -1); count--, ind = (ind + 1) % MAX_DAGS);

    if (dm_dag[ind].dagfd != -1)
    {
        fprintf(stderr,
                "dag_mapper_open %s: failed to allocate mapper - %d devices active\n",
                device, dm_dag_count);
        return -1;
    }

    /* fill in mapper info for new device */
    dm_dag[ind].dagfd = dagfd;
    dm_dag[ind].burst_count = burst_count;
    dm_dag[ind].num_timeslots = 31;		/* XXX set this based on configuration */
    dm_dag[ind].raw_erf_size = (dm_dag[ind].num_timeslots == 24) ? sizeof(t_packed_mc_raw_t1_erf) : sizeof(t_packed_mc_raw_erf);
    dm_dag[ind].burst_size = burst_count * dm_dag[ind].raw_erf_size;
    dm_dag[ind].erf_count = 0;
    dm_dag[ind].delay = 10000;  /* Default 10msec delay */
    strncpy(dm_dag[ind].device, device, MAX_DAG_NAME);

    memset(dm_dag[ind].tcmap, 0, sizeof(dm_dag[ind].tcmap));

    memset(dm_dag[ind].channel, 0, sizeof(dm_dag[ind].channel));
    dm_dag[ind].head_chanp = NULL;
    dm_dag[ind].tail_chanp = NULL;

    /* Set up transmit stream */
    /* 1=transmit stream */
    if (dag_attach_stream(dagfd, 1, 0, dm_dag[ind].burst_size) != 0)
    {
        fprintf(stderr, "dag_attach_stream %s: %s\n", device, strerror(errno));
        return -1;
    }

    /* Should it be started here?  Wait for data first? -- consider for future */
    if (dag_start_stream(dagfd, 1) < 0)
    {
        fprintf(stderr, "dag_start_stream %s: %s\n", device, strerror(errno));
        return -1;
    }

    /* Initialize channels and free channel list */
    for (count = 0; count < MAX_CHANNEL_PER_MODULE; count++)
    {
        dm_dag[ind].channel[count].id = -1;
        dm_dag[ind].channel[count].next_free_id = count + 1;
    }
    dm_dag[ind].free_chan_id = 16;      /* Start at 16 - skip raw channel id's */
    dm_dag[ind].channel[MAX_CHANNEL_PER_MODULE - 1].next_free_id = MAX_CHANNEL;

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	res = pthread_mutex_init(&dm_dag[ind].mutex, NULL);
    if (res)
    {
        fprintf(stderr, "dag_mapper_open: pthread_mutext_init failed, err=%d\n", res);
        return -1;
    }
#elif defined (_WIN32)
    InitializeCriticalSection(&dm_dag[ind].mutex);
#endif /* _WIN32 */

    dm_dag_count++;

    if (debug & DM_DEBUG_ERF_FILE)
    {
        if ((erffile = fopen("dagmap.erf", "wb")) == NULL)
        {
            perror("Failed to open erf output file");
            exit(0);
        }
        printf("Generating output to dagmap.erf\n");
    }

    return dagfd;
}


/**********************************************************************
* FUNCTION:	dag_mapper_close(dagfd)
* DESCRIPTION:	Close the HDLC mapper for the specified device.
* INPUTS:	dagfd	   - device to close the mapper on.
* RETURNS:	0	- success
*		< 0	- failure
**********************************************************************/
int
dag_mapper_close(int dagfd)
{
    t_dag_mapper *dmap;

    dmap = dm_get_mapper(dagfd);

    if (dmap == NULL)
    {
        fprintf(stderr, "dag_close_hdlc_tx(%d) : failed.  Invalid file handle.\n", dagfd);
        return -1;
    }

    if (debug & DM_DEBUG_ERF_FILE)
    {
        fclose(erffile);
    }

    if (dag_stop_stream(dagfd, 1) < 0)
    {
        fprintf(stderr, "dag_stop_stream %s: %s\n", dmap->device, strerror(errno));
    }

    if (dag_detach_stream(dagfd, 1) != 0)
    {
        fprintf(stderr, "dag_detach_stream %s: %s\n", dmap->device, strerror(errno));
    }

    dmap->dagfd = -1;
    dm_dag_count--;

    return 0;

}
#endif


/**********************************************************************
* FUNCTION:	dm_mapper_thread(dmap)
* DESCRIPTION:	The mapper thread.
*		Constantly generates erfs from the pending HDLC frames
*		for the mapper and sends them as bursts to the board.
*		The mapper will top up the transmit memory hole to 
*		match the dmap->burst_count level in order to create
*		a sustainable transmit rate.

*		Tuned via the dmap->delay and dmap->burst_count parameters.
*		For DAG3.7T; E1 = 2.048 Mbits/s, or 8000 E1 frames per
*		second per line.  For 16 lines and a delay of 10msecs,
*		set burst_count to at least 16*.01*8000 = 1280, plus a
*		factor of 2 to allow for host latencies, so 2560.
* INPUTS:	dmap	- mapper to run.
*		    dmap->delay:       sleep delay microseconds per burst
*		    dmap->burst_count: send burst_count erfs per burst
* RETURNS:	0	- success
* NOTES:	If the mapper thread misses its deadline long enough
*		that the transmit memory hole is emptied by the board
*		then any HDLC frames currently being transmitted will
*		be corrupted.  Increasing the burst_count will fix
*		the problem at a cost of increased latency when sending
*		frames.
*		E.g. making the burst_count 1 second of erfs (8000*16) will
*		incur a latency of one second between calling dag_send_frame()
*		and the frame actaully being sent.
**********************************************************************/
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
static int
dm_mapper_thread(t_dag_mapper * dmap)
#elif defined (_WIN32)
static DWORD WINAPI
dm_mapper_thread(LPVOID arg)
#endif                          /* _WIN32 */
{
#if defined (_WIN32)
    t_dag_mapper *dmap = (t_dag_mapper *) arg;
#endif /* _WIN32 */
    void *buffer;
    int active = 1;
    size_t res;
    uint32_t tx_count, send_length;
    uint32_t send_count = 0;
    uint32_t erf_count = dmap->erf_count;
    uint32_t burst_count = dmap->burst_count;
    int run=1;
    int last_active=0;
    t_channel *chanp;

    if (verbose) printf("dm_mapper_thread starting\n");

   // while (dmap->run & active)
    while (run)
    {
        // See how much data is left in the transmit buffer
        tx_count = (dag_get_stream_buffer_level(dmap->dagfd, 1) / dmap->raw_erf_size);
        if (verbose) printf("tx_count: %d (%d)\n", tx_count, burst_count);

        /* Does it need topping up? */
        if (tx_count < burst_count)
        {
            /* Yes */
            send_count = burst_count - tx_count;
            send_length = send_count * dmap->raw_erf_size;

            /* Get space for writing */
            buffer = dag_tx_get_stream_space(dmap->dagfd, 1, send_length);
            if (buffer == NULL)
            {
                dmap->thread_error = 1;
                break;
            }

            active = send_erfs(dmap, buffer, send_count, MAX_LINES, dmap->num_timeslots);

	    if (verbose && (active != last_active)) printf("dm_mapper_thread: active -> %d\n", active);
	    last_active = active;	/* Just for debug/verbose output */

            /* output to erf debug file? */
            if (erffile)
            {
                res = fwrite(buffer, dmap->raw_erf_size, send_count, erffile);
                if (res != send_count)
                {
			fprintf(stderr, "Error writing erf file, res=%ld, errno=%d\n", (long)res, errno);
                    erffile = NULL;
                }
            }

            // Commit data
            buffer = dag_tx_stream_commit_bytes(dmap->dagfd, 1, send_length);
        }

        if (dmap->delay)
            usleep(dmap->delay);
        if (erf_count)
        {
            if (erf_count <= send_count)
            {
                break;
            }
            erf_count -= send_count;
            if (erf_count < burst_count)
                burst_count = erf_count;
        }

	if ((run == 1) && (dmap->run == 0)) {
	    run = 2;	/* exiting */
	    if (verbose) printf("dm_mapper_thread: state=draining\n");
	}

	if ((run == 2) && !active) {
	    if (verbose) printf("dm_mapper_thread: state=exiting\n");
	    break;
	}
    }

    /* Check transmit stream buffer size and exit once it has drained. */
    if ((res=dag_get_stream_buffer_level(dmap->dagfd, 1)) != 0) {
	    if (verbose) printf("dm_mapper_thread waiting for tx stream to drain, level=%ld\n", (long)res);
	do {
	    usleep(10000);
	    res=dag_get_stream_buffer_level(dmap->dagfd, 1);
	    if (verbose && (res < 100)) printf("dm_mapper_thread waiting for tx stream to drain, level=%ld\n", (long )res);
	} while (res > 0);

    }

    if (verbose) {
	printf("dm_mapper_thread exiting\n");

	/* Dump out send totals per channel */
	chanp =  dmap->head_chanp;
	while (chanp != NULL) {
	    printf("chan %d: send_count=%d\n", chanp->id, chanp->send_count);
	    chanp = chanp->next;
	}
    }

    /* Let the main thread know that we've exited. TBD: this will need to be an atomic variable */
    dmap->running = 0;

    return 0;
}


/**********************************************************************
* FUNCTION:	dag_mapper_stop(dagfd)
* DESCRIPTION:	Stop the mapper thread for the specified dag device.
* INPUTS:	dagfd	- device to stop the mapper on.
* RETURNS:	0	- success
*		-1	- mapper not found
*		-2	- timeout waiting for mapper to stop
**********************************************************************/
int dag_mapper_stop(int dagfd)
{
    t_dag_mapper *dmap = dm_get_mapper(dagfd);
    int retry=0;

    if (dmap == NULL)
    {
        return -1;
    }

    dmap->run = 0;
    /* Wait for the mapper to finish */
    for (retry=0; ((volatile int)(dmap->running)) && (retry<20); retry++) {
	usleep(100000);
    }

    if (retry >= 10) {
	return -2;
    }

    return 0;
}


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
int
dag_mapper_start(int dagfd, unsigned int burst_count, unsigned int erf_count, unsigned int delay)
{
    int res = 0;
    t_dag_mapper *dmap = dm_get_mapper(dagfd);
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	struct sched_param sparm;
#endif /* Platform-specific code. */

    if (dmap == NULL)
    {
        fprintf(stderr, "dag_mapper_run(%d) : failed.  Invalid device handle.\n", dagfd);
        return -1;
    }

    if (burst_count) dmap->burst_count = burst_count;
    if (erf_count) dmap->erf_count = erf_count;
    if (delay) dmap->delay = delay;

    if (erf_count) dmap->erf_count = erf_count;

    dmap->thread_error = 0;
    dmap->run = 1;
    dmap->running = 1;

    printf("dag_mapper_start: burst %d total_erfs %d delay %d\n", burst_count, erf_count, delay);

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
	/* Use real-time scheduling for the mapper thread */
    pthread_attr_init(&dmap->thread_attr);
    pthread_attr_setinheritsched(&dmap->thread_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&dmap->thread_attr, SCHED_FIFO);
    sparm.sched_priority = sched_get_priority_max(SCHED_FIFO) - 5;
    pthread_attr_setschedparam(&dmap->thread_attr, &sparm);
    res =
        pthread_create(&dmap->thread, &dmap->thread_attr,
                       (void *) &dm_mapper_thread, (void *) dmap);
    printf("dag_mapper_start: created pthread - res=%d\n",res);
#elif defined (_WIN32)
    dmap->thread = CreateThread(NULL, 0, dm_mapper_thread, (LPVOID) dmap, 0, &dmap->thread_id);
    if (dmap->thread == NULL)
    {
        fprintf(stderr, "dag_mapper_run(%d) : failed to start thread\n", dagfd);
        return -1;
    }
    if (SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS) == 0)
    {
        fprintf(stderr, "SetPriorityClass() failed\n");
    }
    if (SetThreadPriority(dmap->thread, THREAD_PRIORITY_TIME_CRITICAL) == 0)
    {
        fprintf(stderr, "SetThreadPriority() failed\n");
    }
    //SetThreadPriority(dmap->thread, THREAD_PRIORITY_ABOVE_NORMAL);
    //SetThreadPriority(dmap->thread, THREAD_PRIORITY_HIGHEST);
#endif /* _WIN32 */

    return res;
}


/**********************************************************************
* FUNCTION:	dag_mapper_get_level(dagfd)
* DESCRIPTION:	Return the total number of HDLC frames waiting to be sent
* 		(or currently being sent) for the specified device.
* INPUTS:	dagfd	     - device to return level of
* RETURNS:	Number of HDLC frames queued to be sent.
**********************************************************************/
uint32_t
dag_mapper_get_level(int dagfd)
{
    t_dag_mapper *dmap = dm_get_mapper(dagfd);
    if (dmap == NULL)
    {
        fprintf(stderr, "dag_mapper_level(%d) : failed.  Invalid device handle.\n", dagfd);
        return -1;
    }

    return dmap->frame_level;
}


/**********************************************************************
* FUNCTION:	dag_mapper_get_channel_level(dagfd, chanId)
* DESCRIPTION:	Return the number of HDLC frames waiting to be sent
* 		(or currently being sent) the specified channel.
* INPUTS:	dagfd	     - device to return level of
* RETURNS:	Number of HDLC frames queued to be sent on the channel.
**********************************************************************/
uint32_t
dag_mapper_get_channel_level(int dagfd, int uchanId)
{
    uint16_t chanId = DM_CHANID(uchanId);
    t_dag_mapper *dmap = dm_get_mapper(dagfd);
    if (dmap == NULL)
    {
        fprintf(stderr,
                "dag_mapper_get_channel_level(%d) : failed.  Invalid device handle.\n", dagfd);
        return -1;
    }

    /* Make sure the channel is defined */
    if (dmap->channel[chanId].type == CT_UNDEFINED)
    {
        /* channel already in use */
        fprintf(stderr,
                "dag_mapper_get_channel_level(%d, %d) error: channel %d undefined\n",
                dagfd, uchanId, uchanId);
        return -1;
    }

    return (dmap->channel[chanId].frame_count);
}
