/*
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: idt75k_lib.c,v 1.9 2008/05/07 22:20:02 dlim Exp $
 */
/*
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif
#include "idt75k_lib.h"
#include "idt75k_interf.h"
#include "dagapi.h"
#include "dag_config.h"
#include "dag_component.h"
#include "dag_platform.h"
#include "dagutil.h"

#ifndef _WIN32
#undef DD
#undef DA
#ifndef NDEBUG
#define DD(...) printf( __VA_ARGS__)
#else
#define DD(...) if(0) {printf( __VA_ARGS__);}
#endif
#define DA(...) printf( __VA_ARGS__)
#endif

//this need to be verified TODO
#define memcpy_toio memcpy

//#define MAILBOX_ADDRESS 0x103F8000
//need to construct the mailbox address based on the context for full production testing
#define IDT75K_CONSTRUCT_MAILBOX_ADDRESS(context) \
        (uint32_t)(((uint32_t)(context & 0x7F) << 15) | \
                    ((1<<28)))

//CHECK me it may require dagutil_nanosleep or microsleep for exact timings   
#define udelay usleep

/** Constant that defines the maximum number of times to attempt to read from a mailbox before timing-out */
#define MAX_RD_MAILBOX_ATTEMPTS      256

/** Defines the number of microseconds to delay between attempting to read from the mailbox */
#define READ_MAILBOX_INTERVAL        2

#define CONFIG_STATUS_ADDRESS_REG  0x00
#define COMMAND_RAM_HIGH_DATA      0x04
#define COMMAND_RAM_LOW_DATA       0x08
#define READ_INTERFACE_DATA        0x0C
/**
 * Macro that attempts to read the first word from the mailbox, this routine tries MAX_RD_MAILBOX_ATTEMPTS
 * times to read a result with the done bit set (bit 31). The macro completes when either the MAX_RD_MAILBOX_ATTEMPTS
 * tries has been exhausted or the value read has the "done" bit set.
 */
#define READ_MAILBOX_RESULT(x) \
	do { \
		int attempts = 0; \
		x = IDT75K_READ_MAILBOX(dev_p, 0, 0); \
		while ( ((x & 0x80000000) == 0) && (attempts < MAX_RD_MAILBOX_ATTEMPTS) ) { \
			attempts++; \
			x = IDT75K_READ_MAILBOX(dev_p, 0, 0); \
		} \
	} while (0);

#define read_mailbox read_command 

uint32_t read_command(idt75k_dev_t *dev_p,uint32_t context,int offset)
{
	int attempts = 0;
	uint32_t result;
        uint32_t mailbox_address = IDT75K_CONSTRUCT_MAILBOX_ADDRESS(context);	
	
	/*mailbox address offset 0 along with read bit.*/
	*(volatile uint32_t*)(dev_p->mmio_ptr + CONFIG_STATUS_ADDRESS_REG) = (mailbox_address + offset);
		
	result = *(volatile uint32_t*)(dev_p->mmio_ptr + CONFIG_STATUS_ADDRESS_REG);
	while( ((result & (1 << 28)) == 1) && (attempts < MAX_RD_MAILBOX_ATTEMPTS) )
	{
		result = *(volatile uint32_t*)(dev_p->mmio_ptr + CONFIG_STATUS_ADDRESS_REG); 
		attempts ++;
	}
	result = *(volatile uint32_t*) (dev_p->mmio_ptr + READ_INTERFACE_DATA);
	if( attempts == MAX_RD_MAILBOX_ATTEMPTS )
	   	printf("read_mailbox_noresult: timout error fix the firmware :) . \n");
		
	/*done bit in the mailbox result set*/

#ifndef _WIN32
	DD("read mailbox : %x \n",result);
#endif
	return result;
}
uint32_t read_mailbox_result(idt75k_dev_t *dev_p,uint32_t context,int offset)
{
	int attempts;
	uint32_t result;
	/*mailbox address offset 0 along with read bit.*/
	
	attempts = 0;
	result = read_command(dev_p,context,offset);
	while( ((result & 0x80000000) == 0) && (attempts < MAX_RD_MAILBOX_ATTEMPTS) ) {
		result = read_command(dev_p,context,offset);
		attempts++;
	}
	if( attempts == MAX_RD_MAILBOX_ATTEMPTS )
	   	printf("read_mailbox_result: timout TCAM did not answered the prev. command :) . \n");		

#ifndef _WIN32
	DD("MailBox Result : %x \n",result);
#endif
	return result;
}
/*Prepare the low data and high data for the command RAM*/
void dag_write_ram_mode_eighteen(uint8_t *mTCamBase,uint64_t *wr_word,int len) 
{
	int i = 0;
	uint32_t upper_mask = 0;
	uint32_t lower_mask = 0;
	uint32_t hdr_mask = 0;
	uint32_t hdr_shift = 0;
	uint32_t ldr_shift = 0;
	uint32_t end_address = 0;
	uint64_t temp = 0;
	for(i=0 ; i < len ; i++)
	{
		if(i > 0)
		{
			if(i == 1)
			{
				upper_mask = 0xf0000000;
				lower_mask = 0x0fffffff;
				hdr_mask   = 0xf0000000;
				hdr_shift  = 28;
				ldr_shift  = 4;
				end_address = (((len-1) * 32)/36) + 1;
				temp = ((((wr_word[i + 1] & hdr_mask) >> hdr_shift)) | (wr_word[i] & 0xffffffff) << 4);
#ifndef _WIN32
				DD("36 bit contents of the command ram : %"PRIu64"\n",temp);
#endif
				*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA) =  (uint32_t) ((temp & 0xffffffff));
				*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA) =  (((temp & 0xf00000000LL) >> 32)  | (i << 12) | (end_address << 8) | (1 << 4));
				continue;
			}
			if (i == 2)
			{
				upper_mask = 0xf0000000;
				lower_mask = 0x0fffffff;
				hdr_mask   = 0x0f000000;
				hdr_shift  = 24;
				ldr_shift  = 8;
				end_address = (((len-1) * 32)/36) + 1;
			}
			temp = (((((wr_word[i+1] & upper_mask) >> hdr_shift) | ((wr_word[i] & lower_mask) << ldr_shift))) | (((wr_word[i+1] & (hdr_mask)) >> (hdr_shift)))); 

#ifndef _WIN32
			DD("36 bit contents fo the command ram : %"PRIu64"\n",temp);
#endif

			*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA) = ((temp & 0xffffffff));
			*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA) = (((temp & 0xf00000000LL) >> 32) |(i << 12) |(end_address << 8)|(1 << 4));
			hdr_mask   = (hdr_mask >> 4);
			hdr_shift  = hdr_shift - 4;
			ldr_shift  = ldr_shift + 4;
			lower_mask = (lower_mask >> 4);
			upper_mask = (upper_mask | (upper_mask >> 4));
			 
		}
		else
		{
			*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA) = ((wr_word[i] & 0xFFFF) | ((wr_word[i] & 0x3fff0000) << 2));
#ifndef _WIN32
			DD("low data : %x\n",*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA));
#endif
			*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA) = ((wr_word[i] >> 30) | (i << 12) | ((len-1) << 8) | (1 << 4));
#ifndef _WIN32
			DD("high data : %x\n",*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA));
#endif
		}

	}
}
/*Prepare the low data and high data for the command RAM*/
void dag_write_ram_mode_eighteen_direct(uint8_t *mTCamBase,uint64_t *wr_word,int len) 
{
	int i = 0;
	uint32_t upper_mask = 0;
	uint32_t lower_mask = 0;
	uint32_t hdr_mask = 0;
	uint32_t hdr_shift = 0;
	uint32_t ldr_shift = 0;
	uint32_t end_address = 0;
	uint64_t temp = 0;
	for(i=0 ; i < (len -1) ; i++)
	{
			if(i == 0)
			{
				upper_mask = 0xf0000000;
				lower_mask = 0x0fffffff;
				hdr_mask   = 0xf0000000;
				hdr_shift  = 28;
				ldr_shift  = 4;
				end_address = (((len-1) * 32)/36) + 1;
				temp = ((((wr_word[i + 1] & hdr_mask) >> hdr_shift)) | (wr_word[i] & 0xffffffff) << 4);
#ifndef _WIN32
				DD("36 bit contents of the command ram : %"PRIu64" \n",temp);
#endif
				*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA) =  (uint32_t) ((temp & 0xffffffff));
				*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA) =  (((temp & 0xf00000000LL) >> 32)  | (i << 12) | (end_address << 8) | (1 << 4));
				continue;
			}
			if (i == 1)
			{
				upper_mask = 0xf0000000;
				lower_mask = 0x0fffffff;
				hdr_mask   = 0x0f000000;
				hdr_shift  = 24;
				ldr_shift  = 8;
				end_address = (((len-1) * 32)/36) + 1;
			}
			temp = (((((wr_word[i+1] & upper_mask) >> hdr_shift) | ((wr_word[i] & lower_mask) << ldr_shift))) | (((wr_word[i+1] & (hdr_mask)) >> (hdr_shift)))); 
#ifndef _WIN32
			DD("36 bit contents fo the command ram : %"PRIu64" \n",temp);
#endif
			*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA) = ((temp & 0xffffffff));
			*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA) = (((temp & 0xf00000000LL) >> 32) |(i << 12) |(end_address << 8)|(1 << 4));
			hdr_mask   = (hdr_mask >> 4);
			hdr_shift  = hdr_shift - 4;
			ldr_shift  = ldr_shift + 4;
			lower_mask = (lower_mask >> 4);
			upper_mask = (upper_mask | (upper_mask >> 4));
			 

	}
}
/*new code*/
void dag_write_ram(uint8_t *mTCamBase,uint64_t *wr_word,int len)
{
	int i = 0;
	for (i = 0;i < len;i++)
	{
		*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA) = ((wr_word[i] & 0xFFFF) | ((wr_word[i] & 0x3fff0000) << 2));
#ifndef _WIN32
		DD("low data : %x\n",*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_LOW_DATA));
#endif
		*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA) = ((wr_word[i] >> 30) | (i << 12) | ((len-1) << 8) | (1 << 4));
#ifndef _WIN32
		DD("high data : %x\n",*(volatile uint32_t*)(mTCamBase + COMMAND_RAM_HIGH_DATA));
#endif
	}

}
void memcpy_la1(idt75k_dev_t *ptr, uint32_t *wr_word,int size)
{
	/*writing to the csr region*/
	uint64_t temp[10];
	uint32_t csr;
	int region = 0;
	int i;
	int instruction = 0;
	for( i = 1 ; i < size; i++ )  temp[i-1] = wr_word[i];
	
	/*preparing the data in the command ram.*/
	instruction = ((wr_word[0] & ((1 << 13)|(1 << 14)))>> 13);
	region = ((wr_word[1] & ((1 << 23) | (1 << 24)| (1 << 25))) >> 23);
	if(instruction == 0)
	{
		dag_write_ram_mode_eighteen_direct(ptr->mmio_ptr,temp,size-1);	
	}
	else if((region == IDT75K_REGION_CORE_DATA)|| (region == IDT75K_REGION_CORE_MASK) || (region == IDT75K_REGION_GMR))
	{
		dag_write_ram_mode_eighteen( ptr->mmio_ptr, temp, size-1);
	}
	else/*Command Ram to be programmed in 16 bit mode*/
	{
		dag_write_ram(ptr->mmio_ptr,temp,size-1);	
	}
	/*setting the go bit*/
	*(volatile uint32_t*)(ptr->mmio_ptr + CONFIG_STATUS_ADDRESS_REG) = ((1 << 29)| wr_word[0]);

	csr = *(volatile uint32_t*)(ptr->mmio_ptr + CONFIG_STATUS_ADDRESS_REG);
	while((csr & (1 << 29)) == 1) 
	{
#ifndef _WIN32
		DD("Command submission Engine Busy\n");
#endif
		csr = *(volatile uint32_t*)(ptr->mmio_ptr + CONFIG_STATUS_ADDRESS_REG);
	}
} 
/* Reads one of the standard configuration registers. For a list
 * of available configuration registers see the idt75k_csr_t enum.
 *
 * All reads return a 32-bit value regardless of the size of the
 * CSR.
 *
 * @param[in]  dev_p    The PCI device structure for the driver
 * @param[in]  addr     The 19-bit address of the CSR to read, possible
 *                      addresses are listed oin Table5.2 of the datasheet.
 * @param[out] value_p  Upon success this variable will contain the value
 *                      read from the CSR.
 *
 * @return              Returns one of the following error codes.
 * @retval ESUCCESS     The operation was successiful and res contains
 *                      the value read from the chip.
 * @retval EINVAL       An invalid argument was supplied
 * @retval ETIMEDOUT    Timedout waiting for the chip to indicate the reading
 *                      procedure was complete.
 * @retval EFAULT       The error bit was set by the chip in response to
 *                      the read command.
 */
