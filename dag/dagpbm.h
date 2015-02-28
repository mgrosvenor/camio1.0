/*****************************************************************************/
/* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.     */
/* All rights reserved.                                                      */
/*                                                                           */
/* This source code is proprietary to Endace Technology Limited and no part  */
/* of it may be redistributed, published or disclosed except as outlined in  */
/* the written contract supplied with this product.                          */
/*                                                                           */
/* $Id: dagpbm.h,v 1.6 2008/11/14 02:22:02 jomi Exp $                        */
/*****************************************************************************/

#ifndef DAGPBM_H
#define DAGPBM_H

/*****************************************************************************/
/* This header file contains the data structures and constants used to       */
/* access the pbm (PCI Burst Manager) registers.                             */
/*****************************************************************************/

/*****************************************************************************/
/* Interface for accessing pbm registers                                     */
/* This is necessary in order to keep compatibility with different versions  */
/* of the burst manager.                                                     */
/*****************************************************************************/
typedef struct dagpbm_global {
	volatile uint32_t * status;		// Control / Status
	volatile uint32_t * burst_threshold;	// Burst threshold
	volatile uint32_t * burst_timeout;	// Burst timeout
	uint8_t  version;			// Burst manager version
} dagpbm_global_t;

typedef struct dagpbm_stream {
	volatile uint32_t * status;		// Control / Status
	volatile uint32_t * mem_addr;		// Mem hole base address
	volatile uint32_t * mem_size;		// Mem hole size
	volatile uint32_t * record_ptr;		// Record pointer
	volatile uint32_t * limit_ptr;		// Limit pointer
	volatile uint32_t * safetynet_cnt;	// At limit event pointer
	volatile uint32_t * drop_cnt;		// Drop counter
} dagpbm_stream_t;

/*****************************************************************************/
/* MkI - Burst Manager Version 0                                             */
/*****************************************************************************/
typedef struct dagpbm_MkI {
	uint32_t mem_addr;		// Mem hole base address
	uint32_t mem_size;		// Mem hole size
	uint32_t burst_threshold;	// Burst threshold
	uint32_t drop_cnt;		// Drop counter
	uint32_t limit_ptr;		// Limit pointer
	uint32_t safetynet_cnt;		// At limit event pointer
	uint32_t record_ptr;		// Record pointer
	uint32_t status;		// Control / Status
	uint32_t capacity;
	uint32_t underover;
	uint32_t pciwptr;
	uint32_t burst_timeout;		// Burst timeout
	uint32_t build;
	uint32_t maxlevel;
	uint32_t segaddr;
} dagpbm_MkI_t;

/*****************************************************************************/
/* MkII - Burst Manager Version 1                                            */
/*****************************************************************************/
typedef struct dagpbm_global_MkII {
	uint32_t status;		// Control / Status
	uint32_t burst_threshold;	// Burst threshold
	uint32_t burst_timeout;		// Burst timeout
} dagpbm_global_MkII_t;

typedef struct dagpbm_stream_MkII {
	uint32_t status;		// Control / Status
	uint32_t mem_addr;		// Mem hole base address
	uint32_t mem_size;		// Mem hole size
	uint32_t record_ptr;		// Record pointer
	uint32_t limit_ptr;		// Limit pointer
	uint32_t safetynet_cnt;		// At limit event pointer
	uint32_t drop_cnt;		// Drop counter
} dagpbm_stream_MkII_t;

/*****************************************************************************/
/* MkIII - Burst Manager Version 2 (CSBM)                                    */
/*****************************************************************************/
typedef struct dagpbm_stream_MkIII {
	uint32_t mem_addr;		// Mem hole base address
	uint32_t mem_addr_h;		// Mem hole base address high
	uint32_t mem_size;		// Mem hole size
	uint32_t pad1;			// unused
	uint32_t active_ptr;		// Active (write) pointer
	uint32_t active_ptr_h;		// Active (write) pointer high
	uint32_t record_ptr;		// Record pointer
	uint32_t record_ptr_h;		// Record pointer high
	uint32_t limit_ptr;		// Limit pointer
	uint32_t limit_ptr_h;		// Limit pointer high
	uint32_t safetynet_cnt;		// At limit event pointer
	uint32_t pad2;			// unused
	uint32_t status;		// Control / Status
} dagpbm_stream_MkIII_t;
/*****************************************************************************/
/* MkIV - Burst Manager Version 3 (HSBM)                                    */
/*****************************************************************************/
typedef struct dagpbm_global_MkIV {
	uint32_t status;		// Control / Status
	uint32_t pad1;// Was Burst threshold
	uint32_t burst_timeout;		// Burst timeout
    uint32_t stream_counts; // stream counts
    uint32_t record_ptr_memory_base_lo;//Low DW of the Record Pointer Memory Base Address.
    uint32_t record_ptr_memory_base_hi;//High DW of the Record Pointer Memory Base Address.
    uint32_t pad2;
    uint32_t pad3;// pad2 and pad3 occupies 0x18 and 0x1c offsets 
    uint32_t record_ptr_mem_update_interval;
} dagpbm_global_MkIV_t;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */





#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* DAGPBM_H */
