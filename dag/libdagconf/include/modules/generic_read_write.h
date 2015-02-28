/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef GENERIC_READ_WRITE_H
#define GENERIC_READ_WRITE_H

#include "dag_platform.h"
#include "../card.h"
#include "../attribute_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/* 
The generic_read_write module defines an object to handle reading and writing 32 bit values using different mechanisms
The most comonly used mechanism is the one over the DRB(Dag Register Bus).
Another is over the SMBus where the reading and writing is more involved.
Regardless this object is designed for use with attributes so they know how to read/write
themselves to the card without having to worry about the mechanics of it.
*/

typedef enum
{
    kGRWNoError,
    kGRWReadError,
    kGRWWriteError,
    kGRWError
} grw_error_t;

/* Sometimes to enable a feature in the framer means setting the bit to 0. 
 * This allows the attribute to know which operation to use when enabling/disabling 
 * a feature. In most cases enabling a feature means setting the relevant bit to 1, hence the 
 * default setting is kGRWSetBit.
 */
typedef enum
{
    kGRWInvalidOperation,
    kGRWClearBit,
    kGRWSetBit,
} grw_on_operation_t;

/* Forward declarations. */
typedef struct GenericReadWriteObject GenericReadWriteObject;
typedef GenericReadWriteObject* GenericReadWritePtr;

/* Method signatures. */
typedef uint32_t (*GenericReadRoutine)(GenericReadWritePtr grw);
typedef void (*GenericWriteRoutine)(GenericReadWritePtr grw, uint32_t value);

void grw_set_address(GenericReadWritePtr grw, uintptr_t address);
uintptr_t grw_get_address(GenericReadWritePtr grw);
void grw_set_read_routine(GenericReadWritePtr grw, GenericReadRoutine routine);
void grw_set_write_routine(GenericReadWritePtr grw, GenericWriteRoutine routine);
uint32_t grw_read(GenericReadWritePtr grw);
void grw_write(GenericReadWritePtr grw, uint32_t value);
GenericReadWritePtr grw_init(DagCardPtr card, uintptr_t address, GenericReadRoutine read, GenericWriteRoutine write);
void grw_dispose(GenericReadWritePtr grw);
DagCardPtr grw_get_card(GenericReadWritePtr grw);
grw_error_t grw_get_last_error(GenericReadWritePtr grw);
void grw_set_last_error(GenericReadWritePtr grw, grw_error_t error);
void grw_set_on_operation(GenericReadWritePtr grw, grw_on_operation_t op);
grw_on_operation_t grw_get_on_operation(GenericReadWritePtr grw);
void grw_set_attribute(GenericReadWritePtr grw, AttributePtr attribute);
AttributePtr grw_get_attribute(GenericReadWritePtr grw);


/* Various functions that define different ways to retrieve and set register values.*/
void grw_iom_write(GenericReadWritePtr grw, uint32_t value);
uint32_t grw_iom_read(GenericReadWritePtr grw);
uint32_t grw_dag71s_phy_read(GenericReadWritePtr grw);
void grw_dag71s_phy_write(GenericReadWritePtr grw, uint32_t val);
uint32_t grw_mini_mac_raw_smbus_read(GenericReadWritePtr grw);

uint32_t grw_vsc8476_raw_smbus_read(GenericReadWritePtr grw);
uint32_t grw_mini_mac_raw_smbus_word_read(GenericReadWritePtr grw);
void grw_mini_mac_raw_smbus_write(GenericReadWritePtr grw, uint32_t val);

uint32_t grw_xfp_sfp_raw_smbus_read(GenericReadWritePtr grw);
void grw_xfp_sfp_raw_smbus_write(GenericReadWritePtr grw, uint32_t val);

void grw_vsc8476_raw_smbus_write(GenericReadWritePtr grw, uint32_t val);
void grw_mini_mac_raw_smbus_word_write(GenericReadWritePtr grw, uint32_t val);
uint32_t grw_drb_smbus_read(GenericReadWritePtr grw);
void grw_drb_smbus_write(GenericReadWritePtr grw, uint32_t val);
uint32_t grw_raw_smbus_read(GenericReadWritePtr grw);
void grw_raw_smbus_write(GenericReadWritePtr grw, uint32_t val);
void grw_dag43s_write(GenericReadWritePtr grw , uint32_t val);
uint32_t grw_dag43s_read(GenericReadWritePtr grw);
void grw_dag62se_write(GenericReadWritePtr grw , uint32_t val);
uint32_t grw_dag62se_read(GenericReadWritePtr grw);
void grw_lm63_smb_write(GenericReadWritePtr grw , uint32_t val);
uint32_t grw_lm63_smb_read(GenericReadWritePtr grw);
void grw_set_capture_hash(GenericReadWritePtr grw, uint32_t val);
void grw_unset_capture_hash(GenericReadWritePtr grw, uint32_t val);
uint32_t grw_rom_read(GenericReadWritePtr grw);
void grw_dag37d_write(GenericReadWritePtr grw , uint32_t val);
uint32_t grw_dag37d_read(GenericReadWritePtr grw);
uint32_t grw_vsc8476_mdio_read(GenericReadWritePtr grw);
void grw_vsc8476_mdio_write(GenericReadWritePtr grw, uint32_t val);
void grw_hlb_component_set_max_range(GenericReadWritePtr grw, uint32_t value);
uint32_t grw_hlb_component_get_max_range(GenericReadWritePtr grw);
uint32_t grw_hlb_component_get_min_range(GenericReadWritePtr grw);
void grw_hlb_component_set_min_range(GenericReadWritePtr grw, uint32_t value);
uint32_t grw_sonic_status_cache_read(GenericReadWritePtr grw);
uint32_t grw_oc48_deframer_status_cache_read(GenericReadWritePtr grw);
uint32_t grw_sonet_pp_b1_cache_read(GenericReadWritePtr grw);
uint32_t grw_sonet_pp_b2_cache_read(GenericReadWritePtr grw);
uint32_t grw_sonet_pp_b3_cache_read(GenericReadWritePtr grw);
uint32_t grw_sonet_pp_rei_cache_read(GenericReadWritePtr grw);
uint32_t grw_drb_optics_smbus_read(GenericReadWritePtr grw);
void grw_drb_optics_smbus_write(GenericReadWritePtr grw, uint32_t val);
uint32_t grw_vsc8486_mdio_read(GenericReadWritePtr grw);
void grw_vsc8486_mdio_write(GenericReadWritePtr grw, uint32_t val);
uint32_t grw_vsc8479_raw_smbus_read(GenericReadWritePtr grw);
void grw_vsc8479_raw_smbus_write(GenericReadWritePtr grw, uint32_t val);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ATTRIBUTE_H */
