/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_chan_cfg.c,v 1.5 2007/05/02 23:50:41 vladimir Exp $
 */


#include "dag_platform.h"
#include "dagutil.h"
#include "dagapi.h"
#include "dag37t_api.h"

/* Internal project headers. */
#include "../include/channel_config/dag37t_chan_cfg.h"
#include "../include/channel_config/dag37t_map_functions.h"


/* look up table of the four different table offsets in the channel configuration table */
const uint32_t banks[4] = {0, 0x200, 0x400, 0x600};
/* Maps from logical line number to physical line */
const int logical_to_phy_line[16] = {
        13,
        5,
        7,
        15,
        4,
        12,
        14,
        6,
        11,
        3,
        8,
        0,
        2,
        10,
        1,
        9
};

/**
* go through the list of modules to determmine which demapper and/or mapper modules are available
*/

bool get_firmware_modules(int dagfd, 
						  modules_t *pmodule1, 
						  modules_t *pmodule2)
{
	dag_reg_t regs[DAG_REG_MAX_ENTRIES];
	int count = 0;
	int i = 0;
	uint8_t* dagiom = dag_iom(dagfd);
	
	count = dag_reg_find((char*) dagiom, 0, regs);
	if(count < 0)
	{
		dagutil_verbose_level(2, "dag_reg_find failed: %d\n", count);
		return false;
	}

	for (i = 0; i < count; i++)
	{
		switch (regs[i].module)
		{
		case DAG_REG_E1T1_HDLC_DEMAP:
			dagutil_verbose_level(3, "Found HDLC Demap module = 0x%.2x\n", regs[i].module);
			if(pmodule1->type != UNDEFINED)
			{
				pmodule2->type = pmodule1->type;
				pmodule2->addr = pmodule1->addr;
			}
			pmodule1->type = MOD_HDLC_RX;
			pmodule1->addr = regs[i].addr;
			break;

		case DAG_REG_E1T1_ATM_DEMAP:
			dagutil_verbose_level(3, "Found ATM Demap module = 0x%.2x\n", regs[i].module);
			if(pmodule1->type == UNDEFINED)
			{
				pmodule1->type = MOD_ATM_RX;
				pmodule1->addr = regs[i].addr;
			}
			else
			{
				pmodule2->type = MOD_ATM_RX;
				pmodule2->addr = regs[i].addr;
			}
			break;

		case DAG_REG_E1T1_HDLC_MAP:
			dagutil_verbose_level(3, "Found HDLC Map module = 0x%.2x\n", regs[i].module);
			pmodule2->type = MOD_HDLC_TX;
			pmodule2->addr = regs[i].addr;
			break;

		case DAG_REG_E1T1_ATM_MAP:
			dagutil_verbose_level(3, "Found ATM Map module = 0x%.2x\n", regs[i].module);
			if(pmodule1->type == UNDEFINED && pmodule2->type != UNDEFINED)
			{
				pmodule1->type = pmodule2->type;
				pmodule1->addr = pmodule2->addr;
			}
			pmodule2->type = MOD_ATM_TX;
			pmodule2->addr = regs[i].addr;
			break;

		case DAG_REG_RAW_TX:
			dagutil_verbose_level(3, "Found Raw Map module = 0x%.2x\n", regs[i].module);
			pmodule2->type = MOD_RAW_TX;
			pmodule2->addr = regs[i].addr;
			break;

		default:
			break;
		}
	}

	if(pmodule1->type == UNDEFINED || pmodule2->type == UNDEFINED)
	{
		dagutil_verbose_level(2, "Firmware Capabilities were not identified %d %d\n", pmodule1->type, pmodule2->type);
		return false;
	}
	dagutil_verbose_level(3, "Firmware Capabilities found %d %d\n", pmodule1->type, pmodule2->type);

	return true;



}


/**
* Read the channel configuration tables into memory
*/


