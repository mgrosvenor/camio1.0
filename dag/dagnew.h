/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dagnew.h,v 1.39.2.2 2009/11/03 22:26:58 alexey.korolev Exp $
 */

#ifndef DAGNEW_H
#define DAGNEW_H

/* DAG tree headers. */
#include "daginf.h"

/*
 * Driver definitions
 */
typedef enum dagminor
{
	DAGMINORBITS  = 4,
	DAGMINORMASK  = (1<<DAGMINORBITS)-1,
	DAGMINOR_DAG  = 0,
	DAGMINOR_MEM  = 1,
	DAGMINOR_IOM  = 2,
	DAGMINOR_ARM  = 3,
	DAGMINOR_MAX  = 4
	
} dagminor_t;

/*
 * Dag monitor commands
 */
#define DAGMON_MIN              0
#define DAGMON_VERSION          0
#define DAGMON_READ_WORD        1
#define DAGMON_WRITE_WORD       2
#define DAGMON_PROGRAM_XILINX   3
#define DAGMON_ENTER_ADDRESS    4
#define DAGMON_READ_12_WORDS    5
#define DAGMON_WRITE_12_WORDS   6
#define DAGMON_MAX              6

/*
 * Dag monitor responses
 */
#define DAGMON_OK               1
#define DAGMON_GOOD_INIT        2
#define DAGMON_BAD_INIT         3
#define DAGMON_BAD_PROTO        4
#define DAGMON_INVALID_CMD      5
#define DAGMON_UNSUPPORTED_CMD  6
#define DAGMON_RESET_EX         7
#define DAGMON_UNDEFOP_EX       8
#define DAGMON_SOFTWARE_EX      9
#define DAGMON_ABORT_PRE_EX     10
#define DAGMON_ABORT_DATA_EX    11
#define DAGMON_IRQ_EX           12
#define DAGMON_FIQ_EX           13

typedef struct monparams
{
	uint32_t words[16];
} monparams_t;

#define DAG_DEFAULT_NODE      -1
typedef struct user_memory
{
	uint32_t memhole_size;
	uint32_t node;
}user_mem_t;

typedef struct dagpbm
{
	volatile uint32_t   memaddr;      /* 0x00 */
	volatile uint32_t   memsize;      /* 0x04 */
	volatile uint32_t   burstthresh;  /* 0x08 */
	volatile uint32_t   segsize;      /* 0x0c */
	volatile uint32_t   wrsafe;       /* 0x10 */
	volatile uint32_t   redzone;      /* 0x14 */
	volatile uint32_t   curaddr;      /* 0x18 */
	volatile uint32_t   cs;           /* 0x1c */
	volatile uint32_t   capacity;     /* 0x20 */
	volatile uint32_t   underover;    /* 0x24 */
	volatile uint32_t   pciwptr;      /* 0x28 */
	volatile uint32_t   bursttmo;     /* 0x2c */
	volatile uint32_t   build;        /* 0x30 */
	volatile uint32_t   maxlevel;     /* 0x34 */
	volatile uint32_t   segaddr;      /* 0x38 */

} dagpbm_t;

/* PBM mkI structure. */
typedef struct dagpbm_mkI
{
	volatile uint32_t   mem_addr;
	volatile uint32_t   mem_size;
	volatile uint32_t   burst_threshold;
	volatile uint32_t   drop_counter;
	volatile uint32_t   limit_pointer;
	volatile uint32_t   limit_count;
	volatile uint32_t   record_pointer;
	volatile uint32_t   control_status;
	volatile uint32_t   unused_0;
	volatile uint32_t   unused_1;
	volatile uint32_t   burst_timeout;
	volatile uint32_t   unused_2;
	volatile uint32_t   unused_3;
	volatile uint32_t   unused_4;
	
} dagpbm_mkI_t;

/* PBM mkII structure. */
typedef struct dagpbm_mkII
{
	volatile uint32_t control_status;
	volatile uint32_t burst_threshold;
	volatile uint32_t burst_timeout;
	volatile uint32_t unused[13];

} dagpbm_mkII_t;

/* PBM mkII per stream register block */
typedef struct dagpbm_mkII_stream_block
{
	volatile uint32_t stream_status;
	volatile uint32_t mem_addr;
	volatile uint32_t mem_size;
	volatile uint32_t record_pointer;
	volatile uint32_t limit_pointer;
	volatile uint32_t limit_count;
	volatile uint32_t drop_counter;
	volatile uint32_t unused[9];
	
} dagpbm_mkII_stream_block_t;

