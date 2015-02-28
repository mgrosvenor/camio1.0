/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_chan_cfg.h,v 1.4 2007/03/09 01:11:48 cassandra Exp $
 */


#include "dag37t_api.h"

#ifndef DAG37T_CHAN_CFG_H
#define DAG37T_CHAN_CFG_H


#define MAX_MODULES 2
#define MAX_TIMESLOT 31
#define MAX_BANKS 4
#define CHANNEL_MAP_SIZE 0x200






typedef enum{
	UNDEFINED,
	MOD_RAW_TX,
	MOD_HDLC_RX,
	MOD_HDLC_TX,
	MOD_ATM_RX,
	MOD_ATM_TX
}module_type_t;


typedef struct
{
	module_type_t type;
	uint32_t addr;

}modules_t;

/* holds the channel configuration table*/
uint32_t module_chan_data[MAX_MODULES][MAX_BANKS][CHANNEL_MAP_SIZE]; 
/* holds which channels have been identified from the table */
uint32_t channel_list[MAX_CHANNEL]; 



bool get_firmware_modules(int dagfd, 
						  modules_t *pmodule1, 
						  modules_t *pmodule2);

bool read_channels(int dagfd, 
				   uint32_t addr1, 
				   uint32_t addr2);

bool delete_channel(int dagfd,
					modules_t * pmodule,
					int module_index,
					int channel_id);


bool is_channel_active(int dagfd, int channel_id);
int get_37t_channel_direction(int dagfd, int channel_id);
int get_37t_channel_port_num(int dagfd, int channel_id);

int find_chained_connection_port(int channel_id, 
                                 int prev_channel,
                                 modules_t *pmodule1,
                                 modules_t *pmodule2);
bool read_connection_numbers(int dagfd, 
				   uint32_t addr1, 
				   uint32_t addr2);

void read_chaining(int dagfd, 
                   int module,
				   uint32_t addr);

int get_dag_ifc_hec (int dagfd, 
                     uint32_t iface);

int get_dag_ifc_cell_scrambling (int dagfd, 
                                 uint32_t iface);

#endif


