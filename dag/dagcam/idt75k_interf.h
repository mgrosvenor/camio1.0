/*
 * Copyright (c) 2003 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: idt75k_interf.h,v 1.1 2008/01/29 04:03:26 karthik Exp $
 */
#ifndef IDT75K_IOCTL_H
#define IDT75K_IOCTL_H





/*---------------------------------------------------------------
 * IOCTL Magic number
 *---------------------------------------------------------------*/

#define IDT75K_IOCTL_MAGIC  'i'





/*---------------------------------------------------------------
 * IOCTL Structures
 *---------------------------------------------------------------*/



/**
 * The following structure is used in the IDT75K_IOCTL_SET_DBCONF IOCTL to set
 * the details for a particular database.
 */
typedef struct idt75k_set_dbconf_s
{
	uint32_t  db_num;          /**< [in]  the number of the database to set, valid values are 0 to 15 */
	uint32_t  mask;            /**< [in]  the mask to indicate which fields should be changed */
	uint32_t  flags;           /**< [in]  possible flags to set on the database */
	uint32_t  db_size;         /**<       deprecated, dont use */
	uint32_t  age_cnt;         /**< [in]  the age count to set for the database */
	uint32_t  segments;        /**< [in]  bitmask of the segments to include in the database */
} idt75k_set_dbconf_t;


/**
 * The following structure is used in the IDT75K_IOCTL_GET_DBCONF IOCTL to get
 * the details for a particular database.
 */
typedef struct idt75k_get_dbconf_s
{
	uint32_t  db_num;          /**< [in]  the number of the database to get, valid values are 0 to 15 */
	uint32_t  flags;           /**< [out] the database flags read from the chip */
	uint32_t  db_size;         /**<       deprecated, dont use */
	uint32_t  age_cnt;         /**< [out] the age count to set for the database */
	uint32_t  segments;        /**< [out] bitmask of the segments included in the database */
} idt75k_get_dbconf_t;


/* Possible flags and their masks for database configuration */
#define IDT75K_DBCONF_POWERSAVE             (1 << 29)
#define IDT75K_DBCONF_POWERSAVE_MASK        (1 << 29)

#define IDT75K_DBCONF_AR_ONLY               (1 << 28)
#define IDT75K_DBCONF_AR_ONLY_MASK          (1 << 28)

#define IDT75K_DBCONF_AGE_ENABLE            (1 << 27)
#define IDT75K_DBCONF_AGE_ENABLE_MASK       (1 << 27)

#define IDT75K_DBCONF_AD_NONE               (3 << 24)
#define IDT75K_DBCONF_AD_32                 (0 << 24)
#define IDT75K_DBCONF_AD_64                 (1 << 24)
#define IDT75K_DBCONF_AD_128                (2 << 24)
#define IDT75K_DBCONF_AD_MASK               (3 << 24)

#define IDT75K_DBCONF_AFR                   (1 << 23)
#define IDT75K_DBCONF_AFR_MASK              (1 << 23)

#define IDT75K_DBCONF_AFS                   (1 << 22)
#define IDT75K_DBCONF_AFS_MASK              (1 << 22)

#define IDT75K_DBCONF_RTN                   (1 << 21)
#define IDT75K_DBCONF_RTN_MASK              (1 << 21)

#define IDT75K_DBCONF_GMR_LOWER             (0 << 20)
#define IDT75K_DBCONF_GMR_UPPER             (1 << 20)
#define IDT75K_DBCONF_GMR_MASK              (1 << 20)

#define IDT75K_DBCONF_ACTIVITY_ENABLE       (1 << 18)
#define IDT75K_DBCONF_ACTIVITY_ENABLE_MASK  (1 << 18)

#define IDT75K_DBCONF_CORE_36               (1 << 5)
#define IDT75K_DBCONF_CORE_72               (1 << 6)
#define IDT75K_DBCONF_CORE_144              (1 << 7)
#define IDT75K_DBCONF_CORE_288              (1 << 8)
#define IDT75K_DBCONF_CORE_576              (1 << 9)
#define IDT75K_DBCONF_CORE_SIZE_MASK        (0x1F << 5)

