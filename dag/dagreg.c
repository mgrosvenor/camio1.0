/*
 * Copyright (c) 2003-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Ltd and no part
 * of it may be redistributed, published or disclosed except as outlined
 * in the written contract supplied with this product.
 *
 * $Id: dagreg.c,v 1.24 2005/11/07 21:52:24 sfd Exp $
 */

#ifdef __linux__
# ifndef __KERNEL__
#  include <stdlib.h>
# endif /* KERNEL */
#endif /* __linux__ */

#include "dagreg.h"

#ifdef _WIN32
#ifdef __KERNEL__
#include "StdWinDcls.h"
#else
#include "windll.h"
#endif /* __KERNEL__ */
#endif /* _WIN32 */	

static int    	caps_array[] = {
	0, /*CAPS_INVALID*/
	1, /*CAPS_BASIC*/
	1, /*CAPS_ATM*/
	1, /*CAPS_POS*/
	2, /*CAPS_ATM_DUCK*/
	2, /*CAPS_POS_DUCK*/
	1, /*CAPS_ATM_TX*/
	1, /*CAPS_ETH*/
	2, /*CAPS_ETH_DUCK*/
	2, /*CAPS_ATM_DUCK_FIFO*/
	2, /*CAPS_POS_DUCK_FIFO*/
	2, /*CAPS_ETH_DUCK_FIFO*/
	3, /*CAPS_PCI_DAG4*/
	0, /*CAPS_PCI_DAG34*/
	4, /*CAPS_PCI_DAG35*/
	7, /*CAPS_PCI_DAG42*/
	0, /*CAPS_PCI_DAG35ECM*/
	8, /*CAPS_PCI_DAG42E*/
	4, /*CAPS_PCI_DAG35PBM*/
	4, /*CAPS_PCI_DAG35PC*/
	5, /*CAPS_PCI_DAG35E*/
	9, /*CAPS_PCI_DAG60*/
	0, /*CAPS_PCI_DAG38*/
	0, /*CAPS_PCI_DAG43*/
	0, /*CAPS_PCI_DAG43E*/
	6, /*CAPS_PCI_DAG36D*/
	0, /*CAPS_PCI_DAG36E*/
	0, /*CAPS_PCI_DAG36G*/
};

static dag_reg_t reg_array[][13] = {
	{
		/* index 0 Null */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 1 Basic */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0000,
			DAG_REG_ARM, /* arm mailboxes */
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x0000,
			DAG_REG_ARM_RAM,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x2000,
			DAG_REG_ARM_PHY,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_PCI,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_X1,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xf000,
			DAG_REG_ARM_EEP,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		},
	},
	{
		/* index 2 Basic w/duck */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0000,
			DAG_REG_ARM, /* arm mailboxes */
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x00a0,
			DAG_REG_DUCK, /* DUCK */
			0,
			3,
		},
		{
			0x0000,
			DAG_REG_ARM_RAM,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x2000,
			DAG_REG_ARM_PHY,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_PCI,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_X1,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xf000,
			DAG_REG_ARM_EEP,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 3 Dag4.1x */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0000,
			DAG_REG_ARM, /* arm mailboxes */
			0,
			0,
		},
		{
			0x0020,
			DAG_REG_DUCK, /* Emu DUCK */
			0,
			1,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x0100,
			DAG_REG_PBM, /* PBM */
			0,
			0,
		},
		{
			0x0000,
			DAG_REG_ARM_RAM,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x1000,
			DAG_REG_ARM_PHY,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_X3,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x7000,
			DAG_REG_ARM_X2,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xa000,
			DAG_REG_ARM_PCI,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xa000,
			DAG_REG_ARM_X1,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xf000,
			DAG_REG_ARM_EEP,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 4 DAG 3.5x w/arm */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0000,
			DAG_REG_ARM, /* arm mailboxes */
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x0100,
			DAG_REG_PBM, /* PBM */
			0,
			0,
		},
		{
			0x0400,
			DAG_REG_SONIC_3, /* SONIC OC3/12 */
			0,
			0,
		},
		{
			0x0500,
			DAG_REG_GPP, /* GPP */
			0,
			0,
		},
		{
			0x0600,
			DAG_REG_DUCK, /* DUCK */
			0,
			2,
		},
		{
			0x0000,
			DAG_REG_ARM_RAM,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_PCI,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_X1,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xf000,
			DAG_REG_ARM_EEP,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 5 DAG 3.5E w/o arm */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x0100,
			DAG_REG_PBM, /* PBM */
			0,
			0,
		},
		{
			0x0150,
			DAG_REG_FPGAP, /* FPGA Programming space */
			0,
			0,
		},
		{
			0x0158,
			DAG_REG_ROM, /* ROM */
			0,
			0,
		},
		{
			0x0400,
			DAG_REG_ICS1893,
			0,
			0,
		},
		{
			0x0500,
			DAG_REG_GPP, /* GPP */
			0,
			0,
		},
		{
			0x0600,
			DAG_REG_DUCK, /* DUCK */
			0,
			2,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 6 DAG 3.6D w/arm */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0000,
			DAG_REG_ARM, /* arm mailboxes */
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x0100,
			DAG_REG_PBM, /* PBM */
			0,
			0,
		},
		{
			0x0400,
			DAG_REG_SONIC_D, /* SONIC DS3 */
			0,
			0,
		},
		{
			0x0500,
			DAG_REG_GPP, /* GPP */
			0,
			0,
		},
		{
			0x0600,
			DAG_REG_DUCK, /* DUCK */
			0,
			2,
		},
		{
			0x0000,
			DAG_REG_ARM_RAM,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_PCI,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0x4000,
			DAG_REG_ARM_X1,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xf000,
			DAG_REG_ARM_EEP,
			DAG_REG_FLAG_ARM,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 7 DAG 4.2x */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x00a0,
			DAG_REG_DUCK, /* DUCK */
			0,
			0,
		},
		{
			0x0100,
			DAG_REG_PBM, /* PBM */
			0,
			0,
		},
		{
			0x0158,
			DAG_REG_ROM, /* ROM */
			0,
			0,
		},
		{
			0x0180,
			DAG_REG_GPP, /* GPP */
			0,
			0,
		},
		{
			0x1000,
			DAG_REG_VSC9112, /* Vitesse */
			0,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 8 DAG 4.2xE */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x00a0,
			DAG_REG_DUCK, /* DUCK */
			0,
			0,
		},
		{
			0x0100,
			DAG_REG_PBM, /* PBM */
			0,
			0,
		},
		{
			0x0158,
			DAG_REG_ROM, /* ROM */
			0,
			0,
		},
		{
			0x0180,
			DAG_REG_GPP, /* GPP */
			0,
			0,
		},
		{
			0x1000,
			DAG_REG_MINIMAC, /* Minimac */
			0,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
	{
		/* index 9 DAG 6.0/1 */
		{
			0xffa5,
			DAG_REG_START,
			0,
			0,
		},
		{
			0x0080,
			DAG_REG_GENERAL, /* general registers */
			0,
			0,
		},
		{
			0x00C0,
			DAG_REG_SONET,
			0,
			0,
		},
		{
			0x0100,
			DAG_REG_PBM, /* PBM */
			0,
			0,
		},
		{
			0x0158,
			DAG_REG_ROM, /* ROM */
			0,
			0,
		},
		{
			0x0340,
			DAG_REG_FPGAP, /* FPGA Programming space */
			0,
			1,
		},
		{
			0x8000,
			DAG_REG_S19205, /* AMCC */
			0,
			0,
		},
		{
			0xffff,
			DAG_REG_END,
			0,
			0,
		}
	},
};


