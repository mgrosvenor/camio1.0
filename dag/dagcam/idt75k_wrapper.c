/* Endace headers. */
#include "dagapi.h"
#include "dag_platform.h"
#include "dagreg.h"
#include "dagutil.h"
#include "dagclarg.h"

/* Endace cam headers */
#include "dagcam/idt75k_interf.h"
#include "dagcam/idt75k_lib.h"

#if HAVE_EDITLINE
#include <editline/readline.h>
#include <histedit.h>
#endif /* HAVE_EDITLINE */
#define DRB_LA_VERSION
#define DRB_VERSION 
#define SIZE_128K 128 * 1024


int
tcam_set_global_mask_reg(idt75k_dev_t * dev_p, uint32_t index, const uint8_t mask[18])
{
	idt75k_write_gmr_t   gmr_upper;
	idt75k_write_gmr_t   gmr_lower;
	int i,err;
	
	/* check to ensure the driver is loaded */
	if ( index > 31 )
		return EINVAL;


	/* construct the arguments for the request */
	gmr_upper.address = (index << 1);
	for (i=0; i<9; i++)
		gmr_upper.value[i] = mask[i];

	gmr_lower.address = ((index << 1) + 1);
	for (i=0; i<9; i++)
		gmr_lower.value[i] = mask[i + 9];


	/* set the upper 72 bits of the GMR */
	if( (err = idt75k_write_gmr(dev_p, gmr_upper.address, gmr_upper.value)) != 0 )
		    return -1;
	
	
	/* set the upper 72 bits of the GMR */
	if( (err = idt75k_write_gmr(dev_p, gmr_lower.address, gmr_lower.value)) != 0 ) 
		    return -1;

	return 0;
}