#define IDT75K_DBCONF_DB_SIZE_MASK          (0x1F << 0)
#define IDT75K_DBCONF_DB_18SEL_SIZE_72      (0x01 << 0)
#define IDT75K_DBCONF_DB_18SEL_SIZE_144     (0x03 << 0)
#define IDT75K_DBCONF_DB_18SEL_SIZE_288     (0x07 << 0)
#define IDT75K_DBCONF_DB_16SEL_SIZE_72      (0x02 << 0)
#define IDT75K_DBCONF_DB_16SEL_SIZE_144     (0x04 << 0)
#define IDT75K_DBCONF_DB_16SEL_SIZE_288     (0x08 << 0)

/*Possible Flags and their masks for NSE Conf Register*/
#define IDT75K_NSECONF_AR_ONLY_GLOBAL       (1 << 18)  
#define IDT75K_NSECONF_L1PARITY_ENABLE      (1 << 17)
#define IDT75K_NSECONF_L0PARITY_ENABLE      (1 << 16)
#define IDT75K_NSECONF_SDML_REPLACES_MHL    (1 << 15)
#define IDT75K_NSECONF_L1_DATA18_SEL	    (1 << 14)	
#define IDT75K_NSECONF_L0_DATA18_SEL	    (1 << 13)
#define IDT75K_NSECONF_L1_WORD_ORDER	    (1 << 12)
#define IDT75K_NSECONF_L1_BYTE_ORDER        ((1 << 11)| (1 << 10))
#define IDT75K_NSECONF_L1_ENABLE	    (1 << 9)	
#define IDT75K_NSECONF_CD_COMPLETE	    (1 << 8)
#define IDT75K_NSECONF_L0_WORD_ORDER        (1 << 7)
#define IDT75K_NSECONF_L0_BYTE_ORDER        ((1 << 6)| (1 << 5)) 
#define IDT75K_NSECONF_ADSP		    (1 << 3)	
#define IDT75K_NSECONF_DPSIZE		    ((1 << 2)|(1 << 1)|(1 << 0))	

/*Possible Flags and their masks for NSE IDENT1 and NSE IDENT2 register*/
#define IDT75K_NSEIDENT_IEEE_STD             (0x01  << 0)
#define IDT75K_NSEIDENT_IDT_ID		     (0xFFE << 0)	
#define IDT75K_NSEIDENT_IDT_PART	     (0xFFFF << 12)	
#define IDT75K_NSEIDENT_IDT_VERSION	     ((1 << 28) | (1 << 29) | (1 << 30))
#define IDT75K_NSEIDENT_DEV_TYPE	     (1 << 31)	


#define IDT75K_NSEIDENT_DEVICE_ID            ((1 << 0) | (1 << 1) | (1 << 2))
#define IDT75K_NSEIDENT_L0_ID	             ((1 << 3) | (1 << 4))
#define IDT75K_NSEIDENT_L1_ID		     ((1 << 5) | (1 << 6)) 	
#define IDT75K_NSEIDENT_PORT_ID		     ( 1 << 31 ) 



/* these two aren't part of the NSE DB_CONF1 register but are included as masks for the IOCTL */
#define IDT75K_DBCONF_AGE_COUNT_MASK        (1 << 19)
#define IDT75K_DBCONF_SEGMENT_SEL_MASK      (1 << 31)





/**
 * The following structure is used in the IDT75K_IOCTL_LOOKUP IOCTL to lookup
 * a value in the TCAM.
 */
typedef struct idt75k_lookup_s
{
	uint32_t  db_num;             /**< [in]  the database number to search in */
	uint32_t  lookup_size;        /**< [in]  must be one of the following values: 36, 72, 144, 288, 576 */
	uint32_t  lookup[18];         /**< [in]  the value to lookup */
	uint32_t  gmr;                /**< [in]  the GMR to use for the lookup */
	uint32_t  hit;            /**< [out] indicates whether the lookup hit an entry */
	uint32_t  index;              /**< [out] the result of the lookup (may be a translated or absolute index based in database config) */
	uint32_t  associated_data[4]; /**< [out] upto 128 bits of associated data assigned to the entry */
} idt75k_lookup_t;


