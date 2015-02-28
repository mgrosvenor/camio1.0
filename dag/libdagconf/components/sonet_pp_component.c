/*
 * Copyright (c) 2007 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 * sonet packet procesing it includes deframer demaper and handles all the levels up to POS or ATM extraction 
 * At the moment only POS is supported 

 * This component is used by DAG 5.2 SXA - probably others will follow.
 * is used by DAG 5.0s , DAG 8.1sxa
 *
 * $Id: sonet_pp_component.c,v 1.21.2.2 2009/11/06 02:29:17 jomi.gregory Exp $
 */


#include "dagutil.h"

#include "../include/attribute.h"
#include "../include/util/set.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/util/enum_string_table.h"
#include "../include/components/sonet_pp_component.h"
#include "../include/components/gpp_component.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"


/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: sonet_pp_component.c,v 1.21.2.2 2009/11/06 02:29:17 jomi.gregory Exp $";
static const char* const kRevisionString = "$Revision: 1.21.2.2 $";

static void sonet_pp_dispose(ComponentPtr component);
static void sonet_pp_reset(ComponentPtr component);
static void sonet_pp_default(ComponentPtr component);
static int sonet_pp_post_initialize(ComponentPtr component);
static dag_err_t sonet_pp_update_register_base(ComponentPtr component);

static void crc_to_string_routine(AttributePtr attribute);
static void crc_from_string_routine(AttributePtr attribute, const char* string);
static void* crc_get_value(AttributePtr attribute);
static void crc_set_value(AttributePtr attribute, void* value, int length);

static void refresh_cache_set_value(AttributePtr attribute, void* value, int length);
static void* refresh_cache_get_value(AttributePtr attribute);
static void line_rate_set_value(AttributePtr attribute, void* value, int length);
static void* line_rate_get_value(AttributePtr attribute);
static void line_rate_to_string_routine(AttributePtr attribute);
static void line_rate_from_string_routine(AttributePtr attribute, const char* string);

static void network_mode_to_string_routine(AttributePtr attribute);
static void network_mode_from_string_routine(AttributePtr attribute, const char* string);
static void* network_mode_get_value(AttributePtr attribute);
static void network_mode_set_value(AttributePtr attribute, void* value, int length);

static AttributePtr get_new_loss_of_cell_delineation_attribute(ComponentPtr component);
static AttributePtr get_new_pass_idle_attribute(ComponentPtr component);
static AttributePtr get_new_payload_scramble_atm_attribute(ComponentPtr component);

static void payload_scramble_set_value(AttributePtr attribute, void* value, int length);
static void* payload_scramble_get_value(AttributePtr attribute);

typedef enum
{
    // SONET Deframer/Framer
    kControl            = 0x00,
    kStatus             = 0x04,
    kB1ErrorCounter     = 0x08,
    kB2ErrorCounter     = 0x0c,
    kB3ErrorCounter     = 0x10,
    kREICounter         = 0x14,

    kOverheadInfo       = 0x18,
    kTimerAndRevision   = 0x1c,

    // SONET Demapper/Mapper
    kSonetDemapperConfig = 0x20, 
    kSonetDemapperStatus = 0x24, 

    // PoS Demapper/Mapper
    kPosDemapperConfig  = 0x30, 
    kPosDemapperStatus  = 0x34, 

    // ATM Demapper
    kATMDemapperConfig   = 0x38,

} sonet_pp_register_offset_t;

/**
 * sonet pp's Attribute definition array 
 * sonet packet procesing it includes deframer demaper and handles all the levels up to POS or ATM extraction 
 * At the moment only POS is supported 
 */
