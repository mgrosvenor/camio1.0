/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_romutil.c,v 1.209.2.2 2009/08/19 05:22:22 karthik Exp $
 */

/* File header. */
#include "dag_romutil.h"
/* Endace headers. */
#include "dagnew.h"
#include "dagreg.h"
#include "dagapi.h"
#include "dagutil.h"
#include "dag_platform.h"
 
#define CFI_NODELAY  1 /*This was added for bugs in the firmware.no longer needed.if defined no additional delays be used.*/
/* Unit variables. */
typedef struct
{
	uint8_t id;
	char* desc;
	
} rom_manufacturer_t;

static rom_manufacturer_t rom_manufacturer[] =
{
	{
		0x01, "Advanced Micro Devices"
	},
	{
		0x89, "Intel"
	},
	{
		0xbf, "Silicon Storage Technology"
	},
	{
		0xc2, "Macronix"
	},
	{
		0x00, "Unknown manufacturer"
	}
};
typedef struct
{
	uint32_t start;
	uint32_t size;
	char* name;

} processor_region_t;

/*Needs to be passed a param*/
int sector_start_addr[258];
void 
dagrom_print_serial(romtab_t *rp);
static processor_region_t processor_region[DAG_MPU_TOTAL][5] =
{
    {   /* DAG_MPU_37T_XSCALE */
        { 0,            0,             NULL         },
        { BOOT_START,	BOOT_SIZE,     "BOOT"       },
        { KERNEL_START, KERNEL_SIZE,	"KERNEL"     },
        { FS_START,     FS_SIZE,    	"FILESYSTEM" },
        { BOOT_START,   _37T_EMBEDDED_SIZE,    	"EMBEDDED" },

    },
    {   /* DAG_MPU_71S_IXP */
        { 0,                0,               NULL         },
        { IXP_BOOT_START,   IXP_BOOT_SIZE,   "BOOT"       },
        { IXP_KERNEL_START, IXP_KERNEL_SIZE, "KERNEL"     },
        { IXP_FS_START,     IXP_FS_SIZE,     "FILESYSTEM" },
        { IXP_BOOT_START,   IXP_EMBEDDED_SIZE,     "EMBEDDED" }
    },
    {   /* DAG_MPU_COPRO*/
        { 0,	0,	NULL	},
        { COPRO_CURRENT_START_SMALL, COPRO_CURRENT_SIZE_SMALL,	"COPRO_0"	},
        { COPRO_STABLE_START_SMALL, COPRO_STABLE_SIZE_SMALL,	"COPRO_1"	},
        { 0,	0,	"COPRO_2"	},
        { 0,	0,	"COPRO_3"	 }
    },
    {   /* DAG_MPU_COPRO_BIG*/
        { 0,	0,	NULL	},
        { COPRO_CURRENT_START_BIG, COPRO_CURRENT_SIZE_BIG,	"COPRO_BIG_0"	},
        { COPRO_STABLE_START_BIG,  COPRO_STABLE_SIZE_BIG,	"COPRO_BIG_1"	},
        { 0,	0,	"COPRO_BIG_2"	},
        { 0,	0,	"COPRO_BIG_3"	 }
    }

};

static char *processor_name[DAG_MPU_TOTAL] = {
		"Intel 80321 XScale",
		"Intel IXP2350",
		"Copro accelerated Small",
		"Copro accelerated Big"
};

typedef struct
{
	uint16_t id;
	char* desc;

} cfi_device_t;


static cfi_device_t cfi_device[] =
{
	{
		CFI_DEV_X8, "async x8 only"
	},
	{
		CFI_DEV_X16, "async x16 only"
	},
	{
		CFI_DEV_X8_X16, "async x8 and x16 via BYTE#"
	},
	{
		CFI_DEV_X32, "async x32 only"
	},
	{
		CFI_DEV_X16_X32, "async x16 and x32 via WORD#"
	},
	{
		0xFFFF, "Unknown device mode"
	}	/* List terminator */
};

typedef struct
{
	uint16_t id;
	char* desc;
} cfi_vendor_t;

static volatile bool data_sixteen_bits = false;
static volatile bool address_sixteen_bits = false;
static volatile unsigned int address_read_back = 0xffffffff;

static cfi_vendor_t cfi_vendor[] =
{
	{
		CFI_VENDOR_GENERIC, "Generic" /* i.e. no alternate vendor command set */
	},
	{
		CFI_VENDOR_INTEL_EXT, "Intel/Sharp" /* Intel/Sharp Extended Command Set */
	},
	{
		CFI_VENDOR_AMD_EXT, "AMD/Fujitsu/Macronix" /* AMD/Fujitsu/Macronix Standard Command Set */
	},
	{
		CFI_VENDOR_INTEL, "Intel" /* Intel Standard Command Set */
	},
	{
		CFI_VENDOR_AMD, "AMD/Fujitsu" /* AMD/Fujitsu Extended Command Set */
	},
	{
		CFI_VENDOR_MITSUBISHI, "Mitsubishi" /* Mitsubishi Standard Command Set */
	},
	{
		CFI_VENDOR_MITSUBISHI_EXT, "Mitsubishi" /* Mitsubishi Extended Command Set */
	},
	{
		CFI_VENDOR_SST, "SST" /* Page Write Command Set */
	},
	{
		0xFFFF, "Unknown vendor" /* List terminator */ 
	}
};

/* Read/Write attributes to the ROM, use these in preference to direct access to the romtab structure */
int dagrom_get_sectorsize(romtab_t* rp) { return rp->sector; }
int dagrom_get_size(romtab_t* rp) { return rp->size; }
int dagrom_get_bstart(romtab_t* rp) { return rp->bstart; }
int dagrom_get_bsize(romtab_t* rp) { return rp->bsize; }
int dagrom_get_tstart(romtab_t* rp) { return rp->tstart; }
int dagrom_get_tsize(romtab_t* rp) { return rp->tsize; }

/* Internal routines. */
static void lookup_romtab(romtab_t* rp, daginf_t * dinf);
static int identify_rom(romtab_t* rp);
static int cfi_identify(romtab_t* rp);
static int jedec_identify(romtab_t* rp);
static int cfi_read_blocks(romtab_t* rp);

static int sst_program(struct romtab *rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);
static int amd_program(struct romtab *rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);
static int cfi_program(struct romtab *rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);
static int cfi_program_v2(struct romtab *rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);
static int cfi_buffered_program(struct romtab *rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);
static int cfi_buffered_program_v2(struct romtab *rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr);

static int sector_range_calc(romtab_t *rp,uint32_t saddr,uint32_t eaddr,uint32_t *start_sector,uint32_t *end_sector);
static void sector_range_lock(romtab_t *rp,uint32_t saddr,uint32_t eaddr,bool lock);
static int sst_erase(struct romtab *rp, uint32_t saddr, uint32_t eaddr);
static int amd_erase(struct romtab *rp, uint32_t saddr, uint32_t eaddr);
static int cfi_erase(struct romtab *rp, uint32_t saddr, uint32_t eaddr);
static int cfi_erase_v2(struct romtab *rp, uint32_t saddr, uint32_t eaddr);
/* ROM read/write byte functions */
static void preprocess_romwrite_v0(struct romtab *rp, unsigned int addr, unsigned int val);
static unsigned preprocess_romread_v0(struct romtab *rp, unsigned int addr);
static void romwrite_v0(struct romtab *rp, unsigned int val);
static unsigned romread_v0(struct romtab *rp, unsigned int val);
static void romwrite_v1(struct romtab *rp, unsigned int addr, unsigned int val);
static unsigned romread_v1(struct romtab *rp, unsigned int addr);
/*Rom Controller Version 2*/
static void romwrite_v2(struct romtab *rp, unsigned int addr, unsigned int val);
static unsigned romread_v2(struct romtab *rp, unsigned int addr);

/* ROM CFI Status */
static uint8_t cfi_status(struct romtab *rp);

static char* cfi_device_str(uint16_t deviceId);
static char* cfi_vendor_str(uint16_t vendorId);
static char* manufacturer_str(uint8_t manufacturerId) __attribute__((unused));
static char* romid_str(uint16_t romid) __attribute__((unused));

/* Given a DAG. */
extern char* dag_xrev_name(unsigned char* xrev, unsigned int xlen);

/*
 * ROM table
 *
 * Most DAG cards use the same partitioning scheme for a given ROM.
 * For the exceptions there is the ability to match the ROM table entry
 * based on PCI device code as well as the ROM id.
 *
 * The matching logic will use the first ROM table entry that satisfies the following conditions:
 *
 *     1. The 'romid' field matches the id for the physical ROM.
 *     2. EITHER the 'device_code' field in the entry matches that of the daginf structure;
 *            OR the 'device_code' field in the entry is 0.
 *
 * Practically, this means that ROMs that have the same partitioning scheme across all DAG
 * cards can have a single entry with a device_code of 0, and ROMs that are used differently
 * in different DAG cards should have specific (nonzero device_code) entries first followed
 * by a general (device_code of 0) entry.
 */