bool read_channels(int dagfd, 
				   uint32_t addr1, 
				   uint32_t addr2)
{

	int offset = 0;
	int channel;
	int addr = addr1;
	int module = 0;
	int bank = 0;
    uint8_t* dagiom; 


    dagiom = dag_iom(dagfd);

	/* zero channel list in preparation */
	for(channel = 0; channel < MAX_CHANNEL; channel++)
		channel_list[channel] = 0;


	for(module = 0; module < 2; module++)
	{
		latch_counters(dagfd, addr);

		/*get next bank */
	    for(bank = 0; bank<4; bank++)
		{
			for(offset = 0; offset<512; offset++)
			{
				module_chan_data[module][bank][offset] = 
                    read_location(dagfd, dagiom, (banks[bank]+offset), addr);

#if defined NDEBUG
				/*ensure the correct address has been read from */
				dagutil_verbose_level(6,"read : %08x from module %04x bank %02x Address %03x\n",
					module_chan_data[module][bank][offset], addr, banks[bank], offset);
#endif
			}

		}
		/* reset for the second module */
		addr = addr2;

	}


	addr = addr1;

	/*now we have the channels, go through and set the list of channel numbers up */
	for(module = 0; module < 2; module++)
	{
		/*get next bank */
		for(bank = 0; bank<2; bank++)
		{
			for(offset = 0; offset<512; offset++)
			{
				/*check for a mask of some description */
				if((module_chan_data[module][bank+2][offset] & READ_CONN_MASK) > 0) 
				{

                    channel = ((module_chan_data[module][bank][offset] & READ_CONN_MASK) >> 12);
					channel_list[channel] = 1;

#if defined NDEBUG
					/*save the connection number */
					dagutil_verbose_level(4,"Found Connection module %x bank %x offset %x data 0x%08x mask 0x%08x\n", 
						module, bank, offset, module_chan_data[module][bank][offset], module_chan_data[module][bank+2][offset]);

					dagutil_verbose_level(4,"Found Connection number %d \n", channel);
#endif
				}
			}
		}
		/* reset for the second module */
		addr = addr2;

	}


    /* for debugging list the discovered connections if needed */
#if defined NDEBUG
	for(offset = 0;	offset <512; offset++)
	{
		if(channel_list[offset] != 0)
			dagutil_verbose_level(3, "Connection discovered %d\n", offset);

	}
#endif

	return true;
}

/**
* Read the channel configuration tables into memory
*/


bool read_connection_numbers(int dagfd, 
				   uint32_t addr1, 
				   uint32_t addr2)
{

	int offset = 0;
	int channel;
	int addr = addr1;
	int module = 0;
	int bank = 0;
    uint8_t* dagiom; 




    dagiom = dag_iom(dagfd);

	/* zero channel list in preparation */
	for(channel = 0; channel < MAX_CHANNEL; channel++)
		channel_list[channel] = 0;


	for(module = 0; module < 2; module++)
	{
		latch_counters(dagfd, addr);

		/*get next bank */
        for(bank = 0; bank<2; bank++)
		{
			for(offset = 0; offset<512; offset++)
			{
				module_chan_data[module][bank][offset] = 
                    read_location(dagfd, dagiom, (banks[bank]+offset), addr);

#if defined NDEBUG
				/*ensure the correct address has been read from */
				dagutil_verbose_level(6,"read : %08x from module %04x bank %02x Address %03x\n",
					module_chan_data[module][bank][offset], addr, banks[bank], offset);
#endif
			}

		}
		/* reset for the second module */
		addr = addr2;
     

	}


	addr = addr1;

	/* now we have the channels, go through and set the list of channel numbers up ,
     * note that the masks are not set up correctly, so there is the problem of finding 
     * channel 0, this will not be registered at this stage */
	for(module = 0; module < 2; module++)
	{
		/*get next bank */
		for(bank = 0; bank<2; bank++)
		{
			for(offset = 0; offset<512; offset++)
			{
                channel = ((module_chan_data[module][bank][offset] & READ_CONN_MASK) >> 12);

                /*check for a mask of some description */
				if(channel > 0 /*|| (channel == 0 && bank == 0 && offset <= 0x01F)*/) 
				{

					channel_list[channel] = 1;

#if defined NDEBUG
					/*save the connection number */
					dagutil_verbose_level(4,"Found Connection module %x bank %x offset %x data 0x%08x mask 0x%08x\n", 
						module, bank, offset, module_chan_data[module][bank][offset], module_chan_data[module][bank+2][offset]);

					dagutil_verbose_level(4,"Found Connection number %d \n", channel);
#endif
				}
			}
		}
		/* reset for the second module */
		addr = addr2;

	}


    /* for debugging list the discovered connections if needed */
#if defined NDEBUG
	for(offset = 0;	offset <512; offset++)
	{
		if(channel_list[offset] != 0)
			dagutil_verbose_level(3, "Connection discovered %d\n", offset);

	}
#endif

	return true;
}


