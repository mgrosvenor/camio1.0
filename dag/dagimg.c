/*
 * Copyright (c) 2003-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: dagimg.c,v 1.84 2009/06/09 22:22:00 wilson.zhu Exp $
 */

/* File header. */
#include "dagimg.h"

/* Endace headers. */
#include "dagname.h"
#include "dagpci.h"
#include "dagutil.h"

/*
 * Backwards compatability table mapping PCI device IDs + xilinx to
 * image index when this is not available from firmware directly.
 */
static dag_img_t dag_img_array[] = {
	/* 0 DAG 3.5S pci */
	{
		PCI_DEVICE_ID_DAG3_50,
		0,
		1
	},
	/* 1 DAG 3.5S pp */
	{
		PCI_DEVICE_ID_DAG3_50,
		1,
		2
	},

	/* 2 DAG 3.6D pci */
	{
		PCI_DEVICE_ID_DAG3_60D,
		0,
		3
	},
	/* 3 DAG 3.6D pp */
	{
		PCI_DEVICE_ID_DAG3_60D,
		1,
		4
	},

	/* 4 DAG 3.6E pci */
	{
		PCI_DEVICE_ID_DAG3_60E,
		0,
		5
	},
	/* 5 DAG 3.6E pp */
	{
		PCI_DEVICE_ID_DAG3_60E,
		1,
		6
	},
	
	/* 6 DAG 3.6GE pci */
	{
		PCI_DEVICE_ID_DAG3_68E,
		0,
		5 /* same as 3.6E */
	},
	/* 7 DAG 3.6GE pp */
	{
		PCI_DEVICE_ID_DAG3_68E,
		1,
		7
	},
	
	/* 8 DAG 3.7T pci */
	{
		PCI_DEVICE_ID_DAG3_70T,
		0,
		8
	},
	
	/* 8 DAG 3.7T4 pci */
	{
		PCI_DEVICE_ID_DAG3_7T4,
		0,
		8
	},
	
	/* 9 DAG 3.8S pci */
	{
		PCI_DEVICE_ID_DAG3_80,
		0,
		9
	},
	/* 10 DAG 3.8S pp */
	{
		PCI_DEVICE_ID_DAG3_80,
		1,
		10
	},
	
	/* 11 DAG 4.2GE pci */
	{
		PCI_DEVICE_ID_DAG4_23E,
		0,
		11
	},
	
	/* 12 DAG 4.2S pci */
	{
		PCI_DEVICE_ID_DAG4_23,
		0,
		12
	},
	
	/* 13 DAG 4.3GE pci */
	{
		PCI_DEVICE_ID_DAG4_30E,
		0,
		13
	},
	
	/* 14 DAG 4.3S pci */
	{
		PCI_DEVICE_ID_DAG4_30,
		0,
		14
	},
	
	/* 15 DAG 6.1SE pci */
	{
		PCI_DEVICE_ID_DAG6_10,
		0,
		15
	},
	/* 16 DAG 6.1SE rx */
	{
		PCI_DEVICE_ID_DAG6_10,
		1,
		16
	},
	
	/* 17 DAG 6.2SE pci */
	{
		PCI_DEVICE_ID_DAG6_20,
		0,
		17
	},
	/* 18 DAG 6.2SE pp */
	{
		PCI_DEVICE_ID_DAG6_20,
		1,
		18
	},
	
	/* 19 DAG 3.7G pci */
	{
		PCI_DEVICE_ID_DAG3_78E,
		0,
		19
	},
	
	/* 20 DAG 3.8S copro */
	{
		PCI_DEVICE_ID_DAG3_80,
		2,
		20
	},
	
	/* 21 DAG 4.3GE copro */
	{
		PCI_DEVICE_ID_DAG4_30E,
		1,
		21
	},
	
	/* 22 DAG 4.3S copro */
	{
		PCI_DEVICE_ID_DAG4_30,
		1,
		21
	},
	
	/* 23 DAG 7.1S pci */
	{
		PCI_DEVICE_ID_DAG7_10,
		0,
		22
	},
	
	/* 24 DAG 7.1S pp1 */
	{
		PCI_DEVICE_ID_DAG7_10,
		1,
		23
	},
	
	/* 25 DAG 7.1S pp2 */
	{
		PCI_DEVICE_ID_DAG7_10,
		2,
		23
	},

	/* 26 DAG 3.7D PCI */
	{
		PCI_DEVICE_ID_DAG3_70D,
		0,
		24
	},

	/* 27 DAG 4.52e PCI */
	{
		PCI_DEVICE_ID_DAG4_52E,
		0,
		25
	},

	/* 28 DAG 4.54e PCI */
	{
		PCI_DEVICE_ID_DAG4_54E,
		0,
		26
	},

	/* 29 DAG 4.52f PCI */
	{
		PCI_DEVICE_ID_DAG4_52F,
		0,
		26
	},

	/* 30 DAG 8.2 PCI */
	{
		PCI_DEVICE_ID_DAG8_20E,
		0,
		27
	},

	/* 31 5.2x */
	{
		PCI_DEVICE_ID_DAG5_20E,
		0,
		28
	},

	/* 32 DAG 4.52z PCI */
	{
		PCI_DEVICE_ID_DAG4_52Z,
		0,
		29
	},
	
	/* 32 DAG 8.2zx PCI */
	{
		PCI_DEVICE_ID_DAG8_20Z,
		0,
		30
	},
	
	/* 33 DAG 4.52Cf PCI */
	{
		PCI_DEVICE_ID_DAG4_5CF,
		0,
		26
	},
	/* 34 DAG 5.0S PCI */
	{
		PCI_DEVICE_ID_DAG5_00S,
		0,
		31
	},

	/* 35 DAG 5.2SXA */
	{
		PCI_DEVICE_ID_DAG5_2SXA,
		0,
		32
	},
 
	/* 35 DAG 5.21SXA */
	{
		PCI_DEVICE_ID_DAG5_21SXA,
		0,
		46
	},
	/* 36 DAG 5.0Z */
	{
		PCI_DEVICE_ID_DAG5_0Z,
		0,
		33
	},
	/* 37 DAG 5.0DUP */
	{
		PCI_DEVICE_ID_DAG5_0DUP,
		0,
		34
	},
	/*  DAG 8.1S */
	{
		PCI_DEVICE_ID_DAG8_100,
		0,
		35
	},
	/* 39 DAG 4.52z8 PCI */
	{
		PCI_DEVICE_ID_DAG4_52Z8,
		0,
		36
	},
	/*  DAG 8.4i */
	{
		PCI_DEVICE_ID_DAG8_400,
		0,
		37
	},
 	/* 37 DAG 5.0sg2a PCI */
	{
		PCI_DEVICE_ID_DAG5_0SG2A,
		0,
		38
	},
        /* 39  DAG 5.4s12  PCI */
	{
		PCI_DEVICE_ID_DAG5_4S12,
		0,
		39
	},
	/* 40  DAG 5.4sg48  PCI */
	{
		PCI_DEVICE_ID_DAG5_4SG48,
		0,
		40
	},
	/* 41 DAG 5.0sg2a PCI */
	{
		PCI_DEVICE_ID_DAG5_0SG3A,
		0,
		41
	},
	/* 42 DAG 5.4ga PCI */
	{
		PCI_DEVICE_ID_DAG5_4GA,
		0,
		42
	},
        /* 43 DAG 5.4sa12 PCI */
	{
		PCI_DEVICE_ID_DAG5_4SA12,
		0,
		43
	},
	/* 44 DAG 5.4sga48  PCI */
	{
	        PCI_DEVICE_ID_DAG5_4SGA48,
		0,
		44
	},
         /*  DAG 8.1X  */
	{
		PCI_DEVICE_ID_DAG8_102,
		0,
		45
	},
	/*  DAG 7.5G2 */
	{
		PCI_DEVICE_ID_DAG_7_5G2,
		0,
		47
	},	
	/*  DAG 4.588 (4.5g2) */
	{
		PCI_DEVICE_ID_DAG4_588,
		0,
		48
	},	
	/*  DAG 7.5G4 */
	{
		PCI_DEVICE_ID_DAG_7_5G4,
		0,
		49
	},	
	/*  DAG 9.1Rx */
	{
		PCI_DEVICE_ID_DAG_9_1x2Rx,
		0,
		50
	},
	/*  DAG 9.1Tx */
	{
		PCI_DEVICE_ID_DAG_9_1x2Tx,
		0,
		53
	},
	/* DGA 8.5 DI*/
	{	
		PCI_DEVICE_ID_DAG8_500,
		0,
		51
	},
        /*  DAG 8.11S */
	{
		PCI_DEVICE_ID_DAG8_101,
		0,
		52
	},	
        /*  DAG 7.4S */
	{
		PCI_DEVICE_ID_DAG_7_400,
		0,
		54
	},	
         /*  DAG 7.4S48 */
	{
		PCI_DEVICE_ID_DAG_7_401,
		0,
		55
	},	
	/*  DAG 7.5BE */
	{
		PCI_DEVICE_ID_DAG_7_5BE,
		0,
		56
	},
	/*  DAG 7.5CE */
	{
		PCI_DEVICE_ID_DAG_7_5CE,
		0,
		57
	},
	/* end */
	{
		DAG_IMG_END,
		0,
		0
	}
};