static romtab_t romtab[] =
{
	/* Am29LV017D 16MBit as used in the DAG 4.4GE "Extreme Edition". */
	/* DAG 4.4GE "Extreme Edition" specific entry. */
	INIT_ROMTAB(0x01c8, 0x440e, "Am29LV017D 16MBit (for DAG 4.4GE)", amd_erase, amd_program, (2048 * 1024), (64 * 1024), 0, (2048 * 1024) / 2, (2048 * 1024) / 2, (2048 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),

	/* Am29LV017D 16MBit as used in the DAG 3.7G. */
	/* DAG 3.7G specific entry. */
	INIT_ROMTAB(0x01c8, 0x378e, "Am29LV017D 16MBit (for DAG 3.7G)", amd_erase, amd_program, (2048 * 1024), (64 * 1024), 0, (2048 * 1024) / 2, (2048 * 1024) / 2, (2048 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),

	/* Am29LV017D 16MBit as used in the DAG 3.7D. */
	/* DAG 3.7D specific entry. */
	INIT_ROMTAB(0x01c8, 0x370d, "Am29LV017D 16MBit (for DAG 3.7D)", amd_erase, amd_program, (2048 * 1024), (64 * 1024), 0, (2048 * 1024) / 2, (2048 * 1024) / 2, (2048 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),

	/* mx29LV017b 16MBit as used in the DAG 4.4GE "Extreme Edition". */
	/* DAG 4.4GE "Extreme Edition" specific entry. */
	INIT_ROMTAB(0xc2c8, 0x440e, "MX29LV017B 16MBit (for DAG 4.4GE)", amd_erase, amd_program, (2048 * 1024), (64 * 1024), 0, (2048 * 1024) / 2, (2048 * 1024) / 2, (2048 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),

	/* mx29LV017b 16MBit as used in the DAG 3.7G. */
	/* DAG 3.7G specific entry. */
	INIT_ROMTAB(0xc2c8, 0x378e, "MX29LV017B 16MBit (for DAG 3.7G)", amd_erase, amd_program, (2048 * 1024), (64 * 1024), 0, (2048 * 1024) / 2, (2048 * 1024) / 2, (2048 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),

	/* mx29LV017b 16MBit as used in the DAG 3.7D. */
	/* DAG 3.7D specific entry. */
	INIT_ROMTAB(0xc2c8, 0x370d, "MX29LV017B 16MBit (for DAG 3.7D)", amd_erase, amd_program, (2048 * 1024), (64 * 1024), 0, (2048 * 1024) / 2, (2048 * 1024) / 2, (2048 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),

    /* 32Mbit version of the above Macronix chip */
	INIT_ROMTAB(0xc2a3, 0x440e, "MX29LV033C 32MBit (for DAG 4.4GE)", amd_erase, amd_program, (4096 * 1024), (64 * 1024), 0, (4096 * 1024) / 2, (4096 * 1024) / 2, (4096 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0xc2a3, 0x378e, "MX29LV033C 32MBit (for DAG 3.7G)",  amd_erase, amd_program, (4096 * 1024), (64 * 1024), 0, (4096 * 1024) / 2, (4096 * 1024) / 2, (4096 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0xc2a3, 0x370d, "MX29LV033C 32MBit (for DAG 3.7D)",  amd_erase, amd_program, (4096 * 1024), (64 * 1024), 0, (4096 * 1024) / 2, (4096 * 1024) / 2, (4096 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
    
    /* DAG 4.5 2 port GE specific entry. */
	INIT_ROMTAB(0x8917, 0x452e, "Intel 28F640J3 64MBit (for DAG 4.5 2 port GE)", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00200000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),

    /* DAG 4.588 2 port GE specific entry. */
	INIT_ROMTAB(0x8917, 0x4588, "Intel 28F640J3 64MBit (for DAG 4.588 2 port GE)", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00200000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),

    /* DAG 4.5 4 port GE specific entry. */
	INIT_ROMTAB(0x8917, 0x454e, "Intel 28F640J3 64MBit (for DAG 4.5 4 port GE)", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00200000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),

	/* Specific entry for the DAG 3.7T. */
	INIT_ROMTAB(0x8917, 0x3707, "Intel 28F640J3 64MBit (for DAG 3.7T)", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x01c00000, 0x00200000, 0x01e00000, 0x00200000, 0x00000000, 0x00200000, DAG_MPU_37T_XSCALE, 0),
	INIT_ROMTAB(0x8917, 0x3747, "Intel 28F640J3 64MBit (for DAG 3.7T4)", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x01c00000, 0x00200000, 0x01e00000, 0x00200000, 0x00000000, 0x00200000, DAG_MPU_37T_XSCALE, 0),
	INIT_ROMTAB(0x8917, 0x7100, "Intel 28F640J3 64MBit (for DAG 7.1)", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00200000, 0x00200000, 0x00000000, 0x00400000, DAG_MPU_71S_IXP, 1),

	/* DAG 8.0SX has larger firmware images 4MB instead of 2MB */
	INIT_ROMTAB(0x8917, 0x8000, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
	/* DAG 8.11SX has larger firmware images 4MB instead of 2MB not the 8.1 is using the default images down*/
	INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG8_101, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
   	 /*  DAG 8.1X  8100 card*/
    	INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG8_100, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00200000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
    	/*  DAG 8.1X  8102 card*/
    	INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG8_102, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00200000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
	/* DAG 8.4XI has firmware images 4MB */
	INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG8_400, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG8_400, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
    INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG8_500, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00800000, 0x00800000, 0x00800000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
        
	/* DAG 5.0SX has larger firmware images 4MB instead of 2MB
	Original Rev A is 2Mb but the new REV A Images should be changed to 
	4Mb  
	Rev B images are always 4MB 
	*/
	INIT_ROMTAB(0x8917, 0x5000, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG5_00S, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG5_0Z, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG5_0DUP, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),

    /* For DAG 5.4S12 */
    INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG5_4S12, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),

    /* For DAG 5.4SG48 */
    INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG5_4SG48, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),

	/* DAG 4.5G2A has 4 Firmware images 2 for the carrier and 2 for the copro 
	this is a new compare to the 4.3 which was using the dagld to load image in the copro 
	Here we put that the two halfs take half of the iamge and the top half reserv it for 
	embedded CPU or second FPGA
	*/
	/* Quick fix we assign the rom to the cpu region and from there we use two parts*/
	INIT_ROMTAB(0x8917, 0x452A, "Intel 28F640J3 64MBit" , cfi_erase, cfi_program, (  8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00400000, 0x00200000, 0x00000000, 0x00800000, DAG_MPU_COPRO_SMALL, 0),
	INIT_ROMTAB(0x8918, 0x520A, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),
	INIT_ROMTAB(0x8918, 0x521A, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),

	/* the 5.0sg2 accelerated crads for the DUP and normal image TODO: add 5.0sg2az if required*/
	
	INIT_ROMTAB(0x8918, 0x502A, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),
	INIT_ROMTAB(0x8918, 0x50DA, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),

    /* for DAG 5.0Sg3A ( 0x503a) */
    INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG5_0SG3A, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),
    /* for DAG 5.0S3A hacked version for 64Mbit rom small one use the stable half as PCI and use the current haf as Copro do not use the copro options ( 0x503a) */
    INIT_ROMTAB(0x8917, PCI_DEVICE_ID_DAG5_0SG3A, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00400000, 0x00400000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),

    /* for dag 5.4GA*/
    INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG5_4GA, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),
      
    /* for dag 5.4SA12*/
    INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG5_4SA12, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),
      
     /* for dag 5.4SGA48*/
     INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG5_4SGA48, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),

#if 0
		  /*this cards are using nice and funky version of the ROM Contoller 2 and do not need this table 
		  it is populated automaticly at init */
		   
/* For DAG 7.5G2*/
        INIT_ROMTAB(0x891c, PCI_DEVICE_ID_DAG_7_5G2, "Intel 28F256P30 256MBit", cfi_erase_v2, cfi_program_v2, (4*8192 * 1024),0, 0x00040000,0x00200000, 0x001c40000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
/* For DAG 7.5G4*/
        INIT_ROMTAB(0x891c, PCI_DEVICE_ID_DAG_7_5G4, "Intel 28F256P30 256MBit", cfi_erase_v2, cfi_program_v2, (4*8192 * 1024),0, 0x00040000,0x00200000, 0x001c40000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
/* For DAG 7.5G4*/
        INIT_ROMTAB(0x891c, PCI_DEVICE_ID_DAG_8_3E, "Intel 28F256P30 256MBit", cfi_erase_v2, cfi_program_v2, (4*8192 * 1024),0, 0x00040000,0x00200000, 0x001c40000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
#endif
#if 0	
    /* DAG 8.5XID has firmware images 4MB */
	INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG8_500, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00000000, 0x00400000, 0x00800000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),
	/*For the copro image.*/
 
	INIT_ROMTAB(0x8918, PCI_DEVICE_ID_DAG8_500, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (2*8192 * 1024), (128 * 1024), 0x00400000, 0x00400000, 0x00c00000, 0x00400000, 0x00000000, 0x01000000, DAG_MPU_COPRO_BIG, 0),
#endif 
								         
    /*****************************************************************
     * card-independent entries are moved to the end of the table to *
     * avoid accidental override of card-specific entries (search
     * algorithm quits on first match) */

	INIT_ROMTAB(0xbfd7, 0x0000, "SST39LF/VF040 4Mbit", sst_erase, sst_program, (512 * 1024), (4 * 1024), 0, (512 * 1024) / 2, (512 * 1024) / 2, (512 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x0138, 0x0000, "Am29LV081B 8MBit", amd_erase, amd_program, (1024 * 1024), (64 * 1024), 0, (1024 * 1024) / 2, (1024 * 1024) / 2, (1024 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x014f, 0x0000, "Am29LV040B 4MBit", amd_erase, amd_program, (512 * 1024), (64 * 1024), 0, (512 * 1024) / 2, (512 * 1024) / 2, (512 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x01c8, 0x0000, "Am29LV017D 16MBit", amd_erase, amd_program, (1024 * 1024), (64 * 1024), 0, (1024 * 1024) / 2, (1024 * 1024) / 2, (1024 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0xc2c8, 0x0000, "MX29LV017B 16MBit", amd_erase, amd_program, (1024 * 1024), (64 * 1024), 0, (1024 * 1024) / 2, (1024 * 1024) / 2, (1024 * 1024) / 2, 0, 0, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x8916, 0x0000, "Intel 28F320J3 32MBit", cfi_erase, cfi_program, (4096 * 1024), (128 * 1024), 0x01c00000, 0x00200000, 0x01e00000, 0x00200000, 0x00000000, 0x00200000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x8917, 0x0000, "Intel 28F640J3 64MBit", cfi_erase, cfi_program, (8192 * 1024), (128 * 1024), 0x00000000, 0x00200000, 0x00200000, 0x00200000, 0x00000000, 0x00000000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x8918, 0x0000, "Intel 28F128J3 128MBit", cfi_erase, cfi_program, (16384 * 1024), (128 * 1024), 0x01c00000, 0x00200000, 0x01e00000, 0x00200000, 0x00000000, 0x00200000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x891d, 0x0000, "Intel 28F256J3 256MBit", cfi_erase, cfi_program, (32768 * 1024), (128 * 1024), 0x01c00000, 0x00200000, 0x01e00000, 0x00200000, 0x00000000, 0x00200000, DAG_MPU_NONE, 0),
	INIT_ROMTAB(0x0000, 0x0000, "(null)", NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, DAG_MPU_NONE, 0)
};

/* Boolean to enable fast CFI program mode */
static int uFastCfiMode = 1;   /* default: on */
/*The method to be used to reprogram the FPGA.default method is DAVE*/
static int program_method = 3;
static int image_number = 1;
static uint32_t orig_pbi_state = 0x11;

/* --------------------------------------------------------- */
/* ROM byte read/write operations                           */
static void
preprocess_romwrite_v0(romtab_t* rp, unsigned int addr, unsigned int val)
{
	romwrite_v0(rp, addr << 8 | val);
}


static unsigned
preprocess_romread_v0(romtab_t* rp, unsigned int addr)
{
	return romread_v0(rp, addr << 8);
}


/* write to a version 0 ROM Controller */
static void
romwrite_v0(romtab_t* rp, unsigned int val)
{
	if (dagutil_get_verbosity() > 2)
		dagutil_verbose_level(4, "romwrite_0: %.8x", val);

	*(unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = val;
	dagutil_nanosleep(1500);
}


/* read from a version 0 ROM Controller */
static unsigned
romread_v0(romtab_t* rp, unsigned int val)
{
	if (dagutil_get_verbosity() > 2)
		dagutil_verbose_level(4, "romread_0: %.8x\n", val);

	*(unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = (BIT28 | val);
	dagutil_nanosleep(1500);

	return (*(unsigned int*) (rp->iom + rp->base + DAGROM_DATA) & 0xff);
}


/* write to a version 1 ROM Controller */
static void
romwrite_v1(romtab_t* rp, unsigned int addr, unsigned int val)
{
	if (dagutil_get_verbosity() > 2)
		dagutil_verbose_level(4, "romwrite: %.8x %.8x\n", addr, val);

	*(unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = addr;
	*(unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = val;
	dagutil_nanosleep(500);
}

/* write to a version 2 ROM Controller */
static void
romwrite_v2(romtab_t* rp, unsigned int addr, unsigned int val)
{
	/*address 8 bits data 8 bits*/
	if((data_sixteen_bits == false) && (address_sixteen_bits == false))
	{
		if((address_read_back & 0xffff) == 0xffff)
		{
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romwrite: %.8x %.8x\n",addr, val);
		
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = addr;
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = val;
		}
		else	
		{
			dagutil_error("This configuration not supported.Please contact support@endace.com \n");
		}
	}
	/*address 16 bits data 8 bits*/	
	else if ((data_sixteen_bits == false) && (address_sixteen_bits == true))
	{
		dagutil_error("This configuration not supported.Please contact support@endace.com \n");	
	}
	/*address 8 bits data 16 bits*/
	else if ((data_sixteen_bits == true) && (address_sixteen_bits == false))
	{
		if((address_read_back & 0xffff) == 0xffff)
		{
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romwrite: %.8x %.8x\n",addr, val);
		
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = addr;
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = val;
			

		}
		else if((address_read_back & 0xffff) == 0xfffe)
		{
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romwrite: %.8x %.8x\n",addr, val);
		
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = (addr);
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = val;
		}
	}
	/*address 16 bits data 16 bits*/	
	else if ((data_sixteen_bits == true) && (address_sixteen_bits == true))
	{
		if((address_read_back & 0xffff) == 0xffff)
		{	
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romwrite: %.8x %.8x\n",addr, val);
			
				*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) =  addr >> 1;
				*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) =  val;
		}
		else if((address_read_back & 0xffff) == 0xfffe)
		{
			dagutil_error("This configuration not supported.Please contact support@endace.com \n");
		}
	}
}
/* read from a version 1 ROM Controller */
static unsigned int
romread_v1(romtab_t* rp, unsigned int addr)
{
	unsigned int val;

	*(unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = addr;
	*(unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = (BIT28);

	dagutil_nanosleep(500);
	
	val = (*(unsigned int*) (rp->iom + rp->base + DAGROM_DATA) & 0xff);
	if (dagutil_get_verbosity() > 2)
		dagutil_verbose_level(4, "romread(addr,val): 0x%.8x 0x%.8x\n", addr,val);

	return val;
}
/* read from a version 2 ROM Controller */
static unsigned int
romread_v2(romtab_t* rp, unsigned int addr)
{
	unsigned int val = 0;
	
	/*address 8 bit data 8 bit*/
	if((data_sixteen_bits == false) && (address_sixteen_bits == false))
	{
		
		/*for the time being we are bothered about only first 16 bits*/	
		if((address_read_back & 0xffff) == 0xffff)
		{
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = addr;
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = (BIT28);

			val = (*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) & 0xff);
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romread(addr,val): 0x%.8x 0x%.8x\n", addr,val);
		}
		else /*address_read_back = 0xfffe*/
		{
			dagutil_error("Configuration not supported \n");
		}
	
	}
	/*address 16 bits data 8 bits*/	
	else if ((data_sixteen_bits == false) && (address_sixteen_bits == true))
	{
		dagutil_error("This configuration not supported.Please contact support@endace.com \n");
	}
	/*address 8 bits data 16 bits*/
	else if ((data_sixteen_bits == true) && (address_sixteen_bits == false))
	{
		if((address_read_back & 0xffff) == 0xffff)
		{
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = addr;
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = (BIT28);

			val = (*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) & 0xff);
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romread(addr,val): 0x%.8x 0x%.8x\n", addr,val);	
		}
		else if((address_read_back & 0xffff) == 0xfffe)
		{	/*BIT 0 is invalid shifit the address by 1 to left*/
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = (addr);
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) = (BIT28);
			
			val = (*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) & 0xffff);
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romread(addr,val): 0x%.8x 0x%.8x\n", addr,val);	
		}		
	}
	/*address 16 bits data 16 bits*/	
	else if ((data_sixteen_bits == true) && (address_sixteen_bits == true))
	{
		if((address_read_back & 0xffff) == 0xffff)
		{
			*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = (addr);
	        	*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_ADDR) = (BIT28);

			dagutil_nanosleep(1000);

			val = (*(volatile unsigned int*) (rp->iom + rp->base + DAGROM_DATA) & 0xffff);		
			if (dagutil_get_verbosity() > 2)
				dagutil_verbose_level(4, "romread(addr,val): 0x%.8x 0x%.8x\n", addr,val);
		}
		else if ((address_read_back & 0xffff) == 0xfffe)
		{
			/*essentially we are talking about addressing 32 bits.crash!!!*/
			dagutil_error("This configuration not supported.Please contact support@endace.com \n");
		}
	}
	return val;
}
/*  initrom : Given a valid DAG descriptor initialize a supplied 
 *  reference to a ROM descriptor.
 */
romtab_t*
dagrom_init(uint32_t dagfd, int rom_number, int hold_processor)
{
	dag_reg_t result[DAG_REG_MAX_ENTRIES];
	dag_reg_t *regs;
	daginf_t *dinf;
	romtab_t* rp;
	unsigned int regn = 0;
	int loop = 0;
	uint16_t val;
	uint16_t val1,val2;

	rp = malloc(sizeof(romtab_t));
	memset( (void*)rp, '\0', sizeof(romtab_t) );

	regs = dag_regs(dagfd);
	rp->iom = dag_iom(dagfd);
	dinf = dag_info(dagfd);

	if (hold_processor)
		dagrom_pbihold(rp);

	if ((dag_reg_table_find(regs, 0, DAG_REG_ROM, result, &regn)) || (!regn))
	{
		//if (hold_processor) pbi_release();
		dagutil_panic("Error ROM Controller enumeration not found\n");
		errno = ENODEV;
		return NULL;
	}

	if (rom_number >= regn)
	{
		dagutil_panic("only %d available ROM controller(s)\n", regn);
		errno = ENODEV;
		return NULL;
	}

	rp->base = DAG_REG_ADDR(result[rom_number]);
	rp->rom_version = DAG_REG_VER(result[rom_number]);
	dagutil_verbose("ROM version %d, base 0x%04x\n", rp->rom_version, rp->base);

	/* Check for older version 1 image - version not updated, but base
	 * address moved from 0x158 to 0x150.  Maybe.
	 * To Do: Confirm that the 0x158 address is correct for rev 0.
	 */
	if ((rp->rom_version == 0) & (rp->base == 0x150))
	{
		dagutil_verbose("detected unversioned REV 1 ROM Controller\n");
		rp->rom_version = 1;
	}

	/*This should be done only once.and this is expected to increase the speed by atleaset 50%.*/

	*(volatile unsigned int*)(rp->iom + rp->base + DAGROM_ADDR) = address_read_back;
	address_read_back = (*(volatile unsigned int*)(rp->iom + rp->base + DAGROM_ADDR));
	
	data_sixteen_bits    = (*(volatile unsigned int*)(rp->iom + rp->base + DAGROM_DATA) & BIT18) >> 18;
	address_sixteen_bits = (*(volatile unsigned int*)(rp->iom + rp->base + DAGROM_DATA) & BIT19) >> 19;

	/* Initialize read/write ROM functions */
	if (rp->rom_version == 0)
	{
		rp->romread = preprocess_romread_v0;
		rp->romwrite = preprocess_romwrite_v0;
	}
	else if(rp->rom_version == 1)
	{
		rp->romread = romread_v1;
		rp->romwrite = romwrite_v1;
	}
	else if(rp->rom_version == 2)
	{
		rp->romread = romread_v2;
		rp->romwrite = romwrite_v2;
		rp->program = cfi_program_v2;
		rp->erase = cfi_erase_v2;	
	}

	/* After detecting the ROM, identify the ROM manufacturer and device 
	 * This sets the value of the romid, which is a unique value made up
	 * of the manufacturer id and the device id.
	 */
	if (identify_rom(rp) < 0)
	{
		dagutil_verbose("failed to identify ROM controller\n");
		free(rp);
		errno = ENODEV;
		return NULL;
	}

	if( rp->rom_version != 2 ) 
	{
		/* Given a valid romid lookup the appropriate romtab */
		lookup_romtab(rp, dinf);
	}

	/*Read from the Rom and Populate the table*/
	/*Start address rp->size - 256 + 8*/
	val = 0;
	val1 = 0;
	val2 = 0;
    if(rp->rom_version == 2)
    {
	    uint32_t size = ( (rp->rblock.sectors[0] * rp->rblock.size_region[0]) + (rp->rblock.sectors[1]* rp->rblock.size_region[1]) );
	    uint16_t start_address,end_address = 0;
	    for (loop = 0;loop < 16;loop ++)
	    {
		    val = rp->romread(rp,(size - (IMAGE_TABLE_OFFSET - loop*2)));
		    end_address =  val&0xff00;
	    	    end_address = (end_address >> 8);
		    start_address =  val&0x00ff;
		    rp->itable.atable[loop].start_address = (start_address << 18);
		    rp->itable.atable[loop].end_address = ((end_address << 18)); /*end_address << 18 is not a part of this space.the actual end_address is (end_address << 18) - 1*/
		    if(dagutil_get_verbosity() > 0)
		    {
			    if(rp->itable.atable[loop].start_address != 0)
				    printf("start address %x end address %x \n",rp->itable.atable[loop].start_address,rp->itable.atable[loop].end_address);
		    }
		    rp->mpu_id = -1; /*mpu none we dont care for this variable from V2 onwards.only for image checking.*/
	    }
	    val1 = rp->romread(rp,(size - 256));
	    val2 = rp->romread(rp,(size - 256 + 2));
	    rp->itable.signature_field = (val1 << 16) | val2;

        if(rp->itable.signature_field == 0xf9f8fbfa)
	    {
	    	dagutil_verbose("The ROM contains a valid Image Table \n");
	    }
	    else
	    {
		    dagutil_verbose("Image Table in the ROM is invalid \n");	
	    }
    }
	return rp;
}


/*  dagrom_free : Given a valid DAG descriptor initialize a supplied 
 *  reference to a ROM descriptor.
 */
void
dagrom_free(romtab_t* rp)
{
	free(rp);
}


/* lookup_romtab : Given the romid initialize the rest of romtab fields to the 
 * entries in the global static romtab list.
 * Args : rp - pointer to a romtab structure holding the 
 */
static void
lookup_romtab(romtab_t* rp, daginf_t * dinf)
{
	romtab_t* rptmp;

#ifndef NDEBUG
	dagutil_verbose("searching for romtab with id of %d \n", rp->romid);
#endif /* NDEBUG */

	for (rptmp = romtab; rptmp->romid != 0x0000; rptmp++)
	{
		if (rptmp->romid == rp->romid)
		{
			if ((rptmp->device_code == dinf->device_code) || (0 == rptmp->device_code))
			{
				rp->erase = rptmp->erase;
				rp->program = rptmp->program;
				rp->size = rptmp->size;
				rp->ident = rptmp->ident;
				rp->sector = rptmp->sector;
				rp->bstart = rptmp->bstart;
				rp->bsize = rptmp->bsize;
				rp->tstart = rptmp->tstart;
				rp->tsize = rptmp->tsize;
				rp->pstart = rptmp->pstart;
				rp->psize = rptmp->psize;
				rp->device_code = dinf->device_code;
				rp->mpu_id = rptmp->mpu_id;
				rp->mpu_rom = rptmp->mpu_rom;

#ifndef NDEBUG
				dagutil_verbose("device code (daginf) 0x%.4x\n", dinf->device_code);
				dagutil_verbose("rprog %s\n", rp->ident);
#endif /* NDEBUG */

				return;
			}
		}
	}
	dagutil_verbose("Could not identify EEPROM.  Please contact <support@endace.com> for assistance. (Card returned M%.2x D%.2x)\n", rp->manufacturer, rp->device);
}


/*  identify_rom : Given the a valid descriptor to a DAG card identify_rom
 *  will return a pointer to a structure containing the characteristics of
 *  the rom. 
 *  
 *  Args : romtab_t* rp - Pointer to a partially filled romtab structure 
 *  Returns : -1   - Unable to identify ROM device
 *            0    - Device supported.
 *  Note : We attempt to identify the rom using different ROM protocols. We 
 *  currently support SST and AMD parts, interestingly the AMD chip responds 
 *  properly to the SST command sequence, this is not a given from the specs.
 */
static int
identify_rom(romtab_t* rp)
{
	/* Try using CFI to identify the ROM */
	if (cfi_identify(rp) == 0)
		return 0;

	if (jedec_identify(rp) < 0)
	{
		dagutil_warning("Failed to identify the ROM device \n");
         	return 0;
	}

	return 0;
}
int 
set_power_on_img_selection_table(romtab_t *rp, int image_number,int fpga)
{
	/*Right now we have only one fpga. FPGA 0.*/
	uint16_t val,val1 = 0;
	uint32_t addr = 0;
	uint16_t value[4];

	val1 = rp->romread(rp,POWER_ON_IMG_SEL_SIGN);
	if(val1 != 0xfdfc)
	{
		dagutil_warning("Power On - Image Selection Table does not have a valid signature.\n");
		return -1;
	}


	val = rp->romread(rp,POWER_ON_IMG_SEL_TBL);
	/*case 1: set the imge number param to image of FPGA 0 after checking for 00.*/
	switch (fpga)
	{
		case 0:
			if(((val & 0xc) >> 2) == 0)
			{
				if(image_number > 3)
					dagutil_warning("Invalid image number entered.Please enter valid image number.\n");
				val &= 0xfffc; /*Erase the last two bits.We are going to OR the result.*/
				val |= ((image_number & 0x3) >> 0);
			}else
			{
				dagutil_warning ("FPGA 0 will not be programmed at power up cycle\n");
			}
			break;

		case 1: 
			if(((val & 0xc0) >> 6) == 1)
			{
				if(image_number > 3)
					dagutil_warning("Invalid image number entered.Please enter valid image number.\n");
				val &= 0xffcf; /*Erase the bits 6 and 7.We are going to OR the result.*/
				val |= ((image_number & 0x30) >> 4);
				
			}else
			{
				dagutil_warning ("FPGA 1 will not be programmed at power up cycle\n");
			}
			break;
	
		case 2:
			if(((val & 0xc00) >> 10) == 2)
			{
				if(image_number > 3)
					dagutil_warning("Invalid image number entered.Please enter valid image number.\n");
				val &= 0xfcff; /*Erase the bits 10 and 11.We are going to OR the result.*/
				val |= ((image_number & 0x300) >> 8);
				
			}else
			{
				dagutil_warning ("FPGA 2 will not be programmed at power up cycle \n");
			}
			break;

		case 3:
			if(((val & 0xc000) >> 14) == 3)
			{
				if(image_number > 3)
					dagutil_warning("Invalid image number entered.Please enter valid image number.\n");
				val &= 0xcfff; /*Erase the bits 14 and 15.We are going to OR the result.*/
				val |= ((image_number & 0x3000) >> 12);
				
			}else
			{
				dagutil_warning ("FPGA will not be programmed at power up cycle \n");
			}
			break;
		default:
			break;
	
	}
        /*The value has been modified appropriately.write it back.*/
	/*erase the sector 0. it does not contain anything else except this.*/
	/*Unlock the block.*/
	rp->romwrite(rp,addr,0x60);
	rp->romwrite(rp,addr,0xD0);

	/*erase the block.*/
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
	rp->romwrite(rp, addr, CFI_CMD_BLOCK_ERASE);
	rp->romwrite(rp, addr, CFI_CMD_RESUME);

	dagutil_microsleep(20); /* T bp = 20 usecs */

	while (!(cfi_status(rp) & CFI_STATUS_READY))
	{
		/* Wait for the erase to complete */
	}
	dagutil_verbose("erase 0x%.8x sector\r", addr);

	/*Lock the block again*/
	rp->romwrite(rp,addr,0x60);
	rp->romwrite(rp,addr,0x01);

	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);

	rp->romwrite(rp,addr,0x60);
	rp->romwrite(rp,addr,0xD0);
	
	value[0] = val1;
	value[1] = val;
	rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_CLEAR_STATUS);
	rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_WRITE_BYTE);

	rp->romwrite(rp,addr,value[0]);
	
	while(!(cfi_status(rp) & CFI_STATUS_READY));
	
	rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_CLEAR_STATUS);
	rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_WRITE_BYTE);

	rp->romwrite(rp,(addr+2),value[1]);
	
	while(!(cfi_status(rp) & CFI_STATUS_READY));

	rp->romwrite(rp,addr,0x60);
	rp->romwrite(rp,addr,0x01);
	
	return 0;
}
int 
read_next_boot_image(romtab_t *rp,int fpga)
{
	/*Right now we have only one fpga. FPGA 0.*/
	uint16_t val = 0;
	val = rp->romread(rp,POWER_ON_IMG_SEL_SIGN);
	if(val != 0xfdfc)
	{
		dagutil_warning("Power On - Image Selection Table does not have a valid signature.\n");
		return -1;
	}


	val = rp->romread(rp,POWER_ON_IMG_SEL_TBL);
	/*case 1: set the imge number param to image of FPGA 0 after checking for 00.*/
	switch (fpga)
	{
		case 0:
			if(((val & 0xc) >> 2) == 0)
			{
					return (val & 0x3);
			}else
			{
				dagutil_warning ("FPGA 0 will not be programmed at power up cycle \n");
			}
			break;

		case 1: 
			if(((val & 0xc0) >> 6) == 1)
			{
				return ( (val & 0x30) >> 4);
				
			}else
			{
				dagutil_warning ("FPGA 1 will not be programmed at power up cycle \n");
			}
			break;
	
		case 2:
			if(((val & 0xc00) >> 10) == 2)
			{	
				return ((val&0x300) >> 8);
			}else
			{
				dagutil_warning ("FPGA 2 will not be programmed at power up cycle \n");
			}
			break;

		case 3:
			if(((val & 0xc000) >> 14) == 3)
			{
				return ((val & 0x3000) >> 12);
				
			}else
			{
				dagutil_warning ("FPGA will not be programmed at power up cycle \n");
			}
			break;
		default:
			return -1;
			break;
		}
	return -1;
}


/* cfi_identify : Try a CFI query to identify the flash device.
 *
 * Args : romtab_t* rp - Pointer to a partially filled romtab structure
 * Returns : ENODEV - CFI not supported (try JEDEC)
 *           OK - CFI supported
 * Note : If CFI is supported then the device is left in CFI query
 *        mode when this function returns.
 *        *Important* This function must not be called unless the argument
 *        rp has been initialized via a call to initROM.
 *
 */
static int
cfi_identify(romtab_t* rp)
{
	uint16_t vendorId = 0xFFFF; /* returns as 0xFFFF if no CFI device found */
	uint8_t size = 0;
	uint16_t deviceId = 0;

	/* Send a CFI Query */
	rp->romwrite(rp, CFI_QUERY_ADDR, CFI_CMD_QUERY);
	dagutil_microsleep(1); /* T ida = 150 nanoseconds */

#if 0
	uint8_t data;

	data = romread(MUX(CFI_8_Q, 0));
	printf("cfi_identify-%d: CFI_8_Q = 0x%02x\n", __LINE__, data);
	data = romread(MUX(CFI_8_R, 0));
	printf("cfi_identify-%d: CFI_8_R = 0x%02x\n", __LINE__, data);
	data = romread(MUX(CFI_8_Y, 0));
	printf("cfi_identify-%d: CFI_8_Y = 0x%02x\n", __LINE__, data);
#endif

	/* Verify that the chip supports CFI */
	/*The output of ROM READ has been ANDED WITH ff to make it compliant with Version 2 of the ROM CONTROLLER.
	The offsets are idendical to that of 8 bit device.*/
	if (((rp->romread(rp, CFI_8_Q) & 0xff) == 'Q') && ((rp->romread(rp, CFI_8_R) & 0xff)== 'R') && ((rp->romread(rp, CFI_8_Y) & 0xff)== 'Y'))
	{
		/* 8-bit device */
		dagutil_verbose("cfi_identify %d: Detected 8-bit device\n", __LINE__);

		/* Get vendor and id */
		vendorId = rp->romread(rp, CFI_8_VENDOR_LO) | rp->romread(rp, CFI_8_VENDOR_HI) << 8;
		size = rp->romread(rp, CFI_8_SIZE);
		deviceId = rp->romread(rp, CFI_8_DEVICE_LO) | rp->romread(rp, CFI_8_DEVICE_HI) << 8;

		/* Back to read-array mode */
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_RESET); /* For AMD */
	}
	else if ((rp->romread(rp, CFI_16_8_Q) == 'Q') && (rp->romread(rp, CFI_16_8_R) == 'R') && (rp->romread(rp, CFI_16_8_Y) == 'Y'))
	{

		/* 16-bit device in 8-bit mode */
		dagutil_verbose("cfi_identify %d: Detected 16-bit device, 8-bit mode\n", __LINE__);

		/* Get vendor and id */
		vendorId = rp->romread(rp, CFI_16_8_VENDOR_LO) | rp->romread(rp, CFI_16_8_VENDOR_HI) << 8;
		size = rp->romread(rp, CFI_16_8_SIZE);
		deviceId = rp->romread(rp, CFI_16_8_DEVICE_LO) | rp->romread(rp, CFI_16_8_DEVICE_HI) << 8;
		#if 0
		int temp = rp->romread(rp,EXT_TABLE_P);
		temp = rp->romread(rp,EXT_TABLE_R);	
		temp = rp->romread(rp,EXT_TABLE_I);
		temp = rp->romread(rp,EXT_TABLE_MAJOR);
		temp = rp->romread(rp,EXT_TABLE_MINOR);
		temp = rp->romread(rp,EXT_TABLE_P_AND_E_BLOCK);	
		#endif
		cfi_read_blocks(rp);
		/* Back to read-array mode */
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_RESET); /* For AMD */
	}
	
	if (vendorId != 0xFFFF)
	{
		dagutil_verbose("CFI: Vendor %s, interface %s, size 0x%02x\n", cfi_vendor_str(vendorId), cfi_device_str(deviceId), (uint32_t) size);
	}
	else
	{
		dagutil_verbose("CFI not detected\n");
	}

	/* At this time only support CFI for Intel, use JEDEC for the rest to reduce 
	 * the potential for problems with existing products.
	 *  
	 * We only bother to extract the vendorId so we can identify the 
	 * ROM manufacturer and device using CFI.
	 */
	if ((vendorId == CFI_VENDOR_INTEL) || (vendorId == CFI_VENDOR_INTEL_EXT))
	{
		/* Extract the manufacturer and device id's from the ROM by writing
		   CFI_CMD_QUERY to the CFI_QUERY_ADDR offset on the ROM. Wait a bit
		   and wait for the ROM to write the manufacturer and device to the 
		   CFI_BASE_ADDR location.
		 */
		rp->romwrite(rp, CFI_QUERY_ADDR, CFI_CMD_QUERY);
		dagutil_microsleep(1); /* T ida = 150 nanoseconds */
		rp->manufacturer = rp->romread(rp, CFI_BASE_ADDR);
		rp->device = rp->romread(rp, CFI_BASE_ADDR + 2);
		dagutil_verbose("Intel: manufacturer=0x%02x, device=0x%02x\n", rp->manufacturer, rp->device);
		dagutil_verbose("Using CFI mode\n");

		/* This is untested - need to confirm that the manufacturer and
		   device are as expected and have an entry in the rom table once 
		   the DAG3.7T hardware is available 
		 */
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
		rp->romid = (((rp->manufacturer) << 8) | (rp->device));
		return 0;
	}

	return -1;
}
static int
cfi_read_blocks(romtab_t* rp)
{
	int loop = 0;
	int region_offset = 0;
	rp->romwrite(rp, CFI_QUERY_ADDR, CFI_CMD_QUERY);
	dagutil_microsleep(1); /* T ida = 150 nanoseconds */

	rp->rblock.number_regions = rp->romread(rp,ERASE_BLOCK_REGIONS);	
	for (loop = 0;loop < rp->rblock.number_regions;loop++)
	{
		rp->rblock.sectors[loop] = rp->romread(rp,(ERASE_BLOCK_REGION_ONE_INFO + region_offset) ) | rp->romread(rp,(ERASE_BLOCK_REGION_ONE_INFO + region_offset + 2)) << 8;
		//the sectore cointains the actual number of sectors
		rp->rblock.sectors[loop] ++;
		rp->rblock.size_region[loop] = rp->romread(rp,(ERASE_BLOCK_REGION_ONE_INFO + region_offset + 4)) | rp->romread(rp,(ERASE_BLOCK_REGION_ONE_INFO + region_offset + 6)) << 8;
		rp->rblock.size_region[loop]*=256; /*in units of 256 BYTES*/
		/* calculate the opffsets of the regions with in the dice */
		if (loop > 0 ){ 
			rp->rblock.region_offset[loop] = rp->rblock.region_offset[loop - 1] + (rp->rblock.size_region[loop - 1] * rp->rblock.sectors[loop -1]);
		} else {
			rp->rblock.region_offset[loop] = 0;
		}
	 

		region_offset += 8;
	}
	rp->romwrite(rp, CFI_QUERY_ADDR, CFI_CMD_READ_ARRAY);
	/*Back to read Array Mode.*/
	return 0;
}

static int
jedec_identify(romtab_t* rp)
{
	/* CFI not supported, try JEDEC */
	rp->romwrite(rp, 0x5555, 0xAA);
	rp->romwrite(rp, 0x2AAA, 0x55);
	rp->romwrite(rp, 0x5555, 0x90);

	dagutil_microsleep(1); /* T ida = 150 nanoseconds */

	rp->manufacturer = rp->romread(rp, 0);
	rp->device = rp->romread(rp, 1);

	rp->romwrite(rp, 0x5555, 0xAA);
	rp->romwrite(rp, 0x2AAA, 0x55);
	rp->romwrite(rp, 0x5555, 0xF0);

	dagutil_microsleep(1); /* T ida = 150 nanoseconds */

	rp->romid = (rp->manufacturer << 8) | rp->device;

	if (rp->romid == 0 || rp->romid == 65535)
	{
		dagutil_verbose("Could not identify EEPROM.  Please contact <support@endace.com> for assistance.  (Card returned manufacturer = %.2x, device = %.2x.)\n", rp->manufacturer, rp->device);
		errno = ENODEV;
		return -1;
	}

	return 0;
}


/* cfi_device_str : Scan the list of devices for the deviceId 
 * and return a description string.
 * Args:    deviceId    - CFI device id to find
 * Returns: pointer to the vendor description string, or NULL if
 *          one is not found.
 * Globals: Reads the cfi_device table.
 */
static char* 
cfi_device_str(uint16_t deviceId)
{
	int ind;

	for (ind = 0; cfi_device[ind].id != 0xFFFF; ind++)
	{
		if (cfi_device[ind].id == deviceId)
			break;
	}

	return cfi_device[ind].desc;
}
/* cfi_vendor_str : Scan the list of vendors for the vendorId 
 * and return a description string.
 * Args:    vendorId    - CFI vendor id to find
 * Returns: pointer to the vendor description string, or NULL if
 *          one is not found.
 * Globals: Reads the cfi_vendor table.
 */
static char* 
cfi_vendor_str(uint16_t vendorId)
{
	int ind;

	for (ind = 0; cfi_vendor[ind].id != 0xFFFF; ind++)
	{
		if (cfi_vendor[ind].id == vendorId)
			break;
	}

	return cfi_vendor[ind].desc;
}
/* manufacture_str : Scan the list of manufacturers for the 
 * id and return a description string.
 * Args:    manufacturerId  - Manufacture id to find
 * Returns: pointer to the description string, or NULL if
 *          one is not found.
 * Globals: Reads the rom_manufacturer table.
 */
static char* 
manufacturer_str(uint8_t manufacturerId)
{
	int ind;

	for (ind = 0; rom_manufacturer[ind].id != 0x00; ind++)
	{
		if (rom_manufacturer[ind].id == manufacturerId)
			break;
	}

	return rom_manufacturer[ind].desc;
}
/* romid_str : Scan the list of roms for the id and return
 * a description string.
 * Args: romid  - rom id to find
 * Returns: pointer to the description string, or NULL if one is not found.
 * Globals: Reads the romtab table.
 */
static char* 
romid_str(uint16_t romid)
{
	int ind;

	for (ind = 0; romtab[ind].romid != 0x00; ind++)
	{
		if (romtab[ind].romid == romid)
			break;
	}

	if (romtab[ind].ident)
		return romtab[ind].ident;
	else
		return "Unknown romid";
}
/* --------------------------------------------------------- */
/* ROM Read                                                  */

/*	Function: dagrom_check_srctgt
	Given a valid romtable describing the size of the ROM,
	check that the ROM is big enough to hold the source 
	image, who's size, is described by the input parameter.
	Args:	rp - pointer to the romtable.
			
*/
unsigned int
dagrom_check_srctgtsize(romtab_t* rp, uint32_t source_size )
{
	assert( rp );
	
	return rp->tsize >= source_size;
}
/*	Function : readsector
	Given a sector address read the appropriate number of
	bytes into the buffer bp, using the appropriate sector size.
*/
void
dagrom_readsector(romtab_t* rp, uint8_t* bp, uint32_t saddr)
{
	dagrom_read(rp, bp, saddr, saddr + DAGROM_SECTOR);
}
/*	Function : readrom
	Given a ROM descriptor reference read the data between
	the supplied addresses into a buffer.
*/
int
dagrom_read(romtab_t* rp, uint8_t* bp, uint32_t saddr, uint32_t eaddr)
{
	uint32_t addr;
	uint16_t val = 0;
	if (bp == NULL)
	{
		errno = ENODEV;
		return -1;
	}
	if(rp->rom_version == 2)
	{
		for (addr = saddr; addr < eaddr; addr+=2)
		{
			val = rp->romread(rp,addr);
			*bp = val;
			*(bp + 1) = (val & 0xff00) >> 8;
			if ((addr & 0xfff) == 0)
				dagutil_verbose("read  0x%.8x 0x%.4x\r", addr, *(uint16_t*)bp);
			bp++;
			bp++;
		}
	}
	else
	{
		for(addr = saddr; addr < eaddr;addr++)
		{	
			val = rp->romread(rp,addr);
			*bp = val;
			if ((addr & 0xfff) == 0)
				dagutil_verbose("read  0x%.8x 0x%.4x\r", addr, (unsigned) *bp);
			bp++;
		}
	}
	dagutil_verbose("\n");
	return 0;
}


/* --------------------------------------------------------- */
/* ROM Program                                              */

int
dagrom_program(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr)
{
	return rp->program(rp, bp, ibuflen, saddr, eaddr);
}


/* sst_program : Write the supplied buffer to the locations
 * supplied by the user.
 * Args :   rp - valid ROM descriptor pointer
 *          bp - input buffer
 *          ibuflen - length of buffer to write
 *          saddr - start address
 *          eaddr - end address
 * Return : 0 - OK
 *          -1 - error
 */
static int
sst_program(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr)
{
	int count;
	uint32_t addr;

	count = ibuflen;
	for (addr = saddr; addr < eaddr; addr++)
	{
		rp->romwrite(rp, 0x5555, 0xAA);
		rp->romwrite(rp, 0x2AAA, 0x55);
		rp->romwrite(rp, 0x5555, 0xA0); /* byte program */
		rp->romwrite(rp, addr, *bp); /* the real stuff */

		dagutil_microsleep(20); /* T bp = 20 usecs */

		if ((addr & 0xfff) == 0)
			dagutil_verbose("write 0x%.8x 0x%.2x\r", addr, (unsigned) *bp);
		bp++;
		if (--count <= 0)
			break;
	}
	dagutil_verbose("\n");
	return 0;
}


/*  amd_program : Write the supplied buffer into the ROM at the 
 *  between the locations supplied
 *  Args :  rp - valid ROM descriptor pointer
 *          bp - input buffer
 *          ibuflen - length of the input buffer
 *          saddr - start address
 *          eaddr - end address
 *  Returns :   -1 - error
 *              0 - otherwise 
 */
static int
amd_program(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr)
{
	int count, loop, iters;
	uint32_t addr;

	count = ibuflen;
	for (addr = saddr; addr < eaddr; addr++)
	{
		iters = 0;
	  again:
		iters++;
		if (iters > 1000)
		{
			dagutil_verbose("amd_program: write failed, address 0x%08x data 0x%02x is 0x%02x\n", addr, (unsigned) *bp, rp->romread(rp, addr));
			errno = ENODEV;
			return -1;
		}

		rp->romwrite(rp, 0x555, 0xAA);
		rp->romwrite(rp, 0x2AA, 0x55);
		rp->romwrite(rp, 0x555, 0xA0); /* byte program */
		rp->romwrite(rp, addr, *bp); /* the real stuff */

		if ((addr & 0xfff) == 0)
			dagutil_verbose("write 0x%.8x 0x%.2x\r", addr, (unsigned) *bp);

		for (loop = 1000; loop != 0; loop--)
		{
			dagutil_nanosleep(1500);
			if (rp->romread(rp, addr) == *bp)
				break;
		}

		if (loop == 0)
		{
			dagutil_warning("amd_program: write failed, retry #%d addr 0x%08x data 0x%02x is 0x%02x\n", iters, addr, (unsigned) *bp, rp->romread(rp, addr));
			goto again;
		}

		bp++;
		if (--count <= 0)
			break;
	}
	dagutil_verbose("\n");
	return 0;
}


/* cfi_program: 
 *
 */
static int
cfi_program(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr)
{
	uint32_t count, addr;

	if (uFastCfiMode)
	{
		cfi_buffered_program(rp, bp, ibuflen, saddr, eaddr);
		return 0;
	}

	dagutil_verbose("cfi_program: %08x to %08x, count %08x\n", saddr, eaddr, ibuflen);
	count = ibuflen;

	for (addr = saddr; count && (addr < eaddr); addr++, count--)
	{
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
		rp->romwrite(rp, addr, CFI_CMD_WRITE_BYTE);
		rp->romwrite(rp, addr,*bp);
		if ((addr & 0xfff) == 0)
			dagutil_verbose("write 0x%.8x 0x%.2x\r", addr, (unsigned) *bp);

		bp++;

#ifndef CFI_NODELAY
		/* Q. Do we need this? */
		dagutil_microsleep(20); /* T bp = 20 usecs */
#endif

		while (!(cfi_status(rp) & CFI_STATUS_READY));
	}
	dagutil_verbose("\n");

	/* Back to read-array mode */
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
	return 0;
}


static int
cfi_program_v2(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr)
{
	uint32_t count, addr;
	unsigned int val = 0;
	int lock = 0;
	
	int image_sign_field_width = 0;
	
	if (uFastCfiMode)
	{
		cfi_buffered_program_v2(rp, bp, ibuflen, saddr, eaddr);
		return 0;
	}

	/*We have combined image write and raw write in this function.to accomodate both we have to make the following changes.*/

	/*The initial state all sectors are locked.Before writing they have to be unlocked.*/
	sector_range_lock(rp,saddr,eaddr,lock);

	dagutil_verbose("cfi_program: %08x to %08x, count %08x\n", saddr, eaddr, ibuflen);
	count = ibuflen;

	if ((saddr > 0x1fe0000) || (saddr < 0x40000))
	{
		image_sign_field_width = 0;
	}else
	{
		image_sign_field_width = 4;
	}
	for (addr = (saddr + image_sign_field_width); count && (addr < eaddr); addr+=2, count-=2)
	{
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
		rp->romwrite(rp, addr, CFI_CMD_WRITE_BYTE);
		if(((address_sixteen_bits == 0) && ((address_read_back & 0xffff) == 0xfffe)) || ((address_sixteen_bits == 1) && ((address_read_back&0xffff) == 0xffff)))
		{
			val = ((*(bp + 1) << 8) | *(bp));
			bp++;
			bp++;
			rp->romwrite(rp,addr,val);
		}
		else
		{
			rp->romwrite(rp, addr,*bp);
			bp++;
		}
		if ((addr & 0xfff) == 0)
			dagutil_verbose("write 0x%.8x 0x%.4x\r", addr, (unsigned) *bp);
		
#ifndef CFI_NODELAY
		/* Q. Do we need this? */
		dagutil_microsleep(20); /* T bp = 20 usecs */
#endif

		while (!(cfi_status(rp) & CFI_STATUS_READY));
	}
	/*Adding the signature field after data has been written */
	if (image_sign_field_width != 0)
	{
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
		rp->romwrite(rp, saddr, CFI_CMD_WRITE_BYTE);
		val = 0xfc30;
		rp->romwrite(rp,saddr,val);

		dagutil_microsleep(20);

		while(!(cfi_status(rp) & CFI_STATUS_READY));
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
		rp->romwrite(rp, saddr, CFI_CMD_WRITE_BYTE);
	
		val = 0xffff;
	
		rp->romwrite(rp,saddr,val);

		dagutil_microsleep(20);

		while(!(cfi_status(rp) & CFI_STATUS_READY));
	
		/*Lock all the blocks.*/
		lock = 1;
		sector_range_lock(rp,saddr,eaddr,lock);
		dagutil_verbose("\n");
		/* Back to read-array mode */
	}
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
	return 0;
}
/* cfi_buffered_program : Program a block in the flash via the 32 
 * byte buffer in the Intel StrataFlash. This is a high-performance 
 * interface increasing the programming speed up to 32 times.
 * Args:    rp - pointer to a valid ROM descriptor
 *          bp - pointer to input buffer, the data to be programmed
 *          ibuflen - length of the input buffer
 *          saddr - start address
 *          eaddr - end address
 * Returns :    0 - OK
 *              -1 - FAILURE
 */
static int
cfi_buffered_program(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr)
{
	uint32_t count, addr, ind;
	uint8_t size;

	dagutil_verbose("cfi_buffered_program: %08x to %08x, count %08x\n", saddr, eaddr, ibuflen);
	count = ibuflen;

	/* Write a partial buffer if needed to align the data to buffer size. */
	/* XXX TBD - for now just report an error */
	if (saddr % STRATA_BUFFER_SIZE)
	{
		dagutil_error("Program start address 0x%x is not buffer aligned\n", saddr);
		//if (hold_processor) pbi_release();
		errno = ENODEV;
		//      exit(EXIT_FAILURE);
		return -1;
	}

	/* Make sure the device is ready */
	while (!(cfi_status(rp) & CFI_STATUS_READY));

	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);

	/* Write the rest of the data */
	for (addr = saddr; count && (addr < eaddr); addr += size, count -= size)
	{
		rp->romwrite(rp, addr, CFI_CMD_WRITE_BUFFER);

		if (count > STRATA_BUFFER_SIZE)
		{
			size = STRATA_BUFFER_SIZE;
		}
		else
		{
			size = (uint8_t) count;
			dagutil_verbose("\nwrite 0x%.8x 0x%.2x, %d bytes\r", addr, (unsigned) *bp, size);
		}
		rp->romwrite(rp, addr, (size - 1));

		for (ind = 0; ind < size; ind++, bp++)
		{
			if ((addr & 0xfff) == 0)
				dagutil_verbose("write 0x%.8x 0x%.2x\r", addr, (unsigned) *bp);
			rp->romwrite(rp, (addr + ind), *bp);
		}
		rp->romwrite(rp, addr, CFI_CMD_CONFIRM);

		/* Wait until done */
		while (0 == (cfi_status(rp) & CFI_STATUS_READY))
		{
			/* Do nothing. */
		}
	}
	dagutil_verbose("\n");

	/* Back to read-array mode */
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
	return 0;
}

static int
cfi_buffered_program_v2(romtab_t* rp, uint8_t* bp, uint32_t ibuflen, uint32_t saddr, uint32_t eaddr)
{
	uint32_t count, addr, ind;
	uint8_t size;
	unsigned int val = 0;
	int lock = 0;

	unsigned int start_sector,end_sector = 1;	
	unsigned current_sector;
	int image_sign_field_width = 0;	
	
	dagutil_verbose("cfi_buffered_program: %08x to %08x, count %08x\n", saddr, eaddr, ibuflen);
	count = ibuflen;

	/* Write a partial buffer if needed to align the data to buffer size. */
	/* XXX TBD - for now just report an error */
	if (saddr % P30_STRATA_BUFFER_SIZE)
	{
		dagutil_error("Program start address 0x%x is not buffer aligned\n", saddr);
		errno = ENODEV;
		return -1;
	}
	if ((saddr >= 0x1fe0000) || (saddr < 0x40000))
        {
                image_sign_field_width = 0;
        }else
        {
                image_sign_field_width = 4;
        }

	/* Make sure the device is ready */
	while (!(cfi_status(rp) & CFI_STATUS_READY));

	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);

	/*Unlock the blocks in this range.*/
	/*The initial state all sectors are locked.Before writing they have to be unlocked.*/
	sector_range_lock(rp,saddr,eaddr,lock);

	/*get the start and end sector and populate the sector_start_addr array*/
	sector_range_calc(rp,saddr,eaddr,&start_sector,&end_sector);
	current_sector = start_sector;

	/* Write the rest of the data */
	for (addr = (saddr + image_sign_field_width); count && (addr < eaddr); addr += size, count -= size)
	{
		if (count > P30_STRATA_BUFFER_SIZE)
		{
			size = P30_STRATA_BUFFER_SIZE;
		}
		else
		{
			size = (uint8_t) count;
			dagutil_verbose("\nwrite 0x%.8x 0x%.2x, %d bytes\n", addr, (unsigned) *bp, size);
		}
		if((addr > sector_start_addr[current_sector]) && (((addr + size) - sector_start_addr[current_sector + 1])) < P30_STRATA_BUFFER_SIZE)
		{

			/*Buffered write will not work we are headed for trouble*/	
			/*write till the end of this sector and then continue.*/	
			
			size = (P30_STRATA_BUFFER_SIZE - ((addr + size) - sector_start_addr[current_sector + 1]));
			
			for(ind = 0;ind < size; ind+=2, bp+=2)
			{
					/*In the above two cases we have to format 16 bits of data.*/
					rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_CLEAR_STATUS);
					rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_WRITE_BYTE);
					
					val = ((*(bp + 1) << 8) | *(bp));
					rp->romwrite(rp,(addr + ind),val);
					
					while (!(cfi_status(rp) & CFI_STATUS_READY));
			}
			current_sector++;
		}
		else
		{
			/*size < 64 cannot use buffered write.god knows why not.!!!!*/	
			if(size < P30_STRATA_BUFFER_SIZE)
			{
				for(ind = 0;ind < size; ind+=2, bp+=2)
				{
					/*In the above two cases we have to format 16 bits of data.*/
					rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_CLEAR_STATUS);
					rp->romwrite(rp,CFI_BASE_ADDR,CFI_CMD_WRITE_BYTE);
					
					val = ((*(bp + 1) << 8) | *(bp));
					rp->romwrite(rp,(addr + ind),val);
					
					while (!(cfi_status(rp) & CFI_STATUS_READY));
				}
				/* Wait until done */
			}
			else
			{
				sector_range_calc(rp,addr,addr,&current_sector,&current_sector);
				rp->romwrite(rp, addr, CFI_CMD_WRITE_BUFFER);
				rp->romwrite(rp, addr, ((size/2) - 1));
				for (ind = 0; ind < size; ind+=2, bp+=2)
				{
					if(((address_sixteen_bits == 0) && ((address_read_back & 0xffff) == 0xfffe)) || ((address_sixteen_bits == 1) && ((address_read_back & 0xffff)  == 0xffff)))
					{
						/*In the above two cases we have to format 16 bits of data.*/
						val = ((*(bp + 1) << 8) | *(bp));
						dagutil_verbose("write 0x%.8x 0x%.4x\r", addr, val);
						rp->romwrite(rp,(addr + ind),val);
					}
					else
					{
						if ((addr & 0xfff) == 0)
							dagutil_verbose("write 0x%.8x 0x%.2x\r", addr, (unsigned) *bp);
						rp->romwrite(rp, (addr + ind), *bp);
					}
				}
				rp->romwrite(rp, addr, CFI_CMD_CONFIRM);
				/* Wait until done */
				while (0 == (cfi_status(rp) & CFI_STATUS_READY))
				{
					/* Do nothing. */
				}
	 		}
		}
	}
	/*We have to write the signature field to make the image valid.*/
	if (image_sign_field_width != 0)
	{			
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
		rp->romwrite(rp, saddr, CFI_CMD_WRITE_BYTE);
		val = 0xfc30;
		rp->romwrite(rp,saddr,val);

		dagutil_microsleep(20);

		while(!(cfi_status(rp) & CFI_STATUS_READY));
		
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
		rp->romwrite(rp, (saddr + 2), CFI_CMD_WRITE_BYTE);
	
		val = 0xffff;
		rp->romwrite(rp,saddr,val);

		dagutil_microsleep(20);
		/*Lock the blocks*/
		lock = 1;
		sector_range_lock(rp,saddr,eaddr,lock);
	
		dagutil_verbose("\n");
	}
	/* Back to read-array mode */
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
	return 0;
}

