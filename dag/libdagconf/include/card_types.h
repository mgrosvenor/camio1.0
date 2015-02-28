/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef CARD_TYPES_H
#define CARD_TYPES_H

/* C Standard Library headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#elif defined(_WIN32)
#include "wintypedefs.h"
#endif

#include "dag_config.h"

/* Possible locations to load firmware. */
typedef enum
{
	kFirmwareLoadPP,
	kFirmwareLoadPCI,
	kFirmwareLoadCopro,
	
	kFirstFirmwareLoad,
	kLastFirmwareLoad

} firmware_load_t;


/* Forward declarations. */
typedef struct DagCardObject DagCardObject;
typedef DagCardObject* DagCardPtr;

/* Method signatures. */
typedef void (*CardDisposeRoutine)(DagCardPtr card);
typedef int (*CardPostInitializeRoutine)(DagCardPtr card); /* Return 1 if standard post_initialize() should continue, 0 if not. */
typedef void (*CardResetRoutine)(DagCardPtr card);
typedef void (*CardDefaultRoutine)(DagCardPtr card);
typedef dag_err_t (*CardLoadFirmwareRoutine)(DagCardPtr card, uint8_t* image);
typedef dag_err_t (*CardLoadEmbeddedRoutine)(DagCardPtr card, uint8_t* image, int target_processor_region);
typedef dag_err_t (*CardLoadPPImageRoutine)(DagCardPtr card, const char* filename, int which_pp);
typedef dag_err_t (*CardUpdateRegisterBaseRoutine)(DagCardPtr card);


#endif /* CARD_TYPES_H */