/*function added to maintain the old interface*/
int idt75k_read_csr(idt75k_dev_t *dev_p,uint32_t addr, uint32_t * value_p)
{
	uint32_t context = 127;
	int return_value = 0;
	return_value = idt75k_read_csr_ctx(dev_p,context,addr,value_p);
	return return_value;
}
//chaned the Interface to pass context as a parameter.needed for full production testing.
int
idt75k_read_csr_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t addr, uint32_t *value_p)
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[2];
#ifndef _WIN32
	DD(  "idt75k: idt75k_read_csr(0x%p, 0x%08X, 0x%p)\n", dev_p, addr, value_p );
#endif
	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x01F3 )
		return EINVAL;
	if ( value_p == NULL )
		return EINVAL;

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */

	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, 0, 0)
	#endif
	
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context, INSTR_INDIRECT, 0, 0, 0);
	wr_words[1] = ((SUBINSTR_READ << 28) | (IDT75K_REGION_CSR << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF));


	/* write the data to the chip */
#ifndef _WIN32
	DD("word [0] : %x \n",wr_words[0]);
	DD("word [1] : %x \n",wr_words[1]);
#endif

	memcpy_la1(dev_p, wr_words, 2);

	rd_words[0] = read_mailbox_result(dev_p,context,0);
	
	/* Read the result back from the chip and wait till the done bit is set */
	
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	
	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
	{
#ifndef _WIN32
		DD(  "idt75k: idt75k_read_csr(0x%p, 0x%08X, 0x%p) timed-out, mailbox response = 0x%08X\n", dev_p, addr, value_p, rd_words[0]) ;
#endif
		return ETIMEDOUT;
	}

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
	{
#ifndef _WIN32
		DD(  "idt75k: idt75k_read_csr(0x%p, 0x%08X, 0x%p) failed, mailbox response = 0x%08X\n", dev_p, addr, value_p, rd_words[0]) ;
#endif
		return EFAULT;
	}
	/* Read the second result word in the mailbox and asign it to the value */
	
	#if 0
		rd_words[1] = IDT75K_READ_MAILBOX(dev_p, 0, 1);
	#endif

	rd_words[1] = read_mailbox(dev_p ,context,1);	
	
	*value_p = rd_words[1];
	return ESUCCESS;
}
/**
 * Writes a 32-bit value to a CSR on the chip.
 *
 * @param[in]  dev_p  The PCI device structure for the driver
 * @param[in]  addr   The 19-bit address of the CSR to read, possible
 *                    addresses are listed oin Table5.2 of the datasheet.
 * @param[out] value  New 32-bit value to load into the CSR.
 *
 * @return            Returns one of the following error codes.
 * @retval ESUCCESS   The operation was successiful.
 * @retval EINVAL     An invalid argument was supplied.
 * @retval ETIMEDOUT  Timedout waiting for the chip to indicate the write
 *                    procedure was complete.
 * @retval EFAULT     The error bit was set by the chip in response to
 *                    the write command.
 */
/*to maintain the original interface*/
int
idt75k_write_csr(idt75k_dev_t *dev_p, uint32_t addr, uint32_t value)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_write_csr_ctx(dev_p,context,addr,value);
	return return_value;
}
//chaned the Interface to pass context as a parameter.needed for full production testing.
int
idt75k_write_csr_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t addr, uint32_t value)
{
	uint32_t      wr_words[3];
	uint32_t      rd_words[1];

#ifndef _WIN32
	DD(  "idt75k: idt75k_write_csr(0x%p, 0x%08X, 0x%08X)\n", dev_p, addr, value );
#endif


	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x01F3 )
		return EINVAL;


	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 2, INSTR_INDIRECT, 0, 0, 0);
	#endif
	
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE (context,INSTR_INDIRECT, 0, 0, 0);
	wr_words[1] = (SUBINSTR_WRITE << 28) | (IDT75K_REGION_CSR << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);
	wr_words[2] = value;

	/* write the data to the chip */
	#if 0
		memcpy_toio((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 3));
	#endif

	memcpy_la1(dev_p,wr_words,3);

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	/* Success */
	return ESUCCESS;
}
/**
 * Configures one of the 16 possible databases, by writing into the three
 * DB configuration registers. This driver doesn't maintain any information
 * about databases it has configured, it simply writes the configuration
 * information directly to the registers, with minimal sanity checking on
 * the inputs.
 *
 * @param[in]  dev_p    The driver defined structure
 * @param[in]  db_num   The database number to configure, valid values are
 *                      0 to 15.
 * @param[in]  mask     The configuration mask that defines the attributes
 *                      that will be configured by this call.
 * @param[in]  flags    The flags that are being set on the system, only
 *                      the flags that are masked will be set.
 * @param[in]  db_size  The size of the entry lookups for this database.
 * @param[in]  age_cnt  The age count for the database.
 * @param[in]  segments Bitmask of the segments assigned to this database,
 *                      the LSB contains the bit for segment 0. Note that
 *                      the maximum number of sectors that can be assigned
 *                      to a database is 16.
 *
 * @return              Returns one of the following error codes.
 * @retval ESUCCESS     The operation was successiful.
 * @retval EINVAL       An invalid argument was supplied
 * @retval ETIMEDOUT    Timedout waiting for the chip to indicate the reading
 *                      procedure was complete.
 * @retval EFAULT       The error bit was set by the chip in response to
 *                      the read command.
 */