/* --------------------------------------------------------- */
/* ROM SWID read/write                                      */

/*  To erase the SWID, read in the sector memset the SWID bits+HEADER bits
   to 0 and write it back in.
 */
int
dagswid_erase(romtab_t* rp)
{
	uint8_t* sbuf;
	uint32_t tgtsaddr;

	switch (rp->device_code)
	{
		case 0x430e: /* DAG4.3ge */
		case 0x378e: /* DAG3.7G */
		case 0x3707: /* DAG3.7t */
		case 0x3747: /* DAG3.7t4 */
		case 0x7100: /* DAG7.1s */
		{
			tgtsaddr = DAGROM_BSTART + DAGROM_BSIZE - DAGROM_SECTOR;
			
			sbuf = (uint8_t*) malloc(DAGROM_SECTOR);
			//dagutil_verbose("SWID Reading Target Sector 0x%.08x\n", tgtsaddr);
			dagrom_readsector(rp, sbuf, tgtsaddr);
			
			//dagutil_verbose("SWID Erase Target Sector 0x%.08x\n", tgtsaddr);
			rp->erase(rp, tgtsaddr, (tgtsaddr + DAGROM_SECTOR) - 1);
			
			/* Memset the appropriate SWID bits and then write the SWID back into memory */
			//dagutil_verbose("SWID Erased key bits from sector buffer\n");
			memset(sbuf + DAGSERIAL_PACKAGE_OFFSET, 0, DAGSERIAL_PACKAGE_SIZE);
			//dagutil_verbose("SWID Writing sector buffer to ROM at sector address 0x%.8x to address 0x%.8x\n", tgtsaddr, tgtsaddr + DAGROM_SECTOR);
			rp->program(rp, sbuf, DAGROM_SECTOR, tgtsaddr, tgtsaddr + DAGROM_SECTOR);
			
			free(sbuf);
			break;
		}

		default:
		{
			errno = ENODEV;
			return -1;
		}
	}

	return 0;
}


