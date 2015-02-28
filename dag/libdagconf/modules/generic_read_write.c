/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#include "dag_config.h"
#include "../include/component.h"
#include "../include/attribute.h"
#include "../include/modules/generic_read_write.h"
#include "../include/util/utility.h"
#include "../include/modules/raw_smbus_module.h"
#include "../include/attribute_types.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/components/oc48_deframer_component.h"
#include "../include/components/sonet_pp_component.h"
#include "../include/components/sonic_component.h"

#include "dagutil.h"
#include "dagreg.h"

/*Randomly choosen number*/
#define GRW_MAGIC 0xff8dfe69

#define SAFETY_ITERATIONS 5000
#define SAFETY_NANOSECONDS 100
#define SMB_INIT_SAFETY_ITERATIONS 500


struct GenericReadWriteObject
{
   	/* Standard protection. */
	uint32_t mMagicNumber;
	uint32_t mChecksum;

    /* Every member from here onwards is checksummed */
    uintptr_t mAddress;
    DagCardPtr mCard;
    GenericReadRoutine fRead;
    GenericWriteRoutine fWrite;
    grw_error_t mError;
    grw_on_operation_t mOp;
	AttributePtr mAttribute;

};

/* Implementation of internal routines. */
static uint32_t
compute_grw_checksum(GenericReadWritePtr grw)
{
	/* Don't check validity here with valid_attribute() because the attribute may not be fully constructed. */
    if (grw != NULL)
    {
	    return compute_checksum(&grw->mAddress, sizeof(*grw) - 2 * sizeof(uint32_t));
    }
    return 0;
}

int
valid_grw(GenericReadWritePtr grw)
{
    uint32_t checksum;
	
	assert(grw);
	assert(GRW_MAGIC == grw->mMagicNumber);
	
	checksum = compute_grw_checksum(grw);
	assert(checksum == grw->mChecksum);

	if ((NULL != grw) && (GRW_MAGIC == grw->mMagicNumber) && (checksum == grw->mChecksum))
	{
		return 1;
	}
	
	return 0;
}


GenericReadWritePtr
grw_init(DagCardPtr card, uintptr_t address, GenericReadRoutine read, GenericWriteRoutine write)
{
    GenericReadWritePtr result = malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));    
    result->mMagicNumber = GRW_MAGIC;
    result->fRead = read;
    result->fWrite = write;
    result->mAddress = address;
    result->mCard = card;
    result->mOp = kGRWSetBit;
    result->mChecksum = compute_grw_checksum(result);
    (void)valid_grw(result);
    return result;
}

void
grw_set_on_operation(GenericReadWritePtr grw, grw_on_operation_t op)
{
    if (1 == valid_grw(grw))
    {
        grw->mOp = op;
        grw->mChecksum = compute_grw_checksum(grw);
    }
}

grw_on_operation_t
grw_get_on_operation(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        return grw->mOp;
    }
    return kGRWInvalidOperation;
}

void
grw_dispose(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
		free(grw);
    }
}

void
grw_set_read_routine(GenericReadWritePtr grw, GenericReadRoutine routine)
{
    if (1 == valid_grw(grw))
    {
        grw->fRead = routine;
        grw->mChecksum = compute_grw_checksum(grw);
    }
}

void
grw_set_attribute(GenericReadWritePtr grw, AttributePtr attr)
{
	if (1 == valid_grw(grw))
	{
		grw->mAttribute = attr;
		grw->mChecksum = compute_grw_checksum(grw);
	}
}

AttributePtr
grw_get_attribute(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		return grw->mAttribute;
	}
	return NULL;
}

void
grw_set_write_routine(GenericReadWritePtr grw, GenericWriteRoutine routine)
{
    if (1 == valid_grw(grw))
    {
        grw->fWrite = routine;
        grw->mChecksum = compute_grw_checksum(grw);
    }
}

void
grw_set_address(GenericReadWritePtr grw, uintptr_t address)
{
    if (1 == valid_grw(grw))
    {
        grw->mAddress = address;
        grw->mChecksum = compute_grw_checksum(grw);
    }
}

/* Call the object specfic read function */
uint32_t
grw_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw) && grw->fRead)
    {
        return grw->fRead(grw);
    }
    return 0;
}

/* Call the object specfic write function */
void
grw_write(GenericReadWritePtr grw, uint32_t value)
{
    if (1 == valid_grw(grw) && grw->fWrite)
    {
        grw->fWrite(grw, value);
    }
}

/* Read a memory mapped register */
uint32_t
grw_iom_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        if (grw->mAddress)
        {
            uint32_t return_value;
            volatile uint32_t* addr = (volatile uint32_t*)grw->mAddress;
            return_value = *addr;
            dagutil_verbose_level(5, "grw_iom_read: addr = 0x%" PRIxPTR ", val = %d (0x%08x)\n", (uintptr_t)addr, return_value, return_value);
            return return_value;
        }
        else
        {
            grw_set_last_error(grw, kGRWReadError);
        }
    }
    return 0;
}

void
grw_iom_write(GenericReadWritePtr grw, uint32_t value)
{
    if (1 == valid_grw(grw))
    {
        if (grw->mAddress)
        {
            {
                volatile uint32_t* addr = (volatile uint32_t*)grw->mAddress;
                dagutil_verbose_level(5, "grw_iom_write: addr = 0x%" PRIxPTR ", val = %d (0x%08x)\n", (uintptr_t)addr, value, value);
                *addr = value;
            }
        }
        else
        {
            grw_set_last_error(grw, kGRWWriteError);
        }
    }
}
uint32_t
grw_rom_read(GenericReadWritePtr grw)
{
    if(1 == valid_grw(grw))
    {
        int dagfd = 0;
        romtab_t* rp = NULL;
        int rom_number = 0;
        uint8_t buf[128];
        uint8_t* bp = buf;
        uint32_t addr;
        uint16_t value = 0;
        uint32_t size = 0;
        uint32_t return_value = 0;
        dagfd = card_get_fd(grw->mCard);
        rp = dagrom_init(dagfd, rom_number, 1);
    
        bp = buf;
        if (rp->rom_version == 0x2)
        {
            size = (((rp->rblock.sectors[0])*(rp->rblock.size_region[0])) + ((rp->rblock.sectors[1])*(rp->rblock.size_region[1])));
            for(addr = (size - 128); addr < (size - 128 + 40);addr+=2)
            {
                value = rp->romread(rp,addr);
                *(bp + 1) = (value & 0xff00) >> 8;
                *bp =  (value & 0xff);
                bp++;
                bp++;
            }
            if(*(uint32_t *)buf == 0x12345678)
            {
                return_value =  (*(uint32_t*)(buf + grw->mAddress));
            }
        }
        else
        {
            for(addr = DAGROM_TSTART; addr < (DAGROM_TSTART + sizeof(buf)); addr++)
            {
                *bp = rp->romread(rp,addr);
                bp++;
            }
            if(*(int*)buf == 0x12345678)
            {
                return_value = (*(uint32_t*)(buf + grw->mAddress));
            }
        }
        if (NULL != rp )
        {
            dagrom_free(rp);
        }
        return return_value;
    }
    return 0;
}

