/*
* Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
*  $Id: dag37t_add_chan.c,v 1.16 2009/06/18 00:06:50 vladimir Exp $
*/


#include "dag_platform.h"
#include "dagutil.h"
#include "dagapi.h"

#include "dag37t_api.h"
#include "../include/channel_config/dag37t_chan_cfg.h"
#include "../include/channel_config/dag37t_map_functions.h"
#include "../include/channel_config/dag37t_add_chan.h"

#include "dagmapper.h"

#if defined(_WIN32)
#include "windll.h"
#endif

extern uint32_t banks[4];


/* mapping between user interface(POD/Plane interface number) and framer interface number or 
what gets into the ERF before dagbits or dagutil function converts it to readable form*/
const int phy_to_logical_line[16] = {
        11,  /* interface 0 (POD/PLANE or what the user showuld see  */
        14,  /* Interface 1*/
        12, /* ...... */
        9,
        4,
        1,
        7,
        2,
        10,
        15,
        13,
        8,
        5,
        0,
        6,
        3  /* Interface 15 */
};


/* this may need to be altered when the 3.7T-4 is brought to life */
uint32_t max_lines=16;  



int dag_add_channel(int dagfd, 
					uint32_t direction, 
					uint32_t type, 
					uint32_t uline, 
					uint32_t tsConf,
					uint32_t raw_transmit)
{

	modules_t module1;
	modules_t module2;
	uint32_t format = (type & FORMAT_MASK);
	int channel_id = -1;


    dagutil_verbose_level(3, "dag_add_channel(%d, %d, 0x%8x, %d, 0x%8x, %d)\n", dagfd, direction, 
					type, uline, tsConf, raw_transmit);

	type &= ~FORMAT_MASK;

	module1.type = UNDEFINED;
	module2.type = UNDEFINED;


	if (raw_transmit)
	{
		return make_raw_channel(dagfd, direction, type, uline, tsConf, format);
	}

#if defined (_WIN32)
	EnterCriticalSection(&ChannelMutex);
#endif

	/* get the firmware capabilities */
	if(!get_firmware_modules(dagfd, &module1, &module2))
	{
#if defined (_WIN32)
		LeaveCriticalSection(&ChannelMutex);
#endif
		dagutil_verbose_level(1, "Firmware Capabilities could not be determined\n");
		return -1;
	}


	/* read the channel configuration from the card into the configuration table array */
	if(!read_connection_numbers(dagfd, module1.addr, module2.addr))
	{
#if defined (_WIN32)
		LeaveCriticalSection(&ChannelMutex);
#endif
		dagutil_verbose_level(1, "Channels could not be read from the card\n");
		return -1;
	}

	/* check channel type is allowed on this firmware */
	if(!validate_channel(direction, type, format, uline, tsConf, module1, module2))
	{
#if defined (_WIN32)
		LeaveCriticalSection(&ChannelMutex);
#endif
		dagutil_verbose_level(1, "Channel Validation Failed\n");
		return -1;
	}


	/*create channel*/
	channel_id = add_channel(dagfd, direction, type, format, uline, tsConf, module1, module2);

#if defined (_WIN32)
	LeaveCriticalSection(&ChannelMutex);
#endif

	return channel_id;
}