#define DAG_REG_BASE    0x900
#define CAPS_BASE       0x98
#define CAPS_MAGIC      0xa6000000
#define CAPS_MAGIC_MASK 0xff000000

/* If module is 0 (start), then get all the entries, including
 * table pointers and start/ends.
 * If module is non-zero, get only matching records.
 * Return value is number of records found, or -1 on error.
 */ 
int
dag_reg_find(char *iom, dag_reg_module_t module, dag_reg_t result[DAG_REG_MAX_ENTRIES])
{
	dag_reg_t* rptr;
	int caps;
	uint32_t len = 0;
	int retval = 0;

	rptr = (dag_reg_t*)(iom+DAG_REG_BASE);
#ifndef _WIN32
	module &= 0xff;
#endif /* _WIN32 */
	
	if((rptr->module!=DAG_REG_START)||(rptr->addr!=DAG_REG_ADDR_START)) {
		/* No Reg table in HW */
		caps = *(int*)(iom+CAPS_BASE);
		if((caps & CAPS_MAGIC_MASK) != CAPS_MAGIC) {
			return -1;
		}
		caps &= ~CAPS_MAGIC_MASK;
		rptr = reg_array[caps_array[caps]];
	}	
#ifndef _WIN32
	if((retval=dag_reg_table_find(rptr, 0, module, result, &len))!=0)
#else /* _WIN32 */
	if((retval=dag_reg_table_find(rptr, 0, module & 0xff, result, &len))!=0)
#endif /* _WIN32 */
		return -1;

	return len;
}

/* this is an internal function that is recursive for child table pointers
 * It can also be used externally to access a cached result table e.g. dagapi */
int
dag_reg_table_find(dag_reg_t *table, uint32_t toffset, uint8_t module, dag_reg_t *result, uint32_t *len)
{
	dag_reg_t* rptr = table;
	dag_reg_t* child;
	int starts = 0;

	if((rptr->module!=DAG_REG_START)||(rptr->addr!=DAG_REG_ADDR_START)) {
		return -1;
	}

	while(!((rptr->module == DAG_REG_END) && (starts==1))) {
		if (*len == DAG_REG_MAX_ENTRIES-1) {
			return -2;
		}
		if (!(rptr->flags&DAG_REG_FLAG_TABLE) && (rptr->module == DAG_REG_START))
			starts++;
		else if (rptr->module == DAG_REG_END)
			starts--;
		if(rptr->flags&DAG_REG_FLAG_TABLE) {
			if(toffset)
				child = (dag_reg_t*)(((char*)table)+rptr->addr);
			else
				child = (dag_reg_t*)(((char*)table)-DAG_REG_BASE+rptr->addr);
			if(dag_reg_table_find(child, rptr->addr, module, result, len) == -2)
				return -1;
		} else if((!module) || (module && (module == rptr->module))) {
			result[*len] = *rptr;
			if ((rptr->module > DAG_REG_START) && (rptr->module < DAG_REG_END))
				result[*len].addr += toffset;
			*len += 1;
		}
		rptr++;
	}
	
	/* Copy END entry as well */
	if (!module) {
		result[*len] = *rptr;
		*len += 1;
	}

	return 0;
}
