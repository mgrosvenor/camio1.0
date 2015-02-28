/*
 * Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: raw_smbus_module.h,v 1.4 2006/09/13 04:17:32 sashan Exp $
 */

#ifndef RAW_SMBUS_MODULE_H
#define RAW_SMBUS_MODULE_H


#include "dag_platform.h"
#include "../card.h"

/* Forward declarations. */
typedef struct RawSMBusObject RawSMBusObject;
typedef RawSMBusObject* RawSMBusPtr;

typedef enum
{
    kSMBusNoError,
    kSMBusNoACK
} raw_smbus_error_t;

RawSMBusPtr raw_smbus_init();
void raw_smbus_set_base_address(RawSMBusPtr smbus, uintptr_t address);
void raw_smbus_dispose(RawSMBusPtr smbus);
uint8_t raw_smbus_read_byte(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register);
uint16_t raw_smbus_read_word(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register);
void raw_smbus_write_byte(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register, uint8_t value);
void raw_smbus_write_word(DagCardPtr card, RawSMBusPtr smbus, uint8_t device_address, uint8_t device_register, uint16_t value);
void raw_smbus_set_clock_mask(RawSMBusPtr smbus, uint32_t mask);
void raw_smbus_set_data_mask(RawSMBusPtr smbus, uint32_t mask);
raw_smbus_error_t raw_smbus_get_last_error(RawSMBusPtr smbus);

#endif
