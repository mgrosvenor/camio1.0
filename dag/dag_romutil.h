
/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_romutil.h,v 1.38 2008/12/01 05:30:24 karthik Exp $
 */

#ifndef DAG_ROMUTIL_H
#define DAG_ROMUTIL_H

/* DAG headers. */
#include "dag_platform.h"


/* ---------------------------------------------------------*/
/* CFI (Common Flash Interface) Definitions                 */

/* CFI Vendor Identifiers */
#define CFI_VENDOR_GENERIC      0x0000
#define CFI_VENDOR_INTEL_EXT    0x0001
#define CFI_VENDOR_AMD_EXT      0x0002
#define CFI_VENDOR_INTEL        0x0003
#define CFI_VENDOR_AMD          0x0004
#define CFI_VENDOR_MITSUBISHI   0x0100
#define CFI_VENDOR_MITSUBISHI_EXT   0x0101
#define CFI_VENDOR_SST          0x0102

/* CFI Device Interface Codes */
#define CFI_DEV_X8      0x0000  /* async 8-bit only */
#define CFI_DEV_X16     0x0001  /* async 16-bit only */
#define CFI_DEV_X8_X16  0x0002  /* async 8 and 16 bit via BYTE# */
#define CFI_DEV_X32     0x0003  /* async 32-bit only */
#define CFI_DEV_X16_X32 0x0004  /* async 16 and 32 bit via WORD# */

/* CFI Query Addresses */
/* 8-bit device with 8-bit accesses */
#define CFI_8_Q             0x10
#define CFI_8_R             0x11
#define CFI_8_Y             0x12
#define CFI_8_VENDOR_LO     0x13
#define CFI_8_VENDOR_HI     0x14
#define CFI_8_SIZE          0x27
#define CFI_8_DEVICE_LO     0x28
#define CFI_8_DEVICE_HI     0x29

/* 16-bit device with 8-bit accesses */
#define CFI_16_8_Q          0x20
#define CFI_16_8_R          0x22
#define CFI_16_8_Y          0x24
#define CFI_16_8_VENDOR_LO  0x26
#define CFI_16_8_VENDOR_HI  0x28
#define CFI_16_8_SIZE       0x4e
#define CFI_16_8_DEVICE_LO  0x50
#define CFI_16_8_DEVICE_HI  0x52
#define EXT_TABLE_P         0x214
#define EXT_TABLE_R         0x216
#define EXT_TABLE_I         0x218
#define EXT_TABLE_MAJOR     0x21A
#define EXT_TABLE_MINOR     0x21C
#define EXT_TABLE_P_AND_E_BLOCK 0x21A
#define BYTES_IN_BUFFER      0x54
#define ERASE_BLOCK_REGIONS  0x58
#define ERASE_BLOCK_REGION_ONE_INFO 0x5A
#define ERASE_BLOCK_REGION_TWO_INFO 0x62


/* CFI Address for address sensitive devices (e.g AMD) */
/* CFI commands are written to this address */
#define CFI_BASE_ADDR           0x00
#define CFI_QUERY_ADDR          0x55    /* Needed for AMD */

/* CFI Commands */
#define CFI_CMD_WRITE_WORD      0x10    /* Write single word */
#define CFI_CMD_BLOCK_ERASE     0x20    /* Erase a single block */
#define CFI_CMD_BLOCK_Q_ERASE   0x28    /* Erase blocks using queuing */
#define CFI_CMD_WRITE_BYTE      0x40    /* Write single byte */
#define CFI_CMD_WRITE_BUFFER    0xE8    /* Write buffer */
#define CFI_CMD_CLEAR_STATUS    0x50    /* Clear status register */
#define CFI_CMD_READ_STATUS     0x70    /* Read status register */
#define CFI_CMD_QUERY           0x98    /* Enter CFI Query mode */
#define CFI_CMD_ERASE_SUSPEND   0xB0    /* Suspend the current erase operation */
#define CFI_CMD_RESUME          0xD0    /* Resume a suspended operation */
#define CFI_CMD_CONFIRM         0xD0    /* Confirm end of operation (same as resume) */
#define CFI_CMD_RESET           0xF0    /* AMD? reset to Read Array mode */
#define CFI_CMD_READ_ARRAY      0xFF    /* Enter Read Array mode (default) */