Attribute_t sonet_pp_attr[]=
{


    {
        /* Name */                 "ais",
        /* Attribute Code */       kBooleanAttributeAlarmIndicationSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Alarm Indication Signal.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kSonetDemapperStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "b1",
        /* Attribute Code */       kBooleanAttributeB1Error,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "B1 error has occured.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    3,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT3,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "b1_error_count",
        /* Attribute Code */       kUint32AttributeB1ErrorCount,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of times B1 error has occured",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kB1ErrorCounter,
        /* Size/length */          1,
        /* Read */                 grw_sonet_pp_b1_cache_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFFFFFF,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "b2",
        /* Attribute Code */       kBooleanAttributeB2Error,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "B2 error has occurred.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    4,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT4,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "b2_error_count",
        /* Attribute Code */       kUint32AttributeB2ErrorCount,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of times B2 error has occured",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kB2ErrorCounter,
        /* Size/length */          1,
        /* Read */                 grw_sonet_pp_b2_cache_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFFFFFF,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "b3",
        /* Attribute Code */       kBooleanAttributeB3Error,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "B3 error has occurred.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    5,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT5,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,

    },

    {
        /* Name */                 "b3_error_count",
        /* Attribute Code */       kUint32AttributeB3ErrorCount,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of times B3 error has occured",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kB3ErrorCounter,
        /* Size/length */          1,
        /* Read */                 grw_sonet_pp_b3_cache_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFFFFFF,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "counter_latch",
        /* Attribute Code */       kBooleanAttributeCounterLatch,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Latch the statistics counters",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    31,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT31,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "c2_path_label",
        /* Attribute Code */       kUint32AttributeC2PathLabel,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "C2 Path Label",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    8,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kOverheadInfo,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x0000ff00,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_hex_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "crc_select",
        /* Attribute Code */       kUint32AttributeCrcSelect,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Select the crc size.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    5,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kPosDemapperConfig,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT5|BIT6,
        /* Default Value */        0,
        /* SetValue */             crc_set_value,
        /* GetValue */             crc_get_value,
        /* SetToString */          crc_to_string_routine,
        /* SetFromString */        crc_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "crc_error",
        /* Attribute Code */       kBooleanAttributeCRCError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "CRC error",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kPosDemapperStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "fifo_empty",
        /* Attribute Code */       kBooleanAttributeRxFIFOEmpty,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Receive FIFO empty",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    1,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kPosDemapperStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT1,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
 
    {
        /* Name */                 "fifo_full",
        /* Attribute Code */       kBooleanAttributeTxFIFOFull,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Transmit FIFO full",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kPosDemapperStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT2,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
 
    {
        /* Name */                 "fifo_overflow",
        /* Attribute Code */       kBooleanAttributeTxFIFOOverflow,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Transmit FIFO overflow",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    3,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kPosDemapperStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT3,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
 
    {
        /* Name */                 "j0_path_label",
        /* Attribute Code */       kUint32AttributeJ0PathLabel,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Regenerator section trace, J0",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    16,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kOverheadInfo,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x00FF0000,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "j1_path_label",
        /* Attribute Code */       kUint32AttributeJ1PathLabel,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Path trace, J1",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    24,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kOverheadInfo,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFF000000,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "lof",
        /* Attribute Code */       kBooleanAttributeLossOfFrame,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Loss Of Frame",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    1,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT1,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "lop",
        /* Attribute Code */       kBooleanAttributeLossOfPointer,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Loss Of Pointer",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    1,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kSonetDemapperStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT1,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "los",
        /* Attribute Code */       kBooleanAttributeLossOfSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Loss Of Signal",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "mode_select",
        /* Attribute Code */       kBooleanAttributeSonetMode,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Select between Sonet(1) or SDH(0) mode",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    8,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT8,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "oof",
        /* Attribute Code */       kBooleanAttributeOutOfFrame,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Out Of Frame",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT2,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "atm_demapper_available",
        /* Attribute Code */       kBooleanAttributeATMDemapperAvailable,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Shows if the ATM Demapper is present",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    20,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT20,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "refresh_cache",
        /* Attribute Code */       kBooleanAttributeRefreshCache,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Latch the ext stats",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    31,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT31,
        /* Default Value */        0,
        /* SetValue */             refresh_cache_set_value,
        /* GetValue */             refresh_cache_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "scramble", /* scramble frame */
        /* Attribute Code */       kBooleanAttributeScramble,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Scrambling of SONET/SDH frames",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    12,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kControl,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT12,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },
/* This pscamble is the actual payload scramble it is different from scramble which includes scable of the 
hole sonet on the line including the headers*/
    {
        /* Name */                 "pos_pscramble", /* scramble packet */
        /* Attribute Code */       kBooleanAttributePayloadScramblePoS,
        /* Attribute Type */       kAttributeBoolean,
	/* NOTE:  We want in the future if ATM is implemented to be the same bit for ATM scramble (payload scamble means 
	 ATM or POS level scrambling and does not inlcude the exception where the ATM header in ATM mode can be unscrambled and ATM payload scrambled 
	* for that exception is another attribute header scramble not implemented for any other card except 37d make it consistant with dag 37d 
	in the future 	*/
	
        /* Description */          "POS payload scrambling",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    4,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kPosDemapperConfig,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT4,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "rei_error",
        /* Attribute Code */       kBooleanAttributeREIError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Remote Error Indicator",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    6,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT6,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "rei_error_count",
        /* Attribute Code */       kUint32AttributeREIErrorCount,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Number of times REI error has occured",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kREICounter,
        /* Size/length */          1,
        /* Read */                 grw_sonet_pp_rei_cache_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xFFFFFFFF,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "rdi_error",
        /* Attribute Code */       kBooleanAttributeRDIError,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Remote Defect Indicator",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    7,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT7,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "line_rate", 
        /* Attribute Code */       kUint32AttributeLineRate,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Line rate",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    4,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT4|BIT5|BIT6,
        /* Default Value */        kLineRateOC3c,
        /* SetValue */             line_rate_set_value,
        /* GetValue */             line_rate_get_value,
        /* SetToString */          line_rate_to_string_routine,
        /* SetFromString */        line_rate_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "network_mode",
        /* Attribute Code */       kUint32AttributeNetworkMode,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "POS or ATM or RAW mode if supported",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    28,
        /* Register Address */     DAG_REG_SONET_PP,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT28|BIT29|BIT30,
        /* Default Value */        kNetworkModePoS,
        /* SetValue */             network_mode_set_value,
        /* GetValue */             network_mode_get_value,
        /* SetToString */          network_mode_to_string_routine,
        /* SetFromString */        network_mode_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

/* This pscamble is the actual payload scramble it is different from scramble which includes scable of the 
hole sonet on the line including the headers*/
    {
        /* Name */                 "pscramble", /* scramble packet */
        /* Attribute Code */       kBooleanAttributePayloadScramble,
        /* Attribute Type */       kAttributeBoolean,
	/* NOTE:  We want in the future if ATM is implemented to be the same bit for ATM scramble (payload scamble means 
	 ATM or POS level scrambling and does not inlcude the exception where the ATM header in ATM mode can be unscrambled and ATM payload scrambled 
	* for that exception is another attribute header scramble not implemented for any other card except 37d make it consistant with dag 37d 
	in the future 	*/
	
        /* Description */          "POS or ATM scrambling",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,			// Don't care
        /* Register Address */     DAG_REG_SONET_PP,	// Don't care
        /* Offset */               0,			// Don't care
        /* Size/length */          1,			// Don't care
        /* Read */                 NULL,		// Don't care
        /* Write */                NULL,		// Don't care
        /* Mask */                 0,			// Don't care
        /* Default Value */        1,
        /* SetValue */             payload_scramble_set_value,
        /* GetValue */             payload_scramble_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    }

};

/* Number of elements in array */
#define NB_ELEM (sizeof(sonet_pp_attr) / sizeof(Attribute_t))

ComponentPtr
sonet_pp_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentSonetPP, card); 
    sonet_pp_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, sonet_pp_dispose);
        component_set_post_initialize_routine(result, sonet_pp_post_initialize);
        component_set_reset_routine(result, sonet_pp_reset);
        component_set_default_routine(result, sonet_pp_default);
        component_set_update_register_base_routine(result, sonet_pp_update_register_base);
        component_set_name(result, "sonet_pp");
        state = (sonet_pp_state_t*)malloc(sizeof(sonet_pp_state_t));
        memset( state, 0, sizeof(sonet_pp_state_t));
        state->mIndex = index;
		state->mBase = card_get_register_address(card, DAG_REG_SONET_PP, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
sonet_pp_dispose(ComponentPtr component)
{
}

static dag_err_t
sonet_pp_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        sonet_pp_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_SONET_PP, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
sonet_pp_post_initialize(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        sonet_pp_state_t* state = NULL;
        AttributePtr attribute = NULL;
        GenericReadWritePtr grw = NULL;  
        bool atm_demapper_available = 0;
        AttributePtr loss_of_cell_delineation = NULL;
        AttributePtr pass_idle = NULL;
        AttributePtr payload_scramble_atm = NULL;

        state = component_get_private_state(component);
        /* Add attribute of sonet_pp */ 
        read_attr_array(component, sonet_pp_attr, NB_ELEM, state->mIndex);

	/* If ATM demapper is present, add corresponding attributes */
	attribute = component_get_attribute(component, kBooleanAttributeATMDemapperAvailable);
	atm_demapper_available = *(bool *)attribute_boolean_get_value(attribute);
	
	if (atm_demapper_available)
	{
		/* Add the following attributes */
		loss_of_cell_delineation = get_new_loss_of_cell_delineation_attribute(component);
		pass_idle = get_new_pass_idle_attribute(component);
		payload_scramble_atm = get_new_payload_scramble_atm_attribute(component);
	
		component_add_attribute(component, loss_of_cell_delineation);
		component_add_attribute(component, pass_idle);
		component_add_attribute(component, payload_scramble_atm);

		/* Set the payload scramble attribute with inverted boolean */
		attribute = component_get_attribute(component, kBooleanAttributePayloadScrambleATM);
		grw = attribute_get_generic_read_write_object(attribute);
		grw_set_on_operation(grw, kGRWClearBit);
	}

	/* set attribute with inverted boolean
	 * Usually set a bit means "write 1", but in these cases it means "write 0" */
	attribute = component_get_attribute(component, kBooleanAttributeScramble);
	grw = attribute_get_generic_read_write_object(attribute);
	grw_set_on_operation(grw, kGRWClearBit);
	
	// NOTE: Not needed anymore??
	//attribute = component_get_attribute(component, kBooleanAttributePayloadScramble);
	//grw = attribute_get_generic_read_write_object(attribute);
	//grw_set_on_operation(grw, kGRWClearBit);

	attribute = component_get_attribute(component, kBooleanAttributePayloadScramblePoS);
	grw = attribute_get_generic_read_write_object(attribute);
	grw_set_on_operation(grw, kGRWClearBit);

	return 1;
    }
    return kDagErrInvalidParameter;
}

static void
sonet_pp_reset(ComponentPtr component)
{   
    if (1 == valid_component(component))
    {
        /* there is no reset bit available */
        sonet_pp_default(component);
    }	
}


static void
sonet_pp_default(ComponentPtr component)
{

    //set all the attribute default values
    if (1 == valid_component(component))
    {
	int count = component_get_attribute_count(component);
        int index;
        
        for (index = 0; index < count; index++)
        {
           AttributePtr attribute = component_get_indexed_attribute(component, index); 
           void* val = attribute_get_default_value(attribute);
           attribute_set_value(attribute, &val, 1);
        }
	
    }	
}

static AttributePtr
get_new_loss_of_cell_delineation_attribute(ComponentPtr component)
{
    sonet_pp_state_t* state = NULL;
    DagCardPtr card = NULL;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    GenericReadWritePtr grw = NULL; 
    uint32_t masks = BIT4;
    uint32_t length = 1;
    uint32_t default_value = 0;
    AttributePtr result = attribute_init(kBooleanAttributeLossOfCellDelineation); 
    
    if (NULL != result)
    {
        state = component_get_private_state(component);
        card = component_get_card(component);
        base_reg = card_get_register_address(card, DAG_REG_SONET_PP, state->mIndex);
        address = ((uintptr_t)card_get_iom_address(card) + base_reg + kATMDemapperConfig);
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        grw_set_attribute(grw, result);

        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "lcd");
        attribute_set_description(result, "Shows if the ATM Demapper experiencing loss of cell alignment.");
        attribute_set_config_status(result, kDagAttrStatus);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_masks(result, &masks, length);
        attribute_set_default_value (result, (void *)&default_value);

    }
    return result;
}

static AttributePtr
get_new_pass_idle_attribute(ComponentPtr component)
{
    sonet_pp_state_t* state = NULL;
    DagCardPtr card = NULL;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    GenericReadWritePtr grw = NULL; 
    uint32_t masks = BIT1;
    uint32_t length = 1;
    uint32_t default_value = 0;
    AttributePtr result = attribute_init(kBooleanAttributeIdleCellMode); 
    
    if (NULL != result)
    {
        state = component_get_private_state(component);
        card = component_get_card(component);
        base_reg = card_get_register_address(card, DAG_REG_SONET_PP, state->mIndex);
        address = ((uintptr_t)card_get_iom_address(card) + base_reg + kATMDemapperConfig);
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        grw_set_attribute(grw, result);

        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "aidle");
        attribute_set_description(result, "Enable/disable passing of ATM idle cells.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_masks(result, &masks, length);
        attribute_set_default_value (result, (void *)&default_value);
    }
    return result;
}

static AttributePtr
get_new_payload_scramble_atm_attribute(ComponentPtr component)
{
    sonet_pp_state_t* state = NULL;
    DagCardPtr card = NULL;
    uintptr_t base_reg = 0;
    uintptr_t address = 0;
    GenericReadWritePtr grw = NULL; 
    uint32_t masks = BIT0;
    uint32_t length = 1;
    uint32_t default_value = 1;
    AttributePtr result = attribute_init(kBooleanAttributePayloadScrambleATM); 
    
    if (NULL != result)
    {
        state = component_get_private_state(component);
        card = component_get_card(component);
        base_reg = card_get_register_address(card, DAG_REG_SONET_PP, state->mIndex);
        address = ((uintptr_t)card_get_iom_address(card) + base_reg + kATMDemapperConfig);
        grw = grw_init(card, address, grw_iom_read, grw_iom_write);
        grw_set_attribute(grw, result);

        attribute_set_setvalue_routine(result, attribute_boolean_set_value);
        attribute_set_getvalue_routine(result, attribute_boolean_get_value);
        attribute_set_to_string_routine(result, attribute_boolean_to_string);
        attribute_set_from_string_routine(result, attribute_boolean_from_string);
        attribute_set_name(result, "atm_pscramble");
        attribute_set_description(result, "ATM payload scrambling.");
        attribute_set_config_status(result, kDagAttrConfig);
        attribute_set_valuetype(result, kAttributeBoolean);
        attribute_set_generic_read_write_object(result, grw);
        attribute_set_masks(result, &masks, length);
        attribute_set_default_value (result, (void *)&default_value);
    }
    return result;
}

static void payload_scramble_set_value(AttributePtr attribute, void* value, int length)
{
    ComponentPtr component = NULL;
    AttributePtr any_attribute = NULL;
    bool atm_demapper_available = 0;

    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        any_attribute = component_get_attribute(component, kBooleanAttributePayloadScramblePoS); 
        attribute_boolean_set_value(any_attribute, value, 1);

        any_attribute = component_get_attribute(component, kBooleanAttributeATMDemapperAvailable);
        atm_demapper_available = *(bool *)attribute_boolean_get_value(any_attribute);

        if (atm_demapper_available)
        {
            any_attribute = component_get_attribute(component, kBooleanAttributePayloadScrambleATM);
            attribute_boolean_set_value(any_attribute, value, 1);
        }
    }
}

static void* payload_scramble_get_value(AttributePtr attribute)
{
    ComponentPtr component = NULL;
    AttributePtr any_attribute = NULL;
    bool atm_demapper_available = 0;
    bool return_value = 0;

    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        any_attribute = component_get_attribute(component, kBooleanAttributePayloadScramblePoS); 
        return_value = *(bool *)attribute_boolean_get_value(any_attribute);

        any_attribute = component_get_attribute(component, kBooleanAttributeATMDemapperAvailable);
        atm_demapper_available = *(bool *)attribute_boolean_get_value(any_attribute);
        if (atm_demapper_available)
        {
                any_attribute = component_get_attribute(component, kBooleanAttributePayloadScrambleATM);
                return_value |= *(bool *)attribute_boolean_get_value(any_attribute);
        }
    }
    attribute_set_value_array(attribute, &return_value, sizeof(bool));
    return (void *)attribute_get_value_array(attribute);
}

