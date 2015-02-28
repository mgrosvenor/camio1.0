/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* File header. */
#include "../include/util/utility.h"

/* Endace headers. */
#include "dagpci.h"

/* C Standard Library headers. */
#include <assert.h>
#include <errno.h>
#include <stdlib.h>


/* Simple XOR checksum on the given buffer. */
uint32_t
compute_checksum(void* buffer, int bytes)
{
	uint32_t* overlay4 = (uint32_t*) buffer;
	uint16_t* overlay2 = (uint16_t*) buffer;
	uint8_t* overlay1 = (uint8_t*) buffer;
	uint32_t result = 0x12345678;
	int remaining = bytes;
	
	assert(NULL != buffer);
	assert(0 != bytes);
	
	/* Process buffer in multiples of four bytes. */
	while (remaining >= 4)
	{
		result ^= (*overlay4);
		remaining -= 4;
		
		overlay4 += 1;
		overlay2 += 2;
		overlay1 += 4;
	}
	
	/* Three or fewer bytes remaining. */
	if (remaining > 1)
	{
		result ^= (*overlay2);
		remaining -= 2;
		
		overlay1 += 2;
	}
	
	if (1 == remaining)
	{
		result ^= (*overlay1);
	}
	
	return result;
}


dag_card_t
pci_device_to_cardtype(uint32_t pci_device)
{
	switch (pci_device)
	{
		case PCI_DEVICE_ID_DAG3_50E: return kDag35e;
		case PCI_DEVICE_ID_DAG3_50:  return kDag35;
		case PCI_DEVICE_ID_DAG3_60D: return kDag36d;
		case PCI_DEVICE_ID_DAG3_60E: return kDag36e;
		case PCI_DEVICE_ID_DAG3_68E: return kDag36ge;
        	case PCI_DEVICE_ID_DAG3_70D: return kDag37d;
		case PCI_DEVICE_ID_DAG3_78E: return kDag37ge;
		case PCI_DEVICE_ID_DAG3_70T: return kDag37t;
		case PCI_DEVICE_ID_DAG3_7T4: return kDag37t4;
		case PCI_DEVICE_ID_DAG3_80:  return kDag38;
		case PCI_DEVICE_ID_DAG4_22E: return kDag42ge;
		case PCI_DEVICE_ID_DAG4_23E: return kDag423ge;
		case PCI_DEVICE_ID_DAG4_20:  return kDag42;
		case PCI_DEVICE_ID_DAG4_23:  return kDag423;
		case PCI_DEVICE_ID_DAG4_30E: return kDag43ge;
		case PCI_DEVICE_ID_DAG4_30:  return kDag43s;
		case PCI_DEVICE_ID_DAG4_52E: return kDag452e;
		case PCI_DEVICE_ID_DAG4_52F: return kDag452gf;
		case PCI_DEVICE_ID_DAG4_54E: return kDag454e;
		case PCI_DEVICE_ID_DAG4_5CF: return kDag452cf;
		case PCI_DEVICE_ID_DAG4_52Z: return kDag452z;
		case PCI_DEVICE_ID_DAG4_52Z8: return kDag452z8;
		case PCI_DEVICE_ID_DAG5_00S: return kDag50s;
		case PCI_DEVICE_ID_DAG5_0Z: return kDag50z;
		case PCI_DEVICE_ID_DAG5_0DUP: return kDag50dup;
		case PCI_DEVICE_ID_DAG5_0SG2A: return kDag50sg2a;
		case PCI_DEVICE_ID_DAG5_0SG3A: return kDag50sg3a;
		case PCI_DEVICE_ID_DAG5_0SG2ADUP: return kDag50sg2adup;
		case PCI_DEVICE_ID_DAG5_20E: return kDag52x;
		case PCI_DEVICE_ID_DAG5_2SXA: return kDag52sxa;
		case PCI_DEVICE_ID_DAG5_21SXA: return kDag52sxa;
		case PCI_DEVICE_ID_DAG6_00:  return kDag60;
		case PCI_DEVICE_ID_DAG6_10:  return kDag61;
		case PCI_DEVICE_ID_DAG6_20:  return kDag62;
		case 0x7000:                 return kDag70s;
		case PCI_DEVICE_ID_DAG7_00E: return kDag70ge;
		case PCI_DEVICE_ID_DAG7_10:  return kDag71s;
        case PCI_DEVICE_ID_DAG8_20E: return kDag82x;
        case PCI_DEVICE_ID_DAG8_20F: return kDag82x2;
        case PCI_DEVICE_ID_DAG8_20Z: return kDag82z;
        case PCI_DEVICE_ID_DAG8_000: return kDag800;
        case PCI_DEVICE_ID_DAG8_100: return kDag810;
        case PCI_DEVICE_ID_DAG8_101: return kDag810;
        /* 8102 similar to 8100*/
        case PCI_DEVICE_ID_DAG8_102: return kDag810;
        case PCI_DEVICE_ID_DAG8_400: return kDag840;
	    case PCI_DEVICE_ID_DAG5_4S12: return kDag54s12;
	    case PCI_DEVICE_ID_DAG5_4SG48: return kDag54sg48;
	    case PCI_DEVICE_ID_DAG5_4GA: return kDag54ga;
	    case PCI_DEVICE_ID_DAG5_4SA12: return kDag54sa12;
	    case PCI_DEVICE_ID_DAG5_4SGA48: return kDag54sga48;
        case PCI_DEVICE_ID_DAG_7_400: return kDag74s;
        case PCI_DEVICE_ID_DAG_7_401: return kDag74s48;
        case PCI_DEVICE_ID_DAG_7_5G2: return kDag75g2;
        case PCI_DEVICE_ID_DAG_7_5G4: return kDag75g4;
        case PCI_DEVICE_ID_DAG_7_5BE: return kDag75be;
        case PCI_DEVICE_ID_DAG_7_5CE: return kDag75ce;
        case PCI_DEVICE_ID_DAG_9_1x2Rx: return kDag91x2Rx;
        case PCI_DEVICE_ID_DAG_9_1x2Tx: return kDag91x2Tx;
		case PCI_DEVICE_ID_DAG8_500: return kDag850;
        case PCI_DEVICE_ID_DAG9_2X2 : return kDag92x;
 
		default:
			/* Do nothing. */
			break;
	}

	assert(0);
	return kDagUnknown;
}