int add_channel(int dagfd,
				uint32_t direction, 
				uint32_t type, 
				uint32_t format,
				uint32_t uline, 
				uint32_t tsConf,
				modules_t module1,
				modules_t module2)
{
	modules_t module;
	int i = 0, count = 0;
	int line = 0;
	int channel_id = -1;
	int module_index = 0;
	int command = WRITE_ADD_CONN_CMD;
	int write_command = WRITE_CMD;
	ts_used_t ts_type;
 	uint8_t* dagiom = dag_iom(dagfd);
	daginf_t*       daginf;	
	int iface_start = 0; 

	daginf = dag_info(dagfd);
	if(daginf->device_code == PCI_DEVICE_ID_DAG3_7T4)
        {
	/* Max lines on T4 can be 4 or 8 depening if the external (aux) connector used and if harwware supports it */
	#ifdef ENDACE_LAB
	    max_lines = 8;
	#else 
            max_lines = 4;
        #endif 
        } else if(daginf->device_code == PCI_DEVICE_ID_DAG3_70T)
        {
		max_lines = 16;
	};    
        line = phy_to_logical_line[uline];

	iface_start = line*32;        

	dagutil_verbose_level(3, "Channel to be created on logical line %d with type %d and format 0x%x  \n", line, type, format);
	/* pick the module to write to */
    if(direction == DAG_DIR_TRANSMIT ||  (format == CT_FMT_ATM && module2.type == MOD_ATM_RX))
	{
		module = module2;
		module_index = 1;
        dagutil_verbose_level(3, "Channel to be created in second module\n");
	}
	else 
		module = module1;

    /*read the chaining and masks for the module that this connection will be added to */
    read_chaining(dagfd, module_index, module.addr);

	/* check for conflicts with existing channels, and also get the state of timeslots */
	if(!check_for_conflicts(type, tsConf, module_index, iface_start, &ts_type))
		return -1;

	if(type != CT_RAW)
	{

		channel_id = get_first_unused_channel();

		dagutil_verbose_level(3, "Channel to be made will use channel number %d\n", channel_id);

		if(channel_id == -1)
		{
			dagutil_verbose_level(1, "All available channels are in use, channel could not be created\n");
			return channel_id;
		}

	}
	else
	{

		channel_id = line;
		dagutil_verbose_level(3, "Raw channel to be made will use channel number %d\n", channel_id);

		/*set raw bit to indicate raw connection */
		set_raw_mode(dagfd, module.addr, line);
        tsConf = 0xFFFFFFFF;

	}


	/* store if this should be a raw command */
	if(type == CT_SUB_RAW || type == CT_HYPER_RAW || type == CT_CHAN_RAW )
	{
		command = WRITE_ADD_RAW_CMD;
	}


	/* create the channel */

	/* reset the command buffer for channel creation */
	dagutil_verbose_level(3, "reset command count in preparation of new channel\n");
	reset_command_count(dagfd, dagiom, module.addr);

	/* split into multiple timeslot and single timeslot type channels */
	if( type == CT_SUB || type == CT_SUB_RAW)
	{

		dagutil_verbose_level(3, "Create sub channel on timeslot \n");
		create_channel_on_timeslot(dagfd, channel_id, module.addr, module_index, line, 
			((tsConf & 0xFFFF0000) >> 16), (tsConf & 0x0000FFFF), command);
	}

	else if (type == CT_CHAN || type == CT_CHAN_RAW )
	{
		dagutil_verbose_level(3, "Create simple channel on timeslot 0x%x \n", tsConf);
		create_channel_on_timeslot(dagfd, channel_id, module.addr, module_index, 
			line, tsConf, TIMESLOT_MASK, command);
	}
	else
	{

		dagutil_verbose_level(3, "Create channel on multiple timeslots 0x%x \n", tsConf);
		/* this connection is on many different timeslots */

	
		/* go through all the other timeslots and create whichever are needed,
		* only adding on the last timeslot */
		for(i = 1; i <= MAX_TIMESLOT; i++)
		{

			if((tsConf & (0x01 << i)) > 0)
			{
				tsConf &= ~(0x01 << i);
				if(tsConf == 0)
					write_command = command;

				count++;
				if(count > 25)
				{
                   create_channel_on_timeslot(dagfd, channel_id, module.addr, module_index, 
						line, i, TIMESLOT_MASK, command);
					execute_command(dagfd, module.addr);
					count = 0;
				}
				else
                    create_channel_on_timeslot(dagfd, channel_id, module.addr, module_index, 
						line, i, TIMESLOT_MASK, write_command);

			}
		}

        /* special case: timeslot zero (which is sort of timelot 31 due to the firmware set up) */ 
		if((tsConf & 0x01) == 0x01)
		{
			tsConf &= ~0x01;
			if(tsConf == 0)
				write_command = command;

                create_channel_on_timeslot(dagfd, channel_id, module.addr, module_index, 
                    line, 32, TIMESLOT_MASK, write_command);
            count++;
		}


	}


	/* execute command  */
	execute_command(dagfd, module.addr);


	return channel_id;

}


