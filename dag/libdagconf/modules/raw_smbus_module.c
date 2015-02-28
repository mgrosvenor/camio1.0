/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: raw_smbus_module.c,v 1.15 2007/11/13 21:58:28 vladimir Exp $
 */

/* 
This module is for communicating with components on a card via a SMBus (System Management Bus).
See http://www.smbus.org/specs for protocol specification. This is exposed by a firmware interface (RAW_SMBUS)
however we don't make this a component because it is unlikely that a user of the Config API will need
direct access to the SMBus.
*/

#include "dagutil.h"
#include "dag_platform.h"
#include "../include/modules/raw_smbus_module.h"
#include "../include/card.h"


struct RawSMBusObject
{
    uintptr_t mBase;
    uint32_t mClock;
    uint32_t mData;
    raw_smbus_error_t mError;
};

RawSMBusPtr
raw_smbus_init()
{
    RawSMBusPtr result = NULL;
    result = (RawSMBusPtr)malloc(sizeof(*result));
    memset(result, 0, sizeof(*result));
    return result;
}

void
raw_smbus_set_clock_mask(RawSMBusPtr smbus, uint32_t mask)
{
    smbus->mClock = mask;
}

void
raw_smbus_set_data_mask(RawSMBusPtr smbus, uint32_t mask)
{
    smbus->mData = mask;
}

void
raw_smbus_clock(DagCardPtr card, RawSMBusPtr smbus, int value)
{
    uint32_t register_value = 0;

//    dagutil_nanosleep(30000);
    dagutil_nanosleep(15000);
    register_value = card_read_iom(card, smbus->mBase);
    if (value == 1)
    {
        register_value |= smbus->mClock;
    }
    else
    {
        register_value &= ~smbus->mClock;
    }
    card_write_iom(card, smbus->mBase, register_value);
    dagutil_nanosleep(15000);
}

void
raw_smbus_data(DagCardPtr card, RawSMBusPtr smbus, int value)
{
    uint32_t register_value = 0;

//    dagutil_nanosleep(30000);
    dagutil_nanosleep(15000);
    register_value = card_read_iom(card, smbus->mBase);
    if (value == 1)
    {
        register_value |= smbus->mData;
    }
    else
    {
        register_value &= ~smbus->mData;
    }
    card_write_iom(card, smbus->mBase, register_value);
//    dagutil_nanosleep(15000);
}

void
raw_smbus_dispose(RawSMBusPtr smbus)
{
    free(smbus);
}

void
raw_smbus_set_base_address(RawSMBusPtr smbus, uintptr_t address)
{
    smbus->mBase = address;
}

static void
raw_smbus_write_data_line(DagCardPtr card, RawSMBusPtr smbus, uint8_t byte)
{
    int i;
    uint8_t bit = 0;

    for (i = 7; i >= 0; i--)
    {
        bit = (byte >> i) & 0x1;
        raw_smbus_data(card, smbus, bit);
        dagutil_verbose_level(3, " smbbus: Write bit %d\n", bit);
        /* raise clock so that the device can sample it */
        raw_smbus_clock(card, smbus, 1);

        /* lower clock to release it */
        raw_smbus_clock(card, smbus, 0);
	
    }
}
static int
raw_smbus_ack(DagCardPtr card, RawSMBusPtr smbus)
{
    uint32_t ack = 0;

    dagutil_verbose_level(2, " smbbus: ACK\n");
    raw_smbus_clock(card, smbus, 1);
    raw_smbus_data(card, smbus, 0);
    raw_smbus_clock(card, smbus, 0);
    return ack;
}
static uint8_t
raw_smbus_read_data_line(DagCardPtr card, RawSMBusPtr smbus)
{
    uint32_t register_value = 0;
    uint32_t return_value = 0;
    int i;

    for (i = 7; i >= 0; i--)
    {
        raw_smbus_data(card, smbus, 1);
        raw_smbus_clock(card, smbus, 1);
        register_value = card_read_iom(card, smbus->mBase);
        dagutil_verbose_level(3, " smbbus: Read bit 0x%x\n", (register_value & smbus->mData));
        return_value |= ((register_value & smbus->mData) == smbus->mData) << i;
        raw_smbus_clock(card, smbus, 0);
    }
    dagutil_verbose_level(2, " smmbus: Read byte = 0x%8x (%c)\n", (uint8_t)return_value, (uint8_t)return_value);
    return (uint8_t)return_value;
}