int tcam_initialise(idt75k_dev_t *dev_p_la1,idt75k_dev_t *dev_p_la2)
{
	uint32_t             wr_nse_conf;
	uint32_t             rd_nse_conf;
	idt75k_set_dbconf_t  db_set;
	uint8_t              gmr_values[18];
	int err,i;
	uint32_t db,gmr_index;
	uint32_t segments = 0;
	
	/* configure the global config register to match the settings for infiniband */
	wr_nse_conf = (1 << 18) | (1 << 14) | (1 << 13) | (1 << 9) | (1 << 8) | (1 << 3);
	/*Added initialization for the second interface 
	requiremnts first a propper FW to be loadded and then to init the TCAM 
	other wise the TCAM will be initialized only with one LA1-0 interface */
	if(dev_p_la2->mmio_ptr != NULL )
	{
		dagutil_verbose("SECOND LA1 Interface presented LA1-1 %p\tLA1-0: %p\n", dev_p_la2->mmio_ptr, dev_p_la1->mmio_ptr);
	} else {
		dagutil_verbose("SECOND Interface not presented. LA1-1 %p\tLA1-0: %p\n", dev_p_la2->mmio_ptr, dev_p_la1->mmio_ptr);
	}
//CHANGE ME: TO USE THE NSE_CONFIG function from the library:	OPTIONAL
	if( ( err = idt75k_write_csr(dev_p_la1, NSE_CONF_ADDR, wr_nse_conf)) != 0 )
		    return err;
//ZBT steup and hold time in the middle to fix HW problems 		    
	if( ( err = idt75k_write_csr(dev_p_la1, 0x27, 0x7)) != 0 )
		                    return err;

	/* read back the register and confirm the write took place */
	if( ( err = idt75k_read_csr(dev_p_la1, NSE_CONF_ADDR, &rd_nse_conf)) != 0 )
		    return err;
	
	if ( rd_nse_conf != wr_nse_conf )
		return EIO;



	/* put default values in all the databases and clear the sectors */
	memset( &db_set, 0, sizeof(idt75k_set_dbconf_t) );
	
	db_set.mask = IDT75K_DBCONF_POWERSAVE_MASK
	            | IDT75K_DBCONF_AR_ONLY_MASK
	            | IDT75K_DBCONF_AGE_ENABLE_MASK
	            | IDT75K_DBCONF_AD_MASK
	            | IDT75K_DBCONF_AFR_MASK
	            | IDT75K_DBCONF_AFS_MASK
	            | IDT75K_DBCONF_RTN_MASK
	            | IDT75K_DBCONF_ACTIVITY_ENABLE_MASK
	            | IDT75K_DBCONF_CORE_SIZE_MASK
	            | IDT75K_DBCONF_AGE_COUNT_MASK
	            | IDT75K_DBCONF_SEGMENT_SEL_MASK
	            | IDT75K_DBCONF_DB_SIZE_MASK;

	db_set.flags = IDT75K_DBCONF_AR_ONLY
	            | IDT75K_DBCONF_AD_32
	            | IDT75K_DBCONF_CORE_144
	            | IDT75K_DBCONF_DB_18SEL_SIZE_144;

	db_set.age_cnt = 0;
	db_set.segments = 0;

	for (db=0; db<16; db++)
	{
		db_set.db_num = db;
		if( (err = idt75k_set_dbconf(dev_p_la1, db_set.db_num, db_set.mask, db_set.flags, db_set.age_cnt, db_set.segments)) != 0 )
			return err;
	}


	/* set the GMRs to their default state for 144 or 128  bit lookups */
	for (i=0; i<16; i++)
		gmr_values[i] = 0xFF;
#if 1	
	gmr_values[16] = 0xFF;
	gmr_values[17] = 0xFF;
#else 	
	gmr_values[16] = 0x00;
	gmr_values[17] = 0x00;
#endif 	
	

	for (gmr_index=0; gmr_index<16; gmr_index++)
	{
		tcam_set_global_mask_reg(dev_p_la1, gmr_index, gmr_values);
	}
#if 1
	segments = 0xf;
	for (i=0;i<8;i++)
	{
		tcam_set_database_sectors(dev_p_la1, i,segments);
		segments = (segments << 4);
	}
#endif
	return 0;
}
int tcam_set_database_sectors(idt75k_dev_t * dev_p, uint32_t db, uint32_t sectors)
{
	idt75k_set_dbconf_t  db_set;

	/* check to ensure the driver is loaded */
	if ( db > 15 )
		return EINVAL;


	/* set the arguments and send the IOCTL */
	memset( &db_set, 0, sizeof(idt75k_set_dbconf_t) );
	db_set.db_num = db;
	db_set.mask = IDT75K_DBCONF_SEGMENT_SEL_MASK;
	db_set.segments = sectors;

	return  idt75k_set_dbconf(dev_p, db_set.db_num , db_set.mask, db_set.flags, db_set.age_cnt, db_set.segments);
}
/**
 * Writes an entry into the TCAM at the given address. This functions will not change the state 
 * if the state is true. But if the state is false will invalidate the entry. 
 * This functuion assumes that it is a 144 bit DB configured
 *
 * @param[in]  db      The database that contains the entry, needed by the
 *                     hardware to determine the size of the entry.
 * @param[in]  index   The index number of the entry to write in 144 bit address
 * @param[in]  mask    pointer to mask to be written into the TCAM.
 * @param[in]  value   pointer to value(data) to be written into the TCAM core.
 * @param[in]  tag     32 bit value to be written to Associated SRAM
 * @param[in]  state  boolean value to validate or invalidate the entry 
 * 
 *
 * @returns             Returns an error code that indicates either
 *                      success or failure.
 * @retval ESUCCESS     The operation succeeded.
 * @retval EBADF        The driver is not opened, call open first.
 * @retval EIO          Some physical I/O error has occurred
 *
 * @sa tcam_read_cam_entry | tcam_write_cam_entry
 */