/*  dagswid_isswid : Check if there is a SWID stored in the ROM 
   Args :       rp  - pointer to a valid ROM descriptor
   Returns  0   - There is a SWID in the ROM 
   -1   - No SWID.
 */
int
dagswid_isswid(romtab_t* rp)
{
	uint8_t* keybuf; /* key buffer and sector buffer */
	uint32_t rptr; /* Read ptr for the key field transfer  */

	switch( rp->device_code )
	{
		case 0x430e:
		case 0x378e:
		case 0x3707: /* DAG3.7t */
		case 0x3747: /* DAG3.7t4 */
		case 0x7100:
		{
		keybuf = (uint8_t*) malloc(DAGSERIAL_HEADER_SIZE);
			rptr = DAGROM_BSTART + DAGROM_BSIZE - DAGSERIAL_SIZE - DAGSERIAL_HEADER_SIZE;
		
		//dagutil_verbose("SWID: reading SWID header at address 0x%.8x\n", rptr);
		
		if (dagrom_read(rp, keybuf, rptr, rptr + DAGSERIAL_HEADER_SIZE) < 0)
		{
			free(keybuf);
			return -1;
		}
		
		/* Check if we have a valid SWID key by checking for the magic key value */
		if ((*(uint32_t *) keybuf) == DAGSERIAL_MAGIC_MARKER_VAL)
		{
			free(keybuf);
			return 0;
			}
		}
	}
	
	return -1;
}