static void*
crc_get_value(AttributePtr attribute)
{
   crc_t crc = kCrcInvalid;
   uint32_t register_value = 0;
   GenericReadWritePtr crc_grw = NULL;
   uint32_t *masks, mask;
   
   if (1 == valid_attribute(attribute))
   {
       crc_grw = attribute_get_generic_read_write_object(attribute);
	   register_value = grw_read(crc_grw);
       masks = attribute_get_masks(attribute);
       mask = masks[0];
       register_value &= mask;
       while (!(mask & 1))
       {
           mask >>= 1;
           register_value >>= 1;
       }
	   switch(register_value)
	   {
		   case 0x00:
			   crc = kCrcOff;
			   break;
		   case 0x01:
			   crc = kCrc16;
			   break;
		   case 0x02:
			   crc = kCrc32;
			   break;
	   }
   }
   attribute_set_value_array(attribute, &crc, sizeof(crc));
   return (void *)attribute_get_value_array(attribute);
}


static void
crc_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card =NULL;
    uint32_t register_value = 0;
    GenericReadWritePtr crc_grw = NULL;    
    crc_t crc = *(crc_t*)value;
    uint32_t *masks, mask, new_mask = 0;
    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        crc_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(crc_grw);
        masks = attribute_get_masks(attribute);
        mask = masks[0];
        register_value &= ~(mask);
        switch(crc)
        {
           case kCrcInvalid:
              /* nothing */
               return;
           case kCrcOff:
               new_mask = 0;
               break;
           case kCrc16:
               new_mask = 1;
               break;
           case kCrc32:
               new_mask = 2;
               break;
        }

        while (!(mask & 1)) 
        {
            mask >>= 1;
            new_mask <<= 1;
        }
        register_value |= new_mask;
        grw_write(crc_grw, register_value);
    }
}