int tcam_write_cam_entry(idt75k_dev_t *dev_p,uint32_t db, uint32_t index, uint8_t *value, uint8_t *mask, uint32_t tag, int state )
{
	idt75k_clear_valid_t  clr_valid;
	int err,i;
#if defined(USE_DUAL_WRITE)
	idt75k_write_entry_ex_t   wr_entry;
#else
	idt75k_write_entry_t  wr_entry;
	idt75k_write_sram_t   wr_sram;

#endif


	/* Sanity check the input */
	if ( index >= (128 * 1024) )
		return EINVAL;
	if ( db > 15 )
		return EINVAL;

#if defined(USE_DUAL_WRITE)

	memset (&wr_entry, 0x00, sizeof(idt75k_write_entry_ex_t));


	wr_entry.db_num = db;
	wr_entry.address = (index << 2);
	wr_entry.validate = state;

	for (int i=0; i<18; i++)
	{
		wr_entry.mask[i] = mask[i];
		wr_entry.data[i] = value[i];
	}

	wr_entry.ad_value[0] = tag;
	wr_entry.gmask = 31;

	dagutil_verbous("loading TCAM entry into db:%d, addr:0x%04X", db, index);
	err = idt75k_write_cam_entry_ex(dev_p, wr_entry.db_num, wr_entry.address, wr_entry.data, wr_entry.mask, wr_entry.gmask, wr_entry.ad_value);
	if ( err != 0 )    return -err;

#else     /* !defined(USE_DUAL_WRITE) */



	/* write the cam mask and data entries */
	memset (&wr_entry, 0x00, sizeof(idt75k_write_entry_t));

	wr_entry.db_num = db;
	wr_entry.address = (index << 2);
	wr_entry.validate = state ? 1 : 0;
	wr_entry.gmask = 31;

	for (i=0; i<18; i++)
	{
		wr_entry.mask[i] = mask[i];
		wr_entry.data[i] = value[i];
	}
	
	err = idt75k_write_cam_entry(dev_p, wr_entry.db_num, wr_entry.address, wr_entry.data, wr_entry.mask, wr_entry.gmask, wr_entry.validate);
	if ( err != 0 )    return -err;


	/* write the assoicated data (32-bits) from the entry */
	memset (&wr_sram, 0x00, sizeof(wr_sram));
	wr_sram.address = (index << 2);
	wr_sram.db_num = db;
	/*We write first 00 to the ASRAM to cope with HW problems and then the real value 
	as weel it is required or recommended to set bits [0:3] oof CSR 0x27 to 7 to improve the ZBT interface  */
	wr_sram.sram_entry[0] = 0x00000000;
	err = idt75k_write_sram(dev_p, wr_sram.db_num, wr_sram.address, wr_sram.sram_entry);
	if ( err != 0 )      return -err;
        wr_sram.sram_entry[0] = tag;
        err = idt75k_write_sram(dev_p, wr_sram.db_num, wr_sram.address, wr_sram.sram_entry);
	if ( err != 0 )      return -err;
#endif

	/* if the entry should not be valid issue a request to clear the valid flag */
	if ( state == false )
	{
		clr_valid.db_num  = db;
		clr_valid.address = (index << 2);

		err = id75k_set_cam_entry_state(dev_p, db, wr_entry.address, 0);
		return -err;
		
	}

	return 0;
}
/**
 * Reads an entry from the TCAM at the given address.in 144 bit width word addressing
 *
 * @param[in]  db       The database that contains the entry, needed by the
 *                      hardware to determine the size of the entry.
 * @param[in]  index    The index number of the entry to read, valid values
 * @param[in]  value   pointer to value(data) to be written into the TCAM core.
 * @param[in]  mask    pointer to mask to be written into the TCAM.
 * @param[in]  ptag    pointer to 32 bit value will be read from sram
 * @param[in]  pstate  pointer to boolean value whihc gets the current state of the entry
 *
 * @returns             Returns an error code that indicates either
 *                      success or failure.
 * @retval ESUCCESS     The operation succeeded.
 * @retval EBADF        The driver is not opened, call open first.
 * @retval EINVAL       Invalid argument.
 * @retval EIO          Some physical I/O error has occurred
 *
 * @sa read_cam_entry | write_cam_entry
 */
int tcam_read_cam_entry(idt75k_dev_t *dev_p,uint32_t db, uint32_t index, uint8_t *value, uint8_t *mask, uint32_t *ptag, int *pstate )
{
	idt75k_read_entry_t   rd_entry_upper;
	idt75k_read_entry_t   rd_entry_lower;
	idt75k_read_sram_t    rd_sram;
	int err,i;
	unsigned int valid;


	/* Sanity check the input */
	if ( index >= (128 * 1024) )
		return EINVAL;
	if ( db > 15 )
		return EINVAL;
	

	/* because we can only read 72-bit wide entries at a time, we have to perform 2 reads
	 * one for the upper 72-bits and the second for the lower 72 bits.
	 */
	

	/* read the upper 72-bits of the entry */
	memset (&rd_entry_upper, 0x00, sizeof(rd_entry_upper));
	rd_entry_upper.address = (index << 2) + 0;
	
	err = idt75k_read_cam_entry(dev_p, rd_entry_upper.address, rd_entry_upper.data, rd_entry_upper.mask, &valid);
	if(err != 0 ) return err;
	rd_entry_upper.valid = (valid == 0) ? 0 : 1;

	/* read the lower 72-bits of the entry */
	memset (&rd_entry_lower, 0x00, sizeof(rd_entry_lower));
	rd_entry_lower.address = (index << 2) + 2;
	
	err = idt75k_read_cam_entry(dev_p, rd_entry_lower.address, rd_entry_lower.data, rd_entry_lower.mask, &valid);
	if(err != 0 ) return err;
	rd_entry_lower.valid = (valid == 0) ? 0 : 1;
	
	
	/* read the assoicated data (32-bits) from the entry */
	memset (&rd_sram, 0x00, sizeof(rd_sram));
	rd_sram.db_num = db;
	rd_sram.address = (index << 2);
	
		                       
	err = idt75k_read_sram(dev_p, rd_sram.db_num, rd_sram.address, rd_sram.sram_entry);
	if(err != 0 ) return err;

	
	/* gather all the data */
	for (i=0; i<9; i++)
	{
		mask[i] = rd_entry_upper.mask[i];
		value[i] = rd_entry_upper.data[i];
	}
	for (i=0; i<9; i++)
	{
		mask[i+9] = rd_entry_lower.mask[i];
		value[i+9] = rd_entry_lower.data[i];
	}
	*pstate = rd_entry_upper.valid;
	*ptag = rd_sram.sram_entry[0];

	return 0;
}