static int
raw_smbus_check_ack(DagCardPtr card, RawSMBusPtr smbus)
{
    uint32_t ack = 0;

    dagutil_verbose_level(2, " smbbus: Check for ACK\n");
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 1);
    ack = card_read_iom(card, smbus->mBase);
    ack = ack & smbus->mData;
    if (ack)
    {
        dagutil_verbose_level(1, "No ACK!\n");
        smbus->mError = kSMBusNoACK;
    }
    /* lower clock */
    raw_smbus_clock(card, smbus, 0);
    return ack;
}

void
raw_smbus_write_byte(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register, uint8_t value)
{
    uint32_t register_value = 0;
    uint8_t byte = 0;

    register_value = card_read_iom(card, smbus->mBase);
    dagutil_verbose_level(2, " smbbus: Setting IDLE state\n");
    /* Put the bus into an idle state. An idle state is defined as clock = 1 and data = 1.*/
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 1);
    /* A START condition is a transition from 1 to 0 on the data line while the clock line is 1 */
    dagutil_verbose_level(2, " smbbus: START condition\n");
    raw_smbus_data(card, smbus, 0);
    raw_smbus_clock(card, smbus, 0);

    /* Write the slave address and the WRITE bit. Set the write bit to 0 to enforce a write command*/
    dagutil_verbose_level(2, " smbbus: Slave Address and Write bit\n");
    byte = device_address & 0xfe;
    raw_smbus_write_data_line(card, smbus, byte);
    raw_smbus_check_ack(card, smbus);

    /* Write the address of the register on the device */
    dagutil_verbose_level(2, " smbbus: Command Code (register address on the device)\n");
    byte = device_register;
    raw_smbus_write_data_line(card, smbus, byte);
    raw_smbus_check_ack(card, smbus);
    
    /* Send the data byte */
    dagutil_verbose_level(2, " smbbus: Sending data byte.");
    raw_smbus_write_data_line(card, smbus, value);
    raw_smbus_check_ack(card, smbus);

    /* write a STOP condition */
    dagutil_verbose_level(2, " smbbus:  STOP condition\n");
    raw_smbus_clock(card, smbus, 1);
    raw_smbus_data(card, smbus, 1);
}

/*
Read a byte over the SMB Bus. The protocol for reading a byte is fully described
at http://www.smbus.org/specs. */
uint8_t
raw_smbus_read_byte(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register)
{
    uint32_t register_value = 0;
    uint8_t return_value = 0;
    uint8_t byte = 0;
    
    register_value = card_read_iom(card, smbus->mBase);
    dagutil_verbose_level(2, " smbbus: Setting IDLE state\n");
    /* Put the bus into an idle state. An idle state is defined as clock = 1 and data = 1.*/
//Note for VSC3312 i had to comment out the idle state foir read , but can be just a question if needed or not and some testing
    raw_smbus_clock(card, smbus, 0);
    
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 1);
    usleep(100);
    /* A START condition is a transition from 1 to 0 on the data line while the clock line is 1 */
    dagutil_verbose_level(2, " smbbus: START condition\n");
    raw_smbus_data(card, smbus, 0);
    raw_smbus_clock(card, smbus, 0);


    /* Write the slave address and the WRITE bit. Set the write bit to 0 to enforce a write command*/
    dagutil_verbose_level(2, " smbbus: Slave Address and ReadWrite bit\n");
    byte = device_address & 0xfe;
    raw_smbus_write_data_line(card, smbus, byte);
    dagutil_nanosleep(15000);
    raw_smbus_check_ack(card, smbus);

    dagutil_nanosleep(30000);

    /* Write the address of the register on the device */
    dagutil_verbose_level(2, " smbbus: Command Code (register address on the device)\n");
    byte = device_register;
    raw_smbus_write_data_line(card, smbus, byte);
    dagutil_nanosleep(15000);
    raw_smbus_check_ack(card, smbus);

    dagutil_verbose_level(2, " smbbus: START condition\n");
    /*transmit a start condition */
    /* put data high */
    raw_smbus_data(card, smbus, 1);
    /* put clock high */
    raw_smbus_clock(card, smbus, 1);

    register_value = card_read_iom(card, smbus->mBase);
    /* put data low */
    raw_smbus_data(card, smbus, 0);
    /* set clock low */
    raw_smbus_clock(card, smbus, 0);

    dagutil_verbose_level(2, " smbbus: Slave Address and Read bit\n");
    /* Write the slave address. Set last bit to 1 to indicate Read */
    byte = device_address | 1;
    raw_smbus_write_data_line(card, smbus, byte);
    raw_smbus_check_ack(card, smbus);

