/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: dsm_types.h,v 1.5 2008/05/07 21:51:52 dlim Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         dagdsm_types.h
* DESCRIPTION:  Defines the internal types used by the dagdsm API
*               library.
*
*
* HISTORY:      21-01-06 Initial revision
*
**********************************************************************/

#ifndef DSM_TYPES_H
#define DSM_TYPES_H


/* ADT headers */
#include "adt/adt_list.h"
#include "adt/adt_array_list.h"



/*--------------------------------------------------------------------
 
 CONSTANTS

---------------------------------------------------------------------*/

/* filtering details */
#define MATCH_DEPTH             64
#define FILTER_COUNT            8
#define INTERFACE_COUNT         4
#define HLB_COUNT               2
#define HLB_HASH_COUNT			8
#define STREAM_COUNT            8				/*max stream number to support*/

#define ELEMENT_SIZE            8
#define LAST_FILTER_ELEMENT     ( (uint32_t)((MATCH_DEPTH) / (ELEMENT_SIZE)) - 1 )


/* the number of bits per colour lookup table field */
#define DSM_FILTER_BITS         FILTER_COUNT
#define DSM_INTERFACE_BITS      2
#define DSM_HLB_BITS            2
#define DSM_HLB_HASH_BITS		3


/* macros for extracting the various fields from a loopup address */
#define DSM_COLUR_FILTER(x)      ((x) & 0x00FF)
#define DSM_COLUR_IFACE(x)      (((x) & 0x0300) >> 8)
#define DSM_COLUR_HLB0(x)       (((x) & 0x0400) >> 10)
#define DSM_COLUR_HLB1(x)       (((x) & 0x0800) >> 11)
#define DSM_COLUR_IFACE_V_1_4(x)      (((x) & 0x0300) >> 8)				/*2 bits*/
#define DSM_COLUR_IFACE_V_1_8(x)      (((x) & 0x0100) >> 8)				/*1 bits*/
#define DSM_COLUR_HLB_HASH_V_1_4(x)   (((x) & 0x0C00) >> 10)			/*2 bits*/
#define DSM_COLUR_HLB_HASH_V_1_8(x)   (((x) & 0x0E00) >> 9)				/*3 bits*/


/* DSM register offsets */
#define DSM_REG_CSR_OFFSET      0x0000
#define DSM_REG_FILTER_OFFSET   0x0004
#define DSM_REG_COLUR_OFFSET    0x0008
#define DSM_REG_COUNTER_OFFSET  0x000C


/* the numbers of the counters */
#define DSM_CNT_FILTER0         0x0000
#define DSM_CNT_FILTER1         0x0001
#define DSM_CNT_FILTER2         0x0002
#define DSM_CNT_FILTER3         0x0003
#define DSM_CNT_FILTER4         0x0004
#define DSM_CNT_FILTER5         0x0005
#define DSM_CNT_FILTER6         0x0006
#define DSM_CNT_FILTER7         0x0007
#define DSM_CNT_HLB0            0x0008
#define DSM_CNT_HLB1            0x0009
#define DSM_CNT_DROP            0x000A
#define DSM_CNT_STREAM_BASE     0x000B
#define DSM_CNT_STREAM0         (DSM_CNT_STREAM_BASE + 0)
#define DSM_CNT_STREAM2         (DSM_CNT_STREAM_BASE + 1)


/* the virtual filter number of the swap filter */
#define DSM_SWAP_FILTER_NUM     (FILTER_COUNT - 1)



/* modes used when writting the filter bytes */
#define DSM_SET                 0
#define DSM_CLR                 1
#define DSM_AND                 2
#define DSM_OR                  3




/*--------------------------------------------------------------------
 
 SIGNATURES

---------------------------------------------------------------------*/

#define DSM_SIG_CONFIG          0x367836F1
#define DSM_SIG_FILTER          0x672870EA
#define DSM_SIG_PARTIAL_EXPR    0xEA7845F1
#define DSM_SIG_OUTPUT_EXPR     0x892365FA





