/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */
/* 
* this component is presented on all card since 5.0 onwards which has a 
* SFP or XFP modules at the moment the assumptions are :
* that the FW has to have enumeration entry for DAG_REG_XFP
* and version 0 is used for XFP modules, version 1 is used for SFP modules 
*/

/* -- DESCRIPTION XFP module version 0:
-- This module provides the DRB register and the signal connection
-- to the connected XFP module.
--
-- The module implements one DRB Register with the following bits:
-- Bit  Type    Description
--  0   RW      SCL - Serial Clock Line
--  1   RW      SDA - Serial Data Line
--  2   RO      Module absent signal; 1=Module is absent (it is 0 Module detect )
--  3   RO      Module not ready; 1=Module not ready - Not Implemented in SW
--  4   RW      Power down / Reset XFT; 1=Power Down
--  5   RW      Deselect module; 1=deselect  - Not implemented in SW
--  6   RW      Disable TX; 1=Disable
--  7   RO      RX LoS (Loss Of Signal); 1=Lost RX signal
--  8   RO      Interrupt Pending; 0=Pending interrupt - Not implemented 
*/

/*
A new module is introduced for the SFP
   Enum table: MOD_XFP/DAG_REG_XFP - Version 0x1
   Register amount: 1
   Bits:
    -- Bit    Type    Description
    --  0    RW    SDA - Serial Data Line
    --  1    RW    SCL - Serial Clock Line
    --  2    RO    Module absent signal; 1=Module is absent
    --  3    0    reserved. Current Assumption is this bit is equivalent to Module not ready, and should be implemented s TXFault maybe 
    --  4    0    reserved
    --  5    0    reserved
    --  6    RW    Disable TX / Laser off; 1=Disable/Laser off
    --  7    RO    LOS; 1=Lost of signal
    --  8    0    reserved
*/

/* Internal project headers. */
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/components/xfp_component.h"
#include "../include/create_attribute.h"

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

typedef struct
{
    uint32_t mBase;
    uint32_t mPortNumber;
    uint32_t mVersion;
} xfp_state_t;

/* detect attribute. */
//static void* xfp_detect_get_value(AttributePtr attribute);


typedef struct
{
    module_identifier_t module_type;
    const char* string;
} module_type_mapping_t;

static module_type_mapping_t table[] =
{
    {kInvalidModuleType ,"Invalid Module/No Module"},
    {kUnknown ,"Unknown"},
    {kGBIC, "GBIC"},
    {kSolderedConnector,"SolderedConnector"},
    {kSFP,"SFP"},
    {kXBI300Pin, "XBI300Pin"},
    {kXENPAK, "XENPAK"},
    {kXFP,"XFP"},
    {kXFF, "XFF"},
    {kXPFE, "XPFE"},
    {kXPAK ,"XPAK"},
    {kX2 ,"X2"},
};

static char* transceiver_compliance_strings_xfp[] =
{
    /*10G Ethernet Compliance Codes */
    "10GBASE-SR, ",
    "10GBASE-LR, ",
    "10GBASE-ER, ",
    "10GBASE-LRM, ",
    "10GBASE-SW, ",
    "10GBASE-LW, ",
    "10GBASE-EW, ",
    "",/*reserved*/
    /* 10 Gigabit Fibre Channel Compliance*/
    "1200-MX-SN-I, ",
    "1200-SM-LL-L, ",
    "ExtendedReach-1550nm, ",
    "IntermediateReach-1300nmFP, ",
    "",/*reserved*/
     "",/*reserved*/
     "",/*reserved*/
     "",/*reserved*/
    /* 10 Gigabit Copper Links*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    /* Lower Speed Links */
    "1000BASE-SX/1xFCMMF, ",
    "1000BASE-LX/1xFCSMF, ",
    "2xFCMMF, ",
    "2xFCSMF, ",
    "OC48SR, ",
    "OC48IR, ",
    "OC48LR, ",
    "",/*reserved */
    /*SONET/SDH Codes - Interconnect*/
    "I-64.1r, ",
    "I-64.1, ",
    "I-64.2r, ",
    "I-64.2, ",
    "I-64.3, ",
    "I-64.5, ",
    "",/*reserved*/
    "",/*reserved*/
    /*SONET/SDH Codes – Short Haul*/
    "S-64.1, ",
    "S-64.2a, ",
    "S-64.2b, ",
    "S-64.3a, ",
    "S-64.3b, ",
    "S-64.5a, ",
    "S-64.5b, ",
    "",/*reserved*/
    /* SONET/SDH Codes – Long Haul */
    "L-64.1, ",
    "L-64.2a, ",
    "L-64.2b, ",
    "L-64.2c, ",
    "L-64.3, ",
    "G.959.1P1L1-2D2, ",
    "",/*reserved*/
    "",/*reserved*/
    /* SONET/SDH Codes – Very Long Haul*/
    "V-64.2a, ",
    "V-64.2b, ",
    "V-64.3, ",
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
    "",/*reserved*/
};

       
static char* transceiver_compliance_strings[] =
{
    /*10G Ethernet Compliance Codes */
    "Unallocated, ",
    "10G Base-LRM, ",
    "10G Base-LR, ",
    "10G Base-SR, ",
    /*Infiniband Compliance Codes */
    "1X SX, ",
    "1X LX, ",
    "1X Copper Active, ",
    "1X Copper Passive, ",
    /*ESCON Compliance Codes*/
    "ESCON MMF 1310nm LED, ",
    "ESCON SMF 1310nm Laser, ",
    /*SONET Compliance Codes*/
    "OC-192 short reach2, ",
    "SONET reach specifier bit 1, ",
    "SONET reach specifier bit 2, ",
    "OC-48 long reach 2, ",
    "OC-48 intermediate reach 2, ",
    "OC-48 short reach2, ",
    "Unallocated, ",
    "OC-12 single mode, long reach 2, ",
    "OC-12 single mode, inter. reach 2, ",
    "OC-12 short reach 2, ",
    "Unallocated, ",
    "OC-3 single mode, long reach 2, ",
    "OC-3 single mode, inter. reach 2, ",
    "OC-3 short reach 2, ",
    /*  Ethernet Compliance Codes*/
    "BASE-PX 3, ",
    "BASE-BX10 3, ",
    "100BASE-FX, ",
    "100BASE-LX/LX10, ",
    "1000BASE-T, ",
    "1000BASE-CX, ",
    "1000BASE-LX 3, ",
    "1000BASE-SX , ",
};
static char* fibre_link_length_strings[] =
{
    "very long distance (V) ",
    "short distance (S) ",
    "intermediate distance (I) ",
    "long distance (L) ",
    "medium distance (M) ",
};