static void
crc_to_string_routine(AttributePtr attribute)
{
   void* temp = attribute_get_value(attribute);
   const char* string = NULL;
   crc_t crc;
   if (temp)
   {
       crc = *(crc_t*)temp;
       string = crc_to_string(crc);
       if (string)
           attribute_set_to_string(attribute, string);
  }
}

static void
crc_from_string_routine(AttributePtr attribute, const char* string)
{
  if (1 == valid_attribute(attribute))
  {
      if (string)
      {
          crc_t mode = string_to_crc(string);
          crc_set_value(attribute, (void*)&mode, sizeof(mode));
      }
  }
}


static void
refresh_cache_set_value(AttributePtr attribute, void* value, int length)
{
    if (1 == valid_attribute(attribute))
    {
        if (*(uint8_t*)value == 1)
        {
            sonet_pp_state_t* state = NULL;
            ComponentPtr comp = attribute_get_component(attribute);
            DagCardPtr card = attribute_get_card(attribute);

            state = component_get_private_state(comp);
            state->mStatusCache = card_read_iom(card, state->mBase + kStatus); 
            state->mSonetStatusCache = card_read_iom(card, state->mBase + kSonetDemapperStatus);
            state->mPosStatusCache = card_read_iom(card, state->mBase + kPosDemapperStatus);

            state->mB1ErrorCounterCache = card_read_iom(card, state->mBase + kB1ErrorCounter);
            state->mB2ErrorCounterCache = card_read_iom(card, state->mBase + kB2ErrorCounter);
            state->mB3ErrorCounterCache = card_read_iom(card, state->mBase + kB3ErrorCounter);
            state->mREICounterCache = card_read_iom(card, state->mBase + kREICounter);

        }
    }
}

