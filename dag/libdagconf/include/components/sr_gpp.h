/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * $Id: sr_gpp.h,v 1.4 2006/08/09 05:21:17 sashan Exp $
 */

#ifndef SR_GPP_H
#define SR_GPP_H
enum
{
	kSRGPPConfig = 0x0,
	kSRGPPCounter = 0x4,
	kSRGPPStride = 0x8
};


ComponentPtr sr_gpp_get_new_component(DagCardPtr card, uint32_t index);

#endif