#define CFI_CMD_LOCK_BLOCK      0X60

/* CFI Status Register Values */
#define CFI_STATUS_BLOCK_LOCKED     0x02
#define CFI_STATUS_PROGRAM_SUSPENDED    0x04
#define CFI_STATUS_VPP_LOW          0x08
#define CFI_STATUS_PROGRAM_ERROR    0x10
#define CFI_STATUS_ERASE_ERROR      0x20
#define CFI_STATUS_ERASE_SUSPENDED  0x40
#define CFI_STATUS_READY            0x80

/* Intel StrataFlash specifics */
#define STRATA_BUFFER_SIZE      0x20
#define DEVICE_REGIONS          0x2
#define P30_STRATA_BUFFER_SIZE  0x40
/* Processors */
#define DAG_MPU_NONE		-1
#define DAG_MPU_37T_XSCALE	0
#define DAG_MPU_71S_IXP		1
#define DAG_MPU_COPRO_SMALL	2	/* Temporary we use the MPU region for the Copro on the new accelerated cards */
#define DAG_MPU_COPRO_BIG	3	/* Temporary we use the MPU region for the Copro on the new accelerated cards */
#define DAG_MPU_TOTAL		4	/* Number of Microprocessors */

/* Processor regions and offsets */
#define BOOT_REGION   1
#define KERNEL_REGION 2
#define FS_REGION     3
#define EMBEDDED_REGION  4
/* used to identify the COPRO regions */
#define COPRO_REGION  4


/* 3.7T XScale (4Mb available) */
#define BOOT_START    0x00000000
#define BOOT_SIZE     0x00020000	/* 128Kb */
#define KERNEL_START  0x00020000
#define KERNEL_SIZE   0x000E0000	/* 896Kb */
#define FS_START      0x00100000
#define FS_SIZE       0x00200000	/* 2Mb */
//this is used to define all CPU region if needed to write all at once 
#define _37T_EMBEDDED_SIZE 0x00400000

/* 7.1S IXP (8Mb available) */
#define IXP_BOOT_START    0x00000000
#define IXP_BOOT_SIZE     0x00020000	/* 128Kb */
#define IXP_KERNEL_START  0x00020000
#define IXP_KERNEL_SIZE   0x00180000	/* 1.5MB */
#define IXP_FS_START      0x001A0000
#define IXP_FS_SIZE       0x00260000	/* 2.5MB */
#define IXP_EMBEDDED_SIZE 0x00800000	/* 8MB */

/** Copro Images support for the new accelearted cards */
#define COPRO_CURRENT_START_SMALL 	0x00200000   /* starts at offset after the PCI stable image */
#define COPRO_CURRENT_SIZE_SMALL  	0x00200000   /* 2Mb size */
#define COPRO_STABLE_START_SMALL 	0x00600000   /* starts at offset after the PCI current image */
#define COPRO_STABLE_SIZE_SMALL  	0x00200000   /* 2Mb size */

/** Copro Images support for the new accelearted cards */
#define COPRO_CURRENT_START_BIG 0x00400000   /* starts at offset after the PCI stable image */
#define COPRO_CURRENT_SIZE_BIG 	0x00400000   /* 4Mb size */
#define COPRO_STABLE_START_BIG 	0x00c00000   /* starts at offset after the PCI current image */
#define COPRO_STABLE_SIZE_BIG  	0x00400000   /* 4Mb size */

/* Register offsets from ROM base */
#define DAGROM_DATA 0x0000      /* Data register offset */
#define DAGROM_KEY  0x0004      /* Key register offset */
#define DAGROM_ADDR 0x0008      /* Address register offset */
/*Added for the Version 2 of the ROM Controller.*/
#define DAGROM_REPROGRAM_CONTROL  0x0010
#define DAGROM_RESET_CONTROL      0x0014

#define DAGROM_SIZE     (rp->size)
#define DAGROM_SECTOR   (rp->sector)
#define DAGROM_BSTART   (rp->bstart)
#define DAGROM_BSIZE    (rp->bsize)
#define DAGROM_TSTART   (rp->tstart)
#define DAGROM_TSIZE    (rp->tsize)
#define DAGROM_XSTART   (rp->pstart)    /* processor boot image start */
#define DAGROM_XSIZE    (rp->psize)     /* processor boot image size */