/*--------------------------------------------------------------------
 
 FILTER

---------------------------------------------------------------------*/

typedef enum
{
	kFragDontCare = 0,
	kFragReject = 1,
	
} frag_filter_t;

typedef enum 
{
	kEthernet = 1,
	kPoS = 2,
	kEthernetVlan = 3,
	
} layer2_type_t;


typedef enum 
{
	kTCP  = IPPROTO_TCP,
	kUDP  = IPPROTO_UDP,
	kICMP = IPPROTO_ICMP,
	
} layer4_type_t;



typedef struct dsm_filter_s
{
	uint32_t        signature;
	
	/* filter details */
	uint32_t        actual_filter_num;

	uint32_t        term_depth;                    /* the lowest element with the early termination bit set */
		
	uint32_t        enabled    : 1;                /* 1 for enabled, 0 for disabled */
	uint32_t        raw_mode   : 1;                /* 1 if raw bytes are to be used for the filter, otherwise zero */
	uint32_t        reserved   : 22;
	uint32_t        card_type  : 8;                /* the type of the card (will be either kEthernet or kPoS */
	
	
	
	/* encapsulation type */
	layer2_type_t   layer2_type;                   /* layer2 protocol set by the user */
	layer3_type_t   layer3_type;                   /* layer3 protocol set by the user (default: IP) */
	layer4_type_t   layer4_type;                   /* IP protocol field, only valid if layer3_type is IP */
	
	

	/* raw filter values */
	struct {
		uint8_t  value[MATCH_DEPTH];
		uint8_t  mask[MATCH_DEPTH];
	} raw;


	/* layer 2 fields (Ethernet/PoS) */
	union {

		struct {		
			struct {
				uint16_t    id;
				uint16_t    mask;
			} vlan_id;
			struct {
				uint16_t    type;
				uint16_t    mask;
			} ethertype;
			struct {
				uint8_t     mac[6];
				uint8_t     mask[6];
			} src_mac;
			struct {
				uint8_t     mac[6];
				uint8_t     mask[6];
			} dst_mac;
		} ethernet;
		
		struct {
			struct {
				uint8_t     hdlc[4];
				uint8_t     mask[4];
			} hdlc;
		} pos;
			
	} layer2;	
			
	
	/* layer 3 fields (IPv4)*/
	union {
	
		struct {
			struct {
				uint8_t     addr[4];
				uint8_t     mask[4];
			} src_addr;
			struct {
				uint8_t     addr[4];
				uint8_t     mask[4];
			} dst_addr;
			struct {
				uint8_t     ihl;
				uint8_t     mask;
			} ihl;
			frag_filter_t   frag_filter;
			uint8_t         protocol;
		} ipv4;	
		
	} layer3;
	
	
	/* layer 4 fields (TCP/UDP/ICMP) */
	union {
	
		struct {
			struct {
				uint16_t    port;
				uint16_t    mask;
			} src_port;
			struct {
				uint16_t    port;
				uint16_t    mask;
			} dst_port;
		} udp;
		
		struct {
			struct {
				uint16_t    port;
				uint16_t    mask;
			} src_port;
			struct {
				uint16_t    port;
				uint16_t    mask;
			} dst_port;
			struct {
				uint8_t     flags;
				uint8_t     mask;
			} tcp_flags;
		} tcp;
		
		struct {
			struct {
				uint8_t     type;
				uint8_t     mask;
			} icmp_type;
			struct {
				uint8_t     code;
				uint8_t     mask;
			} icmp_code;
		} icmp;
		
	} layer4;
			
	
} dsm_filter_t;







/*--------------------------------------------------------------------
 
 PARTIAL EXPRESSIONS

---------------------------------------------------------------------*/

typedef enum
{
	kDontCare = 0,
	kNonInverted = 1,
	kInverted = 2
	
} comparand_t;


