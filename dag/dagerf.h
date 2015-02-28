/*
 * Copyright (c) 2006-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagerf.h,v 1.2 2007/05/29 21:48:46 ben Exp $
 */

#ifndef DAGERF_H
#define DAGERF_H

/* Endace headers. */
#include "dag_platform.h"


typedef enum tt
{
	TT_ERROR = -1,
	TT_ATM = 0,
	TT_ETH = 1,
	TT_POS = 2
	
} tt_t;

typedef struct legacy_atm_
{
	int64_t  ts;
	uint32_t crc;
	uint32_t header;
	uint32_t pload[12];
	
} legacy_atm_t;

typedef struct legacy_eth_
{
	int64_t  ts;
	uint16_t wlen;
	uint8_t  dst[6];
	uint8_t  src[6];
	uint16_t etype;
	uint32_t pload[10];
	
} legacy_eth_t;

typedef struct legacy_pos_
{
	int64_t  ts;
	uint32_t slen;
	uint16_t loss;
	uint16_t wlen;
	uint32_t pload[12];
	
} legacy_pos_t;

typedef struct tcp_ip_counter_
{
	int64_t ts;
	uint8_t type;
	uint8_t flags;
	uint16_t rlen;
	uint32_t tcp_ip_flag;
	uint8_t pload[1];
	
} tcp_ip_counter_t;

/* MC HDLC Error flags */
enum
{
	MC_HDLC_FCS = 0x01,
	MC_HDLC_SHORT = 0x02,
	MC_HDLC_LONG = 0x04,
	MC_HDLC_ABORT = 0x08,
	MC_HDLC_OCTET = 0x10,
	MC_HDLC_LOST = 0x20
};

/* MC ATM Error flags */
enum
{
	MC_ATM_LOST = 0x01,
	MC_ATM_HECC = 0x02,
	MC_ATM_OAMCRC = 0x04,
	MC_ATM_OAM = 0x08
};

/* MC AAL Error Flags */
enum
{
	MC_AAL_1ST_CELL = 0x10,
	MC_AAL_LEN_ERROR = 0x80
};

/* MC AAL2 Error Flags */
enum
{
	MC_AAL2_MAAL = 0x40,
	MC_AAL2_1ST_CELL = 0x20
};

/* MC AAL5 Error Flags */
enum
{
	MC_AAL5_CRC_CHECK = 0x10,
	MC_AAL5_CRC_ERROR = 0x20,
	MC_AAL5_LEN_CHECK = 0x40,
	MC_AAL5_LEN_ERROR = 0x80,
	MC_AAL5_1ST_CELL = 0x10
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Utility routines for ERF records: displaying the type as a string, extracting length etc.
 */

/*
 * WARNING: routines in this module are provided for convenience
 *          and to promote code reuse among the dag* tools.
 *          They are subject to change without notice.
 */


/**
Get a string describing the given ERF type.
@param type The type field from the ERF record.
@param legacy_type For legacy ERF records, the type to return (TT_ATM, TT_ETH, TT_POS).  Use TT_ERROR to indicate that the ERF should not be a legacy type.
@return A string constant describing the ERF type.  There is no need to free() this string.
*/
const char * dagerf_type_to_string(uint8_t type, tt_t legacy_type);

/**
Get a string describing the type of the given ERF record.
@param erf A pointer to an ERF record.
@param legacy_type For legacy ERF records, the type to return (TT_ATM, TT_ETH, TT_POS).  Use TT_ERROR to indicate that the ERF should not be a legacy type.
@return A string constant describing the type of the ERF record.  There is no need to free() this string.
*/
const char * dagerf_record_to_string(uint8_t * erf, tt_t legacy_type);

/**
Get the length of an ERF record.
@param erf A pointer to an ERF record.
@return The length of the ERF record (not the wire length of the contained packet).
*/
unsigned int dagerf_get_length(uint8_t * erf);

/**
Determine if an ERF record is of a known (legacy or modern) type.
@param erf A pointer to an ERF record.
@return 1 if the ERF is of a known type, 0 otherwise.
*/
unsigned int dagerf_is_known_type(uint8_t * erf);

/**
Determine if an ERF record is of a known non-legacy type.
@param erf A pointer to an ERF record.
@return 1 if the ERF is of a known type, 0 otherwise.
*/
unsigned int dagerf_is_modern_type(uint8_t * erf);

/**
Determine if an ERF record is a legacy type.
@param erf A pointer to an ERF record.
@return 1 if the ERF is a legacy type, 0 otherwise.
*/
unsigned int dagerf_is_legacy_type(uint8_t * erf);

/**
Determine if an ERF record is an Ethernet type.
@param erf A pointer to an ERF record.
@return 1 if the ERF is an Ethernet type, 0 otherwise.
*/
unsigned int dagerf_is_ethernet_type(uint8_t * erf);

/**
Determine if an ERF record is a PoS type.
@param erf A pointer to an ERF record.
@return 1 if the ERF is a PoS type, 0 otherwise.
*/
unsigned int dagerf_is_pos_type(uint8_t * erf);

/**
Determine if an ERF record is a colored type.
@param erf A pointer to an ERF record.
@return 1 if the ERF is a colored type, 0 otherwise.
*/
unsigned int dagerf_is_color_type(uint8_t * erf);

/**
Determine if an ERF record is a multichannel (MC) type.
@param erf A pointer to an ERF record.
@return 1 if the ERF is a multichannel type, 0 otherwise.
*/
unsigned int dagerf_is_multichannel_type(uint8_t * erf);

/**
Calculates the number of extension (if any) within the ERF record.
@param erf A pointer to an ERF record.
@return the number of extnsion headers present, 0 if there are none.
*/
unsigned int dagerf_ext_header_count(uint8_t * erf, size_t len);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAGERF_H */ 
