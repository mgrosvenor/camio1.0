#include <stdio.h>
#include <errno.h>
#include <string.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif
#include "dagapi.h"
#include "dag_config.h"
#include "dag_component.h"
#include "dag_platform.h"
#include "dagutil.h"
#include "idt52k_lib.h"
#include "dagapi.h"
#include "dag_config.h"
#include "dag_component.h"
#include "dag_platform.h"
#include "dagutil.h"
/*Static function prototypes*/
static void tcam_set_last_error(cam_err_t err);
static  cam_err_t tcam_get_last_error();
/*
 * This function writes 72 bit words.noting that the width of the TCAM entry is 72 bits.
 * the function that calls this function has to write in consecutive locations after 
 * appropriately configuring the Width of the database and selecting the segments using LAR and SSR;
 */
#if 0
struct bit72_word_t
{
	uint32_t data0; /**< lower order 64-bits */
	uint32_t data1; /**< higher order 8-bits */
	uint8_t data2;
};
#endif
cam_err_t error_code;
/*Write the 32-bit value to the specified F/W register.*/
void
write_sc256_register(FilterStateHeaderPtr header,uint8_t addr, uint32_t reg_val)
{
	*(header->copro_register_base + addr) = reg_val;
#if USE_NANOSLEEPS
	dagutil_nanosleep(100);
#endif
	dagutil_verbose_level(3,"Content written to the register at offset 0x%x: %08x\n",addr*4,reg_val);
}
/*Read the value from the Specified F/W register.*/
uint32_t
read_sc256_register(FilterStateHeaderPtr header,uint8_t addr)
{
	
	dagutil_verbose_level(3,"\nRegister at offset 0x%08x has been read\n",addr*4);

	return *(header->copro_register_base + addr);
}
void
write_256cam_value(FilterStateHeaderPtr header, uint32_t addr, bit72_word_t value, uint8_t access_region)
{
	unsigned int val_indx = 0;
	uint8_t inst = INSTR_WRITE;
	int timeout = TIMEOUT;
	
	tcam_set_last_error(kValueNoError);
	
	dagutil_verbose_level(3, "Writing to CAM - addr:%08x data:%02x %08x %08x space:%d\n", 
			          addr, value.data2, value.data1 , value.data0,access_region);
	/*Populating the 72 - bits to be written*/
	*(header->copro_register_base + SC256_VAL_ARRAY + val_indx) = (value.data0); 
	val_indx++;
		
	*(header->copro_register_base + SC256_VAL_ARRAY + val_indx) = (value.data1);
	val_indx++;
	
	*(header->copro_register_base + SC256_VAL_ARRAY + val_indx) = (value.data2); 
	
	*(header->copro_register_base + SC256_CSR0) = 0; 
	*(header->copro_register_base + SC256_CSR1) = (((inst & 0x7) << 28 ) | ((access_region & 0x3) << 20) | (addr & 0xfffff)); 
	/*Setting the 'Go' bit.*/
	*(header->copro_register_base + SC256_CSR2) = 0x00000001; 
	
	while ((timeout > 0) && (*(header->copro_register_base + SC256_CSR2) & BIT28))
	{
#if USE_NANOSLEEPS
		dagutil_nanosleep(110);
#endif
		timeout--;
	}
	if (0 == timeout) 
	{
		tcam_set_last_error(kValueTimedout);
		dagutil_warning("timeout in %s\n", __FUNCTION__);
	}
}
bit72_word_t
read_256cam_value(FilterStateHeaderPtr header, uint32_t addr, uint8_t access_region)
{
	bit72_word_t value;
	uint8_t inst = INSTR_READ;
	int timeout = TIMEOUT;
	
	/*Clearing the last error*/
	tcam_set_last_error(kValueNoError);
	
	*(header->copro_register_base + SC256_CSR0) = 0; 
	*(header->copro_register_base + SC256_CSR1) = (((inst & 0x7) << 28 ) | ((access_region & 0x3) << 20) | (addr & 0xfffff)); 
	/*Setting the 'Go' bit*/
	(*(header->copro_register_base + SC256_CSR2)) = 0x00000001; 

	while ((timeout > 0) && (*(header->copro_register_base + SC256_CSR2) & BIT28))
	{
#if USE_NANOSLEEPS
		dagutil_nanosleep(110);
#endif
		timeout--;
	}
	if (0 == timeout) 
	{
		tcam_set_last_error(kValueTimedout);
		dagutil_verbose_level(1, "timeout in %s\n", __FUNCTION__);
		value.data0 = 0;
		value.data1 = 0;
		value.data2 = 0;
		return value;
	}
	
	value.data0 = (*(header->copro_register_base + SC256_RES_ARRAY));
	value.data1 = (*(header->copro_register_base + SC256_RES_ARRAY + 1));
	value.data2 = (*(header->copro_register_base + SC256_RES_ARRAY + 2) & 0xffLL);
	
    	dagutil_verbose_level(3, "Read from CAM  - addr:%08x data:%02x %08x %08x space:%d\n", 
			      addr, value.data2, value.data1, value.data0,access_region);
	return value;
}
/*Read and Write the value array*/
bit72_word_t
read_sc256_val_array(FilterStateHeaderPtr header)
{
	bit72_word_t val;
	val.data0 = (*(header->copro_register_base + SC256_RES_ARRAY));
	val.data1 = (*(header->copro_register_base + SC256_RES_ARRAY + 1));
	val.data2 = (*(header->copro_register_base + SC256_RES_ARRAY + 2) & 0xffLL);

	dagutil_nanosleep(110);
	dagutil_verbose_level(3, "Read from CAM  - data:%02x %08x %08x\n", 
			           val.data2, val.data1, val.data0);
	return val;
}
void 
write_sc256_val_array(FilterStateHeaderPtr header,bit72_word_t value)
{
	unsigned int val_indx = 0;
	dagutil_verbose_level(3, "Writing to CAM - data:%02x %08x %08x\n", 
			          value.data2, value.data1 , value.data0);
	
	/*Populating the 72 - bits to be written*/
	*(header->copro_register_base + SC256_VAL_ARRAY + val_indx) = (value.data0); 
	val_indx++;
	
	*(header->copro_register_base + SC256_VAL_ARRAY + val_indx) = (value.data1);
	val_indx++;

	*(header->copro_register_base + SC256_VAL_ARRAY + val_indx) = (value.data2); 
	
}
static void tcam_set_last_error(cam_err_t err)
{
	error_code = err;
}
static  cam_err_t tcam_get_last_error()
{
	return error_code;
}
/**
 *  * Writes an entry to the TCAM at the given address
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  * @param[in]  addr       The address (index) of the TCAM entry to write.
 *  *
 *  * @param[in]  data       Pointer to an array of bytes that gets loaded
 *  *                        into the data part of the entry.
 *  * @param[in]  mask       Pointer to an array of bytes that gets loaded
 *  *                        into the mask part of the entry.
 *  * @param[in]  gmask      Indicates which global mask to apply to the key
 *  *                        during writing.
 *  *
 *  * @return                Returns one of the following error codes.
 *  * @retval ESUCCESS       The operation was successiful.
 *  * @retval EINVAL         An invalid argument was supplied
 *  * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *  *                        procedure was complete.
 *  * @retval EFAULT         The error bit was set by the chip in response to
 *  *                        the read command.
 *  */
