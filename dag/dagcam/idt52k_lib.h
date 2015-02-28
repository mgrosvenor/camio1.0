/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: idt52k_lib.h,v 1.2.4.1 2009/07/21 02:35:24 karthik Exp $
 */

#ifndef DAGIPF_H
#define DAGIPF_H

/* Endace headers. */
#include "dagutil.h"

#if defined (_WIN32)
#include "wintypedefs.h"
#include "filtertypedefs.h"
#endif /* _WIN32 */

typedef void* FilterStatePtr; /**< Opaque pointer for filter state. */

#define SC256_CSR0 (0x54/4)
#define SC256_CSR1 (0x58/4)
#define SC256_CSR2 (0x5c/4)
#define SC256_VAL_ARRAY 0x00
#define SC256_RES_ARRAY (0x48/4)
#define SC256_VAL_ARRAY_DEPTH (0x12/4)
#define COLOUR_MEMORY_ADDRESS 0x08
#define COLOUR_MEMORY_VALUE 0x0C

typedef enum 
{
     kValueInvalid  = -2,
     kValueTimedout = -1,
     kValueNoError = 0
}cam_err_t;

#ifndef ESUCCESS
#define ESUCCESS   0
#endif

typedef struct 
{
	uint32_t data0; /**< lower order 64-bits */
	uint32_t data1; /**< higher order 8-bits */
	uint8_t data2;
}bit72_word_t;


#if 0
typedef struct bit72_word_t
{
	uint32_t data0; /**< lower order 64-bits */
	uint32_t data1; /**< higher order 8-bits */
	uint8_t data2;
	uint8_t valid;
} bit72_word_t;
#endif
//#define SEGMENT_SIZE 4096
typedef struct FilterStateHeader
{
	uint32_t magic_number;
	uint32_t interface_number;
	uint32_t rule_set;
	volatile uint32_t* copro_register_base;     /* Register space for the coprocessor.*/
	volatile uint8_t* ipf_register_base;
}FilterStateHeader,*FilterStateHeaderPtr;
/*F/W INTERFACE*/
void write_sc256_register(FilterStateHeaderPtr header,uint8_t addr, uint32_t reg_val);

uint32_t read_sc256_register(FilterStateHeaderPtr header,uint8_t addr);

void write_256cam_value(FilterStateHeaderPtr header, uint32_t addr, bit72_word_t value, uint8_t access_region);

bit72_word_t read_256cam_value(FilterStateHeaderPtr header, uint32_t addr, uint8_t access_region);

int
idt52k_cam_lookup_entry(FilterStateHeaderPtr header,uint32_t segment,uint32_t data[18],uint32_t lookup_size,uint32_t gmask,bool *hit_p,uint32_t *index_p,uint32_t *ad_p);

int 
idt52k_write_cam_entry(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t addr, const uint8_t data[72],const uint8_t mask[72]);
int
idt52k_write_sram_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t address,uint32_t value);
int 
idt52k_write_gmr_entry(FilterStateHeaderPtr header,uint32_t address,uint8_t mask[72],uint32_t width);
int
idt52k_read_sram_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t address,uint32_t *value);

int 
idt52k_read_cam_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t addr,uint8_t data[72], uint8_t mask[72]);

void  write_sc256_val_array(FilterStateHeaderPtr header,bit72_word_t value);

bit72_word_t read_sc256_val_array(FilterStateHeaderPtr header);


int
tcam_write_cam_144_entry(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask);

int tcam_read_cam_144_entry(FilterStateHeaderPtr header,uint32_t db_num,uint32_t index,uint8_t *data, uint8_t *mask);

int
tcam_write_cam_576_entry(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask);


int 
tcam_lookup_144_entry(FilterStateHeaderPtr header,uint32_t db, uint8_t value[72], uint32_t gmr, bool *hit, uint32_t *index, uint32_t *tag);

int 
tcam_lookup_576_entry(FilterStateHeaderPtr header,uint32_t db, uint8_t value[72], uint32_t gmr, bool *hit, uint32_t *index, uint32_t *tag);

