#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "dagapi.h"
#include "dag_config.h"
#include "dag_component.h"
#include "dag_platform.h"
#include "dagutil.h"
#include "dagapi.h"
#include "dag_config.h"
#include "dag_component.h"
#include "dag_platform.h"
#include "dagutil.h"
#include "bfs_lib.h"

#define MAX_ATTEMPTS 100

/**
 *  * Writes an entry into the BFS cam 
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  * @param[in]  addr       The address (index) of the TCAM entry to write.
 *  *
 *  * @param[in]  data       Pointer to an array of bytes that gets loaded
 *  *                        into the data part of the entry.
 *  * @param[in]  mask       Pointer to an array of bytes that gets loaded
 *  *                        into the mask part of the entry.
 *  * @param[in]  db         The database (BANK into which the data is to be written.)

 *  * @return                Returns one of the following error codes.
 *  * @retval EINVAL         An invalid argument was supplied
 *  * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *  *                        procedure was complete.
 *  * @retval ESUCCESS       The data and mask in the tcam at the given address is as expected.
 *  */
int
write_bfscam_value(BFSStateHeaderPtr header,uint32_t db ,uint32_t address, uint32_t data[13],uint32_t mask[13])
{
	int index = 0;
	volatile unsigned temp = 0;
	int attempts = 0;
	bool inactive_bank = false;
	bool bank = false; 
	
	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	if ( address > MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	if( db > 1)
		return EINVAL;

	if((db & 0x1) == 0)
		bank = false;
	else if((db & 0x1) == 1)
		bank = true;

	inactive_bank = get_inactive_bank(header);

	if(bank != inactive_bank)
	{
		dagutil_warning("trying to program to the active bank.will not be programmed.");
		return EINVAL;
	}
	dagutil_verbose_level(3,"write_bfscam_value  bptr %p database %d address %d \n",header->bfs_register_base,db,address);	
	/*Word index 1*/

	dagutil_verbose_level(3,"DATA 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],
data[10],data[11],data[12]);
	
	dagutil_verbose_level(3,"MASK 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",mask[0],mask[1],mask[2],mask[3],mask[4],mask[5],mask[6],mask[7],mask[8],mask[9],	mask[10],mask[11],mask[12]);
	
	printf("TCAM ADDRESS REGISTER VALUE %x \n",(address << 16) | (0x00000000));	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_ADDRESS_REGISTER) = ((address << 16) | (0x00000000));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (data[0] & 0x3fffffff );
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (mask[0] & 0x3fffffff );

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	
	index++;
	/*Word index 2*/
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[0] & new_base_msb_mask_2) >> 30 )|((data[1] &  lsb_mask_28 ) << 2 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[0] & new_base_msb_mask_2) >> 30 )|((mask[1] &  lsb_mask_28 ) << 2 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 3 */
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[1] & new_base_msb_mask_4) >> 28 )|((data[2] &  lsb_mask_26) << 4 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[1] & new_base_msb_mask_4) >> 28 )|((mask[2] &  lsb_mask_26) << 4 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;

	
	/*word index 4*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[2] & new_base_msb_mask_6)  >> 26 )|((data[3]  & lsb_mask_24)  << 6 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[2] & new_base_msb_mask_6)  >> 26 )|((mask[3]  & lsb_mask_24)  << 6 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
		
	/*word index 5*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[3] & new_base_msb_mask_8)  >> 24 )|((data[4] &  lsb_mask_22 << 8)));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[3] & new_base_msb_mask_8)  >> 24 )|((mask[4] &  lsb_mask_22 << 8)));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;

	
	
	/*word index 6*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[4] & new_base_msb_mask_10)  >> 22 ) | ((data[5] & lsb_mask_20 << 10)));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[4] & new_base_msb_mask_10)  >> 22 ) | ((mask[5] & lsb_mask_20 << 10)));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	
	
	/*word index 7*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[5] & new_base_msb_mask_12) >> 20 )|((data[6] & lsb_mask_18) <<  12 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[5] & new_base_msb_mask_12) >> 20 )|((mask[6] & lsb_mask_18) <<  12 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);

	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 8*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[6] & new_base_msb_mask_14) >> 18 )|((data[7] & lsb_mask_16) <<  14 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[6] & new_base_msb_mask_14) >> 18 )|((mask[7] & lsb_mask_16) <<  14 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 9*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[7] & new_base_msb_mask_16) >> 16 )|((data[8] & lsb_mask_14 ) <<  16 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[7] & new_base_msb_mask_16) >> 16 )|((mask[8] & lsb_mask_14 ) <<  16 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	
	/*word index 10*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[8] & new_base_msb_mask_18) >> 14)|((data[9] & lsb_mask_12)  <<  18 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[8] & new_base_msb_mask_18) >> 14)|((mask[9] & lsb_mask_12)  <<  18 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	
	
	/*word index 11*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[9] & new_base_msb_mask_20)>> 12) |((data[10] & lsb_mask_10 )  <<  20 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[9] & new_base_msb_mask_20)>> 12) |((mask[10] & lsb_mask_10 )  <<  20 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
		

	/*word index 12*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[10]  & new_base_msb_mask_22)  >> 10)|((data[11] & lsb_mask_8 )  <<  22));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[10]  & new_base_msb_mask_22)  >> 10)|((mask[11] & lsb_mask_8 )  <<  22));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;

	
	
	/*word index 13*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[11] & new_base_msb_mask_24) >> 8)|((data[12] & lsb_mask_6) <<   24));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[11] & new_base_msb_mask_24) >> 8)|((mask[12] & lsb_mask_6) <<   24));  

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while(((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	return ESUCCESS;
}
#if 0
/**
 *  * Writes an entry into the BFS cam 
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  * @param[in]  addr       The address (index) of the TCAM entry to write.
 *  *
 *  * @param[in]  data       Pointer to an array of bytes that gets loaded
 *  *                        into the data part of the entry.
 *  * @param[in]  mask       Pointer to an array of bytes that gets loaded
 *  *                        into the mask part of the entry.
 *  * @param[in]  db         The database (BANK into which the data is to be written.)

 *  * @return                Returns one of the following error codes.
 *  * @retval EINVAL         An invalid argument was supplied
 *  * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *  *                        procedure was complete.
 *  * @retval ESUCCESS       The data and mask in the tcam at the given address is as expected.
 *  */
int
write_bfscam_value(BFSStateHeaderPtr header,uint32_t db ,uint32_t address, uint32_t data[13],uint32_t mask[13])
{
	int index = 0;
	volatile unsigned temp = 0;
	int attempts = 0;
	bool inactive_bank = false;
	bool bank = false; 
	
	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	if ( address > MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	if( db > 1)
		return EINVAL;

	if((db & 0x1) == 0)
		bank = false;
	else if((db & 0x1) == 1)
		bank = true;

	inactive_bank = get_inactive_bank(header);

	if(bank != inactive_bank)
	{
		dagutil_warning("trying to program to the active bank.will not be programmed.");
		return EINVAL;
	}
	dagutil_verbose_level(3,"write_bfscam_value  bptr %p database %d address %d \n",header->bfs_register_base,db,address);	
	/*Word index 1*/

	dagutil_verbose_level(3,"DATA 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],
data[10],data[11],data[12]);
	
	dagutil_verbose_level(3,"MASK 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",mask[0],mask[1],mask[2],mask[3],mask[4],mask[5],mask[6],mask[7],mask[8],mask[9],	mask[10],mask[11],mask[12]);
	
	printf("TCAM ADDRESS REGISTER VALUE %x \n",(address << 16) | (0x00000000));	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_ADDRESS_REGISTER) = ((address << 16) | (0x00000000));

	printf("data 0 %x \n", (data[0] & 0x3fffffff));
	printf("mask 0 %x \n", (mask[0] & 0x3fffffff));
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[0];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[0];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	
	index++;
	/*Word index 2*/
	printf("data 1 %x \n",(((data[0] & new_base_msb_mask_2) >> 30 )|((data[1] &  lsb_mask_28 ) << 2 )));
	printf("mask 1 %x \n",(((mask[0] & new_base_msb_mask_2) >> 30 )|((mask[1] &  lsb_mask_28 ) << 2 )));
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[1];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[1];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 3 */
	printf("data 2 %x\n",(((data[1] & new_base_msb_mask_4) >> 28 )|((data[2] &  lsb_mask_26) << 4 )));
	printf("mask 2 %x\n",(((mask[1] & new_base_msb_mask_4) >> 28 )|((mask[2] &  lsb_mask_26) << 4 )));
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[2];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[2];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;

	printf("data 3 %x \n",(((data[2] & new_base_msb_mask_6)  >> 26 )|((data[3]  & lsb_mask_24)  << 6 )));
	printf("mask 3 %x \n",(((mask[2] & new_base_msb_mask_6)  >> 26 )|((mask[3]  & lsb_mask_24)  << 6 )));
	
	/*word index 4*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[3];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[3];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	printf("data 4 %x \n",((data[3] & new_base_msb_mask_8)  >> 24 )|((data[4] &  lsb_mask_22 << 8)));
	printf("mask 4 %x \n",((mask[3] & new_base_msb_mask_8)  >> 24 )|((mask[4] &  lsb_mask_22 << 8)));
		
	/*word index 5*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[4];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[4];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;

	
	printf("data 5 %x \n",((data[4] & new_base_msb_mask_10)  >> 22  ) | ((data[5] & lsb_mask_20 << 10)));
	printf("mask 5 %x \n",((mask[4] & new_base_msb_mask_8)   >> 24  ) | ((mask[5] &  lsb_mask_22 << 8)));
	
	/*word index 6*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[5];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[5];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	
	printf("data 6 %x \n",((data[5] & new_base_msb_mask_12) >> 20 )|((data[6] & lsb_mask_18) <<  12 ));
	printf("mask 6 %x \n",((mask[5] & new_base_msb_mask_12) >> 20 )|((mask[6] & lsb_mask_18) <<  12 ));

	/*word index 7*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[6];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[6];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);

	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	printf("data 7 %x \n",((data[6] & new_base_msb_mask_14) >> 18 )|((data[7] & lsb_mask_16) <<  14 ));
	printf("mask 7 %x \n",((mask[6] & new_base_msb_mask_14) >> 18 )|((mask[7] & lsb_mask_16) <<  14 ));

	
	/*word index 8*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[7];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[7];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	
	printf("data 8 %x \n",((data[7] & new_base_msb_mask_16) >> 16 )|((data[8] & lsb_mask_14 ) <<  16 ));
	printf("mask 8 %x \n",((mask[7] & new_base_msb_mask_16) >> 16 )|((mask[8] & lsb_mask_14 ) <<  16 ));
	
	/*word index 9*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[8];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[8];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	
	printf("data 9 %x \n",((data[8] & new_base_msb_mask_18) >> 14)|((data[9] & lsb_mask_12)  <<  18 ));
	printf("mask 9 %x \n",((mask[8] & new_base_msb_mask_18) >> 14)|((mask[9] & lsb_mask_12)  <<  18 ));	

	/*word index 10*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[9];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[9];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	
	printf("data 10 %x \n",((data[9] & new_base_msb_mask_20)>> 12) |((data[10] & lsb_mask_10 )  <<  20 ));
	printf("mask 10 %x \n",((mask[9] & new_base_msb_mask_20)>> 12) |((mask[10] & lsb_mask_10 )  <<  20 ));
	
	/*word index 11*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[10];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[10];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	printf("data 11 %x \n",((data[10]  & new_base_msb_mask_22)  >> 10)|((data[11] & lsb_mask_8 )  <<  22));
	printf("mask 11 %x \n",((mask[10]  & new_base_msb_mask_22)  >> 10)|((mask[11] & lsb_mask_8 )  <<  22));
		

	/*word index 12*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[11];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[11];

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;

	
	printf("data 12 %x \n", (((data[11] & new_base_msb_mask_24) >> 8)|((data[12] & lsb_mask_6) <<   24)));
	printf("mask 12 %x \n", (((mask[11] & new_base_msb_mask_24) >> 8)|((mask[12] & lsb_mask_6) <<   24)));
	
	/*word index 13*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = data[12];
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = mask[12];  

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while(((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	return ESUCCESS;
}
#endif
/**
 *  * Verifies if an entry in the TCAM at the given address is correct.
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  * @param[in]  addr       The address (index) of the TCAM entry to write.
 *  *
 *  * @param[in]  data       Pointer to an array of bytes that gets loaded
 *  *                        into the data part of the entry.
 *  * @param[in]  mask       Pointer to an array of bytes that gets loaded
 *  *                        into the mask part of the entry.
 *  * @param[in]  db         The database (BANK into which the data is to be written.)

 *  * @return                Returns one of the following error codes.
 *  * @retval EINVAL         An invalid argument was supplied
 *  * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *  *                        procedure was complete.
 *  * @retval ESUCCESS       The data and mask in the tcam at the given address is as expected.
 *  * @retval ENOMATCH       The data and mask in the tcam at the given address is not as expected.
 *  */
int
read_verify_bfscam_value(BFSStateHeaderPtr header,uint32_t db ,uint32_t address, uint32_t data[13],uint32_t mask[13])
{
	int index = 0;
	volatile unsigned temp = 0;
	int attempts = 0;
	//bool read_match = false;
	bool inactive_bank = false;
	bool bank = false; 
	
	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	if ( address > MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	if( db > 1)
		return EINVAL;


	if((db & 0x1) == 0)
		bank = false;
	else if((db & 0x1) == 1)
		bank = true;

	inactive_bank = get_inactive_bank(header);

	if(bank != inactive_bank)
	{
		dagutil_warning("trying to program to the active bank.will not be programmed.");
		return EINVAL;
	}
	dagutil_verbose_level(3,"write_bfscam_value  bptr %p database %d address %d \n",header->bfs_register_base,db,address);	
	/*Word index 1*/

	dagutil_verbose_level(3,"DATA 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],
	data[10],data[11],data[12]);
	
	dagutil_verbose_level(3,"MASK 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",mask[0],mask[1],mask[2],mask[3],mask[4],mask[5],mask[6],mask[7],mask[8],mask[9],	mask[10],mask[11],mask[12]);
	
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_ADDRESS_REGISTER) = (address << 16) | (0x00000000);

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (data[0] & 0x3fffffff);
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (mask[0] & 0x3fffffff);

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	
	index++;
	/*Word index 2*/
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[0] & new_base_msb_mask_2) >> 30 )|((data[1] &  lsb_mask_28 ) << 2 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[0] & new_base_msb_mask_2) >> 30 )|((mask[1] &  lsb_mask_28 ) << 2 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 3 */
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[1] & new_base_msb_mask_4) >> 28 )|((data[2] &  lsb_mask_26) << 4 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[1] & new_base_msb_mask_4) >> 28 )|((mask[2] &  lsb_mask_26) << 4 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 4*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[2] & new_base_msb_mask_6)  >> 26 )|((data[3]  & lsb_mask_24)  << 6 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[2] & new_base_msb_mask_6)  >> 26 )|((mask[3]  & lsb_mask_24)  << 6 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 5*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[3] & new_base_msb_mask_8)  >> 24 )|((data[4] &  lsb_mask_22 << 8)));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[3] & new_base_msb_mask_8)  >> 24 )|((mask[4] &  lsb_mask_22 << 8)));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 6*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[4] & new_base_msb_mask_10)  >> 22 ) | ((data[5] & lsb_mask_20 << 10)));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[4] & new_base_msb_mask_10)  >> 22 ) | ((data[5] & lsb_mask_20 << 10)));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 7*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[5] & new_base_msb_mask_12) >> 20 )|((data[6] & lsb_mask_18) <<  12 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[5] & new_base_msb_mask_12) >> 20 )|((mask[6] & lsb_mask_18) <<  12 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);

	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 8*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[6] & new_base_msb_mask_14) >> 18 )|((data[7] & lsb_mask_16) <<  14 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[6] & new_base_msb_mask_14) >> 18 )|((mask[7] & lsb_mask_16) <<  14 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 9*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[7] & new_base_msb_mask_16) >> 16 )|((data[8] & lsb_mask_14 ) <<  16 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[7] & new_base_msb_mask_16) >> 16 )|((mask[8] & lsb_mask_14 ) <<  16 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 10*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[8] & new_base_msb_mask_18) >> 14)|((data[9] & lsb_mask_12)  <<  18 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[8] & new_base_msb_mask_18) >> 14)|((mask[9] & lsb_mask_12)  <<  18 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 11*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[9] & new_base_msb_mask_20)>> 12) |((data[10] & lsb_mask_10 )  <<  20 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[9] & new_base_msb_mask_20)>> 12) |((mask[10] & lsb_mask_10 )  <<  20 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 12*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[10]  & new_base_msb_mask_22)  >> 10)|((mask[11] & lsb_mask_8 )  <<  22));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[10]  & new_base_msb_mask_22)  >> 10)|((mask[11] & lsb_mask_8 )  <<  22));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 13*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[11] & new_base_msb_mask_24) >> 8)|((data[12] & lsb_mask_6) <<   24));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[11] & new_base_msb_mask_24) >> 8)|((mask[12] & lsb_mask_6) <<   24));  

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT30) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}

	/*Check the Program Match Vector*/
	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	if((((temp & BIT26) >> 26) != 1) && (attempts < MAX_ATTEMPTS))
	{
		temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	
	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_MATCH_VECTOR);
	if(temp == address)
	{
		dagutil_verbose_level(0,"Tcam Program Match Vector Returned expected index %d\n",temp);
		return ESUCCESS;
	}
	else
	{
		dagutil_verbose_level(0,"Tcam Program Match Vector Returned unexpected index %d\n",temp);	
		return ENOMATCH;
	}
}
/**
 *  * Reads an entry from the BFS cam 
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  * @param[in]  addr       The address (index) of the TCAM entry to write.
 *  *
 *  * @param[out]  data       Pointer to an array of bytes that gets loaded
 *  *                        into the data part of the entry.
 *  * @param[out]  mask       Pointer to an array of bytes that gets loaded
 *  *                        into the mask part of the entry.
 *  * @param[in]   db        The database (BANK into which the data is to be written.)
 *  *
 *  * @return                Returns one of the following error codes.
 *  * @retval EINVAL         An invalid argument was supplied
 *  * @retval ETIMEDOUT      Timedout waiting for the chip to indicate the reading
 *  *                        procedure was complete.
 *  * @retval ESUCCESS       The data and mask in the tcam at the given address
 *  */
