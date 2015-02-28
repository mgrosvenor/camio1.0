/*
 * Copyright (c) 2006-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_protocol_decode.c,v 1.13 2009/06/18 06:41:08 jomi Exp $
 */

/* File header. */
#include "dag_protocol_decode.h"

/* Endace headers. */
#include "dagapi.h"
#include "dagerf.h"
#include "dag_platform.h"
#include "dagutil.h"
/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: dag_protocol_decode.c,v 1.13 2009/06/18 06:41:08 jomi Exp $";
static const char* const kRevisionString = "$Revision: 1.13 $";


/* logical to physical interface mapping for dag37t */
/* Maps from physical line number to logical line */
static int uPhysicalToLogical[] =
{
	13,
	5,
	7,
	15,
	4,
	12,
	14,
	6,
	11,
	3,
	8,
	0,
	2,
	10,
	1,
	9
};


/* IPv6 Extension header values */
#define HOPOPT_EXT_HEADER		0	
#define ROUTE_EXT_HEADER		43
#define FRAG_EXT_HEADER			44
#define AUTH_EXT_HEADER			51
#define ESP_EXT_HEADER			50
#define OPTS_EXT_HEADER			60



/* Internal routines. */
static uint32_t timestamp_to_host(uint32_t timestamp);
static int print_common_erf_header(uint8_t* erf_header, int bytes_remaining);
static int print_legacy_erf_header(uint8_t* erf_header, int bytes_remaining, tt_t legacy_format);
static int print_modern_erf_header(uint8_t* erf_header, int bytes_remaining);
static int print_ext_headers(uint8_t *erf_header, int bytes_remaining);
static int print_common_erf_fourth_word(uint8_t* erf_header, int bytes_remaining, const char* label);
static int print_color_erf_fourth_word(uint8_t* erf_header, int bytes_remaining);
static int print_dsm_erf_fourth_word(uint8_t* erf_header, int bytes_remaining);
static int print_tcp_flow_erf_fourth_word(uint8_t* erf_header, int bytes_remaining);
static int print_erf_hdlc_pos(uint8_t* erf_header, int bytes_remaining);
static int print_erf_eth(uint8_t* erf_header, int bytes_remaining);
static int print_erf_atm(uint8_t* erf_header, int bytes_remaining);
static int print_erf_mc_hdlc(uint8_t* erf_header, int bytes_remaining);
static int print_erf_mc_raw(uint8_t* erf_header, int bytes_remaining);
static int print_erf_mc_atm(uint8_t* erf_header, int bytes_remaining);
static int print_erf_mc_raw_channel(uint8_t* erf_header, int bytes_remaining);
static int print_erf_mc_aal5(uint8_t* erf_header, int bytes_remaining);
static int print_erf_mc_aal2(uint8_t* erf_header, int bytes_remaining);
static void print_tcp_flow_payload(uint8_t* payload, int bytes_remaining, int datatype);


/* Implementation of internal routines. */
static uint32_t
timestamp_to_host(uint32_t timestamp)
{
	/* Timestamps are generally little-endian. */
#if defined(__ppc__)
	return ((timestamp & 0xff000000) >> 24) | ((timestamp & 0x00ff0000) >> 8) | ((timestamp & 0x0000ff00) << 8) | ((timestamp & 0x000000ff) << 24);
#else
	return timestamp;
#endif /* Timestamp swap code. */
}


static int
print_common_erf_header(uint8_t* erf_header, int bytes_remaining)
{
	/* Print first 8 bytes: timestamp. */
	uint32_t * overlay32 = (uint32_t*) erf_header;
	time_t time;
	struct tm* dtime;
	char dtime_str[80];

	assert(erf_header);

	time = timestamp_to_host(overlay32[0]);
	dtime = gmtime((time_t *)(&time));
	if (dtime == NULL)
	{
		printf("erf: timestamp error: %"PRIu64"\n", (uint64_t) time);
	}
	else
	{
		uint64_t time64 = timestamp_to_host(overlay32[0]);
		
		time64 <<= 32;
		time64 += timestamp_to_host(overlay32[1]);
		
		/* Note: windows does not support %T - use %H:%M:%S instead. */
		strftime(dtime_str, sizeof(dtime_str), "%Y-%m-%d %H:%M:%S", dtime);

#if defined(__FreeBSD__) || defined(__linux__) || defined (__NetBSD__) || (defined(__SVR4) && defined (__sun)) || (defined(__APPLE__) && defined(__ppc__))

		printf("erf: ts=0x%.16"PRIx64" %s.%07.0f UTC\n",
			time64, dtime_str,
			(double)(time64 & 0xffffffffll) / 0x100000000ll * 10000000);

#elif defined(_WIN32)

	/* Printf under windows crashes when trying to print a large integer, 
	 * so we have to split ts in two 32-bit integers
	 */
		printf("erf: ts=0x%.16I64x %s.%07.0f UTC\n",
			time64, dtime_str,
			(double)(ULONG)(time64 & 0xffffffff) / 0x100000000i64 * 10000000);

#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif /* Platform specific code. */
	}

	return 8;
}


