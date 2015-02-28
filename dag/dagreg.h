/*
 * Copyright (c) 2003-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: dagreg.h,v 1.87.2.1 2009/07/14 03:43:51 vladimir Exp $
 */

#ifndef DAGREG_H
#define DAGREG_H


#if defined(__linux__)

#ifndef __KERNEL__
#include <inttypes.h>  /* The Linux kernel has its own stdint types. */
#else
#include <linux/kernel.h>
#endif /* __KERNEL__ */

#elif defined(__FreeBSD__)

#ifndef _KERNEL
#include <inttypes.h>
#else
#include <sys/param.h>
#if (__FreeBSD_version >= 700000)
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/stddef.h>
#elif (__FreeBSD_version >= 504000)
#include <inttypes.h>
#else
#include <sys/inttypes.h>  /* So does the FreeBSD kernel. */
#endif
#endif /* _KERNEL */

#elif defined(__NetBSD__)

#ifndef _KERNEL
#include <inttypes.h>
#else
#include <sys/param.h>
#include <sys/inttypes.h>  /* So does the NetBSD kernel. */
#endif /* _KERNEL */

#elif (defined(__APPLE__) && defined(__ppc__))

#include <stdint.h>

#elif defined(_WIN32)

#include <wintypedefs.h>

#elif defined (__SVR4) && defined (__sun)

#include <sys/types.h>

#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif /* Platform-specific code. */


#define DAG_REG_MAX_ENTRIES 128

#define DAG_REG_ADDR_START 0xffa5
#define DAG_REG_ADDR_END   0xffff

typedef struct dag_reg
{
#if defined(__ppc__) || defined(__POWERPC__)

	/* Big-endian layout. */
	uint32_t  addr    : 16;
	uint32_t  module  : 8;
	uint32_t  version : 4;
	uint32_t  flags   : 4;
	
#else

	/* Little-endian layout. */
	uint32_t  addr    : 16;
	uint32_t  module  : 8;
	uint32_t  flags   : 4;
	uint32_t  version : 4;
	
#endif /* Endianness. */
} dag_reg_t;


typedef enum dag_reg_flags
{
	DAG_REG_FLAG_TABLE = 0x8,
	DAG_REG_FLAG_ARM   = 0x4
	
} dag_reg_flags_t;