static void*
refresh_cache_get_value(AttributePtr attribute)
{
     int32_t value;
    /* get value is not used */
    /* place holder to prevent clearing the register on read */
    if (1 == valid_attribute(attribute))
    {
	value = 0;
       attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);;
    }
    return NULL;
}

static void
line_rate_set_value(AttributePtr attribute, void* value, int length)
{
	uint32_t register_value;
	GenericReadWritePtr line_rate_grw = NULL;
	ComponentPtr component = NULL;
	AttributePtr mode_select = NULL;
	GenericReadWritePtr mode_select_grw = NULL;
	uint32_t mode_select_value = 0;
	line_rate_t val = *(line_rate_t*)value;
	uint32_t *masks, mask, new_mask = 0;

	if (1 == valid_attribute(attribute))
	{
		component = attribute_get_component (attribute);
	
		mode_select = component_get_attribute(component, kBooleanAttributeSonetMode);
		mode_select_grw = attribute_get_generic_read_write_object(mode_select);
		mode_select_value = grw_read(mode_select_grw);
		masks = attribute_get_masks(mode_select);
		mode_select_value = (mode_select_value & (*masks)) >> 8;

		line_rate_grw = attribute_get_generic_read_write_object(attribute);
		register_value = grw_read(line_rate_grw);
		masks = attribute_get_masks(attribute);
		mask = masks[0];
		register_value &= ~(mask);

		if (mode_select_value)
			switch (val)
			{
				case kLineRateOC3c:
					new_mask = 1;
					break;
				case kLineRateOC12c:
					new_mask = 2;
					break;
				case kLineRateOC48c:
					new_mask = 3;
					break;
				case kLineRateOC192c:
					new_mask = 4;
					break;
				default:
					return;
			}	
		else
			switch (val)
			{
				case kLineRateSTM1:
					new_mask = 1;
					break;
				case kLineRateSTM4:
					new_mask = 2;
					break;
				case kLineRateSTM16:
					new_mask = 3;
					break;
				case kLineRateSTM64:
					new_mask = 4;
					break;
				default:
					return;
			}	

		while (!(mask & 1))
		{
			mask >>= 1;
			new_mask <<= 1;
		}
		register_value |= new_mask;
		grw_write(line_rate_grw, register_value);
	}
}


