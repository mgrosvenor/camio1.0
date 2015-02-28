/*
 * Copyright (c) 2006-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_smbus.h,v 1.1 2006/03/27 22:06:58 koryn Exp $
 */

#ifndef DAG_SMBUS_H
#define DAG_SMBUS_H

/* Endace headers. */
#include "dag_platform.h"
#include "dagreg.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*
 * WARNING: routines in the dagutil module are provided for convenience
 *          and to promote code reuse among the dag* tools.
 *          They are subject to change without notice.
 */


/* SMB routines.  dagutil_smb_read/write() return 1 if successful, 0 on failure. */
uint32_t dagutil_smb_init(volatile uint8_t * iom, dag_reg_t result[DAG_REG_MAX_ENTRIES], unsigned int count);
int dagutil_smb_write(volatile uint8_t * iom, uint32_t smb_base, unsigned char device, unsigned char address, unsigned char value);
int dagutil_smb_read(volatile uint8_t * iom, uint32_t smb_base, unsigned char device, unsigned char address, unsigned char * value);

/* Setup temperature parameters on LM63 part.  You must ensure the card has this feature before calling this function. */
void dagutil_start_copro_fan(volatile uint8_t * iom, uint32_t smb_base);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAG_SMBUS_H */ 