uint32_t
grw_oc48_deframer_status_cache_read(GenericReadWritePtr grw)
{
    ComponentPtr deframer;
    oc48_deframer_state_t* state;

	if (1 == valid_grw(grw))
	{
		deframer = attribute_get_component(grw->mAttribute);
        if (deframer)
        {
            state = component_get_private_state(deframer);
            if (state) 
            {
                return state->mStatusCache;
            }
            else 
            {
                dagutil_verbose_level(5, "%s: state is NULL\n", __FUNCTION__);
            }
        }
        else
        {
            dagutil_verbose_level(5, "%s: deframer is NULL\n", __FUNCTION__);
        }
	}
    grw_set_last_error(grw, kGRWReadError);
	return 0;
}

uint32_t
grw_sonic_status_cache_read(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		ComponentPtr sonic = attribute_get_component(grw->mAttribute);
		sonic_state_t* sonic_state = component_get_private_state(sonic);
		return sonic_state->mStatusCache;
	}
    grw_set_last_error(grw, kGRWReadError);
	return 0;
}

/**
 * Refresh the cache of the B1 Error count
 */
uint32_t
grw_sonet_pp_b1_cache_read(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		ComponentPtr comp = attribute_get_component(grw->mAttribute);
		sonet_pp_state_t* state = component_get_private_state(comp);
		return state->mB1ErrorCounterCache;
	}
    grw_set_last_error(grw, kGRWReadError);
	return 0;
}

/**
 * Refresh the cache of the B2 Error count
 * */
uint32_t
grw_sonet_pp_b2_cache_read(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		ComponentPtr comp = attribute_get_component(grw->mAttribute);
		sonet_pp_state_t* state = component_get_private_state(comp);
		return state->mB2ErrorCounterCache;
	}
    grw_set_last_error(grw, kGRWReadError);
	return 0;
}

/**
 * Refresh the cache of the B3 Error count
 */
uint32_t
grw_sonet_pp_b3_cache_read(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		ComponentPtr comp = attribute_get_component(grw->mAttribute);
		sonet_pp_state_t* state = component_get_private_state(comp);
		return state->mB3ErrorCounterCache;
	}
    grw_set_last_error(grw, kGRWReadError);
	return 0;
}

/**
 * Refresh the cache of the REI Error count
 * */
uint32_t
grw_sonet_pp_rei_cache_read(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		ComponentPtr comp = attribute_get_component(grw->mAttribute);
		sonet_pp_state_t* state = component_get_private_state(comp);
		return state->mREICounterCache;
	}
    grw_set_last_error(grw, kGRWReadError);
	return 0;
}

/* Read a register on the AMCC Framer. Note: 7.1s after RevD has AMCC1221 (before it was AMCC1213)*/
uint32_t
grw_dag71s_phy_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {        
        int wr_val, rd_val;
        GenericReadWritePtr grw_amcc = NULL;
        uintptr_t amcc_address = 0;
        wr_val = 0;
        wr_val |= BIT31;
        wr_val |= ((grw->mAddress & 0x01F) << 16);
        /*Make new grw object with the address set to the AMCC (the framer on the 7.1s) */
        amcc_address = card_get_register_address(grw->mCard, DAG_REG_AMCC, 0);
        amcc_address = (uintptr_t)(card_get_iom_address(grw->mCard) + amcc_address);
        grw_amcc = grw_init(grw->mCard, amcc_address, grw_iom_read, grw_iom_write);
        grw_write(grw_amcc, wr_val);
        usleep(1000);
        rd_val = grw_read(grw_amcc);
        free(grw_amcc);
        return (rd_val & 0x0000FFFF);
    }    
    return 0;
}

/* Write a register on the AMCC*/
void
grw_dag71s_phy_write(GenericReadWritePtr grw, uint32_t val)
{
    if (1 == valid_grw(grw))
    {        
        int wr_val;
        GenericReadWritePtr grw_amcc = NULL;
        uintptr_t amcc_address = 0;
        wr_val = 0;
        wr_val |= ((grw->mAddress & 0x01F) << 16);
        wr_val |= (val & 0x0ffff);
        /*Make new grw object with the address set to the AMCC (the framer on the 7.1s) */
        amcc_address = card_get_register_address(grw->mCard, DAG_REG_AMCC, 0);
        amcc_address = (uintptr_t)(card_get_iom_address(grw->mCard) + amcc_address);
        grw_amcc = grw_init(grw->mCard, amcc_address, grw_iom_read, grw_iom_write);
        grw_write(grw_amcc, wr_val);
        usleep(1000);
        free(grw_amcc);
    }
}

uint32_t
grw_vsc8476_raw_smbus_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t vsc_index = 0;
        uintptr_t vsc_address = 0;
        RawSMBusPtr smbus = NULL;
        uint32_t ret_val = 0;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        vsc_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        vsc_address = card_get_register_address(grw->mCard, DAG_REG_VSC8476, 1);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT1);
        raw_smbus_set_data_mask(smbus, BIT0);
        raw_smbus_set_base_address(smbus, vsc_address);
        ret_val = raw_smbus_read_byte(grw->mCard, smbus, device_address, device_register);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
        return ret_val;
    }
    return 0;
}

