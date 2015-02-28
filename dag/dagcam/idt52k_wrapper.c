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
#include "idt52k_lib.h"

int
tcam_write_cam_144_entry(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask)
{
	uint32_t address;
	uint8_t wr_data[18];
	uint8_t wr_mask[18];
	int err,i;
	
	uint32_t partitions = (header->interface_number * header->rule_set);
	uint32_t sector  = ((db_num * 16)/partitions); 
	
	//in the below layer db_num = sector;
	db_num = sector;
	
	for(i = 0;i < 18;i++)
	{
		wr_data[i] = data[i];
		wr_mask[i] = mask[i];
	}

	address = (index << 1);

	err = idt52k_write_cam_entry(header,db_num,address,data,mask);
	if(err != 0) return err;

	address += 1;
	
	err = idt52k_write_cam_entry(header,db_num,address,&wr_data[9],&wr_mask[9]);
	if(err != 0) return err;

	return 0;
}
int 
tcam_write_cam_576_entry(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask)
{
	uint32_t address;
	uint8_t wr_data[72];
	uint8_t wr_mask[72];
	int err,i;

	
	uint32_t partitions = (header->interface_number * header->rule_set);
	
	uint32_t sector  = ((db_num * 16)/partitions); 

	//in the below layer db_num = sector;
	db_num = sector;

	for(i = 0;i < 72;i++)
	{
		wr_data[i] = data[i];
		wr_mask[i] = mask[i];
	}

	address = (index << 3);

	for(i = 0; i < 8;i ++)
	{
		err = idt52k_write_cam_entry(header,db_num,address,&data[9*i],&mask[9*i]);
		if(err != 0) return err;
		address ++;
	}
	return 0;
}
int tcam_read_cam_144_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t index,uint8_t *data, uint8_t *mask)
{
	uint32_t address;
	uint8_t rd_data[72];
	uint8_t rd_mask[72];
	int err,i;

	
	uint32_t partitions = (header->interface_number * header->rule_set);
	
	uint32_t sector  = ((db_num * 16)/ partitions); 

	//in the below layer db_num = sector;
	db_num = sector;
	
    address = (index << 1);
	
	
    err = idt52k_read_cam_entry(header,db_num,address,rd_data,rd_mask);
	if(err != 0)  return err;
	
	address++;

	err = idt52k_read_cam_entry(header,db_num,address,&rd_data[9],&rd_mask[9]);
	if(err != 0)  return err;

	for(i = 0;i < 18;i++)
	{
		data[i] = rd_data[i];
		mask[i] = rd_mask[i];	
	}
	return 0;
}
int tcam_read_cam_576_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t index,uint8_t *data, uint8_t *mask)
{
	uint32_t address;
	uint8_t rd_data[72];
	uint8_t rd_mask[72];
	int err,i;
		
	
	uint32_t partitions = (header->interface_number * header->rule_set);
	
	uint32_t sector  = ((db_num * 16)/ partitions); 

	//in the below layer db_num = sector;

	db_num = sector;
	
	address = (index << 3);

	for(i = 0; i < 8;i++)
	{
		err = idt52k_read_cam_entry(header,db_num,address,&rd_data[9*i],&rd_mask[9*i]);
		if(err != 0)  return err;
		address++;
	}

	for(i = 0;i < 72;i++)
	{
		data[i] = rd_data[i];
		mask[i] = rd_mask[i];	
	}
	return 0;
}
int 
tcam_lookup_144_entry(FilterStateHeaderPtr header,uint32_t db, uint8_t value[72], uint32_t gmr, bool *hit, uint32_t *index, uint32_t *tag)
{
	
	int err;
	uint32_t lookup_width = 0;
	uint32_t lookup[18];

	if ( header == NULL)
		return EINVAL;	
	if ( db > 7 )
		return EINVAL;
	if ( value == NULL )
		return EINVAL;
	if ( gmr > 15 )
		return EINVAL;


	/* clear the arguments */
	*hit = false;
	*index = 0;
	*tag = 0;

	lookup_width = 144;

	lookup[0] = ((uint32_t)value[0]  << 0) | ((uint32_t)value[1]  << 8) | ((uint32_t)value[2]  << 16)  | ((uint32_t)value[3]  << 24);
	lookup[1] = ((uint32_t)value[4]  << 0) | ((uint32_t)value[5]  << 8) | ((uint32_t)value[6]  << 16)  | ((uint32_t)value[7]  << 24);
	lookup[2] = ((uint32_t)value[8]  << 0) | ((uint32_t)value[9]  << 8) | ((uint32_t)value[10] << 16)  | ((uint32_t)value[11] << 24);
	lookup[3] = ((uint32_t)value[12] << 0) | ((uint32_t)value[13] << 8) | ((uint32_t)value[14] << 16)  | ((uint32_t)value[15] << 24);
	lookup[4] = ((uint32_t)value[16] << 0) | ((uint32_t)value[17] << 8);
	
	printf("lookup %x %x %x %x %x \n",lookup[0],lookup[1],lookup[2],lookup[3],lookup[4]);

	/* request lookup operation */
	err = idt52k_cam_lookup_entry(header,db,lookup,lookup_width,gmr,hit,index,tag);
	if (err != 0) return EINVAL;
	
	return 0;
}
int 
tcam_lookup_576_entry(FilterStateHeaderPtr header,uint32_t db, uint8_t value[72], uint32_t gmr, bool *hit, uint32_t *index, uint32_t *tag)
{
	
	int err;
	uint32_t lookup_width = 0;
	uint32_t lookup[18];
	uint32_t i,j =0;

	if ( header == NULL)
		return EINVAL;	
	if ( db > 7 )
		return EINVAL;
	if ( value == NULL )
		return EINVAL;
	if ( gmr > 15 )
		return EINVAL;

	/* clear the arguments */
	*hit = false;
	*index = 0;
	*tag = 0;

	lookup_width = 576;
	
	for(i = 0,j =0;j < 80;i+=5,j+=20)
	{
		lookup[i + 0] = ((uint32_t)value[j+0]  << 0)  | ((uint32_t)value[j+1]  << 8)  | ((uint32_t)value[j+2]  << 16) | ((uint32_t)value[j+3]  << 24);
		lookup[i + 1] = ((uint32_t)value[j+4]  << 0)  | ((uint32_t)value[j+5]  << 8)  | ((uint32_t)value[j+6]  << 16) | ((uint32_t)value[j+7]  << 24);
		lookup[i + 2] = ((uint32_t)value[j+8]  << 0)  | ((uint32_t)value[j+9]  << 8)  | ((uint32_t)value[j+10] << 16) | ((uint32_t)value[j+11] << 24);
		lookup[i + 3] = ((uint32_t)value[j+12] << 0)  | ((uint32_t)value[j+13] << 8)  | ((uint32_t)value[j+14] << 16) | ((uint32_t)value[j+15] << 24);
		lookup[i + 4] = ((uint32_t)value[j+16] << 0)  | ((uint32_t)value[j+17] << 8)  | ((uint32_t)value[j+18] << 16) | ((uint32_t)value[j+19] << 24);
	}
	/* request lookup operation */
	err = idt52k_cam_lookup_entry(header,db,lookup,lookup_width,gmr,hit,index,tag);
	if (err != 0) return EINVAL;
	
	
	return 0;
}
int
tcam_set_144_gmr_register(FilterStateHeaderPtr header,uint32_t index,uint8_t mask[18])
{
	uint32_t address = (GMR00 + 8 + index);
	uint32_t width = 2;
	idt52k_write_gmr_entry(header,address,mask,width);
	return 0;
}
int
tcam_set_576_gmr_register(FilterStateHeaderPtr header,uint32_t index,uint8_t mask[72])
{
	uint32_t address = (GMR24); /*ONLY ONE SET AVALIABLE FOR 576 BIT MASK*/
	uint32_t width = 8;
	idt52k_write_gmr_entry(header,address,mask,width);

	return 0;
}
int
tcam_write_cam_144_entry_ex(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask,uint32_t ad_value)
{
	uint32_t address;
	uint8_t wr_data[18];
	uint8_t wr_mask[18];
	int err,i;
	
	uint32_t partitions = (header->interface_number * header->rule_set);
	uint32_t sector  = ((db_num * 16)/partitions); 
	//in the below layer db_num = sector;
	db_num = sector;
	

	for(i = 0;i < 18;i++)
	{
		wr_data[i] = data[i];
		wr_mask[i] = mask[i];
	}

	address = (index << 1);

	err = idt52k_write_cam_entry(header,db_num,address,data,mask);
	if(err != 0) return err;

	/*Write the SRAM VALUE*/

	err = idt52k_write_sram_entry(header,db_num,address,ad_value);
	if(err != 0) return err;
	
	/*Upper bits*/
	address += 1;
	
	err = idt52k_write_cam_entry(header,db_num,address,&wr_data[9],&wr_mask[9]);
	if(err != 0) return err;

	return 0;
}
int 
tcam_write_cam_576_entry_ex(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask,uint32_t ad_value)
{
	uint32_t address;
	uint8_t wr_data[72];
	uint8_t wr_mask[72];
	int err,i;

	uint32_t partitions = (header->interface_number * header->rule_set);
	uint32_t sector  = ((db_num  * 16)/ partitions); 
	//in the below layer db_num = sector;
	db_num = sector;
	
	for(i = 0;i < 72;i++)
	{
		wr_data[i] = data[i];
		wr_mask[i] = mask[i];
	}

	address = (index << 3);
	
	err = idt52k_write_sram_entry(header,db_num,address,ad_value);
	if(err != 0) return err;


	for(i = 0; i < 8;i ++)
	{
		err = idt52k_write_cam_entry(header,db_num,address,&data[9*i],&mask[9*i]);
		if(err != 0) return err;
		address ++;
	}
	return 0;
}
int tcam_read_cam_144_entry_ex(FilterStateHeaderPtr header,uint32_t db_num,uint32_t index,uint8_t *data, uint8_t *mask,uint32_t* ad_value)
{
	uint32_t address;
	uint8_t rd_data[72];
	uint8_t rd_mask[72];
	int err,i;

	uint32_t partitions = (header->interface_number * header->rule_set);
	uint32_t sector  = ((db_num * 16 )/ partitions); 
	//in the below layer db_num = sector;
	db_num = sector;
	
	address = (index << 1);

	err = idt52k_read_cam_entry(header,db_num,address,rd_data,rd_mask);
	if(err != 0)  return err;
	
	err = idt52k_read_sram_entry(header,db_num,address,ad_value);
	if(err != 0) return err;
	
	address++;

	err = idt52k_read_cam_entry(header,db_num,address,&rd_data[9],&rd_mask[9]);
	if(err != 0)  return err;

	for(i = 0;i < 18;i++)
	{
		data[i] = rd_data[i];
		mask[i] = rd_mask[i];	
	}
	return 0;
}
int tcam_read_cam_576_entry_ex(FilterStateHeaderPtr header,uint32_t db_num,uint32_t index,uint8_t *data, uint8_t *mask,uint32_t *ad_value)
{
	uint32_t address;
	uint8_t rd_data[72];
	uint8_t rd_mask[72];
	int err,i;

	uint32_t partitions = (header->interface_number * header->rule_set);
	uint32_t sector  = ((db_num * 16 )/ partitions); 
	//in the below layer db_num = sector;
	db_num = sector;
	
	address = (index << 3);

	err = idt52k_read_sram_entry(header,db_num,address,ad_value);
	if(err != 0) return err;
	
	for(i = 0; i < 8;i++)
	{
		err = idt52k_read_cam_entry(header,db_num,address,&rd_data[9*i],&rd_mask[9*i]);
		if(err != 0)  return err;
		address++;
	}

	for(i = 0;i < 72;i++)
	{
		data[i] = rd_data[i];
		mask[i] = rd_mask[i];	
	}
	return 0;
}


