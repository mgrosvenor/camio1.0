/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef SONET_CHANNEL_MGMT_COMPONENT_H
#define SONET_CHANNEL_MGMT_COMPONENT_H

#define MAX_TIMESLOTS 768

typedef struct timeslot_information
{
	uint8_t indicator;	// Indicates a valid pointer or concatenation
	uint16_t vc_id;		// Stores assigned channel IDs and used by the user for channel selection
	uint16_t timeslot_id;	// Used for configuring the raw SPE extract module
	uint8_t size;		// Size of the SONET subchannels
} timeslot_information_t;

typedef struct sonet_channel_mgmt_state
{
	uint32_t mIndex;
	uint8_t  network_rate;
	uint32_t timeslots;
	uint32_t vc_id_total;
	timeslot_information_t timeslot_info[MAX_TIMESLOTS];
	ComponentPtr mConnection[MAX_TIMESLOTS];
} sonet_channel_mgmt_state_t;

ComponentPtr sonet_channel_mgmt_get_new_component(DagCardPtr card, uint32_t index);

#endif