#define DAGERRBUF_SIZE  4096
#define DAGSERIAL_SIZE  128
#define DAGSERIAL_HEADER_SIZE 8             /* magic 4 byte key, SIWD key */
#define DAGSERIAL_MAGIC_MARKER_VAL 0xABCD   /* the magic marker value */
#define DAGSERIAL_MAGIC_MARKER_SIZE 4       /* our magic byte marker */
#define DAGSERIAL_KEY_SIZE 4                /* key size */

/* Important: OFFSET's are from the start of a SECTOR */
#define DAGSERIAL_KEY_OFFSET DAGROM_SECTOR-(DAGSERIAL_SIZE)-(DAGSERIAL_KEY_SIZE)
#define DAGSERIAL_PACKAGE_SIZE DAGSERIAL_SIZE + DAGSERIAL_HEADER_SIZE
#define DAGSERIAL_PACKAGE_OFFSET DAGROM_SECTOR-(DAGSERIAL_PACKAGE_SIZE)
#define DAGSERIAL_OFFSET DAGROM_SECTOR-DAGSERIAL_SIZE

/* Can remove this macro once the code is stable */
#define MUX(X,Y) 
#define IMAGE_TABLE_OFFSET  248
#define SIGNATURE_FIELD_OFFSET 128
#define IMAGE_SIGN_FIELD_WIDTH 4
#define P30_PARAM_BLOCK_SIZE  32768
#define P30_DATA_BLOCK_SIZE   131072

#define POWER_ON_IMG_SEL_SIGN 0x000
#define POWER_ON_IMG_SEL_TBL  0x02


typedef struct romtab romtab_t;


typedef unsigned int (*romreadf_t) (romtab_t *rp, uint32_t saddr);
typedef void (*romwritef_t) (romtab_t *rp, uint32_t saddr, unsigned int val);
typedef int (*erasef_t) (romtab_t *rp, uint32_t saddr, uint32_t eaddr);
typedef int (*programf_t) (romtab_t *rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);

/* ---------------------------------------------------------*/
/* "Internal" data structures                               */

typedef struct rom_block_description
{
	//size of the sector in bytes or words
	int    size_region[10];
	//the actual number of sectors in each section 
	int    sectors[10]; 
	//the offset of the each section use for easy caluclations 
	int    region_offset[10];
	int    number_regions; 
} rom_block_description_t;

typedef struct addresstable
{
	uint32_t start_address;
	uint32_t end_address;
}addresstab;

typedef struct imagetable
{
	addresstab atable[32];
	uint32_t signature_field;
	uint32_t serial_number;
	uint32_t mac_address;
}imagetab;

struct romtab
{
	unsigned int romid;
	uint16_t     manufacturer;
	uint16_t     device;
	uint8_t*     iom;           /* IO memory pointer */
	uint32_t     base;           /* I/O memory map base offset */
	uint32_t     rom_version;    /* ROM revision number */
	uint16_t     device_code;    /* To restrict matches to a particular DAG card. */
	char         *ident;
	romreadf_t   romread;        /* ROM read function */
	romwritef_t  romwrite;       /* ROM write function */
	erasef_t     erase;          /* pstartROM erase function */
	programf_t   program;        /* ROM program function */
	unsigned int size;           /* size of the ROM in bytes */
	/* if sector is 0 then uses the extended structure */
	unsigned int sector;         /* sector size in bytes */
	unsigned int bstart;         /* Bottom half start */
	unsigned int bsize;          /* Bottom half size */
	unsigned int tstart;         /* Top half start */
	unsigned int tsize;          /* Top half size */
	unsigned int pstart;         /* Processor image start */
	unsigned int psize;          /* Processor image size */
	int          mpu_id;         /* Processor ID */
	int          mpu_rom;        /* The ROM used by the processor (0 or 1 currently) */
	/*newly added structure for version 2.will be used if sector is 0.*/
	rom_block_description_t rblock;
	imagetab   itable;
};	


