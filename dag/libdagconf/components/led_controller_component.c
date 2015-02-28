/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/components/led_controller_component.h"

#include "dagutil.h"
/* C Standard Library headers. */
#include <assert.h>
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#if defined(__FreeBSD__) || defined(__linux__) || (defined(__APPLE__) && defined(__ppc__))
#include <unistd.h>
#endif

/* Enable this define to use the local state management for the led controllers.
 * this speeds up the access several magnitudes but means that only one
 * application can manage the led states
 */
#define USE_FAST_LED_INTERFACE

#define BUFFER_SIZE 1024

typedef struct
{
    uint32_t mSMBusBase;
    uint8_t led_state[2][4];    /* All led states - 2 devices and 4 registers per, 4 leds per register */
} led_controller_state_t;

typedef struct
{
    pca_address_t mPCAAddress;
    pca_register_address_t mPeriodRegister;
} period_state_t;

typedef struct
{
    pca_address_t mPCAAddress;
    uint32_t mLEDRegister;
    uint32_t mLEDBank;
} led_status_state_t;

static led_status_state_t uLEDTable[32] =
{
	{0xce, 6, 0},
	{0xce, 6, 2},
	{0xce, 7, 0},
	{0xce, 7, 2},
	{0xce, 8, 0},
	{0xce, 8, 2},
	{0xce, 9, 0},
	{0xce, 9, 2},
	{0xc6, 6, 0},
	{0xc6, 6, 2},
	{0xc6, 7, 0},
	{0xc6, 7, 2},
	{0xc6, 9, 3},
	{0xc6, 9, 1},
	{0xc6, 8, 3},
	{0xc6, 8, 1},

	{0xce, 6, 1},
	{0xce, 6, 3},
	{0xce, 7, 1},
	{0xce, 7, 3},
	{0xce, 8, 1},
	{0xce, 8, 3},
	{0xce, 9, 1},
	{0xce, 9, 3},
	{0xc6, 6, 1},
	{0xc6, 6, 3},
	{0xc6, 7, 1},
	{0xc6, 7, 3},
	{0xc6, 9, 2},
	{0xc6, 9, 0},
	{0xc6, 8, 2},
	{0xc6, 8, 0}
};

typedef struct
{
    uint8_t mLedSelector[4];
    uint8_t mAddress;
} pca9552_state_t;

pca9552_state_t pca9552[2];

uint32_t pca9552_register_address_map[4] = {6,7,8,9};

//const uint32_t sleep_time = 2000000;

/* led_controller component. */
static void led_controller_dispose(ComponentPtr component);
static void led_controller_reset(ComponentPtr component);
static void led_controller_default(ComponentPtr component);
static int led_controller_post_initialize(ComponentPtr component);
static dag_err_t led_controller_update_register_base(ComponentPtr component);

/* led_status attribute */
static void led_status_dispose(AttributePtr attribute);
static void led_status_set_value(AttributePtr attribute, void* value, int length);
static void led_status_post_initialize(AttributePtr attribute);
static void* led_status_get_value(AttributePtr attribute);

/* period attribute */
static void period_dispose(AttributePtr attribute);
static void period_set_value(AttributePtr attribute, void* value, int length);
static void period_post_initialize(AttributePtr attribute);
static void* period_get_value(AttributePtr attribute);

/* duty_cycle attribute */
static void duty_cycle_dispose(AttributePtr attribute);
static void duty_cycle_set_value(AttributePtr attribute, void* value, int length);
static void duty_cycle_post_initialize(AttributePtr attribute);
static void* duty_cycle_get_value(AttributePtr attribute);

/* internal function */
static uint32_t drb_smbus_busy_wait(DagCardPtr card, uint32_t address, int* countdown);
static void led_state_drb_smbus_write(DagCardPtr card, uint32_t address, uint32_t val);
static uint32_t led_state_drb_smbus_read(DagCardPtr card, uint32_t address);