void read_chaining(int dagfd, 
                   int module,
				   uint32_t addr)
{

	int offset = 0;
	int bank = 0;
    uint8_t* dagiom; 


    dagiom = dag_iom(dagfd);

    latch_counters(dagfd, addr);

    /*get next bank */
    for(bank = 2; bank<4; bank++)
    {
        for(offset = 0; offset<512; offset++)
        {
            module_chan_data[module][bank][offset] = 
                read_location(dagfd, dagiom, (banks[bank]+offset), addr);

#if defined NDEBUG
            /*ensure the correct address has been read from */
            dagutil_verbose_level(6,"read : %08x from module %04x bank %02x Address %03x\n",
                module_chan_data[module][bank][offset], addr, banks[bank], offset);
#endif

        }


    }



	return;
}

/**
* Find if a particular channel number is in use.
*/


bool is_channel_active(int dagfd, int channel_id)
{
   
   	modules_t module1;
	modules_t module2;
	module1.type = UNDEFINED;
	module2.type = UNDEFINED;

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
	if(!read_channels(dagfd, module1.addr, module2.addr))
	{
#if defined (_WIN32)
		LeaveCriticalSection(&ChannelMutex);
#endif
		dagutil_verbose_level(1, "Channels could not be read from the card\n");
		return -1;
	}

#if defined (_WIN32)
		LeaveCriticalSection(&ChannelMutex);
#endif

    if(channel_list[channel_id] == 1)
        return true;
    else
        return false;

}




void update_channel_data(int dagfd,					
                         modules_t *pmodule1, 
                         modules_t *pmodule2)
{
    int offset = 0;
    int channel;
    int module = 0;
    int bank = 0;
    int addr;
    uint8_t* dagiom;

    addr = pmodule1->addr;

    dagiom = dag_iom(dagfd);

    /* zero channel list in preparation */
    for(channel = 0; channel < MAX_CHANNEL; channel++)
        channel_list[channel] = 0;


    for(module = 0; module < 2; module++)
    {
        latch_counters(dagfd, addr);

        /*get next bank */
        for(bank = 0; bank<4; bank++)
        {
            for(offset = 0; offset<512; offset++)
            {
                module_chan_data[module][bank][offset] = read_location(dagfd, dagiom, (banks[bank]+offset), addr);

                /*ensure the correct address has been read from */
                dagutil_verbose_level(6,"read : %08x from module %04x bank %02x Address %03x\n",
                    module_chan_data[module][bank][offset], addr, banks[bank], offset);
            }

        }
        /* reset for the second module */
        addr = pmodule2->addr;
    }



}



int get_37t_channel_direction(int dagfd, int channel_id)
{
    int offset = 0;
    int channel;
    int module = 0;
    int bank = 0;
    int addr;

    modules_t module1;
    modules_t module2;
    module1.type = UNDEFINED;
    module2.type = UNDEFINED;

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

    update_channel_data(dagfd,	&module1, &module2);

    addr = module1.addr;

    /*now we have the channels, go through and look for the one we are interested in*/
    for(module = 0; module < 2; module++)
    {
        /*get next bank */
        for(bank = 0; bank<2; bank++)
        {
            for(offset = 0; offset<512; offset++)
            {
                /*check for a mask of some description */
                if((module_chan_data[module][bank+2][offset] & READ_CONN_MASK) > 0) 
                {
                    /*save the connection number */
                    dagutil_verbose_level(4,"Found Connection module %x bank %x offset %x data 0x%08x mask 0x%08x\n", 
                        module, bank, offset, module_chan_data[module][bank][offset], module_chan_data[module][bank+2][offset]);

                    channel = ((module_chan_data[module][bank][offset] & READ_CONN_MASK) >> 12);

                    if(channel == channel_id)
                    {
#if defined (_WIN32)
                        LeaveCriticalSection(&ChannelMutex);
#endif
                        //this is the channel we are interested in, send the direction
                        if(module == 1)
                            if(module2.type == MOD_HDLC_TX || module2.type == MOD_ATM_TX)
                                return DAG_DIR_TRANSMIT;


                        return DAG_DIR_RECEIVE;

                    }
                }
            }
        }
        /* reset for the second module */
        addr = module2.addr;

    }


#if defined (_WIN32)
    LeaveCriticalSection(&ChannelMutex);
#endif


    return -1;
}