/**
 * Gets the contents of one of the global mask registers (GMR). Note
 * that internally the GMRs are 72-bits wide, but because we are using
 * a 144 bit lookup we join to two together, this gives us a total of
 * 31 masks.
 *
 * @param[in]  index  - Global Mask Register index
 * @param[out]  mask[18]  - Pinter to a char array returning the mask in 144bit values
 *
 * @returns            Returns an error code that indicates either
 *                     success or failure.
 * @retval ESUCCESS    The operation succeeded.
 * @retval EBADF       The driver is not opened, call open first.
 * @retval EIO         Some physical I/O error has occurred
 *
 * @sa   
 */
int tcam_get_global_mask_reg(idt75k_dev_t *dev_p,uint32_t index, uint8_t mask[18])
{
	idt75k_read_gmr_t   gmr_upper;
	idt75k_read_gmr_t   gmr_lower;
	int i;

	/* check to ensure the driver is loaded */
	if ( index > 31 )
		return EINVAL;


	/* construct the arguments for the request */
	gmr_upper.address = (index << 1);
	gmr_lower.address = ((index << 1) + 1);

	/* get the upper 72 bits of the GMR register */
	if ( idt75k_read_gmr(dev_p, gmr_upper.address , gmr_upper.value) != 0 )
		return -11;

	/* get the lower 72 bits of the GMR register */	
	if( idt75k_read_gmr(dev_p, gmr_lower.address , gmr_lower.value) != 0 ) 
		return -12;


	/* copy to the user supplied varaibles */
	for (i=0; i<18; i++)
	{
		if ( i < 9 )
			mask[i] = gmr_upper.value[i];
		else
			mask[i] = gmr_lower.value[i-9];
	}

	return 0;
}

/**
 * Performs a lookup on a database.
 *
 * @param[in]  db       The number of the database to perform the lookup for.
 * @param[in]  value    The value to lookup in the database, this should be
 *                      a 144 bit number (18 bytes), where the MSB at index 0.
 * @param[out] hit      Reference to a value that receives a value indicating
 *                      whether or not the lookup hit on a value or not.
 * @param[out] index    Will contain the index of the item found if the lookup
 *                      resulted in a hit, otherwise zero is set.
 * @param[out] tag      The assoicated data tag assigned to the lookup value
 *                      if a hit occuried.
 *
 * @returns             Returns an error code that indicates either
 *                      success or failure (a lookup miss is not a failure).
 * @retval ESUCCESS     The operation succeeded.
 * @retval EBADF        The driver is not opened, call open first.
 * @retval EIO          Some physical I/O error has occurred
 *
 * @sa tcam_read_cam_entry | tcam_write_cam_entry
 */
int tcam_lookup(idt75k_dev_t *dev_p,uint32_t db, uint8_t value[18], uint32_t gmr, bool *hit, uint32_t *index, uint32_t *tag)
{
	idt75k_lookup_t   lookup;
	int err;

	/* check to ensure the driver is loaded */
	if ( db > 15 )
		return EINVAL;
	if ( value == NULL )
		return EINVAL;
	if ( gmr > 31 )
		return EINVAL;


	/* clear the arguments */
	*hit = false;
	*index = 0;
	*tag = 0;

	/* populate the structure for the driver */
	memset (&lookup, 0x00, sizeof(idt75k_lookup_t));

	lookup.db_num = db;
	lookup.lookup_size = 144;
	lookup.lookup[0] = ((uint32_t)value[0] << 24)  | ((uint32_t)value[1] << 16)  | ((uint32_t)value[2] << 8)  | ((uint32_t)value[3] << 0);
	lookup.lookup[1] = ((uint32_t)value[4] << 24)  | ((uint32_t)value[5] << 16)  | ((uint32_t)value[6] << 8)  | ((uint32_t)value[7] << 0);
	lookup.lookup[2] = ((uint32_t)value[8] << 24)  | ((uint32_t)value[9] << 16)  | ((uint32_t)value[10] << 8) | ((uint32_t)value[11] << 0);
	lookup.lookup[3] = ((uint32_t)value[12] << 24) | ((uint32_t)value[13] << 16) | ((uint32_t)value[14] << 8) | ((uint32_t)value[15] << 0);
	lookup.lookup[4] = ((uint32_t)value[16] << 24) | ((uint32_t)value[17] << 16);
	lookup.gmr = gmr;


	/* request lookup operation */
	
	err = idt75k_lookup(dev_p, lookup.db_num, lookup.lookup, lookup.lookup_size, lookup.gmr, &lookup.hit, &lookup.index, lookup.associated_data);
	if ( err != 0  )
		return err;
	
	/* process the resutls */
	*hit = (lookup.hit == 1) ? true : false;
	if ( *hit )
	{
		*index = lookup.index;
		*tag   = lookup.associated_data[0];
	}


	return 0;
}

