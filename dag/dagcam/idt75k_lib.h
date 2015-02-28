/*
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: idt75k_lib.h,v 1.18 2008/10/07 03:30:31 vladimir Exp $
 */
#ifndef IDT75K_LIB_H
#define IDT75K_LIB_H


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
#include "dagapi.h"
#include "dag_platform.h"
#include "dagreg.h"
#include "dagutil.h"
#include "dagclarg.h"
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <stdint.h>
//#include <sys/io.h>
#endif

//TEMP 
#include "dagcam/infiniband_proto.h"
//#undef DD
//#ifdef DEBUG
//	#define DD(MSG)     printk MSG
//#else
//	#define DD(MSG)
//#endif

#define DA(MSG)     printf MSG
#define DN(MSG)     



/**
 * The device and vendor ID for the IDT75K72234
 */
#ifndef PCI_VENDOR_ID_IDT
#define PCI_VENDOR_ID_IDT              0x111d
#endif
#define PCI_DEVICE_ID_IDT_75K2234      0x021A


#ifndef ESUCCESS
	#define ESUCCESS   0
#endif




/**
 * Structure that stores driver specific data for the IDT75K chip, this
 * struction is used in the dev->dev.driver_data section of the driver
 * device structure.
 *
 */
typedef struct idt75k_dev_s
{
	uint8_t         *mmio_ptr;       /**< memory mapped IO space */
	uint32_t         mmio_size;      /**< size of the memory mapped region */

	uint32_t         device_id;      /**< this is NOT the PCI device id, rather it is the number of the device
	                                  * when multiple are cassacaded together. This value is set by the chip
	                                  * at initialisation, by sampling some bootstrap hardware pins.
	                                  */

	uint32_t         pstore[16];     /**< persistant storage array, used by user mode programs to store meta
	                                  * data about the device.
	                                  */

//	uint32_t         irq;            /**< the irq mapped to the driver */
//	uint32_t         irq_mask;       /**< the interrupt mask for the chip (set during driver initialisation */

//	struct cdev      cdev;           /**< the char device structure */
	
} idt75k_dev_t, *idt75k_dev_p;




/**
 * The following are the addresses of some of the CSR's in the IDT75K2234
 * chip. These addresses don't contain the 'Region Mapping' portion of the
 * address.
 *
 */
#define NSE_CONF_ADDR                 0x0000
#define NSE_IDENT1_ADDR               0x0001
#define NSE_IDENT2_ADDR               0x0002
#define NSE_PARITY_SEG_SELECT_ADDR    0x0003
#define NSE_PARITY_ADDR               0x0004
#define NSE_SRAM_PARITY_ADDR          0x0005
#define IMPEDANCE_CONTROL_ADDR        0x0006
#define GLOBAL_AGING_ADDR             0x0007
#define NSE_PARITY_POINTER_ADDR       0x0008

#define INTERRUPT_ADDR                0x000A
#define PCI_INTERRUPT_MASK_ADDR       0x000C



/**
 * Defines for instruction constants
 */
#define INSTR_LOOKUP                  0x0
#define INSTR_MULTIHIT_LOOKUP         0x1
#define INSTR_INDIRECT                0x3


/**
 * Defines for sub-instruction constants
 */
#define SUBINSTR_READ                 0x00
#define SUBINSTR_WRITE                0x01
#define SUBINSTR_WRITE_KEEP_VALID     0x02
#define SUBINSTR_DUAL_WRITE           0x03
#define SUBINSTR_LEARN                0x04
#define SUBINSTR_SET_VALID            0x05
#define SUBINSTR_CLEAR_VALID          0x06
#define SUBINSTR_SRAM_COPY            0x07
#define SUBINSTR_MULTIHIT_INVALIDATE  0x08
#define SUBINSTR_MDL                  0x09
#define SUBINSTR_RMDL                 0x0A
#define SUBINSTR_LEARN_INIT           0x0B
#define SUBINSTR_RESET                0x0C
#define SUBINSTR_FLUSH                0x0D
#define SUBINSTR_PARITY_CHECK         0x0E
/*#define SUBINSTR_PRELOAD_CLEAR       0x0F     not supported via PCI interface */


/**
 * Defines for interrupt masks
 */
#define INTR_MASK_PCI_ADDR_PARITY    (1 << 25)
#define INTR_MASK_PCI_DATA_PARITY    (1 << 24)
#define INTR_MASK_F1_AFO             (1 << 23)
#define INTR_MASK_F1_AFT             (1 << 22)
#define INTR_MASK_F1_AFH             (1 << 21)
#define INTR_MASK_F1_AFF             (1 << 20)
#define INTR_MASK_F0_AFO             (1 << 19)
#define INTR_MASK_F0_AFT             (1 << 18)
#define INTR_MASK_F0_AFH             (1 << 17)
#define INTR_MASK_F0_AFF             (1 << 16)
#define INTR_MASK_L1_RES             (1 << 10)
#define INTR_MASK_L1_F               (1 << 9)
#define INTR_MASK_L1_Q               (1 << 8)
#define INTR_MASK_L0_RES             (1 << 4)
#define INTR_MASK_L0_F               (1 << 3)
#define INTR_MASK_L0_Q               (1 << 2)
#define INTR_MASK_SRAM_PARITY        (1 << 1)
#define INTR_MASK_CORE_PARITY        (1 << 0)