/* this enumeration must match the value of the firmware enumeration table */
typedef enum dag_reg_module
{
	DAG_REG_START = 0x00,
	DAG_REG_GENERAL = 0x01,
	DAG_REG_DUCK = 0x02,
	DAG_REG_PBM = 0x03,
	DAG_REG_GPP = 0x04,
	DAG_REG_SONET = 0x05,
	DAG_REG_ROM = 0x06,
	DAG_REG_PTX = 0x07,
	DAG_REG_FPGAP = 0x08,
	DAG_REG_UART = 0x09,
	DAG_REG_BTX = 0x0A,
	DAG_REG_ARM = 0x0B,
	DAG_REG_PM5355 = 0x0C,
	DAG_REG_VSC9112 = 0x0D,
	DAG_REG_ICS1893 = 0x0E,
	DAG_REG_SONIC_3 = 0x0F,
	DAG_REG_SONIC_D = 0x10,
	DAG_REG_MINIMAC = 0x11,
	DAG_REG_S19205 = 0x12,
	DAG_REG_PM5381 = 0x13,
	DAG_REG_MAR = 0x14,
	DAG_REG_HDIM = 0x15,
	DAG_REG_MIPF = 0x16,
	DAG_REG_COPRO = 0x17,
	DAG_REG_ARM_RAM = 0x18,
	DAG_REG_ARM_PCI = 0x19,
	DAG_REG_ARM_X1 = 0x1A,
	DAG_REG_ARM_X2 = 0x1B,
	DAG_REG_ARM_X3 = 0x1C,
	DAG_REG_ARM_PHY = 0x1D,
	DAG_REG_ARM_EEP = 0x1E,
	DAG_REG_PM3386 = 0x1F,
	DAG_REG_CAM_MAINT = 0x20,
	DAG_REG_CAM_SEARCH = 0x21,
	DAG_REG_COPRO_BIST = 0x22,
	DAG_REG_BIST_DG = 0x23,
	DAG_REG_COPRO_COUNTERS = 0x24,
	DAG_REG_SAR_CTRL = 0x25,
	DAG_REG_MSA300_RX = 0x26,
	DAG_REG_MSA300_TX = 0x27,
	DAG_REG_PPF = 0x28,
	DAG_REG_ATX = 0x29,
	DAG_REG_QDR = 0x2A,
	DAG_REG_ATM_ARMOUR = 0x2B,
	DAG_REG_BOPT_GEN = 0x2C,
	DAG_REG_BOPT_RESET = 0x2D,
	DAG_REG_SONIC_E1T1 = 0x2E,
	DAG_REG_E1T1_CTRL = 0x2F,
	DAG_REG_TERF64 = 0x30,
	DAG_REG_PP64 = 0x31,
	DAG_REG_DISP64 = 0x32,
	DAG_REG_DP83865 = 0x33,
	DAG_REG_TAP = 0x34,
	DAG_REG_SMB = 0x35,
	DAG_REG_E1T1_HDLC_DEMAP = 0x36,
	DAG_REG_E1T1_HDLC_MAP = 0x37,
	DAG_REG_E1T1_ATM_DEMAP = 0x38,
	DAG_REG_E1T1_ATM_MAP = 0x39,
	DAG_REG_THRESHMODE = 0x3a,
	DAG_REG_PL3_ARMOUR = 0x3b,
	DAG_REG_RC = 0x3c,
	DAG_REG_RAWCAPTYPE = 0x3d,
	DAG_REG_RAW_TX = 0x3e,
	DAG_REG_SAM_MAINT = 0x3f,
	DAG_REG_MV88E1111 = 0x40,
	DAG_REG_AD2D_BRIDGE = 0x41,
	DAG_REG_BIST = 0x42,
	DAG_REG_FAILSAFE = 0x43,
	DAG_REG_BPFI = 0x44, 
	DAG_REG_IDT_TCAM = 0x45,
	DAG_REG_AMCC = 0x46,
	DAG_REG_XRT79L72 = 0x47,
	DAG_REG_RAW_SMBBUS = 0x48,
	DAG_REG_RAW_MAXDS3182 = 0x49,
	DAG_REG_STEERING = 0x4a,
	DAG_REG_BITHDLC_DEMAP = 0x4b,
	DAG_REG_SRGPP = 0x4c,
	DAG_REG_IXP = 0x4d,
	DAG_REG_HMM = 0x4e,
	DAG_REG_STATS = 0x4f,
	DAG_REG_DROP = 0x50,
	DAG_REG_RANDOM_CAPTURE = 0x51,
	DAG_REG_VSC8476 = 0x52,
	DAG_REG_XGMII = 0x53,
	DAG_REG_HLB = 0x54,
	DAG_REG_OC48_DEFRAMER = 0x55,
	/* 0x56 ? */
	DAG_REG_AMCC3485 = 0x57,
	DAG_REG_DSMU = 0x58,
	DAG_REG_DPA = 0x59,
	/* 0x5a ? */
	/* 0x5b ? */
	DAG_REG_COUNT_INTERF = 0x5C,
	/* 0x5d ? */
	/* 0x5e ? */
	/* 0x5f ? */
	/* 0x60 ? */
	/* 0x61 ? */
	DAG_REG_INFINICLASSIFIER = 0x62,
	/* 0x63 ? */
	DAG_REG_XFP = 0x64,
	DAG_REG_VSC8479 = 0x65,
	DAG_REG_SONET_PP = 0x66,
	/* 0x67 ? */
	DAG_REG_XGE_PCS = 0x68,
	DAG_REG_CAT = 0x69,
	DAG_REG_STREAM_FTR = 0x6A,
	/* 0x6b ? */
	DAG_REG_ROM_SERIAL_NUMBER = 0x6c,
	DAG_REG_INFI_FRAMER = 0x6D,
	DAG_REG_INFINICAM = 0x6E,
	DAG_REG_43GE_LASER_CONTROL = 0x6F,
	DAG_REG_GTP_PHY = 0x70,
	DAG_REG_MEMTX = 0x71,
	DAG_REG_DUCK_READER = 0x72,
	DAG_REG_VSC8486 = 0x74,
	DAG_REG_BFSCAM = 0x79,
	DAG_REG_IRIGB = 0x80,
	/* 0x77 ? */
	/* 0x78 ? */
	/* 0x79 ? */
	DAG_REG_PATTERN_MATCH = 0x7A,
	DAG_REG_SONET_CHANNEL_DETECT = 0x7B,
	DAG_REG_SONET_CHANNEL_CONFIG_MEM = 0x7C,
	DAG_REG_SONET_RAW_SPE_EXTRACT = 0x7D,
	DAG_REG_END = 0xFF
	
} dag_reg_module_t;


#if defined (_WIN32)  || (defined(__APPLE__) && defined(__ppc__))

/* note, the cast to dag_reg_t is not a good idea, since the cast should take place BEFORE memory reference 
 * in order to be useful
 */
#define DAG_REG_ADDR(X)  (((X).flags&DAG_REG_FLAG_ARM)?(X).addr<<16:(X).addr)
#define DAG_REG_VER(X)   ((X).version)

#elif defined(__FreeBSD__) || defined(__linux__) || defined(__NetBSD__) || (defined (__SVR4) && defined (__sun))

#define DAG_REG_ADDR(X) ((((dag_reg_t)X).flags&DAG_REG_FLAG_ARM)?((dag_reg_t)X).addr<<16:((dag_reg_t)X).addr)
#define DAG_REG_VER(X) (((dag_reg_t)X).version)

#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif /* Platform-specific code. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


int dag_reg_find(char *iom, dag_reg_module_t module, dag_reg_t result[DAG_REG_MAX_ENTRIES]);
int dag_reg_table_find(dag_reg_t *table, uint32_t toffset, uint8_t module, dag_reg_t *result, uint32_t *len);

/* Internal dagapi functions */
dag_reg_t* dag_regs(int dagfd);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DAGREG_H */