int
read_bfscam_value(BFSStateHeaderPtr header, uint32_t db,uint32_t address, uint32_t data[13],uint32_t mask[13])
{
	int index = 0;
	volatile unsigned temp = 0;
	int attempts = 0;
	bool inactive_bank = false;
	bool bank = false; 
	
	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	if ( address > MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( data == NULL )
		return EINVAL;
	if ( mask == NULL )
		return EINVAL;
	if( db > 1)
		return EINVAL;


	if((db & 0x1) == 0)
		bank = false;
	else if((db & 0x1) == 1)
		bank = true;

	inactive_bank = get_inactive_bank(header);

	if(bank != inactive_bank)
	{
		dagutil_warning("trying to program to the active bank.will not be programmed.");
		return EINVAL;
	}
	dagutil_verbose_level(3,"write_bfscam_value  bptr %p database %d address %d \n",header->bfs_register_base,db,address);	
	/*Word index 1*/

	dagutil_verbose_level(3,"DATA 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],
data[10],data[11],data[12]);
	
	dagutil_verbose_level(3,"MASK 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x \n",mask[0],mask[1],mask[2],mask[3],mask[4],mask[5],mask[6],mask[7],mask[8],mask[9],	mask[10],mask[11],mask[12]);
	
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_ADDRESS_REGISTER) = (address << 16) | (0x00000000);

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (data[0] & 0x3fffffff);
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (mask[0] & 0x3fffffff);

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	/*Word index 2*/
	
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[0] & new_base_msb_mask_2) >> 30 )|((data[1] &  lsb_mask_28 ) << 2 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[0] & new_base_msb_mask_2) >> 30 )|((mask[1] &  lsb_mask_28 ) << 2 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 3 */
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[1] & new_base_msb_mask_4) >> 28 )|((data[2] &  lsb_mask_26) << 4 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[1] & new_base_msb_mask_4) >> 28 )|((mask[2] &  lsb_mask_26) << 4 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 4*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[2] & new_base_msb_mask_6)  >> 26 )|((data[3]  & lsb_mask_24)  << 6 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[2] & new_base_msb_mask_6)  >> 26 )|((mask[3]  & lsb_mask_24)  << 6 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 5*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[3] & new_base_msb_mask_8)  >> 24 )|((data[4] &  lsb_mask_22 << 8)));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[3] & new_base_msb_mask_8)  >> 24 )|((mask[4] &  lsb_mask_22 << 8)));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 6*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[4] & new_base_msb_mask_10)  >> 22 ) | ((data[5] & lsb_mask_20 << 10)));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[4] & new_base_msb_mask_10)  >> 22 ) | ((data[5] & lsb_mask_20 << 10)));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 7*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[5] & new_base_msb_mask_12) >> 20 )|((data[6] & lsb_mask_18) <<  12 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[5] & new_base_msb_mask_12) >> 20 )|((mask[6] & lsb_mask_18) <<  12 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);

	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 8*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[6] & new_base_msb_mask_14) >> 18 )|((data[7] & lsb_mask_16) <<  14 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[6] & new_base_msb_mask_14) >> 18 )|((mask[7] & lsb_mask_16) <<  14 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 9*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[7] & new_base_msb_mask_16) >> 16 )|((data[8] & lsb_mask_14 ) <<  16 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[7] & new_base_msb_mask_16) >> 16 )|((mask[8] & lsb_mask_14 ) <<  16 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 10*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[8] & new_base_msb_mask_18) >> 14)|((data[9] & lsb_mask_12)  <<  18 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[8] & new_base_msb_mask_18) >> 14)|((mask[9] & lsb_mask_12)  <<  18 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0) != 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 11*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) =(((data[9] & new_base_msb_mask_20)>> 12) |((data[10] & lsb_mask_10 )  <<  20 ));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) =(((mask[9] & new_base_msb_mask_20)>> 12) |((mask[10] & lsb_mask_10 )  <<  20 ));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 12*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[10]  & new_base_msb_mask_22)  >> 10)|((mask[11] & lsb_mask_8 )  <<  22));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[10]  & new_base_msb_mask_22)  >> 10)|((mask[11] & lsb_mask_8 )  <<  22));

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	index++;
	
	/*word index 13*/
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_DATA_IN) = (((data[11] & new_base_msb_mask_24) >> 8)|((data[12] & lsb_mask_6) <<   24));
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_MASK_IN) = (((mask[11] & new_base_msb_mask_24) >> 8)|((mask[12] & lsb_mask_6) <<   24));  

	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = ((BIT31) | (bank << 29) | (index << 1));

	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
	while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
	{
		temp = 	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
		attempts++;
	}
	if(attempts == MAX_ATTEMPTS)
	{
		dagutil_warning("timeout in %s\n", __FUNCTION__);
		return ETIMEDOUT;
	}
	return ESUCCESS;
}