/**
 * The following structure is used in the IDT75K_IOCTL_READ_ENTRY IOCTL to read
 * an entry from the TCAM.
 */
typedef struct idt75k_read_entry_s
{
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint8_t   data[9];         /**< [out] the value read from the TCAM */
	uint8_t   mask[9];         /**< [out] the value read from the TCAM */
	uint32_t  valid    : 1;    /**< [out] if set the entry is marked as valid */
	uint32_t  reserved : 31;   /**<       reserved for future use */

} idt75k_read_entry_t;


/**
 * The following structure is used in the IDT75K_IOCTL_WRITE_ENTRY IOCTL to write
 * an entry to the TCAM. If the validate field is non-zero the entry is marked as
 * valid upon a successiful write, if validate is zero then the state of the entry
 * is maintained (i.e. if previous valid it remains valid, if previous invalid it
 * remains invalid).
 */
typedef struct idt75k_write_entry_s
{
	uint32_t  db_num;          /**< [in]  the database number containing the entry (used to determine the size of the entries) */
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint8_t   data[72];        /**< [in]  the value(s) to write to the TCAM */
	uint8_t   mask[72];        /**< [in]  the mask(s) to write to the TCAM */
	uint32_t  gmask;           /**< [in]  the number of the global mask to apply to the key during the write operation */
	uint32_t  validate;        /**< [in]  if a non-zero value the valid bits are set once the write is complete */

} idt75k_write_entry_t;


/**
 * The following structure is used in the IDT75K_IOCTL_WRITE_ENTRY_EX IOCTL to write
 * an entry to the TCAM along with it's associated data, this IOCTL always sets the
 * entry as valid if no error occuried
 */
typedef struct idt75k_write_entry_ex_s
{
	uint32_t  db_num;          /**< [in]  the database number containing the entry (used to determine the size of the entries) */
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint8_t   data[72];        /**< [in]  the value(s) to write to the TCAM */
	uint8_t   mask[72];        /**< [in]  the mask(s) to write to the TCAM */
	uint32_t  ad_value[4];     /**< [in]  the value to write into the SRAM associated data field */
	uint32_t  gmask;           /**< [in]  the number of the global mask to apply to the key during the write operation */

} idt75k_write_entry_ex_t;




/**
 * The following structure is used in the IDT75K_IOCTL_READ_SRAM IOCTL to read
 * an entry from the associated SRAM data store.
 */
typedef struct idt75k_read_sram_s
{
	uint32_t  db_num;          /**< [in]  the database number containing the entry (used to determine the size of the entries) */
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint32_t  sram_entry[4];   /**< [out] the value read from the SRAM */
} idt75k_read_sram_t;


/**
 * The following structure is used in the IDT75K_IOCTL_WRITE_SRAM IOCTL to write
 * an entry to the associated SRAM data store.
 */
typedef struct idt75k_write_sram_s
{
	uint32_t  db_num;          /**< [in]  the database number containing the entry (used to determine the size of the entries) */
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint32_t  sram_entry[4];   /**< [out] the value written to the SRAM */
} idt75k_write_sram_t;





/**
 * The following structure is used in the IDT75K_IOCTL_READ_GMR IOCTL to read
 * a Global Mask Register (GMR) from the device.
 */
typedef struct idt75k_read_gmr_s
{
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint8_t   value[9];        /**< [out] the 72 bits of the GMR register */
} idt75k_read_gmr_t;


/**
 * The following structure is used in the IDT75K_IOCTL_WRITE_GMR IOCTL to write
 * a Global Mask Register (GMR) to the device.
 */
typedef struct idt75k_write_gmr_s
{
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint8_t   value[9];        /**< [in]  the 72 bits of the GMR register */
} idt75k_write_gmr_t;





/**
 * The following structure is used in the IDT75K_IOCTL_READ_CSR IOCTL to read
 * a Control and Status Register (CSR) from the device.
 */
typedef struct idt75k_read_csr_s
{
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint32_t  value;           /**< [out] the 72 bits of the GMR register */
} idt75k_read_csr_t;


/**
 * The following structure is used in the IDT75K_IOCTL_WRITE_CSR IOCTL to write
 * a Control and Status Register (CSR) to the device.
 */