static int
print_legacy_erf_header(uint8_t* erf_header, int bytes_remaining, tt_t legacy_format)
{
	legacy_atm_t * arec = (legacy_atm_t*) erf_header;
	legacy_eth_t * erec = (legacy_eth_t*) erf_header;
	legacy_pos_t * prec = (legacy_pos_t*) erf_header;

	/* Print type. */
	printf("erf: type=%s\n", dagerf_type_to_string(erf_header[0], legacy_format));

	switch (legacy_format)
	{
		case TT_ATM:
			printf("erf: crc=0x%.8x atm=0x%.8x gfc=%2u vpi=%3u vci=%5u pti=%1u clp=%1u\n",
				(int32_t) ntohl(arec->crc),
				(int32_t) ntohl(arec->header),
				(uint32_t) ntohl(arec->header) >> 28,
				(uint32_t) (ntohl(arec->header) >> 20) & 0xff,
				(uint32_t) (ntohl(arec->header) >> 4) & 0xffff,
				(uint32_t) (ntohl(arec->header) >> 1) & 0x7,
				(uint32_t)(ntohl(arec->header) & 0x1));
			return 8;
			break;
		
		case TT_ETH:
			printf("erf: wlen=%u etype=0x%.4x\n"
				"dst=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x "
				"src=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
				ntohs(erec->wlen),
				ntohs(erec->etype),
				erec->dst[0], erec->dst[1], erec->dst[2], erec->dst[3], erec->dst[4], erec->dst[5],
				erec->src[0], erec->src[1], erec->src[2], erec->src[3], erec->src[4], erec->src[5]);
			return 16;
			break;
		
		case TT_POS:
			printf("erf: slen=%u loss=%u wlen=%u\n",
				(uint32_t)ntohl(prec->slen),
				ntohs(prec->loss),
				ntohs(prec->wlen));
			return 8;
			break;

		default:
			assert(0);
			break;
	}
	
	assert(0);
	return 0;
}


static int
print_modern_erf_header(uint8_t* erf_header, int bytes_remaining)
{
	dag_record_t* drec = (dag_record_t*) erf_header;
	int bytes = 0;
	int total_bytes = 0;

	/* Print type. */
	printf("erf: type=%s\n", dagerf_type_to_string(erf_header[0], 0));

	/* Print common third word. */
	printf("erf: dserror=%d rxerror=%d trunc=%d vlen=%d iface=%d rlen=%d\n",
			(int) drec->flags.dserror, (int) drec->flags.rxerror,
			(int) drec->flags.trunc, (int) drec->flags.vlen,
			(int) drec->flags.iface, ntohs(drec->rlen));

	bytes_remaining -= 4;
	total_bytes += 4;
	if (bytes_remaining <= 0)
	{
		return total_bytes;
	}

	/* Print fourth word by ERF type.  Only difference is the label on the loss counter. */
	switch ( (drec->type & 0x7F) )
	{
		case TYPE_AAL5:
		case TYPE_ATM:
		case TYPE_ETH:
		case TYPE_HDLC_POS:
		case TYPE_IP_COUNTER:
		case TYPE_MC_AAL2:
		case TYPE_MC_AAL5:
		case TYPE_MC_ATM:
		case TYPE_MC_HDLC:
		case TYPE_MC_RAW_CHANNEL:
		case TYPE_MC_RAW:
		case TYPE_PAD:
			bytes = print_common_erf_fourth_word(&erf_header[4], bytes_remaining, "lctr");
			break;
		
		case TYPE_TCP_FLOW_COUNTER:
			bytes = print_tcp_flow_erf_fourth_word(&erf_header[4], bytes_remaining);
			break;
	
		case TYPE_COLOR_HDLC_POS:
		case TYPE_COLOR_HASH_POS:
		case TYPE_COLOR_ETH:
		case TYPE_COLOR_HASH_ETH:
			bytes = print_common_erf_fourth_word(&erf_header[4], bytes_remaining, "ipf");
			
			/* Print the same information broken down by bitfields. */
			print_color_erf_fourth_word(&erf_header[4], bytes_remaining);
			break;
	
		case TYPE_DSM_COLOR_HDLC_POS:
		case TYPE_DSM_COLOR_ETH:
			bytes = print_common_erf_fourth_word(&erf_header[4], bytes_remaining, "dsm");
			
			/* Print the same information broken down by bitfields. */
			print_dsm_erf_fourth_word(&erf_header[4], bytes_remaining);
			break;

		default:
			assert(0);
			break;
	}
	
	bytes_remaining -= bytes;
	total_bytes += bytes;
	if (bytes_remaining <= 0)
	{
		return total_bytes;
	}


	/* Print any extension headers */
	bytes = print_ext_headers(&erf_header[0], (bytes_remaining + 8));

	bytes_remaining -= bytes;
	total_bytes += bytes;
	if (bytes_remaining <= 0)
	{
		return total_bytes;
	}


	/* Print the fifth word (and beyond) by ERF type. */
	switch ( (drec->type & 0x7F) )
	{
		case TYPE_HDLC_POS:
		case TYPE_COLOR_HDLC_POS:
		case TYPE_DSM_COLOR_HDLC_POS:
			bytes = print_erf_hdlc_pos(&erf_header[total_bytes], bytes_remaining);
			break;
	
		case TYPE_ETH:
		case TYPE_COLOR_ETH:
		case TYPE_DSM_COLOR_ETH:
			bytes = print_erf_eth(&erf_header[total_bytes], bytes_remaining);
			break;
		
		case TYPE_ATM:
		case TYPE_AAL5:
			bytes = print_erf_atm(&erf_header[total_bytes], bytes_remaining);
			break;
			
		case TYPE_MC_HDLC:
		case TYPE_COLOR_MC_HDLC_POS:
			bytes = print_erf_mc_hdlc(&erf_header[total_bytes], bytes_remaining);
			break;
			
		case TYPE_MC_RAW:
			bytes = print_erf_mc_raw(&erf_header[total_bytes], bytes_remaining);
			break;
			
		case TYPE_MC_ATM:
			bytes = print_erf_mc_atm(&erf_header[total_bytes], bytes_remaining);
			break;
			
		case TYPE_MC_RAW_CHANNEL:
			bytes = print_erf_mc_raw_channel(&erf_header[total_bytes], bytes_remaining);
			break;
			
		case TYPE_MC_AAL5:
			bytes = print_erf_mc_aal5(&erf_header[total_bytes], bytes_remaining);
			break;
	
		case TYPE_MC_AAL2:
			bytes = print_erf_mc_aal2(&erf_header[total_bytes], bytes_remaining);
			break;
	
		case TYPE_IP_COUNTER:
		case TYPE_TCP_FLOW_COUNTER:
		case TYPE_PAD:
			bytes = 0; /* Doesn't have a fifth word. */
			break;
		
		default:
			assert(0);
			break;
	}
	
	total_bytes += bytes;
	return total_bytes;
}