int
idt52k_write_cam_entry(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t addr, const uint8_t data[72],const uint8_t mask[72])
{
	bit72_word_t value;
	uint32_t i = 0;
	uint32_t address = (db_num * SEGMENT_SIZE) + addr;	

	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	if ( addr > 0xFFF)
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	
	/*assume that we have bit72 structure populated.
	*with 72 bits consecutively from the start address data[0]
	we have the data base size*/

	value.data2 = ((uint8_t)data[i+8]);
				
	value.data1 = ((uint32_t)data[i + 7] << 24)|
		       ((uint32_t)data[i + 6] << 16)|
		       ((uint32_t)data[i + 5] << 8) |
		       ((uint32_t)data[i + 4]);
	
        value.data0 = ((uint32_t)data[i + 3] << 24)|
		      ((uint32_t)data[i + 2] << 16)|
		      ((uint32_t)data[i + 1] <<  8)|
		      ((uint32_t)data[i]);

	/*The only error code returned by write_256cam_value -> TCAM timed out.*/
	write_256cam_value(header,address,value,ACCESS_DATA_REGION);
	if(tcam_get_last_error() != kValueNoError)
			return ETIMEDOUT;
	
					
	i = 0;	
	value.data2 = ((uint8_t)mask[i + 8]);
	value.data1 = (((uint32_t)mask[i + 7] << 24)| 
		       ((uint32_t)mask[i + 6] << 16)| 
		       ((uint32_t)mask[i + 5] << 8) | 
		       ((uint32_t)mask[i + 4]));
 
	value.data0 = (((uint32_t)mask[i + 3] << 24)| 
		       ((uint32_t)mask[i + 2] << 16)| 
		       ((uint32_t)mask[i + 1] << 8) | 
		       ((uint32_t)mask[i]));

	/*The only error code returned by write_256cam_value -> TCAM timed out.*/	
	write_256cam_value(header,address,value,ACCESS_MASK_REGION);
	if(tcam_get_last_error() != kValueNoError)
		return ETIMEDOUT;
		
	return ESUCCESS;
}
int
idt52k_read_cam_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t addr,uint8_t data[72], uint8_t mask[72])
{
	int i = 0;
	bit72_word_t value;
	uint32_t address = (db_num*SEGMENT_SIZE) + addr;

	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	if ( addr > 0xFFF)
		return EINVAL;
	if (data == NULL)
		return EINVAL;
	if (mask == NULL)
		return EINVAL;
	
	/*The only error code returned by read_256cam_value -> TCAM timed out.*/			
	value = read_256cam_value(header,address,ACCESS_DATA_REGION);
	if(tcam_get_last_error() != kValueNoError)
		return ETIMEDOUT;
		
	/*Inverting read.*/

	data[i+8] = value.data2;
	data[i+7] = (uint8_t)(value.data1 >> 24);
	data[i+6] = (uint8_t)(value.data1 >> 16);
	data[i+5] = (uint8_t)(value.data1 >> 8);
	data[i+4] = (uint8_t)(value.data1 >> 0);
	data[i+3] = (uint8_t)(value.data0 >> 24);
	data[i+2] = (uint8_t)(value.data0 >> 16);
	data[i+1] = (uint8_t)(value.data0 >> 8);
	data[i] = (uint8_t)(value.data0 >> 0);

		
	i = 0;

	/*The only error code returned by read_256cam_value -> TCAM timed out.*/	
	value = read_256cam_value(header,address,ACCESS_MASK_REGION);
	if(tcam_get_last_error() != kValueNoError)
		return ETIMEDOUT;
		
	mask[i+8] = value.data2;
	mask[i+7] = (uint8_t)(value.data1 >> 24);
	mask[i+6] = (uint8_t)(value.data1 >> 16);
	mask[i+5] = (uint8_t)(value.data1 >> 8);
	mask[i+4] = (uint8_t)(value.data1 >> 0);
	mask[i+3] = (uint8_t)(value.data0 >> 24);
	mask[i+2] = (uint8_t)(value.data0 >> 16);
	mask[i+1] = (uint8_t)(value.data0 >> 8);
	mask[i] = (uint8_t)(value.data0 >> 0);	
		

	return ESUCCESS;
}
int
idt52k_cam_lookup_entry(FilterStateHeaderPtr header,uint32_t segment,uint32_t data[18],uint32_t lookup_size,uint32_t gmask,bool *hit_p,uint32_t *index_p,uint32_t *ad_p)
{
	int loop = 0;
	uint32_t reg_val0,reg_val1,reg_val2 = 0;
	uint32_t val = 0;
	uint32_t ltin = 0;
	uint32_t width = 0;
	uint32_t comparand = 0;
	uint32_t result = 0; /*Learn operation not supported now.*/	

	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	if (segment > 7)
		return EINVAL;
	if (data == NULL)
		return EINVAL;
	
	switch(lookup_size)
	{
		case 72:   width = 3;   break;
		case 144:  width = 5;   break;
		case 288:  width = 9;   break;
		case 576:  width = 18;  break;
		default: 	     return EINVAL;
	}
	
	/*Convert the data (8 - bit) to srch_data (32 - bit)*/
	for(loop = 0;loop < width;loop++)
	{
		write_sc256_register(header,SC256_VAL_ARRAY + loop,data[loop]);
	}

	ltin = (lookup_size/72) - 1;
	val = ((segment << 26) | (comparand << 23) | (result << 20));
	write_sc256_register(header,SC256_CSR0,val);
	
	val = ((INSTR_LOOKUP << 28)| (gmask << 24)| (ltin << 22));
	//val = 0x20c00000;
	write_sc256_register(header,SC256_CSR1,val);
	
	/*Set the 'Go' bit*/
	val = 1;
	write_sc256_register(header,SC256_CSR2,val); 
	
	
#if USE_NANOSLEEPS	
	dagutil_nanosleep(220);
#endif	
	reg_val0 = read_sc256_register(header,SC256_RES_ARRAY);
	reg_val1 = read_sc256_register(header,SC256_RES_ARRAY + 1);
	reg_val2 = read_sc256_register(header,SC256_RES_ARRAY + 2);
	
	printf("Contents of Result Array:  %08x %08x %08x\n",reg_val2, reg_val1, reg_val0);
	printf("valid: %x rdack: %x mmout: %x hitack: %x\n", ((reg_val2 >> 11) & 0x1),((reg_val2 >> 10) & 0x1), 
	       ((reg_val2 >> 9) & 0x1), ((reg_val2 >> 8) & 0x1));
	printf("index: %x\n", (reg_val0 & 0xfffff));

	*index_p = reg_val0 & 0xfffff;
	*hit_p = ((reg_val2 >> 8) & 0x1);
	return 0;
}
int idt52k_tcam_initialise(FilterStateHeaderPtr header,bool mode)
{
	uint32_t value = 0;
	bit72_word_t reg_val;

	if( header == NULL )
		return EINVAL;

	/*Disable Tcam Search Port*/
	
	write_sc256_register(header,SC256_CSR2,0x20000000);

	/*Initialise TCAM*/

	write_sc256_register(header,SC256_CSR2,0x80000000);
	dagutil_nanosleep(640);

	
	/*Burst Write Happens here.*/
	//case 1:
	/*Write Zero's using Burst Write.*/
	write_sc256_register(header,SC256_VAL_ARRAY,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

	value = 0xffff;
	write_sc256_register(header,SC256_CSR0,value);
	value = 0x10000000;
	write_sc256_register(header,SC256_CSR1,value);
	value = 0x2;
	write_sc256_register(header,SC256_CSR2,value);

	while(read_sc256_register(header,SC256_CSR2)& 0x2);

	#if 0
	//case 2
	//2 partitions one with BIT 143 0 and other with BIT 142 1
	/*Write Zero's using Burst Write.*/
	//flipping BIT 142 
	value = 0;
	write_sc256_register(header,SC256_VAL_ARRAY,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

	value = 0x7fff;
	write_sc256_register(header,SC256_CSR0,value);
	value = 0x10000000;
	write_sc256_register(header,SC256_CSR1,value);
	value = 0x2;
	write_sc256_register(header,SC256_CSR2,value);

	while(read_sc256_register(header,SC256_CSR2)& 0x2);
	//first half done now sec half.
	value2 = 0x40;
	for (loop = 8000;loop < 0xffff;loop++)	
	{
		write_sc256_register(header,SC256_VAL_ARRAY,value2);
		write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
		write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

		value = 0xffff;
		write_sc256_register(header,SC256_CSR0,value);
		value = 0x10000000;
		write_sc256_register(header,SC256_CSR1,value);
		value = 0x2;
		write_sc256_register(header,SC256_CSR2,value);

		while(read_sc256_register(header,SC256_CSR2)& 0x2);
	}
	//case 3
	//4 partitions one with BIT 144 and BIT 143 4 combinations 0 - 0 done.
	value = 0;
	write_sc256_register(header,SC256_VAL_ARRAY,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

	value = 0x3fff;
	write_sc256_register(header,SC256_CSR0,value);
	value = 0x10000000;
	write_sc256_register(header,SC256_CSR1,value);
	value = 0x2;
	write_sc256_register(header,SC256_CSR2,value);

	while(read_sc256_register(header,SC256_CSR2)& 0x2);
	//first half done now sec half.
	value2 = 0x40;
	for (loop = 4000;loop < 0x7fff;loop++)	
	{
		write_sc256_register(header,SC256_VAL_ARRAY,value2);
		write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
		write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

		value = 0xffff;
		write_sc256_register(header,SC256_CSR0,value);
		value = 0x10000000;
		write_sc256_register(header,SC256_CSR1,value);
		value = 0x2;
		write_sc256_register(header,SC256_CSR2,value);

		while(read_sc256_register(header,SC256_CSR2)& 0x2);
	}
	for (loop = 0x7fff;loop < 8000;loop++)	
	{
		write_sc256_register(header,SC256_VAL_ARRAY,value2);
		write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
		write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

		value = 0xffff;
		write_sc256_register(header,SC256_CSR0,value);
		value = 0x10000000;
		write_sc256_register(header,SC256_CSR1,value);
		value = 0x2;
		write_sc256_register(header,SC256_CSR2,value);

		while(read_sc256_register(header,SC256_CSR2)& 0x2);
	}
	value = xx;
	for (loop = 0x7fff;loop < 8000;loop++)	
	{
		write_sc256_register(header,SC256_VAL_ARRAY,value2);
		write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
		write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

		value = 0xffff;
		write_sc256_register(header,SC256_CSR0,value);
		value = 0x10000000;
		write_sc256_register(header,SC256_CSR1,value);
		value = 0x2;
		write_sc256_register(header,SC256_CSR2,value);

		while(read_sc256_register(header,SC256_CSR2)& 0x2);
	}
	value = xx;
	for (loop = 0x7fff;loop < 8000;loop++)	
	{
		write_sc256_register(header,SC256_VAL_ARRAY,value2);
		write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
		write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

		value = 0xffff;
		write_sc256_register(header,SC256_CSR0,value);
		value = 0x10000000;
		write_sc256_register(header,SC256_CSR1,value);
		value = 0x2;
		write_sc256_register(header,SC256_CSR2,value);

		while(read_sc256_register(header,SC256_CSR2)& 0x2);
	}
	/*End of Burst Write */

	#endif


	/*Core Mask should always be filled up with 11111111's*/
	
	/*Write ff's using Burst Write.*/
	value = 0xffffffff;
	write_sc256_register(header,SC256_VAL_ARRAY,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 1,value);
	write_sc256_register(header,SC256_VAL_ARRAY + 2,value);

	value = 0xffff;
	write_sc256_register(header,SC256_CSR0,value);
	value = 0x10100000;
	write_sc256_register(header,SC256_CSR1,value);
	value = 0x2;
	write_sc256_register(header,SC256_CSR2,value);

	while(read_sc256_register(header,SC256_CSR2)& 0x2);

	/*mode - 0 -> lookup width 144*/
	/*mode - 1 -> lookup width 576*/
	
	/*Initialise Lookup Allocation Register.*/
	if(mode == 0)
	{
		reg_val.data0 = 0xffff0000;
		reg_val.data1 = 0x00000000;
		reg_val.data2 = 0x00;
	}else if(mode == 1)
	{
		reg_val.data0 = 0x00000000;
		reg_val.data1 = 0xffff0000;
		reg_val.data2 = 0x00;
	}
	

	write_256cam_value(header,LAR,reg_val,ACCESS_REGISTER_REGION);

		
	/*We will configure just 1 database - with 16 segments each.*/
	
	reg_val.data0 = 0x0000ffff;
	reg_val.data1 = 0x00000000;
	reg_val.data2 = 0;

	write_256cam_value(header,SSR0,reg_val,ACCESS_REGISTER_REGION);

	reg_val.data0 = 0;
	reg_val.data1 = 0;
	reg_val.data2 = 0;

	write_256cam_value(header,SSR1,reg_val,ACCESS_REGISTER_REGION);
		
	/*Initialis System Configuration Register.*/

	reg_val.data0 = 0x81;
	reg_val.data1 = 0;
	reg_val.data2 = 0;

	write_256cam_value(header,SCR,reg_val,ACCESS_REGISTER_REGION);

	/*Enable Search Port*/
	write_sc256_register(header,SC256_CSR2,0x40000000);
	
	return 0;
}
int idt52k_get_tcam_mode(FilterStateHeaderPtr header)
{
	bit72_word_t value;

	if(header == NULL)
		return EINVAL;

	value = read_256cam_value(header,LAR,ACCESS_REGISTER_REGION);
		
	/*Only two possible values for the mode - 144 bit or 576  - meaing all the segments will be configured with this width.*/

	if(((value.data0 & 0xffff0000) >> 16) == 0xffff)
		return 144;
	else if(((value.data1 & 0xffff0000) >> 16) == 0xffff) 
		return 576;
	else
		return -1;
}
int 
idt52k_enable_search(FilterStateHeaderPtr header)
{
	uint32_t value = 0x40000000;

	if (header == NULL)
		return EINVAL;

	write_sc256_register(header,SC256_CSR2,value);
	dagutil_verbose("\n Firmware Search Port Enabled.\n");
	return 0;
}
int 
idt52k_inhibit_search(FilterStateHeaderPtr header)
{
	uint32_t value = 0x20000000;

	if (header == NULL)
		return EINVAL;

	write_sc256_register(header,SC256_CSR2,value);
	dagutil_verbose("\n Firmware Search Port Inhibited.\n");
	return 0;
}
int 
idt52k_reset_tcam(FilterStateHeaderPtr header)
{
	uint32_t value = 0x80000000;

	if ( header == NULL)
		return EINVAL;

	write_sc256_register(header,SC256_CSR2,value);
	dagutil_verbose("TCAM reset.\n");
	return 0;
}
int
idt52k_write_sram_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t address,uint32_t value)
{
	/*header->ipf_register_base*/
    uint32_t addr = (db_num * SEGMENT_SIZE) + address;     
	int timeout = TIMEOUT;

	if(header == NULL)
		return EINVAL;
	
	*(uint32_t*)(header->ipf_register_base + COLOUR_MEMORY_ADDRESS) = addr&0xffff;
	 
	while((timeout > 0) && (!(*(volatile uint32_t*)(header->ipf_register_base + COLOUR_MEMORY_ADDRESS) & BIT31)))
	{
#if USE_NANOSLEEPS	
		dagutil_nanosleep(110);
#endif
		timeout--;
	}
	if (0 == timeout) 
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	*(uint32_t*)(header->ipf_register_base + COLOUR_MEMORY_VALUE) = (value&0xffff); 
	
	return ESUCCESS;
}
int
idt52k_read_sram_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t address,uint32_t *ad_value)
{
	uint32_t val;
	/*header->ipf_register_base*/
    uint32_t addr = (db_num * SEGMENT_SIZE) + address;
	int timeout = TIMEOUT;

	if(header == NULL)
		return EINVAL;
	if (ad_value == NULL)
		return EINVAL;
	
	*(uint32_t*)(header->ipf_register_base + COLOUR_MEMORY_ADDRESS) = addr&0xffff;
	 
	val = *(uint32_t*)(header->ipf_register_base + COLOUR_MEMORY_ADDRESS);
	while((timeout > 0) && (!(*(volatile uint32_t*)(header->ipf_register_base + COLOUR_MEMORY_ADDRESS) & BIT31)))
	{
#if USE_NANOSLEEPS	
		dagutil_nanosleep(110);
#endif
		timeout--;
	}
	if (0 == timeout) 
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	*ad_value = *(uint32_t*)(header->ipf_register_base + COLOUR_MEMORY_VALUE); 
	
	return ESUCCESS;
}
int 
idt52k_write_gmr_entry(FilterStateHeaderPtr header,uint32_t address,uint8_t mask[72],uint32_t width)
{
	bit72_word_t value;
	int i =0;
	
	for (i = 0; i < width;i++)
	{

		value.data0 = ((mask[3+9*i] << 24) | (mask[2+9*i] << 16) | (mask[1+9*i] << 8) | (mask[0+9*i]));
		value.data1 = ((mask[7+9*i] << 24) | (mask[6+9*i] << 16) | (mask[5 +9*i] << 8) | (mask[4+9*i]));
		value.data2 = mask[8+9*i];
		write_256cam_value(header,address,value,ACCESS_REGISTER_REGION);
		address++;
	}
	return 0;
}