int tcam_write_cam_entry_new(idt75k_dev_t *dev_p,uint32_t db, uint32_t index, uint8_t *value, uint8_t *mask, uint32_t tag, int state,int gmask)
{
	idt75k_clear_valid_t  clr_valid;
	int err,i;
#if defined(USE_DUAL_WRITE)
	idt75k_write_entry_ex_t   wr_entry;
#else
	idt75k_write_entry_t  wr_entry;
	idt75k_write_sram_t   wr_sram;

#endif


	/* Sanity check the input */
	if ( index >= (128 * 1024) )
		return EINVAL;
	if ( db > 15 )
		return EINVAL;

#if defined(USE_DUAL_WRITE)

	memset (&wr_entry, 0x00, sizeof(idt75k_write_entry_ex_t));


	wr_entry.db_num = db;
	wr_entry.address = (index << 2);
	wr_entry.validate = state;

	for (int i=0; i<18; i++)
	{
		wr_entry.mask[i] = mask[i];
		wr_entry.data[i] = value[i];
	}

	wr_entry.ad_value[0] = tag;
	wr_entry.gmask = 31;

	dagutil_verbous("loading TCAM entry into db:%d, addr:0x%04X", db, index);
	err = idt75k_write_cam_entry_ex(dev_p, wr_entry.db_num, wr_entry.address, wr_entry.data, wr_entry.mask, wr_entry.gmask, wr_entry.ad_value);
	if ( err != 0 )    return -err;

#else     /* !defined(USE_DUAL_WRITE) */



	/* write the cam mask and data entries */
	memset (&wr_entry, 0x00, sizeof(idt75k_write_entry_t));

	wr_entry.db_num = db;
	wr_entry.address = (index << 2);
	wr_entry.validate = state ? 1 : 0;
	wr_entry.gmask = gmask;

	for (i=0; i<18; i++)
	{
		wr_entry.mask[i] = mask[i];
		wr_entry.data[i] = value[i];
	}
	
	err = idt75k_write_cam_entry(dev_p, wr_entry.db_num, wr_entry.address, wr_entry.data, wr_entry.mask, wr_entry.gmask, wr_entry.validate);
	if ( err != 0 )    return -err;


	/* write the assoicated data (32-bits) from the entry */
	memset (&wr_sram, 0x00, sizeof(wr_sram));
	wr_sram.address = (index << 2);
	wr_sram.db_num = db;
	/*We write first 00 to the ASRAM to cope with HW problems and then the real value 
	as weel it is required or recommended to set bits [0:3] oof CSR 0x27 to 7 to improve the ZBT interface*/
	wr_sram.sram_entry[0] = 0x00000000;
	err = idt75k_write_sram(dev_p, wr_sram.db_num, wr_sram.address, wr_sram.sram_entry);
	if ( err != 0 )      return -err;
        wr_sram.sram_entry[0] = tag;
        err = idt75k_write_sram(dev_p, wr_sram.db_num, wr_sram.address, wr_sram.sram_entry);
	if ( err != 0 )      return -err;
#endif

	/* if the entry should not be valid issue a request to clear the valid flag */
	if ( state == false )
	{
		clr_valid.db_num  = db;
		clr_valid.address = (index << 2);

		err = id75k_set_cam_entry_state(dev_p, db, wr_entry.address, 0);
		return -err;
		
	}

	return 0;
}