/*
 * Table mapping image index numbers to A record prefixes and B records
 * for type checking.
 */
static img_id_t img_id_array[] = {

	/* DAG 3.5S pci */
	{
		1,
		"dag35pci",
		"2s200fg256",
		-1,
		0
	},

	/* DAG 3.5S pp */
	{
		2,
		"dag35atm",
		"2s200fg256",
		-1,
		0
	},

	/* DAG 3.6D pci */
	{
		3,
		"dag36dpci",
		"2s200fg256",
		-1,
		0
	},

	/* DAG 3.6D pp */
	{
		4,
		"dag36dpp",
		"2s200fg256",
		-1,
		0
	},

	/* DAG 3.6G/E pci */
	{
		5,
		"dag36gepci",
		"2s200fg256",
		-1,
		0
	},

	/* DAG 3.6E pp */
	{
		6,
		"dag36epp",
		"2s200fg256",
		-1,
		0
	},

	/* DAG 3.6GE pp */
	{
		7,
		"dag36gepp",
		"2s200fg256",
		-1,
		0
	},

	/* DAG 3.7T pci */
	{
		8,
		"dag37tpci",
		"3s1500fg456",
		-1,
		0
	},

	/* DAG 3.8S pci */
	{
		9,
		"edag38spci",
		"2v1000fg456",
		-1,
		0
	},

	/* DAG 3.8S pp */
	{
		10,
		"dag38spp",
		"2s300eft256",
		-1,
		0
	},

	/* DAG 4.2GE pci */
	{
		11,
		"dag423ge",
		"2v1000fg456",
		-1,
		0
	},

	/* DAG 4.2S pci */
	{
		12,
		"dag423pos",
		"2v1000fg456",
		-1,
		0
	},

	/* DAG 4.3GE pci */
	{
		13,
		"edag43epci",
		"2v1000ff896",
		-1,
		0
	},

	/* DAG 4.3S pci */
	{
		14,
		"edag43spcix",
		"2v1000ff896",
		-1,
		0
	},

	/* DAG 6.1SE pci */
	{
		15,
		"edag61sepcix",
		"2v1000fg456",
		-1,
		0
	},

	/* DAG 6.1SE rx */
	{
		16,
		"edag61serx",
		"2v1000fg456",
		-1,
		0
	},

	/* DAG 6.2SE pci */
	{
		17,
		"edag62sepcix",
		"2v1000fg456",
		-1,
		0
	},
	/* DAG 6.2SE pp */
	{
		18,
		"edag62sepp",
		"2v1000ff896",
		-1,
		0
	},

	/* DAG 3.7G pci */
	{
		19,
		"dag37gepci",
		"3s1500fg456",
		-1,
		0
	},

	/* SC128 Copro 16-bit */
	{
		20,
		"ec10gcp_aal5-16_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC128,
		0
	},

	/* SC128 Copro 16-bit */
	{
		20,
		"ec10gcp_ipf_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC128,
		0
	},

	/* SC256 Copro 16-bit */
	{
		20,
		"ec20gcp_aal5-16_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256,
		0
	},

	/* SC256 Copro 16-bit */
	{
		20,
		"ec20gcp_ipf_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256,
		0
	},

	/* SC256 Rev C Copro 16-bit */
	{
		20,
		"ec20gcp_aal5-16_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256_C,
		0
	},

	/* SC256 Rev C Copro 16-bit */
	{
		20,
		"ec20gcp_ipf_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256_C,
		0
	},

	/* SC128 Copro 32-bit */
	{
		21,
		"ec10gcp_aal5_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC128,
		0
	},

	/* SC128 Copro 32-bit */
	{
		21,
		"ec10gcp_ipf32_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC128,
		0
	},

	/* SC256 Copro 32-bit */
	{
		21,
		"ec20gcp_aal5_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256,
		0
	},

	/* SC256 Copro 32-bit */
	{
		21,
		"ec20gcp_ipf32_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256,
		0
	},

    {
		21,
		"ec20gcp_stats_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256,
		0
	},

    {
		21,
		"ec20gcp_rndcap_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256,
		0
	},

	/* SC256 Rev C Copro 32-bit */
	{
		21,
		"ec20gcp_aal5_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256_C,
		0
	},

    {
		21,
		"ec20gcp_rndcap_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256_C,
		0
	},

	/* SC256 Rev C Copro 32-bit */
	{
		21,
		"ec20gcp_ipf32_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256_C,
		0
	},

    {
		21,
		"ec20gcp_stats_",
		"2v2000ff896",
		COPRO_DEVICE_ID_SC256_C,
		0
	},

	/* DAG 7.1S pci */
	{
		22,
		"edag71spci",
		"2vp30ff1152",
		-1,
		0
	},
	
#if ENDACE_LAB
	/* DAG 7.1S pci UNENCRYPTED */
	{
		22,
		"dag71spci",
		"2vp30ff1152",
		-1,
		0
	},
#endif /* ENDACE_LAB */

	/* DAG 7.1S pp */
	{
		23,
		"dag71spp",
		"3s2000fg676",
		-1,
		0
	},

	/* DAG 3.7D pci */
	{
		24,
		"dag37dpci",
		"3s1500fg456",
		-1,
		0
	},

	/* DAG 4.52ge */
	{
		25,
		"edag45g2pci",
		"2vp30ff1152",
		-1,
		0
	},
    /* DUP images for DAG 4.5G2 */
	{
		25,
		"edag45duppci",
		"2vp30ff1152",
		-1,
		0
	},
	/* DAG 4.52ge (common G2 and G4 image) */
	{
		25,
		"edag45gepci",
		"2vp30ff1152",
		-1,
		0
	},

	/* DAG 4.54ge */
	{
		26,
		"edag45g4pci",
		"2vp30ff1152",
		-1,
		0
	},

	/* DAG 4.54ge (common G2 and G4 image) */
	{
		26,
		"edag45gepci",
		"2vp30ff1152",
		-1,
		0
	},
    /* DUP images for DAG 4.5G4 */
	{
		26,
		"edag45duppci",
		"2vp30ff1152",
		-1,
		0
	},
	/* DAG 8.2X */
	
	{
		27,
		"edag82xpci",
		"4vsx35ff668",
		-1,
		0
	},
	
	/* DAG 5.2X */
	{
		28,
		"edag52xpci",
		"4vlx40ff1148",
		-1,
		0
	},
	
	/* DAG 4.52z */
	{
		29,
		"edag45z2pci",
		"2vp30ff1152",
		-1,
		0
	},

	/* DAG 8.2z */
	{
		30,
		"edag82zxpci",
		"4vsx35ff668",
		-1,
		0
	},
	/* DAG 5.0s OC48 LX40 FPGA (2MB)image */
/* depreciated Images . lx40s are now 54 cards 
	{
		31,
		"edag50spci",
		"4vlx40ff1148",
		-1,
		0
	},
*/
	/* DAG 5.0sg2a PCI */
	{
		38,
		"eapp50s2apci_",
		"4vlx80ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a Copro */
	{
		38,
		"eapp50s2acp_",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
    /* DAG 5.0sg2a PCI */
	{
		38,
		"edag50s2a_apppci",
		"4vlx80ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a Copro */
	{
		38,
		"edag50s2a_appcp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.0sg2a PCI */
	{
		38,
		"edag50s2a_apppci",
		"4vlx80ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a Copro */
	{
		38,
		"edag50s2a_appcp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	
	/* DAG 5.0s OC48 LX80 FPGA (4Mb) image */
	{
		31,
		"edag50spci",
		"4vlx80ff1148",
		-1,
		0
	},
	
	/* DAG 5.0s DUP3 image */
/* Depreciated Images for 5.0s.
	{
		31,
		"edag50sdup",
		"4vlx40ff1148",
		-1,
		0
	},

	{
		31,
		"edag50dup",
		"4vlx80ff1148",
		-1,
		0
	},
	*/
	/* DAG 5.0s OC3/OC12 image */
/* Depreciated Images LX40 are now 54 cards	

	{
		31,
		"edag50serf",
		"4vlx40ff1148",
		-1,
		0
	},
*/	
	/* DAG 5.2SXA appliances */
/*	This was depriciated for the appliances has a new names 
	{
		32,
		"edag52sxacp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
*/	
	/* DAG 5.21SXA general*/
	{
		46,
		"edag52sxacp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
 
	/* DAG 5.2SXA */
/*	This was depriciated for the appliances has a new names 
 {
		32,
		"edag52sxapci",
		"4vsx55ff1148",
		-1,
		0
	},
 */
	/* DAG 5.21SXA general*/
	{
		46,
		"edag52sxapci",
		"4vsx55ff1148",
		-1,
		0
	},
    /* DAG 5.2SXA - Copro , Appliance's image*/
	{
		32,
		"eapp52sxacp_",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	
	/* DAG 5.2SXA - PCI ,Appliance's image*/
	{
		32,
		"eapp52sxapci_",
		"4vsx55ff1148",
		-1,
		0
	},
	/* DAG 5.0Z */
	{
		33,
		"edag50zpci",
		"4vlx80ff1148",
		-1,
		0
	},
	/* DAG 8.1sx */
/*	Depreciated image. they are now 81x
	{
		35,
		"edag81sx",
		"5vlx50tff1136",
		-1,
		0
	},
*/
	/* DAG 8.11sx bigger FPGA*/
	{
		52,
		"edag81sx",
		"5vlx85tff1136",
		-1,
		0
	},
	/* DAG 8.4i bigger FPGA*/
	{
		37,
		"edag84i",
		"5vlx110tff1738",
		-1,
		0
	},
#if ENDACE_LAB
	/* DAG 8.11sx bigger FPGA*/
	{
		52,
		"d81sx",
		"5vlx85tff1136",
		-1,
		0
	},
	/* DAG 8.4i bigger FPGA*/
	{
		37,
		"d84i",
		"5vlx110tff1738",
		-1,
		0
	},
#endif 
	/* DAG 4.5z8 */
	{
		36,
		"edag45z8pci_hlb8",
		"2vp30ff1152",
		-1,
		0
	},
#if ENDACE_LAB
		/* DAG 5.0Z */
	{
		33,
		"d50zpci",
		"4vlx80ff1148",
		-1,
		0
	},
#endif
	/* DAG 5.0DUP */
	{
		34,
		"edag50duppci",
		"4vlx80ff1148",
		-1,
		0
	},
#if ENDACE_LAB
	/* DAG 5.0DUP */
	{
		34,
		"dag50duppci",
		"4vlx80ff1148",
		-1,
		0
	},
#endif 	
	
#if ENDACE_LAB
	/* DAG 5.0s PCI 2MB UNENCRYPTED */
/* Depreciated Images . LX40 are now 54 cards	
	{
		31,
		"dag50s_pci",
		"4vlx40ff1148",
		-1,
		0
	},
*/
    /* DAG 5.0sg2a PCI  UNENCRYPTED*/
	{
		38,
		"a50s2apci_",
		"4vlx80ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a Copro UNENCRYPTED */
	{
		38,
		"a50s2acp_",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
 	/* DAG 5.20sg2a PCI UNENCRYPTED */
	{
		38,
		"d50s2a_pci",
		"4vlx80ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.20sg2a copro UNENCRYPTED */
	{
		38,
		"d50s2a_cp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},

	/* DAG 5.0s PCI 4MB UNENCRYPTED */
	{
		31,
		"dag50s_pci",
		"4vlx80ff1148",
		-1,
		0
	},
	
	/* DAG 5.0s DUP3 UNENCRYPTED */
/* Depreciated Images. LX40 are now 54 cards	
	{
		31,
		"dag50sdup",
		"4vlx40ff1148",
		-1,
		0
	},
*/
	
	/* DAG 4.52ge PCI UNENCRYPTED */
	{
		25,
		"d45g2_pci",
		"2vp30ff1152",
		-1,
		0
	},

	/* DAG 4.52ge PCI UNENCRYPTED (common G2 and G4 image) */
	{
		25,
		"d45ge_pci",
		"2vp30ff1152",
		-1,
		0
	},

	/* DAG 4.54ge PCI UNENCRYPTED */
	{
		26,
		"d45g4_pci",
		"2vp30ff1152",
		-1,
		0
	},

	/* DAG 4.54ge PCI UNENCRYPTED (common G2 and G4 image) */
	{
		26,
		"d45ge_pci",
		"2vp30ff1152",
		-1,
		0
	},

	/* DAG 8.2X PCI UNENCRYPTED */
	{
		27,
		"d82x_pci",
		"4vsx35ff668",
		-1,
		0
	},
	{
		28,
		"d52x_pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.2SXA PCI UNENCRYPTED */
	{
		32,
		"d52sxa_pci",
		"4vsx55ff1148",
		-1,
		0
	},
	{
		32,
		"d52sxa_cp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
    /* DAG 5.2SXA - Copro , Appliance's image UNENCRYPTED*/
	{
		32,
		"a52sxacp_",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	
	/* DAG 5.2SXA - PCI ,Appliance's image UNENCRYPTED*/
	{
		32,
		"a52sxapci_",
		"4vsx55ff1148",
		-1,
		0
	},
	/* DAG 8.100 PCI UNENCRYPTED */
	{
		35,
		"d81sx",
		"4vsx55ff1148",
		-1,
		0
	},
	{
		32,
		"d52sxa_cp",
		"5vlx50tff1136",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 4.5z8 */
	{
		36,
		"dag45z8pci_hlb8",
		"2vp30ff1152",
		-1,
		0
	},
#endif /* ENDACE_LAB */

       /* DAG 5.4S12 OC3/OC12 image */
	{
		39,
		"edag54s12pci",
		"4vlx40ff1148",
		-1,
		0
	},
        /* DAG 5.4SG48 OC48 image */
	{
		40,
		"edag54sg48pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a PCI */
	{
		41,
		"edag50s2apci",
		"4vlx80ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a Copro */
	{
		41,
		"edag50s2acp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.4ga PCI */
	{
		42,
		"edag54gapci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.4ga Copro */
	{
		42,
		"edag54gacp",
		"4vlx40ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.4sa12 PCI */
	{
		43,
		"edag54sa12pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.4sa12 Copro */
	{
		43,
		"edag54sa12cp",
		"4vlx40ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.4sga48 PCI */
	{
		44,
		"edag54sga48pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.4sga48 Copro */
	{
		44,
		"edag54sga48cp",
		"4vlx40ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
#ifdef ENDACE_LAB
       /* DAG 5.4S12 OC3/OC12 image UNENCRYPTED */
	{
		39,
		"d54s12_pci",
		"4vlx40ff1148",
		-1,
		0
	},
        /* DAG 5.4SG48 OC48 image UNENCRYPTED */
	{
		40,
		"d54sg48_pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a PCI UNENCRYPTED */
	{
		41,
		"d50s2a_pci",
		"4vlx80ff1148",
		-1,
		0
	},
	/* DAG 5.0sg2a Copro UNENCRYPTED */
	{
		41,
		"d50s2a_cp",
		"4vsx55ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.4ga PCI UNENCRYPTED */
	{
		42,
		"d54ga_pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.4ga Copro UNENCRYPTED */
	{
		42,
		"d54ga_cp",
		"4vlx40ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.4sa12 PCI UNENCRYPTED */
	{
		43,
		"d54sa12_pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.4sa12 Copro UNENCRYPTED */
	{
		43,
		"d54sa12_cp",
		"4vlx40ff1148",
		(int)kCoprocessorBuiltin,
		0
	},
	/* DAG 5.4sga48 PCI UNENCRYPTED */
	{
		44,
		"d54sga48_pci",
		"4vlx40ff1148",
		-1,
		0
	},
	/* DAG 5.4sga48 Copro UNENCRYPTED */
	{
		44,
		"d54sga48_cp",
		"4vlx40ff1148",
		(int)kCoprocessorBuiltin,
		0
	},

#endif /* ENDACE_LAB */
    /* DAG 8.1X (8102) */
	{
		45,
		"edag81xpci_",
		"5vlx50tff1136",
		-1,
		0
	},
    /* DAG 8.1X (8100) */
	{
		35,
		"edag81xpcies_",
		"5vlx50tff1136",
		-1,
		0
	},
#ifdef ENDACE_LAB
/* DAG 8.1X  (8102)- Unencrypted*/
	{
		45,
		"d81x_pci_",
		"5vlx50tff1136",
		-1,
		0
	},
    /* DAG 8.1X (8100) - Unencrypted*/
	{
		35,
		"d81x_pcies_",
		"5vlx50tff1136",
		-1,
		0
	},
#endif /* ENDACE_LAB*/
	#if 0
	/*Dag 7.5g2 card.*/
	{
		47,
		"d75g2_pci",
		"5vlx50tff1136",
		-1,
		0
	},
	#endif
	/*Dag 7.5g2 card.*/
	{
		47,
		"edag75g2pci",
		"5vlx50tff1136",
		-1,
		0
	},
	#if 0
	/*Dag 7.5g4 card.*/
	{
		49,
		"d75g4_pci",
		"5vlx85tff1136",
		-1,
		0
	},
	#endif
	/*Dag 7.5g4 card.*/
	{
		49,
		"edag75g4pci",
		"5vlx85tff1136",
		-1,
		0
	},
	/*Dag 7.5be card.*/
	{
		56,
		"edag75bepci",
		"5vlx110tff1136",
		-1,
		0
	},
	/*Dag 7.5ce card.*/
	{
		57,
		"edag75cepci",
		"5vlx55tff1136",
		-1,
		0
	},
	/*Dag 7.4s card.*/
	{
		54,
		"edag74spci",
		"5vlx85tff1136",
		-1,
		0
	},
	/*Dag 7.4s48 card.*/
	{
		55,
		"edag74s48pci",
		"5vlx85tff1136",
		-1,
		0
	},
	/*Dag 8.3x card.*/
	{
		50,
		"edag91xrx_",
		"5vsx50tff665",
		0,
		0
	},
	/*Dag 9.1xTx card*/
	{
		50,
		"edag91xtx_",
		"5vlx50tff665",
		1,
		0
	},
	/* DAG 4.588 */
	{
		48,
		"edag45ptppci",
		"2vp30ff1152",
		-1,
		0
	},
    /*Dag 8.5 Infiniband card.*/
    {
        51,
        "edag85idpci_",
        "5vfx130tff1738",
        -1,
        0
     },

   	/* end */
	{
		IMG_ID_END,
		NULL,
		NULL,
		-1,
		-1
	}
};

/*
 * device_id is the pci device id.
 * device_index is the fpga position, e.g. 0 for pci, 1 for pp, 2 for copro
 * Returns img_idx or -1 if not found.
 */
int
dag_get_img_idx(int device_id, int device_index)
{
	int c;

	for(c=0; dag_img_array[c].device_id != DAG_IMG_END; c++) {
		if((dag_img_array[c].device_id == device_id) && (dag_img_array[c].load_idx == device_index))
			return dag_img_array[c].img_idx;
	}

	return -1;
}

/*
 * img_idx is the result from dag_get_img_idx
 * cropro_id is the copro device id from dagpci
 * Returns *img_id_t to first matching entry or NULL if not found.
 */
img_id_t*
dag_get_img_id(int img_idx, int copro_id, int board_rev)
{
	int c;

	for(c=0; img_id_array[c].img_idx != IMG_ID_END; c++) {
		if ((img_id_array[c].img_idx == img_idx) && (img_id_array[c].copro_id == copro_id) && (board_rev >= img_id_array[c].board_rev))
			return &img_id_array[c];
	}

	return NULL;
}

/*
 * img_idx is teh result from dag_get_img_idx
 * copro_id is the copro device id from dagpci
 * arec is the Xilinx bitstream A record from the image you wish to load
 * brec is the Xilinx bitstream B record from the image you wish to load
 * Returns 0 for exact match, 1 for brec only match, 2 for non-match.
 */
int
dag_check_img(int img_idx, int copro_id, char *arec, char *brec, int board_rev)
{
	int c;
	int retval=2; /* no match */

	for(c=0; img_id_array[c].img_idx != IMG_ID_END; c++) {
		if (img_id_array[c].img_idx == img_idx) {
			if (board_rev >= img_id_array[c].board_rev) {
				if(strcmp(brec, img_id_array[c].img_type) == 0) {
					/* brec match, allow with force */
					retval = 1;
					if(strncmp(arec, img_id_array[c].img_name, dagutil_min(strlen(arec), strlen(img_id_array[c].img_name))) == 0)
						if ( (img_id_array[c].copro_id == -1) ||
						(img_id_array[c].copro_id == copro_id) )
							return 0; /* exact match */
				}
			}
		}
	}

	return retval;
}

/*
 * img_idx is the result from dag_get_img_idx
 * copro_id is the copro device id from dagpci
 * img is a pointer to the actual image you want to load
 * img_size is the size of the image you want to load
 * Returns 0 for exact match, 1 for brec only match, 2 for non-match, -1 on error.
 */
int
dag_check_img_ptr(int img_idx, int copro_id, char* img, int img_size, int board_rev)
{
	int c, first=0;
	int retval=2; /* no match */
	int copro_mismatch = 0;
	char *xrev[5];

	if (img_size < 128) /* not enough data */
		return -1;

	/* check for serial block */
	if (*(int*)img == DAGSERIAL_ID) {
		img += DAGSERIAL_SIZE;
		img_size -= DAGSERIAL_SIZE;
	}

	dag_xrev_parse((uint8_t*) img, img_size, xrev);

	if(xrev[0] == NULL) /* no arec found */
		return -1;

	if(xrev[1] == NULL) /* no brec found */
		return -1;

	for(c=0; img_id_array[c].img_idx != IMG_ID_END; c++) {
		if (img_id_array[c].img_idx == img_idx) {
			if (board_rev >= img_id_array[c].board_rev) {
				if(!first) first=c;
				if(strcmp(xrev[1], img_id_array[c].img_type) == 0) {
					/* brec match, allow with force */
					retval = 1;
					if((strncmp(xrev[0], img_id_array[c].img_name, dagutil_min(strlen(xrev[0]), strlen(img_id_array[c].img_name))) == 0)) {
						if ( (img_id_array[c].copro_id == -1) ||
						     (img_id_array[c].copro_id == copro_id) )
						{
							return 0; /* exact match */
						} else {
							copro_mismatch = 1;
						}
					}
				}
			}
		}
	}

	if(retval) {
		dagutil_warning("expecting: %s %s* your image: %s %s\n",
				img_id_array[first].img_type,
				img_id_array[first].img_name,
				xrev[1],
				xrev[0]);
		if (copro_mismatch) {
			dagutil_warning("Copro model mismatch!\n");
			dagutil_warning("Check you are loading the correct firmware for your copro model: %s\n", 
					dag_copro_name(copro_id,0));
		}
	}

	return retval;
}


/*
 * (1) check whether the image file matches the DAG card. return 0 if it does not match 
 * (2) if they match, then return -1 for non-coprocessor image
 * (3) if they match, then return copro_id for coprocessor image
 */
int
dag_check_img_type(int img_idx, int copro_id, char* img, int img_size, int board_rev)
{
	int c;
	char *xrev[5];

	if (img_size < 128) /* not enough data */
		return 0;

	/* check for serial block */
	if (*(int*)img == DAGSERIAL_ID) {
		img += DAGSERIAL_SIZE;
		img_size -= DAGSERIAL_SIZE;
	}

	dag_xrev_parse((uint8_t*) img, img_size, xrev);

	if(xrev[0] == NULL) /* no arec found */
		return 0;

	if(xrev[1] == NULL) /* no brec found */
		return 0;

	for(c=0; img_id_array[c].img_idx != IMG_ID_END; c++) {
		if (img_id_array[c].img_idx == img_idx) {
			if (board_rev >= img_id_array[c].board_rev) {
				if(strcmp(xrev[1], img_id_array[c].img_type) == 0) {
					if(strncmp(xrev[0], img_id_array[c].img_name, dagutil_min(strlen(xrev[0]), strlen(img_id_array[c].img_name))) == 0) {
						return img_id_array[c].copro_id ;
					}
				}
			}
		}
	}

	return 0;
}

