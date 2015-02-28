/*
 * Copyright (c) 2006-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: dag_smbus.c,v 1.2 2008/07/01 05:36:40 wilson.zhu Exp $
 */

/* Routines that are generally useful throughout the programs in the tools directory. 
 * Subject to change at a moment's notice, not supported for customer applications, etc.
 */

/* File header. */
#include "dag_smbus.h"

/* Endace headers. */
#include "dagapi.h"
#include "dagutil.h"

/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: dag_smbus.c,v 1.2 2008/07/01 05:36:40 wilson.zhu Exp $";


enum
{
	/* From SMB. */
	SMB_Ctrl = 0x0,
	SMB_Read = 0x4
};


/* Safety counters for SMBus read/write. 
 * 500us in total waiting for an answer is enough by Gerard's reckoning.
 */
#define SAFETY_ITERATIONS 5000
#define SAFETY_NANOSECONDS 300

/* Counter for smb_init() loop that reads the ready bit. */
#define SMB_INIT_SAFETY_ITERATIONS 500


/* SMBus related routines. */
uint32_t
dagutil_smb_init(volatile uint8_t* iom, dag_reg_t result[DAG_REG_MAX_ENTRIES], unsigned int count)
{
	int index;

	for (index = 0; index < count; index++)
	{
		volatile uint32_t readval = 0;
		uint32_t safety_counter = SMB_INIT_SAFETY_ITERATIONS;
		uint32_t reg_address = DAG_REG_ADDR(result[index]);
		
		/* Probe for thermal chip. */
		*(volatile uint32_t *)(iom + reg_address + SMB_Ctrl) = ((((uint32_t) LM63_TEMP_LOC << 8) | (uint32_t) LM63 | 0x01000000));
		usleep(400);
		
		/* Wait for ready bit (bit 1) to be set. */
		readval = *(volatile uint32_t *)(iom + reg_address + SMB_Read);
		while (0 == (readval & BIT1))
		{
			usleep(400);
			readval = *(volatile uint32_t *)(iom + reg_address + SMB_Read);
			
			safety_counter--;
			if (0 == safety_counter)
			{
				/* At least one second has passed. */
				dagutil_warning("exceeded safety counter while looking for thermal chip.\n");
				return 0;
			}
		}
		
		/* Valid SMBus should have 1 in bit 24 () and 0 in bit 3 (error bit). */
		if ((readval & BIT24) && (0 == (readval & BIT3)))
		{
			/* Found the SMBus with the fan. */
			return reg_address;
		}
	}
	
	dagutil_panic("could not find thermal controller on SMBus.\n");
	return 0;
}


int
dagutil_smb_write(volatile uint8_t* iom, uint32_t smb_base, unsigned char device, unsigned char address, unsigned char value)
{
	int safety = SAFETY_ITERATIONS;
	volatile uint32_t reg_value;

	if (smb_base == 0)
	{
		dagutil_warning("No SMB controller\n");
		return 0;
	}
	
	*(volatile uint32_t *)(iom + smb_base + SMB_Ctrl) = (((uint32_t) value << 16) | ((uint32_t) address << 8) | (uint32_t) device);

	reg_value = (*(volatile uint32_t *)(iom + smb_base + SMB_Read) & 0x2);
	while ((0 != safety) && (0 == reg_value))
	{
		dagutil_nanosleep(SAFETY_NANOSECONDS);
		safety--;
		reg_value = (*(volatile uint32_t *)(iom + smb_base + SMB_Read) & 0x2);
	}

	if (0 == reg_value)
	{
		dagutil_warning("exceeded safety counter while writing to SMBus\n");
		return 0;
	}
	
	return 1;
}


int
dagutil_smb_read(volatile uint8_t* iom, uint32_t smb_base, unsigned char device, unsigned char address, unsigned char *value)
{
	int safety = SAFETY_ITERATIONS;
	volatile uint32_t readval;

	if (smb_base == 0)
	{
		dagutil_warning("No SMB controller\n");
		return 0;
	}
	
	*value = 0;

	*(volatile uint32_t *)(iom + smb_base + SMB_Ctrl) = ((((uint32_t) address << 8) | (uint32_t) device | 0x01000000));
	usleep(400);
	
	readval = *(volatile uint32_t *)(iom + smb_base + SMB_Read);
	
	while ((0 != safety) && (0 == (readval & BIT24)))
	{
		
		dagutil_nanosleep(SAFETY_NANOSECONDS);
		safety--;
		*(volatile uint32_t *)(iom + smb_base + SMB_Ctrl) = ((((uint32_t) address << 8) | (uint32_t) device | 0x01000000));
		usleep(400);
		readval = *(volatile uint32_t *)(iom + smb_base + SMB_Read);
	}
	
	if (readval & BIT24)
	{
		*value = (unsigned char) (readval >> 16);
		return 1;
	}

	dagutil_warning("exceeded safety counter while reading from SMBus\n");
	return 0;
}


/*
 * Setup temperature parameters on LM63 part.
 * You must ensure the card has this feature
 * before calling this function.
 */
void
dagutil_start_copro_fan(volatile uint8_t* iom, uint32_t smb_base)
{
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_PWM_RPM, 0x20); /* enable PWM writes */

#ifdef DYNAMIC_FAN
	/* Deprecated, remote diode sensor readings are not reliable */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T1, 0x00); /* 0C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P1, 0x00); /* 0% */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T2, 0x30); /* 48C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P2, 0x10); /* 25% */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T3, 0x38); /* 56C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P3, 0x20); /* 50% */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T4, 0x40); /* 64C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P4, 0x30); /* 75% */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T5, 0x50); /* 80C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P5, 0x3f); /* 100% */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T6, 0x60); /* 96C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P6, 0x3f); /* 100% */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T7, 0x70); /* 112C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P7, 0x3f); /* 100% */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_T8, 0x80); /* 128C */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOOKUP_P8, 0x3f); /* 100% */

	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_PWM_RPM, 0x0); /* Use dynamic table */
#else
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_PWM_VALUE, 0x3f); /* Set PWM to max */
#endif
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_CONF, 0x01); /* RDTS fault queue enable */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_SPINUP, 0x25); /* spin-up at 100% for 0.8s */

	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_LOC_HIGH, 0x55); /* 85C - local high temperature */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_REM_HIGH, 0x63); /* 99C - remote high temperature */
	(void) dagutil_smb_write(iom, smb_base, LM63, LM63_ALERT_MASK, 0xBF); /* local high alert only */
}