/*  dagswid_checkkey : Check that the key provided, iswidkey matches the key stored
   in the ROM.
   Args :       rp - pointer to a valid ROM descriptor
   bp - swidbuf pointer
   iswidlen - size of the buffer.
   Returns :    0  - The supplied key matches the stored key.
   -1 - Cannot read from ROM
   -2 - No key present
   -3 - The supplied key does not match the stored key.
 */
int
dagswid_checkkey(romtab_t* rp, uint32_t iswidkey)
{
	uint32_t strswidkey; /* swidkey stored on the ROM */
	uint32_t ret;
	
	switch (rp->device_code)
	{
		case 0x430e: /* DAG4.3ge */
		case 0x378e: /* DAG3.7G */
		case 0x3707: /* DAG3.7t */
		case 0x3747: /* DAG3.7t4 */
		case 0x7100: /* DAG7.1s */
		{
			ret = dagswid_readkey(rp, &strswidkey);
			if (ret < 0)
			{
				dagutil_verbose("SWID: failed to find key on ROM\n");
				return ret;
			}
			
			if (strswidkey != iswidkey)
				return -3;
			
			break;
		}

		default:
		{
			dagutil_verbose("Device id: 0x%.4x does not support SWID\n ", rp->device);
			errno = ENODEV;
			return -1;
		}
	}
	
	return 0;
}

/*  dagswid_readkey : Read out the swidkey if there is a valid magic marker.
 *  Args :      rp - pointer to a valid ROM descriptor.
 *              iswidkey - pointer to integer to write the key into.
 *  Returns :   0 - On success
 *              -1 - error cannot read from ROM
 *              -2 - No key present
 *              -3 - Card does not support this form SWID key query
 */
int
dagswid_readkey(romtab_t* rp, uint32_t * iswidkey)
{
	uint8_t* keybuf; /* key buffer and sector buffer */
	uint32_t rptr; /* Read ptr for the key field transfer  */
	
	switch (rp->device_code)
	{
		case 0x430e: /* DAG4.3ge */
		case 0x378e: /* DAG3.7G */
		case 0x3707: /* DAG3.7t */
		case 0x3747: /* DAG3.7t4 */
		case 0x7100:
		{
			keybuf = (uint8_t*) malloc(DAGSERIAL_HEADER_SIZE);
			rptr = DAGROM_BSTART + DAGROM_BSIZE - DAGSERIAL_SIZE - DAGSERIAL_HEADER_SIZE;
			
			//dagutil_verbose("SWID reading header at address 0x%.8x \n", rptr);
			
			if (dagrom_read(rp, keybuf, rptr, rptr + DAGSERIAL_HEADER_SIZE) < 0)
			{
				//dagutil_warning("dagrom_read : failed at address 0x%.8x \n", rptr);
				free(keybuf);
				return -1;
			}
			
			/* Check if we have a valid SWID key first by checking for the magic key value */
			if ((*(uint32_t *) keybuf) == DAGSERIAL_MAGIC_MARKER_VAL)
			{
				rptr = DAGROM_BSTART + DAGROM_BSIZE - DAGSERIAL_SIZE - DAGSERIAL_KEY_SIZE;
				//dagutil_verbose("SWID reading key at offset 0x%.8x \n", rptr);
				
				//*iswidkey = *((uint32_t*)(keybuf+DAGSERIAL_MAGIC_MARKER_SIZE));
				if (dagrom_read(rp, keybuf, rptr, rptr + DAGSERIAL_KEY_SIZE) < 0)
				{
					free(keybuf);
					return -1;
				}
				
				*iswidkey = *((uint32_t *) (keybuf));
				
				free(keybuf);
				return 0;
			}
			else
			{
				errno = ENODEV;
				free(keybuf);
				return -2;
			}
			break;
		}

		default:
		{
			dagutil_verbose("Device id: 0x%.4x does not support SWID\n ", rp->device_code);
			errno = ENODEV;
			return -3;
		}
	}

	return 0;
}


/*  dagswid_read : Switch on the device_id, for DAG3.7G and DAG4.3GE. We can 
 *  use the device_id to determine how to write to the ROM.
 *  Args :      rp - pointer to a valid ROM descriptor
 *              bp - swidbuf pointer
 *              iswidlen - size of the buffer.
 *  Returns :   0 - On success
 *              -1 - error reading from the ROM
 */
int
dagswid_read(romtab_t* rp, uint8_t* swidbuf, uint32_t iswidlen)
{
	uint8_t* keybuf; /* key buffer and sector buffer */
	uint32_t rptr; /* Read ptr for the key field transfer  */

	switch (rp->device_code)
	{
		case 0x430e: /* DAG4.3ge */
		case 0x378e: /* DAG3.7G */
		case 0x3707: /* DAG3.7t */
		case 0x3747: /* DAG3.7t4 */
		case 0x7100: /* DAG7.1s */
		{
			keybuf = (uint8_t*) malloc(DAGSERIAL_HEADER_SIZE + iswidlen);
			rptr = DAGROM_BSTART + DAGROM_BSIZE - DAGSERIAL_SIZE - DAGSERIAL_HEADER_SIZE;
			
			/* Read the first 4 bytes and check if it equals the serial number key 
			*/
			if (dagrom_read(rp, keybuf, rptr, rptr + (DAGSERIAL_HEADER_SIZE + 1)) < 0)
			{
				//dagutil_verbose("SIWD dagrom_read failed for addresses 0x%.8x to 0x%.8x\n", rptr, rptr + DAGSERIAL_HEADER_SIZE);
				free(keybuf);
				return -1;
			}
			
			if ((*(uint32_t *) keybuf) == DAGSERIAL_MAGIC_MARKER_VAL)
			{
				//dagutil_verbose("SWID key header found at location 0x%.8x\n", rptr);
				//dagutil_verbose("SWID reading SWID at location 0x%.8x to 0x%.8x\n", rptr + DAGSERIAL_HEADER_SIZE, rptr + DAGSERIAL_PACKAGE_SIZE);
				dagrom_read(rp, swidbuf, rptr + DAGSERIAL_HEADER_SIZE, rptr + DAGSERIAL_HEADER_SIZE + iswidlen);
			}
			else
			{
				//dagutil_verbose("SWID key header not found at location 0x%.8x\n", rptr);
			}
			
			free(keybuf);
			break;
		}
		
		default:
		{
			dagutil_verbose("Device id: 0x%.4x does not support SWID\n ", rp->device_code);
			errno = ENODEV;
			return -1;
		}
	}

	return 0;
}


/*  dagswid_write : Switch on the device_id to determine if we are dealing with
 *  the DAG3.7G or DAG4.3GE.
 *  
 *  Args:       rp - pointer to a valid ROM descriptor
 *              swidbuf - pointer to a buffer containing the SWID to write to the DAG card.
 *              iswidlen - length of the SWID to write to the dag card.
 *              iswidkey - SWID key, will write the key in with the SWID.
 *  Returns:    0 - 
 *              -1 -  
 *  
 *  Notes : 
 *  -   Write operation must start with a sector read for the DAG4.3GE
 *      because the image overlaps with the last sector. Perform a sector erase
 *      to reset all bits to 1 (for the AMD chip). We then write the sector back
 *      to the ROM (after appending the SWID) to the sector. 
 */
int
dagswid_write(romtab_t* rp, uint8_t* swidbuf, uint32_t iswidlen, uint32_t iswidkey)
{
	uint8_t* sbuf; /* Sector buffer */
	uint32_t tgtsaddr; /* Target SWID sector address */

	switch (rp->device_code)
	{
		case 0x430E: /* DAG4.3G */
		case 0x378E: /* DAG3.7G */
		case 0x3707: /* DAG3.7t */
		case 0x3747: /* DAG3.7t4 */
		case 0x7100: /* DAG7.1S */
		{
			/* Read in the target sector, the sector address is the last sector in 
			the top-half image.
			*/
			tgtsaddr = (DAGROM_BSTART + DAGROM_BSIZE) - DAGROM_SECTOR;
			
			/* Read the sector into the temporary store buffer */
			sbuf = (uint8_t*) malloc(DAGROM_SECTOR);
			//dagutil_verbose("SWID Writing SWID %s\n", swidbuf);
			//dagutil_verbose("SWID Reading Target Sector 0x%.08x\n", tgtsaddr);
			dagrom_readsector(rp, sbuf, tgtsaddr);

			/* Erase the sector to set all bits to 1 */
			//dagutil_verbose("SWID Erase Target Sector 0x%.08x\n", tgtsaddr);
			rp->erase(rp, tgtsaddr, (tgtsaddr + DAGROM_SECTOR) - 1);
			
			/* Write the magic key code and SWID into the buffer */
			//dagutil_verbose("SWID Writing Magic Key to sector buffer at offset 0x%.8x \n", DAGSERIAL_PACKAGE_OFFSET);
			*((uint32_t *) (sbuf + DAGSERIAL_PACKAGE_OFFSET)) = DAGSERIAL_MAGIC_MARKER_VAL;
			
			/* Write the SWID key into the SWID header */
			//dagutil_verbose("SWID Writing SWID key %d to sector buffer at offset 0x%.8x \n", iswidkey, DAGSERIAL_KEY_OFFSET);
			*((uint32_t *) (sbuf + DAGSERIAL_KEY_OFFSET)) = iswidkey;
			
			/* Write the SWID into the buffer */
			//dagutil_verbose("SWID Writing SWID to sector buffer at offset 0x%.8x of length %d\n", DAGSERIAL_OFFSET, iswidlen);
			iswidlen = dagutil_min(DAGSERIAL_SIZE, iswidlen);
			memset(sbuf + DAGSERIAL_OFFSET, 0, DAGSERIAL_SIZE);
			memcpy(sbuf + DAGSERIAL_OFFSET, swidbuf, iswidlen);

            /* Erase the sector to set all bits to 1 */
			//dagutil_verbose("SWID Erase Target Sector 0x%.08x\n", tgtsaddr);
			rp->erase(rp, tgtsaddr, (tgtsaddr + DAGROM_SECTOR) - 1);
			
			/* Write the new sector back to the ROM */
			//dagutil_verbose("SWID Writing sector buffer to ROM at sector address 0x%.8x to address 0x%.8x\n", tgtsaddr, tgtsaddr + DAGROM_SECTOR);
			rp->program(rp, sbuf, DAGROM_SECTOR, tgtsaddr, tgtsaddr + DAGROM_SECTOR);
			
			free(sbuf);
			break;
		}
		
		default:
		{
			errno = ENODEV;
			return -1;
		}
	}

	return 0;
}


/* --------------------------------------------------------- */
/* ROM erase                                                */

int
dagrom_erase(romtab_t* rp, uint32_t saddr, uint32_t eaddr)
{
	return rp->erase(rp, saddr, eaddr);
}


/* sst_erase : Erase the ROM between the supplied sectors.
 * using the SST protocol.
 */
static int
sst_erase(romtab_t* rp, uint32_t saddr, uint32_t eaddr)
{
	uint32_t addr;

	for (addr = saddr; addr < eaddr; addr += rp->sector)
	{
		rp->romwrite(rp, 0x5555, 0xAA);
		rp->romwrite(rp, 0x2AAA, 0x55);
		rp->romwrite(rp, 0x5555, 0x80); /* erase */

		rp->romwrite(rp, 0x5555, 0xAA);
		rp->romwrite(rp, 0x2AAA, 0x55);
		rp->romwrite(rp, addr, 0x30); /* sector erase */

		dagutil_verbose("erase 0x%.8x sector\r", addr);
		dagutil_microsleep(25 * 1000); /* T se = 25 millisecs */
	}

	dagutil_verbose("\n");
	return 0;
}
/*
 * cfi_erase(): Erase a block in the flash. 
 * Inputs:  rp - pointer to the current rom definition
 *          ibuf - pointer to the input buffer
 *          buflen - length of the input buffer
 *          saddr - Start sector address,
 *          eaddr - End sector address
 *
 * Returns: none.
 */
static int
cfi_erase(romtab_t* rp, uint32_t saddr, uint32_t eaddr)
{
	uint32_t addr, ibuflen;
        uint32_t i, failures = 0;

	ibuflen = eaddr - saddr;

	dagutil_verbose("cfi_erase: %08x to %08x, count %08x\n", saddr, eaddr, ibuflen);

	for (addr = saddr; addr < eaddr; addr += DAGROM_SECTOR)
	{
		rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
		rp->romwrite(rp, addr, CFI_CMD_BLOCK_ERASE);
		rp->romwrite(rp, addr, CFI_CMD_RESUME);

#ifndef CFI_NODELAY
		/* Q. Do we need this? */
		dagutil_microsleep(20); /* T bp = 20 usecs */
#endif

		while (!(cfi_status(rp) & CFI_STATUS_READY))
		{
			/* Wait for the erase to complete */
		}

		dagutil_verbose("erase 0x%.8x sector\r", addr);

        /* check if erase has been successful */

       	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);

        for(i = addr; i< (addr+DAGROM_SECTOR); i++)
        {
            if(rp->romread(rp, i) != 0xFF)
            {
    		    dagutil_verbose_level(1,"erase 0x%.8x sector failed, value 0x%.8x, retry\n", i, rp->romread(rp, i));
                failures++;
                addr -= DAGROM_SECTOR;
                if(failures < 4)
                    break;
                else
                {
                    dagutil_verbose_level(1, "sector erase has been unsuccessful\n");
                    return -1;
                }
            }
            else
            {
                failures = 0;
            }
        }

	}
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
	dagutil_verbose("\n");
	return 0;
}
static int 
sector_range_calc(romtab_t *rp,uint32_t saddr,uint32_t eaddr,uint32_t *start_sector,uint32_t *end_sector)
{
	int l_region;
	int i;
	/* 
		ROM DEVICE SIZE SINGLE DICE FLASH ROM  =  
		rp->rblock.size_region[0]*rp->rblock.sectors[0] + rp->rblock.size_region[1]*rp->rblock.sectors[1] ...  rp->rblock.size_region[number_regions]*rp->rblock.sectors[1]
	*/
	
	
	l_region = -1;	
	*start_sector = 0;
	//walk through the regions to find the start sector where belongs to 
	for( i = 0 ; i < rp->rblock.number_regions; i++ )
	{
		if(saddr < rp->rblock.region_offset[i] + (rp->rblock.size_region[i] * rp->rblock.sectors[i]))
		{
			//region in which the addr is located
			l_region= i;
			//*start_sector += rp->rblock.sectors[i];
		 	break;
		}
		//add the past regions number of sectors 
		*start_sector += rp->rblock.sectors[i];
	}
	#if 1
	if( l_region  < 0 ) 
		dagutil_panic("region not found the start addesss out of range \n");
	
	#endif	
	//add the number of sectors with in the region where saddr is located 
	*start_sector += ((saddr - rp->rblock.region_offset[l_region] ) / rp->rblock.size_region[l_region]);
	l_region = -1;	
	*end_sector = 0;
	//walk through the regions to find the start sector where belongs to 
	for( i = 0 ; i < rp->rblock.number_regions; i++ )
	{
		if(eaddr <= rp->rblock.region_offset[i] + ((rp->rblock.size_region[i] * rp->rblock.sectors[i])))
		{
			l_region= i;
		 	break;
		}
		//add the past regions number of sectors 
		*end_sector += rp->rblock.sectors[i];
	}
	
	if( l_region  < 0 ) 
		dagutil_panic("region not found the end addesss out of rnage");
	
	*end_sector += ((eaddr - rp->rblock.region_offset[l_region]) / (rp->rblock.size_region[l_region]));

	dagutil_verbose_level(2,"start sector %d end Sector %d start address %x end address %x \n",*start_sector,*end_sector,saddr,eaddr);	
	
	return 0;
}
/*Need a function to convert from Sector to Address.To be used only with erase and lock.*/
static uint32_t 
sector_to_address(romtab_t *rp, uint32_t sector)
{

        int tmp_sector = 0;
        int tmp_address = 0;
       	int i = 0; 
        int l_region = -1;
        for (i = 0; i < rp->rblock.number_regions;i++)
        {
                if (sector <= (tmp_sector + rp->rblock.sectors[i] ) /*rp->rblock.sectors[i]*/)
                {
                       	l_region = i;
			break; 
                }
		tmp_sector += rp->rblock.sectors[i];
        }

	if(l_region == -1)
		dagutil_panic("sector addr outside the range \n");

	tmp_address = rp->rblock.region_offset[l_region];
	tmp_address +=( sector - tmp_sector )* rp->rblock.size_region[l_region];
	return tmp_address;
}