//changed the interface to pass context as a parameter.used for full production testing
int
idt75k_set_dbconf(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t mask, uint32_t flags, uint32_t age_cnt, uint32_t segments)
{
	int return_value = 0;
	uint32_t context = 127;

	return_value = idt75k_set_dbconf_ctx(dev_p,context,db_num,mask,flags,age_cnt,segments);
	return return_value;
}
//changed the interface to pass context as a parameter.used for full production testing
int
idt75k_set_dbconf_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t mask, uint32_t flags, uint32_t age_cnt, uint32_t segments)
{
	uint32_t  dbconf1_addr, dbconf1_value;
	uint32_t  dbconf2_addr, dbconf2_value;
	uint32_t  dbconf3_addr, dbconf3_value;
	int       err;
	int       bit;
	int       num_segments;

#ifndef _WIN32
	DD(  "idt75k: idt75k_get_dbconf(0x%p, %d, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n", dev_p, db_num, mask, flags, age_cnt, segments) ;
#endif

	/* Sanity check the input *///set_database_sectors
	if ( db_num > 15 )
		return EINVAL;
	if ( dev_p == NULL )
		return EINVAL;


	/* This is a sanity check to ensure that only the supported amount of segments are added to a database */
	num_segments = 0;

	for (bit=0; bit<32; bit++)
		if ( segments & (1 << bit) )
			num_segments++;

	if ( num_segments > 8 )
		return EINVAL;



	dbconf1_addr = 0x0100 | (db_num << 4);
	dbconf2_addr = 0x0101 | (db_num << 4);
	dbconf3_addr = 0x0102 | (db_num << 4);
	

	
	/* Read the current values from the registers */
	if ( (err = idt75k_read_csr_ctx(dev_p,context,dbconf1_addr, &dbconf1_value)) != ESUCCESS )
		return err;
	if ( (err = idt75k_read_csr_ctx(dev_p,context,dbconf2_addr, &dbconf2_value)) != ESUCCESS )
		return err;
	if ( (err = idt75k_read_csr_ctx(dev_p,context,dbconf3_addr, &dbconf3_value)) != ESUCCESS )
		return err;



	/* Perform the commands specified by the mask and argument */
	if ( mask & IDT75K_DBCONF_POWERSAVE_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_POWERSAVE_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_POWERSAVE_MASK);
	}
	if ( mask & IDT75K_DBCONF_AR_ONLY_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_AR_ONLY_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_AR_ONLY_MASK);
	}
	if ( mask & IDT75K_DBCONF_AGE_ENABLE_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_AGE_ENABLE_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_AGE_ENABLE_MASK);
	}
	if ( mask & IDT75K_DBCONF_AD_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_AD_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_AD_MASK);
	}
	if ( mask & IDT75K_DBCONF_AFR_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_AFR_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_AFR_MASK);
	}
	if ( mask & IDT75K_DBCONF_AFS_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_AFS_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_AFS_MASK);
	}
	if ( mask & IDT75K_DBCONF_RTN_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_RTN_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_RTN_MASK);
	}
	if ( mask & IDT75K_DBCONF_GMR_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_GMR_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_GMR_MASK);
	}
	if ( mask & IDT75K_DBCONF_ACTIVITY_ENABLE_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_ACTIVITY_ENABLE_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_ACTIVITY_ENABLE_MASK);
	}
	if ( mask & IDT75K_DBCONF_CORE_SIZE_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_CORE_SIZE_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_CORE_SIZE_MASK);
	}
	
	/* Set the DB_SIZE values, this is actually just the number of transactions required to access a core entry */
	if ( mask & IDT75K_DBCONF_DB_SIZE_MASK )
	{
		dbconf1_value &= ~IDT75K_DBCONF_DB_SIZE_MASK;
		dbconf1_value |= (flags & IDT75K_DBCONF_DB_SIZE_MASK);
	}

	/* Set the age count if specified in the mask */
	if ( mask & IDT75K_DBCONF_AGE_COUNT_MASK )
		dbconf2_value = age_cnt & 0x00FFFFFF;

	/* Set the segments that will be encompassed in the database (if specified in the mask) */
	if ( mask & IDT75K_DBCONF_SEGMENT_SEL_MASK )
	{
		dbconf3_value = segments;
	
	}

	/* Write all the data back to the device */
	if ( (err = idt75k_write_csr_ctx(dev_p,context,dbconf1_addr, dbconf1_value)) != ESUCCESS )
		return err;
	if ( (err = idt75k_write_csr_ctx(dev_p,context,dbconf2_addr, dbconf2_value)) != ESUCCESS )
		return err;
	if ( (err = idt75k_write_csr_ctx(dev_p,context,dbconf3_addr, dbconf3_value)) != ESUCCESS )
		return err;


	return ESUCCESS;
}
//to keep the original interface intact
int
idt75k_set_nse_conf(idt75k_dev_t *dev_p, uint32_t mask, uint32_t value)
{
	int return_value = 0;
	uint32_t context = 127;
	idt75k_set_nse_conf_ctx(dev_p,context,mask,value);
	return return_value;

}
//changed the interface to pass context as a parameter.needed for full production testing.
int
idt75k_set_nse_conf_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t mask, uint32_t value)
{
	uint32_t nseconf_address, nseconf_value;
	int err;	
	
	if ( dev_p == NULL )
	      return EINVAL;

	nseconf_address = 0x0000;
	
	/* Read the current values from the registers */
	if ( (err = idt75k_read_csr_ctx(dev_p,context,nseconf_address, &nseconf_value)) != ESUCCESS )
		return err;

	if( mask & IDT75K_NSECONF_AR_ONLY_GLOBAL)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_AR_ONLY_GLOBAL;
	        nseconf_value |= (value & IDT75K_NSECONF_AR_ONLY_GLOBAL);
	}
	if( mask & IDT75K_NSECONF_L1PARITY_ENABLE)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L1PARITY_ENABLE;
	        nseconf_value |= (value & IDT75K_NSECONF_L1PARITY_ENABLE);
	}
	if( mask & IDT75K_NSECONF_L0PARITY_ENABLE)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L0PARITY_ENABLE;
	        nseconf_value |= (value & IDT75K_NSECONF_L0PARITY_ENABLE);
	}
	if( mask & IDT75K_NSECONF_SDML_REPLACES_MHL)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_SDML_REPLACES_MHL;
	        nseconf_value |= (value & IDT75K_NSECONF_SDML_REPLACES_MHL);
	}
	if( mask & IDT75K_NSECONF_L1_DATA18_SEL)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L1_DATA18_SEL;
	        nseconf_value |= (value & IDT75K_NSECONF_L1_DATA18_SEL);
	}
	if( mask & IDT75K_NSECONF_L0_DATA18_SEL)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L0_DATA18_SEL;
	        nseconf_value |= (value & IDT75K_NSECONF_L0_DATA18_SEL);
	}
	if( mask & IDT75K_NSECONF_L1_WORD_ORDER)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L1_WORD_ORDER;
	        nseconf_value |= (value & IDT75K_NSECONF_L1_WORD_ORDER);
	}
	if( mask & IDT75K_NSECONF_L1_BYTE_ORDER)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L1_BYTE_ORDER;
	        nseconf_value |= (value & IDT75K_NSECONF_L1_BYTE_ORDER);
	}
	if( mask & IDT75K_NSECONF_L1_ENABLE)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L1_ENABLE;
	        nseconf_value |= (value & IDT75K_NSECONF_L1_ENABLE);
	}
	if( mask & IDT75K_NSECONF_CD_COMPLETE)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_CD_COMPLETE;
	        nseconf_value |= (value & IDT75K_NSECONF_CD_COMPLETE);
	}
	if( mask & IDT75K_NSECONF_L0_WORD_ORDER)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L0_WORD_ORDER;
	        nseconf_value |= (value & IDT75K_NSECONF_L0_WORD_ORDER);
	}
	if( mask & IDT75K_NSECONF_L0_BYTE_ORDER)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L0_BYTE_ORDER;
	        nseconf_value |= (value & IDT75K_NSECONF_L0_BYTE_ORDER);
	}
	if( mask & IDT75K_NSECONF_ADSP)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_ADSP;
	        nseconf_value |= (value & IDT75K_NSECONF_ADSP);
	}
	if( mask & IDT75K_NSECONF_L0_BYTE_ORDER)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_L0_BYTE_ORDER;
	        nseconf_value |= (value & IDT75K_NSECONF_L0_BYTE_ORDER);
	}
	if( mask & IDT75K_NSECONF_ADSP)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_ADSP;
	        nseconf_value |= (value & IDT75K_NSECONF_ADSP);
	}
	if( mask & IDT75K_NSECONF_DPSIZE)
	{	
	 	nseconf_value &= ~IDT75K_NSECONF_DPSIZE;
	        nseconf_value |= (value & IDT75K_NSECONF_DPSIZE);
	}

	/* Write all the data back to the device */
	if ( (err = idt75k_write_csr_ctx(dev_p,context ,nseconf_address, nseconf_value)) != ESUCCESS )
		return err;

	return ESUCCESS;
}
int
idt75k_get_nseconf(idt75k_dev_t *dev_p, uint32_t *value_p)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_get_nseconf_ctx(dev_p,context,value_p);
	return return_value;
}
//changed the interface to pass context as a parameter.Needed for full production testing.
int
idt75k_get_nseconf_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t *value_p)
{
	uint32_t nseconf_addr, nseconf_value;
	int err;
	
	if(dev_p == NULL)
		return EINVAL;

	nseconf_addr = 0x0000;

	if ((err = idt75k_read_csr_ctx(dev_p,context,nseconf_addr, &nseconf_value)) != ESUCCESS )
		return err;

	if (value_p != NULL)
		*value_p = nseconf_value;

	return ESUCCESS;
}
int 
idt75k_get_nseident1(idt75k_dev_t *dev_p, uint32_t *value_p)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_get_nseident1_ctx(dev_p,context,value_p);
	return return_value;
}
//changed the interface to pass context as a parameter.Needed for full production testing.
int 
idt75k_get_nseident1_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t *value_p)
{
	 uint32_t nseident1_addr, nseident1_value;
	 int err;

	if(dev_p == NULL)
		return EINVAL;
		
	nseident1_addr = 0x0001;

	if ((err = idt75k_read_csr_ctx(dev_p,context,nseident1_addr, &nseident1_value)) != ESUCCESS )
		return err;
		
	if (value_p != NULL)
		*value_p = nseident1_value;

	return ESUCCESS;
}
//to keep the original interface
int 
idt75k_get_nseident2(idt75k_dev_t *dev_p, uint32_t *value_p)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_get_nseident2_ctx(dev_p,context,value_p);
	return return_value;
}
//changed the interface to pass context as a paramter.Needed for full production testing.
int 
idt75k_get_nseident2_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t *value_p)
{
	 uint32_t nseident2_addr, nseident2_value;
	 int err;

	if(dev_p == NULL)
		return EINVAL;
		
	nseident2_addr = 0x0001;

	if ((err = idt75k_read_csr_ctx(dev_p,context,nseident2_addr, &nseident2_value)) != ESUCCESS )
		return err;
		
	if (value_p != NULL)
		*value_p = nseident2_value;

	return ESUCCESS;
}
/**
 * Reads the configuration registers for a database in the device. This
 * function reads the DB_CONF1, DB_CONF2 and DD_CONF3 for the selected
 * database number.
 *
 * @param[in]  dev_p      The driver defined structure
 * @param[in]  db_num     The database number to configure, valid values are
 *                        0 to 15.
 * @param[out] flags_p    Pointer to a 32-bit value that will receive the
 *                        flags set in the configuration, this is just
 *                        the contents of the DB_CONF1 register.
 * @param[out] db_size_p  Receives the size of the DB entries (in bits).
 * @param[out] age_cnt_p  The age count for the database.
 * @param[out] segments_p Receives the segments assoicated with this DB.
 *
 * @return                Returns one of the following error codes.
 * @retval ESUCCESS       The operation was successiful.
 * @retval EINVAL         An invalid argument was supplied
 * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *                        procedure was complete.
 * @retval EFAULT         The error bit was set by the chip in response to
 *                        the read command.
 */
//to keep the original interface
int
idt75k_get_dbconf(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t *flags_p, uint32_t *age_cnt_p, uint32_t *segments_p)
{
	
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_get_dbconf_ctx(dev_p,context,db_num,flags_p,age_cnt_p,segments_p);
	return return_value;
}
//changed the interface to include context as a paramter.needed for full production testing.
int
idt75k_get_dbconf_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t *flags_p, uint32_t *age_cnt_p, uint32_t *segments_p)
{
	uint32_t  dbconf1_addr, dbconf1_value;
	uint32_t  dbconf2_addr, dbconf2_value;
	uint32_t  dbconf3_addr, dbconf3_value;
	int       err;

#ifndef _WIN32
	DD ( "idt75k: idt75k_get_dbconf(0x%p, %d, --, --, --, --)\n", dev_p, db_num) ;
#endif

	/* Sanity check the input */
	if ( db_num > 15 )
		return EINVAL;
	if ( dev_p == NULL )
		return EINVAL;
	
	dbconf1_addr = 0x0100 | (db_num << 4);
	dbconf2_addr = 0x0101 | (db_num << 4);
	dbconf3_addr = 0x0102 | (db_num << 4);
	
	
	/* Read the current values from the registers */
	if ( (err = idt75k_read_csr_ctx(dev_p,context,dbconf1_addr, &dbconf1_value)) != ESUCCESS )
		return err;
	if ( (err = idt75k_read_csr_ctx(dev_p,context,dbconf2_addr, &dbconf2_value)) != ESUCCESS )
		return err;
	if ( (err = idt75k_read_csr_ctx(dev_p,context,dbconf3_addr, &dbconf3_value)) != ESUCCESS )
		return err;


	if ( flags_p != NULL )
		*flags_p = dbconf1_value;

	if ( age_cnt_p != NULL )
		*age_cnt_p = dbconf2_value & 0x00FFFFFF;

	if ( segments_p != NULL )
		*segments_p = dbconf3_value;

	return ESUCCESS;
}
/**
 * Reads an entry from the TCAM at the given address, note that addresses
 * are indexed in 36-bit blocks. The mask address is different from the
 * data address, however this functions writes both entries.
 *
 * @param[in]  dev_p      The driver defined structure
 * @param[in]  addr       The address (index) of the TCAM entry to read.
 * @param[out] data       Pointer to an array of bytes that gets populated
 *                        with the data values of the entry.
 * @param[out] mask       Pointer to an array of bytes that gets populated
 *                        with the mask values of the entry.
 * @param[out] valid_p    Pointer to a value that upon return will contain
 *                        a non-zero value if the entry is marked as valid
 *                        otherwise zero.
 *
 * @return                Returns one of the following error codes.
 * @retval ESUCCESS       The operation was successiful.
 * @retval EINVAL         An invalid argument was supplied
 * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *                        procedure was complete.
 * @retval EFAULT         The error bit was set by the chip in response to
 *                        the read command.
 */
//to keep the original interface.
int
idt75k_read_cam_entry(idt75k_dev_t *dev_p, uint32_t addr, uint8_t data[9], uint8_t mask[9], uint32_t *valid_p)
{
	uint32_t context = 127;
	int return_value = 0;	
	return_value = idt75k_read_cam_entry_ctx(dev_p,context,addr,data,mask,valid_p);
	return return_value;
}
//need to change the interface to pass context as a paramter.needed for full procuction testing.
int
idt75k_read_cam_entry_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t addr, uint8_t data[9], uint8_t mask[9], uint32_t *valid_p)
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[4];

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_cam_entry(0x%p, 0x%08X, --, --)\n", dev_p, addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x0007FFFF )
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	if ( valid_p == NULL )
		return EINVAL;

	/* Read the core data */

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,0,0);
	wr_words[1] = (SUBINSTR_READ << 28) | (IDT75K_REGION_CORE_DATA << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif

	memcpy_la1(dev_p,wr_words,2);

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif

	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if th24e error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;


	/* Read the second result word in the mailbox and asign it to the value */
	#if 0
		rd_words[1] = IDT75K_READ_MAILBOX(dev_p, 0, 1);
		rd_words[2] = IDT75K_READ_MAILBOX(dev_p, 0, 2);
		rd_words[3] = IDT75K_READ_MAILBOX(dev_p, 0, 3);
	#endif

	rd_words[1] = read_mailbox(dev_p,context,1);
	rd_words[2] = read_mailbox(dev_p,context,2);
	rd_words[3] = read_mailbox(dev_p,context,3);	
	
	/* Convert the data to the array specified */
	data[0] = ((rd_words[1] >> 24) & 0xFF);
	data[1] = ((rd_words[1] >> 16) & 0xFF);
	data[2] = ((rd_words[1] >>  8) & 0xFF);
	data[3] = ((rd_words[1] >>  0) & 0xFF);
	data[4] = ((rd_words[2] >> 24) & 0xFF);
	data[5] = ((rd_words[2] >> 16) & 0xFF);
	data[6] = ((rd_words[2] >>  8) & 0xFF);
	data[7] = ((rd_words[2] >>  0) & 0xFF);
	data[8] = ((rd_words[3] >> 24) & 0xFF);

	*valid_p = (rd_words[3] & (1 << 21)) ? 1 : 0;

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_cam_entry: data %08X_%08X_%08X\n", rd_words[1], rd_words[2], rd_words[3]) ;
#endif

	/* Read the core mask */
	
	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context, INSTR_INDIRECT, 0, 0, 0);
	wr_words[1] = (SUBINSTR_READ << 28) | (IDT75K_REGION_CORE_MASK << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif
	memcpy_la1(dev_p,wr_words,2);


	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif

	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;


	/* Read the second result word in the mailbox and asign it to the value */
	#if 0
		rd_words[1] = IDT75K_READ_MAILBOX(dev_p, 0, 1);
		rd_words[2] = IDT75K_READ_MAILBOX(dev_p, 0, 2);
		rd_words[3] = IDT75K_READ_MAILBOX(dev_p, 0, 3);
	#endif
	
	rd_words[1] = read_mailbox(dev_p,context,1);
	rd_words[2] = read_mailbox(dev_p,context,2);
	rd_words[3] = read_mailbox(dev_p,context,3);
	


	/* Convert the data to the array specified */
	mask[0] = ((rd_words[1] >> 24) & 0xFF);
	mask[1] = ((rd_words[1] >> 16) & 0xFF);
	mask[2] = ((rd_words[1] >>  8) & 0xFF);
	mask[3] = ((rd_words[1] >>  0) & 0xFF);
	mask[4] = ((rd_words[2] >> 24) & 0xFF);
	mask[5] = ((rd_words[2] >> 16) & 0xFF);
	mask[6] = ((rd_words[2] >>  8) & 0xFF);
	mask[7] = ((rd_words[2] >>  0) & 0xFF);
	mask[8] = ((rd_words[3] >> 24) & 0xFF);

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_cam_entry: mask %08X_%08X_%08X\n", rd_words[1], rd_words[2], rd_words[3]) ;
#endif
	return ESUCCESS;
}