ComponentPtr
get_new_led_controller(DagCardPtr card)
{
    ComponentPtr result = component_init(kComponentLEDController, card); 
    led_controller_state_t* state = NULL;

    if (NULL != result)
    {
        component_set_dispose_routine(result, led_controller_dispose);
        component_set_post_initialize_routine(result, led_controller_post_initialize);
        component_set_reset_routine(result, led_controller_reset);
        component_set_default_routine(result, led_controller_default);
        component_set_name(result, "led_controller");
        component_set_update_register_base_routine(result, led_controller_update_register_base);
        state = (led_controller_state_t*)malloc(sizeof(led_controller_state_t));
        memset(state, 0, sizeof(led_controller_state_t));
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
led_controller_dispose(ComponentPtr component)
{
}

static int
led_controller_post_initialize(ComponentPtr component)
{
    /* Return 1 if standard post_initialize() should continue, 0 if not.
    * "Standard" post_initialize() calls post_initialize() recursively on subcomponents.
    */
    DagCardPtr card = NULL;
    led_controller_state_t* state = NULL;

#ifdef USE_FAST_LED_INTERFACE
	uint32_t register_value = 0;
	uint32_t address;
	int dev = 0;	/* Controller device index */ 
	int reg;	/* controller register index */
	int led;
	
#endif

    card = component_get_card(component);
    state = component_get_private_state(component);
    state->mSMBusBase = card_get_register_address(card, DAG_REG_SMB, 0);

#ifdef USE_FAST_LED_INTERFACE
	/* Initialize the state of the LED's cache. */
	for (led = 0; led < 32; led ++)
	{
		address = uLEDTable[led].mLEDRegister | uLEDTable[led].mPCAAddress << 8;
		register_value = led_state_drb_smbus_read(card, address);
		
		switch (uLEDTable[led].mPCAAddress) {
        	case kPCAAddress1:    
            		dev=0;
           		break;
        	case kPCAAddress0:    
           		dev=1;
            		break;
        	default:
            		assert(0);
           	 	break;
        	}
		
        	reg = uLEDTable[led].mLEDRegister - 6;    /* get the register index in the range 0 - 3 */

		state->led_state[dev][reg] = register_value;
	}
#endif

    return 0;
}

static dag_err_t
led_controller_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = NULL;
        led_controller_state_t* state = NULL;
        card = component_get_card(component);
        state = component_get_private_state(component);
        state->mSMBusBase = card_get_register_address(card, DAG_REG_SMB, 0);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static void
led_controller_reset(ComponentPtr component)
{
}


static void
led_controller_default(ComponentPtr component)
{
}

AttributePtr
get_new_led_status_attribute(uint32_t pca_address, uint32_t register_address, uint32_t bank, int index)
{
    AttributePtr result = attribute_init(kUint32AttributeLEDStatus); 
    led_status_state_t* state;
    
    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        attribute_set_dispose_routine(result, led_status_dispose);
        attribute_set_post_initialize_routine(result, led_status_post_initialize);
        attribute_set_setvalue_routine(result, led_status_set_value);
        attribute_set_getvalue_routine(result, led_status_get_value);
        sprintf(buffer, "led_status%d", index);
        attribute_set_name(result, buffer);
        attribute_set_valuetype(result, kAttributeUint32);
        attribute_set_description(result, "Turn the LED on the pod of the DAG 3.7 t on/off");
        attribute_set_config_status(result, kDagAttrConfig);
        state = (led_status_state_t*)malloc(sizeof(led_status_state_t));
        memset(state, 0, sizeof(led_status_state_t));
        attribute_set_private_state(result, state);
        state->mPCAAddress = pca_address;
        state->mLEDRegister = register_address;
        state->mLEDBank = bank;
    }
    
    return result;
}

static void
led_status_dispose(AttributePtr attribute)
{
}

static void
led_status_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr led_controller;
    led_controller_state_t* controller_state = NULL;
    led_status_state_t* led_state = NULL;
    led_status_t led_status = *(led_status_t*)value;
    uint32_t register_value;
    uint32_t address;
#ifdef USE_FAST_LED_INTERFACE
    //uint32_t wr_sleep_time = 1000000;
    int device=0;    /* Controller device index */
    int reg;        /* Controller register index */
    int led;        /* LED bit offset in register */
#else
    uint32_t wr_sleep_time = 100000000;
#endif
    
    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        led_controller = attribute_get_component(attribute);
        assert(component_get_component_code(led_controller) == kComponentLEDController);
        controller_state = (led_controller_state_t*)component_get_private_state(led_controller);
        led_state = (led_status_state_t*)attribute_get_private_state(attribute);
#ifdef USE_FAST_LED_INTERFACE
        /* The state of the LED's is cached locally in the controller_state structure. */

        /* since each register controls the state for four LEDs, when an LED state is changed
         * the current state of the other three LEDs must be preserved.
         * This code uses the cached state values to do this
         */

        /* Two LED controller devices: Get device 0 or 1 */
        switch (led_state->mPCAAddress) {
        case kPCAAddress1:    
            device=0;
            break;
        case kPCAAddress0:    
            device=1;
            break;
        default:
            assert(0);
            break;
        }
        reg = led_state->mLEDRegister - 6;    /* get the register index in the range 0 - 3 */
        led = led_state->mLEDBank*2;        /* LED bit offset in the register, 0,2,4,6 - 2 bits per LED */

        /* Get the current state of the four LEDs */
        register_value = controller_state->led_state[device][reg];

        /* mask out the old led value and set the new value */
        register_value = (register_value & ~(0x3 << led)) | ((led_status & 0x3) << led);

        /* Save this state for other led operations */
        controller_state->led_state[device][reg] = (uint8_t)register_value;
        
        /* Add in the device's SMBus Id and the register number */
        //register_value = (register_value << 16) | (led_state->mLEDRegister << 8) | led_state->mPCAAddress;
	    address = led_state->mLEDRegister | led_state->mPCAAddress << 8;
#else
#if 0
        /* read the current led status so that we can preserve the appropriate bits of the led selector*/
        register_value = 1 << 24 |  led_state->mLEDRegister << 8 | led_state->mPCAAddress;
        card_write_iom(card, controller_state->mSMBusBase, register_value);
        dagutil_nanosleep(wr_sleep_time);        /* This is a really long sleep... */
        register_value = card_read_iom(card, controller_state->mSMBusBase + 4);
        register_value >>= 16;
        /*make sure that the first byte is preserved and the remaining are zeroed */
        register_value &= 0xff;
#endif
	    address = led_state->mLEDRegister | led_state->mPCAAddress << 8;
	    register_value = led_state_drb_smbus_read(card, address);

        /* Clear the bits we want to change */
        register_value = register_value & ~(3 << (led_state->mLEDBank * 2));
        /* Set them depending on the desired value */
        if (kLEDOn == led_status)
        {
            register_value |= kLEDOn << (led_state->mLEDBank * 2);
            //register_value = register_value << 16 | led_state->mLEDRegister << 8 | led_state->mPCAAddress;
	        address = led_state->mLEDRegister | led_state->mPCAAddress << 8;
        }
        else if (kLEDOff == led_status)
        {
            register_value |= kLEDOff << (led_state->mLEDBank * 2);
            //register_value = register_value << 16 | led_state->mLEDRegister << 8 | led_state->mPCAAddress;
	        address = led_state->mLEDRegister | led_state->mPCAAddress << 8;
        }
        else if (kLEDAtBlinkRate0 == led_status)
        {
            register_value |= kLEDAtBlinkRate0 << (led_state->mLEDBank * 2);
            //register_value = register_value << 16 | led_state->mLEDRegister << 8 | led_state->mPCAAddress;
	        address = led_state->mLEDRegister | led_state->mPCAAddress << 8; 
        }
#endif
	    led_state_drb_smbus_write(card, address, register_value); 
	
        //card_write_iom(card, controller_state->mSMBusBase, register_value);
       // dagutil_nanosleep(wr_sleep_time);
    }
}