static int
print_common_erf_fourth_word(uint8_t* erf_header, int bytes_remaining, const char* label)
{
	uint16_t * overlay16 = (uint16_t *) erf_header;
	int lctr = ntohs(overlay16[0]);
	int wlen = ntohs(overlay16[1]);

	assert(erf_header);
	assert(bytes_remaining >= 4);
	assert(label);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}
	
	printf("erf: %s=0x%04x wlen=%d\n", label, lctr, wlen);
	
	return 4;
}


/**
 * This function prints the DAG2 extension header fields out to stdout, it
 * is called by the print_rec function when an extension header is 
 * detected in a packet. This function assumes that the existance of the
 * DAG2 extension headers has already been verified.
 *
 * @param[in]  erf_header       Pointer to the ERF record starting after the
 *                              timestamp.
 * @param[in]  bytes_remaining  The number of bytes left in the ERF record
 *                              after the timestamp.
 *
 * @returns                     The number of byts consumed by the function,
 *                              in this case it will be the number extension
 *                              headers times eight.
 *
 */
static int
print_ext_headers(uint8_t *erf_header, int bytes_remaining)
{
	uint64_t hdr;
	uint8_t  hdr_type;
	uint32_t hdr_num;


	/* sanity check we have enough bytes for at least one header */
	if ( bytes_remaining < 8 )
		return 0;


	/* loop over the extension headers */
	hdr_num = 0;
	do {
	
		/* sanity check we have enough bytes */
		if ( bytes_remaining <= (16 + (hdr_num * 8)) )
			break;

		/* get the header type */
		hdr_type = erf_header[(8 + (hdr_num * 8))];

		/* get the header value */
		hdr = *((uint64_t*) (&erf_header[(8 + (hdr_num * 8))]));
		hdr = bswap_64(hdr);

		hdr_num++;

		/* display the header */
		switch ( hdr_type & 0x7F )
		{
			case EXT_HDR_TYPE_NP40G1:
				printf ("erf: np40g1 header=0x%016"PRIx64" match=%u multi_match=%u tag=%u class=%u\n",
				        (uint64_t)(hdr),
				        (uint32_t)((hdr >> 55) & 0x0001),
				        (uint32_t)((hdr >> 54) & 0x0001),
				        (uint32_t)((hdr >> 36) & 0xffff),
				        (uint32_t)((hdr >> 32) & 0x0003));
				break;
			
			default:
				printf ("erf: unknown extension header type (0x%02X)\n", (hdr_type & 0x7F));
				break;
		}
	
	} while ( hdr_type & 0x80 );
	
	return (hdr_num * 8);
}


static int
print_color_erf_fourth_word(uint8_t* erf_header, int bytes_remaining)
{
	uint16_t * overlay16 = (uint16_t *) erf_header;
	uint16_t color = ntohs(overlay16[0]);
	
	printf("erf: color=%u dststream=%u\n", (color >> 2), (color & 0x03));
	
	return 4;
}


static int
print_dsm_erf_fourth_word(uint8_t* erf_header, int bytes_remaining)
{
	uint16_t * overlay16 = (uint16_t *) erf_header;
	uint16_t color = ntohs(overlay16[0]);
	
	printf("erf: hlb=%u%u filters=%u%u%u%u%u%u%u%u stream=%u\n", (color >> 15) & 0x01, (color >> 14) & 0x01, 
		(color >> 13) & 0x01, (color >> 12) & 0x01, (color >> 11) & 0x01, (color >> 10) & 0x01,
		(color >> 9) & 0x01, (color >> 8) & 0x01, (color >> 7) & 0x01, (color >> 6) & 0x01,
		(color & 0x3f));
	
	return 4;
}


static int
print_tcp_flow_erf_fourth_word(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	printf("erf: tcp_ip_flag = %u\n", (uint32_t) ntohl(overlay32[0]));

	return 4;
}


static int
print_erf_hdlc_pos(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	printf("erf: hdlc header=0x%08x\n", (uint32_t) ntohl(overlay32[0]));

	return 4;
}