/**
 * Writes an entry to the TCAM at the given address, note that addresses
 * are indexed in 36-bit blocks. The mask address is different from the
 * data address, however this functions writes both entries.
 *
 * @param[in]  dev_p      Pointer to the driver defined structure.
 * @param[in]  db_num     The number of the database performing the write to,
 *                        this is needed by the TCAM to determine the size
 *                        (number of bits) of the write operation.
 * @param[in]  addr       The address (index) of the TCAM entry to read.
 * @param[in]  data       Pointer to an array of bytes that gets loaded
 *                        into the data part of the entry.
 * @param[in]  mask       Pointer to an array of bytes that gets loaded
 *                        into the mask part of the entry.
 * @param[in]  validate   Sets the valid bits once the write is complete.
 * @param[in]  gmask      Indicates which global mask to apply to the key
 *                        during writing.
 *
 * @return                Returns one of the following error codes.
 * @retval ESUCCESS       The operation was successiful.
 * @retval EINVAL         An invalid argument was supplied
 * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *                        procedure was complete.
 * @retval EFAULT         The error bit was set by the chip in response to
 *                        the read command.
 */
int
idt75k_write_cam_entry(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t addr, uint8_t data[72], uint8_t mask[72], uint32_t gmask, uint32_t validate)
{
	int return_value = 0;
	uint32_t context = 127;	
	return_value = idt75k_write_cam_entry_ctx(dev_p,context,db_num,addr,data,mask,gmask,validate);
	return return_value;	
}
//changed the interface to include context as a param.needed for the full production testing.
int
idt75k_write_cam_entry_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, uint8_t data[72], uint8_t mask[72], uint32_t gmask, uint32_t validate)
{
	uint32_t      wr_words[20];
	uint32_t      rd_words[1];
	uint32_t      sub_instr;
	uint32_t      db_conf;
	uint32_t      db_core_size;
	uint32_t      db_bytes;
	uint32_t      db_words;
	int           err;
	uint32_t      i, j;

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_cam_entry(--, %d, 0x%08X, --, --)\n", db_num, addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x07FFFF )
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	if ( db_num > 15 )
		return EINVAL;
	if ( gmask > 31 )
		return EINVAL;

#if 0
	printf("Inside Write Cam entry \n");
#endif 
	/* Read the database configuration, in particular the size of the core size */
	if ( (err = idt75k_read_csr_ctx(dev_p,context,(0x100 | (db_num << 4)), &db_conf)) != ESUCCESS )
	{
		printf("Error : %d\n",err);
		return err;
	}
	db_core_size = (db_conf >> 5) & 0x1F;
	switch (db_core_size)
	{
		case 0x00:              /* reserved */
			return EINVAL;

		case 0x01:              /* 36-bit wide entries */
			db_bytes = 5;
			break;

		case 0x02:              /* 72-bit wide entries */
			db_bytes = 9;
			break;

		case 0x04:              /* 144-bit wide entries */
			db_bytes = 18;
			break;

		case 0x08:              /* 288-bit wide entries */
			db_bytes = 36;
			break;

		case 0x10:              /* 576-bit wide entries */
			db_bytes = 72;
			break;

		default:                /* invalid */
			return EINVAL;
	}

	/* Calculate the number of 32-bit words that need to transfered (excluding the instruction and sub-instruction words) */
	db_words = ((db_bytes + 3) >> 2);

	/* Write the core mask */
	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	
	#if 0	
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, (db_words + 1), INSTR_INDIRECT, gmask, db_num, 0);
	#endif
	/*need to check once again */
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context, INSTR_INDIRECT, gmask, db_num, 0);	

	wr_words[1] = (SUBINSTR_WRITE << 28) | (IDT75K_REGION_CORE_MASK << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);
	
	for (i=0, j=0; i<db_words; i++, j+=4)
		wr_words[i+2] = ((uint32_t)mask[j] << 24) | ((uint32_t)mask[j+1] << 16) | ((uint32_t)mask[j+2] << 8) | ((uint32_t)mask[j+3]);

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_cam_entry: mask [%d] %08X_%08X_%08X_%08X_%08X\n", db_words, wr_words[2], wr_words[3], wr_words[4], wr_words[5], wr_words[6]) ;
#endif

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * (db_words+2)));
	#endif
	
	memcpy_la1 ( dev_p, wr_words, (db_words + 2));



	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;


	/* Write the core data */

	/* determine the type of instruction to use for the read */
	if ( validate == 0 )
		sub_instr = SUBINSTR_WRITE_KEEP_VALID;
	else
		sub_instr = SUBINSTR_WRITE;


	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, (db_words + 1), INSTR_INDIRECT, gmask, db_num, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,gmask,db_num,(db_words + 1));	
	wr_words[1] = (sub_instr << 28) | (IDT75K_REGION_CORE_DATA << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);
	
	for (i=0, j=0; i<db_words; i++, j+=4)
		wr_words[i+2] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j+1] << 16) | ((uint32_t)data[j+2] << 8) | ((uint32_t)data[j+3]);

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_cam_entry: data [%d] %08X_%08X_%08X_%08X_%08X\n", db_words, wr_words[2], wr_words[3], wr_words[4], wr_words[5], wr_words[6]) ;
#endif

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * (db_words+2)));
	#endif
	
	memcpy_la1(dev_p,wr_words,(db_words +2));


	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	return ESUCCESS;
}

/**
 * Writes an entry to the TCAM at the given address like idt75k_write_cam_entry,
 * but also writes the assoicated data.
 *
 * @param[in]  dev_p      The driver defined structure
 * @param[in]  db_num     The number of the database performing the write to,
 *                        this is needed by the TCAM to determine the size
 *                        (number of bits) of the write operation.
 * @param[in]  addr       The address (index) of the TCAM entry to read.
 * @param[in]  data       Pointer to an array of bytes that gets loaded
 *                        into the data part of the entry.
 * @param[in]  mask       Pointer to an array of bytes that gets loaded
 *                        into the mask part of the entry.
 * @param[in]  gmask      Indicates which global mask to apply to the key
 *                        during writing.
 * @param[in]  ad_value   The value(s) to write into the assoicated data
 *
 * @return                Returns one of the following error codes.
 * @retval ESUCCESS       The operation was successiful.
 * @retval EINVAL         An invalid argument was supplied
 * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *                        procedure was complete.
 * @retval EFAULT         The error bit was set by the chip in response to
 *                        the read command.
 */
int
idt75k_write_cam_entry_ex(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t addr, const uint8_t data[72], const uint8_t mask[72], uint32_t gmask, const uint32_t ad_value[4])
{
	uint32_t context = 127;
	int return_value;
	return_value = idt75k_write_cam_entry_ex_ctx(dev_p,context,db_num,addr,data,mask,gmask,ad_value);
	return return_value;
}
int
idt75k_write_cam_entry_ex_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t db_num, uint32_t addr, const uint8_t data[72], const uint8_t mask[72], uint32_t gmask, const uint32_t ad_value[4])
{
	uint32_t      wr_words[24];
	uint32_t      rd_words[1];
	uint32_t      db_conf;
	uint32_t      db_core_size;
	uint32_t      db_bytes;
	uint32_t      db_words;
	uint32_t      db_ad_size;
	uint32_t      ad_words = 0;
	int           err;
	uint32_t      i, j;

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_cam_entry_ex(--, %d, 0x%08X, --, --, --)\n", db_num, addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x07FFFF )
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	if ( db_num > 15 )
		return EINVAL;
	if ( gmask > 31 )
		return EINVAL;


	/* Read the database configuration, in particular the size of the core size */
	if ( (err = idt75k_read_csr_ctx(dev_p,context,(0x100 | (db_num << 4)), &db_conf)) != ESUCCESS )
	{
#ifndef _WIN32
		DA(  "idt75k: idt75k_read_csr failed in idt75k_write_cam_entry_ex with %d error code\n", err) ;
#endif
		return err;
	}

	db_core_size = (db_conf >> 5) & 0x1F;
	switch (db_core_size)
	{
		case 0x00:              /* reserved */
#ifndef _WIN32
			DA(  "idt75k: idt75k_write_cam_entry_ex: db_core_size invalid (0x%08X)\n", db_conf) ;
#endif
			return EINVAL;

		case 0x01:              /* 36-bit wide entries */
			db_bytes = 5;
			break;

		case 0x02:              /* 72-bit wide entries */
			db_bytes = 9;
			break;

		case 0x04:              /* 144-bit wide entries */
			db_bytes = 18;
			break;

		case 0x08:              /* 288-bit wide entries */
			db_bytes = 36;
			break;

		case 0x10:              /* 576-bit wide entries */
			db_bytes = 72;
			break;

		default:                /* invalid */
#ifndef _WIN32
			DA ( "idt75k: idt75k_write_cam_entry_ex: db_core_size invalid (0x%08X)\n", db_conf) ;
#endif
			return EINVAL;
	}

	/* Calculate the number of 32-bit words that need to transfered (excluding the instruction and sub-instruction words) */
	db_words = ((db_bytes + 3) >> 2);


	/* Calculate the size of the associated data */
	db_ad_size = (db_conf >> 24) & 0x03;
	switch (db_ad_size)
	{
		case 0x3:              /* no associated data */
			ad_words = 0;
			break;
		case 0x0:              /* 32-bits of associated data */
			ad_words = 1;
			break;
		case 0x1:              /* 64-bits of associated data */
			ad_words = 2;
			break;
		case 0x2:              /* 128-bits of associated data */
			ad_words = 4;
			break;
	}




	/* Write the core mask */

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, (db_words + 1), INSTR_INDIRECT, gmask, db_num, 0);
	#endif
	
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,gmask,db_num,0);
	wr_words[1] = (SUBINSTR_WRITE << 28) | (IDT75K_REGION_CORE_MASK << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);
	
	for (i=0, j=0; i<db_words; i++, j+=4)
		wr_words[i+2] = ((uint32_t)mask[j] << 24) | ((uint32_t)mask[j+1] << 16) | ((uint32_t)mask[j+2] << 8) | ((uint32_t)mask[j+3]);

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_cam_entry_ex: mask [%d] %08X_%08X_%08X_%08X_%08X\n", db_words, wr_words[2], wr_words[3], wr_words[4], wr_words[5], wr_words[6] );
#endif

	/* write the data to the chip */
	memcpy_la1 (dev_p, wr_words, (sizeof(uint32_t) * (db_words+2)));


	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif

	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	/* Write the core data and assoicated data */

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, (db_words + ad_words + 1), INSTR_INDIRECT, gmask, db_num, 0);
	#endif	
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,gmask,db_num,0);
	wr_words[1] = (SUBINSTR_DUAL_WRITE << 28) | (IDT75K_REGION_CORE_DATA << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);

	for (i=0, j=0; i<db_words; i++, j+=4)
		wr_words[i+2] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j+1] << 16) | ((uint32_t)data[j+2] << 8) | ((uint32_t)data[j+3]);

	for (i=0; i<ad_words; i++)
		wr_words[i+db_words+2] = ad_value[i];

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_cam_entry_ex: data [%d] %08X_%08X_%08X_%08X_%08X\n", db_words, wr_words[2], wr_words[3], wr_words[4], wr_words[5], wr_words[6]) ;
#endif

	/* write the data to the chip */
	memcpy_la1 (dev_p, wr_words, (sizeof(uint32_t) * (db_words + ad_words + 2)));



	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	return ESUCCESS;
}
/**
 * Sets the valid state of an entry in the TCAM
 *
 * @param[in]  dev_p    The PCI device structure for the driver
 * @param[in]  db_num   The number of the database performing the write to,
 *                      this is needed by the TCAM to determine the size
 *                      (number of bits) of the write operation.
 * @param[in]  addr     The 19-bit address of the CSR to read, possible
 *                      addresses are listed oin Table5.2 of the datasheet.
 * @param[in]  valid    The new valid state to set for a TCAM entry, a
 *                      non-zero value sets the item valid and a zero value
 *                      sets the entry invalid.
 *
 * @return              Returns one of the following error codes.
 * @retval ESUCCESS     The operation was successiful and res contains
 *                      the value read from the chip.
 * @retval EINVAL       An invalid argument was supplied
 * @retval ETIMEDOUT    Timedout waiting for the chip to indicate the reading
 *                      procedure was complete.
 * @retval EFAULT       The error bit was set by the chip in response to
 *                      the read command.
 */
