/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef LED_CONTROLLER_COMPONENT_H
#define LED_CONTROLLER_COMPONENT_H

typedef enum
{
    kPeriodRegister0 = 2,
    kDutyCycleRegister0 = 3,
    kPeriodRegister1 = 4,
    kDutyCycleRegister1 = 5
} pca_register_address_t;

typedef enum
{
    kPCAAddress0 = 0xce,
    kPCAAddress1 = 0xc6
} pca_address_t;

ComponentPtr get_new_led_controller(DagCardPtr card);
AttributePtr get_new_led_status_attribute(pca_address_t pca_address, uint32_t register_value, uint32_t bank, int index);
AttributePtr get_new_period_attribute(pca_address_t pca_address, uint32_t register_value, int index);
AttributePtr get_new_duty_cycle_attribute(pca_address_t pca_address, uint32_t register_value, int index);


#endif
