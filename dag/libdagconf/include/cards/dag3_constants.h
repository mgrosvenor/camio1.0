/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef DAG3_CONSTANTS_H
#define DAG3_CONSTANTS_H

/* C Standard Library headers. */
#if defined (__linux__)
#include <inttypes.h>
#endif



enum
{
	INT_ENABLES = 0x8c,
	DAG_CONFIG = 0x88,
	FROFF_DEVICEID = 0x00,
	FROFF_RXCONFIG = 0x04,
	FROFF_CS = 0x08,
	FROFF_ATMPOS = 0x0C,
	FROFF_COUNT1 = 0x10,
	FROFF_COUNT2 = 0x14,
	FROFF_TEST = 0x18,
	FROFF_BIP1 = 0x20,
	FROFF_BIP2 = 0x24,
	FROFF_BIP3 = 0x28,
	FROFF_REIL = 0x2C,
	FROFF_REIP = 0x30
};


/**
 * DS3182 Constants Maxim
 * DAG 3.7D 
 */
enum
{
    D3182_GLOBAL_CONTROL_1          = 0x002,
    D3182_GLOBAL_CONTROL_2          = 0x004,
    D3182_GLOBAL_GPIO_CONTROL       = 0x00A,
    
    D3182_PORT_CONTROL_1_A          = 0x040,
    D3182_PORT_CONTROL_2_A          = 0x042,
    D3182_PORT_CONTROL_3_A          = 0x044,
    D3182_PORT_CONTROL_4_A          = 0x046,
    D3182_PORT_STATUS_A             = 0x052,
    D3182_LINE_RECEIVE_STATUS_A     = 0x094,

    D3182_PORT_CONTROL_1_B          = 0x240,
    D3182_PORT_CONTROL_2_B          = 0x242,
    D3182_PORT_CONTROL_3_B          = 0x244,
    D3182_PORT_CONTROL_4_B          = 0x246,
    D3182_PORT_STATUS_B             = 0x252,
    D3182_LINE_RECEIVE_STATUS_B     = 0x294,

    D3182_CP_TRANSMIT_CONTROL_A     = 0x1A0,
    D3182_CP_RECEIVE_CONTROL_A      = 0x1C0,
    D3182_CP_TRANSMIT_CONTROL_B     = 0x3A0,
    D3182_CP_RECEIVE_CONTROL_B      = 0x3C0,

    D3182_T3_RECEIVE_STATUS_A       = 0x124,
    D3182_T3_RECEIVE_STATUS_B       = 0x324,

    D3182_FIFO_TRANSMIT_CONTROL_A   = 0x180,
    D3182_FIFO_TRANSMIT_PORT_ADDR_A = 0x184,
    D3182_FIFO_RECEIVE_CONTROL_A    = 0x190,
    D3182_FIFO_RECEIVE_PORT_ADDR_A  = 0x194,

    D3182_FIFO_TRANSMIT_CONTROL_B   = 0x380,
    D3182_FIFO_TRANSMIT_PORT_ADDR_B = 0x384,
    D3182_FIFO_RECEIVE_CONTROL_B    = 0x390,
    D3182_FIFO_RECEIVE_PORT_ADDR_B  = 0x394,

    D3182_GLOBAL_STATUS                = 0x014,
    D3182_SI_TRANSMIT_CONTROL          = 0x030,

    D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_A = 0x14A,
    D3182_FRACT_RECEIVE_SECTION_A_SIZE_A = 0x14C,

    D3182_FRACT_RECEIVE_DATA_GROUP_SIZE_B = 0x34A,
    D3182_FRACT_RECEIVE_SECTION_A_SIZE_B = 0x34C,

    /* Cell Processor (ATM mode) transmit/receive constants for Ports A/B */
    D3182_CP_TRANSMIT_CELL_COUNT_1_A   = 0x1B4,
    D3182_CP_TRANSMIT_CELL_COUNT_2_A   = 0x1B6,
    D3182_CP_RECEIVE_CELL_COUNT_1_A    = 0x1D4,
    D3182_CP_RECEIVE_CELL_COUNT_2_A    = 0x1D6,

