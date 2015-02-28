/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef CARD_H
#define CARD_H

/* Public API headers. */
#include "dag_config.h"

/* Internal project headers. */
#include "card_types.h"
#include "component_types.h"

/* Endace headers. */
#include "daginf.h"
#include "dagreg.h"
#include "dag_romutil.h"

/* C Standard Library headers. */
#if defined (__linux__)
#include <inttypes.h>
#endif

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	/* Factory method. */
	DagCardPtr card_init(dag_card_t card_type);

	/* Card methods. */
	void card_dispose(DagCardPtr card);
	void card_post_initialize(DagCardPtr card);
	void card_reset(DagCardPtr card);
	void card_default(DagCardPtr card);
	dag_err_t card_load_firmware(DagCardPtr card, uint8_t* image);
	dag_err_t card_load_embedded(DagCardPtr card, uint8_t* image, int target_processor_region);
	dag_err_t card_load_pp_image(DagCardPtr card, const char* filename, int which);
	ComponentPtr card_get_root_component(DagCardPtr card);
	void card_set_root_component(DagCardPtr card, ComponentPtr root_component);
	volatile uint8_t* card_get_iom_address(DagCardPtr card);
	void card_set_iom_address(DagCardPtr card, volatile uint8_t* iom);
	daginf_t* card_get_info(DagCardPtr card);
	void card_set_info(DagCardPtr card, daginf_t* info);
	dag_reg_t* card_get_registers(DagCardPtr card);
	void card_set_registers(DagCardPtr card, dag_reg_t* registers);
	int card_get_fd(DagCardPtr card);
	void card_set_fd(DagCardPtr card, int fd);
	int card_get_rx_stream_count(DagCardPtr card);
	void card_set_rx_stream_count(DagCardPtr card, int val);
	int card_get_tx_stream_count(DagCardPtr card);
	void card_set_tx_stream_count(DagCardPtr card, int val);
	uint32_t card_read_iom(DagCardPtr card, uintptr_t address);
	void card_write_iom(DagCardPtr card, uintptr_t address, uint32_t value);
	void* card_get_private_state(DagCardPtr card);
	void card_set_private_state(DagCardPtr card, void* state);
	dag_err_t card_get_last_error(DagCardPtr card);
	void card_set_last_error(DagCardPtr card, dag_err_t error_code);
	dag_card_t card_get_card_type(DagCardPtr card);
	const char* card_get_card_type_as_string(DagCardPtr card);
	void card_set_card_type(DagCardPtr card, dag_card_t type);
	void card_erase_rom(DagCardPtr card, romtab_t* rp, uint32_t start_address, uint32_t end_address);
	dag_err_t card_update_register_base(DagCardPtr card);
	
	void card_set_dispose_routine(DagCardPtr card, CardDisposeRoutine routine);
	void card_set_post_initialize_routine(DagCardPtr card, CardPostInitializeRoutine routine);
	void card_set_reset_routine(DagCardPtr card, CardResetRoutine routine);
	void card_set_default_routine(DagCardPtr card, CardDefaultRoutine routine);
	void card_set_load_firmware_routine(DagCardPtr card, CardLoadFirmwareRoutine routine);
	void card_set_update_register_base_routine(DagCardPtr card, CardUpdateRegisterBaseRoutine routine);
	
	int card_lock_stream(DagCardPtr card, int stream);
	int card_unlock_stream(DagCardPtr card, int stream);
	int card_lock_all_streams(DagCardPtr card);
	void card_unlock_all_streams(DagCardPtr card);
	uintptr_t card_get_register_address(DagCardPtr card, dag_reg_module_t reg_code, uint32_t offset);
	int card_get_register_version(DagCardPtr card, dag_reg_module_t reg_code, uint32_t offset);
    int card_get_register_index_by_version(DagCardPtr card, dag_reg_module_t reg_code, uint32_t version, uint32_t offset);
    uint32_t card_get_register_count(DagCardPtr card, dag_reg_module_t code);
	void card_reset_datapath(DagCardPtr card);
	int card_pbm_config_stream(DagCardPtr card, uint32_t stream, uint32_t size);
	int card_pbm_get_stream_mem_size(DagCardPtr card, uint32_t stream);
	int card_pbm_config_overlap(DagCardPtr card);
	int card_pbm_is_overlapped(DagCardPtr card, uint8_t* is_overlapped);
	void* card_read_file(FILE * fp, unsigned int maxlen, size_t* lenp);

	/* Verification routine.  Returns 1 if the DagCardPtr is valid, 0 otherwise. */
	int valid_card(DagCardPtr card);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CARD_H */