typedef struct dsm_partial_expr_s
{
	uint32_t       signature;
	
	uint32_t       interfaces;                      /* the number of interfaces supported by the card */
	
	comparand_t    filter[FILTER_COUNT];            /* filters used for partial expression */
	comparand_t    iface[INTERFACE_COUNT];          /* interfaces used for the partial expression */
	comparand_t    hlb[HLB_COUNT];                  /* hash load balance output used for the partial expression */
	comparand_t    hlb_hash[HLB_HASH_COUNT];		/* New hash load balance association table output used for partial expression, replace hlb*/
} dsm_partial_expr_t;




/*--------------------------------------------------------------------
 
 OUTPUT EXPRESSIONS

---------------------------------------------------------------------*/

typedef struct dsm_output_expr_s
{
	uint32_t       signature;
	
	ListPtr        partial_expr;                    /* dynamic list of pointers to partial expressions */
	ListPtr        inv_partial_expr;                /* dynamic list of pointers to inverted partial expressions */

} dsm_output_expr_t;




/*--------------------------------------------------------------------
 
 CONFIGURATION

---------------------------------------------------------------------*/

typedef struct dsm_config_s
{
	uint32_t          signature;
	
	int               dagfd;
	uint32_t          device_code;
	
	volatile uint8_t* base_reg;
	
	uint32_t          rx_streams;                /* the number of RX streams available */
	uint32_t          interfaces;                /* the number of interfaces for the current card */
	uint32_t          bits_per_entry;            /* the number of bits per COLUR entry (log(n) + 1) */
	
	dsm_filter_t      filters[FILTER_COUNT];
	uint32_t          swap_filter;
	
	ListPtr           partial_expr;              /* dynamic list of partial expressions */
	dsm_output_expr_t output_expr[STREAM_COUNT]; /* the output expressions, made up of partial expressions */

	dsm_version_t     dsm_version;
} dsm_config_t;





/*--------------------------------------------------------------------
 
 INTERNAL FUNCTIONS

---------------------------------------------------------------------*/


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	/* sets the last error code */
	void dagdsm_set_last_error (int errorcode);
	
	
	/* dsm_card.c */
	volatile uint8_t * dsm_get_base_reg (int dagfd);
	

	/* dsm_load.c */
	uint32_t dsm_drb_delay (volatile uint32_t * dsm_csr_reg_p);

	//void dsm_drb_delay (void);
	void dsm_gen_filter_values (dsm_filter_t * filter_p, uint8_t value[MATCH_DEPTH], uint8_t mask[MATCH_DEPTH]);
	int dsm_is_filter_enabled (volatile uint8_t * base_reg, uint32_t filter);
	int dsm_enable_filter (volatile uint8_t * base_reg, uint32_t filter, uint32_t enable);
	int dsm_write_filter (volatile uint8_t * base_reg, uint32_t filter, uint32_t term_depth, const uint8_t * value, const uint8_t * mask);
	int dsm_read_filter (volatile uint8_t * base_reg, uint32_t filter, uint32_t * term_depth, uint8_t value[MATCH_DEPTH], uint8_t mask[MATCH_DEPTH]);
	int dsm_read_colur (dsm_config_t * config_p, volatile uint8_t * base_reg, uint32_t bank, uint16_t * table, uint32_t size);
	int dsm_write_colur (dsm_config_t * config_p, volatile uint8_t * base_reg, uint32_t bank, uint16_t * table, uint32_t size);
	uint32_t dsm_get_inactive_colur_bank (volatile uint8_t * base_reg);
	void dsm_set_active_colur_bank (volatile uint8_t * base_reg, uint32_t bank);
	int dsm_construct_colur (dsm_config_t * config_p, uint16_t * table, uint32_t size);

	/* dsm_counters.c */
	uint32_t dsm_read_counter (volatile uint8_t * base_reg, uint16_t addr);


#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif    // DSM_TYPES_H
