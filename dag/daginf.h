/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
* $Id: daginf.h,v 1.34.2.2 2009/07/14 03:43:51 vladimir Exp $
 */

#ifndef DAGINF_H
#define DAGINF_H

#if defined(__linux__)

#ifndef __KERNEL__
#include <inttypes.h>  /* The Linux kernel has its own stdint types. */
#include <time.h>
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
#if (__FreeBSD_version >= 700000)
#include <sys/time.h>
#else 
#include <time.h>
#endif 

#elif defined(__NetBSD__)

#ifndef _KERNEL
#include <inttypes.h>
#else
#include <sys/param.h>
#include <sys/inttypes.h>  /* So does the FreeBSD kernel. */
#endif /* _KERNEL */
#include <time.h>

#elif defined(_WIN32)

#include <wintypedefs.h>
#include <time.h>

#elif defined(__SVR4) && defined(__sun)

#include <sys/types.h>

#elif defined(__APPLE__) && defined(__ppc__)

#include <inttypes.h>
#include <time.h>

#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif /* Platform-specific code. */
#define DAG_ID_SIZE 20 /* for daginf.bus_id, from BUS_ID_SIZE = KOBJ_NAME_LEN */
typedef struct daginf {
	uint32_t		id;		/* DAG device number */

	uint32_t		phy_addr ;
//	unsigned long		phy_addr;	/* PCI address of large buffer (ptr) */

	uint32_t		buf_size;	/* its size */
	uint32_t		iom_size;	/* iom size */
	uint16_t		device_code;	/* PCI device ID */
#if defined(__linux__) || defined(__FreeBSD__) || (defined (__SVR4) && defined (__sun))
	char			bus_id[DAG_ID_SIZE];
	uint8_t 		brd_rev;	/**Card revision ID which is stored in the PCI configuration space */
#endif
#if defined(_WIN32)
	uint32_t		bus_num;	/* PCI bus number */
	uint16_t		dev_num;	/* PCI device number */
	uint16_t		fun_num;	/* Function number within a PCI device */
	uint32_t		ui_num;		/* User-perceived slot number */
	uint16_t 		brd_rev;	/**Card revision ID which is stored in the PCI configuration space */
#endif
} daginf_t;

/* Bitfields for DAG_IOSETDUCK, Set_Duck_Field parameter */
/* Processing order is impled numerically ascending */
#define Set_Duck_Clear_Stats	0x0001
#define Set_Duck_Health_Thresh	0x0002
#define Set_Duck_Phase_Corr	0x0004

#ifdef _WIN64
#pragma pack (1)
#endif
typedef struct duckinf
{
	int64_t         Phase_Correction;
	uint64_t	Last_Ticks;
	uint64_t	Last_TSC;
	time_t		Stat_Start, Stat_End;
	uint32_t	Crystal_Freq;
	uint32_t	Synth_Freq;
	uint32_t	Resyncs;
	uint32_t	Bad_Pulses;
	uint32_t	Worst_Freq_Err, Worst_Phase_Err;
	uint32_t	Health_Thresh;
	uint32_t	Pulses, Single_Pulses_Missing, Longest_Pulse_Missing;
	uint32_t	Health, Sickness;
	int32_t		Freq_Err, Phase_Err;
	uint32_t	Set_Duck_Field;
} duckinf_t;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DAGINF_H */