#define INIT_ROMTAB(romid, devicecode, ident, erase, program, size, sector, bstart, bsize, tstart, tsize, xstart, xsize, mpu_id, mpu_rom)  \
{                                             \
	/*romid:*/      (romid),              \
	/*manufacturer:*/   -1,               \
	/*device:*/         -1,               \
	/*iom:*/            NULL,             \
	/*base:*/           -1,               \
	/*romversion:*/    -1,                \
	/*devicecode:*/ (devicecode),         \
	/*ident:*/     ident,                 \
	/*romread:*/   NULL,                  \
	/*romwrite:*/  NULL,                  \
	/*erase:*/      erase,                \
	/*program:*/    program,              \
	/*size :*/      (size),               \
	/*sector :*/    (sector),             \
	/*bstart :*/    (bstart),             \
	/*bsize :*/     (bsize),              \
	/*tstart :*/    (tstart),             \
	/*tsize : */    (tsize),              \
	/*pstart : */   (xstart),             \
	/*psize :*/     (xsize),              \
	/*mpu_id :*/    (mpu_id),             \
	/*mpu_rom :*/   (mpu_rom),            \
}


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Given a DAG file descriptor return its ROM descriptor */
romtab_t* dagrom_init(uint32_t dagfd, int rom_number, int hold_processor);
void dagrom_free(romtab_t* rt);
/*Set the method to reprogram the FPGA.*/
void dagrom_set_program_method(int method);
void dagrom_set_image_number(int number);
/* Erasure is by sector and starts at saddr and ends at eaddr */
int dagrom_erase(romtab_t* rp, uint32_t saddr, uint32_t eaddr);

unsigned int dagrom_check_srctgtsize(romtab_t* rp, uint32_t source_size );

/*Function to initialise the Power On Image selection field.Currently only for FPGA 0.*/
int set_power_on_img_selection_table(romtab_t *rp, int image_number,int fpga);

/* ROM Read */
void dagrom_readsector(romtab_t* rp, uint8_t* bp, uint32_t saddr);
int dagrom_read(romtab_t* rp, uint8_t* bp, uint32_t saddr, uint32_t eaddr);

/* ROM Program */
int dagrom_program(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);

/* SWID functions */
int dagswid_checkkey(romtab_t* rp, uint32_t iswidkey);
int dagswid_readkey(romtab_t* rp, uint32_t* iswidkey);
int dagswid_write(romtab_t* rp, uint8_t* swidbuf, uint32_t iswidlen, uint32_t iswidkey);
int dagswid_read(romtab_t* rp, uint8_t* swidbuf, uint32_t iswidlen);
int dagswid_erase(romtab_t* rp);
int dagswid_isswid(romtab_t* rp);

/* ROM Verify */
int dagrom_verify(romtab_t* rp, uint8_t* ibuf, uint32_t ibuflen, uint8_t* obuf, uint32_t obuflen, uint32_t saddr, uint32_t eaddr);

unsigned dagrom_romreg(romtab_t* rp);
unsigned dagrom_reprogram_control_reg(romtab_t* rp);
void dagrom_keywreg(romtab_t* rp, unsigned int val);

void dagrom_loadcurrent(uint32_t dagfd, romtab_t* rp);
void dagrom_loadstable(uint32_t dagfd);

/* Xilinx revision strings */
void dagrom_xrev(romtab_t* rp);
void dagrom_xrev_copro(romtab_t* rp);

/* XScale functions */
void dagrom_pbihold(romtab_t* rp);
void dagrom_pbirelease(romtab_t* rp);

/* XScale functions */
uint32_t dagrom_procsize(int target_processor, int region);
uint32_t dagrom_procstart(int target_processor, int region);
const char* dagrom_procname(int target_processor, int region);
/*This function is only for the version 2 of the Rom controller.Uses the entries in the image table.*/
bool dagrom_check_for_copro(romtab_t *rp);
/* Return the name of the processor as an asciiz string. */
const char* dagrom_mpuname(int mpu_id);

/* Set fast CFI mode: 1 => on, 0 => off. */
void dagrom_set_fast_cfi_mode(int mode);

/* check the pbi bus status on the 3.7T */
int dagrom_pbi_bus_status( romtab_t* rp );

/**
 * Read the mac address which corresponts to the port index
 * \param rp pointer to rom info
 * \param port_index port index
 * \return mac address 
 */
uint64_t dagrom_read_mac(romtab_t * rp, int port_index);
/**
 * Display the active configuration
 * Displays the active firmware and the serial number
 * \param rp pointer to rom info
 */
void dagrom_display_active_configuration(romtab_t * rp);

int ChangeFirmwareHalf(int dagfd, int half, int rom_version);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAG_ROMUTIL_H */
