/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef DAG62SE_PORT_COMPONENT_H
#define DAG62SE_PORT_COMPONENT_H

typedef enum dagctrl {
/* from 0x0 in pci register space */
PCI_OPTICS      = 0x0300,
PCI_OPTICS_I2C  = 0x0304,
RX_OPTICS       = 0x1300,
RX_PARITY       = 0x1304,
TX_OPTICS       = 0x1b00
} dagctrl_t;

ComponentPtr dag62se_get_new_port(DagCardPtr card, int index);
#endif