void
grw_vsc8476_raw_smbus_write(GenericReadWritePtr grw, uint32_t val)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t vsc_index = 0;
        uintptr_t vsc_address = 0;
        RawSMBusPtr smbus = NULL;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        vsc_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        vsc_address = card_get_register_address(grw->mCard, DAG_REG_VSC8476, 0);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT1);
        raw_smbus_set_data_mask(smbus, BIT0);
        raw_smbus_set_base_address(smbus, vsc_address);
        raw_smbus_write_byte(grw->mCard, smbus, device_address, device_register, (uint8_t)val);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
    }
}
    
uint32_t
grw_mini_mac_raw_smbus_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t mini_mac_index = 0;
        uintptr_t mini_mac_address = 0;
        RawSMBusPtr smbus = NULL;
        uint32_t ret_val = 0;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        mini_mac_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        mini_mac_address = card_get_register_address(grw->mCard, DAG_REG_MINIMAC, mini_mac_index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT15);
        raw_smbus_set_data_mask(smbus, BIT14);
        raw_smbus_set_base_address(smbus, mini_mac_address);
        ret_val = raw_smbus_read_byte(grw->mCard, smbus, device_address, device_register);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
        return ret_val;
    }
    return 0;
}

/* 
    expected parameters is the address in the grw object to be set up in advance no other parameters
    in form: bit0-bit7  - register address, bit8 - reserved (value X), bit9-bit15 - device address 
    bit 16 - bit 17 - XFP component index 0,1,2,3 (port number from 0-3)
*/
uint32_t
grw_xfp_sfp_raw_smbus_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t port_index = 0;
        uintptr_t xfp_address = 0;
        RawSMBusPtr smbus = NULL;
        uint32_t ret_val = 0;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
	/* Note supported up to 4 ports */
        port_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        xfp_address = card_get_register_address(grw->mCard, DAG_REG_XFP, port_index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT0);
        raw_smbus_set_data_mask(smbus, BIT1);
        raw_smbus_set_base_address(smbus, xfp_address);
        ret_val = raw_smbus_read_byte(grw->mCard, smbus, device_address, device_register);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
        return ret_val;
    }
    return 0;
}


uint32_t
grw_raw_smbus_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t clock_line_index = 0;
        uint8_t index = 0;
        RawSMBusPtr smbus = NULL;
        uintptr_t raw_smbus_address = 0;
        uint32_t ret_val = 0;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        clock_line_index = (uint8_t)((0xff0000 & grw->mAddress) >> 16);
        index = (uint8_t)((0xff000000 & grw->mAddress) >> 24);
        raw_smbus_address = card_get_register_address(grw->mCard, DAG_REG_RAW_SMBBUS, index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT0 << (clock_line_index*2 + 1));
        raw_smbus_set_data_mask(smbus, BIT0 << clock_line_index*2);
        raw_smbus_set_base_address(smbus, raw_smbus_address);
        ret_val = raw_smbus_read_byte(grw->mCard, smbus, device_address, device_register);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
        return ret_val;
    }
    return 0;
}

void
grw_set_last_error(GenericReadWritePtr grw, grw_error_t error)
{
    if (1 == valid_grw(grw))
    {
        grw->mError = error;
        grw->mChecksum = compute_grw_checksum(grw);
    }
}

void
grw_raw_smbus_write(GenericReadWritePtr grw, uint32_t val)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t clock_line_index = 0;
        uint8_t index = 0;
        RawSMBusPtr smbus = NULL;
        uintptr_t raw_smbus_address = 0;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
//	printf("grw_rwa_smbus_write: DEVICE ADDRESS: 0x%x, grw_>mAddress: 0x%x\n",device_address,grw->mAddress);
        device_register = (uint8_t)(0xff & grw->mAddress);
//        clock_line_index = (uint8_t)(0xff0000 & grw->mAddress);
        clock_line_index = (uint8_t)((0xff0000 & grw->mAddress) >> 16);
//        index = (uint8_t)(0xff000000 & grw->mAddress);
        index = (uint8_t)((0xff000000 & grw->mAddress) >> 24);
        raw_smbus_address = card_get_register_address(grw->mCard, DAG_REG_RAW_SMBBUS, index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT1);
        raw_smbus_set_data_mask(smbus, BIT0);
        raw_smbus_set_base_address(smbus, raw_smbus_address);
        raw_smbus_write_byte(grw->mCard, smbus, device_address, device_register, (uint8_t)val);
        raw_smbus_dispose(smbus);
    }
}

/*
Returns the value of the status register of the smbus. countdown is
a number specifiying how many times to try reading from the register
*/
uint32_t
grw_drb_smbus_busy_wait(GenericReadWritePtr grw, int* countdown)
{
    uintptr_t smbus_address = 0;
    uint32_t status_register = 0;
    uint32_t index = 0;

    if (1 == valid_grw(grw) && countdown)
    {
        index = (0x30000 & grw->mAddress) >> 16;
        smbus_address = card_get_register_address(grw->mCard, DAG_REG_SMB, index);
        status_register = card_read_iom(grw->mCard, smbus_address+0x4);
        while (((status_register & BIT1) == 0) && *countdown)
        {
            dagutil_verbose_level(4, "drb_smbus: status_register 0x%x, countdown = %d\n", status_register, *countdown);
            dagutil_verbose_level(5, "drb_smbus: busy waiting\n");
            *countdown = *countdown - 1;
            dagutil_nanosleep(100);
            status_register = card_read_iom(grw->mCard, smbus_address+0x4);
        }
        if (*countdown == 0)
        {
            grw_set_last_error(grw, kGRWWriteError);
            dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
        }
        return status_register;
    }
    return 0;
}