static char* fibre_tr_media_strings[] = 
{
    "Twin Axial Pair (TW) ",
    "Twisted Pair (TP) ",
    "Miniature Coax (MI) ",
    "Video Coax (TV) ",
    "Multimode, 62.5um (M6) ",
    "Multimode, 50um (M5)",
    "Unallocated ",
    "Single Mode (SM)"
};
static const char* module_type_to_string(module_identifier_t type);
/* xfp  */
static void xfp_dispose(ComponentPtr component);
static void xfp_reset(ComponentPtr component);
static void xfp_default(ComponentPtr component);
static int xfp_post_initialize(ComponentPtr component);
static dag_err_t xfp_update_register_base(ComponentPtr component);

/* signal */
//static void* xfp_los_get_value(AttributePtr attribute);

/* sfppwr */
//static void* xfp_pwr_get_value(AttributePtr attribute);
//static void xfp_pwr_set_value(AttributePtr attribute, void* value, int length);

static void* module_identifier_get_value(AttributePtr attribute);
static void module_identifier_to_string(AttributePtr attribute);

static void* extended_module_identifier_get_value(AttributePtr attribute);

static void* transceiver_codes_get_value(AttributePtr attribute);
//static void transceiver_code_to_string(AttributePtr attribute);
static void* vendor_name_get_value(AttributePtr attribute);
static void* vendor_pn_get_value(AttributePtr attribute);
static void common_to_string_function(AttributePtr attribute);

static void* link_length_get_value(AttributePtr attribute);
static void* transmission_media_get_value(AttributePtr attribute);
static void* diagnostic_monitoring_get_value(AttributePtr attribute);