int tcam_get_device(idt75k_dev_t *dev_p,char *dagname,int dev_index)
{
	/*expects dagname to be /dev/dagN*/

	
	int g_dagfd = -1;
	uint8_t* g_iom_p = NULL;
	int count = 0;
	dag_reg_t regs[DAG_REG_MAX_ENTRIES];
	
	
	g_dagfd = dag_open(dagname);
	if(g_dagfd < 0)
	{
		printf("Error: Unable to open device.\n");
		g_dagfd = -1;
		g_iom_p = NULL;

	}
	else
	{
		g_iom_p = dag_iom(g_dagfd);
		if(g_iom_p == NULL)
			dagutil_panic("dag_iom failed \n");

		count = dag_reg_find((char *)g_iom_p,DAG_REG_INFINICAM,regs);
		if(count < dev_index+1)
		{
			dagutil_panic("dag_reg_find: failed \n");
		}
		dev_p->mmio_ptr = (void*) (g_iom_p + regs[dev_index].addr);
		dev_p->device_id = 0;
	}
	return 0;
}

/**
 * Performs a read and decode from  TCAM entry at the moment 144 bit to infiniband bit field structure
 *
 * @param[out]  fulter_rule   a infiniband filter rule structure  will be set according to 
 * 				the rule read from the TCAM 
 * @param[in]  db       The database entry to read from 
 * @param[in]  index    The rule entry to read
 * @param[in] debug     if true will print to stdout the decoded values
 *
 * @returns             Returns an error code that indicates either
 *                      success or failure (a lookup miss is not a failure).
 * @retval ESUCCESS     The operation succeeded.
 *
 * @sa 
 */

int infiniband_read_entry (idt75k_dev_t *dev_p, int db, int index, infiniband_filter_rule_t *filter_rule, int debug)
{
	int               res;
	uint32_t          tag;
	uint8_t           mask[18];
	uint8_t           data[18];
	int              state;


	/* First, Mask and Data */
	res = tcam_read_cam_entry (dev_p,db,(db*16384) + index, data, mask, &tag,&state);
	if (res == 0)
	{	

			/*BIT 108 - BIT 111 4 bits*/
			filter_rule->service_level.data = ((data[4] & 0xf0) >> 4);  
		
			/*The user tag. from the SRAM*/
			filter_rule->user_tag = tag >> 4; 
		
			/*BIT 106 - BIT 107 2 bits */
			filter_rule->lnh.data = ((data[4] & 0x0c) >> 2);
			
			/*BIT 128 - BIT 143 16 bits*/
			filter_rule->src_local_id.data  = ((data[0] << 8) | (data[1]));
	
			/*BIT 112 - BIT 127 16 bits*/
			filter_rule->dest_local_id.data = ((data[2] << 8) | (data[3]));	

			/*BIT98 - BIT105*/
			filter_rule->opcode.data = ((data[4] & 0x03) << 6) | ((data[5] & 0xfc) >> 2);	 

			/*BIT 74 - BIT 97  24 bits*/
			filter_rule->dest_qp.data = ((data[5] & 0x3) << 22) | (data[6] << 14) | (data[7] << 6) | ((data[8] & 0xfc) >> 2);
			
			/*BIT 50 - BIT 73  24 bits*/				
			filter_rule->src_qp.data = (((data[8] & 0x03)) << 22) | (data[9] << 14) | (data[10] << 6) | ((data[11] & 0xfc) >> 2); 

			/*BIT 108 - BIT 111 4 bits*/
			filter_rule->service_level.mask = ((mask[4] & 0xf0) >> 4);  
		
			/*The user tag. from the SRAM*/
			filter_rule->user_tag = tag; 
			/*The action filed accept or reject*/
			filter_rule->action = (tag & 0x4) >> 2; 
			/*The action filed accept or reject*/
			filter_rule->user_class = (tag & 0x3); 
		
			/*BIT 106 - BIT 107 2 bits */
			filter_rule->lnh.mask = ((mask[4] & 0x0c) >> 2);
			
			/*BIT 128 - BIT 143 16 bits*/
			filter_rule->src_local_id.mask  = ((mask[0] << 8) | (mask[1]));
	
			/*BIT 112 - BIT 127 16 bits*/
			filter_rule->dest_local_id.mask = ((mask[2] << 8) | (mask[3]));	
			
			/*BIT 98 - BIT 105 16 bits*/
			filter_rule->opcode.mask = ((mask[4] & 0x03) << 6)| ((mask[5] & 0xfc) >> 2);	

			/*BIT 75 - BIT 97  24 bits*/
			filter_rule->dest_qp.mask = ((mask[5] & 0x3) << 22) | (mask[6] << 14) | (mask[7] << 6) | ((mask[8] & 0xfc) >> 2);
			
			/*BIT 50 - BIT 74  24 bits*/
			filter_rule->src_qp.mask = ((mask[8] & 0x03) << 22) | (mask[9] << 14) | (mask[10] << 6) | ((mask[11]& 0xfc) >> 2); 

			if(debug) {
			fprintf( stdout, "User tag: %d, action: %s, steering: %d \n\n",
                        	filter_rule->user_tag, (filter_rule->action == 0)?"through":"drop" , filter_rule->user_class );
                	fprintf( stdout, "service_level data: 0x%01x, service_level mask: 0x%01x \n\n",
                        	filter_rule->service_level.data,filter_rule->service_level.mask );
                	fprintf( stdout, "lnh.data: 0x%01x, lnh.mask: 0x%01x \n\n",
                        	filter_rule->lnh.data, filter_rule->lnh.mask);
                	fprintf(stdout, "src_local_id.data: 0x%04x, src_local_id.mask: 0x%04x \n\n",
                        	filter_rule->src_local_id.data, filter_rule->src_local_id.mask);
                	fprintf( stdout, "dest_local_id.data: 0x%04x, dest_local_id.mask: 0x%04x \n\n",
                        	filter_rule->dest_local_id.data, filter_rule->dest_local_id.mask);
                	fprintf( stdout, "opcode.data: 0x%01x, opcode.mask: 0x%01x \n\n",
                        	filter_rule->opcode.data, filter_rule->opcode.mask);
                	fprintf( stdout, "dest_qp.data: 0x%06x, dest_qp.mask: 0x%06x \n\n",
                        	filter_rule->dest_qp.data, filter_rule->dest_qp.mask);
        		/* DETH Header classification fields */
                	fprintf(stdout, "src_qp.data: 0x%06x, src_qp.mask: 0x%06x \n\n",
                        	filter_rule->src_qp.data, filter_rule->src_qp.mask);
			}
	} else {
		if(debug) {
                	fprintf(stdout, "ERROR: infiniband_read_entry: db:%d addr:%d,\n", db, index);
		}
			

	};
	return res;

}