//changing the interface to pass context as a parameter.needed for full production testing.
int
id75k_set_cam_entry_state(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t addr, uint32_t valid)
{	
	int return_value = 0;
	uint32_t context = 127;
	return_value = id75k_set_cam_entry_state_ctx(dev_p,context,db_num,addr,valid);
	return return_value;
}
int
id75k_set_cam_entry_state_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, uint32_t valid)
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[1];
	uint32_t      sub_instr;

#ifndef _WIN32
	DD ( "idt75k: id75k_set_cam_entry_state(--, %d, 0x%08X, %d)\n", db_num, addr, valid) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x07FFFF )
		return EINVAL;
	if ( db_num > 15 )
		return EINVAL;


	/* Write the core mask */

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, db_num, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,db_num,0);
	if ( valid == 0 )
		sub_instr = SUBINSTR_CLEAR_VALID;
	else
		sub_instr = SUBINSTR_SET_VALID;
	wr_words[1] = (sub_instr << 28) | (dev_p->device_id << 19) | (addr & 0x7FFFF);


	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif
	memcpy_la1(dev_p,wr_words,2);

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if we timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	return ESUCCESS;
}
/**
 * Reads one of the standard configuration registers. For a list
 * of available configuration registers see the idt75k_csr_t enum.
 *
 * All reads return a 32-bit value regardless of the size of the
 * CSR.
 *
 * @param[in]  dev_p    The PCI device structure for the driver
 * @param[in]  db_num   The number of the database performing the read,
 *                      this is needed by the TCAM to determine the size
 *                      (number of bits) of the associated data.
 * @param[in]  addr     The 19-bit address of the CSR to read, possible
 *                      addresses are listed oin Table5.2 of the datasheet.
 * @param[out] value_p  Upon success this variable will contain the value
 *                      read from the CSR.
 *
 * @return              Returns one of the following error codes.
 * @retval ESUCCESS     The operation was successiful and res contains
 *                      the value read from the chip.
 * @retval EINVAL       An invalid argument was supplied
 * @retval ETIMEDOUT    Timedout waiting for the chip to indicate the reading
 *                      procedure was complete.
 * @retval EFAULT       The error bit was set by the chip in response to
 *                      the read command.
 */
int
idt75k_read_sram(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t addr, uint32_t sram_entry[4])
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_read_sram_ctx(dev_p,context,db_num,addr,sram_entry);
	return return_value;
}
//changed the interface to include context as a paramter.needed for full production testing.
int
idt75k_read_sram_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t db_num, uint32_t addr, uint32_t sram_entry[4])
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[5];
	uint32_t      db_ad_size;
	uint32_t      db_conf;
	uint32_t      ad_words = 0;
	int           err;
	uint32_t      i;

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_sram(--, %d, 0x%08X, --)\n", db_num, addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x7FFFF )
		return EINVAL;
	if ( sram_entry == NULL )
		return EINVAL;
	if ( db_num > 15 )
		return EINVAL;


	/* Read the database configuration, in particular the size of the associated data */
	if ( (err = idt75k_read_csr_ctx(dev_p,context,(0x100 | (db_num << 4)), &db_conf)) != ESUCCESS )
		return err;

	/* Calculate the size of the associated data */
	db_ad_size = (db_conf >> 24) & 0x03;
	switch (db_ad_size)
	{
		case 0x3:              /* no associated data */
			return EINVAL;
		case 0x0:              /* 32-bits of associated data */
			ad_words = 1;
			break;
		case 0x1:              /* 64-bits of associated data */
			ad_words = 2;
			break;
		case 0x2:              /* 128-bits of associated data */
			ad_words = 4;
			break;
	}



	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, db_num, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,db_num,0);
	wr_words[1] = (SUBINSTR_READ << 28) | (IDT75K_REGION_SRAM << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif
	
	memcpy_la1(dev_p,wr_words,2);



	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif	
	
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	/* Read the results from the mailbox */
	for (i=1; i<5; i++)
	{
		#if 0
			rd_words[i] = IDT75K_READ_MAILBOX(dev_p, 0, i);
		#endif
		rd_words[i] = read_mailbox(dev_p,context,i);
	}

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_sram: %d %08X %08X %08X %08X %08X\n", ad_words, rd_words[0], rd_words[1], rd_words[2], rd_words[3], rd_words[4]) ;
#endif

	for (i=0; i<ad_words; i++)
		sram_entry[i] = rd_words[1 + i];

	return ESUCCESS;
}
/**
 * Writes a 32, 64 or 128 bit value into the associated data of a TCAM entry,
 *
 * @param[in]  dev_p       The PCI device structure for the driver
 * @param[in]  db_num      The number of the database performing the read,
 *                         this is needed by the TCAM to determine the size
 *                         (number of bits) of the associated data.
 * @param[in]  addr        The 19-bit address of the NSE SRAM to write, note
 *                         that the addresses are mapped by the device to
 *                         TCAM entries, they are not the SRAM bytes addresses.
 * @param[in]  sram_entry  The data to write into the entry.
 * @param[in]  sram_size   The size of the entry, possible values are 32, 64 or 128.
 *
 * @return                 Returns one of the following error codes.
 * @retval ESUCCESS        The operation was successiful.
 * @retval EINVAL          An invalid argument was supplied.
 * @retval ETIMEDOUT       Timedout waiting for the chip to indicate the write
 *                         procedure was complete.
 * @retval EFAULT          The error bit was set by the chip in response to
 *                         the write command.
 */
int
idt75k_write_sram(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t addr, const uint32_t sram_entry[4])
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_write_sram_ctx(dev_p,context,db_num,addr,sram_entry);
	return return_value;
}
//changed the interface to pass context as a paramter.needed for full production testing.
int
idt75k_write_sram_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, const uint32_t sram_entry[4])
{
	uint32_t      wr_words[6];
	uint32_t      rd_words[1];
	uint32_t      i;
	uint32_t      db_ad_size;
	uint32_t      db_conf;
	uint32_t      ad_words = 0;
	int           err;

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_sram(--, %d, 0x%08X, --)\n", db_num, addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x7FFFF )
		return EINVAL;
	if ( sram_entry == NULL )
		return EINVAL;
	if ( db_num > 15 )
		return EINVAL;


	/* Read the database configuration, in particular the size of the associated data */
	if ( (err = idt75k_read_csr_ctx(dev_p,context,(0x100 | (db_num << 4)), &db_conf)) != ESUCCESS )
		return err;

	/* Calculate the size of the associated data */
	db_ad_size = (db_conf >> 24) & 0x03;
	switch (db_ad_size)
	{
		case 0x3:              /* no associated data */
			return EINVAL;
		case 0x0:              /* 32-bits of associated data */
			ad_words = 1;
			break;
		case 0x1:              /* 64-bits of associated data */
			ad_words = 2;
			break;
		case 0x2:              /* 128-bits of associated data */
			ad_words = 4;
			break;
	}

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0	
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, (ad_words + 1), INSTR_INDIRECT, 0, db_num, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,db_num,0);
	wr_words[1] = (SUBINSTR_WRITE << 28) | (IDT75K_REGION_SRAM << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);

	/* write the data to the chip */
	for (i=0; i<ad_words; i++)
		wr_words[i+2] = sram_entry[i];

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_sram: %d %08X %08X %08X %08X\n", ad_words, wr_words[0], wr_words[1], wr_words[2], wr_words[3]) ;
#endif

	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * (2 + ad_words)));
	#endif
	
	memcpy_la1(dev_p,wr_words,(2 + ad_words));

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;


	/* Success */
	return ESUCCESS;
}
/**
 * Copies the contents of one SRAM location to another. Only a single
 * call is needed regardless of the NSE attached associated data width.
 *
 * @param[in]  dev_p       The PCI device structure for the driver
 * @param[in]  db_num      The number of the database performing the read,
 *                         this is needed by the TCAM to determine the size
 *                         (number of bits) of the associated data.
 * @param[in]  dst_addr    The destination address to copy the new SRAM to.
 * @param[in]  src_addr    The source address to copy the SRAM from.
 *
 * @return                 Returns one of the following error codes.
 * @retval ESUCCESS        The operation was successiful.
 * @retval EINVAL          An invalid argument was supplied.
 * @retval ETIMEDOUT       Timedout waiting for the chip to indicate the write
 *                         procedure was complete.
 */
int
idt75k_copy_sram(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t dst_addr, uint32_t src_addr)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_copy_sram_ctx(dev_p,context,db_num,dst_addr,src_addr);
	return return_value;
}
//changing the interface to pass context as a parameter.needed for full production testing.
int
idt75k_copy_sram_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t dst_addr, uint32_t src_addr)
{
	uint32_t      wr_words[3];
	uint32_t      rd_words[1];

#ifndef _WIN32
	DD ( "idt75k: idt75k_copy_sram(--, %d, 0x%08X, 0x%08X)\n", db_num, dst_addr, src_addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( db_num > 15 )
		return EINVAL;
	if ( dst_addr > 0x7FFFF )
		return EINVAL;
	if ( src_addr > 0x7FFFF )
		return EINVAL;

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 2, INSTR_INDIRECT, 0, db_num, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,db_num,0);
	wr_words[1] = (SUBINSTR_SRAM_COPY << 28) | (IDT75K_REGION_SRAM << 23) | (src_addr & 0x7FFFF);
	wr_words[2] =                              (IDT75K_REGION_SRAM << 23) | (dst_addr & 0x7FFFF);

	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 3));
	#endif
	memcpy_la1(dev_p,wr_words,3);

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Success */
	return ESUCCESS;
}
/**
 * Reads a 32-bit SMT entry from the device.
 *
 * @param[in]  dev_p       The PCI device structure for the driver
 * @param[in]  addr        The address of the SMT entry to read, for non-cascaded configs
 *                         this value should be between 0 and 31.
 * @param[out] smt_entry_p Pointer to a 32-bit value that upon return will contain the
 *                         entry read from the device.
 *
 * @return                 Returns one of the following error codes.
 * @retval ESUCCESS        The operation was successiful.
 * @retval EINVAL          An invalid argument was supplied.
 * @retval ETIMEDOUT       Timedout waiting for the chip to indicate the write
 *                         procedure was complete.
 * @retval EFAULT          The error bit was set by the chip in response to
 *                         the write command.
 */