typedef struct idt75k_write_csr_s
{
	uint32_t  address;         /**< [in]  the address of the entry to read */
	uint32_t  value;           /**< [in]  the 72 bits of the GMR register */
} idt75k_write_csr_t;




/**
 * The following structure is used in the IDT75K_IOCTL_SET_VALID IOCTL
 */
typedef struct idt75k_set_valid_s
{
	uint32_t  db_num;          /**< [in]  the database number containing the entry (used to determine the size of the entry) */
	uint32_t  address;         /**< [in]  the address of the entry to read */
} idt75k_set_valid_t;


/**
 * The following structure is used in the IDT75K_IOCTL_CLEAR_VALID IOCTL
 */
typedef struct idt75k_clear_valid_s
{
	uint32_t  db_num;          /**< [in]  the database number containing the entry (used to determine the size of the entry) */
	uint32_t  address;         /**< [in]  the address of the entry to read */
} idt75k_clear_valid_t;





/**
 * The following structure is used in the IDT75K_IOCTL_READ_PSTORE IOCTL
 */
typedef struct idt75k_write_pstore_s
{
	uint32_t  index;          /**< [in]  the index of the entry to write */
	uint32_t  value;          /**< [in]  the value to write into the entry */
} idt75k_write_pstore_t;


/**
 * The following structure is used in the IDT75K_IOCTL_WRITE_PSTORE IOCTL
 */
typedef struct idt75k_read_pstore_s
{
	uint32_t  index;          /**< [in]  the index of the entry to write */
	uint32_t  value;          /**< [out] the value read from the entry */
} idt75k_read_pstore_t;





/**
 * The following structure is used in the IDT75K_IOCTL_COPY_SRAM IOCTL
 */
typedef struct idt75k_copy_sram_s
{
	uint32_t  db_num;          /**< [in]  the database number containing the source (used to determine the size of the SRAM to copy) */
	uint32_t  src_addr;        /**< [in]  the source address of the data to copy, this is the address of a core entry */
	uint32_t  dst_addr;        /**< [in]  the destination address of the data to copy, this is the address of a core entry */
} idt75k_copy_sram_t;





/**
 * The following structure is used in the IDT75K_IOCTL_READ_SMT IOCTL to read
 * an entry from a SMT (segment mapping table) register.
 */
typedef struct idt75k_read_smt_s
{
	uint32_t  addr;            /**< [in]  the address of the SMT register to read */
	uint32_t  value;           /**< [out] the value read from the SMT register */
} idt75k_read_smt_t;


/**
 * The following structure is used in the IDT75K_IOCTL_WRITE_SMT IOCTL to write
 * an entry to a SMT (segment mapping table) register.
 */
typedef struct idt75k_write_smt_s
{
	uint32_t  addr;            /**< [in]  the address of the SMT register to write */
	uint32_t  value;           /**< [in]  the value to write to the SMT register */
} idt75k_write_smt_t;





/**
 * Possible flags that can be set for the reset command.
 */
#define IDT75K_RESET_ALL_INTERFACES        0x00000001
#define IDT75K_RESET_DLL_MAILBOX           0x00000002


/*---------------------------------------------------------------
 * IOCTLs
 *---------------------------------------------------------------*/


/**
 * Sets/Gets the global configuration options for the device
 */
#define IDT75K_IOCTL_GET_NSE_CONF        _IOR(IDT75K_IOCTL_MAGIC, 1, uint32_t)
#define IDT75K_IOCTL_SET_NSE_CONF        _IOW(IDT75K_IOCTL_MAGIC, 2, uint32_t)


/**
 * Gets the two indentification registers from the device.
 */
#define IDT75K_IOCTL_GET_IDENT1_REG      _IOR(IDT75K_IOCTL_MAGIC, 3, uint32_t)
#define IDT75K_IOCTL_GET_IDENT2_REG      _IOR(IDT75K_IOCTL_MAGIC, 4, uint32_t)


/**
 * Configures a database inside the TCAM, note that the driver does manage the database configuration,
 * it is very simple and just blindly writes the configuration to the correct registers.
 */