int create_channel_on_timeslot(int dagfd,
							   int channel_id, 
							   int module_addr, 
							   int module_index, 
							   int line, 
							   uint32_t timeslot, 
							   uint32_t mask,
							   uint32_t command)
{
	uint8_t* dagiom = dag_iom(dagfd);
	int ts_offset = (line*32) + timeslot - 1;
	int entry_index;
	int bank = 0;


	/* get the current value of the timeslot in the channel configuration table */
	uint32_t mask_space = (module_chan_data[module_index][2][ts_offset] & READ_TIMESLOT_MASK) >> 12;
	uint32_t chain_bit = (module_chan_data[module_index][2][ts_offset] & READ_CHAIN_BIT_MASK) >> 20;
	uint32_t conn_num = (module_chan_data[module_index][0][ts_offset] & READ_CONN_MASK) >> 12;


	/* wait for command buffer ready */
	wait_not_busy(dagiom, module_addr);

	/* check if the space is free */
	if(mask_space == 0)
	{
		dagutil_verbose_level(3, "Create channel %d on timeslot %d in free space 0x%8x\n", channel_id, timeslot, module_chan_data[module_index][0][ts_offset]);

		/* add mask to to this position */
		write_mask_to_table(dagiom, mask, 0, module_addr,  module_chan_data[module_index][2][ts_offset]);
		/* write channel_id to corresponding position */
		write_conn_num_to_table(dagiom, channel_id, module_addr,  module_chan_data[module_index][0][ts_offset], command);


	}
	else
	{

        /* there is another channel in this position, get the mask and chaining of this module 
        * so that channels can be chained together */


		/* keep checking until the end of the chain is found, then set up the required channels  */
		dagutil_verbose_level(3, "Create channel %d on timeslot %d with other channels\n", channel_id, timeslot);

		entry_index = ts_offset;
		bank = 0;

		do
		{
			chain_bit = (module_chan_data[module_index][bank+2][entry_index] & READ_CHAIN_BIT_MASK) >> 20;

			/* is this the last channel in the chain? */
			if(chain_bit == 0)
			{
				/* check if it is a raw connection*/
				if(conn_num < 16)
				{
					/* add the connection here, and the mask, with chaining*/
					write_mask_to_table(dagiom, mask, 1, module_addr,  module_chan_data[module_index][bank+2][entry_index]);
					write_conn_num_to_table(dagiom, channel_id, module_addr,  module_chan_data[module_index][bank][entry_index], command);


					/* in the chained position add raw connection number and suitable mask, without chaining */
					write_mask_to_table(dagiom, TIMESLOT_MASK, 0, module_addr,  module_chan_data[module_index][3][channel_id]);
					write_conn_num_to_table(dagiom, line, module_addr,  module_chan_data[module_index][1][channel_id], WRITE_CMD );

				}
				else
				{ 
					/* re-write the mask of this channel with chaining bit set */
					write_mask_to_table(dagiom, mask_space, 1, module_addr,  module_chan_data[module_index][bank+2][entry_index]);


					/* in the next position write the new channel number and mask without chaining */
					write_mask_to_table(dagiom, mask, 0, module_addr,  module_chan_data[module_index][3][conn_num]);
					write_conn_num_to_table(dagiom, channel_id, module_addr,  module_chan_data[module_index][1][conn_num], command);


				}

			}
			else
			{
				/* no, set up for next entry in chain */ 
				bank = 1;
				entry_index = conn_num;
				mask_space = (module_chan_data[module_index][3][entry_index] & READ_TIMESLOT_MASK) >> 12;
				conn_num = (module_chan_data[module_index][1][entry_index] & READ_CONN_MASK) >> 12;

			}



		}while(chain_bit != 0);

	}

	copy_to_execution_buffer(dagiom, module_addr);
	wait_not_busy(dagiom, module_addr);

	return 0;
}


int get_first_unused_channel()
{
	int i = 0;

	for(i = 16; i < 512; i++)
	{
		if (channel_list[i] != 1)
			return i;
	}
	return -1;
}