    D3182_CP_TRANSMIT_CELL_COUNT_1_B   = 0x3B4,
    D3182_CP_TRANSMIT_CELL_COUNT_2_B   = 0x3B6,
    D3182_CP_RECEIVE_CELL_COUNT_1_B    = 0x3D4,
    D3182_CP_RECEIVE_CELL_COUNT_2_B    = 0x3D6,

    D3182_CP_RECEIVE_FILTERED_CELL_COUNT_1_A = 0x1E4,
    D3182_CP_RECEIVE_FILTERED_CELL_COUNT_2_A = 0x1E6,
    D3182_CP_RECEIVE_FILTERED_CELL_COUNT_1_B = 0x3E4,
    D3182_CP_RECEIVE_FILTERED_CELL_COUNT_2_B = 0x3E6,

    D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_A = 0x1C4,
    D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_A = 0x1C6,
    D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_1_B = 0x3C4,
    D3182_CP_RECEIVE_HEADER_PATTERN_CONTROL_2_B = 0x3C6,
    D3182_CP_RECEIVE_HEADER_PATTERN_MASK_1_A = 0x1C8,
    D3182_CP_RECEIVE_HEADER_PATTERN_MASK_2_A = 0x1CA,
    D3182_CP_RECEIVE_HEADER_PATTERN_MASK_1_B = 0x3C8,
    D3182_CP_RECEIVE_HEADER_PATTERN_MASK_2_B = 0x3CA,
    D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_1_A = 0x1DC,
    D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_2_A = 0x1DE,
    D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_1_B = 0x3DC,
    D3182_CP_RECEIVE_HEADER_PATTERN_CELL_COUNT_2_B = 0x3DE,

    /* Packet Processor (HDLC mode) transmit/receive constants for Ports A/B */
    D3182_PP_TRANSMIT_PACKET_COUNT_1_A = 0x1B4,
    D3182_PP_TRANSMIT_PACKET_COUNT_2_A = 0x1B6,
    D3182_PP_RECEIVE_PACKET_COUNT_1_A  = 0x1D4,
    D3182_PP_RECEIVE_PACKET_COUNT_2_A  = 0x1D6,

    D3182_PP_TRANSMIT_BYTE_COUNT_1_A   = 0x1B8,
    D3182_PP_TRANSMIT_BYTE_COUNT_2_A   = 0x1BA,
    D3182_PP_RECEIVE_BYTE_COUNT_1_A    = 0x1E8,
    D3182_PP_RECEIVE_BYTE_COUNT_2_A    = 0x1EA,

    D3182_PP_TRANSMIT_PACKET_COUNT_1_B = 0x3B4,
    D3182_PP_TRANSMIT_PACKET_COUNT_2_B = 0x3B6,
    D3182_PP_RECEIVE_PACKET_COUNT_1_B  = 0x3D4,
    D3182_PP_RECEIVE_PACKET_COUNT_2_B  = 0x3D6,

    D3182_PP_TRANSMIT_BYTE_COUNT_1_B   = 0x3B8,
    D3182_PP_TRANSMIT_BYTE_COUNT_2_B   = 0x3BA,
    D3182_PP_RECEIVE_BYTE_COUNT_1_B    = 0x3E8,
    D3182_PP_RECEIVE_BYTE_COUNT_2_B    = 0x3EA

};
 
/* PM 5355 Registers. (DAG ??) */
enum
{
	PM5355_MASTER              = 0x00,
	PM5355_CONFIG              = 0x01,
	PM5355_MASTER_CTRL_MON     = 0x04,
	PM5355_POP                 = 0x06,
	PM5355_RSOP_STATUS         = 0x11,
	PM5355_RSOP_SBIP_LSB       = 0x12,
	PM5355_RSOP_LBIP_LSB       = 0x1A,
	PM5355_RSOP_LFEBE_LSB      = 0x1D,
	PM5355_RPOP_C2             = 0x37,
	PM5355_RPOP_PBIP_LSB       = 0x38,
	PM5355_RPOP_PFEBE_LSB      = 0x3A,
	PM5355_TPOP_PATH_SIG_LABEL = 0x48,
	PM5355_RACP_CTRL           = 0x50,
	PM5355_RACP_RXCELL_LSB     = 0x59,
	PM5355_RACP_GFC_CTRL       = 0x5c,
	PM5355_TACP_CTRL_STA       = 0x60,
	PM5355_TACP_FIFO_CTRL      = 0x63
};