static void*
line_rate_get_value(AttributePtr attribute)
{
	line_rate_t rate = kLineRateInvalid;
	uint32_t register_value = 0;
	GenericReadWritePtr line_rate_grw = NULL;
	ComponentPtr component = NULL;
	AttributePtr mode_select = NULL;
	GenericReadWritePtr mode_select_grw = NULL;
	uint32_t mode_select_value = 0;
	uint32_t *masks, mask;

	if (1 == valid_attribute(attribute))
	{
		component = attribute_get_component (attribute);
	
		mode_select = component_get_attribute(component, kBooleanAttributeSonetMode);
		mode_select_grw = attribute_get_generic_read_write_object(mode_select);
		mode_select_value = grw_read(mode_select_grw);
		masks = attribute_get_masks(mode_select);
		mode_select_value = (mode_select_value & (*masks)) >> 8;
	
		line_rate_grw = attribute_get_generic_read_write_object(attribute);
		register_value = grw_read(line_rate_grw);
		masks = attribute_get_masks(attribute);
		mask = masks[0];
		register_value &= mask;
		while (!(mask & 1))
		{
			mask >>= 1;
			register_value >>= 1;
		}

		if (mode_select_value)
			switch(register_value)
			{
				case 0x00:
					rate = kLineRateOC1c;
					break;
				case 0x01:
					rate = kLineRateOC3c;
					break;
				case 0x02:
					rate = kLineRateOC12c;
					break;
				case 0x03:
					rate = kLineRateOC48c;
					break;
				case 0x04:
					rate = kLineRateOC192c;
					break;
			}
		else
			switch(register_value)
			{
				case 0x00:
					rate = kLineRateSTM0;
					break;
				case 0x01:
					rate = kLineRateSTM1;
					break;
				case 0x02:
					rate = kLineRateSTM4;
					break;
				case 0x03:
					rate = kLineRateSTM16;
					break;
				case 0x04:
					rate = kLineRateSTM64;
					break;
			}

		attribute_set_value_array(attribute, &rate, sizeof(rate));
		return (void *)attribute_get_value_array(attribute);
	}
	return NULL;
}