int idt52k_tcam_initialise(FilterStateHeaderPtr header,bool mode);

int idt52k_enable_search(FilterStateHeaderPtr header);

int idt52k_inhibit_search(FilterStateHeaderPtr header);

int idt52k_reset_tcam(FilterStateHeaderPtr header);

int idt52k_get_tcam_mode(FilterStateHeaderPtr header);

int
tcam_write_cam_144_entry_ex(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask,uint32_t ad_value);

int
tcam_write_cam_576_entry_ex(FilterStateHeaderPtr header,uint32_t db_num ,uint32_t index,uint8_t *data,uint8_t *mask,uint32_t ad_value);

int 
tcam_read_cam_144_entry_ex(FilterStateHeaderPtr header,uint32_t db_num,uint32_t index,uint8_t *data, uint8_t *mask,uint32_t* ad_value);

int 
tcam_read_cam_576_entry_ex(FilterStateHeaderPtr header,uint32_t db_num,uint32_t index,uint8_t *data, uint8_t *mask,uint32_t *ad_value);


/*This structure will be filled by init function of the tool.Opaque decleration.more members will be added.*/
#if 0
typedef struct FilterStateHeader
{
	uint32_t magic_number;
	volatile uint32_t* copro_register_base;     /* Register space for the coprocessor.*/
} FilterStateHeader, *FilterStateHeaderPtr;
#endif

#define CAM256_ENTRIES_72       65536
#define CAM256_ENTRIES_144      (CAM256_ENTRIES_72/2)
#define CAM256_ENTRIES_288      (CAM256_ENTRIES_72/4)
#define CAM256_ENTRIES_576      (CAM256_ENTRIES_72/8)

/*Regions inside the TCAM.*/

#define ACCESS_DATA_REGION       0x00
#define ACCESS_MASK_REGION	 	 0x01
#define ACCESS_SRAM_REGION	 	 0x02
#define ACCESS_REGISTER_REGION   0x03

/*Instructions to the TCAM.*/
#define INSTR_READ                 0x00
#define INSTR_WRITE    		       0x01	
#define INSTR_LOOKUP		       0x02	
#define INSTR_LEARN		       	   0x03 	
#define INSTR_SRAM_NOWAIT_READ	   0x04	 
#define INSTR_DUAL_WRITE	       0x05	
#define INSTR_REISSUE		       0x06	
#define INSTR_RESERVED		       0x07 	

/* number of retries when checking busy bit in CAM read/write */
#define TIMEOUT 10               /* FIXME - what value is needed here??? */
#define SEGMENT_SIZE  4*1024
/*MASK FOR lookup allocation register. 64 bit mask for determining the width of the segment and related shift factors.*/
#define LAR_MASK_72  0x0000ffff
#define LAR_MASK_144 0xffff0000
#define LAR_MASK_288 0x0000ffff
#define LAR_MASK_576 0xffff0000

#define LAR_72_SHIFT_FACTOR  0
#define LAR_144_SHIFT_FACTOR 16
#define LAR_288_SHIFT_FACTOR 0
#define LAR_576_SHIFT_FACTOR 16

#define SSR_0_MASK_0 0x0000ffff
#define SSR_0_MASK_1 0xffff0000



