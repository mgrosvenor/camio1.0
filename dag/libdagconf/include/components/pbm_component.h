/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef PBM_COMPONENT_H
#define PBM_COMPONENT_H

ComponentPtr pbm_get_new_pbm(DagCardPtr card, uint32_t index);
uint32_t pbm_get_version(ComponentPtr pbm);
uint32_t pbm_get_global_status_register(ComponentPtr pbm);

#endif