void
grw_drb_smbus_write(GenericReadWritePtr grw, uint32_t val)
{
	if (1 == valid_grw(grw))
	{
		uint8_t device_address = 0;
		uint8_t device_register = 0;
		uintptr_t smbus_address = 0;
		uint32_t register_value = 0;
		uint32_t status_register = 0;
		uint32_t index = 0;
		int countdown = 1000;
        
		device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
		device_register = (uint8_t)(0xff & grw->mAddress);
		index = (0xffff0000 & grw->mAddress) >> 16;
		status_register = grw_drb_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return;
		}
		smbus_address = card_get_register_address(grw->mCard, DAG_REG_SMB, index);

		register_value |= device_address;
		register_value |= device_register << 8;
		register_value |= val << 16;
		register_value &= ~BIT24;
		card_write_iom(grw->mCard, smbus_address, register_value);

		/* wait till the write completes */
		countdown = 1000;
		status_register = grw_drb_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return;
		}
		
		/* check if the command succeeded or failed */
		if (status_register & (BIT5 | BIT4 | BIT3 | BIT2))
		{
			grw_set_last_error(grw, kGRWWriteError);
			dagutil_verbose_level(4, "drb_smbus: command failed!\n");
			return;
		}

	}
}

uint32_t
grw_drb_smbus_read(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		uint8_t device_address = 0;
		uint8_t device_register = 0;
		uintptr_t smbus_address = 0;
		uint32_t ret_val = 0;
		uint32_t register_value = 0;
		uint32_t status_register = 0;
		int index = 0;
		int countdown = 1000;
        
		device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
		device_register = (uint8_t)(0xff & grw->mAddress);
		index = (0xffff0000 & grw->mAddress) >> 16;
		smbus_address = card_get_register_address(grw->mCard, DAG_REG_SMB, index);
		
		status_register = grw_drb_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return 0;
		}
		register_value |= device_address;
		register_value |= device_register << 8;
		register_value |= BIT24;
		card_write_iom(grw->mCard, smbus_address, register_value);
		countdown = 1000;
		status_register = grw_drb_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return 0;
		}
		
		/* check if the command succeeded or failed */
		if (status_register & (BIT5 | BIT4 | BIT3 | BIT2))
		{
			grw_set_last_error(grw, kGRWReadError);
			dagutil_verbose_level(4, "drb_smbus: command failed!\n");
			return 0;
		}
		
		ret_val = (status_register & 0xff0000) >> 16;
		return ret_val;
	}
	return 0;
}

/* THIS IS NEW!!! - TESTING ONLY!!! */
uint32_t
grw_drb_optics_smbus_busy_wait(GenericReadWritePtr grw, int* countdown)
{
    uintptr_t smbus_address = 0;
    uint32_t status_register = 0;
    uint32_t index = 0;

    if (1 == valid_grw(grw) && countdown)
    {
        index = (0x30000 & grw->mAddress) >> 16;
        smbus_address = card_get_register_address(grw->mCard, DAG_REG_SMB, index + 1);		/* Change was made here, added 1 to index!! */
	//printf ("Warning! %s: %s - SMBUS address (busy_wait): 0x%x\n", __FILE__, __FUNCTION__, smbus_address);

        status_register = card_read_iom(grw->mCard, smbus_address+0x4);

        while (((status_register & BIT1) == 0) && *countdown)
        {
            dagutil_verbose_level(4, "drb_smbus: status_register 0x%x, countdown = %d\n", status_register, *countdown);
            dagutil_verbose_level(5, "drb_smbus: busy waiting\n");
            *countdown = *countdown - 1;
            dagutil_nanosleep(100);
            status_register = card_read_iom(grw->mCard, smbus_address+0x4);
        }
        if (*countdown == 0)
        {
            grw_set_last_error(grw, kGRWWriteError);
            dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
        }

        return status_register;
    }
    return 0;
}


void
grw_drb_optics_smbus_write(GenericReadWritePtr grw, uint32_t val)
{
	if (1 == valid_grw(grw))
	{
		uint8_t device_address = 0;
		uint8_t device_register = 0;
		uintptr_t smbus_address = 0;
		uint32_t register_value = 0;
		uint32_t status_register = 0;
		uint32_t index = 0;
		int countdown = 1000;

		int i, j = 0;
		int smbus_version;
		uint32_t reg_count; 

		device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
		device_register = (uint8_t)(0xff & grw->mAddress);
		index = (0xffff0000 & grw->mAddress) >> 16;
		status_register = grw_drb_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return;
		}
		
		//printf ("Warning! %s: %s - Index: 0x%x\n", __FILE__, __FUNCTION__, index);
		reg_count = card_get_register_count(grw-> mCard, DAG_REG_SMB);
		//printf ("Warning! %s: %s - Card register count: 0x%x\n", __FILE__, __FUNCTION__, reg_count);

		for (i = 0; i < reg_count; i++)
		{
			smbus_version = card_get_register_version(grw->mCard, DAG_REG_SMB, i);
			//printf ("Warning! %s: %s - SMBUS version (read) (i: %d): 0x%x\n", __FILE__, __FUNCTION__, i, smbus_version);

			if (smbus_version == 2)
			{
				if (j == index)
				{
					smbus_address = card_get_register_address(grw->mCard, DAG_REG_SMB, j + 1);
					//printf ("Warning! %s: %s - SMBUS address (read) (j: %d): 0x%x\n", __FILE__, __FUNCTION__, j, smbus_address);
					break;	
				}
				j++;
			}
		}

		//printf ("Warning! %s: %s - SMBUS address (read): 0x%x\n", __FILE__, __FUNCTION__, smbus_address);

		register_value |= device_address;
		register_value |= device_register << 8;
		register_value |= val << 16;
		register_value &= ~BIT24;
		card_write_iom(grw->mCard, smbus_address, register_value);

		/* wait till the write completes */
		countdown = 1000;
		status_register = grw_drb_optics_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return;
		}
		
		/* check if the command succeeded or failed */
		if (status_register & (BIT5 | BIT4 | BIT3 | BIT2))
		{
			grw_set_last_error(grw, kGRWWriteError);
			dagutil_verbose_level(4, "drb_smbus: command failed!\n");
			return;
		}

	}
}