//to keep the old interface
int
idt75k_read_smt(idt75k_dev_t *dev_p, uint32_t addr, uint32_t *smt_entry_p)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_read_smt_ctx(dev_p,context,addr,smt_entry_p);
	return return_value;
}
//changed the interface to pass context as a parameter.Needed for full production testing.
int
idt75k_read_smt_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t addr, uint32_t *smt_entry_p)
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[2];

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_smt(--, 0x%08X, --)\n", addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 31 )
		return EINVAL;
	if ( smt_entry_p == NULL )
		return EINVAL;

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,0,0);
	wr_words[1] = (SUBINSTR_READ << 28) | (IDT75K_REGION_CSR << 23) | (dev_p->device_id << 19) | (0x2 << 16) | (addr & 0xFF);

	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif
	
	memcpy_la1(dev_p, wr_words,2);

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	/* Read the result */
	#if 0
		*smt_entry_p = IDT75K_READ_MAILBOX(dev_p, 0, 1);
	#endif
	*smt_entry_p = read_mailbox(dev_p,context,1);

	/* Success */
	return ESUCCESS;
}
/**
 * Writes a 32-bit SMT entry to the device.
 *
 * @param[in]  dev_p       The PCI device structure for the driver
 * @param[in]  addr        The address of the SMT entry to write, for non-cascaded configs
 *                         this value should be between 0 and 31.
 * @param[in] smt_entry    The 32-bit value to write to the entry.
 *
 * @return                 Returns one of the following error codes.
 * @retval ESUCCESS        The operation was successiful.
 * @retval EINVAL          An invalid argument was supplied.
 * @retval ETIMEDOUT       Timedout waiting for the chip to indicate the write
 *                         procedure was complete.
 * @retval EFAULT          The error bit was set by the chip in response to
 *                         the write command.
 */
//to keep the old inteface
int
idt75k_write_smt(idt75k_dev_t *dev_p, uint32_t addr, const uint32_t smt_entry)
{
        int return_value = 0;
        uint32_t context = 127;
        return_value = idt75k_write_smt_ctx(dev_p,context,addr,smt_entry);
        return return_value;
}
//changed the interface to include context as a parameter.used for full production testing.
int
idt75k_write_smt_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t addr, const uint32_t smt_entry)
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[2];

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_smt(--, 0x%08X, 0x%08X)\n", addr, smt_entry) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 31 )
		return EINVAL;

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 2, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,0,0);
	wr_words[1] = (SUBINSTR_WRITE << 28) | (IDT75K_REGION_CSR << 23) | (dev_p->device_id << 19) | (0x2 << 16) | (addr & 0xFF);
	wr_words[2] = smt_entry;
	
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 3));
	#endif
	
	memcpy_la1(dev_p,wr_words,3);

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	/* Success */
	return ESUCCESS;
}
/**
 * Performs a lookup operation on the device, this function returns the
 * address of the entry found and the value stored in the associated SRAM
 * (if present). A single lookup only is performed in a single database.
 *
 * @param[in]  dev_p        The driver defined structure
 * @param[in]  db_num       The database number to configure, valid values are
 *                          0 to 15.
 * @param[in]  lookup_p     Pointer to a 32-bit value array that contains the
 *                          value to lookup.
 * @param[in]  lookup_size  The size of the lookup in bits, this should match
 *                          the database size. Should be one of the following
 *                          values; 36, 72, 144, 288, 576.
 * @param[in]  gmr          The number of Global Mask Register (GMR) to use in
 *                          the lookup.
 * @param[out] hit_p        Will contain a non-zero value if the lookup value
 *                          hit on a TCAM entry, if a miss the value will be
 *                          set to zero.
 * @param[out] index_p      Receives the size of the DB entries (in bits).
 * @param[out] ad_p         Pointer to a 32-bit array that receives the
 *                          associated data on a hit.
 *
 * @return                  Returns one of the following error codes.
 * @retval ESUCCESS         The operation was successiful (this is the case even if
 *                          the TCAM didn't hit on any entries).
 * @retval EINVAL           An invalid argument was supplied
 * @retval ETIMEDOUT        Timedout waiting for the chip to indicate the lookup
 *                          procedure was complete.
 * @retval EFAULT           The error bit was set by the chip in response to
 *                          the lookup command.
 */
//to keep the old interface.
int
idt75k_lookup(idt75k_dev_t *dev_p, uint32_t db_num, uint32_t *lookup_p, uint32_t lookup_size, uint32_t gmr, uint32_t *hit_p, uint32_t *index_p, uint32_t *ad_p)
{	
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_lookup_ctx(dev_p,context,db_num,lookup_p,lookup_size,gmr,hit_p,index_p,ad_p);
	return return_value;
}
//changing the interface to pass context as a parameter.needed for full production testing.
int
idt75k_lookup_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t db_num, uint32_t *lookup_p, uint32_t lookup_size, uint32_t gmr, uint32_t *hit_p, uint32_t *index_p, uint32_t *ad_p)
{
	uint32_t      wr_words[21];
	uint32_t      rd_words[5];
	uint32_t      i;
	uint32_t      pci_size;
	uint32_t      pci_rd_size = 0;
	uint32_t      dbconf1_value;
	uint32_t      associated_data[4] = { 0, 0, 0, 0 };
	uint32_t      index = 0;
	int           err;

#ifndef _WIN32
	DD ( "idt75k: idt75k_lookup(--, %d, --, %d, %d, --, --, --)\n", db_num, lookup_size, gmr) ;
#endif

	/* Sanity check the input */
	if ( db_num > 15 )
		return EINVAL;
	if ( dev_p == NULL )
		return EINVAL;
	if ( lookup_p == NULL )
		return EINVAL;
	if ( gmr > 0x1F )
		return EINVAL;

	/* Check the lookup size is valid */
	switch(lookup_size)
	{
		case 36:     pci_size = 2;       break;
		case 72:     pci_size = 3;       break;
		case 144:    pci_size = 5;       break;
		case 288:    pci_size = 9;       break;
		case 576:    pci_size = 18;      break;
		default:                         return EINVAL;
	}


	/* First we read the database CSR to determine it's current state and therefore the
	 * format of the lookup output. This is of course very inefficent, ideally the DB
	 * state should be cached inside this driver.
	 */
	if ( (err = idt75k_read_csr_ctx(dev_p,context ,(0x100 | (db_num << 4)), &dbconf1_value)) != ESUCCESS )
		return err;

	/* Construct the request and send it to the device */
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_LOOKUP,gmr,db_num,0);
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, pci_size, INSTR_LOOKUP, gmr, db_num, 0);
	#endif
	//wr_words[1] = 0x00000000;
	for (i=0; i<pci_size; i++)
		wr_words[1+i] = lookup_p[i];

#ifndef _WIN32
	DD ( "idt75k: idt75k_lookup: write %d words { %08X %08X %08X %08X %08X %08X %08X }\n", (pci_size + 2), wr_words[0], wr_words[1], wr_words[2], wr_words[3], wr_words[4], wr_words[5], wr_words[6]) ;
#endif

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * (pci_size+2)));
	#endif
	//printf("no of words : %d\n",(pci_size + 1));
	memcpy_la1(dev_p,wr_words,(pci_size + 1));


	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if we timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if we hit on an entry */
	if ( hit_p != NULL )
		*hit_p = (rd_words[0] & 0x40000000) ? 1 : 0;



	/* Read the assoiated index out of the result mailbox (using the config register to determine the format),
	 * we currently assume that some associated data SRAM is attached.
	 */
	switch (dbconf1_value & IDT75K_DBCONF_AD_MASK)
	{
		case IDT75K_DBCONF_AD_NONE:    pci_rd_size = 1;     break;
		case IDT75K_DBCONF_AD_32:      pci_rd_size = 2;     break;
		case IDT75K_DBCONF_AD_64:      pci_rd_size = 3;     break;
		case IDT75K_DBCONF_AD_128:     pci_rd_size = 5;     break;
	}

	/* Read the additional required words */
	for (i=1; i<pci_rd_size; i++)
	{
		#if 0
			d_words[i] = IDT75K_READ_MAILBOX(dev_p, 0, i);
		#endif
		rd_words[i] = read_mailbox(dev_p,context,i);
	}

#ifndef _WIN32
	DD ( "idt75k: idt75k_lookup: read %d words from mailbox { %08X %08X %08X %08X }\n", (pci_rd_size+1), rd_words[0], rd_words[1], rd_words[2], rd_words[3]) ;
#endif
    
	/* If the caller didn't want the data, get out now */
	if ( (index_p == NULL) && (ad_p == NULL) )
		return ESUCCESS;


	/* Tease the data out of the result mailbox and format correctly */
	if ( dbconf1_value & IDT75K_DBCONF_RTN )
	{
		switch (dbconf1_value & IDT75K_DBCONF_AD_MASK)
		{
			case IDT75K_DBCONF_AD_NONE:
				index = (rd_words[0] & 0x1FFFFFFF);
				break;
			case IDT75K_DBCONF_AD_32:
				associated_data[0] = (rd_words[0] & 0x1FFFFFFF);
				index = (rd_words[1] & 0x001FFFFF);
				break;
			case IDT75K_DBCONF_AD_64:
				associated_data[0] = (rd_words[0] & 0x1FFFFFFF);
				associated_data[1] = (rd_words[1] & 0xFFFFFFFF);
				index = (rd_words[2] & 0x001FFFFF);
				break;
			case IDT75K_DBCONF_AD_128:
				associated_data[0] = (rd_words[0] & 0x1FFFFFFF);
				associated_data[1] = (rd_words[1] & 0xFFFFFFFF);
				associated_data[2] = (rd_words[2] & 0xFFFFFFFF);
				associated_data[3] = (rd_words[3] & 0xFFFFFFFF);
				index = (rd_words[4] & 0x001FFFFF);
				break;
		}
	}
	else
	{
		switch (dbconf1_value & IDT75K_DBCONF_AD_MASK)
		{
			case IDT75K_DBCONF_AD_NONE:
				index = (rd_words[0] & 0x1FFFFFFF);
				break;
			case IDT75K_DBCONF_AD_32:
				associated_data[0] = (rd_words[1] & 0xFFFFFFFF);
				index = (rd_words[0] & 0x001FFFFF);
				break;
			case IDT75K_DBCONF_AD_64:
				associated_data[0] = (rd_words[1] & 0xFFFFFFFF);
				associated_data[1] = (rd_words[2] & 0xFFFFFFFF);
				index = (rd_words[0] & 0x001FFFFF);
				break;
			case IDT75K_DBCONF_AD_128:
				associated_data[0] = (rd_words[1] & 0xFFFFFFFF);
				associated_data[1] = (rd_words[2] & 0xFFFFFFFF);
				associated_data[2] = (rd_words[3] & 0xFFFFFFFF);
				associated_data[3] = (rd_words[4] & 0xFFFFFFFF);
				index = (rd_words[0] & 0x001FFFFF);
				break;
		}
	}


	/* Copy the assoicated data and index to the return values */
	if ( ad_p )
	{
		for (i=0; i<4; i++)
			ad_p[i] = associated_data[i];
	}
	if ( index_p )
	{
		*index_p = index;
	}

	return ESUCCESS;
}



