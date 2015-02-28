/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: bfs_lib.h,v 1.22 2009/05/20 07:29:12 karthik Exp $
 */

#ifndef DAGBFS_H
#define DAGBFS_H

/* Endace headers. */
#include "dagutil.h"

#if defined (_WIN32)
#include "wintypedefs.h"
#include "filtertypedefs.h"
#endif /* _WIN32 */

#define TCAM_CONFIG_REG  0x0
#define TCAM_DATA_IN 0x4
#define TCAM_MASK_IN 0x8
#define TCAM_ADDRESS_REGISTER 0xC
#define TCAM_PROGRAM_INTERFACE 0x10
#define TCAM_SRAM_INTERFACE 0x14
#define TCAM_PROGRAM_MATCH_VECTOR 0x18
#define TCAM_RECOVERY_CONTROL_REG 0x40
#define TCAM_DATA_RECOVERY_BASE_REG 0x44


#define MAX_BFS_SEGMENTS 8 
#define MAX_BFS_POSSIBLE_RULES 64
#define BFS_MAX_RULESETS  2  
#define BFS_RULE_WIDTH   389

#define ENOMATCH 1
#ifndef ESUCCESS
#define ESUCCESS   0
#endif

#define new_base_msb_mask_2         0xc0000000
#define new_base_msb_mask_4         0xf0000000
#define new_base_msb_mask_6         0xfc000000
#define new_base_msb_mask_8         0xff000000
#define new_base_msb_mask_10        0xffc00000
#define new_base_msb_mask_12        0xfff00000
#define new_base_msb_mask_14        0xfffc0000
#define new_base_msb_mask_16        0xffff0000
#define new_base_msb_mask_18        0xffffc000
#define new_base_msb_mask_20        0xfffff000
#define new_base_msb_mask_22        0xfffffc00
#define new_base_msb_mask_24        0xffffff00
#define new_base_msb_mask_26        0xffffffc0
#define new_base_msb_mask_28        0xfffffff0




#define msb_mask_2         0x30000000
#define msb_mask_4         0x3c000000
#define msb_mask_6         0x3f000000
#define msb_mask_8         0x3fc00000
#define msb_mask_10        0x3ff00000
#define msb_mask_12        0x3ffc0000
#define msb_mask_14        0x3fff0000
#define msb_mask_16        0x3fffc000
#define msb_mask_18        0x3ffff000
#define msb_mask_20        0x3ffffc00
#define msb_mask_22        0x3fffff00
#define msb_mask_24        0x3fffffc0
#define msb_mask_26        0x3ffffff0
#define msb_mask_28        0x3ffffffc






#define lsb_mask_2         0x00000003
#define lsb_mask_4         0x0000000f
#define lsb_mask_6         0x0000003f
#define lsb_mask_8         0x000000ff
#define lsb_mask_10        0x000003ff
#define lsb_mask_12        0x00000fff
#define lsb_mask_14        0x00003fff
#define lsb_mask_16        0x0000ffff
#define lsb_mask_18        0x0003ffff
#define lsb_mask_20        0x000fffff
#define lsb_mask_22        0x003fffff
#define lsb_mask_24        0x00ffffff
#define lsb_mask_26        0x03ffffff
#define lsb_mask_28        0x0fffffff
#define lsb_mask_30        0x3fffffff


typedef struct BFSStateHeader
{
	uint32_t magic_number;
	uint32_t interface_number;
	uint32_t rule_set;
	volatile uint32_t* bfs_register_base;     /* Register space for the coprocessor.*/
	volatile uint32_t* ppf_register_base; 
}BFSStateHeader,*BFSStateHeaderPtr;


/*Static functions to be implemented.*/
void swap_active_bank(BFSStateHeaderPtr header);
bool get_inactive_bank(BFSStateHeaderPtr header);

/*Non static interface.*/
int write_bfscam_value(BFSStateHeaderPtr header,uint32_t db ,uint32_t address, uint32_t data[13],uint32_t mask[13]);
int read_bfscam_value(BFSStateHeaderPtr header,uint32_t db ,uint32_t address, uint32_t data[13],uint32_t mask[13]);

int read_verify_bfscam_value(BFSStateHeaderPtr header,uint32_t db ,uint32_t address, uint32_t data[13],uint32_t mask[13]);


int bfs_cam_initialise(BFSStateHeaderPtr header);

int bfs_write_sram_entry(BFSStateHeaderPtr header,uint32_t db ,uint32_t address,uint32_t colour);

int bfs_read_sram_entry(BFSStateHeaderPtr header,uint32_t db ,uint32_t address,uint32_t *colour);

int bfs_get_lookup_width(BFSStateHeaderPtr header);

int
bfscam_write_cam_389_entry(BFSStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask);

int
bfscam_write_cam_389_entry_ex(BFSStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask,uint32_t ad_value);


int bfscam_read_cam_389_entry_ex(BFSStateHeaderPtr header,uint32_t db_num, uint32_t index, uint8_t *data, uint8_t *mask, uint32_t *ad_value);

int bfscam_read_cam_389_entry(BFSStateHeaderPtr header,uint32_t db_num, uint32_t index, uint8_t *data, uint8_t *mask);

int
bfscam_read_verify_cam_entry_ex(BFSStateHeaderPtr header,uint32_t db_num ,uint32_t index, uint8_t *data,uint8_t * mask,uint32_t *ad_value);


#endif