/**
 * Defines for possible regions when doing a read request.
 *
 */
#define IDT75K_REGION_CORE_DATA       0x0
#define IDT75K_REGION_CORE_MASK       0x1
#define IDT75K_REGION_SRAM            0x2
#define IDT75K_REGION_CSR             0x3
#define IDT75K_REGION_GMR             0x4
#define IDT75K_REGION_AGING           0x5
#define IDT75K_REGION_AGE_ACTIVITY    0x6




/**
 * The expected value of the IDENT1 register as specified in
 * datasheet. These are used for sanity checking when the driver is first probed.
 */
#define IDT75K_NSE_IDENT1_VALUE       0x0021A067




/**
 * Macro the constructs the PCI write word for the IDT75K.
 *
 */
#define IDT75K_CONSTRUCT_PCI_WRITE(no_result, pci_size, instr, gmask, db, count_cmd) \
	(uint32_t)( ((uint32_t)(no_result & 0x1) << 31) | \
	            ((uint32_t)(pci_size & 0x1F) << 19) | \
	            ((uint32_t)(instr & 0x3) << 15) | \
	            ((uint32_t)(gmask & 0x1F) << 10) | \
	            ((uint32_t)(db & 0xF) << 6) | \
	            ((uint32_t)(count_cmd & 0xF) << 2) )

/*Construction of Word 0 for the LA-1 Interface Command Interface.*/
#define IDT75K_CONSTRUCT_LA1_WRITE(context,instr,gmask,db,word_count) \
	(uint32_t)( ((uint32_t)(context & 0x7F) << 15) | \
		    ((uint32_t)(instr & 0x3) << 13 ) | \
		    ((uint32_t)(gmask & 0x1F) << 8 ) | \
		    ((uint32_t)(db & 0xF) << 4 ) | \
		    ((uint32_t)(word_count & 0xF)) ) 

		 	



/**
 * Macro to read the contents of one 32-bit mailbox entry in the given
 * context.
 */
/*
define IDT75K_READ_MAILBOX(dev, ctx, entry) \
	ioread32( (void*) ((unsigned)dev->mmio_ptr + (ctx << 8) + (entry << 2)) )
*/
//not operational yet 
#define IDT75K_READ_MAILBOX(dev, ctx, entry) \
	inl( (int) ((unsigned)dev->mmio_ptr + (ctx << 8) + (entry << 2)) )