static uint32_t
sector_to_end_address(romtab_t *rp, uint32_t sector)
{

        int tmp_sector = 0;
        int tmp_address = 0;
        int i = 0;
        int l_region = -1;
        for (i = 0; i < rp->rblock.number_regions;i++)
        {
                if (sector < (tmp_sector + rp->rblock.sectors[i] ) /*rp->rblock.sectors[i]*/)
                {
                        l_region = i;
                        break;
                }
                tmp_sector += rp->rblock.sectors[i];
        }

        if(l_region == -1)
                dagutil_panic("sector addr outside the range \n");

        tmp_address = rp->rblock.region_offset[l_region];
        tmp_address +=( sector - tmp_sector )* rp->rblock.size_region[l_region];
	tmp_address += (rp->rblock.size_region[l_region]);
        return tmp_address;
}




static void 
sector_range_lock(romtab_t *rp,uint32_t saddr,uint32_t eaddr,bool lock)
{
	uint32_t start_sector,end_sector = 0;
	int addr = 0;
	int current_sector = 0;
	
	sector_range_calc(rp,saddr,eaddr,&start_sector,&end_sector);
	/*We need actual sector boundaries for this to work.so we introduce this function.*/
	
	for (current_sector = start_sector; current_sector <= end_sector ; current_sector++)
	{
			/*Unlock the block.*/
			addr = sector_to_address(rp,current_sector);
			//CHECKME:
			sector_start_addr[current_sector] = addr;
			if(lock == 0)
			{
				rp->romwrite(rp,addr,0x60);
				rp->romwrite(rp,addr,0xD0);
			}
			else/*lock == 1 -> lock it ....*/
			{	
				rp->romwrite(rp,addr,0x60);
				rp->romwrite(rp,addr,0x01);
			}
	}
}
/*
 * cfi_erase(): Erase a block in the flash. 
 * Inputs:  rp - pointer to the current rom definition
 *          ibuf - pointer to the input buffer
 *          buflen - length of the input buffer
 *          saddr - Start sector address,
 *          eaddr - End sector address
 *
 * Returns: none.
 */

static int
cfi_erase_v2(romtab_t* rp, uint32_t saddr, uint32_t eaddr)
{
	uint32_t addr, ibuflen;
        uint32_t i, failures = 0;
	uint32_t start_sector = 0;
	uint32_t end_sector = 0;
	int sector_size = 0;
	int current_sector = 0;

	ibuflen = eaddr - saddr;

	dagutil_verbose("cfi_erase: %08x to %08x, count %08x\n", saddr, eaddr, ibuflen);
	if(rp->sector == 0)
	{
		sector_range_calc(rp,saddr,eaddr,&start_sector,&end_sector);
		dagutil_verbose("cfi_erase temp: %08x to %08x, count %08x\n", saddr, eaddr, ibuflen);
				

		if(start_sector == end_sector)
		{
		for (current_sector = start_sector; current_sector <= end_sector; current_sector++)
		{	
			addr = sector_to_address(rp,current_sector);
			/*Unlock the block.*/
				
			rp->romwrite(rp,addr,0x60);
			rp->romwrite(rp,addr,0xD0);

			rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
			rp->romwrite(rp, addr, CFI_CMD_BLOCK_ERASE);
			rp->romwrite(rp, addr, CFI_CMD_RESUME);
#ifndef CFI_NODELAY			
			dagutil_microsleep(20); /* T bp = 20 usecs */
#endif		
			while (!(cfi_status(rp) & CFI_STATUS_READY))
			{
				/* Wait for the erase to complete */
			}
			dagutil_verbose("erase 0x%.8x sector\r", addr);

        		/*Lock the block again*/
			rp->romwrite(rp,addr,0x60);
			rp->romwrite(rp,addr,0x01);
			
			rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);

			/*Check if erase has been successful*/
			for(i = addr; i < (sector_to_end_address(rp,current_sector)); i+=2)
        		{

				if(rp->romread(rp, i) != 0xFFFF)
            			{
    		    			dagutil_verbose_level(1,"erase 0x%.8x sector failed, value 0x%.8x,\n", i,rp->romread(rp, i));
                			failures++;
                			addr -= sector_size;
                			if(failures < 4)
	                    			break;
        	        		else
                			{
                    				dagutil_verbose_level(1, "sector erase has been unsuccessful\n");
                    				return -1;
                			}
				}
            			else
            			{
                			failures = 0;
            			}
        		}
		}



		}else
		{

		for (current_sector = start_sector; current_sector < end_sector; current_sector++)
		{	
			addr = sector_to_address(rp,current_sector);
			/*Unlock the block.*/
				
			rp->romwrite(rp,addr,0x60);
			rp->romwrite(rp,addr,0xD0);

			rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
			rp->romwrite(rp, addr, CFI_CMD_BLOCK_ERASE);
			rp->romwrite(rp, addr, CFI_CMD_RESUME);
#ifndef CFI_NODELAY			
			dagutil_microsleep(20); /* T bp = 20 usecs */
#endif		
			while (!(cfi_status(rp) & CFI_STATUS_READY))
			{
				/* Wait for the erase to complete */
			}
			dagutil_verbose("erase 0x%.8x sector\r", addr);

        		/*Lock the block again*/
			rp->romwrite(rp,addr,0x60);
			rp->romwrite(rp,addr,0x01);
			
			rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);

			/*Check if erase has been successful*/
			for(i = addr; i < (sector_to_end_address(rp,current_sector)); i+=2)
        		{

				if(rp->romread(rp, i) != 0xFFFF)
            			{
    		    			dagutil_verbose_level(1,"erase 0x%.8x sector failed, value 0x%.8x,\n", i,rp->romread(rp, i));
                			failures++;
                			addr -= sector_size;
                			if(failures < 4)
	                    			break;
        	        		else
                			{
                    				dagutil_verbose_level(1, "sector erase has been unsuccessful\n");
                    				return -1;
                			}
				}
            			else
            			{
                			failures = 0;
            			}
        		}
		}
	}
		
	}
	else /*Normal case constant sector size*/
	{
		dagutil_panic("should not use hardcoded sector sizes \n");	
		for (addr = saddr; addr < eaddr; addr += DAGROM_SECTOR)
		{
			rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_CLEAR_STATUS);
			rp->romwrite(rp, addr, CFI_CMD_BLOCK_ERASE);
			rp->romwrite(rp, addr, CFI_CMD_RESUME);

	#ifndef CFI_NODELAY
			/* Q. Do we need this? */
			dagutil_microsleep(20); /* T bp = 20 usecs */
	#endif
			while (!(cfi_status(rp) & CFI_STATUS_READY))
			{
				/* Wait for the erase to complete */
			}

			dagutil_verbose("erase 0x%.8x sector\r", addr);

        		/* check if erase has been successful */

       			rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);

        		for(i = addr; i< (addr+DAGROM_SECTOR); i++)
        		{
            			if(rp->romread(rp, i) != 0xFF)
            			{
    		    			dagutil_verbose_level(1,"erase 0x%.8x sector failed, value 0x%.8x, retry\n", i, rp->romread(rp, i));
                			failures++;
                			addr -= DAGROM_SECTOR;
                			if(failures < 4)
                    				break;
                			else
                			{
                    				dagutil_verbose_level(1, "sector erase has been unsuccessful\n");
                    				return -1;
                			}
            			}
            			else
            			{
                			failures = 0;
            			}
        		}

		}
	}
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_ARRAY);
	dagutil_verbose("\n");
	return 0;

}
/*
 * amd_erase : An AMD erase operation takes a sector address saddr and erases
 * up to and including the sector eaddr in sector increments of rp->sector.
 *
 * Note : A sector/chip erase is the only way to set 0 bits to 1. And therefore
 * *must* be executed before writing any data, to ensure integrity.
 */
static int
amd_erase(romtab_t* rp, uint32_t saddr, uint32_t eaddr)
{
	uint32_t addr;
	int restart;
	uint32_t baddr;
#if defined(_WIN32)
	int count = 0;
#endif

	dagutil_verbose("erase blitz start...\n");

	/* This one is only about 3 seconds, needs monitoring. */
	restart = 1;
	for (addr = saddr; addr < eaddr; addr += rp->sector)
	{
	again:
		if (restart)
		{
			rp->romwrite(rp, 0x555, 0xAA); /* unlock */
			rp->romwrite(rp, 0x2AA, 0x55); /* unlock */
			rp->romwrite(rp, 0x555, 0x80); /* set-up */

			rp->romwrite(rp, 0x555, 0xAA); /* unlock */
			rp->romwrite(rp, 0x2AA, 0x55); /* unlock */

			restart = 0;
		}
		rp->romwrite(rp, addr, 0x30); /* sector erase */

		if (rp->romread(rp, addr) & BIT3)
		{
			/* sector erase timeout */
			while (!(rp->romread(rp, addr) & BIT7))
			{
				/* wait for erase to complete */
#if defined(_WIN32)
				count ++;
				if(count > 100000)
					break;
#endif
			}
			
			restart = 1;

			/* determine if 'current' sector was erased */
			for (baddr=addr; baddr < (addr + rp->sector); baddr++) {
				if (rp->romread(rp, baddr) != 0xff) {
					/* sector not erased */
					dagutil_verbose("erase sector 0x%.8x not erased, restarting sector\n", addr);
					goto again;
				} 
			} /* else sector was erased, continue */
			dagutil_verbose("erase restart from sector 0x%.8x\n", addr + rp->sector);
		}
	}
	addr -= rp->sector;
	while (!(rp->romread(rp, addr) & BIT3))
	{
		/* wait for the chip to accept command */
	}

	dagutil_verbose("erase blitz accepted...\n");
	while (!(rp->romread(rp, addr) & BIT7))
	{
		/* wait for erase to complete */
	}
	dagutil_verbose("erase blitz complete\n");
	return 0;
}


/* --------------------------------------------------------- */
/* ROM CFI Status                                           */

/* cfi_status : Return the current value of the status register. 
 * Inputs : rp - pointer to the current rom definition.
 * Returns : status register contents
 */
static uint8_t
cfi_status(romtab_t* rp)
{
	rp->romwrite(rp, CFI_BASE_ADDR, CFI_CMD_READ_STATUS);
#ifndef CFI_NODELAY
	/* Q. Do we need this? */
	dagutil_microsleep(20);
#endif

	return (uint8_t) rp->romread(rp, CFI_BASE_ADDR);
}


/* romreg : Return the rom registration */
unsigned
dagrom_romreg(romtab_t* rp)
{
	return *(unsigned int*) (rp->iom + rp->base + DAGROM_DATA);
}
unsigned
dagrom_reprogram_control_reg(romtab_t* rp)
{
	return *(unsigned int*) (rp->iom + rp->base + DAGROM_REPROGRAM_CONTROL);
}

void
dagrom_keywreg(romtab_t* rp, unsigned val)
{
	*(unsigned int*) (rp->iom + rp->base + DAGROM_KEY) = val;
	dagutil_nanosleep(500);
}


/* --------------------------------------------------------- */
/* ROM Verify                                               */

/* verify_rom : Given the input buffer, read back the ROM into
 * the supplied output buffer and then compare the contents.
 * Args :   rp - pointer to a valid ROM descriptor 
 *          ibuf - input buffer 
 *          ibuflen - input buffer length
 *          obuf - output buffer
 *          obuflen - output buffer length
 *          saddr - start address
 *          eaddr - end address
 * Return:  0 - On success
 *          -1 - On failure (sets errno )
 * Note : Read back and compare with original data content.
 * Can't use memcmp(3), because we need to know the
 * position of the non-matching byte.
 */
int
dagrom_verify(romtab_t* rp, uint8_t* ibuf, uint32_t ibuflen, uint8_t* obuf, uint32_t obuflen, uint32_t saddr, uint32_t eaddr)
{
	int len = dagutil_min(ibuflen, obuflen);
	int i;
	int image_sign_field_width = 0;
	
	

	/* Read the ROM between the supplied address range */
	dagrom_read(rp, obuf, saddr, eaddr);
	
	if((saddr > 0x1fe0000) || (saddr < 0x40000))
	{
		image_sign_field_width = 0;
	}else
	{
		image_sign_field_width = 4;
	}

	if(rp->rom_version == 2)
	{
		for (i = 0; i < len ; i++)
		{
			if (ibuf[i] != obuf[i + image_sign_field_width])
			{
				dagutil_error("verify 0x%.8x differs 0x%.2x 0x%.2x\n", saddr + i, ibuf[i], obuf[i + image_sign_field_width]);
				//if (hold_processor) pbi_release();
				//errno = ENODEV;
				return -1;
				//exit(EXIT_FAILURE);
			}
		}
	}
	else
	{
		for (i = 0; i < len ; i++)
		{
			if (ibuf[i] != obuf[i])
			{
				dagutil_error("verify 0x%.8x differs 0x%.4x 0x%.4x\n", saddr + i, ibuf[i], obuf[i]);
				//if (hold_processor) pbi_release();
				//errno = ENODEV;
				return -1;
				//exit(EXIT_FAILURE);
			}
		}
	}

	if(rp->rom_version == 2)
	{
		if((saddr < 0x1fe0000) || (saddr >= 0x40000))
		{
			if((rp->romread(rp,saddr) == 0xfc30) && (rp->romread(rp,(saddr + 2)) == 0xffff))
			{
				dagutil_verbose("verify ok\n");
			}
			else
			{
				dagutil_verbose("Invalid Image \n");	
			}
		}
	}
	return 0;
}
/* --------------------------------------------------------- */

/* xrev : Report xilinx revision strings. 
 * Args :   rp - valid pointer to a romtab
 */
