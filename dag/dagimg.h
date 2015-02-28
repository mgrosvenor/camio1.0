/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: dagimg.h,v 1.5 2008/09/25 02:20:27 dlim Exp $
 */

#ifndef DAGIMG_H
#define DAGIMG_H

#define	DAGSERIAL_SIZE	128
#define	DAGSERIAL_ID	0x12345678

/*
 * Backwards compatability struct for mapping PCI device IDs + xilinx to
 * image index when this is not available from firmware directly.
 */
typedef struct dag_img {
	int	device_id;
	int	load_idx;
	int	img_idx;
} dag_img_t;

#define DAG_IMG_END 0xffff

/*
 * Struct for mapping image index numbers to A record pefixes and B records
 * for type checking.
 */
typedef struct img_id {
	int	img_idx;
	char	*img_name;
	char	*img_type;
	int	copro_id;
	int	board_rev;
} img_id_t;

#define IMG_ID_END 0xffff


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


int dag_get_img_idx(int device_id, int device_index);
int dag_check_img(int img_idx, int copro_id, char *arec, char *brec, int board_rev);
int dag_check_img_ptr(int img_idx, int copro_id, char *img, int img_size, int board_rev);
int dag_check_img_type(int img_idx, int copro_id, char* img, int img_size, int board_rev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAGIMG_H */
