/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef STREAM_COMPONENT_H
#define STREAM_COMPONENT_H

ComponentPtr get_new_stream(DagCardPtr card, uint32_t idx);
AttributePtr get_new_mem_attribute(void);
AttributePtr get_new_mem_bytes_attribute(void);

#endif

