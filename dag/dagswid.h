
/* Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagswid.h,v 1.2 2006/07/27 20:36:31 cassandra Exp $
 */

#ifndef DAGSWID_H
#define DAGSWID_H

/* Endace headers. */
#include "dag_platform.h"

#define DAG_SERIAL_SIZE 128
/* Maximum size of the 3.7T software ID */
#define D37T_SOFTWARE_ID_SIZE	128

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


int dag_set_drbmode(int mode_flag2);
int dag_get_drbmode(void);

int dag_write_software_id(int dagfd, uint32_t num_bytes, uint8_t* datap, uint32_t key);
int dag_read_software_id(int dagfd, uint32_t num_bytes, uint8_t* datap);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAGSWID_H */