static int
print_erf_eth(uint8_t* eth_header, int bytes_remaining)
{
	uint16_t * overlay16 = (uint16_t *) eth_header;
	uint16_t ethertype = ntohs(overlay16[6]);
	uint8_t * pload = NULL;
	
	assert(eth_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == eth_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	printf("eth: dst=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x src=%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
			eth_header[0], eth_header[1], eth_header[2], eth_header[3], eth_header[4], eth_header[5],
			eth_header[6], eth_header[7], eth_header[8], eth_header[9], eth_header[10], eth_header[11]);

	if (ethertype == 0x8100)
	{
		uint16_t* payload = (uint16_t*) &overlay16[14];
		uint16_t length_type = ntohs(payload[1]);
		
		/* VLAN ethernet. */
		printf("eth: vlan=0x%04x ", ntohs(payload[0]));
		
		if (length_type >= 0x600)
		{
			/* Non-VLAN Type-encoded ethernet. */
			printf("etype=0x%04x\n", length_type);
			pload = (uint8_t*) &payload[2];
		}
		else
		{
			/* Non-VLAN Length-encoded ethernet. */
			printf("length=0x%04x\n", length_type);
			pload = (uint8_t*) &payload[2];
		}
		
		return 18;
	}
	else if (ethertype >= 0x600)
	{
		/* Non-VLAN Type-encoded ethernet. */
		printf("eth: etype=0x%04x\n", ethertype);
	}
	else
	{
		/* Non-VLAN Length-encoded ethernet. */
		printf("eth: length=0x%04x\n", ethertype);
	}

	return 14;
}


static int
print_erf_atm(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;
	uint32_t header = ntohl(overlay32[0]);

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	printf("atm: atm header=0x%08x gfc=%2d vpi=%3d vci=%5d pti=%1d clp=%1d\n",
			(int32_t) header,
			(uint32_t) header >> 28,
			(uint32_t) (header >> 20) & 0xff,
			(uint32_t) (header >> 4) & 0xffff,
			(uint32_t) (header >> 1) & 0x7,
			(uint32_t) (header & 0x1));

	return 4;
}


static int
print_erf_mc_hdlc(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;
	uint32_t header = ntohl(overlay32[0]);
	uint32_t errs;
	uint32_t connection;
	int result;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	errs = (uint32_t) header >> 24;
	connection = (uint32_t) header & 0x1ff;
	
	printf("erf: channel=%-3d errors 0x%02x: fcs %d short %d long %d abort %d octet %d lostbyte %d\n",
			connection,
			errs,
			(errs & MC_HDLC_FCS) != 0,
			(errs & MC_HDLC_SHORT) != 0,
			(errs & MC_HDLC_LONG) != 0,
			(errs & MC_HDLC_ABORT) != 0,
			(errs & MC_HDLC_OCTET) != 0,
			(errs & MC_HDLC_LOST) != 0);
	result = 4;

	if (bytes_remaining >= 8)
	{
		result += print_erf_hdlc_pos(&erf_header[4], bytes_remaining);
	}
	
	return result;
}


static int
print_erf_mc_raw(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;
	uint32_t header = ntohl(overlay32[0]);
	uint32_t connection;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	connection = (uint32_t) header & 0x1ff;
	printf("erf: channel=%-3d errors=0x%02x\n",
		(uint32_t) connection,
		(uint32_t) header >> 24);
	
	return 4;
}


static int
print_erf_mc_atm(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;
	uint32_t mcheader = ntohl(overlay32[0]);
	uint32_t connection;
	uint8_t errs = 0;
	int result;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	if ((uint32_t) mcheader & 0x8000)
	{
		printf("erf: ImaId");
	}
	else
	{
		printf("erf: channel");
	}
	errs = (uint32_t) mcheader >> 24;
	connection = (uint32_t) mcheader & 0x1ff;
	printf("=%-3u ifc=%-2u flags 0x%02x: oam %d lost %d hec corrected %d oamcrcerr %d\n",
			connection,
			(uint32_t) uPhysicalToLogical[(mcheader >> 16) & 0x0f],
			errs,
			(errs & MC_ATM_OAM) != 0,
			(errs & MC_ATM_LOST) != 0,
			(errs & MC_ATM_HECC) != 0,
			(errs & MC_ATM_OAMCRC) != 0);
	result = 4;

	if (bytes_remaining >= 8)
	{
		result += print_erf_atm(&erf_header[4], bytes_remaining);
	}

	return result;
}


static int
print_erf_mc_raw_channel(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;
	uint32_t mcheader = ntohl(overlay32[0]);
	uint32_t connection;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	connection = (uint32_t) mcheader & 0x1ff;
	printf("erf: channel=%-3d errors=0x%02x\n",
			(uint32_t) connection,
			(uint32_t) mcheader >> 29);
	
	return 4;
}


static int
print_erf_mc_aal5(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;
	uint32_t mcheader = ntohl(overlay32[0]);
	uint32_t errs;
	uint32_t connection;
	int result;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}

	errs = (uint32_t) mcheader >> 24;
	connection = (uint32_t) mcheader & 0x1ff;
	printf("erf: mc header %x channel=%-3u ifc=%-2u \n",
			mcheader,
			connection,
			(uint32_t) uPhysicalToLogical[(mcheader >> 16) & 0x0f]);
	
	printf("erf: flags 0x%02x: CRC chk %d error %d len chk %d error %d 1st cell %d \n", 
			errs,
			(errs & MC_AAL5_CRC_CHECK) != 0,
			(errs & MC_AAL5_CRC_ERROR) != 0,
			(errs & MC_AAL5_LEN_CHECK) != 0,
			(errs & MC_AAL_LEN_ERROR) != 0,
			((((uint32_t) mcheader >> 24) & MC_AAL_1ST_CELL) != 0));
	result = 4;

	if (bytes_remaining >= 8)
	{
		result += print_erf_atm(&erf_header[4], bytes_remaining);
	}
	
	return result;
}


static int
print_erf_mc_aal2(uint8_t* erf_header, int bytes_remaining)
{
	uint32_t * overlay32 = (uint32_t *) erf_header;
	uint32_t mcheader = ntohl(overlay32[0]);
	uint32_t connection;
	uint8_t errs = 0;
	int result;

	assert(erf_header);
	assert(bytes_remaining >= 4);
	
	if ((NULL == erf_header) || (bytes_remaining < 4))
	{
		return 0;
	}
	
	errs = (uint32_t) mcheader >> 24;
	connection = (uint32_t) mcheader & 0x1ff;
	printf("mc header %x channel=%-3u ifc=%-2u \n",
			mcheader,
			connection,
			(uint32_t) uPhysicalToLogical[(mcheader >> 16) & 0x0f]);
	printf("flags 0x%02x: len error %d 1st cell %d \n", 
		   errs,
		   (errs & MC_AAL_LEN_ERROR) != 0,
		   ((((uint32_t) mcheader >> 24) & MC_AAL_1ST_CELL) != 0));
	result = 4;

	if (bytes_remaining >= 8)
	{
		result += print_erf_atm(&erf_header[4], bytes_remaining);
	}
	
	return result;
}


