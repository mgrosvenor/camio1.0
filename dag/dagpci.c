#if defined(__linux__)
# ifdef __KERNEL__
#  include <linux/kernel.h>
#  include <linux/init.h>
#  include <linux/pci.h>
# else
#  include <stdio.h>
# endif
#elif defined (_KERNEL) && defined (__FreeBSD__) 
#else
# include <stdio.h>
#endif

#if defined(_WIN32)
typedef __int32 uint32_t;
#define snprintf _snprintf 
#endif /* _WIN32 */

#include "dagpci.h"

#define PCI_VENDOR_ID_DAG	0x121b
#define PCI_VENDOR_ID_ENDACE	0xeace


#ifdef __linux__
# ifdef __KERNEL__
struct pci_device_id dag_pci_tbl[] __devinitdata = {
	{ PCI_VENDOR_ID_DAG,	PCI_DEVICE_ID_DAG3_20,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_DAG,	PCI_DEVICE_ID_DAG3_50,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_DAG,	PCI_DEVICE_ID_DAG4_10,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_DAG,	PCI_DEVICE_ID_DAG4_11,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_DAG,	PCI_DEVICE_ID_DAG4_22E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_DAG,	PCI_DEVICE_ID_DAG4_23,	PCI_ANY_ID, PCI_ANY_ID, },

	{ PCI_VENDOR_ID_ENDACE,	PCI_DEVICE_ID_DAG3_50,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_50E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_60D,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_60E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_68E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_70D,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_70T,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_7T4,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_78E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG3_80,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_22E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_23,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_23E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_30,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_30E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_40E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_52E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_52F,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_54E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_588,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_5CF, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_52Z, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_52Z8, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_00S,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_20E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG6_00,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG6_10,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG6_20,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG7_00E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG7_10,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_20E,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_20F,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_20Z,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_000,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_100,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_101,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_102,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAGX1_00,	PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG4_52A, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_0SG2A, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_0SG2ADUP, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_2SXA, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_21SXA, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_0Z, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_0DUP, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG8_400, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_4S12, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_4SG48, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_0SG3A, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_4GA, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_4SA12, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG5_4SGA48, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_7_400, PCI_ANY_ID, PCI_ANY_ID, },
        { PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_7_401, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_7_5G2, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_7_5G4, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_7_5BE, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_7_5CE, PCI_ANY_ID, PCI_ANY_ID, },
	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_9_1x2Rx, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG_9_1x2Tx, PCI_ANY_ID, PCI_ANY_ID, },
    	{ PCI_VENDOR_ID_ENDACE,PCI_DEVICE_ID_DAG8_500,PCI_ANY_ID,PCI_ANY_ID},
        { PCI_VENDOR_ID_ENDACE, PCI_DEVICE_ID_DAG9_2X2, PCI_ANY_ID, PCI_ANY_ID, },
 
    //removed due on some systems is duplicated 
//	{ PCI_VENDOR_ID_ENDACE, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, },

	{ 0, }
};
# endif /* __KERNEL__ */
#endif /* __linux__ */