void
dagrom_xrev(romtab_t* rp)
{
    /* current is user 
     * stable is factory
     */
	int current = 0;
	int serial = 0;
	char *first = "user: ";
	char *second ="factory:";
	char* first_new  = "user  1:";
	char* second_new = "factory:";
	char* third  = "user  2:";
	char* fourth = "user  3:";
	char* factive = "";
	char* sactive = "";
	char* aactive = "";
	char* bactive = "";
	char* cactive = "";
	char* dactive = "";
	bool image_valid[16] = {false,false,false,false,
			        false,false,false,false, 	 
			        false,false,false,false,
			        false,false,false,false			
			       };	
	uint32_t addr;
	uint8_t buf[128];
	int value = 0;
	int sign_field_width = 4; /*Width of the signature field.For Version 2*/
	uint8_t* bp = buf;
	int i = 0;
	bool fallback = false;
	int count = 0;
	int boot_image = 0;
	unsigned int reprogram_control;
	if(rp->rom_version != 0x2)
	{
		current = ((dagrom_romreg(rp) & BIT30) == BIT30);
	}
	/*for Version 2 of the ROM controller to read out the current image.*/
	/*Read out from DAGROM_REPROGRAM_CONTROL register BIT 0:1 for the first image where
	00 - stable image 01 - current 10 - bad CPLD 11 - To Be Done.*/
	/*The status of current no longer outputs a yes or a no as we have 4 images / FPGA.
	however atleast as of now 1 means current 0 means stable and 2 - faulty CPLD and 3 means TBD.*/
	if(rp->rom_version == 2)
	{
		boot_image = read_next_boot_image(rp,0);
	}
	reprogram_control = *(unsigned int*)(rp->iom + rp->base + DAGROM_REPROGRAM_CONTROL);
	if(rp->rom_version == 2)
	{
		if((reprogram_control & 0x3) == 0)
		{	
			current = 0;
		}
		else if ((reprogram_control & 0x3) == 1)
		{	
			current = 1;
		}
		else if((reprogram_control & 0x3) == 2)
		{
			current = 2;
		}
		else if((reprogram_control & 0x3) == 3)
		{
			current = 3;
		}
		/*Support for additional FPGA. for the Dag 9.1x cards.*/
	}
	/*Check for Fallback.If Fallback is on the Image is always 0.*/
	if(rp->rom_version == 2)
	{
		if(reprogram_control & (1 << 12))
		{
			fallback = true; /*Always factory image is active*/
		}
	}
	if(rp->rom_version == 2)
	{
		/*Find the number of valid entries in the image table..*/
		for(i = 0;i < 16;i++)
		{
			if((rp->itable.atable[i].start_address != 0)&& (rp->itable.atable[i].end_address!= 0))
				count++;
		}
		i = 0;
		/*check if images are there.*/
		for(i = 0;i < 4;i++)
		{
			if((rp->romread(rp,rp->itable.atable[i].start_address) == 0xfc30) && (rp->romread(rp,(rp->itable.atable[i].start_address + 2)) == 0xffff))
	 		{
				image_valid[i] = true;
				dagutil_verbose("location %d contains a valid image.\n",i);
			}
	 		else
	 		{
				dagutil_verbose("location %d does not contain a valid image\n",i);
		 	}	
		}
	}
	if(rp->rom_version == 2)
	{
		if(current == 0)
		{
			if(fallback == false)
			{
				aactive = "(active)";
				bactive = "";
				cactive = "";
				dactive = "";
			}else
			{
				printf("Invalid condition \n");
			}
		}
		else if(current == 1)
		{	
			if(fallback == false)
			{
				aactive = "";
				bactive = "(active)";
				cactive = "";
				dactive = "";
			}else
			{
				aactive = "(active)";
				bactive = "(failed)";
				cactive = "";
				dactive = "";
			}
		}
		else if (current == 2)
		{	
			if(fallback == false)
			{
				aactive = "";
				bactive = "";
				cactive = "(active)";
				dactive = "";
			}else
			{
				aactive = "(active)";
				bactive = "";
				cactive = "(failed)";
				dactive = "";
			}
		}
		else /*current = 3*/ 
		{
			if(fallback == false)
			{
				aactive = "";
				bactive = "";
				cactive = "";
				dactive = "(active)";
			}else
			{
				aactive = "(active)";
				bactive = "";
				cactive = "";
				dactive = "(failed)";

			}
		}
	}
	else
	{
		if (current)
		{
			factive = " (active)";
			sactive = "";
		}
		else
		{
			factive = "";
			sactive = " (active)";
		}
	}
	i = 0;
	if(rp->rom_version == 2)
	{
		while(i < count  /*&& image_valid[i] == true*/)
		{
			
			for (addr = (rp->itable.atable[i].start_address + sign_field_width); addr < (rp->itable.atable[i].start_address + sizeof(buf) + sign_field_width); addr+=2)
			{	
				value = rp->romread(rp, addr);
				*(bp + 1) = (value & 0xff00) >> 8;
				*bp =  (value & 0xff);
				bp++;
				bp++;
			}
			bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
			if(i == 0)
			{	
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",second_new,bp,aactive);
				}else
					printf("%s No Valid Image \n",second_new);
			}
			else if(i == 1)
			{
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",first_new,bp,bactive);
				}else
					printf("%s No Valid Image \n",first_new);
			}
			else if(i == 2)
			{	
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",third,bp,cactive);
				}else
					printf("%s No Valid Image \n",third);
			}
			else if(i == 3)
			{
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",fourth,bp,dactive);
				}else
					printf("%s No Valid Image \n",fourth);
			}
			bp = buf;
			i++;
		}
		switch(boot_image)
        	{
                	case 0:
                        	printf("Factory Image will load on power up cycle \n");
                        	break;
                	case 1:
                        	printf("User Image 1 will load on power up cycle \n");
                        	break;
                	case 2:
                        	printf("User Image 2 will load on power up cycle \n");
                        	break;
                	case 3:
                        	printf("User Image 3 will load on power up cycle \n");
                        	break;
                	default:
                        	break;
		}

		if(!(dagrom_check_for_copro(rp)))
		{
			dagrom_print_serial(rp);
		}
		return;
	}
	else
	{
		for (addr = DAGROM_BSTART; addr < (DAGROM_BSTART + sizeof(buf)); addr++)
		{	
			*bp = rp->romread(rp, addr);
			bp++;
		}
		bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
		printf("%s\t\t%s%s\n",first,bp,factive);
	
		bp = buf;
		for (addr = DAGROM_TSTART; addr < (DAGROM_TSTART + sizeof(buf)); addr++)
		{
			*bp = rp->romread(rp, addr);
			bp++;
		}
	}
	/* If serial number is present, skip over */
	if (*(int *) buf == 0x12345678)
	{
		serial = (*(int *) (buf + 4));
		bp = buf;
		
		for (addr = DAGROM_TSTART + DAGSERIAL_SIZE; addr < (DAGROM_TSTART + DAGSERIAL_SIZE + sizeof(buf)); addr++)
		{
			*bp = rp->romread(rp, addr);
			bp++;
		}
	}
	bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
	printf("%s\t%s%s\n", second, bp,sactive);
	if (serial)
		printf("Card Serial: %d\n", serial);
}
//fixme to add support for the small copro flash rom
// and not to be executed if no copr
void 
dagrom_print_serial(romtab_t *rp)
{
	uint32_t serial = 0;
	uint32_t sign = 0;
	uint32_t val = 0;
	uint32_t index,addr = 0;
	uint16_t value_array[16];
	uint32_t size = ( (rp->rblock.sectors[0] * rp->rblock.size_region[0]) + (rp->rblock.sectors[1]* rp->rblock.size_region[1]) );
	for(addr = (size - 128),index = 0; addr < (size - 128 + 32);addr+=2,index++)
	{
		value_array[index] = rp->romread(rp,addr);
	}
	sign = ((value_array[1] << 16) | (value_array[0]));	
	if(sign == 0x12345678)
	{
		serial = ((value_array[3] << 16) | (value_array[2]));
		printf("Card Serial: %d\n",serial);
	}
	/*To print whether if two jumpers are fitted/not.*/
	val = dagrom_reprogram_control_reg(rp);	
	if(val & BIT16)
		dagutil_warning("Force Factory jumper is fitted.FPGA's will program the factory image on power up cycle.\n");
	
	if(val & BIT17)
		dagutil_warning("Inhibit Program jumper is fitted.The FPGA's will not be programmed on power up cycle \n");
}
void
dagrom_xrev_copro(romtab_t* rp)
{
    /* current is user 
     * stable is factory
     */



	int current = ((dagrom_romreg(rp) & BIT30) == BIT30);
	int serial = 0;
	char *first = "user: ";
	char *second ="factory:";
	char* first_new  = "user  1:";
	char* second_new = "factory:";
	char* third  = "user  2:";
	char* fourth = "user  3:";
	char* factive = "";
	char* sactive = "";
	char* aactive = "";
	char* bactive = "";
	char* cactive = "";
	char* dactive = "";
	bool copro_not_configured = false;
	bool image_valid[16] = {false,false,false,false,
			       false,false,false,false,
			       false,false,false,false,
			       false,false,false,false		
				};	
	uint32_t addr;
	int boot_image = 0;
	uint8_t buf[128];
	int value = 0;
	int sign_field_width = 4; /*Width of the signature field.For Version 2*/
	uint8_t* bp = buf;
	int i = 0;
	bool fallback = false;
	int count = 0;
	unsigned int reprogram_control = *(unsigned int*)(rp->iom + rp->base + DAGROM_REPROGRAM_CONTROL);

	if(rp->rom_version == 2)
	{
		boot_image = read_next_boot_image(rp,1);
	}
	
	if(rp->rom_version == 2)
	{
		if(((reprogram_control & 0xc) >> 2 )== 0)
		{	
			current = 0;
		}
		else if (((reprogram_control & 0xc) >> 2 )== 1)
		{	
			current = 1;
		}
		else if(((reprogram_control & 0xc) >> 2 )== 2)
		{
			current = 2;
		}
		else if(((reprogram_control & 0xc) >> 2) == 3)
		{
			current = 3;
		}
		/*Also check if FPGA 1 is configured.It is possible that it is not configured at all.*/
		if((!(reprogram_control & BIT9)) == BIT9)
		{	/*FPGA1 - COPRO is not configured.*/
			copro_not_configured = true;
		}
		/*Support for additional FPGA. for the Dag 9.1x cards.*/
	}
	if(rp->rom_version == 2)
	{
		if(reprogram_control & (1 << 13))
		{
			fallback = true; /*Always factory image is active*/
		}
	}
	if(rp->rom_version == 2)
	{
		/*Find the number of valid entries in the image table..*/
		for(i = 0;i < 16;i++)
		{
			if((rp->itable.atable[i].start_address != 0)&& (rp->itable.atable[i].end_address!= 0))
				count++;
		}
		i = 4;
		/*check if images are there.*/
		for(i = 4;i < count;i++)
		{
			if((rp->romread(rp,rp->itable.atable[i].start_address) == 0xfc30) && (rp->romread(rp,(rp->itable.atable[i].start_address + 2)) == 0xffff))
	 		{
				image_valid[i] = true;
				dagutil_verbose("location %d contains a valid image.\n",i);
			}
	 		else
	 		{
				dagutil_verbose("location %d does not contain a valid image\n",i);
		 	}	
		}
	}
	if(rp->rom_version == 2)
	{
		if(copro_not_configured == true)
		{	
			printf("Fitted but no image loaded \n");
			aactive = "";
			bactive = "";
			cactive = "";
			dactive = "";
		}
		else
		{
			if(current == 0)
			{
				if(fallback == false)
				{
					aactive = "(active)";
					bactive = "";
					cactive = "";
					dactive = "";
				}else
				{
					printf("Invalid condition \n");
				}
			}
			else if(current == 1)
			{		
				if(fallback == false)
				{
					aactive = "";
					bactive = "(active)";
					cactive = "";
					dactive = "";
				}else
				{
					aactive = "(active)";
					bactive = "(failed)";
					cactive = "";
					dactive = "";
				}
			}
			else if (current == 2)
			{	
				if(fallback == false)
				{
					aactive = "";
					bactive = "";
					cactive = "(active)";
					dactive = "";
				}else
				{
					aactive = "(active)";
					bactive = "";
					cactive = "(failed)";
					dactive = "";
				}
			}
			else 
			{
				if(fallback == false)
				{
					aactive = "";
					bactive = "";
					cactive = "";
					dactive = "(active)";
				}else
				{
					aactive = "(active)";
					bactive = "";
					cactive = "";
					dactive = "(failed)";	
				}
			}
		}
	}
	else
	{
		if (current)
		{
			factive = " (active)";
			sactive = "";
		}
		else
		{
			factive = "";
			sactive = " (active)";
		}
	}
	//start of current copro start
	i = 4;
	if(rp->rom_version == 2)
	{
		
		while(i < count  /*&& image_valid[i] == true*/)
		{
			
			for (addr = (rp->itable.atable[i].start_address + sign_field_width); addr < (rp->itable.atable[i].start_address + sizeof(buf) + sign_field_width); addr+=2)
			{	
				value = rp->romread(rp, addr);
				*(bp + 1) = (value & 0xff00) >> 8;
				*bp =  (value & 0xff);
				bp++;
				bp++;
			}
			bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
			if(i == 4)
			{	
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",second_new,bp,aactive);
				}else
					printf("%s No Valid Image \n",second_new);
			}
			else if(i == 5)
			{
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",first_new,bp,bactive);
				}else
					printf("%s No Valid Image \n",first_new);
			}
			else if(i == 6)
			{	
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",third,bp,cactive);
				}else
					printf("%s No Valid Image \n",third);
			}
			else if(i == 7)
			{
				if(image_valid[i] == true)
				{
					printf("%s %s%s\n",fourth,bp,dactive);
				}else
					printf("%s No Valid Image \n",fourth);
			}
			bp = buf;
			i++;
		}
		switch(boot_image)
        {
                	case 0:
                        	printf("Factory Image will load on power up cycle \n");
                        	break;
                	case 1:
                        	printf("User Image 1 will load on power up cycle \n");
                        	break;
                	case 2:
                        	printf("User Image 2 will load on power up cycle \n");
                        	break;
                	case 3:
                        	printf("User Image 3 will load on power up cycle \n");
                        	break;
                	default:
                        	break;
		}
		dagrom_print_serial(rp);
		return;
	}
	else
	{
		for (addr = COPRO_CURRENT_START_BIG; addr < (COPRO_CURRENT_START_BIG + sizeof(buf)); addr++)
		{
			*bp = rp->romread(rp, addr);
			bp++;
		}
		bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
		printf("%s\t\t%s%s\n", first, bp, factive);
	
		bp = buf;
		for (addr = COPRO_STABLE_START_BIG; addr < (COPRO_STABLE_START_BIG + sizeof(buf)); addr++)
		{
			*bp = rp->romread(rp, addr);
			bp++;
		}
	}
	/* If serial number is present, skip over */
	if (*(int *) buf == 0x12345678)
	{
	
	       printf("SHOULD OR SHOUDL NOT HAPPEN CHECKME\n");
		serial = (*(int *) (buf + 4));
		bp = buf;
		for (addr = DAGROM_TSTART + DAGSERIAL_SIZE; addr < (DAGROM_TSTART + DAGSERIAL_SIZE + sizeof(buf)); addr++)
		{
			*bp = rp->romread(rp, addr);
			bp++;
		}
	}
	
	
	bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
	printf("%s\t%s%s\n", second, bp, sactive);

	//TO DO.
	if (serial)
		printf("Card Serial: %d\n", serial);
}


void 
dagrom_display_active_configuration(romtab_t * rp)
{
    /* current is user 
     * stable is factory
     */
	int current = ((dagrom_romreg(rp) & BIT30) == BIT30);
	int serial = 0;
	char* active;
	uint32_t addr;
	uint8_t buf[128];
	uint8_t* bp = buf;

	if (current)
	{
		active = " (user)";
	}
	else
	{
		active = " (factory)";
	}
	
	for (addr = DAGROM_BSTART; addr < (DAGROM_BSTART + sizeof(buf)); addr++)
	{
		*bp = rp->romread(rp, addr);
		bp++;
	}
	
	bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
    if(current)
	    printf("Firmware: %s %s\n", bp, active);
    
	bp = buf;
	for (addr = DAGROM_TSTART; addr < (DAGROM_TSTART + sizeof(buf)); addr++)
	{
		*bp = rp->romread(rp, addr);
		bp++;
	}

	/* If serial number is present, skip over */
	if (*(int *) buf == 0x12345678)
	{
		serial = (*(int *) (buf + 4));
		bp = buf;
		for (addr = DAGROM_TSTART + DAGSERIAL_SIZE; addr < (DAGROM_TSTART + DAGSERIAL_SIZE + sizeof(buf)); addr++)
		{
			*bp = rp->romread(rp, addr);
			bp++;
		}
	}
	
	bp = (uint8_t*) dag_xrev_name(buf, sizeof(buf));
    if(current == 0)
    	printf("Firmware: %s %s\n", bp, active);

	if (serial)
		printf("Card Serial: %d\n", serial);
}
uint64_t
dagrom_read_mac(romtab_t * rp, int port_index)
{
    uint8_t buf[128];
    uint8_t* bp = buf;
    uint32_t addr;
        
    bp = buf;
    for(addr = DAGROM_TSTART; addr < (DAGROM_TSTART + sizeof(buf)); addr++)
    {
        *bp = rp->romread(rp,addr);
        bp++;
    }
    if(*(int*)buf == 0x12345678)
    {
        /* 
         * 8 is the offset to the first MAC address, and each mac address is 8 bytes long
         */
        return  (*(uint64_t*)(buf + 8 + (8*port_index)));
    }
    return 0;
}




/**
 * This function loads the firmware stored in the current (also known as user) half
 * of the flash ROM into the FPGA. This function will block for at least 100ms while
 * the FPGA programming is taking place.  Note that if a corrupt or invalid image
 * is loaded into the flash ROM, then the FPGA will not accept the image and the stable
 * half will be reverted to. The following code snippet can be used to  determine which
 * firmware half (current or stable) is loaded:
 *
 * @code
 *        int current = ((dagrom_romreg(rp) & BIT30) == BIT30);
 *        printf ("Active firmware = %s", current ? "User" : "Stable");
 * @endcode
 *
 * This function will obviously completely reset the card and put it back in the initial
 * boot-up state. Therefore any configuration options will be reset to the defaults
 * and all active capture sessions will be terminated.
 * 
 * @param[in]  dagfd        The file descriptor of the DAG driver, this should have
 *                          been returned by the dag_open function.
 * @param[in]  rp           Pointer to the ROM structure initialised by the dagrom_init
 *                          function. This parameter may be NULL, however it is not
 *                          recommended and may break future versions.
 *
 * @sa dag_open | dagrom_init | dagrom_loadstable
 */