int get_37t_channel_port_num(int dagfd, int channel_id)
{

    int offset = 0;
    int channel;
    int module = 0;
    int bank = 0;
    int addr;
    int port = 0;

    modules_t module1;
    modules_t module2;
    module1.type = UNDEFINED;
    module2.type = UNDEFINED;

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

    update_channel_data(dagfd, &module1, &module2);

    addr = module1.addr;

    /*now we have the channels, go through and look for the one we are interested in*/
    for(module = 0; module < 2; module++)
    {
        /*get next bank */
        for(bank = 0; bank<2; bank++)
        {
            for(offset = 0; offset<512; offset++)
            {
                /*check for a mask of some description */
                if((module_chan_data[module][bank+2][offset] & READ_CONN_MASK) > 0) 
                {
                    /*save the connection number */
                    dagutil_verbose_level(4,"Found Connection module %x bank %x offset %x data 0x%08x mask 0x%08x\n", 
                        module, bank, offset, module_chan_data[module][bank][offset], module_chan_data[module][bank+2][offset]);

                    channel = ((module_chan_data[module][bank][offset] & READ_CONN_MASK) >> 12);

                    if(channel == channel_id)
                    {
  #if defined (_WIN32)
                        LeaveCriticalSection(&ChannelMutex);
#endif
                        //this is the channel we are interested in, work backwards to find 
                        //the port number
                        if(channel_id < 16)
                        {
                            //raw channel, return transformed number
                            return logical_to_phy_line[channel_id];
                        }
                        else
                        {
                            if(bank == 0)
                            {
                                //no chaining, work out port number from address position
                                port = (int)(offset/32);
                                return logical_to_phy_line[port];


                            }
                            else
                            {
                                //chained connection. nasty. Work back through chain to find source
                                return find_chained_connection_port(channel_id, offset-0x200, &module1, &module2);
                            }

                        }




                       

                    }
                }
            }
        }
        /* reset for the second module */
        addr = module2.addr;

    }


#if defined (_WIN32)
    LeaveCriticalSection(&ChannelMutex);
#endif

    //channel not found.  return error
    return -1;
}



int find_chained_connection_port(int channel_id, 
                                 int prev_channel,
                                 modules_t *pmodule1,
                                 modules_t *pmodule2)
{
    int offset = 0;
    int channel;
    int module = 0;
    int bank = 0;
    int addr = pmodule1->addr;
    int port = 0;



    /* go through and look for the position of the previous connection number
    * in the chain, untill we have a connection that is first in the chain */
    for(module = 0; module < 2; module++)
    {
        /*get next bank */
        for(bank = 0; bank<2; bank++)
        {
            for(offset = 0; offset<512; offset++)
            {
                /*check for a mask of some description */
                if((module_chan_data[module][bank+2][offset] & READ_CONN_MASK) > 0) 
                {
                    /*save the connection number */
                    dagutil_verbose_level(4,"Found Connection module %x bank %x offset %x data 0x%08x mask 0x%08x\n", 
                        module, bank, offset, module_chan_data[module][bank][offset], module_chan_data[module][bank+2][offset]);

                    channel = ((module_chan_data[module][bank][offset] & READ_CONN_MASK) >> 12);

                    if(channel == prev_channel)
                    {

                        //this is the previous channel we are interested in, 
                        //check if this is the first in the chain
                        if(bank == 0)
                        {
                            //first in chain, return the port number
                            port = (int)(offset/32);
                            return logical_to_phy_line[port];


                        }
                        else
                        {
                            //intermediate chained connection. Keep working through chain to find source
                            return find_chained_connection_port(channel_id, offset-0x200, pmodule1, pmodule2);

                        }

                    }
                }
            }
        }
        /* reset for the second module */
        addr = pmodule2->addr;

    }

    /* something has gone wrong, either the chain is broken, or a connection number has been 
     * missed.  In either case, return error */

    return -1;
}