static void
print_tcp_flow_payload(uint8_t* payload, int bytes_remaining, int datatype)
{
	uint8_t * pos = (uint8_t*) payload;
	uint8_t * last = (uint8_t*) (payload + bytes_remaining);

	while (pos < last)
	{
		if (datatype /* ntohl(tcp_ip_counter->tcp_ip_flag) */ == 1)
		{
			/* print ip counter addresses*/
			printf("IP Address = %u.%u.%u.%u\n", pos[0], pos[1], pos[2], pos[3]);
			pos += 4;
			printf("Source Count = %u\n", (uint32_t)ntohl(*(uint32_t*)pos));
			pos += 4;
			printf("Dest Count = %u\n", (uint32_t)ntohl(*(uint32_t*)pos));
			pos += 4;
			printf("\n");
		}
		else if (datatype /* ntohl(tcp_ip_counter->tcp_ip_flag) */ == 2)
		{
			/* print tcp flow addresses */
			printf("Source IP Address = %u.%u.%u.%u\n", pos[0], pos[1], pos[2], pos[3]);
			pos += 4;
			printf("Dest IP Address = %u.%u.%u.%u\n", pos[0], pos[1], pos[2], pos[3]);
			pos += 4;
			printf("IP Protocol = %u\n", pos[0]);
			pos += 4;
			printf("Source Port = %u\n", ntohs(*(uint16_t*)pos));
			pos += 2;
			printf("Destination Port = %u\n", ntohs(*(uint16_t*)pos));
			pos += 2;
			printf("Count = %u\n", (uint32_t)ntohl(*(uint32_t*)pos));
			pos += 4;
			printf("\n");
		}
		else
		{
			break;
		}
	}
}


/* Public routines. */
uint8_t*
dagpd_find_ip_header(uint8_t * rec, int verbose, tt_t legacy_type)
{
	dag_record_t* drec = (dag_record_t *) rec;
	uint8_t       erf_type = 0x00;
	uint16_t      erf_rlen;
	unsigned      ext_hdrs = 0;
	
	
	erf_type = (drec->type & 0x7F);
	erf_rlen = ntohs(drec->rlen);
	
	
	/* For some unknown reason this function doesn't take a record length parameter, so
	 * we use the rlen from the record and hope it is accurate.
	 */
	ext_hdrs = dagerf_ext_header_count (rec, erf_rlen);
	if ( ext_hdrs > 0 )
	{
		/* Hack: because the following code only uses the data fields below the standard 16-byte
		 *       ERF header, we can offset the "record" overlay by the number of extension headers
		 *       and the rest of the fields should line up.
		 */
		 drec = (dag_record_t *) ((uintptr_t)rec + (ext_hdrs * 8));
	}
	

	switch ( erf_type )
	{
		case TYPE_LEGACY:
			switch (legacy_type)
			{
				case TT_ATM:
					/* assuming RFC2225 Classical ATM over IP LLC/SNAP */
					if (ntohs(*(uint16_t*)(rec+22)) == 0x0800) 
						return (rec+24);
					else
					{
						if (verbose)
							printf("%s: etype=0x%.4x, non-IP\n", __func__, ntohs(*(uint16_t*) (rec+22)) );
						return NULL;
					}
					break;
				
				case TT_ETH:
					if (ntohs(((legacy_eth_t*)rec)->etype) != 0x0800)
					{
						if (verbose)
							printf("%s: etype=0x%.4x, non-IP\n", __func__, ntohs(((legacy_eth_t*) rec)->etype));
						return NULL;
					}
					return (rec+24);
				
				case TT_POS:
					if (ntohs(*(uint16_t*)(rec+18)) == 0x0800) 
						return (rec+20);
					else
					{
						if (verbose)
							printf("%s: etype=0x%.4x, non-IP\n", __func__, ntohs(*(uint16_t*) (rec+18)) );
						return NULL;
					}
					break;
				
				case TT_ERROR:
					/* Client has specified that the ERF isn't legacy type, so this is an error. */
					return NULL;
				
				default:
					assert(0); /* Unknown type. */
					break;
			}
		
		case TYPE_HDLC_POS:
		case TYPE_COLOR_HDLC_POS:
		case TYPE_COLOR_HASH_POS:
		case TYPE_DSM_COLOR_HDLC_POS:
			{
				unsigned int hdlc = ntohl(drec->rec.pos.hdlc);
				
				/* Check for MPLS */
				if ( hdlc == 0xFF030281 || hdlc == 0xFF030283 || hdlc == 0x0F008847 || hdlc == 0x0F008848 )
				{
					uint32_t* overlay32 = (uint32_t*) drec->rec.pos.pload;
					uint32_t  mpls_value;
					uint32_t  mpls_index = 0;
					
					/* Jump over MPLS Shim headers */
					if ( erf_rlen > 4 )
					{
						do 
						{
							mpls_value = ntohl (overlay32[mpls_index]);
							mpls_index++;
							erf_rlen -= 4;

							if (verbose)
							{
								printf("mpls: label=%u stackbottom=%d ttl=%u\n", (mpls_value >> 12), 
									((mpls_value >> 8) & 0x1), (mpls_value & 0xFF) );
							}

						} while ( ((mpls_value & 0x00000100) == 0) && (erf_rlen > 4) );
					}

					if ( erf_rlen == 0 )
					{
						return NULL;
					}

					return (uint8_t*) &overlay32[mpls_index];
				}

				else if ( hdlc == 0xFF030021 || hdlc == 0xFF030057 || hdlc == 0x0F000800 || hdlc == 0x0F0086DD )
				{
					return (uint8_t*) drec->rec.pos.pload;
				}

				else
				{
					return NULL;
				}
			}
			
		case TYPE_ETH:
		case TYPE_COLOR_ETH:
		case TYPE_COLOR_HASH_ETH:
		case TYPE_DSM_COLOR_ETH:
			{
				int ethertype = ntohs(drec->rec.eth.etype);
				
				if ((ethertype == 0x8100) || (ethertype == 0x88a8) )
				{
                    /* VLAN ethernet encapsulations here */
                    uint16_t* overlay16;
                    int length_type ;
                    uint8_t*    temp_pload_ptr = (uint8_t*) drec->rec.eth.pload;
                    do{ 
                        overlay16 = (uint16_t*) temp_pload_ptr;
                        length_type = ntohs(overlay16[1]);
                        /* check if its ipv4 or ipv6 */
                        if((0x0800 == length_type) || (0x86dd == length_type))
                        {
                            /* Found IP header. */
                            if ( verbose )
                                printf("vlan: id=%04u etype=0x%04x\n", (ntohs(overlay16[0]) & 0xfff),  length_type);
                            return (uint8_t*) &overlay16[2];
                        }
                        if (verbose)
                        {
                            printf("vlan: id=%04u etype=0x%04x\n", (ntohs(overlay16[0]) & 0xfff),  length_type);
                        }
                        /* Jump over VLAN tags*/
                        temp_pload_ptr = (uint8_t*) &overlay16[2];
					}while ( (length_type == 0x8100) || (length_type == 0x88a8));
                    /* could not find IP header */
					return NULL;
				}
                else if ((ethertype == 0x8848) || (ethertype == 0x8847) )
                {
                    /* MPLS ethernet encapsulations here */
                    uint32_t* overlay32 = (uint32_t*) drec->rec.eth.pload;
                    uint32_t  mpls_value;
                    uint32_t  mpls_index = 0;
                    
                    /* Jump over MPLS Shim headers */
                    if ( erf_rlen > 4 )
                    {
                        do 
                        {
                            mpls_value = ntohl (overlay32[mpls_index]);
                            mpls_index++;
                            erf_rlen -= 4;
                            
                            if (verbose)
                            {
                                printf("mpls: label=%u stackbottom=%d ttl=%u\n", (mpls_value >> 12), 
                                    ((mpls_value >> 8) & 0x1), (mpls_value & 0xFF) );
                            }
                        } while ( ((mpls_value & 0x00000100) == 0) && (erf_rlen > 4) );
                    }
                    
                    if ( erf_rlen == 0 )
                    {
                    return NULL;
                    }

                    return (uint8_t*) &overlay32[mpls_index];
				}
				else if ((ethertype == 0x0800) || (ethertype == 0x86dd))
				{
					/* Non-VLAN Type-encoded ethernet carrying IP. */
					return (uint8_t*) drec->rec.eth.pload;
				}
				
				if (verbose)
				{
					printf("%s: etype=0x%04x, non-IP\n", __func__, ethertype);
				}
				return NULL;
			}
		
		case TYPE_ATM:
			/* assuming RFC2225 Classical ATM over IP LLC/SNAP */
			if (ntohs(*(uint16_t*)(drec->rec.atm.pload + 6)) != 0x0800)
			{
				if (verbose)
					printf("%s: ATM etype=0x%.4x, non-IP\n", __func__, ntohs(*(uint16_t*)(drec->rec.atm.pload + 6)));
				
				return NULL;
			}
			return (uint8_t*) drec->rec.atm.pload + 8;
		
		default:
			assert(0); /* Invalid ERF type. */
			printf("%s: not implemented for ERF type %d\n", __func__, erf_type);
			return NULL;
	}

	assert(0); /* Invalid ERF type. */
	return NULL;
}