enum {
	SCR	    = 0x00,
	IDR	    = 0x01,
	SSR0	= 0x02,
	nor3	= 0x03, // Reserved
	SSR1	= 0x04,
	nor5	= 0x05, // Reserved
	LAR	    = 0x06,
	nor7	= 0x07, // Reserved
	RSLT0	= 0x08,
	RSLT1	= 0x09,
	RSLT2	= 0x0a,
	RSLT3	= 0x0b,
	RSLT4	= 0x0c,
	RSLT5	= 0x0d,
	RSLT6	= 0x0e,
	RSLT7	= 0x0f,
	CMPR00	= 0x10,
	CMPR01	= 0x11,
	CMPR02	= 0x12,
	CMPR03	= 0x13,
	CMPR04	= 0x14,
	CMPR05	= 0x15,
	CMPR06	= 0x16,
	CMPR07	= 0x17,
	CMPR08	= 0x18,
	CMPR09	= 0x19,
	CMPR10	= 0x1a,
	CMPR11	= 0x1b,
	CMPR12	= 0x1c,
	CMPR13	= 0x1d,
	CMPR14	= 0x1e,
	CMPR15	= 0x1f,
	GMR00	= 0x20,
	GMR01	= 0x21,
	GMR02	= 0x22,
	GMR03	= 0x23,
	GMR04	= 0x24,
	GMR05	= 0x25,
	GMR06	= 0x26,
	GMR07	= 0x27,
	GMR08	= 0x28,
	GMR09	= 0x29,
	GMR10	= 0x2a,
	GMR11	= 0x2b,
	GMR12	= 0x2c,
	GMR13	= 0x2d,
	GMR14	= 0x2e,
	GMR15	= 0x2f,
	GMR16	= 0x30,
	GMR17	= 0x31,
	GMR18	= 0x32,
	GMR19	= 0x33,
	GMR20	= 0x34,
	GMR21	= 0x35,
	GMR22	= 0x36,
	GMR23	= 0x37,
	GMR24	= 0x38,
	GMR25	= 0x39,
	GMR26	= 0x3a,
	GMR27	= 0x3b,
	GMR28	= 0x3c,
	GMR29	= 0x3d,
	GMR30	= 0x3e,
	GMR31	= 0x3f,
	MAX_REGN
};



#define SYSTEM_CONFIGURATION_REGISTER  0x00
#define IDENTIFICATION_REGISTER        0x01
#define SEGMENT_SELECT_REGISTER_0      0x02
#define SEGMENT_SELECT_REGISTER_1      0x04 
#define LOOKUP_ALLOCATION_REGISTER     0x06
#if 0
#define RSLT0	= 0x08
#define RSLT1	= 0x09
#define	RSLT2	= 0x0a
#define	RSLT3	= 0x0b
#define	RSLT4	= 0x0c
#define	RSLT5	= 0x0d
#define	RSLT6	= 0x0e
#define	RSLT7	= 0x0f
#define	CMPR00	= 0x10
#define	CMPR01	= 0x11
#define	CMPR02	= 0x12
#define	CMPR03	= 0x13
#define	CMPR04	= 0x14
#define	CMPR05	= 0x15
#define	CMPR06	= 0x16
#define	CMPR07	= 0x17
#define	CMPR08	= 0x18
#define	CMPR09	= 0x19
#define	CMPR10	= 0x1a
#define	CMPR11	= 0x1b
#define	CMPR12	= 0x1c
#define	CMPR13	= 0x1d
#define	CMPR14	= 0x1e
#define	CMPR15	= 0x1f
#define	GMR00	= 0x20
#define	GMR01	= 0x21
#define	GMR02	= 0x22
#define	GMR03	= 0x23
#define	GMR04	= 0x24
#define	GMR05	= 0x25
#define	GMR06	= 0x26
#define	GMR07	= 0x27
#define	GMR08	= 0x28
#define	GMR09	= 0x29
#define	GMR10	= 0x2a
#define	GMR11	= 0x2b
#define	GMR12	= 0x2c
#define	GMR13	= 0x2d
#define	GMR14	= 0x2e
#define GMR15	= 0x2f
#define GMR16	= 0x30
#define	GMR17	= 0x31
#define	GMR18	= 0x32
#define	GMR19	= 0x33
#define	GMR20	= 0x34
#define GMR21	= 0x35
#define GMR22	= 0x36
#define	GMR23	= 0x37
#define	GMR24	= 0x38
#define	GMR25	= 0x39
#define	GMR26	= 0x3a
#define	GMR27	= 0x3b
#define	GMR28	= 0x3c
#define	GMR29	= 0x3d
#define	GMR30	= 0x3e
#define	GMR31	= 0x3f
#endif
#endif
