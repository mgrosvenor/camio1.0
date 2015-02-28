
/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *  $Id: dag37t_map_functions.h,v 1.4 2007/03/09 01:11:48 cassandra Exp $
 */


#ifndef DAG37T_MAP_FUNCTIONS_H
#define DAG37T_MAP_FUNCTIONS_H


/* register offsets */
#define MAP_CTRL_REG            0x00
#define CFG_BUF_CMD_REG         0x04
#define CFG_BUF_DATA_REG        0x08
#define CFG_WRITE_CMD_REG       0x0C
#define CFG_TABLE_DATA_REG      0x14
#define LINE_FEATURE_CTRL_REG   0x18


#define RESET_COMMAND 0x10
#define RESET_MASK    0x10

#define CMD_BUF_READY   0x0100
#define CMD_COUNT_MASK  0x7F 

#define CONFIG_ADDR_MASK 0x7FF
#define READ_CONN_MASK   0x00FFF000
#define READ_TIMESLOT_MASK  0x000FF000
#define CONN_ADDR_MASK 0x00000FFF
#define READ_CHAIN_BIT_MASK 0x00100000
#define LATCH_MASK       0x8000
#define TIMESLOT_MASK_OFFSET 12

#define WRITE_ALL_MASK 0xFFFFFFFF

/*commands used in Configuration Write Command/Data Register (0x0C) */
#define CMD_MASK             (0xF << 24)
#define NOP_CMD	             (0x0 << 24)
#define WRITE_CMD            (0x1 << 24)
#define READ_CMD             (0x2 << 24)
#define WRITE_ADD_CONN_CMD   (0x5 << 24)
#define WRITE_ADD_RAW_CMD    (0xd << 24)
#define WRITE_DEL_CONN_CMD   (0x9 << 24)

#define CFG_BUF_CMD_MASK      0x7
#define CFG_BUF_COPY_CMD      0x2
#define CFG_BUF_EXEC_CMD      0x4

#define TIMESLOT_MASK         0xFF
#define RAW_MODE_MASK         0xFFFF0000

#define MUX_HOST    0x400
#define MUX_LINE    0x404
#define MUX_IOP     0x408


#if defined(__FreeBSD__) || defined(__linux__) || (defined(__SVR4) && defined(__sun)) || (defined(__APPLE__) && defined(__ppc__))
#define INLINE inline
#elif defined(_WIN32)
#ifdef INLINE
    #undef INLINE
#endif
#define INLINE __inline
#else
#error Compiling on an unsupported platform - please contact <support@endace.com> for assistance.
#endif 		/* Platform-specific code. */




int wait_not_busy(uint8_t* dagiom, uint32_t module);
INLINE uint32_t iom_read(uint8_t* dagiom, uint32_t addr);
INLINE uint32_t iom_write(uint8_t* dagiom, uint32_t addr, uint32_t mask,  uint32_t val);
uint32_t read_location (int dagfd, uint8_t* dagiom, int addr, uint32_t module_addr );
int latch_counters(int dagfd, uint32_t module_addr);
INLINE void reset_command_count(int dagfd, uint8_t* dagiom, uint32_t module_addr);
INLINE int copy_to_execution_buffer(uint8_t *dagiom, int module_addr);
int write_conn_num_to_table(uint8_t* dagiom, uint32_t conn_num, uint32_t module_addr,  uint32_t table_entry, uint32_t command);
int write_mask_to_table(uint8_t* dagiom, uint32_t mask, uint32_t chain_bit, uint32_t module_addr, uint32_t table_entry);
int set_raw_mode(int dagfd, int module_addr, int line);
int execute_command(int dagfd, int module_addr);

#endif