/**
 *  * Swaps the active BANK.If BANK 0 is active activates BANK 1 and vice-versa
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  */
void swap_active_bank(BFSStateHeaderPtr header)
{
	uint32_t temp = 0;
	temp =  *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_CONFIG_REG);
	if((temp & BIT28) == 0)/*bank 0 is active*/
	{
		
		temp = 0xc00d0020;
	}
	else
	{	
		temp = 0x400d0020;
	}
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_CONFIG_REG) = temp;
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_CONFIG_REG) = 0x00000000;
}
/**
 *  * Reads an entry from the BFS cam 
 *  * @param[in]  header    The structure which contains the F/W interface
 *  * 
 *  * @return               Returns the active BANK.
 *  */
bool get_inactive_bank(BFSStateHeaderPtr header)
{
	
	uint32_t attempts = 0;
	
	uint32_t temp =  *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_CONFIG_REG);
	
	while((((temp & BIT29) >> 29)!= 0 ) && (attempts < 50))
	{
		temp =  *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_CONFIG_REG);
		attempts ++;
	}
	if(attempts == 50)
	{
		dagutil_panic("Not able to read the bank status");
	}
	
	if((temp & BIT28) == 0)
	{
		dagutil_verbose_level(2,"BANK 0 is active \n");
		return 1;
	}
	else
	{	
		dagutil_verbose_level(2,"BANK 1 is active \n");
		return 0;
	}
}
/**
 *  * Writes an entry into the BFS cam 
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  * @param[in]  addr       The address (index) of the TCAM entry to write.
 *  *
 *  * @param[in]  colour     The TCAM colour to be written to SRAM
 *  *                        into the data part of the entry.
 *  * @param[in]  db         The database (BANK into which the data is to be written.)

 *  * @return                Returns one of the following error co
 *  * @retval ESUCCESS       The data and mask in the tcam at the given address is as expected.
 *  */