void
dagpd_decode_protocols(uint8_t* erf_record, int erf_length)
{
	uint8_t* ip_header = NULL;

	/* Find IP header - at the moment 'decode' only works for IP packets. */
	ip_header = dagpd_find_ip_header(erf_record, dagutil_get_verbosity(), 0);
	if (ip_header)
	{
		uintptr_t bytes_remaining = erf_length - (uintptr_t) ((uintptr_t) ip_header - (uintptr_t) erf_record);

		dagpd_decode_ip(ip_header, bytes_remaining);
	}
}


void
dagpd_decode_erf(uint8_t* erf_header, int erf_length, tt_t legacy_format)
{
	dag_record_t* drec = (dag_record_t*) erf_header;
	tcp_ip_counter_t * trec = (tcp_ip_counter_t*) erf_header;
	int bytes_remaining = erf_length;
	int bytes = 0;
	int offset = 0;

	assert(erf_header);
	if (NULL == erf_header)
	{
		return;
	}

	/* Print the ERF headers. */
	bytes = dagpd_print_erf_header(erf_header, erf_length, legacy_format);
	bytes_remaining -= bytes;
	offset += bytes;
	if (bytes_remaining <= 0)
	{
		return;
	}
	
	/* Now the payload. */
	if ((TYPE_TCP_FLOW_COUNTER == drec->type) || (TYPE_IP_COUNTER == drec->type))
	{
		/* It's an IP/TCP flow counter packet. */
		print_tcp_flow_payload(&erf_header[offset], bytes_remaining, ntohl(trec->tcp_ip_flag));
		return;
	}

	/* Print the IP headers. */
	dagpd_decode_protocols(erf_header, erf_length);
}