/* National DP83865 Registers. (DAG 3.6GE) */
enum
{
	DP83865_BMCR    = 0x00, /* Basic Mode Control Register. */
	DP83865_BMSR    = 0x01, /* Basic Mode Status Register. */
	DP83865_ANAR    = 0x04, /* Auto-Negotiation Advertisement Register. */
	DP83865_1KTCR   = 0x09, /* 1000BASE-T Control Register. */
	DP83865_1KTSR   = 0x0a, /* 1000BASE-T Status Register. */
	DP83865_LINK_AN = 0x11 /* Link and Auto-Negotiation Register. */
};


/* Marvell 88e1111 Registers. (DAG 3.7GE) */
enum
{
	MV88E1111_BMCR    = 0x00, /* Basic Mode Control Register. */
	MV88E1111_BMSR    = 0x01, /* Basic Mode Status Register. */
    MV88E1111_PHYID1  = 0x02, /* PHY Identifier */
    MV88E1111_PHYID2  = 0x03, /* PHY Identifier */  
	MV88E1111_ANAR    = 0x04, /* Auto-Negotiation Advertisement Register. */
	MV88E1111_1KTCR   = 0x09, /* 1000BASE-T Control Register. */
	MV88E1111_1KTSR   = 0x0a, /* 1000BASE-T Status Register. */
	MV88E1111_LINK_AN = 0x11, /* PHY Specific Status Register. */
	MV88E1111_ERROR   = 0x15, /* Error counter */
	MV88E1111_EXAR    = 0x16, /* Extended Address Register */
    MV88E1111_EPSSR   = 0x1b  /* PHY Extended Specific Register */
        
};


/* EXAR 83L38 Registers. (DAG 3.7T) */
enum
{
	EXAR8338_CHAN_EQC   = 0x00,
	EXAR8338_CHAN_TERM  = 0x01,
	EXAR8338_CHAN_LOOP  = 0x02,
	EXAR8338_CHAN_CODE  = 0x03,
	
	EXAR8338_CHAN_STAT  = 0x05,
	
	EXAR8338_CHAN_CLOS  = 0x07,
	
	EXAR8338_CTRL       = 0x80,
	EXAR8338_CLK        = 0x81,
	
	EXAR8338_GUAGE      = 0x83
};


enum
{
	SONIC_E1T1_INTERFACE    = 0x0000,   /* 16 32-bit entries */
	SONIC_E1T1_CONFIG       = 0x0040,   /* Interface configuration */
	SONIC_E1T1_COUNTER_CTRL = 0x0050,
	SONIC_E1T1_COUNTER_DATA = 0x0054
};

enum
{
    HDLC_DROP_COUNT_REG = 0x18,
    ATM_DROP_COUNT_REG = 0x1c,
	ATM_LCD_STATUS_REGISTER = 0x20
};


#define DAG3_REG_BASE		0x400
#define DAG3_REG_KENTROX	0x404
#define DAG3_REG_FRACT		0x408

#define EXAR8338_B8ZS 0x20        /* b8zs or ami configuration bit (0=b8zs) */


/* SONIC E1T1 CONFIG register values */
#define STE_CONFIG_CLEAR_STATS          0x80000000  /* Clear status reg 0 stats */
#define STE_CONFIG_CLEAR_EVER           0x40000000  /* Clear status reg ever seen stats */
#define STE_CONFIG_CLEAR_TX_DUMP        0x10000000  /* Clear tx path and stats */
#define STE_CONFIG_CLEAR_MOST           0xc0000000
#define STE_CONFIG_CLEAR_FATAL          0xf0000000
#define STE_CONFIG_RX_STATS_PRESENT     0x00080000
#define STE_CONFIG_RX_COUNTERS_PRESENT  0x00040000
#define STE_CONFIG_TX_STATS_PRESENT     0x00020000
#define STE_CONFIG_RX_BUSY              0x00008000  /* RX configuration port is busy */
#define STE_CONFIG_TX_BUSY              0x00004000  /* TX configuration port is busy */
#define STE_CONFIG_BUSY                 (STE_CONFIG_RX_BUSY | STE_CONFIG_TX_BUSY)
#define STE_CONFIG_RESET                0x00002000
#define STE_CONFIG_IS_T1                0x00001000  /* 1=T1, 0=E1 */
#define STE_CONFIG_WRITE                0x00000200  /* Write config */
#define STE_CONFIG_RESYNC               0x00000100  /* Hold channel in resync until configured */