int 
bfs_write_sram_entry(BFSStateHeaderPtr header,uint32_t db ,uint32_t address,uint32_t colour)
{
	bool bank = false;

	if( (db & 0x1) == 0)
		bank = false;
	else if ((db & 0x1) == 1)
		bank = true;
		
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_SRAM_INTERFACE)  = ((BIT31)| (bank << 29) |((address & 0x1fff) << 16) |(colour & 0xffff));

	return ESUCCESS;
}
/**
 *  * Writes an entry into the BFS SRAM 
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  * @param[in]  addr       The address (index) of the TCAM entry to write.
 *  *
 *  * @param[in]  db         The database (BANK into which the data is to be written.)
 *  *
 *  * @param[out]  colour    The TCAM colour to be read from SRAM
 *  *
 *  * @return                Returns one of the following error co
 *  * @retval ESUCCESS       The data and mask in the tcam at the given address is as expected.
 *  */
int 
bfs_read_sram_entry(BFSStateHeaderPtr header,uint32_t db ,uint32_t address,uint32_t *colour)
{
	bool bank = false;
	uint32_t temp = 0;

	if( (db & 0x1) == 0)
		bank = false;
	else if ((db & 0x1) == 1)
		bank = true;
		
	*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_SRAM_INTERFACE)  = ((BIT30)| (bank << 29) |((address & 0x1fff) << 16));
	
	temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_SRAM_INTERFACE);

	*colour = temp;
	
	dagutil_verbose_level(2,"colour 0x%04x\n",*colour);

	return ESUCCESS;
}
/**
 *  * Returns the lookup width of the TCAM
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  *
 *  * @return                Returns one of the following error codes
 *  * @retval 				 lookup_width
 *  */