uint32_t
grw_drb_optics_smbus_read(GenericReadWritePtr grw)
{
	if (1 == valid_grw(grw))
	{
		uint8_t device_address = 0;
		uint8_t device_register = 0;
		uintptr_t smbus_address = 0;
		uint32_t ret_val = 0;
		uint32_t register_value = 0;
		uint32_t status_register = 0;
		int index = 0;
		int countdown = 1000;
		int i, j = 0;
		int smbus_version;
		uint32_t reg_count ;

		device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
		device_register = (uint8_t)(0xff & grw->mAddress);
		index = (0xffff0000 & grw->mAddress) >> 16;
		
		//printf ("Warning! %s: %s - Index: 0x%x\n", __FILE__, __FUNCTION__, index);
		reg_count = card_get_register_count(grw-> mCard, DAG_REG_SMB);
		//printf ("Warning! %s: %s - Card register count: 0x%x\n", __FILE__, __FUNCTION__, reg_count);

		for (i = 0; i < reg_count; i++)
		{
			smbus_version = card_get_register_version(grw->mCard, DAG_REG_SMB, i);
			//printf ("Warning! %s: %s - SMBUS version (read) (i: %d): 0x%x\n", __FILE__, __FUNCTION__, i, smbus_version);

			if (smbus_version == 2)
			{
				if (j == index)
				{
					smbus_address = card_get_register_address(grw->mCard, DAG_REG_SMB, j + 1);
					//printf ("Warning! %s: %s - SMBUS address (read) (j: %d): 0x%x\n", __FILE__, __FUNCTION__, j, smbus_address);
					break;	
				}
				j++;
			}
		}

		//printf ("Warning! %s: %s - SMBUS address (read): 0x%x\n", __FILE__, __FUNCTION__, smbus_address);

		status_register = grw_drb_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return 0;
		}
		register_value |= device_address;
		register_value |= device_register << 8;
		register_value |= BIT24;

		//printf ("Warning! %s: %s - Device address (read): 0x%x\n", __FILE__, __FUNCTION__, device_address);
		//printf ("Warning! %s: %s - Device register (read): 0x%x\n", __FILE__, __FUNCTION__, device_register);
		//printf ("Warning! %s: %s - Register value (read): 0x%x\n", __FILE__, __FUNCTION__, register_value);

		card_write_iom(grw->mCard, smbus_address, register_value);
		countdown = 1000;
		status_register = grw_drb_optics_smbus_busy_wait(grw, &countdown);
		if (countdown == 0)
		{
			dagutil_verbose_level(4, "drb_smbus: busy wait timeout!\n");
			return 0;
		}
		
		//if (status_register & BIT3)
		//	printf ("Warning! %s: %s - Status register command error!\n", __FILE__, __FUNCTION__);

		/* check if the command succeeded or failed */
		if (status_register & (BIT5 | BIT4 | BIT3 | BIT2))
		{
			grw_set_last_error(grw, kGRWReadError);
			dagutil_verbose_level(4, "drb_smbus: command failed!\n");
			return 0;
		}
		
		//printf ("Warning! %s: %s - Status register (read): 0x%x\n", __FILE__, __FUNCTION__, status_register);
		ret_val = (status_register & 0xff0000) >> 16;
		return ret_val;
	}
	return 0;
}

/* 
    expected parameters is the address in the grw object to be set up in advance and the value is given to val
    in form: bit0-bit7  - register address, bit8 - reserved (value X), bit9-bit15 - device address 
    bit 16 - bit 17 - XFP component index 0,1,2,3 (port number from 0-3)
*/

void
grw_mini_mac_raw_smbus_write(GenericReadWritePtr grw, uint32_t val)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t mini_mac_index = 0;
        uintptr_t mini_mac_address = 0;
        RawSMBusPtr smbus = NULL;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        mini_mac_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        mini_mac_address = card_get_register_address(grw->mCard, DAG_REG_MINIMAC, mini_mac_index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT15);
        raw_smbus_set_data_mask(smbus, BIT14);
        raw_smbus_set_base_address(smbus, mini_mac_address);
        raw_smbus_write_byte(grw->mCard, smbus, device_address, device_register, (uint8_t)val);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
    }
}


void
grw_xfp_sfp_raw_smbus_write(GenericReadWritePtr grw, uint32_t val)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t port_index = 0;
        uintptr_t xfp_address = 0;
        RawSMBusPtr smbus = NULL;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        port_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        xfp_address = card_get_register_address(grw->mCard, DAG_REG_XFP, port_index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT0);
        raw_smbus_set_data_mask(smbus, BIT1);
        raw_smbus_set_base_address(smbus, xfp_address);
        raw_smbus_write_byte(grw->mCard, smbus, device_address, device_register, (uint8_t)val);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
    }
}


uintptr_t 
grw_get_address(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {   
        return grw->mAddress;
    }   
    return 0;
}

DagCardPtr 
grw_get_card(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        return grw->mCard;
    }
    return NULL;
}
/* Read from a register on PMC5381 */
uint32_t
grw_dag43s_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        GenericReadWritePtr grw_pm5381 = NULL;
        int rd_val = 0;
        uintptr_t pm5381_address = 0;
        pm5381_address = card_get_register_address(grw->mCard, DAG_REG_PM5381, 0);
        pm5381_address = (uintptr_t)(card_get_iom_address(grw->mCard) + pm5381_address);
        pm5381_address += (grw->mAddress << 2);
        grw_pm5381 = grw_init(grw->mCard, pm5381_address, grw_iom_read, grw_iom_write);
        rd_val = grw_read(grw_pm5381);
        free(grw_pm5381);
        return (rd_val & 0xffff);
    }
    return 0;
}

/* Write a register on the PMC5381*/
void
grw_dag43s_write(GenericReadWritePtr grw, uint32_t val)
{
    if(1 == valid_grw(grw))
    {
        GenericReadWritePtr grw_pm5381 = NULL;
        uintptr_t pm5381_address = 0;
        pm5381_address  = card_get_register_address(grw->mCard, DAG_REG_PM5381, 0);
        pm5381_address = (uintptr_t)(card_get_iom_address(grw->mCard) + pm5381_address);
        pm5381_address += (grw->mAddress << 2);
        grw_pm5381 = grw_init(grw->mCard, pm5381_address, grw_iom_read, grw_iom_write);
        grw_write(grw_pm5381, val);
        free(grw_pm5381);
    }
}
/* Read from a register on S19205 */
uint32_t
grw_dag62se_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        GenericReadWritePtr grw_s19205 = NULL;
        int rd_val = 0;
        uintptr_t s19205_address = 0;
        s19205_address = card_get_register_address(grw->mCard, DAG_REG_S19205, 0);
        s19205_address = (uintptr_t)(card_get_iom_address(grw->mCard) + s19205_address);
        s19205_address += (grw->mAddress << 2);
        grw_s19205 = grw_init(grw->mCard, s19205_address, grw_iom_read, grw_iom_write);
        rd_val = grw_read(grw_s19205);
        free(grw_s19205);
        return (rd_val & 0xffff);
    }
    return 0;
}