int
dagpd_print_erf_header(uint8_t* erf_header, int erf_length, tt_t legacy_format)
{
	/* Print entire ERF header - delegates to internal routines. */
	dag_record_t* drec = (dag_record_t*) erf_header;
	int bytes_remaining = erf_length;
	int bytes = 0;
	int offset = 0;

	assert(erf_header);
	if (NULL == erf_header)
	{
		return -1;
	}

	bytes = print_common_erf_header(erf_header, bytes_remaining);
	bytes_remaining -= bytes;
	offset += bytes;

	if (0 == drec->type)
	{
		/* Legacy format */
		if (TT_ERROR == legacy_format)
		{
			/* The client has specified that receiving legacy ERF is an error. */
			assert(0);
			return offset;
		}
		
		bytes = print_legacy_erf_header(&erf_header[offset], bytes_remaining, legacy_format);
	}
	else
	{
		bytes = print_modern_erf_header(&erf_header[offset], bytes_remaining);
	}
	offset += bytes;
	
	return offset;
}


void
dagpd_decode_ip(uint8_t* ip_header, int bytes_remaining)
{
	uint8_t* next_header = NULL;
	int header_bytes;
	uint8_t next_header_type = 0;

	assert(ip_header);
	assert(bytes_remaining > 0);
	assert(0 == (bytes_remaining & 0x03));

	/* Print the IP header. */
	header_bytes = dagpd_print_ip_header(ip_header, bytes_remaining, &next_header_type);

	bytes_remaining -= header_bytes;
	if (bytes_remaining > 0)
	{
		next_header = &ip_header[header_bytes];

		/* Print any further protocol headers. */
		if (IPPROTO_TCP == next_header_type)
		{
			dagpd_decode_tcp(next_header, bytes_remaining);
		}
		else if (IPPROTO_UDP == next_header_type)
		{
			dagpd_decode_udp(next_header, bytes_remaining);
		}
		else if (IPPROTO_ICMP == next_header_type)
		{
			dagpd_decode_icmp(next_header, bytes_remaining);
		}
		else /* unknown, Raw IP or payload, print remainder as bytes. */
		{
			dagpd_print_payload_bytes(next_header, bytes_remaining);
		}
	}
}


int
dagpd_print_ip_header(uint8_t* ip_header, int bytes_remaining, uint8_t *next_proto)
{
	uint16_t* overlay16 = (uint16_t*) ip_header;
	uint32_t* overlay32 = (uint32_t*) ip_header;
	int ip_version;
	int option_words = 0;
	int i;
	int offset = 0;
	
	
	ip_version = ((ip_header[0] & 0xf0) >> 4);
	
	if (ip_version == 4)
	{
		if (bytes_remaining < 20)
			return 20;

		printf("ip: version=%u headerwords=%u tos=%u length=%u\n", ip_version, ip_header[0] & 0x0f, ip_header[1], ntohs(overlay16[1]));
		printf("ip: id=%u flags=0x%x fragmentoffset=%u\n", ntohs(overlay16[2]), (ip_header[6] & 0xe0) >> 5, ((ip_header[6] & 0x1f) << 8) | ip_header[7]);
		printf("ip: ttl=%u protocol=%u checksum=0x%04x\n", ip_header[8], ip_header[9], ntohs(overlay16[5]));
		printf("ip: sourceaddress=%u.%u.%u.%u\n", ip_header[12], ip_header[13], ip_header[14], ip_header[15]);
		printf("ip: destinaddress=%u.%u.%u.%u\n", ip_header[16], ip_header[17], ip_header[18], ip_header[19]);
		offset += 20;
		
        *next_proto = ip_header[9];

		/* Print IP options as raw hex. */
		option_words = (ip_header[0] & 0x0f) - 5;
		for (i = 0; i < option_words; i++)
		{
			printf("ip: 0x%08x\n", overlay32[5 + i]);
			offset += 4;
		}
	}
	else if (ip_version == 6)
	{
		int finished;
		uint8_t next_ext_hdr;

		/* sanity check we have enough of the packet to process the header */
		if (bytes_remaining < 40)
			return 40;

		printf("ip: version=%u trafficclass=%u flowlabel=%u\n", ip_version, ((ntohs(overlay16[0]) & 0xFF0) >> 4), (unsigned int)(ntohl(overlay32[0]) & 0x000FFFFF));
		printf("ip: payloadlength=%u nextheader=0x%u hoplimit=%u\n", ntohs(overlay16[2]), ip_header[6], ip_header[7]);
		printf("ip: sourceaddress=%04x", ntohs(overlay16[4]));
		offset += 10;
		
		for (i = 5; i < 12; i++)
		{
			printf(":%04x", ntohs(overlay16[i]));
			offset += 2;
		}
		printf("\nip: destinaddress=%04x", ntohs(overlay16[12]));
		offset += 2;
		for (i = 13; i < 20; i++)
		{
			printf(":%04x", ntohs(overlay16[i]));
			offset += 2;
		}
		printf("\n");

		/* Display any extension headers */
		finished     = 0;
		next_ext_hdr = ip_header[6];
		while (!finished)
		{
			switch (next_ext_hdr)
			{
				case HOPOPT_EXT_HEADER:
					next_ext_hdr = ip_header[offset];
					printf ("ip: hop-by-hop ext header\n");
					offset += (ip_header[offset + 1] * 8) + 8;
					break;
					
				case ROUTE_EXT_HEADER:
					next_ext_hdr = ip_header[offset];
					printf ("ip: routing ext header\n");
					offset += (ip_header[offset + 1] * 8) + 8;
					break;
					
				case OPTS_EXT_HEADER:
					next_ext_hdr = ip_header[offset];
					printf ("ip: destoptions ext header\n");
					offset += (ip_header[offset + 1] * 8) + 8;
					break;
					
				case FRAG_EXT_HEADER:
					next_ext_hdr = ip_header[offset];
					printf ("ip: fragment ext header\n");
					offset += 8;
					break;
					
				case AUTH_EXT_HEADER:
					next_ext_hdr = ip_header[offset];
					printf ("ip: authentication ext header\n");
					offset += (ip_header[offset + 1] * 4) + 8;
					break;
				
				case ESP_EXT_HEADER:
					printf ("ip: encapsulated security ext header\n");
					finished = 1;
					break;

				default:
					finished = 1;
			}
				

			/* again determine we have enough space to read the next header */
			if ( !finished && (bytes_remaining < (offset + 8)) )
				return offset;
				
		}

		*next_proto = next_ext_hdr;
	}
	
	return offset;
}