typedef enum pbmcs
{
	DAGPBM_UNPAUSED = 0x00,
	DAGPBM_PAUSED   = 0x01,
	DAGPBM_AUTOWRAP = 0x02,
	DAGPBM_FLUSH    = 0x04,
	DAGPBM_BYTESWAP = 0x08,
	DAGPBM_SAFETY   = 0x10,
	DAGPBM_WIDEBUS  = 0x20,
	DAGPBM_SYNCL2R  = 0x40,
	DAGPBM_REQPEND  = 0x80,
	DAGPBM_DEAD     = 0x100
	
} pbmctrl_t;

typedef struct daggpp
{
	volatile uint32_t control;   /* 0x00 */
	volatile uint32_t reserved;  /* 0x04 */
	volatile uint32_t padword;   /* 0x08 */
	volatile uint32_t snaplen;   /* 0x0c */
	
} daggpp_t;

typedef struct duck_irq_time
{
    uint32_t    nsec;
    uint32_t    sec; 
} duck_irq_time_t;

/* Structure which keeps detailed info about actual memory 
allocation */
#define DAG_MAX_NODES 16
typedef struct dag_meminfo
{
    uint32_t page_size;
    uint32_t nid_pages[DAG_MAX_NODES];
} dag_meminfo_t;

/*
 * Dag iocontrols
 */

#if defined(__SVR4) && defined(__sun)

#define DAGIOCRESET     0
#define DAGIOCMONITOR   1
#define DAGIOCINFO      2
#define DAGIOCLOCK      3
#define DAGIOCDUCK	4
#define DAGIOCPHYADDR	5
#define DAGIOCIRQTIME   6
#define DAGIOCDEVNID	7
#define DAGIOCMEMINFO	8
#define DAGIOCSETMEM	9


#else

#define DAG_IOC_MAGIC  'd'
#define DAGIOCRESET    _IOW(DAG_IOC_MAGIC,  0, uint32_t)
#define DAGIOCMONITOR  _IOWR(DAG_IOC_MAGIC, 1, monparams_t)
#define DAGIOCINFO     _IOR(DAG_IOC_MAGIC,  2, daginf_t)
#define DAGIOCLOCK     _IOWR(DAG_IOC_MAGIC, 3, int)
#define DAGIOCDUCK     _IOWR(DAG_IOC_MAGIC, 4, duckinf_t)
#define DAGIOCPHYADDR  _IOR(DAG_IOC_MAGIC, 5, uint64_t)
#define DAGIOCIRQTIME  _IOR(DAG_IOC_MAGIC, 6, duck_irq_time_t)
#define DAGIOCDEVNID  _IOR(DAG_IOC_MAGIC, 7, uint32_t)
#define DAGIOCMEMINFO  _IOR(DAG_IOC_MAGIC, 8, dag_meminfo_t)
#define DAGIOCSETMEM  _IOWR(DAG_IOC_MAGIC, 9, user_mem_t)

#endif /* Solaris */


#if (defined(__APPLE__) && defined(__ppc__))
enum
{
	kDAGUserClientOpen,
	kDAGUserClientClose,
	kDAGNumberOfMethods
};

enum
{
	kDAGUserClientMmapIOM,
	kDAGUserClientMmapBuffer
};
#endif /* Mac OS X */

#define DAG_IOC_MAXNR 9

//The maximum streams of 64 is requred by HSBM 
//TODO - softdag and future versions of HSBM can support up to 256
#define DAG_STREAM_MAX 64

#define DAG_MIN_LATENCY	64

typedef enum reset_magic
{
	DAGRESET_FULL    = 0,
	DAGRESET_REBOOT  = 1,
/*  DAGRESET_INIT    = 2,  XXX unknown */
	DAGRESET_DUCK    = 3,
	DAGRESET_GEORGE  = 4,
	DAGRESET_DAVE    = 5

} reset_magic_t;


#define DAGN(X)      (((X)&0xf000)>>12)
#define DAG2(X)      (((X)&0xf000)==0x2000)
#define DAG3(X)      (((X)&0xf000)==0x3000)
#define DAG32(X)     (((X)&0xff00)==0x3200)
#define DAG35(X)     (((X)&0xff00)==0x3500)
#define DAG36(X)     (((X)&0xff00)==0x3600)
#define DAG37(X)     (((X)&0xff00)==0x3700)
#define DAG38(X)     (((X)&0xff00)==0x3800)
#define DAG4(X)      (((X)&0xf000)==0x4000)
#define DAG6(X)      (((X)&0xf000)==0x6000)
#define DAG8(X)      (((X)&0xf000)==0x8000)
#define ETHERDAG(X)  (((X)&0x000f)==0x000e)


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* DAGNEW_H */
