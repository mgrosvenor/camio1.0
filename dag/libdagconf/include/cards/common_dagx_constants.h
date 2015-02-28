/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef COMMON_DAGX_CONSTANTS_H
#define COMMON_DAGX_CONSTANTS_H


/* GPP register offsets */
enum {
    GPP_CONFIG      = 0x0000, /* from gpp_base */
    IPP_STATUS      = 0x0004,
    GPP_PAD         = 0x0008, /* from gpp_base */
   	GPP_SNAPLEN     = 0x000c, /* from gpp_base */
    IPP_DROP        = 0x0024
};

/* Stream Processor offsets, each GPP port has a SP at gpp_base + N*SP_offset */
enum {
    SP_STATUS       = 0x0000, /* from sp base */
    SP_DROP         = 0x0004, /* from sp base */
    SP_CONFIG       = 0x0008, /* from sp base */
    SP_OFFSET       = 0x0020
};

#if 0
typedef struct pbm_offsets {
	uint32_t globalstatus;		// Global status
	uint32_t streambase;		// Offset of first stream
	uint32_t streamsize;		// Size of each stream
	uint32_t streamstatus;		// Control / Status
	uint32_t mem_addr;		// Mem hole base address
	uint32_t mem_size;		// Mem hole size
	uint32_t record_ptr;		// Record pointer
	uint32_t limit_ptr;		// Limit pointer
	uint32_t safetynet_cnt;		// At limit event pointer
	uint32_t drop_cnt;		// Drop counter
} pbm_offsets_t;
#endif

#endif /* COMMON_DAGX_CONSTANTS_H */

