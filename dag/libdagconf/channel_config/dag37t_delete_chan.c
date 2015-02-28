/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_delete_chan.c,v 1.11 2007/05/31 02:13:34 usama Exp $
 */


#include "dag_platform.h"
#include "dagutil.h"
#include "dagapi.h"

#include "../include/channel_config/dag37t_chan_cfg.h"
#include "../include/channel_config/dag37t_map_functions.h"

#if defined(_WIN32)
#include "windll.h"
#endif

/* look up table of the four different table offsets in the channel configuration table */
extern uint32_t banks[4];


int dag_delete_board_channels(int dagfd)
{
	uint8_t* dagiom = dag_iom(dagfd);
	modules_t *module, module1, module2;
	int modules, offset;


	module1.type = UNDEFINED;
	module2.type = UNDEFINED;

#if defined (_WIN32)
	EnterCriticalSection(&ChannelMutex);
#endif

	/* check if the ATM_RX Module is loaded */
	get_firmware_modules(dagfd, &module1, &module2);

	reset_command_count(dagfd, dagiom, module1.addr);
	reset_command_count(dagfd, dagiom, module2.addr);


	if(module2.type == MOD_HDLC_TX)
	{
		/* this clears the enable FCS generation bit in the mapper, this is a hack for backwards compatibility.
		*  With the old dagchan the FCS generation was disabled by default, so we have to do it here as well.
		*/
		iom_write(dagiom, module2.addr+ MAP_CTRL_REG, BIT16, 0x0);
	}
	if(module1.type == MOD_HDLC_RX)
	{
		/* reset raw mode status */
		iom_write(dagiom, module1.addr+ MAP_CTRL_REG, RAW_MODE_MASK, 0x0);
	}

	if(module1.type == MOD_ATM_RX)
	{
		dagutil_verbose_level(2, "reseting hec correction and scrambling in module 1\n");
		/* reset hec correction and scrambling */
		iom_write(dagiom, module1.addr + LINE_FEATURE_CTRL_REG, WRITE_ALL_MASK, 0x0);
	}
	else if(module2.type == MOD_ATM_RX)
	{
		dagutil_verbose_level(2, "reseting HEC correction and scrambling in module 2\n");
		/* reset HEC correction and scrambling */
		iom_write(dagiom, module2.addr+ LINE_FEATURE_CTRL_REG, WRITE_ALL_MASK, 0x0);
	}

	dagutil_verbose_level(3, "reseting all sections of channel configuration table %d\n", module1.type);

	/* rather than reading in the entire table, then resetting it all, just 
	* write no mask, and no connection number to all addresses in all modules
	* this is written in groups of interface size to keep below the limits
	*/

	reset_command_count(dagfd, dagiom, module1.addr);
	reset_command_count(dagfd, dagiom, module2.addr);

	module = &module1;
	for(modules = 0; modules <2; modules++)
	{
		dagutil_verbose_level(1, "module_address for deletion is %x\n", module->addr); 


		/* delete all masks and chains that may be present */
		for (offset = 0; offset < CHANNEL_MAP_SIZE; offset++)
		{
			if((offset + 1) % MAX_TIMESLOT == 0)
			{
				execute_command(dagfd, module->addr);
			}

			write_mask_to_table(dagiom, 0, 0, module->addr, banks[2]+offset);
			write_mask_to_table(dagiom, 0, 0, module->addr, banks[3]+offset);
		}

		/* delete all entries in the timeslot connection holders*/
		for (offset = 0; offset < CHANNEL_MAP_SIZE; offset++)
		{
			if((offset + 1) % MAX_TIMESLOT == 0)
			{
				execute_command(dagfd, module->addr);
			}

			write_conn_num_to_table(dagiom, 0x0, module->addr,  banks[0]+offset, WRITE_CMD);
		}

		/* delete any chained connections */
		for (offset = 0; offset < CHANNEL_MAP_SIZE; offset++)
		{
			if((offset + 1)  % MAX_TIMESLOT == 0)
			{
				execute_command(dagfd, module->addr);
			}

			write_conn_num_to_table(dagiom, 0x0, module->addr,  banks[1]+offset, WRITE_DEL_CONN_CMD);
		}

		module = &module2;
	}

#if defined (_WIN32)
			LeaveCriticalSection(&ChannelMutex);
#endif


	return 0;

}