#define STE_CONFIG_INTERFACE_MASK   0x000000F0  /* 7:4 - interface number to apply configuration to */
#define STE_CONFIG_INTERFACE_OFFSET 4

#define STE_CONFIG_TYPE_MASK        0x00000007
#define STE_CONFIG_TYPE_T1_ESF      0x00000001
#define STE_CONFIG_TYPE_T1_D4_SF    0x00000002
#define STE_CONFIG_TYPE_T1_D4       0x00000003  /* Unframed? */
#define STE_CONFIG_TYPE_E1          0x00000004
#define STE_CONFIG_TYPE_E1_CRC      0x00000005
#define STE_CONFIG_TYPE_UNFRAMED    0x00000006
#define STE_CONFIG_TYPE_OFF         0x00000000

#define STE_CNTR_CLEAR 0x80000000 /* clear. mutually exclusive with flip */
#define STE_CNTR_CBSY  0x10000000 /* clear busy. do not flip or clear */
#define STE_CNTR_FLIP  0x00080000 /* flip. mutually exclusive with clear */
#define STE_CNTR_FBSY  0x00010000 /* flip busy. do not flip or clear */
#define STE_CNTR_IFMSK 0x000000F0 /* Interface number */
#define STE_CNTR_IFSHAMT 4        /* Amount to shift IF number by */
#define STE_CNTR_AIS  0x00000004  /* Select AIS Counter */
#define STE_CNTR_CRC  0x00000002  /* Select CRC Counter */
#define STE_CNTR_FRM  0x00000001  /* Select FRAME Counter */


#define STE_STAT_TYPE_MASK          0x7
#define STE_STAT_TYPE_OFFSET        28
#define STE_STAT_SEEN_TYPE_MASK     0x7
#define STE_STAT_SEEN_TYPE_OFFSET   25

#define STE_STAT_EVER_RX_ONE        0x01000000
#define STE_STAT_EVER_RX_ZERO       0x00800000
#define STE_STAT_EVER_OVERRUN       0x00400000
#define STE_STAT_IS_UP              0x00200000
#define STE_STAT_SEEN_UP            0x00100000
#define STE_STAT_SEEN_DOWN          0x00080000
#define STE_STAT_SEEN_FRAMER_ERROR  0x00040000
#define STE_STAT_SEEN_CRC_ERROR     0x00020000
#define STE_STAT_SEEN_AIS_ERROR     0x00010000
#define STE_STAT_EVER_TX_ONE        0x00000100
#define STE_STAT_EVER_TX_ZERO       0x00000080
#define STE_STAT_EVER_UNDERRUN      0x00000040

#define D37T_MAX_LINES      	16
#ifdef ENDACE_LAB
#define D37T4_MAX_LINES     	8
#else
#define D37T4_MAX_LINES         4 
#endif 
/* ERF Mux interface for 3.7T */
#define MUX_BASE    0x400       /* Not enumerated yet */
enum
{
	MUX_RX_HOST = 0x00,
	MUX_RX_PHY  = 0x04,
	MUX_RX_IOP  = 0x08
};

#define MUX_CONNECT_TX_HOST 0x00
#define MUX_CONNECT_TX_PHY  0x01
#define MUX_CONNECT_TX_IOP  0x02


/* E.g. configure line 0 for E1:  *(sonic_e1t1_base + 0x40) = 0x0204
 *      configure line 4 for E1:  *(sonic_e1t1_base + 0x40) = 0x0244
 */

#endif /* DAG3_CONSTANTS_H */
