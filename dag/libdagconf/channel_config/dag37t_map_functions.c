/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_map_functions.c,v 1.6 2007/06/18 04:22:11 vladimir Exp $
 */

#include "dagutil.h"
#include "dagapi.h"
#include "dag_platform.h"

#include "../include/channel_config/dag37t_map_functions.h"

int latch_counters(int dagfd, uint32_t module_addr)
{
	uint8_t* dagiom = dag_iom(dagfd);

	uint32_t reg = *(volatile uint32_t*)(dagiom + module_addr);
	reg |= LATCH_MASK;

	dagutil_verbose_level(4, "latching counters....\n");
	*(volatile uint32_t*)(dagiom + module_addr) = reg;
	

	return 0;
}

INLINE void reset_command_count(int dagfd, uint8_t* dagiom, uint32_t module_addr)
{
    uint32_t reg_val = 0;


#if defined(NDEBUG)
	dagutil_verbose_level(6, "Reset command count\n");
#endif

	/* reset the command counter first (write to 0x10 in register 0x04) */
	reg_val = *(volatile uint32_t*)( dagiom + module_addr + CFG_BUF_CMD_REG );

	reg_val &= ~RESET_MASK;
	reg_val |= RESET_COMMAND; 

	*(volatile uint32_t*)(dagiom + module_addr + CFG_BUF_CMD_REG) = reg_val;

	reg_val &= ~RESET_MASK;

	*(volatile uint32_t*)(dagiom + module_addr + CFG_BUF_CMD_REG) = reg_val;

}



uint32_t read_location (int dagfd, uint8_t* dagiom, int addr, uint32_t module_addr )
{
    uint32_t return_value;


	/* reset the command counter first (write to 0x10 in register 0x04) */
	reset_command_count(dagfd, dagiom, module_addr);



	/* write the read command (write the read command, and the address to read from into the cfg write register) 
	* this will cause the data at the address to be put in the data register for reading
	*/
	iom_write(dagiom, module_addr + CFG_WRITE_CMD_REG, (CONFIG_ADDR_MASK | CMD_MASK), 
		((addr & CONFIG_ADDR_MASK) | READ_CMD));


	/* copy to execution buffer (write to 0x07 in register 0x04) 
	* This sets up the read command for execution by the firmware 
	*/
	copy_to_execution_buffer(dagiom, module_addr);

	/* Wait for not busy */
	wait_not_busy(dagiom, module_addr);

	/* tell firmware to execute command (write to 0x07 in register 0x04) */
	iom_write(dagiom, module_addr + CFG_BUF_CMD_REG, CFG_BUF_CMD_MASK, CFG_BUF_EXEC_CMD);

	/* wait for data to be transfered to the buffer (yes it is possible to read the 
	* register before the data is there! )
	*/
	wait_not_busy(dagiom, module_addr);

	/* read the data and return to calling routine */
	return_value = iom_read(dagiom, module_addr +CFG_TABLE_DATA_REG);

    return return_value;

}


int wait_not_busy(uint8_t* dagiom, uint32_t module)
{
	int count=5;
	/* wait for the card to not be busy by reading the ready bit on the 
	* Configuration Buffer Data register (bit 8, register 0x08)*/
	while((iom_read(dagiom, module + CFG_BUF_DATA_REG) & CMD_BUF_READY) == 0 && count > 0) 
	{
#if defined(NDEBUG)
		dagutil_verbose_level(6, "waiting for command buffer for module 0x%8x\n", module);
#endif
        count--;
	}

#if defined(NDEBUG)
	if(count == 0)
	{
		dagutil_verbose_level(1, "Card did not leave busy state\n");
	}
#endif
	return 0;

}

INLINE int copy_to_execution_buffer(uint8_t *dagiom, int module_addr)
{
	return iom_write(dagiom, module_addr + CFG_BUF_CMD_REG, CFG_BUF_CMD_MASK, CFG_BUF_COPY_CMD);
}



int write_conn_num_to_table(uint8_t* dagiom, uint32_t conn_num, uint32_t module_addr,  uint32_t table_entry, uint32_t command)
{
	/*add connection number to table entry */
	table_entry &= ~READ_CONN_MASK;
	table_entry |= (conn_num << 12);

	/*set up the write entry command in it */
	table_entry &= ~CMD_MASK;
	table_entry |= command;


	dagutil_verbose_level(5, "writing 0x%08x to %p (conn_num)\n", table_entry, (void *)(dagiom+module_addr+CFG_WRITE_CMD_REG));
	iom_write(dagiom, module_addr+CFG_WRITE_CMD_REG, WRITE_ALL_MASK, table_entry); 

	/* do the copy instructions and wait for completion */
	copy_to_execution_buffer(dagiom, module_addr);
	wait_not_busy(dagiom, module_addr);

	return 0;

}

int write_mask_to_table(uint8_t* dagiom, uint32_t mask, uint32_t chain_bit, uint32_t module_addr, uint32_t table_entry)
{
	/* add mask to table entry */
	table_entry &= ~READ_TIMESLOT_MASK;
	table_entry |= (mask << 12);

	/* add chaining bit to table entry */
	table_entry &= ~READ_CHAIN_BIT_MASK;
	if(chain_bit != 0)
		table_entry |= READ_CHAIN_BIT_MASK;

	/* set up the write entry command in it */
	table_entry &= ~CMD_MASK;
	table_entry |= WRITE_CMD;

	dagutil_verbose_level(5, "writing 0x%08x to %p (mask)\n", table_entry, (void *)(dagiom+module_addr+CFG_WRITE_CMD_REG));
	iom_write(dagiom, module_addr+CFG_WRITE_CMD_REG, WRITE_ALL_MASK, table_entry); 

	/* do the copy instructions and wait for completion */
	copy_to_execution_buffer(dagiom, module_addr);
	wait_not_busy(dagiom, module_addr);

	return 0;

}

int set_raw_mode(int dagfd, int module_addr, int line)
{
	uint8_t* dagiom = dag_iom(dagfd);

	return iom_write(dagiom, module_addr+MAP_CTRL_REG, (0x01 << (16 + line)),  (0x01 << (16 + line)));


}


int execute_command(int dagfd, int module_addr)
{
	uint8_t* dagiom = dag_iom(dagfd);

	iom_write(dagiom, module_addr+CFG_BUF_CMD_REG, CFG_BUF_CMD_MASK,  CFG_BUF_EXEC_CMD);

	wait_not_busy(dagiom, module_addr);
	
	reset_command_count(dagfd, dagiom, module_addr);
	
	return 0; 


}


INLINE  uint32_t iom_read(uint8_t* dagiom, uint32_t addr)
{
	return *(volatile uint32_t*)(dagiom + addr);
}


INLINE uint32_t iom_write(uint8_t* dagiom, uint32_t addr, uint32_t mask,  uint32_t val)
{

	uint32_t reg_val = 0;

	reg_val = *(volatile uint32_t*)( dagiom + addr );

	reg_val &= ~mask;
	reg_val |= val; 

#if defined(NDEBUG)
	dagutil_verbose_level(7, "writing %08x to %08x\n", reg_val, *dagiom+addr);
#endif

	*(volatile uint32_t*)(dagiom + addr) = reg_val;

	return 0;
}