int dag_delete_channel(int dagfd, uint32_t channel_id)
{

	modules_t module1, module2;
	uint8_t* dagiom = dag_iom(dagfd);
    uint32_t raw_modes = 0;

	module1.type = UNDEFINED;
	module2.type = UNDEFINED;

#if defined (_WIN32)
	EnterCriticalSection(&ChannelMutex);
#endif

	/* get the firmware module addresses */
	if(!get_firmware_modules(dagfd, &module1, &module2))
	{
		dagutil_verbose_level(1, "Firmware modules could not be determined\n");
#if defined (_WIN32)
    	LeaveCriticalSection(&ChannelMutex);
#endif
		return -1;
	}

	if(channel_id < 16 )
	{
		/* reset raw mode status */
		iom_write(dagiom, module1.addr+ MAP_CTRL_REG, 16 << channel_id, 0x0);
	}


	/* read the channel configuration from the card into the configuration table array */
	if(!read_connection_numbers(dagfd, module1.addr, module2.addr))
	{
		dagutil_verbose_level(1, "Channels could not be read from the card\n");
#if defined (_WIN32)
    	LeaveCriticalSection(&ChannelMutex);
#endif
		return -1;
	}

	if(!delete_channel(dagfd, &module1, 0, channel_id))
	{
		if(!delete_channel(dagfd, &module2, 1, channel_id))
		{
			dagutil_verbose_level(1, "channel could not be identified\n");
#if defined (_WIN32)
			LeaveCriticalSection(&ChannelMutex);
#endif
			return -1;
		}
	}

    /* reset raw mode status if required*/
    if(channel_id < 16)
    {
        raw_modes = iom_read(dagiom, module1.addr+ MAP_CTRL_REG);
		/* reset raw mode status */
        raw_modes &= ((~channel_id) << 16);
		iom_write(dagiom, module1.addr+ MAP_CTRL_REG, RAW_MODE_MASK, raw_modes);

    }


#if defined (_WIN32)
	LeaveCriticalSection(&ChannelMutex);
#endif


	return 0;
}


bool delete_channel(int dagfd,
					modules_t * pmodule,
					int module_index,
					int channel_id)
{
	uint8_t* dagiom = dag_iom(dagfd);
	bool found = false;
	int offset = 0;
	int command = WRITE_DEL_CONN_CMD;
	int bank = 0;
	uint32_t conn_num, chain_bit;
	uint32_t entry, chained_conn;

    int iCommandCount = 0;

	for(bank = 0; bank < 2; bank++)
	{
		/* go through all addresses of the banks, looking for the connection number*/
		for(offset = 0; offset < CHANNEL_MAP_SIZE; offset++)
		{
			conn_num = (module_chan_data[module_index][bank][offset] & READ_CONN_MASK) >> 12;

            /** Fix for hyperchannel configuration problem **/

            if(iCommandCount > 25)
             {
		         execute_command(dagfd, pmodule->addr);
                 iCommandCount = 0;
             }

			/* if found, remember that the connection has been found */
			if(conn_num == (uint32_t)channel_id)
			{
                if(found != true)
                    read_chaining(dagfd, module_index, pmodule->addr);
				
                found = true;


				dagutil_verbose_level(3, "Channel %d found for deletion \n", conn_num);

				chain_bit = (module_chan_data[module_index][bank+2][offset] & READ_CHAIN_BIT_MASK) >> 20;
				/* check the chaining bit */
				if(chain_bit == 0)
				{
    				dagutil_verbose_level(4, "Chain bit of channel %d is not set \n", conn_num);

                    /* not set, set connection number and mask to null */
					write_mask_to_table(dagiom, 0, 0, pmodule->addr, banks[bank+2]+offset);
					write_conn_num_to_table(dagiom, 0x0, pmodule->addr,  banks[bank]+offset, command);
                    ++iCommandCount;
					command = WRITE_CMD;
				}
				else
				{
    				dagutil_verbose_level(4, "Chain bit of channel %d is set \n", conn_num);
					/* nasty, there is a chain here, need to move the values across, 
					 * so that the chain is not broken ,first of all write the existing channel
                     * to be deleted with a delete command */

                    entry = module_chan_data[module_index][bank][offset];
					write_conn_num_to_table(dagiom, conn_num, pmodule->addr,  entry, command);
                     ++iCommandCount;
					command = WRITE_CMD;
                    
                    /* now write what is being pointed to into the newly deleted entry (both mask and 
                     * connection number from chained connection) */
					entry = module_chan_data[module_index][3][conn_num];
					entry &= ~CONN_ADDR_MASK;
					entry |= banks[bank+2]+offset;
					write_mask_to_table(dagiom, (entry & READ_TIMESLOT_MASK)>>TIMESLOT_MASK_OFFSET, (entry & READ_CHAIN_BIT_MASK) , pmodule->addr, entry);

					entry = module_chan_data[module_index][1][conn_num];
					entry &= ~CONN_ADDR_MASK;
					entry |= banks[bank]+offset;
					chained_conn = (entry & READ_CONN_MASK) >> 12;
					write_conn_num_to_table(dagiom, chained_conn, pmodule->addr,  entry, command);
                    ++iCommandCount;
					/* now that the chain is patched up to the next entry, null the position of the previously
                     * chained connection out as it is now in the position of the connection that has been deleted */
					write_mask_to_table(dagiom, 0, 0, pmodule->addr, banks[3]+channel_id);
					write_conn_num_to_table(dagiom, 0x0, pmodule->addr,  banks[1]+channel_id, command);
                    ++iCommandCount;
				}
			}
			else
            {
                if(conn_num != 0)
				    dagutil_verbose_level(5, "Channel %d found no match with %d\n", conn_num, channel_id);
            }

		}

	}

	if(found)
		execute_command(dagfd, pmodule->addr);

	return found;

}