/**
 * Reads the 72-bit value from one of the global mask registers (GMRs).
 *
 * @param[in]  dev_p        Pointer to the driver defined structure
 * @param[in]  addr         Address of the GMR to read, this is actually
 *                          the index of the GMR. Valid values are from
 *                          0x00 to 0x3F.
 * @param[out] value        Pointer to an array of bytes that receives
 *                          the value in the array, this array should
 *                          contain at least 9 bytes. The GMR data is
 *                          loaded into this array with the MSB located
 *                          at index 0 and the LSB as index 8.
 *
 * @return                  Returns one of the following error codes.
 * @retval ESUCCESS         The operation was successiful.
 * @retval EINVAL           An invalid argument was supplied
 * @retval ETIMEDOUT        Timedout waiting for the chip to indicate the lookup
 *                          procedure was complete.
 */
//to keep the old interface
int
idt75k_read_gmr(idt75k_dev_t *dev_p, uint32_t addr, uint8_t value[9])
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_read_gmr_ctx(dev_p,context,addr,value);
	return return_value;
}
//changed the interface to pass context as a parameter.needed for full production testing.
int
idt75k_read_gmr_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t addr, uint8_t value[9])
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[4];

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_gmr(--, 0x%08X, --)\n", addr) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x03F )
		return EINVAL;
	if ( value == NULL )
		return EINVAL;


	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,0,0);
	wr_words[1] = (SUBINSTR_READ << 28) | (IDT75K_REGION_GMR << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif
	memcpy_la1(dev_p,wr_words,2);



	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif

	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;


	/* Read the second result word in the mailbox and asign it to the value */
	#if 0
		rd_words[1] = IDT75K_READ_MAILBOX(dev_p, 0, 1);
		rd_words[2] = IDT75K_READ_MAILBOX(dev_p, 0, 2);
		rd_words[3] = IDT75K_READ_MAILBOX(dev_p, 0, 3);
	#endif

	rd_words[1] = read_mailbox(dev_p,context,1);
	rd_words[2] = read_mailbox(dev_p,context,2);
	rd_words[3] = read_mailbox(dev_p,context,3);

	/* Convert the data to the array specified */
	value[0] = ((rd_words[1] >> 24) & 0xFF);
	value[1] = ((rd_words[1] >> 16) & 0xFF);
	value[2] = ((rd_words[1] >>  8) & 0xFF);
	value[3] = ((rd_words[1] >>  0) & 0xFF);
	value[4] = ((rd_words[2] >> 24) & 0xFF);
	value[5] = ((rd_words[2] >> 16) & 0xFF);
	value[6] = ((rd_words[2] >>  8) & 0xFF);
	value[7] = ((rd_words[2] >>  0) & 0xFF);
	value[8] = ((rd_words[3] >> 24) & 0xFF);

#ifndef _WIN32
	DD ( "idt75k: idt75k_read_gmr(--, 0x%08X, %02X%02X%02X%02X%02X%02X%02X%02X%02X)\n", addr, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8]) ;
#endif

	return ESUCCESS;
}
/**
 * Writes the 72-bit value into one of the global mask registers (GMRs).
 *
 * @param[in]  dev_p        Pointer to the driver defined structure
 * @param[in]  addr         Address of the GMR to read, this is actually
 *                          the index of the GMR. Valid values are from
 *                          0x00 to 0x3F.
 * @param[in]  value        Pointer to an array of bytes that receives
 *                          the value in the array, this array should
 *                          contain at least 9 bytes. The GMR data is
 *                          loaded into this array with the MSB located
 *                          at index 0 and the LSB as index 8.
 *
 * @return                  Returns one of the following error codes.
 * @retval ESUCCESS         The operation was successiful.
 * @retval EINVAL           An invalid argument was supplied
 * @retval ETIMEDOUT        Timedout waiting for the chip to indicate the lookup
 *                          procedure was complete.
 */
int
idt75k_write_gmr(idt75k_dev_t *dev_p, uint32_t addr, const uint8_t value[9])
{
	int return_value = 0;
	uint32_t context = 127;
	
	return_value = idt75k_write_gmr_ctx(dev_p,context,addr,value);
	return return_value;
}
int
idt75k_write_gmr_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t addr, const uint8_t value[9])
{
	uint32_t      wr_words[5];
	uint32_t      rd_words[1];

#ifndef _WIN32
	DD ( "idt75k: idt75k_write_gmr(--, 0x%08X, %02X%02X%02X%02X%02X%02X%02X%02X%02X)\n", addr, value[0], value[1], value[2], value[3], value[4], value[5], value[6], value[7], value[8]) ;
#endif

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( addr > 0x03F )
		return EINVAL;
	if ( value == NULL )
		return EINVAL;


	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 4, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,0,0);
	wr_words[1] = (SUBINSTR_WRITE << 28) | (IDT75K_REGION_GMR << 23) | (dev_p->device_id << 19) | (addr & 0x7FFFF);
	wr_words[2] = ((uint32_t)value[0] << 24) | ((uint32_t)value[1] << 16) | ((uint32_t)value[2] << 8) | ((uint32_t)value[3]);
	wr_words[3] = ((uint32_t)value[4] << 24) | ((uint32_t)value[5] << 16) | ((uint32_t)value[6] << 8) | ((uint32_t)value[7]);
	wr_words[4] = ((uint32_t)value[8] << 24);

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 5));
	#endif
	memcpy_la1(dev_p,wr_words,5);



	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	rd_words[0] = read_mailbox_result(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Check if the error bit is also set */
	if ( rd_words[0] & 0x40000000 )
		return EFAULT;

	return ESUCCESS;
}
/**
 * Performs a reset on one or more interfaces of the device. There is a self imposed
 * delay of 4096 PCI clock cycles (@33Mhz ~ 125us) after a reset, as required by the
 * device.
 *
 * @param[in]  dev_p        Pointer to the driver defined structure
 * @param[in]  reset_args   Should contain one or more flag values indicating
 *                          the type of reset to perform, possible options are:
 *                             IDT75K_RESET_ALL_INTERFACES
 *                             IDT75K_RESET_DLL_MAILBOX
 *
 * @return                  Returns one of the following error codes.
 * @retval ESUCCESS         The operation was successiful.
 * @retval EINVAL           An invalid argument was supplied
 * @retval ETIMEDOUT        Timedout waiting for the chip to indicate the lookup
 *                          procedure was complete.
 */
int
idt75k_reset_device(idt75k_dev_t *dev_p,uint32_t reset_args)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_reset_device_ctx(dev_p,context,reset_args);
	return return_value;
}
//changed the interface to include context as a paramter.needed for full procdcution testing.
int
idt75k_reset_device_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t reset_args)
{
	uint32_t  wr_words[2];

#ifndef _WIN32
	DD ( "idt75k: id75k_reset_devce(--, 0x%08X)\n", reset_args) ;
#endif

	/* sanity checking */
	if ( dev_p == NULL )
		return EINVAL;


	/* construct the request */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,0,0);
	wr_words[1] = (SUBINSTR_RESET << 28);

	if ( (reset_args & IDT75K_RESET_ALL_INTERFACES) == 0 )
		wr_words[1] |= (1 << 25);
	if ( (reset_args & IDT75K_RESET_DLL_MAILBOX) == IDT75K_RESET_DLL_MAILBOX )
		wr_words[1] |= (1 << 24);


	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif
	memcpy_la1(dev_p,wr_words,2);

	/* we have to wait at least 4096 PCI clock cycles before trying to access the device again */
	udelay(200);

	/* there is no result issued in rsponse to a reset instruction */
	return ESUCCESS;
}
/**
 * Performs a flush on the device, which just invalidates all entries
 * in the TCAM.
 *
 * @param[in]  dev_p        Pointer to the driver defined structure
 *
 * @return                  Returns one of the following error codes.
 * @retval ESUCCESS         The operation was successiful.
 * @retval EINVAL           An invalid argument was supplied.
 */
int
idt75k_flush_device(idt75k_dev_t *dev_p)
{
	int return_value = 0;
	uint32_t context = 127;
	return_value = idt75k_flush_device_ctx(dev_p,context);
	return return_value;
}
//changed the interface to include context as a parameter.needed for full production testing.
int
idt75k_flush_device_ctx(idt75k_dev_t *dev_p,uint32_t context)
{
	uint32_t      wr_words[2];
	uint32_t      rd_words[1];

	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;

	/* Construct the request and send it to the device, there is two 32-bit words sent
	 * because we are using the indirect instruction.
	 */
	#if 0
		wr_words[0] = IDT75K_CONSTRUCT_PCI_WRITE(0, 1, INSTR_INDIRECT, 0, 0, 0);
	#endif
	wr_words[0] = IDT75K_CONSTRUCT_LA1_WRITE(context,INSTR_INDIRECT,0,0,0);
	wr_words[1] = (SUBINSTR_FLUSH << 28);

	/* write the data to the chip */
	#if 0
		memcpy_toio ((void*)dev_p->mmio_ptr, wr_words, (sizeof(uint32_t) * 2));
	#endif
	memcpy_la1(dev_p,wr_words,2);

	/* Read the result back from the chip and wait till the done bit is set */
	#if 0
		READ_MAILBOX_RESULT (rd_words[0]);
	#endif
	rd_words[0] = read_mailbox(dev_p,context,0);

	/* Check if wwe timed-out waiting for the done bit (bit 31) */
	if ( (rd_words[0] & 0x80000000) == 0 )
		return ETIMEDOUT;

	/* Success */
	return ESUCCESS;
}
/**
 * Reads a 32-bit from the drivers persistant storage array. The persistant
 * storage is used to store user defined meta data about the device in a
 * process independant location. The data remains valid until the driver is
 * unloaded.
 *
 * @param[in]  dev_p        Pointer to the driver defined structure
 * @param[in]  index        The index into the array of the item being read,
 *                          there are only 16 entries in the persistance array
 *                          so valid values are from 0 to 15 inclusive.
 * @param[out] value_p      Pointer to a value that gets populated with
 *                          data from the array.
 *
 * @return                  Returns one of the following error codes.
 * @retval ESUCCESS         The operation was successiful.
 * @retval EINVAL           An invalid argument was supplied.
 */
int
idt75k_read_pstore(idt75k_dev_t *dev_p, uint32_t index, uint32_t *value_p)
{
	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( index > 15 )
		return EINVAL;
	if ( value_p == NULL )
		return EINVAL;


	*value_p = dev_p->pstore[index];
	return ESUCCESS;
}

/**
 * Writes a 32-bit from the drivers persistant storage array. The persistant
 * storage is used to store user defined meta data about the device in a
 * process independant location. The data remains valid until the driver is
 * unloaded.
 *
 * @param[in]  dev_p        Pointer to the driver defined structure
 * @param[in]  index        The index into the array of the item being read,
 *                          there are only 16 entries in the persistance array
 *                          so valid values are from 0 to 15 inclusive.
 * @param[in]  value        Value to write into the data array.
 *
 * @return                  Returns one of the following error codes.
 * @retval ESUCCESS         The operation was successiful.
 * @retval EINVAL           An invalid argument was supplied.
 */
int
idt75k_write_pstore(idt75k_dev_t *dev_p, uint32_t index, uint32_t value)
{
	/* Sanity checking */
	if ( dev_p == NULL )
		return EINVAL;
	if ( index > 15 )
		return EINVAL;


	dev_p->pstore[index] = value;
	return ESUCCESS;
}

/**
 * IOCTL handler for the device, a list of available IOCTLs is given in the
 * idt75k.h header file. This function mearly consists of a switch statement
 * and calls to other functions that perform the nessecary operation.
 * 
 */