/* Write a register on S19205*/
void
grw_dag62se_write(GenericReadWritePtr grw, uint32_t val)
{
    if(1 == valid_grw(grw))
    {
        GenericReadWritePtr grw_s19205 = NULL;
        uintptr_t s19205_address = 0;
        s19205_address  = card_get_register_address(grw->mCard, DAG_REG_S19205, 0);
        s19205_address = (uintptr_t)(card_get_iom_address(grw->mCard) + s19205_address);
        s19205_address += (grw->mAddress << 2);
        grw_s19205 = grw_init(grw->mCard, s19205_address, grw_iom_read, grw_iom_write);
        grw_write(grw_s19205, val);
        free(grw_s19205);
    }
}


grw_error_t
grw_get_last_error(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        grw_error_t error;

        error = grw->mError;
        grw->mError = kGRWNoError;
        grw->mChecksum = compute_grw_checksum(grw);
        return error;
    }
    return kGRWError;
}

/**
 * The only thing read from the LM63 is the local temperature settings: LM63_TEMP_LOC
 */

void
grw_lm63_smb_write(GenericReadWritePtr grw, uint32_t val)
{
    if(1 == valid_grw(grw))
    {
        uintptr_t smb_base = grw->mAddress;/* Get SMB Base */
        
        int safety = SAFETY_ITERATIONS;
        volatile uint32_t reg_value;
        uint32_t value = ((uint32_t)val << 16) | (uint32_t)(LM63_TEMP_LOC << 8) | (uint32_t)LM63;
        card_write_iom(grw->mCard, smb_base + LM63_SMB_Ctrl, value);
        reg_value = card_read_iom(grw->mCard, smb_base + LM63_SMB_Read) & 0x2;
        while ((0 != safety) && (0 == reg_value))
        {
            dagutil_nanosleep(SAFETY_NANOSECONDS);
            safety--;
            reg_value = card_read_iom(grw->mCard, smb_base + LM63_SMB_Read) & 0x2;
        }
        if(0 != reg_value)
        {
            dagutil_warning("exceeded safety counter while writing to SMBus\n");           
        }
    }
}

uint32_t
grw_lm63_smb_read(GenericReadWritePtr grw)
{
    int timeout_counter = 0;
    if(1 == valid_grw(grw))
    {
        /* Try 10 times to read before timimg out */
        while (timeout_counter < 10)
        {
            int safety = SAFETY_ITERATIONS;
            volatile uint32_t readval;
            uintptr_t smb_base = grw->mAddress;
            uint32_t val = (uint32_t)(LM63_TEMP_LOC << 8) | (uint32_t)(LM63 | 0x01000000);
            card_write_iom(grw->mCard, smb_base + LM63_SMB_Ctrl, val);
            usleep(400);
            readval = card_read_iom(grw->mCard, smb_base + LM63_SMB_Read);
            while((0 != safety) && ( 0 == (readval & BIT24)))
            {
                dagutil_nanosleep(SAFETY_NANOSECONDS);
                safety--;
                readval = card_read_iom(grw->mCard, smb_base + LM63_SMB_Read);
            }
            if(readval & BIT24)
            {
                return (readval >> 16) & 0xff;
            }
            
            timeout_counter++;
        }
        //dagutil_warning("exceeded safety counter while writing to SMBus\n");
        //return 0;
    }
	dagutil_warning("exceeded safety counter while reading from SMBus\n");
	return 0;
}

/* Used to set the hash value on the the packet capture module for use with the copro */
void
grw_set_capture_hash(GenericReadWritePtr grw, uint32_t hash)
{
    if (1 == valid_grw(grw))
    {
        uint32_t val = hash;
        uint32_t reg_val = 0;
        val |= BIT20;
        /* wait for the hash table to be ready*/
        reg_val = *(volatile uint32_t*)grw->mAddress;
        while ((reg_val & BIT28) == 0)
        {
            reg_val = *(volatile uint32_t*)grw->mAddress;
            dagutil_verbose_level(4, "Waiting for hash table ready bit... 0x%08x\n", reg_val);
        }
        *(volatile uint32_t*)grw->mAddress = val;
        dagutil_verbose_level(5, "grw_set_capture_hash: addr = 0x%8x, val = %d (0x%08x)\n", (uint32_t)grw->mAddress, val, val);
    }
}

void
grw_unset_capture_hash(GenericReadWritePtr grw, uint32_t hash)
{
    if (1 == valid_grw(grw))
    {
        uint32_t val = hash;
        uint32_t reg_val = 0;
        val |= BIT21;
        /* wait for the hash table to be ready*/
        reg_val = *(volatile uint32_t*)grw->mAddress;
        while ((reg_val & BIT28) == 0)
        {
            reg_val = *(volatile uint32_t*)grw->mAddress;
            dagutil_verbose_level(4, "Waiting for hash table ready bit... 0x%08x\n", reg_val);
        }
        *(volatile uint32_t*)grw->mAddress = val;
        dagutil_verbose_level(4, "grw_unset_capture_hash: addr = 0x%8x, val = %d (0x%08x)\n", (uint32_t)grw->mAddress, val, val);
    }
}