static void* transceiver_temperature_get_value(AttributePtr attribute); 
static void* transceiver_voltage_get_value(AttributePtr attribute);
static void* transceiver_tx_power_get_value(AttributePtr attribute);
static void* transceiver_rx_power_get_value(AttributePtr attribute);
static uint8_t is_ext_calibarated(DagCardPtr card, xfp_state_t* state);
Attribute_t xfp_sfp_attr[]=
{
   
   {
        /* Name */                 "detect",
        /* Attribute Code */       kBooleanAttributeDetect,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Detect presence of XFP/SFP module.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,
        /* Register Address */     DAG_REG_XFP,
        /* Offset */               0,
        /* Size/length */          1,
        /* Read */                 grw_iom_read,
        /* Write */                grw_iom_write,
        /* Mask */                 BIT2,
        /* Default Value */        1,
        /* SetValue */             attribute_boolean_set_value,
	/* GetValue */             attribute_boolean_get_value,
	/* SetToString */          attribute_boolean_to_string,
	/* SetFromString */        attribute_boolean_from_string,
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
 
	{
        /* Name */                 "physical_los",
        /* Attribute Code */       kBooleanAttributeLossOfSignal,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Loss of signal from the SFP/XFP module.",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    7,
        /* Register Address */     DAG_REG_XFP,
        /* Offset */               0,
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
	/*This attribute is only for XFP */
	{
        /* Name */                 "laser",
        /* Attribute Code */       kBooleanAttributeLaser,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Shows if laser or TX is powered",
        /* Config-Status */        kDagAttrConfig,
        /* Index in register */    6,
        /* Register Address */     DAG_REG_XFP,
        /* Offset */               0,
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
};

Attribute_t xfp_attr[]=
{

	/*This attribute is only for XFP */
	{
	/* Name */                 "optics_pwr",
	/* Attribute Code */       kBooleanAttributeSfpPwr,
	/* Attribute Type */       kAttributeBoolean,
	/* Description */          "Shows if module is powered",
	/* Config-Status */        kDagAttrConfig,
	/* Index in register */    4,
	/* Register Address */     DAG_REG_XFP,
	/* Offset */               0,
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
};
Attribute_t module_attr[] = 
{
    {
        /* Name */                 "module_identifier",
        /* Attribute Code */       kUint32AttributeTransceiverIdentifier,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Module type identifier",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             module_identifier_get_value,
	/* SetToString */          module_identifier_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "extended_identifier",
        /* Attribute Code */       kUint32AttributeTransceiverExtendedIdentifier,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Extended Module type identifier",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             extended_module_identifier_get_value,
	/* SetToString */          attribute_uint32_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
    
    {
        /* Name */                 "vendor_name",
        /* Attribute Code */       kStringAttributeTransceiverVendorName,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Vendor name of the module ",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             vendor_name_get_value,
	/* SetToString */          common_to_string_function,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "vendor_pn",
        /* Attribute Code */       kStringAttributeTransceiverVendorPN,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Vendor PN ",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             vendor_pn_get_value,
	/* SetToString */          common_to_string_function,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "transceiver_compliances",
        /* Attribute Code */       kStringAttributeTransceiverCodes,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Transceiver Compliance",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             transceiver_codes_get_value,
	/* SetToString */          common_to_string_function,//transceiver_code_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "link_length",
        /* Attribute Code */       kStringAttributeTransceiverLinkLength,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Fibre Channel link length",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             link_length_get_value,
	/* SetToString */          common_to_string_function,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "transmission_media",
        /* Attribute Code */       kStringAttributeTransceiverMedia,
        /* Attribute Type */       kAttributeString,
        /* Description */          "Fibre Channel media",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             transmission_media_get_value,
	/* SetToString */          common_to_string_function,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
{
        /* Name */                 "diagnostic_monitoring",
        /* Attribute Code */       kBooleanAttributeTransceiverMonitoring,
        /* Attribute Type */       kAttributeBoolean,
        /* Description */          "Indicates diagnostic monitoring has been implemented",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             diagnostic_monitoring_get_value,
	/* SetToString */          attribute_boolean_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },

};
/* following attributes are to be added only if the diagnostic_monitoring is set */
Attribute_t module_diagnostic_monitor_attributes[] =
{
    {
        /* Name */                 "module_temperature",
        /* Attribute Code */       kFloatAttributeTransceiverTemperature,
        /* Attribute Type */       kAttributeFloat,
        /* Description */          "Transceiver temperature",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             transceiver_temperature_get_value,
	/* SetToString */          attribute_float_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
   {
        /* Name */                 "supply_voltage",
        /* Attribute Code */       kUint32AttributeTransceiverVoltage,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Transceiver supply voltage",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             transceiver_voltage_get_value,
	/* SetToString */          attribute_uint32_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
    {
        /* Name */                 "rx_power",
        /* Attribute Code */       kUint32AttributeTransceiverRxPower,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Internally measured RX Power",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             transceiver_rx_power_get_value,
	/* SetToString */          attribute_uint32_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
{
        /* Name */                 "tx_power",
        /* Attribute Code */       kUint32AttributeTransceiverRxPower,
        /* Attribute Type */       kAttributeUint32,
        /* Description */          "Internally measured TX Power",
        /* Config-Status */        kDagAttrStatus,
        /* Index in register */    2,/* dont care */
        /* Register Address */     DAG_REG_XFP, /* dont care */
        /* Offset */               0,/* dont care */
        /* Size/length */          1,/* dont care */
        /* Read */                 NULL,/* dont care */
        /* Write */                NULL,/* dont care */
        /* Mask */                 BIT2,/* dont care */
        /* Default Value */        1,/* dont care */
        /* SetValue */             attribute_boolean_set_value,/* Status attribute -funtion not needed*/
	/* GetValue */             transceiver_tx_power_get_value,
	/* SetToString */          attribute_uint32_to_string,
	/* SetFromString */        attribute_boolean_from_string,/* Status attribute -funtion not needed*/
	/* Dispose */              attribute_dispose,
	/* PostInit */             attribute_post_initialize,
    },
};
/* Number of elements in array */
#define NB_ELEM_XFP (sizeof(xfp_attr) / sizeof(Attribute_t))
#define NB_ELEM_XFP_SFP (sizeof(xfp_sfp_attr) / sizeof(Attribute_t))
#define NB_ELEM_MODULE_ATTR (sizeof(module_attr) / sizeof(Attribute_t))
#define NB_ELEM_MODULE_MONITOR_ATTR (sizeof(module_diagnostic_monitor_attributes) / sizeof(Attribute_t))

#define BUFFER_SIZE 1024

ComponentPtr
xfp_get_new_component(DagCardPtr card, uint32_t port_number)
{
    ComponentPtr result = component_init(kComponentOptics, card); 
    xfp_state_t* state = NULL;
    char buffer[BUFFER_SIZE];
    
    if (NULL != result)
    {
        component_set_dispose_routine(result, xfp_dispose);
        component_set_post_initialize_routine(result, xfp_post_initialize);
        component_set_reset_routine(result, xfp_reset);
        component_set_default_routine(result, xfp_default);
        component_set_update_register_base_routine(result, xfp_update_register_base);
        sprintf(buffer, "optics%d", port_number);
        component_set_name(result, buffer);
        state = (xfp_state_t*)malloc(sizeof(xfp_state_t));
        memset(state, 0, sizeof(xfp_state_t));
        state->mPortNumber = port_number;
	state->mBase = card_get_register_address(card, DAG_REG_XFP, state->mPortNumber);
	/* Note version 0 means XFP, version 1 means SFP */
	state->mVersion = card_get_register_version(card, DAG_REG_XFP, state->mPortNumber);
	
        component_set_private_state(result, state);
        
    }
    
    return result;
}


static void
xfp_dispose(ComponentPtr component)
{
}

static dag_err_t
xfp_update_register_base(ComponentPtr component)
{
    if (1 == valid_component(component))
    {
        DagCardPtr card = NULL;
		xfp_state_t* state = (xfp_state_t*)component_get_private_state(component);
		/* Get card reference */
		card = component_get_card(component);

        state->mBase = card_get_register_address(card, DAG_REG_XFP, state->mPortNumber);
        return kDagErrNone;
    }
    return kDagErrInvalidParameter;
}

static int
xfp_post_initialize(ComponentPtr component)
{
	DagCardPtr card = NULL;
	AttributePtr attribute = NULL;
	GenericReadWritePtr grw = NULL;  
    uint8_t *module_detect = NULL;
    uint8_t *diagnostic_monitor_present = NULL;
	
	xfp_state_t* xfp_state = (xfp_state_t*)component_get_private_state(component);

	/* Get card reference */
	card = component_get_card(component);
	
	// this is redundant with the new method 
	xfp_state->mBase = card_get_register_address(card, DAG_REG_XFP, xfp_state->mPortNumber);
	
	/* Add attribute of counter */ 
	read_attr_array(component, xfp_sfp_attr, NB_ELEM_XFP_SFP, xfp_state->mPortNumber);
	
	/* XFP module presented if version 0 if version 1 SFP module presented*/
	if( xfp_state->mVersion == 0 ) {
		
		/* Add attribute of counter */ 
		read_attr_array(component, xfp_attr, NB_ELEM_XFP, xfp_state->mPortNumber);
        /* set attribute with inverted boolean
	   * Usually set a bit means "write 1", but in these cases it means "write 0" */
	   attribute = component_get_attribute(component, kBooleanAttributeSfpPwr);
	   grw = attribute_get_generic_read_write_object(attribute);
	   grw_set_on_operation(grw, kGRWClearBit);
    
	}
	
	/* set attribute with inverted boolean
	   * Usually set a bit means "write 1", but in these cases it means "write 0" */
	attribute = component_get_attribute(component, kBooleanAttributeLaser);
	grw = attribute_get_generic_read_write_object(attribute);
	grw_set_on_operation(grw, kGRWClearBit);

    attribute = component_get_attribute(component, kBooleanAttributeDetect);
	grw = attribute_get_generic_read_write_object(attribute);
	grw_set_on_operation(grw, kGRWClearBit);

    /* use above attribute (kBooleanAttributeDetect) to check the presense of module and add the module attributes to the components */

    module_detect = (uint8_t*) attribute_boolean_get_value(attribute);
    if ( (NULL != module_detect) && (1 == *module_detect ) )
    {
        /* read the module attributes and fill in the component */
        read_attr_array( component, module_attr, NB_ELEM_MODULE_ATTR, xfp_state->mPortNumber);
    }

    /* check whether the digital diagnostic implementations are present, and 
     if so, add the temperature, voltage , power attributes */
    attribute = component_get_attribute(component, kBooleanAttributeTransceiverMonitoring);
    diagnostic_monitor_present = (uint8_t*) diagnostic_monitoring_get_value(attribute);
#if 0
    if ( (NULL != diagnostic_monitor_present) && (1 == *diagnostic_monitor_present ) )
    {
        read_attr_array( component, module_diagnostic_monitor_attributes, NB_ELEM_MODULE_MONITOR_ATTR, xfp_state->mPortNumber);
    }
#endif 
	/* Return 1 if standard post_initialize() should continue, 0 if not.
	* "Standard" post_initialize() calls post_initialize() recursively on subcomponents.
	*/
    return 1;
}

static void
xfp_reset(ComponentPtr component)
{
}


static void
xfp_default(ComponentPtr component)
{
}

void* module_identifier_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        module_identifier_t  module_type = kInvalidModuleType;
        xfp_state_t* xfp_state =   NULL;
        
        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        address = 0xa0 << 8;
        address |= (xfp_state->mPortNumber << 16);

	    sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);

        assert(sfp_grw);
        module_type = (module_identifier_t) grw_read(sfp_grw);
        attribute_set_value_array(attribute, &module_type, sizeof(module_identifier_t));
        grw_dispose(sfp_grw);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

void module_identifier_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        const char *string = NULL;
        module_identifier_t *ptr_type = NULL;

        ptr_type = (module_identifier_t*) module_identifier_get_value(attribute);
        if ( NULL != ptr_type )
        {
            string = module_type_to_string(*ptr_type);
        }
       // printf("Converted sttring %s\n",string);
        if( NULL != string )
        {
            attribute_set_to_string(attribute, string);
        }
        return;
     }
}

const char*
module_type_to_string(module_identifier_t type)
{
    int i;
    int count = sizeof(table)/sizeof(module_type_mapping_t);
    for (i = 0; i < count; i++)
    {
        if (table[i].module_type == type)
        {
            return table[i].string;
        }
    }
    return table[0].string;/* invalid module */
}


void* extended_module_identifier_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint32_t extended_type = 0;
        xfp_state_t* xfp_state =   NULL;
        
        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        address = 0xa0 << 8;
        address |= (xfp_state->mPortNumber << 16);
        address |= 0x01;

	    sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);

        assert(sfp_grw);
        extended_type =  (uint32_t) grw_read(sfp_grw);
        attribute_set_value_array(attribute, &extended_type, sizeof(extended_type));
        grw_dispose(sfp_grw);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

void* vendor_name_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        xfp_state_t* xfp_state =   NULL;
        char vendor_name[32] = {0};
        int i = 0;

        AttributePtr attr_mod_type; 
        module_identifier_t module_type = kInvalidModuleType;
        module_identifier_t *ptr_type = NULL;

        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        attr_mod_type = component_get_attribute(component, kUint32AttributeTransceiverIdentifier);
        ptr_type = (module_identifier_t *) module_identifier_get_value(attr_mod_type);
        if( NULL != ptr_type  )
        {
           module_type = *ptr_type;
        }
        if(kUnknown == module_type)
        {
            strcpy(vendor_name,"Unknown Module");
        }
        else if( kXFP == module_type )
        {
            /* for XFP modules as defined by INF-8077i, it is a 2 level memory map (upper and lower). 
             To acces the upper memory map, the page should be selected in the byte 127 of lower map*/ 
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 127;
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
            grw_write(sfp_grw, 0x01);
            address = (address & 0xffffff00) | 148; /* vendor name starts at the byte 148 of page 1*/
            grw_set_address(sfp_grw, address);
        }
        else if (kSFP == module_type)
        {
            
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 20;/* vendor name starts at byte 20*/
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        }
        else
        {
            strcpy(vendor_name,"Not Implemented");
        }

        if(sfp_grw)
        { 
           for( i = 0 ; i < 16; i++)
           {
                grw_set_address(sfp_grw, address);
                vendor_name[i] = (char) grw_read(sfp_grw);
                address++;
            }
            vendor_name[i] = '\0';
			grw_dispose(sfp_grw); /* memory leak fix */
        }
        
        attribute_set_value_array(attribute, vendor_name, strlen(vendor_name) + 1);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
void* vendor_pn_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        xfp_state_t* xfp_state =   NULL;
        char vendor_pn[32] = {0};
        int i = 0;
        AttributePtr attr_mod_type; 
        module_identifier_t module_type = kInvalidModuleType;
        module_identifier_t *ptr_type = NULL;

        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        attr_mod_type = component_get_attribute(component, kUint32AttributeTransceiverIdentifier);
        ptr_type = (module_identifier_t *) module_identifier_get_value(attr_mod_type);
        if( NULL != ptr_type  )
        {
           module_type = *ptr_type;
        }
        if( kUnknown == module_type)
        {
            strcpy(vendor_pn,"Unknown Module");
        }
        else if( kXFP == module_type )
        {
            /* for XFP modules as defined by INF-8077i, it is a 2 level memory map (upper and lower). 
             To acces the upper memory map, the page should be selected in the byte 127 of lower map*/ 
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 127;
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
            grw_write(sfp_grw, 0x01);
            address = (address & 0xffffff00) | 168; /* vendor PN starts at the byte 168 of page 1*/
            grw_set_address(sfp_grw, address);
        }
        else if (kSFP == module_type)
        {
            
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 40;/*vendor pn starts at byte 40*/
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        }
        else
        {
            strcpy(vendor_pn,"Not Implemented");
        }
        
        if(sfp_grw)
        { 
           for( i = 0 ; i < 16; i++)
           {
                grw_set_address(sfp_grw, address);
                vendor_pn[i] = (char) grw_read(sfp_grw);
                address++;
            }
            vendor_pn[i] = '\0';
			grw_dispose(sfp_grw); 
        }
       attribute_set_value_array(attribute, vendor_pn, strlen(vendor_pn) + 1);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

void common_to_string_function(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        char *string = NULL;
        string = (char*) attribute_get_value(attribute);
        if ( NULL != string)
        {
            attribute_set_to_string(attribute, string);
        }
        return;
    }   
}

void* transceiver_codes_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint32_t tr_code = 0;
        uint8_t read_byte  = 0;
        xfp_state_t* xfp_state =   NULL;
        char trans_compli_string[512]={'\0'};
        int i = 0, j = 0;
        int string_array_size = 0;
        AttributePtr attr_mod_type; 
        module_identifier_t module_type = kInvalidModuleType;
        module_identifier_t *ptr_type = NULL;

        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        attr_mod_type = component_get_attribute(component, kUint32AttributeTransceiverIdentifier);
        ptr_type = (module_identifier_t *) module_identifier_get_value(attr_mod_type);
        if( NULL != ptr_type  )
        {
           module_type = *ptr_type;
        }
        if( kUnknown == module_type)
        {
            strcpy(trans_compli_string,"Unknown Module");
        }
        else if( kSFP == module_type)
        {
            component = attribute_get_component(attribute);
            xfp_state = (xfp_state_t*)component_get_private_state(component);
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 0x06;

	        sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
            assert(sfp_grw);
            for( i = 0 ; i < 4; i++)
            {
                grw_set_address(sfp_grw, address);
                tr_code |= grw_read(sfp_grw) << (i *8 );
                //    printf(" 0x%02x at %d\n", grw_read(sfp_grw),6-i);            
                address--;

            }
            string_array_size = sizeof(transceiver_compliance_strings) / sizeof( char*);
            for ( i = 0; i < string_array_size; i++)
            {
                if ( (tr_code) & (0x01 << i ) )
                {
                    strcat(trans_compli_string, transceiver_compliance_strings[string_array_size-1-i]);
                }
            }
			grw_dispose(sfp_grw);
        }
        else if (kXFP == module_type)
        {
            component = attribute_get_component(attribute);
            xfp_state = (xfp_state_t*)component_get_private_state(component);
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 127; /* to select the page address for upper memory */
            sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
            grw_write(sfp_grw, 0x01);
            address = (address & 0xffffff00) | 131; /* transceiver codes starts at bye 131 of page 1*/
            /* total 64 strings */
            string_array_size = sizeof(transceiver_compliance_strings) / sizeof( char*);
            for ( i = 0; i < 8; i++)
            {
                grw_set_address(sfp_grw, address);
                read_byte = grw_read(sfp_grw);
                /* for each bit in the byte */
                for( j = 0; j < 8; j++)
                {
                    if ( read_byte & ( 0x1 << (7 - j) ) )
                        strcat(trans_compli_string, transceiver_compliance_strings_xfp[(i * 8 ) + j]);
                }
                address++;
            }
			grw_dispose(sfp_grw);
        }
        else
        {
            strcpy(trans_compli_string,"Not Implemented");
        }
        attribute_set_value_array(attribute, trans_compli_string, strlen(trans_compli_string) + 1);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
/*
void transceiver_code_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        uint32_t *trans_code = NULL;
        int i = 0;
        trans_code = (uint32_t*) attribute_get_value(attribute);
        if ( NULL == trans_code)
        {
           return; 
        }
               attribute_set_to_string(attribute, trans_compli_string);
        return;
    }   
}

*/


void* link_length_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint32_t length_code = 0;
        xfp_state_t* xfp_state =   NULL;
        char string[512]={0} ;
        int i = 0;
        int string_array_size = 0;
        AttributePtr attr_mod_type; 
        module_identifier_t module_type = kInvalidModuleType;
        module_identifier_t *ptr_type = NULL;

        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        attr_mod_type = component_get_attribute(component, kUint32AttributeTransceiverIdentifier);
        ptr_type = (module_identifier_t *) module_identifier_get_value(attr_mod_type);
        if( NULL != ptr_type  )
        {
           module_type = *ptr_type;
        }

        if( kUnknown == module_type)
        {
            strcpy(string,"Unknown Module");
        }
        else if( kSFP == module_type)
        {
            component = attribute_get_component(attribute);
            xfp_state = (xfp_state_t*)component_get_private_state(component);
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 0x07;

	       sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
            assert(sfp_grw);
            length_code = grw_read(sfp_grw);
            length_code &= 0x1f;
            //printf("LengthCode 0x%x\n",length_code);
            string_array_size = (sizeof(fibre_link_length_strings) / sizeof (char*));
            for(i =0; i < string_array_size; i++)
            {
                if ( length_code & (0x01 << i ) )
                {
                    strcat(string, fibre_link_length_strings[string_array_size-1-i]);
                }
            }
            if(0 == strlen(string))
                strcpy(string, "Not specified");
			grw_dispose(sfp_grw);
        }
        else
        {
             strcpy(string, "Not Implemented");
        }
        attribute_set_value_array(attribute, string, strlen(string) + 1);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
void* transmission_media_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint32_t media_code = 0;
        xfp_state_t* xfp_state =   NULL;
        char string[512]={0} ;
        int i = 0;
        int string_array_size = 0;
        AttributePtr attr_mod_type; 
        module_identifier_t module_type = kInvalidModuleType;
        module_identifier_t *ptr_type = NULL;

        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        attr_mod_type = component_get_attribute(component, kUint32AttributeTransceiverIdentifier);
        ptr_type = (module_identifier_t *) module_identifier_get_value(attr_mod_type);
        if( NULL != ptr_type  )
        {
           module_type = *ptr_type;
        }
        if (kUnknown == module_type )
        {
            strcpy(string,"Unknown Module");
        }
        else if( kSFP == module_type)
        {
            component = attribute_get_component(attribute);
            xfp_state = (xfp_state_t*)component_get_private_state(component);
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 0x09;

	       sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
            assert(sfp_grw);
            media_code = grw_read(sfp_grw);
            string_array_size = (sizeof(fibre_tr_media_strings) / sizeof (char*));
            for(i =0; i < string_array_size; i++)
            {
                if ( (media_code) & (0x01 << i ) )
                {
                     strcat(string, fibre_tr_media_strings[string_array_size-1-i]);
                }
            }
            if(0 == strlen(string))
                strcpy(string, "Not specified");
			grw_dispose(sfp_grw);
        }
        else
        {
                strcpy(string, "Not Implemented");
        }
        attribute_set_value_array(attribute, string, strlen(string) + 1);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

void* diagnostic_monitoring_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint8_t read_byte  = 0;
        uint8_t value = 0;
        xfp_state_t* xfp_state =   NULL;
        AttributePtr attr_mod_type; 
        module_identifier_t module_type = kInvalidModuleType;
        module_identifier_t *ptr_type = NULL;

        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        attr_mod_type = component_get_attribute(component, kUint32AttributeTransceiverIdentifier);
        ptr_type = (module_identifier_t *) module_identifier_get_value(attr_mod_type);
        if( NULL != ptr_type  )
        {
           module_type = *ptr_type;
        }
        if( kSFP == module_type)
        {
            component = attribute_get_component(attribute);
            xfp_state = (xfp_state_t*)component_get_private_state(component);
            address = 0xa0 << 8;
            address |= (xfp_state->mPortNumber << 16);
            address |= 92;

	       sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
            assert(sfp_grw);
            read_byte = grw_read(sfp_grw);
        //printf("MediaCode 0x%x\n",media_code);
            if ( read_byte & BIT6 )
            {
                value = 1;
            }
			grw_dispose(sfp_grw);
        }
        attribute_set_value_array(attribute, &value, sizeof(value));
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

void *transceiver_temperature_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        float temp = 0;
        
        xfp_state_t* xfp_state =   NULL;
        
        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        address = 0xa2 << 8;
        address |= (xfp_state->mPortNumber << 16);
        address |= 96;
        sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        assert(sfp_grw);
        if( is_ext_calibarated(card, xfp_state) )
        {
            int32_t  t_slope = 0;
            int32_t  t_offset = 0;
            int32_t tmp_temp = (int8_t) grw_read(sfp_grw);
            address++;
            grw_set_address(sfp_grw, address);
            tmp_temp = (tmp_temp << 8) | grw_read(sfp_grw);
            address = (address & 0xffffff00) | 84;
            grw_set_address(sfp_grw, address);
            t_slope =  grw_read(sfp_grw);
            address++;
            grw_set_address(sfp_grw, address);
            t_slope = (t_slope << 8) | grw_read(sfp_grw);
            address++;
            grw_set_address(sfp_grw, address);
            t_offset = (int8_t) grw_read(sfp_grw);
            address++;
            grw_set_address(sfp_grw, address);
            t_offset = (t_offset<< 8) | grw_read(sfp_grw);
            temp = (float)(t_slope * tmp_temp) + t_offset;
            
        }
        else
        {
	        temp = (int8_t)grw_read(sfp_grw);
            address++;
            grw_set_address(sfp_grw, address);
            temp += (1 / grw_read(sfp_grw));
        }
        attribute_set_value_array(attribute, &temp, sizeof(float) );
        grw_dispose(sfp_grw);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
void *transceiver_voltage_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint32_t volt= 0;
        xfp_state_t* xfp_state =   NULL;
        
        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        address = 0xa2 << 8;
        address |= (xfp_state->mPortNumber << 16);
        address |= 98;
        /* Byte 98 is MSB Byte 99 is LSB */
	    sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        assert(sfp_grw);
        volt = grw_read(sfp_grw) << 8;

        address++;
        grw_set_address(sfp_grw, address);
        volt |= grw_read(sfp_grw);
        attribute_set_value_array(attribute, &volt, sizeof(volt) );
        grw_dispose(sfp_grw);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
void *transceiver_tx_power_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint32_t txpower= 0;
        xfp_state_t* xfp_state =   NULL;
        
        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        address = 0xa2 << 8;
        address |= (xfp_state->mPortNumber << 16);
        address |= 102;
        /* Byte 102 is MSB and 103 is LSB */
	    sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        assert(sfp_grw);
        txpower = grw_read(sfp_grw) << 8;

        address++;
        grw_set_address(sfp_grw, address);
        txpower |= grw_read(sfp_grw);
        attribute_set_value_array(attribute, &txpower, sizeof(txpower) );
        grw_dispose(sfp_grw);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}
void *transceiver_rx_power_get_value(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        GenericReadWritePtr sfp_grw = NULL;
        uint32_t address = 0;
        DagCardPtr card = attribute_get_card(attribute);
        ComponentPtr component = NULL;
        uint32_t rxpower= 0;
        xfp_state_t* xfp_state =   NULL;
        
        component = attribute_get_component(attribute);
        xfp_state = (xfp_state_t*)component_get_private_state(component);
        address = 0xa2 << 8;
        address |= (xfp_state->mPortNumber << 16);
        address |= 104;
        /* Byte 104 is MSB and 105 is LSB */
	    sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
        assert(sfp_grw);
        rxpower = grw_read(sfp_grw) << 8;

        address++;
        grw_set_address(sfp_grw, address);
        rxpower |= grw_read(sfp_grw);
        attribute_set_value_array(attribute, &rxpower, sizeof(rxpower) );
        grw_dispose(sfp_grw);
        return attribute_get_value_array(attribute);
    }
    return NULL;
}

/* // detect 
static void*
xfp_detect_get_value(AttributePtr attribute)
{
    ComponentPtr component;
    DagCardPtr card;
    static uint8_t return_value = 0;
    uint32_t register_value = 0;
    uint32_t mask = 0;
    if (1 == valid_attribute(attribute))
    {
        xfp_state_t* state = NULL;
        component = attribute_get_component(attribute);
        state = (xfp_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
		register_value = card_read_iom(card, state->mBase);
        mask = BIT0 << state->mPortNumber*4;
        return_value = (register_value & mask) == mask;
        return (void*)&return_value;
    }
    return NULL;
}


// buffer size 
static void*
xfp_los_get_value(AttributePtr attribute)
{
    static uint8_t return_value = 0;
    uint32_t register_value = 0;
    uint32_t mask = 0;
    xfp_state_t* state = NULL;
    ComponentPtr component = NULL;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (xfp_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
        mask = (BIT1 << (state->mPortNumber*4));
        register_value = card_read_iom(card, state->mBase);
        return_value = (register_value & mask) == mask;
        return (void*)&return_value;
    }
    return NULL;
}


// sfppwr count
static void*
xfp_pwr_get_value(AttributePtr attribute)
{
    static uint8_t return_value = 0;
    uint32_t register_value = 0;
    uint32_t mask = 0;
    xfp_state_t* state = NULL;
    ComponentPtr component = NULL;

    if (1 == valid_attribute(attribute))
    {
        DagCardPtr card = attribute_get_card(attribute);
        component = attribute_get_component(attribute);
        state = (xfp_state_t*)component_get_private_state(component);
        card = attribute_get_card(attribute);
        mask = BIT2 << (state->mPortNumber*4);
		register_value = card_read_iom(card, state->mBase);
        return_value = (register_value & mask) == mask;
        return (void*)&return_value;
    }
    return NULL;
}

static void
xfp_pwr_set_value(AttributePtr attribute, void* value, int length)
{
	ComponentPtr component = NULL;
    xfp_state_t* state = NULL;
    uint32_t register_value = 0;
    DagCardPtr card = NULL;
  
    if (1 == valid_attribute(attribute))
    {
        component = attribute_get_component(attribute);
        state = component_get_private_state(component);
        card = component_get_card(component);
        if (*(uint8_t*)value == 0)
        {
            register_value = card_read_iom(card, state->mBase);
            switch (state->mPortNumber)
            {
                case 0:
                {
                    register_value &= ~BIT2;
                    break;
                }
                case 1:
                {
                    register_value &= ~BIT6;
                    break;
                }
                case 2:
                {
                    register_value &= ~BIT10;
                    break;
                }
                case 3:
                {
                    register_value &= ~BIT14;
                    break;
                }
            }
            card_write_iom(card, state->mBase, register_value);
        }
        else
        {
            register_value= card_read_iom(card, state->mBase);
            switch (state->mPortNumber)
            {
                case 0:
                {
                    register_value |= BIT2;
                    break;
                }
                case 1:
                {
                    register_value |= BIT6;
                    break;
                }
                case 2:
                {
                    register_value |= BIT10;
                    break;
                }
                case 3:
                {
                    register_value |= BIT14;
                    break;
                }
            }
            card_write_iom(card, state->mBase, register_value);
        }
    }
}

*/
/*
uint32_t
xfp_get_port_number(ComponentPtr component)
{
    xfp_state_t* state = NULL;
    state = (xfp_state_t*)component_get_private_state(component);
    return state->mPortNumber;
}

void
xfp_set_port_number(ComponentPtr component, uint32_t val)
{
    xfp_state_t* state = NULL;
    state = (xfp_state_t*)component_get_private_state(component);
    state->mPortNumber = val;
}
*/

uint8_t is_ext_calibarated(DagCardPtr card, xfp_state_t* state)
{
    GenericReadWritePtr sfp_grw = NULL;
    uint32_t address = 0;
    uint8_t value = 0;
    uint8_t read_byte = 0;
    address = 0xa0 << 8;
    address |= (state->mPortNumber << 16);
    address |= 92;

	sfp_grw = grw_init(card, address, grw_xfp_sfp_raw_smbus_read, grw_xfp_sfp_raw_smbus_write);
    assert(sfp_grw);
    read_byte = grw_read(sfp_grw);
    if ( read_byte & BIT4 )
    {
        value = 1;
    }
    grw_dispose(sfp_grw);
    return value;
    
}

