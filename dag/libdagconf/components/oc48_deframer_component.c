/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 *
 * This component is used by DAG 5.0S (OC-48) and DAG8.0SX (OC-192) as well.
 *
 * $Id: oc48_deframer_component.c,v 1.14.20.1 2009/08/19 05:41:59 dlim Exp $
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
#include "../include/components/oc48_deframer_component.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/attribute_factory.h"
#include "../include/create_attribute.h"


/* CVS Header. */
static const char* const kCvsHeader __attribute__ ((unused)) = "$Id: oc48_deframer_component.c,v 1.14.20.1 2009/08/19 05:41:59 dlim Exp $";
static const char* const kRevisionString = "$Revision: 1.14.20.1 $";

static void oc48_deframer_dispose(ComponentPtr component);
static void oc48_deframer_reset(ComponentPtr component);
static void oc48_deframer_default(ComponentPtr component);
static int oc48_deframer_post_initialize(ComponentPtr component);
static dag_err_t oc48_deframer_update_register_base(ComponentPtr component);

static void crc_to_string_routine(AttributePtr attribute);
static void crc_from_string_routine(AttributePtr attribute, const char* string);
static void* crc_get_value(AttributePtr attribute);
static void crc_set_value(AttributePtr attribute, void* value, int length);

static void refresh_cache_set_value(AttributePtr attribute, void* value, int length);
static void* refresh_cache_get_value(AttributePtr attribute);
static void reset_set_value(AttributePtr attribute, void* value, int len);
static void* reset_get_value(AttributePtr attribute);


// DAG 5.0S
typedef enum
{
    kConfig             = 0x00,
    kStatus             = 0x04, // LOS, LOF, OOF, B1, B2, B3, LOP, AIS, LCD
    kB1ErrorCounter     = 0x08,
    kB2ErrorCounter     = 0x0c,
    kB3ErrorCounter     = 0x10,

    kRXMaxLen           = 0x18, // length checking
    kRXMinLen           = 0x1c,

    // statistics counters
    kRXPacketCounter    = 0x20, 
    kRXByteCounter      = 0x24, 
    kTXPacketCounter    = 0x28, 
    kTXByteCounter      = 0x2c, 

    kRXLongCounter      = 0x30,
    kRXShortCounter     = 0x34,
    kRXAbortsCounter    = 0x38,
    kRXFCSErrorCounter  = 0x3c,
} oc48_register_offset_t;

/**
 * OC48c Deframer's Attribute definition array 
 */