/* make sure the address is specified using the 8 bit address mode (see datasheet ds3181-ds3184 from Maxim)*/
uint32_t
grw_dag37d_read(GenericReadWritePtr grw)
{
    /* convert to 32 bit address */
    if (1 == valid_grw(grw))
    {
        GenericReadWritePtr grw_ds3182 = NULL;
        int rd_val = 0;
        uintptr_t ds3182_address = 0;
        ds3182_address = card_get_register_address(grw->mCard, DAG_REG_RAW_MAXDS3182, 0);
        ds3182_address = (uintptr_t)(card_get_iom_address(grw->mCard) + ds3182_address);
        ds3182_address += (grw->mAddress*4);
        usleep(100);
        grw_ds3182 = grw_init(grw->mCard, ds3182_address, grw_iom_read, grw_iom_write);
        rd_val = grw_read(grw_ds3182);
        free(grw_ds3182);
        return (rd_val & 0xffff);
    }
    return 0;    
}

/* make sure the address is specified using the 8 bit address mode (see datasheet ds3181-ds3184 from Maxim)*/
void
grw_dag37d_write(GenericReadWritePtr grw, uint32_t val)
{
	/* convert to 32 bit address */
    if(1 == valid_grw(grw))
    {
        GenericReadWritePtr grw_ds3182 = NULL;
        uintptr_t ds3182_address = 0;
        ds3182_address  = card_get_register_address(grw->mCard, DAG_REG_RAW_MAXDS3182, 0);
        ds3182_address = (uintptr_t)(card_get_iom_address(grw->mCard) + ds3182_address);
        ds3182_address += (grw->mAddress*4);
        usleep(100);
        grw_ds3182 = grw_init(grw->mCard, ds3182_address, grw_iom_read, grw_iom_write);
        grw_write(grw_ds3182, val);
        free(grw_ds3182);
    }    
}

uint32_t
grw_vsc8476_mdio_read(GenericReadWritePtr grw)
{
    if(1 == valid_grw(grw))
    {
        uint32_t val = 0;
        uintptr_t register_address = 0;
        int reg_ver = -1;
        int i = 0;
        uint32_t mmd = 0;
        uint32_t reg = 0;
        do
        {
           register_address = card_get_register_address(grw->mCard, DAG_REG_VSC8476, i);
           reg_ver = card_get_register_version(grw->mCard, DAG_REG_VSC8476, i);
           i++;
        }while(reg_ver != 0);
               
        /* 
         * grw->mAddress is of format:
         *  mmd (20:16) and reg (15:0)
         */
        mmd = (grw->mAddress >> 16) & 0x1f;
        reg = grw->mAddress & 0xffff;
        

        /* Write mmd and reg to iom first */
        val = 0;
        val |= grw->mAddress;
        card_write_iom(grw->mCard, register_address, val);

        /*Sleep for 200 microseconds */
        usleep(200);

        /* Notify the mmd that we want to read */
        /* Setting BIT31 and BIT30 */
        val = 0;
        val |= (mmd <<16);
        val |= (BIT31|BIT30);
        card_write_iom(grw->mCard, register_address, val);

        
        /* Sleeep in bursts of 200 microseconds until BIT31 is set */
        /* Data is then valid */
        i = 0;
        do
        {
            usleep(200);
            val = card_read_iom(grw->mCard, register_address);
            i++;
            if(i >=5)
                return 0;
        } while((val & BIT31) != BIT31);

        return (val & 0xffff);
    }
    return 0;
}

void
grw_vsc8476_mdio_write(GenericReadWritePtr grw, uint32_t value)
{
    if(1 == valid_grw(grw))
    {
        uint32_t val = 0;
        uintptr_t register_address = 0;
        int reg_ver = -1;
        int i = 0;
        uint32_t mmd = 0;
        uint32_t reg = 0;
        do
        {
            register_address = card_get_register_address(grw->mCard, DAG_REG_VSC8476, i);
            reg_ver = card_get_register_version(grw->mCard, DAG_REG_VSC8476, i);
            i++;
        }while(reg_ver != 0);

        /* 
         * grw->mAddress is of format:
         *  mmd (20:16) and reg (15:0)
         */
        mmd = (grw->mAddress >> 16) & 0x1f;
        reg = grw->mAddress & 0xffff;
        

        /* Write mmd and reg to iom first */
        val = 0;
        val |= grw->mAddress;
        card_write_iom(grw->mCard, register_address, val);

        /*Sleep for 200 microseconds */
        usleep(200);

        /* Write the value to iom */
        val = 0;
        val |= (mmd <<16);
        val |= (value & 0xffff);
        val |= BIT30;

        card_write_iom(grw->mCard, register_address, val);
        
        usleep(200);
    }
}
/*The different function from 8476 mdio because 9.1 which has 2 8486 devices connected to the bus.need to address 
mdio addressing BIT 21- BIT 25 selects the device.*/
uint32_t
grw_vsc8486_mdio_read(GenericReadWritePtr grw)
{
    if(1 == valid_grw(grw))
    {
        uint32_t val = 0;
        uintptr_t register_address = 0;
        int reg_ver = -1;
        int i = 0;
        uint32_t mmd = 0;
        uint32_t reg = 0;
	uint32_t dev_address = 0;
        do
        {
           register_address = card_get_register_address(grw->mCard, DAG_REG_VSC8486, i);
           reg_ver = card_get_register_version(grw->mCard, DAG_REG_VSC8486, i);
           i++;
        }while(reg_ver != 0);
               
        /* 
         * grw->mAddress is of format:
         *  mmd (20:16) and reg (15:0)
         */
	dev_address = (grw->mAddress >> 21) & 0x1f;
        mmd = (grw->mAddress >> 16) & 0x1f;
        reg = grw->mAddress & 0xffff;
        

        /* Write mmd and reg to iom first */
        val = 0;
        val |= grw->mAddress;
        card_write_iom(grw->mCard, register_address, val);

        /*Sleep for 200 microseconds */
        usleep(200);

        /* Notify the mmd that we want to read */
        /* Setting BIT31 and BIT30 */
        val = 0;
	val |= (dev_address<<21);
        val |= (mmd <<16);
        val |= (BIT31|BIT30);
        card_write_iom(grw->mCard, register_address, val);

        
        /* Sleeep in bursts of 200 microseconds until BIT31 is set */
        /* Data is then valid */
        i = 0;
        do
        {
            usleep(200);
            val = card_read_iom(grw->mCard, register_address);
            i++;
            if(i >=5)
                return 0;
        } while((val & BIT31) != BIT31);

        return (val & 0xffff);
    }
    return 0;
}