static void*
led_status_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr led_controller;
    led_controller_state_t* controller_state = NULL;
    led_status_state_t* led_state = NULL;
    uint32_t register_value;
    led_status_t val;
    uint32_t address;

    if (1 == valid_attribute(attribute))
    {
        card = attribute_get_card(attribute);
        led_controller = attribute_get_component(attribute);
        assert(component_get_component_code(led_controller) == kComponentLEDController);
        controller_state = (led_controller_state_t*)component_get_private_state(led_controller);
        led_state = (led_status_state_t*)attribute_get_private_state(attribute);
#if 0	
        /* We want to read the value of the appropriate 2 bits corresponding to the led represented by this attribute */
        register_value = 1 << 24 | 0x0 << 16 | led_state->mLEDRegister << 8 | led_state->mPCAAddress;
        dagutil_nanosleep(sleep_time);
        card_write_iom(card, controller_state->mSMBusBase, register_value);
        dagutil_nanosleep(sleep_time);
        register_value = card_read_iom(card, controller_state->mSMBusBase + 4);
        register_value >>= 16;
#endif 
	    address = led_state->mLEDRegister | led_state->mPCAAddress << 8;
	    register_value = led_state_drb_smbus_read(card, address);
        
	    register_value = (register_value >> (led_state->mLEDBank*2)) & 3;
        val = register_value;
        attribute_set_value_array(attribute, &val, sizeof(val));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

static void
led_status_post_initialize(AttributePtr attribute)
{

}

AttributePtr
get_new_period_attribute(pca_address_t pca_address, pca_register_address_t address, int index)
{
    AttributePtr result = attribute_init(kUint32AttributePeriod); 
    period_state_t* period_state = NULL;
    
    if (NULL != result)
    {
        char buffer[BUFFER_SIZE];
        attribute_set_dispose_routine(result, period_dispose);
        attribute_set_post_initialize_routine(result, period_post_initialize);
        attribute_set_setvalue_routine(result, period_set_value);
        attribute_set_getvalue_routine(result, period_get_value);
        sprintf(buffer, "period%d", index);
        attribute_set_name(result, buffer);
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        period_state = (period_state_t*)malloc(sizeof(period_state_t));
        period_state->mPCAAddress = pca_address;
        period_state->mPeriodRegister = address;
        attribute_set_private_state(result, (void*)period_state);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
period_dispose(AttributePtr attribute)
{

}

static void
period_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr led_controller;
    uint32_t register_value;
    uint32_t address;
    led_controller_state_t* controller_state = NULL;
    period_state_t* period_state = NULL;
//    uint32_t wr_sleep_time = 100000000;

    if (1 == valid_attribute(attribute))
    {
        /* The period of the blink is determined by the following formula
        (register_value + 1)/38 (see the PCA9552 datasheet). So we convert the given value
        in 100ths of a second to 38ths of a second. */
        register_value = (uint32_t)(37.0/100.0 * *(uint32_t*)value + 0.5);
        card = attribute_get_card(attribute);
        led_controller = attribute_get_component(attribute);
        controller_state = component_get_private_state(led_controller);
        period_state = attribute_get_private_state(attribute);
        assert(component_get_component_code(led_controller) == kComponentLEDController);
/*
     	register_value <<= 16;
        register_value |= period_state->mPeriodRegister << 8 | period_state->mPCAAddress;
        dagutil_nanosleep(wr_sleep_time);
        card_write_iom(card, controller_state->mSMBusBase, register_value);
        dagutil_nanosleep(wr_sleep_time);

*/
	    address = period_state->mPeriodRegister | period_state->mPCAAddress << 8;
	    led_state_drb_smbus_write(card, address, register_value); 
	
    }
}

static void*
period_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr led_controller;
    uint32_t register_value;
    uint32_t address;
    led_controller_state_t* controller_state = NULL;
    period_state_t* period_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        /* The period of the blink is determined by the following formula
        (register_value + 1)/38 (see the PCA9552 datasheet). So we convert the value
        from 38ths of a second to 100ths of a second */
        card = attribute_get_card(attribute);
        led_controller = attribute_get_component(attribute);
        controller_state = component_get_private_state(led_controller);
        period_state = attribute_get_private_state(attribute);
        assert(component_get_component_code(led_controller) == kComponentLEDController);
/* 
	register_value = 1 << 24 |  period_state->mPeriodRegister << 8 | period_state->mPCAAddress;
        dagutil_nanosleep(sleep_time);
        card_write_iom(card, controller_state->mSMBusBase, register_value);
        while((register_value = card_read_iom(card, controller_state->mSMBusBase + 4)) & 0x1);
        register_value >>= 16;
*/	
	    address = period_state->mPeriodRegister | period_state->mPCAAddress << 8;
	    register_value = led_state_drb_smbus_read(card, address);
	
	    register_value &= 0xff;

        register_value = (uint32_t)(100.0 * register_value / 37.0);
        attribute_set_value_array(attribute, &register_value, sizeof(register_value));
        return (void *)attribute_get_value_array(attribute);
        }
    return NULL;
}