/*****************************************************************************/
/* Function declarations                                                     */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	/*With the context as parameter for full production testing*/
	int idt75k_read_csr_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t addr, uint32_t * value_p);
	
	int idt75k_write_csr_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t addr, uint32_t value);
	
	int idt75k_set_dbconf_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t mask, uint32_t flags, uint32_t age_cnt, uint32_t segments);
	
	int idt75k_get_dbconf_ctx(idt75k_dev_t *dev_p, uint32_t context ,uint32_t db_num, uint32_t *flags_p, uint32_t *age_cnt_p, uint32_t *segments_p);
	
	int idt75k_read_cam_entry_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t addr, uint8_t data[9], uint8_t mask[9], uint32_t *valid_p);
	
	int idt75k_write_cam_entry_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, uint8_t data[72], uint8_t mask[72], uint32_t gmask, uint32_t validate);
	
	int idt75k_write_cam_entry_ex_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, const uint8_t data[72], const uint8_t mask[72], uint32_t gmask, const uint32_t ad_value[4]);
	
	int id75k_set_cam_entry_state_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, uint32_t valid);
	
	int idt75k_read_sram_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, uint32_t sram_entry[4]);
	
	int idt75k_write_sram_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t addr, const uint32_t sram_entry[4]);
	
	int idt75k_copy_sram_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t dst_addr, uint32_t src_addr);
	
	int idt75k_lookup_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t db_num, uint32_t *lookup_p, uint32_t lookup_size, uint32_t gmr, uint32_t *hit_p, uint32_t *index_p, uint32_t *ad_p);
	
	int idt75k_read_gmr_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t addr, uint8_t value[9]);
	
	int idt75k_write_gmr_ctx(idt75k_dev_t *dev_p,uint32_t context,uint32_t addr, const uint8_t value[9]);
	
	int idt75k_reset_device_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t reset_args);
	
	int idt75k_flush_device_ctx(idt75k_dev_t *dev_p,uint32_t context);
	
	int idt75k_read_pstore(idt75k_dev_t *dev_p, uint32_t index, uint32_t *value_p);
	
	int idt75k_write_pstore(idt75k_dev_t *dev_p, uint32_t index, uint32_t value);
	
	int idt75k_read_smt_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t addr, uint32_t *smt_entry_p);
	
	int idt75k_write_smt_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t addr, const uint32_t smt_entry);

	int idt75k_set_nse_conf_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t mask, uint32_t value);
	
	int idt75k_get_nseconf_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t *value_p);

	int idt75k_get_nseident1_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t *value_p);

	int idt75k_get_nseident2_ctx(idt75k_dev_t *dev_p,uint32_t context ,uint32_t *value_p);

	/*without the context to maintain the old interface*/
	int idt75k_read_csr(idt75k_dev_t *dev_p,uint32_t addr, uint32_t * value_p);

	int idt75k_write_csr(idt75k_dev_t *dev_p,uint32_t addr, uint32_t value);
	
	int idt75k_set_dbconf(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t mask, uint32_t flags, uint32_t age_cnt, uint32_t segments);
	
	int idt75k_get_dbconf(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t *flags_p, uint32_t *age_cnt_p, uint32_t *segments_p);
	
	int idt75k_read_cam_entry(idt75k_dev_t *dev_p,uint32_t addr, uint8_t data[9], uint8_t mask[9], uint32_t *valid_p);
	
	int idt75k_write_cam_entry(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t addr, uint8_t data[72], uint8_t mask[72], uint32_t gmask, uint32_t validate);
	
	int idt75k_write_cam_entry_ex(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t addr, const uint8_t data[72], const uint8_t mask[72], uint32_t gmask, const uint32_t ad_value[4]);
	
	int id75k_set_cam_entry_state(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t addr, uint32_t valid);
	
	int idt75k_read_sram(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t addr, uint32_t sram_entry[4]);
	
	int idt75k_write_sram(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t addr, const uint32_t sram_entry[4]);
	
	int idt75k_copy_sram(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t dst_addr, uint32_t src_addr);
	
	int idt75k_lookup(idt75k_dev_t *dev_p,uint32_t db_num, uint32_t *lookup_p, uint32_t lookup_size, uint32_t gmr, uint32_t *hit_p, uint32_t *index_p, uint32_t *ad_p);
	
	int idt75k_read_gmr(idt75k_dev_t *dev_p,uint32_t addr, uint8_t value[9]);
	
	int idt75k_write_gmr(idt75k_dev_t *dev_p,uint32_t addr, const uint8_t value[9]);
	
	int idt75k_reset_device(idt75k_dev_t *dev_p,uint32_t reset_args);
	
	int idt75k_flush_device(idt75k_dev_t *dev_p);
	
	int idt75k_read_smt(idt75k_dev_t *dev_p,uint32_t addr, uint32_t *smt_entry_p);
	
	int idt75k_write_smt(idt75k_dev_t *dev_p,uint32_t addr, const uint32_t smt_entry);

	int idt75k_set_nse_conf(idt75k_dev_t *dev_p,uint32_t mask, uint32_t value);

	int idt75k_get_nseconf(idt75k_dev_t *dev_p,uint32_t *value_p);
	
	int idt75k_get_nseident1(idt75k_dev_t *dev_p,uint32_t *value_p);

	int idt75k_get_nseident2(idt75k_dev_t *dev_p,uint32_t *value_p);

	/*
 	 * These are the set of function that have been moved from 
 	 * the Tool to the library.
 	 * 	
 	 * */

	int tcam_set_database_sectors(idt75k_dev_t *dev_p, uint32_t db, uint32_t sectors);

	int tcam_write_cam_entry(idt75k_dev_t* dev_p,uint32_t db, uint32_t index, uint8_t *value, uint8_t *mask, uint32_t tag, int state );

	int tcam_read_cam_entry(idt75k_dev_t* dev_p,uint32_t db, uint32_t index, uint8_t *value, uint8_t *mask, uint32_t *ptag, int *pstate );
	
	int tcam_initialise(idt75k_dev_t* dev_p_la1,idt75k_dev_t* dev_p_la2);

        int tcam_set_global_mask_reg(idt75k_dev_t *dev_p, uint32_t index, const uint8_t mask[18]);

        int tcam_lookup(idt75k_dev_t *dev_p,uint32_t db, uint8_t value[18], uint32_t gmr, bool *hit, uint32_t *index, uint32_t *tag);

        int tcam_get_global_mask_reg(idt75k_dev_t *dev_p,uint32_t index, uint8_t mask[18]);

	int tcam_write_cam_entry_new(idt75k_dev_t *dev_p,uint32_t db, uint32_t index, uint8_t *value, uint8_t *mask, uint32_t tag, int state,int gmask);

	int tcam_get_device(idt75k_dev_t *dev_p,char *dagname, int dev_index);

	// This functions are high level and probably shouyld be moved out a here 
	// Fot this release will do 
	int infiniband_write_entry (idt75k_dev_t * dev_p, int db, int index, infiniband_filter_rule_t *filter_rule, int debug);
	int infiniband_read_entry (idt75k_dev_t *dev_p, int db, int index, infiniband_filter_rule_t *filter_rule, int debug);

#ifdef __cplusplus
}
#endif /* __cplusplus */




#endif  // IDT75K_H