#define IDT75K_IOCTL_SET_DBCONF          _IOW(IDT75K_IOCTL_MAGIC, 5, idt75k_set_dbconf_t)
#define IDT75K_IOCTL_GET_DBCONF          _IOWR(IDT75K_IOCTL_MAGIC, 6, idt75k_get_dbconf_t)


/**
 * Performs a lookup on one of the databases in the device and returns the address of
 * the entry found. Note this does not return the associated SRAM data.
 */
#define IDT75K_IOCTL_LOOKUP              _IOWR(IDT75K_IOCTL_MAGIC, 7, idt75k_lookup_t)

/**
 * Reads and writes an entry in the TCAM.
 */
#define IDT75K_IOCTL_READ_ENTRY          _IOWR(IDT75K_IOCTL_MAGIC, 8, idt75k_read_entry_t)
#define IDT75K_IOCTL_WRITE_ENTRY         _IOW(IDT75K_IOCTL_MAGIC, 9, idt75k_write_entry_t)
#define IDT75K_IOCTL_WRITE_ENTRY_EX      _IOW(IDT75K_IOCTL_MAGIC, 10, idt75k_write_entry_ex_t)

/**
 * Reads and writes an entry into the associated SRAM data.
 */
#define IDT75K_IOCTL_READ_SRAM          _IOWR(IDT75K_IOCTL_MAGIC, 11, idt75k_read_sram_t)
#define IDT75K_IOCTL_WRITE_SRAM         _IOW(IDT75K_IOCTL_MAGIC, 12, idt75k_write_sram_t)


/**
 * Sets of vlears the valid bit assoicated with a TCAM entry.
 */
#define IDT75K_IOCTL_SET_VALID          _IOW(IDT75K_IOCTL_MAGIC, 13, idt75k_set_valid_t)
#define IDT75K_IOCTL_CLEAR_VALID        _IOW(IDT75K_IOCTL_MAGIC, 14, idt75k_clear_valid_t)


/**
 * Reads and writes the global mask registers.
 */
#define IDT75K_IOCTL_READ_GMR          _IOWR(IDT75K_IOCTL_MAGIC, 15, idt75k_read_gmr_t)
#define IDT75K_IOCTL_WRITE_GMR         _IOW(IDT75K_IOCTL_MAGIC, 16, idt75k_write_gmr_t)

/**
 * Performs a soft reset on the device (use with caution)
 */
#define IDT75K_IOCTL_RESET             _IOW(IDT75K_IOCTL_MAGIC, 17, uint32_t)


/**
 * Flushes the entire TCAM (invalidates all entries)
 */
#define IDT75K_IOCTL_FLUSH             _IO(IDT75K_IOCTL_MAGIC, 18)



/**
 * Reads / Writes one of the addressed CSR registers
 */
#define IDT75K_IOCTL_READ_CSR          _IOWR(IDT75K_IOCTL_MAGIC, 19, idt75k_read_csr_t)
#define IDT75K_IOCTL_WRITE_CSR         _IOW(IDT75K_IOCTL_MAGIC, 20, idt75k_write_csr_t)



/**
 * Reads / Writes 32-bit values to the driver's persistant storage elements. These IOCTLs
 * can be used by user programs to store meta data about the state of the device.
 */
#define IDT75K_IOCTL_READ_PSTORE       _IOWR(IDT75K_IOCTL_MAGIC, 21, idt75k_read_pstore_t)
#define IDT75K_IOCTL_WRITE_PSTORE      _IOW(IDT75K_IOCTL_MAGIC, 22, idt75k_write_pstore_t)


/**
 * Performs a SRAM copy from one core entry to another.
 */
#define IDT75K_IOCTL_COPY_SRAM         _IOW(IDT75K_IOCTL_MAGIC, 23, idt75k_copy_sram_t)


/**
 * Reads / Writes one of the addressed SMT (segment mapping table) registers
 */
#define IDT75K_IOCTL_WRITE_SMT         _IOW(IDT75K_IOCTL_MAGIC, 24, idt75k_write_smt_t)
#define IDT75K_IOCTL_READ_SMT          _IOWR(IDT75K_IOCTL_MAGIC, 25, idt75k_read_smt_t)




















#endif  // IDT75K_IOCTL_H
