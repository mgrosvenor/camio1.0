/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef DAG6_CONSTANTS_H
#define DAG6_CONSTANTS_H

typedef enum s19205cbi {
	S19205_RX_TO_TX_SONET_LOOPBACK  = 0x000F,
	S19205_RX_TO_TX_SYS_LOOPBACK    = 0x0022, /* Also TX_PYLD_SCR_INH */
	S19205_TX_MAP                   = 0x0023,

	S19205_TX_C2INS                 = 0x02C1,
	S19205_TX_SYS_PRTY_ERR_E        = 0x0340,
	S19205_TX_SIZE_MODE             = 0x0344, /* Also TX_SYS_PRTY_CTL_INH */
	S19205_TX_FIFOERR_CNT           = 0x034C,
	S19205_TX_XMIT_HOLDOFF          = 0x034E,

	S19205_TX_POS_PMIN		        = 0x0381,
	S19205_TX_POS_PMAX1		        = 0x0382,
	S19205_TX_POS_PMAX2		        = 0x0383,
	
	S19205_TX_POS_ADRCTL_INS        = 0x038A,

	S19205_XG_CTR_CLR		        = 0x03C6, /* Also XG_CTR_SNAPSHOT */
    S19205_XG_MAC_ADDR              = 0x03C0,

	S19205_TX_ETH_INH1		        = 0x0401,
	S19205_TX_ETH_PMAX1		        = 0x0402,
	S19205_TX_ETH_PMAX2		        = 0x0403,

	S19205_LATCH_CNT                = 0x0800,
	S19205_TX_TO_RX_SONET_LOOPBACK  = 0x0825,

	S19205_RX_MAP                   = 0x0848, /* Also TX_TO_RX_SYS_LOOPBACK & RX_PYLD_DSCR_INH*/

	S19205_RX_LOSEXT_LEVEL          = 0x0909,
	S19205_RX_LOS                   = 0x090D,
	S19205_RX_B1_ERRCNT1            = 0x0940,
	S19205_RX_B1_ERRCNT2            = 0x0941,
	S19205_RX_B2_ERRCNT1            = 0x0943,
	S19205_RX_B2_ERRCNT2            = 0x0944,
	S19205_RX_B2_ERRCNT3            = 0x0945,

	S19205_RX_PI_LOP                = 0x0C08,

	S19205_RX_C2EXP                 = 0x0D12,
	S19205_RX_C2MON                 = 0x0D15,
	S19205_RX_B3_ERRCNT1            = 0x0D16,
	S19205_RX_B3_ERRCNT2            = 0x0D17,

	S19205_RX_POS_PMIN              = 0x0E00,
	S19205_RX_POS_PMAX1             = 0x0E01,
	S19205_RX_POS_PMAX2             = 0x0E02,
	S19205_RX_POS_PMIN_ENB          = 0x0E08, /* Also RX_POS_PMAX_ENB & RX_POS_ADRCTL_DROP_INH & RX_POS_FCS_INH*/
	S19205_RX_POS_FCS_ERRCNT1       = 0x0E11,
	S19205_RX_POS_FCS_ERRCNT2       = 0x0E12,
	S19205_RX_POS_FCS_ERRCNT3       = 0x0E13,
	S19205_RX_POS_PKT_CNT1          = 0x0E14,
	S19205_RX_POS_PKT_CNT2          = 0x0E15,
	S19205_RX_POS_PKT_CNT3          = 0x0E16,
	S19205_RX_POS_PKT_CNT4          = 0x0E17,
	
	S19205_RX_FIFOERR_CNT           = 0x1008,

	S19205_RX_ETH_INH1		        = 0x1204,
	S19205_RX_ETH_INH2		        = 0x1205,
	S19205_RX_ETH_PMAX1		        = 0x1206,
	S19205_RX_ETH_PMAX2		        = 0x1207,

	S19205_RX_ETH_PHY		        = 0x1214,

	S19205_RX_ETH_GOODCNT1		    = 0x1258,
    S19205_RX_ETH_OCTETS_OK         = 0x1260,
	S19205_RX_ETH_FCSCNT1		    = 0x12B0,
	S19205_RX_ETH_OVERRUNCNT1		= 0x12B8,
	S19205_RX_ETH_DROPCNT1		    = 0x12C0,

	S19205_RX_ETH_BADCNT1		    = 0x12E8,
    S19205_RX_ETH_OCTETS_BAD        = 0x12F0,

	S19205_DEV_VER                  = 0x1FFD,
	S19205_DEV_ID                   = 0x1FFF
} s19205cbi_t;

#endif