static void
period_post_initialize(AttributePtr attribute)
{

}

AttributePtr
get_new_duty_cycle_attribute(pca_address_t pca_address, pca_register_address_t address, int index)
{
    AttributePtr result = attribute_init(kUint32AttributeDutyCycle); 
    period_state_t* duty_cycle_state = NULL;
    
    if (NULL != result)
    {
        /* The period of the blink is determined by the following formula
        (256 - register_value)/256 (see the PCA9552 datasheet). */
        char buffer[BUFFER_SIZE];
        attribute_set_dispose_routine(result, duty_cycle_dispose);
        attribute_set_post_initialize_routine(result, duty_cycle_post_initialize);
        attribute_set_setvalue_routine(result, duty_cycle_set_value);
        attribute_set_getvalue_routine(result, duty_cycle_get_value);
        sprintf(buffer, "duty_cycle%d", index);
        attribute_set_name(result, buffer);
        attribute_set_description(result, "");
        attribute_set_config_status(result, kDagAttrConfig);
        duty_cycle_state = (period_state_t*)malloc(sizeof(period_state_t));
        duty_cycle_state->mPCAAddress = pca_address;
        duty_cycle_state->mPeriodRegister = address;
        attribute_set_private_state(result, (void*)duty_cycle_state);
        attribute_set_valuetype(result, kAttributeUint32);
    }
    
    return result;
}