//    dagutil_nanosleep(15000);

    return_value = raw_smbus_read_data_line(card, smbus);
    dagutil_verbose_level(2, " smbbus:  ACK/NACK on read after get the data \n");
    /* write a NACK to the slave */
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 1);
    raw_smbus_clock(card, smbus, 0);

    /* write a STOP condition */
    dagutil_verbose_level(2, " smbbus:  STOP condition\n");
    raw_smbus_clock(card, smbus, 1);

    raw_smbus_data(card, smbus, 1);
    return return_value;
}
/*
Read a byte over the SMB Bus. The protocol for reading a byte is fully described
at http://www.smbus.org/specs. */
uint16_t
raw_smbus_read_word(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register)
{
    uint32_t register_value = 0;
    uint16_t return_value = 0;
    uint8_t byte = 0;
    
    register_value = card_read_iom(card, smbus->mBase);
    dagutil_verbose_level(2, " smbbus: Setting IDLE state\n");
    /* Put the bus into an idle state. An idle state is defined as clock = 1 and data = 1.*/
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 1);
    usleep(100);
    /* write a STOP condition */
    raw_smbus_clock(card, smbus, 0);
    raw_smbus_data(card, smbus, 0);
    dagutil_verbose_level(2, " smbbus:  STOP condition\n");
    raw_smbus_clock(card, smbus, 1);

    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card,smbus, 0);

    /* Put the bus into an idle state. An idle state is defined as clock = 1 and data = 1.*/
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 1);
    usleep(100);    
    
    /* A START condition is a transition from 1 to 0 on the data line while the clock line is 1 */
    dagutil_verbose_level(2, " smbbus: START condition\n");
    raw_smbus_data(card, smbus, 0);
    raw_smbus_clock(card, smbus, 0);


    /* Write the slave address and the WRITE bit. Set the write bit to 0 to enforce a write command*/
    dagutil_verbose_level(2, " smbbus: Slave Address and Write bit\n");
    byte = device_address & 0xfe;
