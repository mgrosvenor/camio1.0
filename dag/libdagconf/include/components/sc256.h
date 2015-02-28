/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: sc256.h,v 1.1 2006/03/28 22:46:16 trisha Exp $
 */

#ifndef SC256_H
#define SC256_H

#if 0
#define SC256_CSR0 (0x54/4)
#define SC256_CSR1 (0x58/4)
#define SC256_CSR2 (0x5c/4)
#define SC256_VAL_ARRAY 0x00
#define SC256_RES_ARRAY (0x48/4)
#define SC256_VAL_ARRAY_DEPTH (0x12/4)
#endif

#define SC256_CSR0 (0x54)
#define SC256_CSR1 (0x58)
#define SC256_CSR2 (0x5c)
#define SC256_VAL_ARRAY 0x00
#define SC256_RES_ARRAY (0x48)
#define SC256_VAL_ARRAY_DEPTH (0x12)

enum {
	SCR	= 0x00,
	IDR	= 0x01,
	SSR0	= 0x02,
	nor3	= 0x03, // Reserved
	SSR1	= 0x04,
	nor5	= 0x05, // Reserved
	LAR	= 0x06,
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
	MAX_REGN // Must be last entry in table
};

ComponentPtr sc256_get_new_component(DagCardPtr card);

#endif