void
grw_vsc8486_mdio_write(GenericReadWritePtr grw, uint32_t value)
{
    if(1 == valid_grw(grw))
    {
        uint32_t val = 0;
        uintptr_t register_address = 0;
        int reg_ver = -1;
        int i = 0;
        uint32_t mmd = 0;
        uint32_t reg = 0;
	uint32_t device_address = 0;
        do
        {
            register_address = card_get_register_address(grw->mCard, DAG_REG_VSC8486, i);
            reg_ver = card_get_register_version(grw->mCard, DAG_REG_VSC8486, i);
            i++;
        }while(reg_ver != 0);

        /* 
         * grw->mAddress is of format:
         *  mmd (20:16) and reg (15:0)
         */
	device_address = (grw->mAddress >> 21) & 0x1f;
        mmd = (grw->mAddress >> 16) & 0x1f;
        reg = grw->mAddress & 0xffff;
        

        /* Write mmd and reg to iom first */
        val = 0;
        val |= grw->mAddress;
        card_write_iom(grw->mCard, register_address, val);

        /*Sleep for 200 microseconds */
        usleep(200);

        /* Write the value to iom */
        val = 0;
	val |= (device_address << 21);
        val |= (mmd <<16);
        val |= (value & 0xffff);
        val |= BIT30;

        card_write_iom(grw->mCard, register_address, val);
        
        usleep(200);
    }
}

uint32_t
grw_hlb_component_get_min_range(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        AttributePtr attr;
        ComponentPtr hlb;
        ComponentPtr root;
        hlb_range_t* hlb_range = NULL;
        uint32_t stream_index = 0;

        root = card_get_root_component(grw->mCard);
        hlb = component_get_subcomponent(root, kComponentHlb, 0);
        stream_index = grw->mAddress;
        attr = component_get_attribute(hlb, kStructAttributeHlbRange);
        hlb_range = (hlb_range_t*)attribute_get_value(attr);
        if (hlb_range)
        {
            return hlb_range->stream_range[stream_index].min;
        }
    }
    return 0;
}

void
grw_hlb_component_set_min_range(GenericReadWritePtr grw, uint32_t value)
{
    if (1 == valid_grw(grw))
    {
        ComponentPtr hlb;
        ComponentPtr root;
        AttributePtr attr;
        hlb_range_t* hlb_range = NULL;
        uint32_t stream_index = 0;

        root = card_get_root_component(grw->mCard);
        hlb = component_get_subcomponent(root, kComponentHlb, 0);
        stream_index = grw->mAddress;
        attr = component_get_attribute(hlb, kStructAttributeHlbRange);
        hlb_range = (hlb_range_t*)attribute_get_value(attr);
        if (hlb_range)
        {
            hlb_range->stream_range[stream_index].min = value;
            attribute_set_value(attr, hlb_range, 0);
        }
    }
}

uint32_t
grw_hlb_component_get_max_range(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        ComponentPtr hlb;
        ComponentPtr root;
        AttributePtr attr;
        hlb_range_t* hlb_range = NULL;
        uint32_t stream_index = 0;

        root = card_get_root_component(grw->mCard);
        hlb = component_get_subcomponent(root, kComponentHlb, 0);
        stream_index = grw->mAddress;
        attr = component_get_attribute(hlb, kStructAttributeHlbRange);
        hlb_range = (hlb_range_t*)attribute_get_value(attr);
        if (hlb_range)
        {
            return hlb_range->stream_range[stream_index].max;
        }
    }
    return 0;
}

void
grw_hlb_component_set_max_range(GenericReadWritePtr grw, uint32_t value)
{
    if (1 == valid_grw(grw))
    {
        ComponentPtr hlb;
        ComponentPtr root;
        AttributePtr attr;
        hlb_range_t* hlb_range = NULL;
        uint32_t stream_index = 0;

        root = card_get_root_component(grw->mCard);
        hlb = component_get_subcomponent(root, kComponentHlb, 0);
        stream_index = grw->mAddress;
        attr = component_get_attribute(hlb, kStructAttributeHlbRange);
        hlb_range = (hlb_range_t*)attribute_get_value(attr);
        if (hlb_range)
        {
            hlb_range->stream_range[stream_index].max = value;
            attribute_set_value(attr, hlb_range, 0);
        }
    }
}

uint32_t
grw_vsc8479_raw_smbus_read(GenericReadWritePtr grw)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t vsc_index = 0;
        uintptr_t vsc_address = 0;
        RawSMBusPtr smbus = NULL;
        uint32_t ret_val = 0;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        vsc_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        vsc_address = card_get_register_address(grw->mCard, DAG_REG_VSC8479, vsc_index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT25);
        raw_smbus_set_data_mask(smbus, BIT24);
        raw_smbus_set_base_address(smbus, vsc_address);
        ret_val = raw_smbus_read_byte(grw->mCard, smbus, device_address, device_register);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
        return ret_val;
    }
    return 0;
}

void
grw_vsc8479_raw_smbus_write(GenericReadWritePtr grw, uint32_t val)
{
    if (1 == valid_grw(grw))
    {
        uint8_t device_address = 0;
        uint8_t device_register = 0;
        uint8_t vsc_index = 0;
        uintptr_t vsc_address = 0;
        RawSMBusPtr smbus = NULL;
        
        device_address = (uint8_t)((0xff00 & grw->mAddress) >> 8);
        device_register = (uint8_t)(0xff & grw->mAddress);
        vsc_index = (uint8_t)((0x30000 & grw->mAddress) >> 16);
        vsc_address = card_get_register_address(grw->mCard, DAG_REG_VSC8479, vsc_index);
        smbus = raw_smbus_init();
        raw_smbus_set_clock_mask(smbus, BIT25);
        raw_smbus_set_data_mask(smbus, BIT24);
        raw_smbus_set_base_address(smbus, vsc_address);
        raw_smbus_write_byte(grw->mCard, smbus, device_address, device_register, (uint8_t)val);
        if (raw_smbus_get_last_error(smbus) != kSMBusNoError)
        {
            grw_set_last_error(grw, kGRWReadError);
        }
        raw_smbus_dispose(smbus);
    }
}