int bfs_get_lookup_width(BFSStateHeaderPtr header)
{
	uint32_t lookup_width,value = 0;

	value = *(volatile unsigned int*)((uint8_t*)header->ppf_register_base + 0x4);

	lookup_width = ((value & 0xffff0000) >> 16);

	return lookup_width;
}

/**
 *  * Writes the SRAM miss value.
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  *
 *  * @return                Returns one of the following error codes
 *  * @retval 				 void       
 *  */

void bfs_set_sram_miss_value(BFSStateHeaderPtr header,uint32_t sram_miss_value)
{
	*(volatile unsigned int*)((uint8_t*)header->ppf_register_base + 0x1C) = sram_miss_value;
	return; 
}

/**
 *  * Initialises the TCAM.
 *  *
 *  * @param[in]  header     The structure which contains the F/W interface
 *  * 
 *  *
 *  * @return                Returns one of the following error codes
 *  * @retval ESUCCESS       Initialization fo the TCAM successful.
 *  * @retval EINVAL         Argument to the function is invalid.
 *  */

int bfs_cam_initialise(BFSStateHeaderPtr header)
{
	bool inactive_bank = false;
	uint32_t data = 0;
	uint32_t attempts,temp = 0;
	int loop = 0;
	/* Sanity checking */
	if ( header == NULL )
		return EINVAL;
	
	inactive_bank = get_inactive_bank(header);
	dagutil_verbose_level(3,"write_bfscam_value  bptr %p \n",header->bfs_register_base);	
	
	if(inactive_bank)
	{
		data = ((BIT27) | (1 << 29));
	}else
	{
		data = ((BIT27) | (0 << 29));
	}

	for (loop = 0; loop < MAX_BFS_POSSIBLE_RULES; loop++)
	{
		*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_ADDRESS_REGISTER) = (loop & 0xffff);
		*(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE) = data;

		attempts = 0;
		temp = *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
        while (((temp & BIT0)!= 1) && (attempts <  MAX_ATTEMPTS))
        {
             temp =  *(volatile unsigned int*)((uint8_t*)header->bfs_register_base + TCAM_PROGRAM_INTERFACE);
             dagutil_microsleep(10);
             attempts++;
        }
        if(attempts == MAX_ATTEMPTS)
        {
              dagutil_warning("timeout in %s\n", __FUNCTION__);
              return ETIMEDOUT;
        }	
	}
	return ESUCCESS;
}