static void
line_rate_to_string_routine(AttributePtr attribute)
{
    void* temp = attribute_get_value(attribute);
    const char* string = NULL;
    line_rate_t lr;
    if (temp)
    {
        lr = *(line_rate_t*)temp;
        string = line_rate_to_string(lr);
        if (string)
            attribute_set_to_string(attribute, string);
    }
}

static void
line_rate_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        line_rate_t mode = string_to_line_rate(string);
        line_rate_set_value(attribute, (void*)&mode, sizeof(mode));
    }
}

static void
network_mode_to_string_routine(AttributePtr attribute)
{
    network_mode_t nm = *(network_mode_t*)attribute_get_value(attribute);
    if (1 == valid_attribute(attribute))
    {
        const char* temp = NULL;
        temp = network_mode_to_string(nm);
        attribute_set_to_string(attribute, temp);
    }
}

static void
network_mode_from_string_routine(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute) && string != NULL)
    {
        network_mode_t nm = string_to_network_mode(string);
        network_mode_set_value(attribute, (void*)&nm, sizeof(nm));
    }
}

static void*
network_mode_get_value(AttributePtr attribute)
{
   network_mode_t mode = kNetworkModeInvalid;
   uint32_t register_value = 0;
   GenericReadWritePtr network_grw = NULL;
   uint32_t *masks, mask;
   
   if (1 == valid_attribute(attribute))
   {
       network_grw = attribute_get_generic_read_write_object(attribute);
       register_value = grw_read(network_grw);
       masks = attribute_get_masks(attribute);
       mask = masks[0];
       register_value &= mask;
       while (!(mask & 1))
       {
           mask >>= 1;
           register_value >>= 1;
       }
       switch(register_value)
       {
	      	case 0x00:
		   mode = kNetworkModePoS;
		   break;
		case 0x01:
		   mode = kNetworkModeRAW;
		   break;
		case 0x02:
		   mode = kNetworkModeATM;
		   break;
        case 0x03:
		   mode = kNetworkModeWAN;
		   break;
        default:
            mode = kNetworkModeInvalid;
            break;
       }
    }
    attribute_set_value_array(attribute, &mode, sizeof(mode));
   return (void *)attribute_get_value_array(attribute);
	   
}


static void
network_mode_set_value(AttributePtr attribute, void* value, int length)
{
    //DagCardPtr card =NULL;
    uint32_t register_value = 0;
    GenericReadWritePtr network_grw = NULL;    
    network_mode_t mode = *(network_mode_t*)value;
    uint32_t *masks, mask, new_mask = 0;

    if (1 == valid_attribute(attribute))
    {
        //card = attribute_get_card(attribute);
        network_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(network_grw);
        masks = attribute_get_masks(attribute);
        mask = masks[0];
        register_value &= ~(mask);
	switch (mode)
	{
	   case kNetworkModePoS:
		new_mask = 0;
	   	break;

	   case kNetworkModeRAW:
		new_mask = 1;
		break;

	   case kNetworkModeATM:
		new_mask = 2;
		break;
	   default:
		break;
	}	
        while (!(mask & 1)) 
        {
            mask >>= 1;
            new_mask <<= 1;
        }
        register_value |= new_mask;
        grw_write(network_grw, register_value);
    }
}

