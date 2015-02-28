
/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_add_chan.h,v 1.1 2006/07/27 21:05:48 cassandra Exp $
 */

#include "dag37t_chan_cfg.h"

#ifndef DAG37T_ADD_CHAN_H
#define DAG37T_ADD_CHAN_H

typedef enum{
	TS_NOTHING,
	TS_RAW,
	TS_NON_RAW,
	TS_NON_CONFLICT_SUB,
	TS_NON_CONFLICT_SUB_AND_RAW
}ts_used_t;






#define FORMAT_MASK 0xFFFF0000




bool validate_channel(uint32_t direction, 
					  uint32_t type, 
					  uint32_t format,
					  uint32_t uline, 
					  uint32_t tsConf,
					  modules_t module1,
					  modules_t module2);



int add_channel(int dagfd,
				uint32_t direction, 
				uint32_t type, 
				uint32_t format,
				uint32_t uline, 
				uint32_t tsConf,
				modules_t module1,
				modules_t module2);

bool check_for_conflicts(uint32_t type, 
						 uint32_t tsConf,
						 int module_index,
						 int iface_start,
						 ts_used_t * pts_type);

ts_used_t is_timeslot_used(int module, 
						   int ts_offset, 
						   uint32_t mask);

int get_first_unused_channel();

int create_channel_on_timeslot(int dagfd,
							   int channel_id, 
							   int module_addr, 
							   int module_index, 
							   int line, 
							   uint32_t timeslot, 
							   uint32_t mask,
							   uint32_t command);

int make_raw_channel(int dagfd, 
					 uint32_t direction, 
					 uint32_t type, 
					 uint32_t uline, 
					 uint32_t tsConf,
					 uint32_t format);

#endif