bool check_for_conflicts(uint32_t type, 
						 uint32_t tsConf,
						 int module_index,
						 int iface_start,
						 ts_used_t * pts_type)
{
	int i = 0;
	uint32_t mask = 0;
	uint32_t timeslot = 0;



	if(type == CT_RAW)
	{
		for(i = 1; i < 33; i++)
		{
			*pts_type = is_timeslot_used(module_index, iface_start + i , TIMESLOT_MASK);
			if( *pts_type == TS_RAW || *pts_type == TS_NON_CONFLICT_SUB_AND_RAW)
			{
				dagutil_verbose_level(1, "Raw channel already exists, new raw channel cannot be created %x %d\n", iface_start, *pts_type);
				return false;
			}
		}


	}
	else /* non raw types */
	{
		if(type == CT_HYPER || type == CT_HYPER_RAW)
		{
			for(i = 0; i< 32; i++)
			{
				if(((tsConf >> i) & 0x01) == 1)
				{
                    if(i == 0)
                    {
                        if((*pts_type = is_timeslot_used(module_index, iface_start + 31 , TIMESLOT_MASK)) == TS_NON_RAW)
					    {
						    dagutil_verbose_level(1, "Timeslot is in use, Channel cannot be created, timeslot = %x\n", iface_start + 31);
						    return false;
					    }
                    }
                    else if((*pts_type = is_timeslot_used(module_index, iface_start + i , TIMESLOT_MASK)) == TS_NON_RAW)
					{
						dagutil_verbose_level(1, "Timeslot is in use, Channel cannot be created, timeslot = %x\n", iface_start + i);
						return false;
					}
				}
			}
		}
		else 
		{
			/* at this stage we have either a sub channel or a simple channel ('c') type connection */
			/* check to see if there is already a channel on the timeslot */
			if(type == CT_SUB || type == CT_SUB_RAW)
			{
				mask = tsConf & 0x0000FFFF;
				timeslot = (tsConf & 0xFFFF0000 ) >> 16;
			}
			else 
			{
				mask = TIMESLOT_MASK;
				timeslot = tsConf ;
			}
			dagutil_verbose_level(1, "Timeslot = %d mask = %x tsConf = %x\n", timeslot, mask, tsConf);

			if((*pts_type = is_timeslot_used(module_index, iface_start + timeslot , mask)) == TS_NON_RAW)
			{
				dagutil_verbose_level(1, "Timeslot is in use, Channel cannot be created timeslot = %d\n", timeslot);
				return false;
			}

		}
	}

	return true;
}


ts_used_t is_timeslot_used(int module, 
						   int ts_offset, 
						   uint32_t mask)
{
	/* one is removed from the timeslot offset as the timeslots are numbered 1-31, then 0 */
	uint32_t mask_space = (module_chan_data[module][2][ts_offset-1] & READ_TIMESLOT_MASK) >> 12;
	uint32_t chain_bit = (module_chan_data[module][2][ts_offset-1] & READ_CHAIN_BIT_MASK) >> 20;
	uint32_t conn_num = (module_chan_data[module][0][ts_offset-1] & READ_CONN_MASK) >> 12;

	/* Is there any connection here at all? */
	if((mask_space & TIMESLOT_MASK) > 0 )
	{
		/* ok there is something here, now lets figure out what it is */

		/* is there just the one channel to check or do we have multiple channels to worry about? */
		if((chain_bit & 0x01) > 0 )
		{
			/* more pain, there is some chained connections , check this one, then continue to the next */
			/* first look for non conflicting sub channels */
			if((mask_space & mask) != 0 )
			{
				/* there is now either a raw connection, or a non raw connection, check the connection number */
				if(conn_num < 16)
					return TS_RAW;
				else 
					return TS_NON_RAW;
			}

			/* get next connection, this was a non conflicting sub_channel */
			do
			{   /*update information on new connection */
				mask_space = (module_chan_data[module][3][conn_num] & READ_TIMESLOT_MASK) >> 12;
				conn_num = (module_chan_data[module][1][conn_num] & READ_CONN_MASK) >> 12;

				/* check the mask of the chained connection */
				if((mask_space & mask ) == 0 )
				{
					/* yet another non conflicting sub channel check if this is the last*/
					chain_bit = ((module_chan_data[module][3][conn_num] & READ_CHAIN_BIT_MASK) >> 20) & 0x01;
				}
				else
				{
					/* this is a conflict.  Check if it is a raw or non raw */
					if(conn_num < 16)
						return TS_NON_CONFLICT_SUB_AND_RAW;
					else 
						return TS_NON_RAW;

					chain_bit = 0;
				}
			}
			while(chain_bit != 0);

			return TS_NON_CONFLICT_SUB;
		}
		else
		{
			/* at least there is just the one connection to check */
			/* first look for non conflicting sub channels */
			if((mask_space & mask) == 0 )
				return TS_NON_CONFLICT_SUB;
			else
			{
				/* there is now either a raw connection, or a non raw connection, check the connection number */
				if(conn_num < 16)
					return TS_RAW;
				else 
					return TS_NON_RAW;
			}
		}
	}
	else
		return TS_NOTHING;

}