Attribute_t oc48_deframer_attr[]=
{

    {
        /* Name */                 "aborts",
        /* Attribute Code */       kUint32AttributeAborts,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Aborted packets counter",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXAbortsCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "b1",
        /* Attribute Code */       kBooleanAttributeB1Error,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "B1 error has occured.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    3,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
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
        /* Description */          "",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kB1ErrorCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
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
        /* Description */          "",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kB2ErrorCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
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
        /* Description */          "",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kB3ErrorCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "c2_path_label",
        /* Attribute Code */       kUint32AttributeC2PathLabel,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "C2 Path Label",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    24,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0xff000000,
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
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kConfig,
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
        /* Name */                 "crc_select",
        /* Attribute Code */       kUint32AttributeCrcSelect,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Select the crc size.",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    15,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kConfig,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT15|BIT16,
        /* Default Value */        0,
        /* SetValue */             crc_set_value,
        /* GetValue */             crc_get_value,
        /* SetToString */          crc_to_string_routine,
        /* SetFromString */        crc_from_string_routine,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "crc_errors", /* CRC == FCS */
        /* Attribute Code */       kUint32AttributeCRCErrors,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "CRC errors.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXFCSErrorCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "lais",
        /* Attribute Code */       kBooleanAttributeLineAlarmIndicationSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Line Alarm Indication Signal.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    13,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT13,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "lcd",
        /* Attribute Code */       kBooleanAttributeLossOfCellDelineation,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Loss Of Cell Delineation",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    14,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT14,
        /* Default Value */        0,
        /* SetValue */             attribute_boolean_set_value,
        /* GetValue */             attribute_boolean_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
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
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
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
        /* Index in register */    12,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT12,
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
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
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
        /* Name */                 "max_pkt_len",
        /* Attribute Code */       kUint32AttributeMaxPktLen,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "POS maximum packet length",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXMaxLen,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x3FFF,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "max_pkt_len_error",
        /* Attribute Code */       kUint32AttributeMaxPktLenError,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "RX long packet count",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXLongCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "min_pkt_len",
        /* Attribute Code */       kUint32AttributeMinPktLen,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "POS minimum packet length",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXMinLen,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 0x007F,
        /* Default Value */        0,
        /* SetValue */             attribute_uint32_set_value,
        /* GetValue */             attribute_uint32_get_value,
        /* SetToString */          attribute_uint32_to_string,
        /* SetFromString */        attribute_uint32_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "min_pkt_len_error",
        /* Attribute Code */       kUint32AttributeMinPktLenError,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "RX short packet count",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXShortCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "oof",
        /* Attribute Code */       kBooleanAttributeOutOfFrame,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Out Of Frame",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kStatus,
        /* Size/length */          1,
        /* Read */                 grw_oc48_deframer_status_cache_read,
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
        /* Name */                 "pcramble",
        /* Attribute Code */       kBooleanAttributePayloadScramble,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "POS scrambling",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    11,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kConfig,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT11,
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
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kConfig,
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

    /* this is tricky: aggregate of 3 bits in 2 registers */
    {
        /* Name */                 "reset",
        /* Attribute Code */       kBooleanAttributeReset,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Reset the component",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kConfig,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT0,
        /* Default Value */        0,
        /* SetValue */             reset_set_value,
        /* GetValue */             reset_get_value,
        /* SetToString */          attribute_boolean_to_string,
        /* SetFromString */        attribute_boolean_from_string,
        /* Dispose */              attribute_dispose,
        /* PostInit */             attribute_post_initialize,
    },

    {
        /* Name */                 "rx_bytes",
        /* Attribute Code */       kUint32AttributeRxBytes,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Received bytes.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXByteCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "rx_frames",
        /* Attribute Code */       kUint32AttributeRxFrames,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Received packets.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kRXPacketCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "scramble",
        /* Attribute Code */       kBooleanAttributeScramble,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "SPE scrambling",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    12,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kConfig,
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


    /* TODO: steer */


    {
        /* Name */                 "tx_bytes",
        /* Attribute Code */       kUint32AttributeTxBytes,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Transmitted bytes.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kTXByteCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
        /* Name */                 "tx_frames",
        /* Attribute Code */       kUint32AttributeTxFrames,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Transmitted packets.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    0,
        /* Register Address */     DAG_REG_OC48_DEFRAMER,
        /* Offset */               kTXPacketCounter,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
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
};

/* Number of elements in array */
#define NB_ELEM (sizeof(oc48_deframer_attr) / sizeof(Attribute_t))

/* terf component. */
ComponentPtr
oc48_deframer_get_new_component(DagCardPtr card, uint32_t index)
{
    ComponentPtr result = component_init(kComponentDemapper, card); 
    oc48_deframer_state_t* state = NULL;
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, oc48_deframer_dispose);
        component_set_post_initialize_routine(result, oc48_deframer_post_initialize);
        component_set_reset_routine(result, oc48_deframer_reset);
        component_set_default_routine(result, oc48_deframer_default);
        component_set_update_register_base_routine(result, oc48_deframer_update_register_base);
        component_set_name(result, "oc48_deframer");
        state = (oc48_deframer_state_t*)malloc(sizeof(oc48_deframer_state_t));
        state->mIndex = index;
		state->mBase = card_get_register_address(card, DAG_REG_OC48_DEFRAMER, state->mIndex);
        component_set_private_state(result, state);
    }
    
    return result;
}


static void
oc48_deframer_dispose(ComponentPtr component)
{
}

static dag_err_t
oc48_deframer_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        oc48_deframer_state_t* state = NULL;
        DagCardPtr card;

        state = component_get_private_state(component);
        card = component_get_card(component);
        NULL_RETURN_WV(state, kDagErrGeneral);
        state->mBase = card_get_register_address(card, DAG_REG_OC48_DEFRAMER, state->mIndex);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
oc48_deframer_post_initialize(ComponentPtr component)
{
     if (1 == valid_component(component))
    {
        oc48_deframer_state_t* state = NULL;

        state = component_get_private_state(component);
        /* Add attribute of oc48_deframer */ 
        read_attr_array(component, oc48_deframer_attr, NB_ELEM, state->mIndex);

	return 1;
    }
    return kDagErrInvalidParameter;
}

static void
oc48_deframer_reset(ComponentPtr component)
{   
    uint8_t val = 0;
    if (1 == valid_component(component))
    {
	AttributePtr attribute = component_get_attribute(component, kBooleanAttributeReset);
	attribute_set_value(attribute, (void*)&val, 1);
	oc48_deframer_default(component);
    }	
}


static void
oc48_deframer_default(ComponentPtr component)
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
            oc48_deframer_state_t* state = NULL;
            ComponentPtr comp = attribute_get_component(attribute);
            DagCardPtr card = attribute_get_card(attribute);

            state = component_get_private_state(comp);
            state->mStatusCache = card_read_iom(card, state->mBase + kStatus);

            state->mB1ErrorCounterCache = card_read_iom(card, state->mBase + kB1ErrorCounter);
            state->mB2ErrorCounterCache = card_read_iom(card, state->mBase + kB2ErrorCounter);
            state->mB3ErrorCounterCache = card_read_iom(card, state->mBase + kB3ErrorCounter);

            state->mRXPacketCounterCache = card_read_iom(card, state->mBase + kRXPacketCounter);
            state->mRXByteCounterCache = card_read_iom(card, state->mBase + kRXByteCounter);
            state->mTXPacketCounterCache = card_read_iom(card, state->mBase + kTXPacketCounter);
            state->mTXByteCounterCache = card_read_iom(card, state->mBase + kTXByteCounter);

            state->mRXLongCounterCache = card_read_iom(card, state->mBase + kRXLongCounter);
            state->mRXShortCounterCache = card_read_iom(card, state->mBase + kRXShortCounter);
            state->mRXAbortCounterCache = card_read_iom(card, state->mBase + kRXAbortsCounter);
            state->mRXFCSErrorCounterCache = card_read_iom(card, state->mBase + kRXFCSErrorCounter);
        }
    }
}