static void
duty_cycle_dispose(AttributePtr attribute)
{

}

static void
duty_cycle_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card;
    ComponentPtr led_controller;
    uint32_t register_value;
    uint32_t address;
    led_controller_state_t* controller_state = NULL;
    period_state_t* duty_cycle_state = NULL;
//  uint32_t wr_sleep_time = 100000000;

    if (1 == valid_attribute(attribute))
    {
        /* The duty_cycle of the blink is determined by the following formula
        (256 - register_value)/256 (see the PCA9552 datasheet).*/ 
        register_value = (uint32_t)(256.0* *(uint32_t*)value / 100.0) + 0.5;
        card = attribute_get_card(attribute);
        led_controller = attribute_get_component(attribute);
        controller_state = component_get_private_state(led_controller);
        duty_cycle_state = attribute_get_private_state(attribute);
        assert(component_get_component_code(led_controller) == kComponentLEDController);
/*
        register_value <<= 16;
        register_value |= duty_cycle_state->mPeriodRegister << 8 | duty_cycle_state->mPCAAddress;
        dagutil_nanosleep(wr_sleep_time);
        card_write_iom(card, controller_state->mSMBusBase, register_value);
        dagutil_nanosleep(wr_sleep_time);
*/
	address = duty_cycle_state->mPeriodRegister | duty_cycle_state->mPCAAddress << 8;
	led_state_drb_smbus_write(card, address, register_value); 


    }
}

static void*
duty_cycle_get_value(AttributePtr attribute)
{
    DagCardPtr card;
    ComponentPtr led_controller;
    uint32_t register_value;
    uint32_t address;
    led_controller_state_t* controller_state = NULL;
    period_state_t* duty_cycle_state = NULL;

    if (1 == valid_attribute(attribute))
    {
        /* The duty_cycle of the blink is determined by the following formula
        (256 - register_value)/256 (see the PCA9552 datasheet). */
        card = attribute_get_card(attribute);
        led_controller = attribute_get_component(attribute);
        controller_state = component_get_private_state(led_controller);
        duty_cycle_state = attribute_get_private_state(attribute);
        assert(component_get_component_code(led_controller) == kComponentLEDController);
/*
	register_value = 1 << 24 |  duty_cycle_state->mPeriodRegister << 8 | duty_cycle_state->mPCAAddress;
        dagutil_nanosleep(sleep_time);
        card_write_iom(card, controller_state->mSMBusBase, register_value);
        while((register_value = card_read_iom(card, controller_state->mSMBusBase + 4)) & 0x1);
        register_value >>= 16;
*/
	    address = duty_cycle_state->mPeriodRegister | duty_cycle_state->mPCAAddress << 8;
	    register_value = led_state_drb_smbus_read(card, address);
        register_value &= 0xff;
    
        register_value = (uint32_t)((100.0 * register_value/256) + 0.5);
        attribute_set_value_array(attribute, &register_value, sizeof(register_value));
        return (void *)attribute_get_value_array(attribute);
        }
    return NULL;
}