/**
* Error check for any channel options which are in conflict with the modules available in this firmware
*/



bool validate_channel(uint32_t direction, 
					  uint32_t type, 
					  uint32_t format,
					  uint32_t uline, 
					  uint32_t tsConf,
					  modules_t module1,
					  modules_t module2)
{
	bool return_val = false;

	/*check the line and timeslot is within range */
	if((uline < 0) || (uline > max_lines)) 
	{
		dagutil_verbose_level(2, "line is outside of range. line = %d \n", uline);
		return return_val;
	}

	if(type == CT_SUB || type == CT_SUB_RAW)
	{
		if(((tsConf >> 16 ) < 1) || ((tsConf >> 16 ) > MAX_TIMESLOT)) 
		{
			dagutil_verbose_level(2, "timeslot is outside of range. timeslot = %d \n", tsConf >> 16);
			return return_val;
		}

	}
	else if(type == CT_SUB && type == CT_SUB_RAW)
	{
		if((tsConf < 1) || (tsConf > MAX_TIMESLOT)) 
		{
			dagutil_verbose_level(2, "timeslot is outside of range. timeslot = %d \n", tsConf);
			return return_val;
		}
	}
	else if(type == CT_RAW && direction == DAG_DIR_TRANSMIT) 
	{
		dagutil_verbose_level(2, "Raw transmit channels are not supported\n");
		return return_val;
	}

	/*check the firmware capabilities */
	if(direction != DAG_DIR_TRANSMIT)
	{
		if(module1.type == MOD_HDLC_RX)
		{
			return true;
		}
		else /* atm firmware */
		{
			/* check for unsupported channel types with ATM demapper */
			if(type != CT_RAW && type != CT_SUB && type != CT_SUB_RAW && 
				type != CT_HYPER_RAW && type != CT_CHAN_RAW)
			{ 

				return_val = true;
			}
			else
			{
				dagutil_verbose_level(2, "This channel type is not supported on ATM firmware\n");
				return return_val;
			}
		}
	}

	switch(module2.type)
	{
	case MOD_RAW_TX:
		if(direction == DAG_DIR_RECEIVE)
		{
			dagutil_verbose_level(2, "Channels do not exist in raw transmit, please set data into channels instead\n");
			return false;
		}
		break;  

	case MOD_HDLC_TX:
		if(direction == DAG_DIR_TRANSMIT)
			return_val = true;
		break;

	case MOD_ATM_TX:
		if(format == CT_FMT_HDLC)
			return false;

		/*There is a missing break here on purpose.  The drop through needs to occur */
	case MOD_ATM_RX:
		/* check for unsupported channel types with ATM demapper */
		if((type == CT_RAW || type == CT_SUB || type == CT_SUB_RAW) && (format == CT_FMT_ATM || direction == DAG_DIR_RECEIVE))
		{ 
			dagutil_verbose_level(2, "This channel type is not supported on ATM firmware\n");
			return return_val;
		}
		else
		{
			return_val = true;
		}
		break;

	case UNDEFINED:
	case MOD_HDLC_RX:
	    dagutil_verbose_level(3, "firmware module not recognised\n");
		return_val = false;
		break;
	}

	dagutil_verbose_level(3, "Channel passed all capability validation rules\n");
	return return_val;

}

int make_raw_channel(int dagfd, 
					uint32_t direction, 
					uint32_t type, 
					uint32_t uline, 
					uint32_t tsConf,
					uint32_t format)
{
	uint32_t ts=0;
	
	/* send any transmit channels to the mapper */
	if (direction == DAG_DIR_TRANSMIT) {
		switch (type) {
			case CT_SUB_RAW:
			case CT_SUB:
				ts = (tsConf & 0xFFFF0000) >> 16;
				tsConf &= 0xFF;
				break;
			case CT_HYPER_RAW:
			case CT_HYPER:
				break;
			case CT_CHAN_RAW:
			case CT_CHAN:
				ts = tsConf;
				tsConf = 0;
				break;
			default:
				dagutil_verbose_level(1, "Trying to add unknown channel type %d\n", type);  
				return -1;  
		} 

		return dag_mapper_add_channel(dagfd, (format | type), uline, ts, tsConf);
	}
	return -1;
}


