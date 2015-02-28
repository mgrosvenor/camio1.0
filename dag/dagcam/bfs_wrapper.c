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
#include "bfs_lib.h"
int
bfscam_write_cam_389_entry(BFSStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask)
{
	uint32_t address;
	int err = 0;

	/* Sanity check the input */
	if ( index >= MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( db_num > 8 )
		return EINVAL;
	
	address = index;

	err = write_bfscam_value(header,db_num,address,(uint32_t*)data,(uint32_t*)mask);
	if(err != 0) return err;

	return 0;
}
int
bfscam_write_cam_389_entry_ex(BFSStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask,uint32_t ad_value)
{
	uint32_t address;
	int err = 0;
	
	/* Sanity check the input */
	if ( index >= MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( db_num > 8 )
		return EINVAL;

	address = index;
	
	err = write_bfscam_value(header,db_num,address,(uint32_t*)data,(uint32_t*)mask);
	if(err != 0) return err;

	
	err = bfs_write_sram_entry(header,db_num,address,ad_value);
	if(err != 0) return err;

	return 0;
}
int
bfscam_read_verify_cam_entry_ex(BFSStateHeaderPtr header,uint32_t db_num ,uint32_t index, uint8_t *data,uint8_t * mask,uint32_t *ad_value)
{
	int err = 0;
	uint32_t address = 0;
	uint32_t colour = 0;
	/* Sanity check the input */
	if ( index >= MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( db_num > 8 )
		return EINVAL;
	
	address = index;

	err = read_verify_bfscam_value(header,db_num,address,(uint32_t*)data,(uint32_t*)mask);
	if(err != 0)
		return err;


	/*Read the sram value.*/
	err = bfs_read_sram_entry(header,db_num,address,&colour);
	if(err != 0)
		return err;

	if(colour == *ad_value)
	{
		return 0;
	}
	else 
	{
		return -1;
	}

}

int bfscam_read_cam_389_entry_ex(BFSStateHeaderPtr header,uint32_t db_num, uint32_t index, uint8_t *data, uint8_t *mask, uint32_t *ad_value)
{
	int err = 0;
	uint32_t address = 0;
	/* Sanity check the input */
	if ( index >= MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( db_num > 8 )
		return EINVAL;
	
	address = index;

	err = read_bfscam_value(header,db_num,address,(uint32_t*)data,(uint32_t*)mask);
	if(err != 0) return err;

	
	/*Read the sram value.*/
	err = bfs_read_sram_entry(header,db_num,address,ad_value);
	if(err != 0)
		return err;

	return 0;
}
int bfscam_read_cam_389_entry(BFSStateHeaderPtr header,uint32_t db_num, uint32_t index, uint8_t *data, uint8_t *mask)
{
	int err = 0;
	uint32_t address = 0;
	/* Sanity check the input */
	if ( index >= MAX_BFS_POSSIBLE_RULES)
		return EINVAL;
	if ( db_num > 8 )
		return EINVAL;
	
	address = index;
		
	err = read_bfscam_value(header,db_num,address,(uint32_t*)data,(uint32_t*)mask);
	if(err != 0) return err;

	return 0;
}