static void
duty_cycle_post_initialize(AttributePtr attribute)
{

}


/*
Returns the value of the status register of the smbus. countdown is
a number specifiying how many times to try reading from the register
*/
uint32_t
drb_smbus_busy_wait(DagCardPtr card, uint32_t address, int* countdown)
{
    uintptr_t smbus_address = 0;
    uint32_t status_register = 0;
    uint32_t index = 0;

    if (countdown)
    {
        index = (0x30000 & address) >> 16;
        smbus_address = card_get_register_address(card, DAG_REG_SMB, index);
        status_register = card_read_iom(card, smbus_address+0x4);
        while (((status_register & BIT1) == 0) && *countdown)
        {
            dagutil_verbose_level(4, "drb_smbus: status_register 0x%x, countdown = %d\n", status_register, *countdown);
            dagutil_verbose_level(5, "drb_smbus: busy waiting\n");
            *countdown = *countdown - 1;
            dagutil_nanosleep(100);
            status_register = card_read_iom(card, smbus_address+0x4);
	    if ((status_register & BIT24) == 1)
		    break;
        }
        if (*countdown == 0)
        {
            dagutil_verbose_level(1,"drb_smbus: busy wait timeout!\n");
        }

        return status_register;
    }
    return 0;
}

void
led_state_drb_smbus_write(DagCardPtr card, uint32_t address, uint32_t val)
{
	uint8_t device_address = 0;
	uint8_t device_register = 0;
	uintptr_t smbus_address = 0;
	uint32_t register_value = 0;
	uint32_t status_register = 0;
	uint32_t index = 0;
	int countdown = 1000;

	device_address = (uint8_t)((0xff00 & address) >> 8);
	device_register = (uint8_t)(0xff & address);
	index = (0xffff0000 & address) >> 16;
	status_register = drb_smbus_busy_wait(card, address, &countdown);
	if (countdown == 0)
	{
		dagutil_verbose_level(1,"drb_smbus: busy wait timeout!\n");
		return;
	}
	smbus_address = card_get_register_address(card, DAG_REG_SMB, index);

	register_value |= device_address;
	register_value |= device_register << 8;
	register_value |= val << 16;
	register_value &= ~BIT24;
	card_write_iom(card, smbus_address, register_value);

	/* wait till the write completes */
	countdown = 1000;
	status_register = drb_smbus_busy_wait(card, address, &countdown);
	
	if (countdown == 0)
	{
		dagutil_verbose_level(1,"drb_smbus: busy wait timeout!\n");
		return;
	}
	
	/* check if the command succeeded or failed */
	if (status_register & (BIT5 | BIT3 | BIT2))
	{
		dagutil_verbose_level(1,"drb_smbus: command failed!\n");
		return;
	}

}

uint32_t
led_state_drb_smbus_read(DagCardPtr card, uint32_t address)
{

	uint8_t device_address = 0;
	uint8_t device_register = 0;
	uintptr_t smbus_address = 0;
	uint32_t ret_val = 0;
	uint32_t register_value = 0;
	uint32_t status_register = 0;
	int index = 0;
	int countdown = 1000;

	device_address = (uint8_t)((0xff00 & address) >> 8);
	device_register = (uint8_t)(0xff & address);
	index = (0xffff0000 & address) >> 16;
	smbus_address = card_get_register_address(card, DAG_REG_SMB, index);
	
	status_register = drb_smbus_busy_wait(card, address, &countdown);
	if (countdown == 0)
	{
		dagutil_verbose_level(1,"drb_smbus: busy wait timeout!\n");
		return -1;
	}
	register_value |= device_address;
	register_value |= device_register << 8;
	register_value |= BIT24;
	card_write_iom(card, smbus_address, register_value);
	countdown = 1000;
	status_register = drb_smbus_busy_wait(card, address, &countdown);
	
	if (countdown == 0)
	{
		dagutil_verbose_level(1,"drb_smbus: busy wait timeout!\n");
		return -1;
	}

	/* check if the command succeeded or failed */
	if (status_register & (BIT5 | BIT3 | BIT2))
	{
		dagutil_verbose_level(1, "drb_smbus: command failed!\n"); 
		return -1;
	}
	
	ret_val = (status_register & 0xff0000) >> 16;

	return ret_val;

}