char * dag_device_name(uint32_t device, uint32_t flag)
{
	static char retbuf[64];

	switch(device) {
		/*
		 * DAG3 series
		 */
	  case PCI_DEVICE_ID_DAG3_20:	return "DAG 3.2";
	  case PCI_DEVICE_ID_DAG3_50:	return "DAG 3.5";
	  case PCI_DEVICE_ID_DAG3_50E:	return "DAG 3.5E";
	  case PCI_DEVICE_ID_DAG3_60D:	return "DAG 3.6D";
	  case PCI_DEVICE_ID_DAG3_60E:	return "DAG 3.6E";
	  case PCI_DEVICE_ID_DAG3_68E:	return "DAG 3.6GE";
	  case PCI_DEVICE_ID_DAG3_70D:	return "DAG 3.7D";
	  case PCI_DEVICE_ID_DAG3_70T:	return "DAG 3.7T";
	  case PCI_DEVICE_ID_DAG3_7T4:	return "DAG 3.7T4";
          case PCI_DEVICE_ID_DAG3_78E:  return "DAG 3.7G";
	  case PCI_DEVICE_ID_DAG3_80:	return "DAG 3.8S";
		/*
		 * DAG4 series
		 */
	  case PCI_DEVICE_ID_DAG4_10:	return "DAG 4.10";
	  case PCI_DEVICE_ID_DAG4_11:	return "DAG 4.11";
	  case PCI_DEVICE_ID_DAG4_22E:	return "DAG 4.2GE";
	  case PCI_DEVICE_ID_DAG4_23:	return "DAG 4.2S";
	  case PCI_DEVICE_ID_DAG4_23E:	return "DAG 4.2GE";
	  case PCI_DEVICE_ID_DAG4_30:	return "DAG 4.3S";
	  case PCI_DEVICE_ID_DAG4_30E:	return "DAG 4.3GE";
	  case PCI_DEVICE_ID_DAG4_40E:	return "DAG 4.4GE";
	  case PCI_DEVICE_ID_DAG4_52E:	return "DAG 4.5G2";
	  case PCI_DEVICE_ID_DAG4_52F:	return "DAG 4.5GF";
	  case PCI_DEVICE_ID_DAG4_54E:	return "DAG 4.5G4";
	  case PCI_DEVICE_ID_DAG4_588:	return "DAG 4.588";
	  case PCI_DEVICE_ID_DAG4_5CF:	return "DAG 4.5CF";
	  case PCI_DEVICE_ID_DAG4_52Z:	return "DAG 4.5Z2";
	  case PCI_DEVICE_ID_DAG4_52Z8:	return "DAG 4.5Z8";
	  /* Accelerated card with */
	  case PCI_DEVICE_ID_DAG4_52A:	return "DAG 4.52A";
		/*
		 * DAG5 series
		 */
	  case PCI_DEVICE_ID_DAG5_00S:	return "DAG 5.0SG2";
	  case PCI_DEVICE_ID_DAG5_20E:	return "DAG 5.2X";
	  
	  case PCI_DEVICE_ID_DAG5_0Z:	return "DAG 5.0Z";
	  case PCI_DEVICE_ID_DAG5_0DUP:	return "DAG 5.0DUP";
	  /* Accelerated cards */
	  case PCI_DEVICE_ID_DAG5_0SG2A: return "DAG 5.0SG2A_APP";
	  case PCI_DEVICE_ID_DAG5_0SG3A: return "DAG 5.0SG2A";
	  case PCI_DEVICE_ID_DAG5_0SG2ADUP: return "DAG 5.0SG2ADUP";
	  case PCI_DEVICE_ID_DAG5_2SXA: return "DAG 5.2SXA appliance Stu";
	  case PCI_DEVICE_ID_DAG5_21SXA: return "DAG 5.2SXA general S.W";
      case PCI_DEVICE_ID_DAG5_4S12: return "DAG 5.4S12";
	  case PCI_DEVICE_ID_DAG5_4SG48: return "DAG 5.4SG48";
	  case PCI_DEVICE_ID_DAG5_4GA: return "DAG 5.4GA" ;
      case PCI_DEVICE_ID_DAG5_4SA12: return "DAG 5.4SA12";
	  case PCI_DEVICE_ID_DAG5_4SGA48: return "DAG 5.4SGA48";
		/*
		 * DAG6 series
		 */
	  case PCI_DEVICE_ID_DAG6_00:	return "DAG 6.0SE";
	  case PCI_DEVICE_ID_DAG6_10:	return "DAG 6.1SE";
	  case PCI_DEVICE_ID_DAG6_20:	return "DAG 6.2SE";
		/*
		 * DAG7 series
		 */
	  case PCI_DEVICE_ID_DAG7_10:	return "DAG 7.1S";
	  case PCI_DEVICE_ID_DAG7_00E:	return "DAG 7.0GE";
		/*
		 * DAG8 series
		 */
	  case PCI_DEVICE_ID_DAG8_20E:	return "DAG 8.2E";
	  case PCI_DEVICE_ID_DAG8_20F:	return "DAG 8.2F";
	  case PCI_DEVICE_ID_DAG8_20Z:	return "DAG 8.2Z";
	  case PCI_DEVICE_ID_DAG8_000:	return "DAG 8.00";
	  case PCI_DEVICE_ID_DAG8_100:	return "DAG 8.1X";
	  case PCI_DEVICE_ID_DAG8_101:	return "DAG 8.1SX";
	  case PCI_DEVICE_ID_DAG8_102:	return "DAG 8.1X";
	  case PCI_DEVICE_ID_DAG8_400:	return "DAG 8.4I";
	
	case PCI_DEVICE_ID_DAG_7_400: return "DAG 7.4S"; 
        case PCI_DEVICE_ID_DAG_7_401: return "DAG 7.4S48";
      	case PCI_DEVICE_ID_DAG_7_5G2: return "DAG 7.5G2";
      	case PCI_DEVICE_ID_DAG_7_5G4: return "DAG 7.5G4";
      	case PCI_DEVICE_ID_DAG_7_5BE: return "DAG 7.5BE";
      	case PCI_DEVICE_ID_DAG_7_5CE: return "DAG 7.5CE";
      	case PCI_DEVICE_ID_DAG_9_1x2Rx: return "DAG 9.1X2Rx";
      	case PCI_DEVICE_ID_DAG_9_1x2Tx: return "DAG 9.1X2Tx";	
		/*
		 * DAG X series
		 */
	  case PCI_DEVICE_ID_DAGX1_00:	return "DAG X 1.0";

	  case PCI_DEVICE_ID_DAG8_500:  return "DAG 8.5DI";
          case PCI_DEVICE_ID_DAG9_2X2:  return "DAG 9.2X2";
 		/*
		 * None of the above
		 */
	  default:
#ifdef ENDACE_LAB
		/* Support prototype cards for the LAB */
		(void)snprintf(retbuf, 64, "unknown card 0x%.4x", device);
		return retbuf;
#else
		if(flag) {
			(void)snprintf(retbuf, 64, "unknown card 0x%.4x", device);
			return retbuf;
		} else {
			return NULL;
		}
#endif
	}

	/*NOTREACHED*/
	return NULL;
}

char * dag_copro_name(coprocessor_t device, uint32_t flag)
{
	static char retbuf[64];

	switch(device) {
	  case kCoprocessorNotSupported:	return "NA";

	  case kCoprocessorNotFitted:	return "None";

	  case kCoprocessorPrototype:	return "Prototype DO NOT USE";

	  case kCoprocessorSC128:	return "SC128";
	  case kCoprocessorSC256:	return "SC256";
	  case kCoprocessorSC256C:	return "SC256 Rev C";

	  case kCoprocessorBuiltin:	return "Built-in";

		/*
		 * None of the above
		 */
	  default:
#ifdef ENDACE_LAB
		(void)snprintf(retbuf, 64, "unknown copro 0x%.2x", device);
		return retbuf;
#else
		if(flag) {
			(void)snprintf(retbuf, 64, "unknown copro 0x%.2x", device);
			return retbuf;
		} else {
			return NULL;
		}
#endif
	}

	/*NOTREACHED*/
	return NULL;
}
