/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef UTILITY_H
#define UTILITY_H

/* Public API headers. */
#include "dag_config.h"

/* Endace headers. */
#include "../card.h"

/* C Standard Library headers. */
#if defined (__linux__)
#include <inttypes.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	/* General utility routines that don't use a DagCardPtr. */
	uint32_t compute_checksum(void* buffer, int bytes);
	dag_card_t pci_device_to_cardtype(uint32_t pci_device);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UTILITY_H */
