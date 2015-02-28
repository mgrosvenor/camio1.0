
/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_aal_connections.h,v 1.4 2006/12/11 23:27:50 cassandra Exp $
 */

#ifndef DAGAALCONNS_H
#define DAGAALCONNS_H

#ifndef _WIN32
/* C Standard Library headers. */
#include <inttypes.h>
#else /* _WIN32 */
#include <wintypedefs.h>
#endif /* _WIN32 */

#define HASH_TABLE_SIZE 64  /*this MUST be a power of two */
#define HASH_TABLE_MASK 0x003F /*mask used to find modular 
						(x%HASH_TABLE_SIZE = x&HASH_TABLE_MASK)*/



typedef struct mc_aal_vc {
	uint64_t last_ts;
	uint32_t vci;
	uint32_t vpi;
	uint32_t connection_num;

#ifdef DAGBITS_BUILD
	int64_t last_delta;
    int64_t std_delta_variance;
#endif

	void *pNext;
}mc_aal_vc_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


uint16_t CalculateHashFunction(uint32_t vci, uint32_t vpi);
mc_aal_vc_t* RetrieveVCFromMemory(uint32_t vci, uint32_t vpi, uint32_t connection_num);
uint32_t StoreNewVirtualChannel(mc_aal_vc_t *pVC);
uint32_t StoreTimestamp(mc_aal_vc_t *pVC);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