static void*
refresh_cache_get_value(AttributePtr attribute)
{
    /* get value is not used */
    /* place holder to prevent clearing the register on read */
    if (1 == valid_attribute(attribute))
    {
        return  0;
    }
    return NULL;
}

/*
 * There is no genereal reset bit on DAG 5.0S, we must reset the
 * framer, deframer and PHY separately
 */
static void
reset_set_value(AttributePtr attribute, void* value, int len)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t sdh_config_register; /* framer reset, deframer reset */
    uint32_t phy_config_register; /* PHY reset (negated!!!) */
    oc48_deframer_state_t* state = NULL;
    GenericReadWritePtr grw_phy = NULL;
    uintptr_t phy_address = 0;

    if (1 == valid_attribute(attribute))
    {
        /* read SDH config register */
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        sdh_config_register = card_read_iom(card, state->mBase + kConfig);

        /* read PHY control register */
        phy_address = card_get_register_address(card, DAG_REG_AMCC3485, 0); /* PHY A */
        phy_address = (uintptr_t)(card_get_iom_address(card) + phy_address);
        /* phy_address should be 0x0100 (A) or 0x0104 (B) */
        phy_address += (state->mIndex << 2);

        grw_phy = grw_init(card, phy_address, grw_iom_read, grw_iom_write);
        phy_config_register = grw_read(grw_phy);

        if (*(uint8_t*)value == 0)
        {
            sdh_config_register &= ~BIT6; /* framer reset */
            sdh_config_register &= ~BIT7; /* deframer reset */
            phy_config_register |= BIT11; /* PHY reset is active low */
        }
        else
        {
            sdh_config_register |= BIT6;
            sdh_config_register |= BIT7;
            phy_config_register &= ~BIT11;
        }
        card_write_iom(card, state->mBase + kConfig, sdh_config_register);
        grw_write(grw_phy, phy_config_register);
        free(grw_phy);
    }
}


/*
 * Return the reset value of the framer, deframer and PHY
 */
static void*
reset_get_value(AttributePtr attribute)
{
    DagCardPtr card = NULL;
    ComponentPtr port;
    uint32_t sdh_config_register; /* framer reset, deframer reset */
    uint32_t phy_config_register; /* PHY reset (negated!!!) */
    oc48_deframer_state_t* state = NULL;
    GenericReadWritePtr grw_phy = NULL;
    uintptr_t phy_address = 0;
    uint32_t value;

    if (1 == valid_attribute(attribute))
    {
        /* read SDH config register */
        card = attribute_get_card(attribute);
        port = attribute_get_component(attribute);
        state = component_get_private_state(port);
        sdh_config_register = card_read_iom(card, state->mBase + kConfig);

        /* read PHY control register */
        phy_address = card_get_register_address(card, DAG_REG_AMCC3485, 0); /* PHY A */
        phy_address = (uintptr_t)(card_get_iom_address(card) + phy_address);
        /* phy_address should be 0x0100 (A) or 0x0104 (B) */
        phy_address += (state->mIndex << 2);

        grw_phy = grw_init(card, phy_address, grw_iom_read, grw_iom_write);
        phy_config_register = grw_read(grw_phy);

        if ((sdh_config_register & (BIT6|BIT7)) || 
            !(phy_config_register & BIT11)) 
        {
            value = 1;
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return (void *)attribute_get_value_array(attribute);
    }
    return NULL;
}