void
dagpd_decode_tcp(uint8_t* tcp_header, int bytes_remaining)
{
	uint8_t* next_header = NULL;
	int header_bytes;
	
	assert(tcp_header);
	assert(bytes_remaining > 0);

	/* Print the TCP header. */
	dagpd_print_tcp_header(tcp_header, bytes_remaining);

	/* FIXME: find the payload offset. */
	header_bytes = 4 * ((tcp_header[12] & 0xf0) >> 4);
	bytes_remaining -= header_bytes;
	if (bytes_remaining > 0)
	{
		next_header = &tcp_header[header_bytes];
		dagpd_print_payload_bytes(next_header, bytes_remaining);
	}
}


int
dagpd_print_tcp_header(uint8_t* tcp_header, int bytes_remaining)
{
	uint16_t* overlay16 = (uint16_t*) tcp_header;
	uint32_t* overlay32 = (uint32_t*) tcp_header;
	int option_words = 0;
	int total_bytes = 0;
	int i;
	
	assert(tcp_header);
	assert(bytes_remaining > 0);
	
	if (bytes_remaining > 0)
	{
		printf("tcp: sourceport=%u destinport=%u\n", ntohs(overlay16[0]), ntohs(overlay16[1]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		printf("tcp: sequence=0x%08x\n", (uint32_t) ntohl(overlay32[1]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		printf("tcp: acknowledgement=0x%08x\n", (uint32_t) ntohl(overlay32[2]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		printf("tcp: offset=%u control=%u window=%u\n", (tcp_header[12] & 0xf0) >> 4, tcp_header[13], ntohs(overlay16[7]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		printf("tcp: checksum=0x%04x urgent=%u\n", ntohs(overlay16[8]), ntohs(overlay16[9]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		/* Print TCP options as raw hex. */
		option_words = ((tcp_header[12] & 0xf0) >> 4) - 5;
		for (i = 0; (i < option_words) && (bytes_remaining > 0); i++)
		{
			printf("tcp: 0x%08x\n", overlay32[5 + i]);
			bytes_remaining -= 4;
			total_bytes += 4;
		}
	}
	
	return total_bytes;
}


void
dagpd_decode_udp(uint8_t* udp_header, int bytes_remaining)
{
	uint8_t* next_header = NULL;
	
	assert(udp_header);
	assert(bytes_remaining > 0);

	/* Print the UDP header. */
	dagpd_print_udp_header(udp_header, bytes_remaining);

	/* Find the payload offset. */
	bytes_remaining -= 8;
	if (bytes_remaining > 0)
	{
		next_header = &udp_header[8];
		dagpd_print_payload_bytes(next_header, bytes_remaining);
	}
}


int
dagpd_print_udp_header(uint8_t* udp_header, int bytes_remaining)
{
	uint16_t* overlay16 = (uint16_t*) udp_header;
	int total_bytes = 0;
	
	assert(udp_header);
	assert(bytes_remaining > 0);

	if (bytes_remaining > 0)
	{
		printf("udp: sourceport=%u destinport=%u\n", ntohs(overlay16[0]), ntohs(overlay16[1]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		printf("udp: length=%u checksum=0x%04x\n", ntohs(overlay16[2]), ntohs(overlay16[3]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}

	return total_bytes;
}


void
dagpd_decode_icmp(uint8_t* icmp_header, int bytes_remaining)
{
	uint8_t* next_header = NULL;
	
	assert(icmp_header);
	assert(bytes_remaining > 0);

	/* Print the ICMP header. */
	dagpd_print_icmp_header(icmp_header, bytes_remaining);

	/* Find the payload offset. */
	bytes_remaining -= 12; /* ICMP packets have a fixed size. */
	if (bytes_remaining > 0)
	{
		next_header = &icmp_header[12];
		dagpd_print_payload_bytes(next_header, bytes_remaining);
	}
}


int
dagpd_print_icmp_header(uint8_t* icmp_header, int bytes_remaining)
{
	uint16_t* overlay16 = (uint16_t*) icmp_header;
	uint32_t* overlay32 = (uint32_t*) icmp_header;
	int total_bytes = 0;
	
	assert(icmp_header);
	assert(bytes_remaining > 0);

	if (bytes_remaining > 0)
	{
		printf("icmp: type=%u code=%u checksum=0x%04x\n", icmp_header[0], icmp_header[1], ntohs(overlay16[1]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		printf("icmp: id=%u sequence=0x%04x\n", ntohs(overlay16[2]), ntohs(overlay16[3]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	if (bytes_remaining > 0)
	{
		printf("icmp: addressmask=0x%08x\n", (uint32_t) ntohl(overlay32[2]));
		bytes_remaining -= 4;
		total_bytes += 4;
	}
	
	return total_bytes;
}


void
dagpd_print_payload_bytes(uint8_t* payload, int bytes_remaining)
{
	int i;
	char abuf[17];

	/* Print in hexadecimal and ASCII. */
	memset(abuf, 0, 17);
	for (i = 0; i < bytes_remaining; i++)
	{
		abuf[i%16] = isprint((*payload) & 0xff) ? ((*payload) & 0xff) : '.';
		printf("%02x ", (*payload) & 0xff);
		if ((i % 16) == 15)
		{
			printf("        %s\n", abuf);
			memset(abuf, 0, 17);
		}
		payload++;
	}
	
	if (i % 16 != 15)
	{
		while (0 != (i % 16))
		{
			printf("   ");
			i++;
		}
		printf("        %s\n", abuf);
	}
	printf("\n");
}