/**
 * Performs a encode from infiniband bit field structure to a bit field array at the moment 144 bit
 *
 * @param[in]  fulter_rule   a infiniband filter rule structure 
 * 			will be set according to the dat,mask and tag 
 * @param[in]  db    	database to wirte to 
 * @param[in]  index    the address to write to 
 * @param[in]  debug    if tru will print out encoded value
 *
 * @returns             Returns an error code that indicates either
 *                      success or failure (a lookup miss is not a failure).
 * @retval ESUCCESS     The operation succeeded.
 *
 * @sa 
 */

int infiniband_write_entry (idt75k_dev_t * dev_p, int db, int index, infiniband_filter_rule_t *filter_rule, int debug)
{
	int               res;
	uint32_t          tag;
	uint8_t           mask[18];
	uint8_t           data[18];
	int              state;


			memset(data,'\0',(sizeof(char)*18));
			memset(mask,'\0',(sizeof(char)*18));
			/*BIT 108 - BIT 111 4 bits*/
			if (filter_rule->service_level.data > 15) 
			{
				//CHANGEME: only for debug mode 
				dagutil_panic("incurrect service level");
			}				
			/*BIT 108 - BIT 111 4 bits*/
			data[4] = (filter_rule->service_level.data & 0xf) << 4;  
			/*BIT 108 - BIT 111 4 bits*/
			mask[4] = (filter_rule->service_level.mask & 0xf) << 4;
		
			/*The user tag. from the SRAM*/
			tag = filter_rule->user_tag;
			/*shifting the user tag from BIT 4 - BIT 19 (16 BITS)*/
			tag = (tag << 4);
			/*The action filed accept or reject bit 2 1 for drop and true in the yser action is drop*/
			tag |= (filter_rule->action) ? 0x4:0; 
			/*The action filed accept or reject*/
			tag |= filter_rule->user_class & 0x3; 

			/*BIT 106 - BIT 107 2 bits */
			data[4] |= (filter_rule->lnh.data & 0x3) << 2 ;
			/*BIT 106 - BIT 107 2 bits */
			mask[4] |= (filter_rule->lnh.mask & 0x3) << 2 ;
			
			/*BIT 128 - BIT 143 16 bits*/
			data[0] = (filter_rule->src_local_id.data >> 8) & 0xFF;
			data[1] = (filter_rule->src_local_id.data &  0xFF );
			/*BIT 128 - BIT 143 16 bits*/
			mask[0] = (filter_rule->src_local_id.mask >> 8) & 0xFF;
			mask[1] = (filter_rule->src_local_id.mask &  0xFF );
	
			
			/*BIT 112 - BIT 127 16 bits data*/
			data[2] = (filter_rule->dest_local_id.data >> 8) & 0xFF;
			data[3] = (filter_rule->dest_local_id.data &  0xFF );	
			/*BIT 112 - BIT 127 16 bits mask */
			mask[2] = (filter_rule->dest_local_id.mask >> 8) & 0xFF;
			mask[3] = (filter_rule->dest_local_id.mask &  0xFF );

			/*BIT 75 - BIT 97  24 bits*/
			/*opcode*/
			data[5] = (filter_rule->opcode.data & 0x3f) << 2;
			data[4]|= (filter_rule->opcode.data & 0xc0) >> 6;
			mask[5] = (filter_rule->opcode.mask & 0x3f) << 2;
			mask[4]|= (filter_rule->opcode.mask & 0xc0) >> 6;

			data[5]|= (filter_rule->dest_qp.data >> 22) & 0x3 ;
			data[6] = (filter_rule->dest_qp.data >> 14) & 0xFF ;
			data[7] = (filter_rule->dest_qp.data >>  6) & 0xFF ;
			data[8] = (filter_rule->dest_qp.data & 0x3F) << 2;
			/*BIT 75 - BIT 97  24 bits*/


			mask[5]|= (filter_rule->dest_qp.mask >> 22) & 0x3 ;
			mask[6] = (filter_rule->dest_qp.mask >> 14) & 0xFF ;
			mask[7] = (filter_rule->dest_qp.mask >>  6) & 0xFF ;
			mask[8] = (filter_rule->dest_qp.mask &  0x3F) << 2;
			
			/*BIT 50 - BIT 74  24 bits*/				
			data[8] |= (filter_rule->src_qp.data >> 22) & 0x3 ;
			data[9]  = (filter_rule->src_qp.data >> 14) & 0xFF ;
			data[10] = (filter_rule->src_qp.data >>  6) & 0xFF ;
			data[11] = (filter_rule->src_qp.data &  0x3F) << 2;
			/*BIT 50 - BIT 74  24 bits*/				
			mask[8] |= (filter_rule->src_qp.mask >> 22) & 0x3 ;
			mask[9]  = (filter_rule->src_qp.mask >> 14) & 0xFF ;
			mask[10] = (filter_rule->src_qp.mask >>  6) & 0xFF ;
			mask[11] = (filter_rule->src_qp.mask & 0x3F) << 2;

			/*BIT 49.*/
			data[11] |= (filter_rule->link_id.data & 0x1) << 1;
			mask[11] |= (filter_rule->link_id.mask & 0x1) << 1;
	

			

			if(debug) {
			fprintf( stdout, "User tag: %d, action: %s, steering: %d \n\n",
                        	filter_rule->user_tag, (filter_rule->action == 0)?"through":"drop" , filter_rule->user_class );
                	fprintf( stdout, "service_level data: 0x%01x, service_level mask: 0x%01x \n\n",
                        	filter_rule->service_level.data,filter_rule->service_level.mask );
                	fprintf( stdout, "lnh.data: 0x%01x, lnh.mask: 0x%01x \n\n",
                        	filter_rule->lnh.data, filter_rule->lnh.mask);
                	fprintf(stdout, "src_local_id.data: 0x%04x, src_local_id.mask: 0x%04x \n\n",
                        	filter_rule->src_local_id.data, filter_rule->src_local_id.mask);
                	fprintf( stdout, "dest_local_id.data: 0x%04x, dest_local_id.mask: 0x%04x \n\n",
                        	filter_rule->dest_local_id.data, filter_rule->dest_local_id.mask);
                	fprintf( stdout, "opcode.data: 0x%01x, opcode.mask: 0x%01x \n\n",
                        	filter_rule->opcode.data, filter_rule->opcode.mask);
                	fprintf( stdout, "dest_qp.data: 0x%06x, dest_qp.mask: 0x%06x \n\n",
                        	filter_rule->dest_qp.data, filter_rule->dest_qp.mask);
        		/* DETH Header classification fields */
                	fprintf(stdout, "src_qp.data: 0x%06x, src_qp.mask: 0x%06x \n\n",
                        	filter_rule->src_qp.data, filter_rule->src_qp.mask);
			}
	// validates the ent6ry one's written 
	state = 1;
	
	res = tcam_write_cam_entry( dev_p, db, index + (db*16384), data, mask, tag, state);
	
	if (res != 0)
	{
		dagutil_panic("infiniband_write_entry: Write entry to tcam failed\n");
	}
	return 0;
}