#if 0
int
idt75k_ioctl(struct inode *inode, struct file *fp, u_int cmd, u_long arg)
{
	idt75k_dev_t *dev_p = fp->private_data;
	int           err = 0;

#ifndef _WIN32
	DD ( "idt75k: idt75k_ioctl(--, --, 0x%08x, --)\n", cmd) ;
#endif

	/* Sanity check the input */
	if ( _IOC_TYPE(cmd) != IDT75K_IOCTL_MAGIC )
		return -ENOTTY;

	if ( _IOC_DIR(cmd) & _IOC_READ )
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if ( _IOC_DIR(cmd) & _IOC_WRITE )
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if ( err )   return -EFAULT;




	switch (cmd)
	{
		/* Reads the NSE configuration register */
		case IDT75K_IOCTL_GET_NSE_CONF:
		{
			uint32_t value;
			if ( (err = idt75k_read_csr(dev_p, NSE_CONF_ADDR, &value)) != ESUCCESS )
				return -err;

			__put_user(value, (uint32_t __user *)arg);
			return 0;
		}

		/* Writes the NSE configuration register */
		case IDT75K_IOCTL_SET_NSE_CONF:
		{
			uint32_t value;
			
			__get_user(value, (uint32_t __user *)arg);
			if ( (err = idt75k_write_csr(dev_p, NSE_CONF_ADDR, value)) != ESUCCESS )
				return -err;
			else
				return 0;
		}


		/* Reads a CSR register */
		case IDT75K_IOCTL_READ_CSR:
		{
			idt75k_read_csr_t  csr;

			if (copy_from_user(&csr, (void*)arg, sizeof(idt75k_read_csr_t)))
				return -EFAULT;
			
			err = idt75k_read_csr(dev_p, csr.address, &csr.value);
			
			if (copy_to_user((void*)arg, &csr, sizeof(idt75k_read_csr_t)))
				return -EFAULT;
			
			return -err;
		}

		/* Writes the NSE configuration register */
		case IDT75K_IOCTL_WRITE_CSR:
		{
			idt75k_write_csr_t  csr;

			if (copy_from_user(&csr, (void*)arg, sizeof(idt75k_write_csr_t)))
				return -EFAULT;
			
			err = idt75k_write_csr(dev_p, csr.address, csr.value);
			
			//if (copy_to_user((void*)arg, &csr, sizeof(idt75k_write_csr_t)))
			//	return -EFAULT;
			
			return -err;
		}



		/* Reads the identification registers from the device */
		case IDT75K_IOCTL_GET_IDENT1_REG:
		case IDT75K_IOCTL_GET_IDENT2_REG:
		{
			uint32_t value;
			uint32_t addr = (cmd == IDT75K_IOCTL_GET_IDENT1_REG) ? NSE_IDENT1_ADDR : NSE_IDENT2_ADDR;
			if ( (err = idt75k_read_csr(dev_p, addr, &value)) != ESUCCESS )
				return -err;

			__put_user(value, (uint32_t __user *)arg);
			return 0;
		}

		/* Read a database configuration from the device */
		case IDT75K_IOCTL_GET_DBCONF:
		{
			idt75k_get_dbconf_t  dbconf;

			if (copy_from_user(&dbconf, (void*)arg, sizeof(idt75k_get_dbconf_t)))
				return -EFAULT;
			
			err = idt75k_get_dbconf(dev_p, dbconf.db_num, &dbconf.flags, &dbconf.age_cnt, &dbconf.segments);
			
			if (copy_to_user((void*)arg, &dbconf, sizeof(idt75k_get_dbconf_t)))
				return -EFAULT;
			
			return -err;
		}

		/* Configures a database on the device */
		case IDT75K_IOCTL_SET_DBCONF:
		{
			idt75k_set_dbconf_t  dbconf;

			if (copy_from_user(&dbconf, (void*)arg, sizeof(idt75k_set_dbconf_t)))
				return -EFAULT;
			
			err = idt75k_set_dbconf(dev_p, dbconf.db_num, dbconf.mask, dbconf.flags, dbconf.age_cnt, dbconf.segments);
		
			return -err;
		}


		/* Reads the mask value from one of the GMRs */
		case IDT75K_IOCTL_READ_GMR:
		{
			idt75k_read_gmr_t  gmr;

			if (copy_from_user(&gmr, (void*)arg, sizeof(idt75k_read_gmr_t)))
				return -EFAULT;
			
			err = idt75k_read_gmr(dev_p, gmr.address, gmr.value);
			
			if (copy_to_user((void*)arg, &gmr, sizeof(idt75k_read_gmr_t)))
				return -EFAULT;
			
			return -err;
		}


		/* Reads the mask value from one of the GMRs */
		case IDT75K_IOCTL_WRITE_GMR:
		{
			idt75k_write_gmr_t  gmr;

			if (copy_from_user(&gmr, (void*)arg, sizeof(idt75k_write_gmr_t)))
				return -EFAULT;
			
			err = idt75k_write_gmr(dev_p, gmr.address, gmr.value);
			
			return -err;
		}


		/* Performs a lookup on the device */
		case IDT75K_IOCTL_LOOKUP:
		{
			idt75k_lookup_t  lookup;
			uint32_t         hit = 0;

			if (copy_from_user(&lookup, (void*)arg, sizeof(idt75k_lookup_t)))
				return -EFAULT;
			
			err = idt75k_lookup(dev_p, lookup.db_num, lookup.lookup, lookup.lookup_size, lookup.gmr, &hit, &lookup.index, lookup.associated_data);
			lookup.hit = hit ? 1 : 0;
			
			if (copy_to_user((void*)arg, &lookup, sizeof(idt75k_lookup_t)))
				return -EFAULT;
			
			return -err;
		}


		/* Reads an entry from the TCAM core */
		case IDT75K_IOCTL_READ_ENTRY:
		{
			idt75k_read_entry_t  entry;
			uint32_t             valid;

			if (copy_from_user(&entry, (void*)arg, sizeof(idt75k_read_entry_t)))
				return -EFAULT;
			
			err = idt75k_read_cam_entry(dev_p, entry.address, entry.data, entry.mask, &valid);
			entry.valid = (valid == 0) ? 0 : 1;

			if (copy_to_user((void*)arg, &entry, sizeof(idt75k_read_entry_t)))
				return -EFAULT;
			
			return -err;
		}


		/* Writes an entry to the TCAM core */
		case IDT75K_IOCTL_WRITE_ENTRY:
		{
			idt75k_write_entry_t  entry;

			if (copy_from_user(&entry, (void*)arg, sizeof(idt75k_write_entry_t)))
				return -EFAULT;
			
			err = idt75k_write_cam_entry(dev_p, entry.db_num, entry.address, entry.data, entry.mask, entry.gmask, entry.validate);

			return -err;
		}


		/* Writes an entry to the TCAM core with extra options */
		case IDT75K_IOCTL_WRITE_ENTRY_EX:
		{
			idt75k_write_entry_ex_t  entry;

			if (copy_from_user(&entry, (void*)arg, sizeof(idt75k_write_entry_ex_t)))
				return -EFAULT;
			
			err = idt75k_write_cam_entry_ex(dev_p, entry.db_num, entry.address, entry.data, entry.mask, entry.gmask, entry.ad_value);
			if ( err != 0 )    return -err;

			/* for now we have to do this twice ... yes I don't know why */
			//udelay(500);
			//err = idt75k_write_sram(dev_p, entry.db_num, entry.address, entry.ad_value);

			return -err;
		}


		/* Reads a value from the assoicated SRAM data */
		case IDT75K_IOCTL_READ_SRAM:
		{
			idt75k_read_sram_t  rd_sram;

			if (copy_from_user(&rd_sram, (void*)arg, sizeof(idt75k_read_sram_t)))
				return -EFAULT;
			
			err = idt75k_read_sram(dev_p, rd_sram.db_num, rd_sram.address, rd_sram.sram_entry);
			
			if (copy_to_user((void*)arg, &rd_sram, sizeof(idt75k_read_sram_t)))
				return -EFAULT;
			
			return -err;
		}


		/* Reads a value from the assoicated SRAM data */
		case IDT75K_IOCTL_WRITE_SRAM:
		{
			idt75k_write_sram_t  wr_sram;

			if (copy_from_user(&wr_sram, (void*)arg, sizeof(idt75k_write_sram_t)))
				return -EFAULT;
			
			err = idt75k_write_sram(dev_p, wr_sram.db_num, wr_sram.address, wr_sram.sram_entry);
			if ( err != 0 )      return -err;
			
			/* for now we have to do this twice ... yes I don't know why */
			//err = idt75k_write_sram(dev_p, wr_sram.db_num, wr_sram.address, wr_sram.sram_entry);
			
			return -err;
		}



		/* Reads a value from the assoicated SRAM data */
		case IDT75K_IOCTL_COPY_SRAM:
		{
			idt75k_copy_sram_t  cp_sram;

			if (copy_from_user(&cp_sram, (void*)arg, sizeof(idt75k_copy_sram_t)))
				return -EFAULT;
			
			err = idt75k_copy_sram(dev_p, cp_sram.db_num, cp_sram.dst_addr, cp_sram.src_addr);
			return -err;
		}



		/* Reads a value from the segment mapping table (SMT) */
		case IDT75K_IOCTL_READ_SMT:
		{
			idt75k_read_smt_t  rd_smt;

			if (copy_from_user(&rd_smt, (void*)arg, sizeof(idt75k_read_smt_t)))
				return -EFAULT;
			
			err = idt75k_read_smt(dev_p, rd_smt.addr, &rd_smt.value);
			
			if (copy_to_user((void*)arg, &rd_smt, sizeof(idt75k_read_smt_t)))
				return -EFAULT;

			return -err;
		}


		/* Writes a value from the segment mapping table (SMT) */
		case IDT75K_IOCTL_WRITE_SMT:
		{
			idt75k_write_smt_t  wr_smt;

			if (copy_from_user(&wr_smt, (void*)arg, sizeof(idt75k_write_smt_t)))
				return -EFAULT;
			
			err = idt75k_write_smt(dev_p, wr_smt.addr, wr_smt.value);
			return -err;
		}



		/* Sets the valid bit assoicated with a TCAM entry */
		case IDT75K_IOCTL_SET_VALID:
		{
			idt75k_set_valid_t  set_valid;

			if (copy_from_user(&set_valid, (void*)arg, sizeof(idt75k_set_valid_t)))
				return -EFAULT;
			
			err = id75k_set_cam_entry_state(dev_p, set_valid.db_num, set_valid.address, 1);
			return -err;
		}


		/* Sets the valid bit assoicated with a TCAM entry */
		case IDT75K_IOCTL_CLEAR_VALID:
		{
			idt75k_clear_valid_t  clr_valid;

			if (copy_from_user(&clr_valid, (void*)arg, sizeof(idt75k_clear_valid_t)))
				return -EFAULT;
			
			err = id75k_set_cam_entry_state(dev_p, clr_valid.db_num, clr_valid.address, 0);
			return -err;
		}


		/* Resets the device */
		case IDT75K_IOCTL_RESET:
		{
			uint32_t reset_arg;
			__get_user(reset_arg, (uint32_t __user *)arg);

			err = id75k_reset_device(dev_p, reset_arg);
			return -err;
		}


		/* Flushes the contents of the TCAM */
		case IDT75K_IOCTL_FLUSH:
		{
			err = id75k_flush_device(dev_p);
			return -err;
		}
	


		/* Read a 32-bit value from the persistant storage array */
		case IDT75K_IOCTL_READ_PSTORE:
		{
			idt75k_read_pstore_t  rd_pstore;

			if (copy_from_user(&rd_pstore, (void*)arg, sizeof(idt75k_read_pstore_t)))
				return -EFAULT;
			
			err = idt75k_read_pstore(dev_p, rd_pstore.index, &rd_pstore.value);
			
			if (copy_to_user((void*)arg, &rd_pstore, sizeof(idt75k_read_pstore_t)))
				return -EFAULT;
			
			return -err;
		}

		/* Writes a 32-bit value to the persistant storage array */
		case IDT75K_IOCTL_WRITE_PSTORE:
		{
			idt75k_write_pstore_t  wr_pstore;

			if (copy_from_user(&wr_pstore, (void*)arg, sizeof(idt75k_write_pstore_t)))
				return -EFAULT;
			
			err = idt75k_write_pstore(dev_p, wr_pstore.index, wr_pstore.value);
			return -err;
		}

		default:
			return -ENODEV;
	}
}
#endif 



