//     byte = device_address | 0x01;
    raw_smbus_write_data_line(card, smbus, byte);
    if(raw_smbus_check_ack(card, smbus))
    {
        dagutil_verbose_level(2, " smbbus: START condition\n");
        /*transmit a start condition */
        /* put data high */
        raw_smbus_data(card, smbus, 1);
        /* put clock high */
        raw_smbus_clock(card, smbus, 1);

        register_value = card_read_iom(card, smbus->mBase);
        /* put data low */
        raw_smbus_data(card, smbus, 0);
        /* set clock low */
        raw_smbus_clock(card, smbus, 0);


        /* Write the slave address and the WRITE bit. Set the write bit to 0 to enforce a write command*/
        dagutil_verbose_level(2, " smbbus: Slave Address and Write bit\n");
        byte = device_address & 0xfe;
        raw_smbus_write_data_line(card, smbus, byte);        
        raw_smbus_check_ack(card, smbus);
            
    }

    /* Write the address of the register on the device */
    dagutil_verbose_level(2, " smbbus: Command Code (register address on the device)\n");
    byte = device_register;
    raw_smbus_write_data_line(card, smbus, byte);
    raw_smbus_check_ack(card, smbus);

    dagutil_verbose_level(2, " smbbus: START condition\n");
    /*transmit a start condition */
    /* put data high */
    raw_smbus_data(card, smbus, 1);
    /* put clock high */
    raw_smbus_clock(card, smbus, 1);

    register_value = card_read_iom(card, smbus->mBase);
    /* put data low */
    raw_smbus_data(card, smbus, 0);
    /* set clock low */
    raw_smbus_clock(card, smbus, 0);

    dagutil_verbose_level(2, " smbbus: Slave Address and Read bit\n");
    /* Write the slave address. Set last bit to 1 to indicate Read */
    byte = device_address | 1;
    raw_smbus_write_data_line(card, smbus, byte);
    raw_smbus_check_ack(card, smbus);

    return_value = raw_smbus_read_data_line(card, smbus);
    raw_smbus_ack(card, smbus);

    byte = raw_smbus_read_data_line(card, smbus);
    return_value |= (byte << 8);
    
    dagutil_verbose_level(2, " smbbus:  NACK\n");
    /* write a NACK to the slave */
    raw_smbus_clock(card, smbus, 1);
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 0);

    /* write a STOP condition */
    raw_smbus_data(card, smbus, 0);
    dagutil_verbose_level(2, " smbbus:  STOP condition\n");
    raw_smbus_clock(card, smbus, 1);

    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card,smbus, 0);
    return return_value;
}

raw_smbus_error_t
raw_smbus_get_last_error(RawSMBusPtr smbus)
{
    raw_smbus_error_t error;
    
    error = smbus->mError;
    smbus->mError = kSMBusNoError;
    return error;
}

void
raw_smbus_write_word(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register, uint16_t value)
{
    uint32_t register_value = 0;
    uint8_t byte = 0;

    register_value = card_read_iom(card, smbus->mBase);
    dagutil_verbose_level(2, " smbbus: Setting IDLE state\n");
    /* Put the bus into an idle state. An idle state is defined as clock = 1 and data = 1.*/
    raw_smbus_data(card, smbus, 1);
    raw_smbus_clock(card, smbus, 1);
    /* A START condition is a transition from 1 to 0 on the data line while the clock line is 1 */
    dagutil_verbose_level(2, " smbbus: START condition\n");
    raw_smbus_data(card, smbus, 0);
    raw_smbus_clock(card, smbus, 0);

    /* Write the slave address and the WRITE bit. Set the write bit to 0 to enforce a write command*/
    dagutil_verbose_level(2, " smbbus: Slave Address and Write bit\n");
    byte = device_address & 0xfe;
    raw_smbus_write_data_line(card, smbus, byte);
    raw_smbus_check_ack(card, smbus);

    /* Write the address of the register on the device */
    dagutil_verbose_level(2, " smbbus: Command Code (register address on the device)\n");
    byte = device_register;
    raw_smbus_write_data_line(card, smbus, byte);
    raw_smbus_check_ack(card, smbus);
    
    /* Send the data byte */
    dagutil_verbose_level(2, " smbbus: Sending data byte LSB.");
    raw_smbus_write_data_line(card, smbus, (value & 0xff));
    raw_smbus_check_ack(card, smbus);

    dagutil_verbose_level(2, " smbbus: Sending data byte MSB.");
    raw_smbus_write_data_line(card, smbus, ((value >> 8) & 0xff));
    raw_smbus_check_ack(card, smbus);    

    /* write a STOP condition */
    dagutil_verbose_level(2, " smbbus:  STOP condition\n");
    raw_smbus_clock(card, smbus, 1);
    raw_smbus_data(card, smbus, 1);
}