void
dagrom_loadcurrent(uint32_t dagfd, romtab_t* rp)
{
	dag_reg_t           result[DAG_REG_MAX_ENTRIES];
	dag_reg_t         * regs;
	unsigned int        regn = 0;
	volatile uint8_t  * iom_p = dag_iom(dagfd);
	volatile uint32_t * general_reg_p;
	volatile uint32_t * rom_ctrl_reg_p;
	volatile uint32_t *interrupt_mask_p;
	volatile uint32_t *interrupts_p;
	int i;

	/* Currently there are 2 ways of loading the current half:
	 *
	 * 1. The original way (still used on most cards), is to write bit 29 in the ROM
	 *    programming register, delay a bit and then tell the driver to reload the
	 *    PCI configuration space.
	 *
	 * 2. The other way that is used on some PCI-e cards is to setup the general
	 *    purpose register to tell the firmware when the link status goes down
	 *    we should re-program either the current or stable half. This requires
	 *    support from the driver to change the link status. On linux this
	 *    done via the the standard DAGIOCRESET IOCTL with the DAGRESET_FULL arg.
	 *    On windows the whole process is done by the ChangeFirmwareHalf function.
	 *    The new format of the general reset register (0x88) is as follows:
	 *        bit       use
	 *        31        L2_RST to design
	 *        30        L1_RST to design / CPLD
	 *        27        on next link down, reprogram
	 *        26        use new reprogramming mechanism
	 *        25        reprogram to Current / User half
	 *        24        reprogram to Stable / Factory half
	 *        23        read '1' if running out of the Current Half
	 *        20        always read as '1' to indicate new programming available. If 	
	 o                  read as 0 then only old programming available.
	 *
	 *
	 * To determine which method to use, we look at the version of the general
	 * register (register 0x88), if it is version 1 we use method 2 above.
	 * Added two new methods to reprogram the FPGA for the new version of the ROM Controller.
	 * 3. Method George.
	 * Arm the FPGA to be reprogram by method George.then issue the interrupt- 
	 * instruct the upstream bridge to undergo link disable + retrain link.
         * 4 Method Dave	
	 * Arm the FPGA to be reprogramed by method Dave.the issue the interrupt -
	 * instruct the upstream bridge to undergo Secondry Bus Hot Reset.	
	 */
	/* Check for the firmware revision to see if we can use the safe PCI-E load current */
	regs = dag_regs(dagfd);
	dag_reg_table_find(regs, 0, DAG_REG_GENERAL, result, &regn);
	general_reg_p = (volatile uint32_t *) ((uintptr_t)iom_p + result[0].addr + 8);
	
	/* make sure there are no outstanding interrupts */
	//Disable Interrupts
	interrupt_mask_p = (volatile uint32_t *) ((uintptr_t)iom_p + result[0].addr + 0xc);
	*interrupt_mask_p = 0x0;
	usleep(1000);
	
	//check for outstanding interrupts 
	interrupts_p = (volatile uint32_t *) ((uintptr_t)iom_p + result[0].addr + 0);
	for(i=0; i<10; i++) {	
		if( (((*interrupts_p) & 0xFFFF) == 0xffff) || ( ((*interrupts_p) & 0xFFFF) == 0x0) ) 
			break;
		
		dagutil_warning("Warning: this message is acceptable but should not happened very often 0x%x\n",(*interrupts_p) & 0xFFFF); 
		usleep(50000);
	};

	/*For Version 2 of the Rom Controller.*/
	/*Have to use Reprogramming Control Register and Development Reset Control Register.*/
	/*Step 1: Select the Image to be programmed. - As of now Current / Stable - in this function current.*/
	/*Step 2: Arm the PCI FPGA to be reprogrammed with Ringo,George/Dave,Paul,John*/
	if(rp->rom_version == 2)
	{
		if (image_number < 4)
		{
			if(image_number == 0)
			{
				dagutil_warning("To switch to the factory image (0), the use of dagreset is recommended!\n");
			}else
			{
				
				if(dagrom_check_for_copro(rp))
				{
					image_number |= (image_number << 2);
					image_number |= (BIT8|BIT9);
					(*(unsigned int*)(rp->iom + rp->base + DAGROM_REPROGRAM_CONTROL)) = image_number;
					dagutil_verbose("Reprogram Control Register contains %d \n",image_number);
				}
				else	
				{
					(*(unsigned int*)(rp->iom + rp->base + DAGROM_REPROGRAM_CONTROL)) = image_number;
				}
			}
		}	
		if(program_method == 1)
		{
			dagutil_verbose_level(3,"Program method : %x\n",program_method);
			(*(unsigned int*)(rp->iom + rp->base + DAGROM_RESET_CONTROL)) = 2;
			usleep(100 * 1000);
			/*Method 1*/
			/*Tell the FPGA to reload the PCI config registers */
	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
			{
				int magic = DAGRESET_REBOOT;
				if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
					dagutil_error("loadcurrent DAGIOCRESET DAGRESET_REBOOT: %s\n", strerror(errno));
			}
	#elif defined(_WIN32)
			{
				DWORD BytesTransferred;
				if (DeviceIoControl(dag_gethandle(dagfd), IOCTL_REBOOT, NULL, 0, NULL, 0, &BytesTransferred, NULL) == FALSE)
					dagutil_error("loadcurrent IOCTL_REBOOT\n");
			}
	#endif /* Platform-specific code. */
		}
		#if 0
		else if(program_method == 2) /*Hot Burn Reset for PCI -Express*/
		{
			dagutil_verbose_level(3,"Hot Burn Reset \n");
			(*(unsigned int*)(rp->iom + rp->base + DAGROM_RESET_CONTROL)) = 4; /*Hot Burn Reset*/
			usleep(100 * 1000);
			/*Method 1*/
			/*Tell the FPGA to reload the PCI config registers */
	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
			int magic = DAGRESET_FULL;
		

			*general_reg_p |= (BIT27 | BIT26 | BIT25);
			*general_reg_p &= ~BIT24;

			if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
				dagutil_error("loadcurrent DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));

	#elif defined(_WIN32)
			ChangeFirmwareHalf(dagfd, 1, rp->rom_version);


	#endif /* Platform-specific code. */
		}
		#endif
		else if(program_method == 2)
		{
			dagutil_verbose_level(3,"reprogram by method George(Link Disable + Link Retrain) \n");
			(*(unsigned int*)(rp->iom + rp->base + DAGROM_RESET_CONTROL)) = 4;

	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
			{
				int magic;
				/*Reprogram the FPGA by method George.
				Arm the FPGA to be reprogrammed by method George.
				Then instruct the upstream bridge to undergo Link Disable + Link Retrain.
				*/
				magic = DAGRESET_GEORGE;
				if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
					dagutil_error("loadcurrent DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
			}
	#elif defined(_WIN32)
			ChangeFirmwareHalf(dagfd, 1, rp->rom_version);
	#endif /* Platform-specific code. */
		}
		else if(program_method == 3)
		{
			dagutil_verbose_level(3,"reprogram by method Dave(Secondary Bus Hot Reset) \n");
			(*(unsigned int*)(rp->iom + rp->base + DAGROM_RESET_CONTROL)) = 4;

	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
			{
				int magic;
				/*Arm the FPGA to be reprogrammed by method Dave.*/
				/*The instruct the upstream bridge to undergo Secondry Bus Hot Reset.*/
				magic = DAGRESET_DAVE;
				if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
					dagutil_error("loadcurrent DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
			}
	#elif defined(_WIN32)
			ChangeFirmwareHalf(dagfd, 1, rp->rom_version);
	#endif /* Platform-specific code. */
		}
		else
			printf("Please specify a correct reset method.\n");
	}
	/* Method 1 - Original method using the ROM programming register */
	else 
#if  defined(__FreeBSD__)  
	if (1) 
#else 
	if ( /*(result[0].version != 0x1) ||*/ !(*general_reg_p & BIT20) )
#endif 	
	{
		/* L1 reset with loading (current half) to firmware */
		if ( rp != NULL )
		{
			rp->romwrite(rp, 0x0, BIT29);
		}
		else
		{
			if ( dag_reg_table_find(regs, 0, DAG_REG_ROM, result, &regn) < 1 )
				return;
				
			rom_ctrl_reg_p = (volatile uint32_t *) ((uintptr_t)iom_p + result[0].addr + 0);
			*rom_ctrl_reg_p = BIT29;
		}

		usleep(100 * 1000);

		/* Tell the FPGA to reload the PCI config registers */
	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		{
			int magic = DAGRESET_REBOOT;
			if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
				dagutil_error("loadcurrent DAGIOCRESET DAGRESET_REBOOT: %s\n", strerror(errno));
		}
	#elif defined(_WIN32)
		{
			DWORD BytesTransferred;
			if (DeviceIoControl(dag_gethandle(dagfd), IOCTL_REBOOT, NULL, 0, NULL, 0, &BytesTransferred, NULL) == FALSE)
				dagutil_error("loadcurrent IOCTL_REBOOT\n");
		}
	#endif /* Platform-specific code. */
	}
	
	
	/* Method 2 - Use the extended PCI-e method */
	else
	{	
	#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		int magic = DAGRESET_FULL;

		*general_reg_p |= (BIT27 | BIT26 | BIT25);
		*general_reg_p &= ~BIT24;

		if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
			dagutil_error("loadcurrent DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
		
	#elif defined(_WIN32)
		
		ChangeFirmwareHalf(dagfd, 1, rp->rom_version);

	#endif /* Platform-specific code. */
	}
}
/**
 * This function loads the firmware stored in the stable half of the flash ROM into
 * the FPGA. This function will block for at least 100ms and upto 500ms while
 * the FPGA programming is taking place.  This is equivalent to running the command
 * line tool dagreset.
 *
 * This function will obviously completely reset the card and put it back in the initial
 * boot-up state. Therefore any configuration options will be reset to the defaults
 * and all active capture sessions will be terminated.
 * 
 * @param[in]  dagfd        The file descriptor of the DAG driver, this should have
 *                          been returned by the dag_open function.
 *
 * @sa dag_open | dagrom_loadcurrent
 */
void
dagrom_loadstable(uint32_t dagfd)
{
	dag_reg_t           result[DAG_REG_MAX_ENTRIES];
	dag_reg_t         * regs;
	unsigned int        regn = 0;
	volatile uint8_t  * iom_p = dag_iom(dagfd);
	volatile uint32_t * general_reg_p;
	volatile uint32_t * interrupt_mask_p;
	volatile uint32_t * interrupts_p;
	int i;
	int method = 3;/*default method is Dave*/
	/* Currently there are 2 ways of loading the stable half:
	 *
	 * 1. The original way (still used on most cards), is to send an IOCTL to the
	 *    driver, which issues a reset and then reloads the PCI config space.
	 *
	 * 2. The other way that is used on some PCI-e cards is to setup the general
	 *    purpose register to tell the firmware when the link status goes down
	 *    we should re-program either the current or stable half. This requires
	 *    support from the driver to change the link status. On linux this
	 *    done via the the standard DAGIOCRESET IOCTL with the DAGRESET_FULL arg.
	 *    On windows the whole process is done by the ChangeFirmwareHalf function.
	 *    The new format of the general reset register (0x88) is as follows:
	 *        bit       use
	 *        31        L2_RST to design
	 *        30        L1_RST to design / CPLD
	 *        27        on next link down, reprogram
	 *        26        use new reprogramming mechanism
	 *        25        reprogram to Current / User half
	 *        24        reprogram to Stable / Factory half
	 *        23        read '1' if running out of the Current Half
	 *        20        always read as '1' to indicate new programming available. If 	
	 *                  read as 0 then only old programming available.
	 *
	 *
	 * To determine which method to use, we look at the version of the general
	 * register (register 0x88), if it is version 1 we use method 2 above.
	 *
	 */


	/* Check for the firmware revision to see if we can use the safe PCI-E load current */
	regs = dag_regs(dagfd);
	dag_reg_table_find(regs, 0, DAG_REG_GENERAL, result, &regn);
	general_reg_p = (volatile uint32_t *) ((uintptr_t)iom_p + result[0].addr + 8);
	
		
	/* make sure there are no outstanding interrupts */
	//Disable Interrupts
	interrupt_mask_p = (volatile uint32_t *) ((uintptr_t)iom_p + result[0].addr + 0xc);
	*interrupt_mask_p = 0x0;
	usleep(1000);
	
	//check for outstanding interrupts 
	interrupts_p = (volatile uint32_t *) ((uintptr_t)iom_p + result[0].addr + 0);
		for(i=0; i<10; i++) {	
				if( ((*interrupts_p & 0xFFFF) == 0xffff) || ( ((*interrupts_p) & 0xFFFF) == 0x0) ) 
			break;
		

		dagutil_warning("Warning: this message is acceptable but should not happened very often 0x%x\n",(*interrupts_p) & 0xFFFF);
		usleep(50000);
	};
	
	//dag_reg_table_find(regs,0,DAG_REG_ROM,result,&regn);

	if(dag_reg_find((char*)iom_p,DAG_REG_ROM,result)< 1)
	{
		dagutil_error("dag reg find for ROM controller failed \n");
//		return EINVAL;
	}

	/*Version 2 of the rom controller multiple reset methods supported*/
	if(result[0].version == 2)
	{
			dagutil_verbose_level(3,"setting image number to 0\n");
		 (*(unsigned int*)(iom_p + result[0].addr + DAGROM_REPROGRAM_CONTROL)) = 0;

		if(method == 1)
		{
			*(unsigned int*)(iom_p + result[0].addr + DAGROM_RESET_CONTROL) = 2; /*using method ringo*/
	
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
			{
				int magic = DAGRESET_FULL;
				if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
					dagutil_error("[dagrom_loadstable] DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
			}
#elif defined(_WIN32)
			{
				DWORD BytesTransferred;
				if (DeviceIoControl(dag_gethandle(dagfd), IOCTL_RESET, NULL, 0, NULL, 0, &BytesTransferred, NULL) == FALSE)
					dagutil_error("[dagrom_loadstable] IOCTL_RESET\n");
			}
#endif /*Platform Specific code */
		}
		else if(method == 2)
		{
			
			(*(volatile uint32_t*)((uintptr_t)iom_p + result[0].addr + DAGROM_RESET_CONTROL)) = 4; /*using method George/Dave*/
	
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
			{
				int magic = DAGRESET_GEORGE;
				if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
					dagutil_error("[dagrom_loadstable] DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
			}
#elif defined(_WIN32)

			ChangeFirmwareHalf(dagfd, 0, result[0].version);

#endif /* Platform-specific code. */
		}
		else if(program_method == 3)
		{
			(*(volatile uint32_t*)((uintptr_t)iom_p + result[0].addr + DAGROM_RESET_CONTROL)) = 4; /*using method George/Dave*/

#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
			{
				int magic = DAGRESET_DAVE;
				if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
					dagutil_error("[dagrom_loadstable] DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
			}
#elif defined(_WIN32)

			ChangeFirmwareHalf(dagfd, 0, result[0].version);

#endif /* Platform-specific code. */
		}
		else
			printf("Please specify a correct reset method.\n");
	
	}
	/* Method 1 - Original method using the driver */
	else if ( /*(result[0].version != 0x1) ||*/ !(*general_reg_p & BIT20) )
	{
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		{
			int magic = DAGRESET_FULL;
			if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
				dagutil_error("[dagrom_loadstable] DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
		}
#elif defined(_WIN32)
		{
			DWORD BytesTransferred;
			if (DeviceIoControl(dag_gethandle(dagfd), IOCTL_RESET, NULL, 0, NULL, 0, &BytesTransferred, NULL) == FALSE)
				dagutil_error("[dagrom_loadstable] IOCTL_RESET\n");
		}
#endif /* Platform-specific code. */
	}
	
	
	/* Method 2 - Use the extended PCI-e method */
	else
	{	
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
		int magic = DAGRESET_FULL;

		*general_reg_p |= (BIT27 | BIT26 | BIT24);
		*general_reg_p &= ~BIT25;

		if (ioctl(dagfd, DAGIOCRESET, &magic) < 0)
			dagutil_error("[dagrom_loadstable] DAGIOCRESET DAGRESET_FULL: %s\n", strerror(errno));
		
#elif defined(_WIN32)
		
		ChangeFirmwareHalf(dagfd, 0, result[0].version);

#endif /* Platform-specific code. */
	}
	
}


/* --------------------------------------------------------- */

/*  dagrom_pbihold : Gets the XScale processor to release the Bus
*/
void
dagrom_pbihold(romtab_t* rp)
{
	volatile uint32_t *xscale_control = NULL;
    
	/* Get PBI bus from XScale */
	xscale_control = (volatile uint32_t *) (rp->iom + 0x15c);

    orig_pbi_state = *xscale_control;
    
	*xscale_control = 0x11; /* HOLD_A request to XScale for the PBI bus */
}

/*  dagrom_pbirelease : Releases the bus */
void
dagrom_pbirelease(romtab_t* rp)
{
	volatile uint32_t *xscale_control = NULL;

	xscale_control = (volatile uint32_t *) (rp->iom + 0x15c);

	*xscale_control = orig_pbi_state; /* return XScale to the original state */
}

/* --------------------------------------------------------- */

uint32_t
dagrom_procsize(int target_processor, int region)
{
	return processor_region[target_processor][region].size;
}


uint32_t
dagrom_procstart(int target_processor, int region)
{
	return processor_region[target_processor][region].start;
}


const char*
dagrom_procname(int target_processor, int region)
{
	return processor_region[target_processor][region].name;
}

/*	Function: dagrom_mpuname(int mpu_id)
	Return the name of the processor as an asciiz string.
	Args: 	mpu_id	- processor id.
	Returns: Pointer to the processor name string, or
			NULL if the mpu_id is not valid.
*/
const char* dagrom_mpuname(int mpu_id)
{
	/* Only return the name for valid ids */
	if ((mpu_id >= 0) && (mpu_id < DAG_MPU_TOTAL)) {
		return processor_name[mpu_id];
	} else {
		return NULL;
	}
}


void
dagrom_set_fast_cfi_mode(int mode)
{
	uFastCfiMode = mode;
}
void
dagrom_set_program_method(int method)
{
	program_method = method;
}
void 
dagrom_set_image_number(int number)
{
	image_number = number;
}
int dagrom_pbi_bus_status( romtab_t* rp )
{
	/* Try using CFI to identify the ROM */
	if (cfi_identify(rp) < 0)
	{
		return -1;
	}

	return 0;
	
}
/*returns true if copro is present / fitted. else returns false.*/
bool dagrom_check_for_copro(romtab_t *rp)
{
	/*Logic:Read the Image address table.If it contains valid address for entries from 4 -7 that means copro is present / fitted.*/
	int i = 4;
	
	/*Find the number of valid entries in the image table..*/
	/*check if images are there.*/
	if((rp->itable.atable[i].start_address != 0)&&(rp->itable.atable[i].end_address!= 0))
	{
	 	return true; /*Yes,this card does have a copro fitted.*/
	}
	return false; /*No this card does not have a copro fitted.*/

}
