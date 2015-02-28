
#include <string.h>
#include <stdio.h>
#include "../include/util/enum_string_table.h"
#include "dag_config.h"

typedef struct
{
    termination_t termination;
    const char* string;
} termination_string_map_t;

typedef struct
{
    line_type_t line_type;
    const char* string;
} line_type_string_map_t;

typedef struct
{
    line_rate_t line_rate;
    const char* string;
} line_rate_string_map_t;

typedef struct
{
    counter_select_t counter_select;
    const char* string;
} counter_select_string_map_t;

typedef struct
{
    vc_size_t vc_size;
    const char* string;
} vc_size_string_map_t;

typedef struct
{
    pci_bus_speed_t pci_bus_speed;
    const char* string;
} pci_bus_speed_string_map_t;

typedef struct
{
    pci_bus_type_t pci_bus_type;
    const char* string;
} pci_bus_type_string_map_t;


typedef struct
{
    payload_mapping_t payload_mapping;
    const char* string;
} payload_mapping_string_map_t;

typedef struct
{
    crc_t crc;
    const char* string;
} crc_string_map_t;

typedef struct
{
    tx_command_t tx_command;
    const char* string;
} tx_command_string_map_t;

typedef struct
{
    network_mode_t network_mode;
    const char* string;

} network_mode_string_map_t;

typedef struct
{
    sonet_type_t st;
    const char* string;
} sonet_type_string_map_t;

typedef struct
{
    master_slave_t ms;
    const char* string;
} master_slave_string_map_t;

typedef struct
{
    dag_card_t card_type;
    const char* string;
} card_string_map_t;

typedef struct
{
    dag71s_channelized_rev_id_t id;
    const char* string;
} dag71s_channelized_rev_id_string_map_t;

typedef struct
{
    ethernet_mode_t val;
    const char* string;
} ethernet_mode_string_map_t;

typedef struct
{
    terf_strip_t val;
    const char* string;
} terf_strip_string_map_t;

typedef struct
{
    terf_time_mode_t val;
    const char* string;
} terf_time_mode_string_map_t;

typedef struct
{
    steer_t val;
    const char* string;
} steer_string_map_t;

typedef struct
{
    erfmux_steer_t val;
    const char* string;
} erfmux_steer_string_map_t;

typedef struct
{
	zero_code_suppress_t val;
	const char* string;
} zero_code_string_map_t;

typedef struct
{
    framing_mode_t val;
    const char * string;
}framing_mode_string_map_t;

typedef struct
{
    dag_firmware_t firmware;
    const char * string;
}active_firmware_string_map_t;

typedef struct
{
    coprocessor_t coprocessor;
    const char * string;
}coprocessor_string_map_t;
/* structure used to convert component codes to strings*/
typedef struct
{
	dag_component_code_t comp_code;
	const char *string;
} dag_component_code_string_map_t;

typedef struct
{
	dag_attribute_code_t comp_code;
	const char *string;
} dag_attribute_code_string_map_t;


static active_firmware_string_map_t active_firmware_string[] =
{
    {kFirmwareFactory, "factory"},
    {kFirmwareUser, "user"},
    {kFirmwareUser1,"user"},
    {kFirmwareUser2,"user"},
    {kFirmwareUser3,"user"},	 	

};
static coprocessor_string_map_t coprocessor_string[] =
{
    {kCoprocessorNotSupported, "Not Supported"},
    {kCoprocessorNotFitted, "Not Fitted"},
    {kCoprocessorPrototype, "Prototype"},
    {kCoprocessorSC128, "SC128"},
    {kCoprocessorSC256, "SC256"},
    {kCoprocessorSC256C, "SC256C"},
    {kCoprocessorUnknown, "Unknown"},
    {kCoprocessorBuiltin, "Built-in"}

};


static framing_mode_string_map_t framing_mode_string[] =
{
    {kFramingModeDs3m23, "ds3_m23"},
    {kFramingModeDS3M23PLCP, "ds3_m23_plcp"},
    {kFramingModeDs3m23FF, "ds3_m23_ff"}, 
    {kFramingModeDs3m23EF, "ds3_m23_ef"},
    {kFramingModeDs3m23IF, "ds3_m23_if"},
    {kFramingModeDs3Cbit, "ds3_cbit"},
    {kFramingModeDs3CbitFF, "ds3_cbit_ff"},
    {kFramingModeDs3CbitEF, "ds3_cbit_ef"},
    {kFramingModeDs3CbitIF, "ds3_cbit_if"},
    {kFramingModeDS3CbitPLCP, "ds3_cbit_plcp"},
    {kFramingModeE3, "e3"},
    {kFramingModeE3G751IF, "e3_g751_if"},
    {kFramingModeE3G751PLCP, "e3_g751_plcp"},
    {kFramingModeE3CC, "e3_cc"}

};
static zero_code_string_map_t zero_code_string[] =
{
	{kZeroCodeInvalid, "invalid"},
	{kZeroCodeSuppressAMI, "ami"},
	{kZeroCodeSuppressB8ZS, "b8zs/hdb3"}
};

static ethernet_mode_string_map_t ethernet_mode_string[] =
{
    {kEthernetModeInvalid, "invalid"},
    {kEthernetMode10GBase_LR, "10gbase_lr"},
    {kEthernetMode10GBase_LW, "10gbase_lw"},
    {kEthernetMode10GBase_SR, "10gbase_sr"},
    {kEthernetMode10GBase_ER, "10gbase_er"},
};

static card_string_map_t card_string[] =
{
    {kDagUnknown, "unknown"},
    {kDag35e, "dag35e"},
    {kDag35, "dag35"},
    {kDag36d, "dag36d"},
    {kDag36e, "dag36e"},
    {kDag36ge, "dag36ge"},
    {kDag37d, "dag37d"},
    {kDag37ge, "dag37ge"},
    {kDag37t, "dag37t"},
    {kDag37t4, "dag37t4"},
    {kDag38, "dag38s"},
    {kDag42ge, "dag42ge"},
    {kDag423ge, "dag423ge"},
    {kDag42, "dag42"},
    {kDag423, "dag423"},
    {kDag43ge, "dag43ge"},
    {kDag43s, "dag43s"},
    {kDag452e, "dag452e"},
    {kDag452gf, "dag452gf"},
    {kDag452cf, "dag452cf"},
    {kDag454e, "dag454e"},
    {kDag452z, "dag452z"},
    {kDag452z8, "dag452z8"},
    {kDag50s, "dag50sg2"},
    {kDag52x, "dag52x"},
    {kDag52sxa, "dag52sxa"},
    {kDag60, "dag60"},
    {kDag61, "dag61"},
    {kDag62, "dag62"},
    {kDag70s, "dag70s"},
    {kDag70ge, "dag70ge"},
    {kDag71s, "dag71s"},
    {kDag82x, "dag82x"},
    {kDag82x2, "dag82x2"},
    {kDag82z, "dag82z"},
    {kDag800, "dag800"},
    {kDag810, "dag810"},
//TODO: Due a changes in the tools for the time being the tools 
//    and config and status api use the same constatants and assumptions for the 8.101 as 8.10
//    {kDag811, "dag81sx"},
    {kDag50z, "dag50z"},
    {kDag50dup, "dag50dup"},
    {kDag50sg2a, "dag50sg2aapp"},
    {kDag50sg2adup, "dag50sg2adup"},
    {kDag840, "dag84i"},
    {kDag54s12,"dag54s12"},
    {kDag54sg48,"dag54sg48"},
    {kDag50sg3a, "dag50sg2a"},
    {kDag54ga, "dag54ga"},
    {kDag54sa12, "dag54sa12"},
    {kDag54sga48, "dag54sga48"},
    {kDag74s, "dag74s"},
    {kDag74s48, "dag74s48"},
    {kDag75g2, "dag75g2"},
    {kDag75g4, "dag75g4"},
    {kDag75be, "dag75be"},
    {kDag75ce, "dag75ce"},
    {kDag91x2Rx, "dag91x Rx"},
    {kDag91x2Tx, "dag91x Tx"},
    {kDag850, "dag85i"},
    {kDag92x, "dag92x"},
};

static steer_string_map_t steer_string[] =
{
    {kSteerStream0, "stream0"}, 
    {kSteerCrc, "crc"}, 
    {kSteerParity, "parity"},
    {kSteerIface, "iface"},
    {kSteerInvalid, "invalid"},
    {kSteerColour, "colour/dsm"},
};

static erfmux_steer_string_map_t erfmux_steer_string[] =
{
	{kSteerInvalid, "invalid"},
	{kSteerLine, "line"},
	{kSteerHost, "host"},
	{kSteerIXP, "ixp/xscale"},
	{kSteerDirectionBit, "direction"},
};

static termination_string_map_t termination_string[] =
{
    {kTerminationExternal, "termination_external"}, 
    {kTerminationRxExternalTx75ohm, "termination_tx_75"}, 
    {kTerminationRxExternalTx100ohm, "termination_tx_100"},
    {kTerminationRxExternalTx120ohm, "termination_tx_120"},
    {kTerminationRx75ohmTxExternal, "termination_rx_75"},
    {kTerminationRx100ohmTxExternal, "termination_rx_100"},
    {kTerminationRx120ohmTxExternal, "termination_rx_120"},
    {kTermination75ohm, "termination_75"},
    {kTermination100ohm, "termination_100"},
    {kTermination120ohm, "termination_120"}
};

static master_slave_string_map_t master_slave_string[] =
{
    {kMaster, "master"},
    {kSlave, "slave"}
};

static line_type_string_map_t line_type_string[] =
{
    {kLineTypeNoPayload, "no_payload"},
    {kLineTypeE1unframed, "e1_unframed"},
    {kLineTypeE1, "e1"},
    {kLineTypeE1crc, "e1_crc"},
    {kLineTypeT1, "t1"},
    {kLineTypeT1esf, "t1esf"},
    {kLineTypeT1sf, "t1sf"},
    {kLineTypeT1unframed, "t1_unframed"},
    {kLineTypeInvalid, "invalid"}
};

static line_rate_string_map_t line_rate_string[] =
{
//check me wher do we use oc1 maybe e1/t1 ??
    {kLineRateInvalid, "invalid"},
    {kLineRateOC1c, "oc1"},
    {kLineRateOC12c, "oc12"},
    {kLineRateOC3c, "oc3"},
    {kLineRateEthernet1000, "1000"},
    {kLineRateEthernet100, "100"},
    {kLineRateEthernet10, "10"},
    {kLineRateOC192c, "oc192"},
    {kLineRateOC48c, "oc48"},
    {kLineRateAuto, "auto"},
    {kLineRateSTM0, "stm0"},
    {kLineRateSTM1, "stm1"},
    {kLineRateSTM4, "stm4"},
    {kLineRateSTM16, "stm16"},
    {kLineRateSTM64, "stm64"}
};

static counter_select_string_map_t counter_select_string[] =
{
    {kCounterSelectB1Error, "b1_error"},
    {kCounterSelectB2Error, "b2_error"},
    {kCounterSelectB3Error, "b3_error"},
    {kCounterSelectATMBadHEC, "atm_bad_hec"},
    {kCounterSelectATMCorrectableHEC,"atm_cor_hec"},
    {kCounterSelectATMRxIdle, "atm_rx_idle"},
    {kCounterSelectATMRxCell, "atm_rx_cell"},
    {kCounterSelectPoSBadCRC, "pos_bad_crc"},
    {kCounterSelectPoSMinPktLenError, "pos_min_error"},
    {kCounterSelectPoSMaxPktLenError, "pos_max_error"},
    {kCounterSelectPoSAbort, "pos_abort"},
    {kCounterSelectPoSGoodFrames, "pos_good_frames"},
    {kCounterSelectPoSRxBytes, "pos_rx_bytes"}
};


static payload_mapping_string_map_t payload_mapping_string[] =
{
    {kPayloadMappingAsync, "async"},
    {kPayloadMappingBitSync, "bit_sync"},
    {kPayloadMappingByteSync1, "byte_sync_1"},
    {kPayloadMappingByteSync2, "byte_sync_2"},
    {kPayloadMappingDisabled, "disabled"}
};

static vc_size_string_map_t vc_size_string[] =
{
    {kVC3, "vc3"},
    {kVC4, "vc4"},
    {kVC4C, "vc4c"}
};

static pci_bus_speed_string_map_t pci_bus_speed_string[] =
{
    {kPCIBusSpeedUnstable, "unstable"},
    {kPCIBusSpeedUnknown, "unknown"},
    {kPCIBusSpeed133Mhz, "133MHz"},
    {kPCIBusSpeed100Mhz, "100MHz"},
    {kPCIBusSpeed66Mhz, "66MHz"},
    {kPCIBusSpeed33Mhz, "33MHz"},
    {kPCIEBusSpeed2Gbs, "2Gbs, 1lane"},
    {kPCIEBusSpeed4Gbs, "4Gbs, 2lane"},
    {kPCIEBusSpeed8Gbs, "8Gbs, 4lane"},
    {kPCIEBusSpeed16Gbs, "16Gbs, 8lane"},
};

static pci_bus_type_string_map_t pci_bus_type_string[] =
{
    { kBusTypeUnknown, "Unknown"},
    { kBusTypePCI, "PCI"},
    { kBusTypePCIX, "PCI-X"},
    { kBusTypePCIE, "PCI-e"},
};

static crc_string_map_t crc_string[] =
{
    {kCrc32, "crc32"},
    {kCrc16, "crc16"},
    {kCrcOff, "nocrc"}
};

static network_mode_string_map_t network_mode_string[] =
{
    {kNetworkModeATM, "atm"},
    {kNetworkModePoS, "pos"},
    {kNetworkModeRAW, "raw"},
    {kNetworkModeEth, "eth"},
    {kNetworkModeHDLC, "hdlc"},
    {kNetworkModeWAN, "wan"},
    {kNetworkModeInvalid ,"unknown_mode"},
};

static sonet_type_string_map_t sonet_type_string[] =
{
    {kSonetTypeConcatenated, "concatenated"},
    {kSonetTypeChannelized, "channelized"}
};

static tx_command_string_map_t tx_command_string[] =
{
	{kTxCommandStop, "stop"},
	{kTxCommandBurst, "burst"},
	{kTxCommandStart, "start"}
};

static dag71s_channelized_rev_id_string_map_t dag71s_channelized_rev_id_string[] =
{
    {kDag71sRevIdHDLC, "HDLC"},
    {kDag71sRevIdHDLCRAW, "HDLC and RAW"},
    {kDag71sRevIdATMHDLCRAW, "ATM and HDLC and RAW"},
    {kDag71sRevIdATMHDLC, "ATM and HDLC"},
    {kDag71sRevIdATM, "ATM and HDLC"},
    {kDag71sRevIdInvalid, "Unknown Revision Id"}
};

static terf_strip_string_map_t terf_strip_string[] =
{
    {kTerfNoStrip, "noterf_strip"},
    {kTerfStrip16, "terf_strip16"},
    {kTerfStrip32, "terf_strip32"}
};

static terf_time_mode_string_map_t terf_time_mode_string[] =
{
    {kTrTerfNoDelay, "no_time_mode"},
    {kTrTerfRelative, "relative"},
};

static dag_component_code_string_map_t dag_component_code_string[] =
{
    {kComponentInvalid,               "kComponentInvalid"},
    {kComponentRoot,                   "kComponentRoot"},
    {kComponentConnectionSetup,       "kComponentConnectionSetup"},
    {kComponentConnection,            "kComponentConnection"},
    {kComponentDeframer,              "kComponentDeframer"},
    {kComponentDrop,                  "kComponentDrop"},
    {kComponentDUCK,                  "kComponentDUCK"},
    {kComponentIRIGB,                 "kComponentIRIGB"},
    {kComponentE1T1,                  "kComponentE1T1"},
    {kComponentFramer,                "kComponentFramer"},
    {kComponentDemapper,              "kComponentDemapper"},
    {kComponentMapper,                "kComponentMapper"},
    {kComponentGpp,                   "kComponentGpp"},
    {kComponentSRGPP,                 "kComponentSRGPP"},
    {kComponentLEDController,         "kComponentLEDController"},
    {kComponentMiniMacStatistics,     "kComponentMiniMacStatistics"},
    {kComponentMux,                   "kComponentMux"},
    {kComponentPhy,                   "kComponentPhy"},
    {kComponentPacketCapture,         "kComponentPacketCapture"},
    {kComponentPort,                  "kComponentPort"},
    {kComponentStream,                "kComponentStream"},
    {kComponentSteering,              "kComponentSteering"},
    {kComponentOptics,                "kComponentOptics"},
    {kComponentTerf,                  "kComponentTerf"},
    {kComponentTrTerf,                "kComponentTrTerf"},
    {kComponentPbm,                   "kComponentPbm"},
    {kComponentHardwareMonitor,       "kComponentHardwareMonitor"},
    {kComponentSingleAttributeCounter,"kComponentSingleAttributeCounter"},
    {kComponentSC256,                 "kComponentSC256"},
    {kComponentErfMux,                "kComponentErfMux"},
    {kComponentMem,                   "kComponentMem"},
    {kComponentSonic,                 "kComponentSonic"},
    {kComponentXGMII,                 "kComponentXGMII"},
    {kComponentXGMIIStatistics,       "kComponentXGMIIStatistics"},
    {kComponentDPA,                   "kComponentDPA"},
    {kComponentInterface,             "kComponentInterface"},
    {kComponentCounter,               "kComponentCounter"},
    {kComponentGeneral,               "kComponentGeneral"},
    {kComponentAllPorts,              "kComponentAllPorts"},
    {kComponentAllGpps,               "kComponentAllGpps"},
    {kComponentHlb,                   "kComponentHlb"},
    {kComponentPCS,                   "kComponentPCS"},
    {kComponentCardInfo,              "kComponentCardInfo"},
    {kComponentSonetPP,	              "kComponentSonetPP"},
    {kComponentStreamFeatures,	     "kComponentStreamFeatures"},
    {kComponentIPF,	              "kComponentIPF"},
    {kComponentDSM,                "kComponentDSM"},
    {kComponentCrossPointSwitch,   "kComponentCrossPointSwitch"},
    {kComponentInfiniBandFramer,   "kComponentInfiniBandFramer"},
    {kComponentInfinibandClassifier, "kComponentInfinibandClassifier"},
    {kComponentCAT,                  "kComponentCAT"},
    {kComponentHAT,                  "kComponentHAT"}
    
//    kFirstComponentCode = kComponentRoot, "
//    kLastComponentCode = kComponentSonetPP
};

static dag_attribute_code_string_map_t dag_attribute_code_string[] = {
    {kAttributeInvalid,                                "kAttributeInvalid"},
    {kBooleanAttributeActive,                          "kBooleanAttributeActive"},
    {kBooleanAttributeAlarmSignal,                     "kBooleanAttributeAlarmSignal"},
    {kBooleanAttributeAlign64,                         "kBooleanAttributeAlign64"},
    {kBooleanAttributeAutoNegotiationComplete,         "kBooleanAttributeAutoNegotiationComplete"},
    {kBooleanAttributeATMScramble,                     "kBooleanAttributeATMScramble"},
    {kBooleanAttributeB1Error,                         "kBooleanAttributeB1Error"},
    {kBooleanAttributeB2Error,                         "kBooleanAttributeB2Error"},
    {kBooleanAttributeB3Error,                         "kBooleanAttributeB3Error"},
    {kBooleanAttributeCore,                            "kBooleanAttributeCore"},
    {kBooleanAttributeClear,                           "kBooleanAttributeClear"},
    {kBooleanAttributeCRCErrorEverLo,                  "kBooleanAttributeCRCErrorEverLo"},
    {kBooleanAttributeCRCErrorEverHi,                  "kBooleanAttributeCRCErrorEverHi"},
//    {kBooleanAttributeCrcInsert,                       "kBooleanAttributeCrcInsert"},
    {kUint32AttributeDirection,                        "kUint32AttributeDirection"},
    {kBooleanAttributeDiscardInvalidFcs,               "kBooleanAttributeDiscardInvalidFcs"},
    {kBooleanAttributeDetect,                          "kBooleanAttributeDetect"},
    {kBooleanAttributeDUCKSynchronized,                "kBooleanAttributeDUCKSynchronized"},
    {kUint32AttributeDUCKThreshold,                    "kUint32AttributeDUCKThreshold"},
    {kUint32AttributeDUCKFailures,                     "kUint32AttributeDUCKFailures"},
    {kUint32AttributeDUCKResyncs,                      "kUint32AttributeDUCKResyncs"},
    {kInt32AttributeDUCKFrequencyError,                "kInt32AttributeDUCKFrequencyError"},
    {kInt32AttributeDUCKPhaseError,                    "kInt32AttributeDUCKPhaseError"},
    {kUint32AttributeDUCKWorstFrequencyError,          "kUint32AttributeDUCKWorstFrequencyError"},
    {kUint32AttributeDUCKWorstPhaseError,              "kUint32AttributeDUCKWorstPhaseError"},
    {kUint32AttributeDUCKCrystalFrequency,             "kUint32AttributeDUCKCrystalFrequency"},
    {kUint32AttributeDUCKSynthFrequency,               "kUint32AttributeDUCKSynthFrequency"},
    {kUint32AttributeDUCKPulses,                       "kUint32AttributeDUCKPulses"},
    {kUint32AttributeDUCKSinglePulsesMissing,          "kUint32AttributeDUCKSinglePulsesMissing"},
    {kUint32AttributeDUCKLongestPulseMissing,          "kUint32AttributeDUCKLongestPulseMissing"},
    {kBooleanAttributeDUCKRS422Input,                  "kBooleanAttributeDUCKRS422Input"},
    {kBooleanAttributeDUCKHostInput,                   "kBooleanAttributeDUCKHostInput"},
    {kBooleanAttributeDUCKOverInput,                   "kBooleanAttributeDUCKOverInput"},
    {kBooleanAttributeDUCKAuxInput,                    "kBooleanAttributeDUCKAuxInput"},
    {kBooleanAttributeDUCKRS422Output,                 "kBooleanAttributeDUCKRS422Output"},
    {kBooleanAttributeDUCKLoop,                        "kBooleanAttributeDUCKLoop"},
    {kBooleanAttributeDUCKHostOutput,                  "kBooleanAttributeDUCKHostOutput"},
    {kBooleanAttributeDUCKOverOutput,                  "kBooleanAttributeDUCKOverOutput"},
    {kBooleanAttributeDUCKSetToHost,                   "kBooleanAttributeDUCKSetToHost"},
    {kNullAttributeDUCKSync,                           "kNullAttributeDUCKSync"},
    {kUint32AttributeDUCKSyncTimeout,                  "kUint32AttributeDUCKSyncTimeout"},
    {kUint64AttributeDUCKTSC,                          "kUint64AttributeDUCKTSC"},
    {kStructAttributeDUCKTimeInfo,                     "kStructAttributeDUCKTimeInfo"},
    {kNullAttributeDUCKSetToHost,                      "kNullAttributeDUCKSetToHost"},
    {kNullAttributeDUCKClearStats,                     "kNullAttributeDUCKClearStats"},
    {kUint64AttributeDUCKPhaseCorrection,              "kUint64AttributeDUCKPhaseCorrection"},
    {kBooleanAttributeDrop,                            "kBooleanAttributeDrop"},
    {kBooleanAttributeDriverMonitorOutput,             "kBooleanAttributeDriverMonitorOutput"},
    {kBooleanAttributeEquipmentLoopback,               "kBooleanAttributeEquipmentLoopback"},
    {kBooleanAttributeE1T1AISError,                    "kBooleanAttributeE1T1AISError"},
    {kBooleanAttributeE1T1CRCError,                    "kBooleanAttributeE1T1CRCError"},
    {kBooleanAttributeE1T1FramerError,                 "kBooleanAttributeE1T1FramerError"},
    {kBooleanAttributeE1T1GenerateAlarmIndication,     "kBooleanAttributeE1T1GenerateAlarmIndication"},
    {kBooleanAttributeE1T1GlobalConfiguration,         "kBooleanAttributeE1T1GlobalConfiguration"},
    {kBooleanAttributeE1T1Link,                        "kBooleanAttributeE1T1Link"},
    {kBooleanAttributeE1T1LinkAIS,                     "kBooleanAttributeE1T1LinkAIS"},
    {kBooleanAttributeE1T1LinkCRCError,                "kBooleanAttributeE1T1LinkCRCError"},
    {kBooleanAttributeE1T1LinkFramingError,            "kBooleanAttributeE1T1LinkFramingError"},
    {kBooleanAttributeE1T1LinkSynchronized,            "kBooleanAttributeE1T1LinkSynchronized"},
    {kBooleanAttributeE1T1LinkSynchronizedUp,          "kBooleanAttributeE1T1LinkSynchronizedUp"},
    {kBooleanAttributeE1T1LinkSynchronizedDown,        "kBooleanAttributeE1T1LinkSynchronizedDown"},
    {kBooleanAttributeE1T1Resynchronize,               "kBooleanAttributeE1T1Resynchronize"},
    {kBooleanAttributeE1T1Rx0,                         "kBooleanAttributeE1T1Rx0"},
    {kBooleanAttributeE1T1Rx1,                         "kBooleanAttributeE1T1Rx1"},
    {kBooleanAttributeE1T1Tx0,                         "kBooleanAttributeE1T1Tx0"},
    {kBooleanAttributeE1T1Tx1,                         "kBooleanAttributeE1T1Tx1"},
    {kBooleanAttributeEnableCounterModules,            "kBooleanAttributeEnableCounterModules"},
    {kBooleanAttributeEnableTCPFlowCounter,            "kBooleanAttributeEnableTCPFlowCounter"},
    {kBooleanAttributeEnableIPAddressCounter,          "kBooleanAttributeEnableIPAddressCounter"},
    {kBooleanAttributeEnablePacketCaptureModules,      "kBooleanAttributeEnablePacketCaptureModules"},
    {kBooleanAttributeEnableProbabilityHashTable,      "kBooleanAttributeEnableProbabilityHashTable"},
    {kBooleanAttributeEnableProbabilitySampler,        "kBooleanAttributeEnableProbabilitySampler"},
    {kBooleanAttributeEnableHostFlowTable,             "kBooleanAttributeEnableHostFlowTable"},
    {kBooleanAttributeEnableHostHashTable,             "kBooleanAttributeEnableHostHashTable"},
    {kBooleanAttributeFacilityLoopback,                "kBooleanAttributeFacilityLoopback"},
    {kBooleanAttributeFIFOLimitStatus,                 "kBooleanAttributeFIFOLimitStatus"},
    {kBooleanAttributeFIFOError,                       "kBooleanAttributeFIFOError"},
    {kBooleanAttributeFullDuplex,                      "kBooleanAttributeFullDuplex"},
    {kBooleanAttributeGenerateIPCounterErf,            "kBooleanAttributeGenerateIPCounterErf"},
    {kBooleanAttributeGenerateTCPFlowCounterErf,       "kBooleanAttributeGenerateTCPFlowCounterErf"},
    {kBooleanAttributeHashTableReady,                  "kBooleanAttributeHashTableReady"},
    {kBooleanAttributeHECCorrection,                   "kBooleanAttributeHECCorrection"},
    {kBooleanAttributeIdleCellMode,                    "kBooleanAttributeIdleCellMode"},
    {kBooleanAttributeInterfaceRewrite,                "kBooleanAttributeInterfaceRewrite"},
    {kBooleanAttributeJabber,                          "kBooleanAttributeJabber"},
    {kBooleanAttributeLaser,                           "kBooleanAttributeLaser"},
    {kBooleanAttributeLock,                            "kBooleanAttributeLock"},
    {kBooleanAttributeLineCodeViolation,               "kBooleanAttributeLineCodeViolation"},
    {kBooleanAttributeLossOfCellDelineation,           "kBooleanAttributeLossOfCellDelineation"},
    {kBooleanAttributeLossOfPointer,                   "kBooleanAttributeLossOfPointer"},
    {kBooleanAttributeLossOfSignal,                    "kBooleanAttributeLossOfSignal"},
    {kBooleanAttributeSFPLossOfSignalCurrent,          "kBooleanAttributeSFPLossOfSignalCurrent"},
    {kBooleanAttributeSFPLossOfSignalEverLo,           "kBooleanAttributeSFPLossOfSignalEverLo"},
    {kBooleanAttributeSFPLossOfSignalEverHi,           "kBooleanAttributeSFPLossOfSignalEverHi"},
    {kBooleanAttributeLossOfFrame,                     "kBooleanAttributeLossOfFrame"},
    {kBooleanAttributeLossOfFramingCurrent,            "kBooleanAttributeLossOfFramingCurrent"},
    {kBooleanAttributeLossOfFramingEverHi,             "kBooleanAttributeLossOfFramingEverHi"},
    {kBooleanAttributeLossOfFramingEverLo,             "kBooleanAttributeLossOfFramingEverLo"},
    {kBooleanAttributeMiniMacLostSync,                 "kBooleanAttributeMiniMacLostSync"},
    {kBooleanAttributeMerge,                           "kBooleanAttributeMerge"},
    {kBooleanAttributeNic,                             "kBooleanAttributeNic"},
    {kBooleanAttributeOutOfFrame,                      "kBooleanAttributeOutOfFrame"},
    {kBooleanAttributePayloadScramble,                 "kBooleanAttributePayloadScramble"},
    {kBooleanAttributeReceiveLossOfSignal,             "kBooleanAttributeReceiveLossOfSignal"},
    {kBooleanAttributeRDIError,                        "kBooleanAttributeRDIError"},
    {kUint32AttributeERDIError,                        "kUint32AttributeERDIError"},
    {kBooleanAttributeREIError,                        "kBooleanAttributeREIError"},
    {kBooleanAttributeRemoteErrorCurrent,              "kBooleanAttributeRemoteErrorCurrent"},
    {kBooleanAttributeRemoteErrorEverLo,               "kBooleanAttributeRemoteErrorEverLo"},
    {kBooleanAttributeRemoteErrorEverHi,               "kBooleanAttributeRemoteErrorEverHi"},
    {kBooleanAttributeReset,                           "kBooleanAttributeReset"},
    {kBooleanAttributeResetHashTable,                  "kBooleanAttributeResetHashTable"},
    {kBooleanAttributeResetProbabilityHashTable,       "kBooleanAttributeResetProbabilityHashTable"},
    {kBooleanAttributeRocketIOPower,                   "kBooleanAttributeRocketIOPower"},
    {kBooleanAttributeTxCrc,                           "kBooleanAttributeTxCrc"},
    {kBooleanAttributeScramble,                        "kBooleanAttributeScramble"},
    {kBooleanAttributeSfpPwr,                          "kBooleanAttributeSfpPwr"},
    {kBooleanAttributeSFPDetect,                       "kBooleanAttributeSFPDetect"},
    {kBooleanAttributeSFPTxFaultCurrent,               "kBooleanAttributeSFPTxFaultCurrent"},
    {kBooleanAttributeSFPTxFaultEverLo,                "kBooleanAttributeSFPTxFaultEverLo"},
    {kBooleanAttributeSFPTxFaultEverHi,                "kBooleanAttributeSFPTxFaultEverHi"},
    {kBooleanAttributeVarlen,                          "kBooleanAttributeVarlen"},
    {kBooleanAttributeTxPkts,                          "kBooleanAttributeTxPkts"},
    {kBooleanAttributeRxPkts,                          "kBooleanAttributeRxPkts"},
    {kBooleanAttributeSignal,                          "kBooleanAttributeSignal"},
    {kBooleanAttributeSync,                            "kBooleanAttributeSync"},
    {kBooleanAttributeSplit,                           "kBooleanAttributeSplit"},
    {kBooleanAttributeSwap,                            "kBooleanAttributeSwap"},
    {kBooleanAttributeLink,                            "kBooleanAttributeLink"},
    {kBooleanAttributeLinkCurrent,                     "kBooleanAttributeLinkCurrent"},
    {kBooleanAttributeLinkEverHi,                      "kBooleanAttributeLinkEverHi"},
    {kBooleanAttributeLinkEverLo,                      "kBooleanAttributeLinkEverLo"},
    {kBooleanAttributeRemoteFault,                     "kBooleanAttributeRemoteFault"},
    {kBooleanAttributeLocalFault,                      "kBooleanAttributeLocalFault"},
    {kBooleanAttributeOverlap,                         "kBooleanAttributeOverlap"},
    {kBooleanAttributePeerLink,                        "kBooleanAttributePeerLink"},
    {kBooleanAttributePeerLinkCurrent,                 "kBooleanAttributePeerLinkCurrent"},
    {kBooleanAttributePeerLinkEverLo,                  "kBooleanAttributePeerLinkEverLo"},
    {kBooleanAttributePeerLinkEverHi,                  "kBooleanAttributePeerLinkEverHi"},
    {kBooleanAttributeCrcStrip,                        "kBooleanAttributeCrcStrip"},
    {kBooleanAttributeLinkDiscard,                     "kBooleanAttributeLinkDiscard"},
    {kBooleanAttributeLineAlarmIndicationSignal,       "kBooleanAttributeLineAlarmIndicationSignal"},
    {kBooleanAttributeLineRemoteDefectIndicationSignal,"kBooleanAttributeLineRemoteDefectIndicationSignal"},
    {kBooleanAttributeDataOutOfLock,                   "kBooleanAttributeDataOutOfLock"},
    {kBooleanAttributeReferenceOutOfLock,              "kBooleanAttributeReferenceOutOfLock"},
    {kBooleanAttributeLineSideFacilityLoopback,        "kBooleanAttributeLineSideFacilityLoopback"},
    {kBooleanAttributeLineSideEquipmentLoopback,       "kBooleanAttributeLineSideEquipmentLoopback"},
    {kBooleanAttributePMinCheck,                       "kBooleanAttributePMinCheck"},
    {kBooleanAttributePMaxCheck,                       "kBooleanAttributePMaxCheck"},
    {kBooleanAttributeLossOfClock,                     "kBooleanAttributeLossOfClock"},
    {kBooleanAttributeTxParityError,                   "kBooleanAttributeTxParityError"},
    {kBooleanAttributeHighBitErrorRateDetected,        "kBooleanAttributeHighBitErrorRateDetected"},
    {kBooleanAttributeTimeStampEnd,                    "kBooleanAttributeTimeStampEnd"},
    {kBooleanAttributeTransmitFIFOError,               "kBooleanAttributeTransmitFIFOError"},
    {kBooleanAttributeTransmitAlarmIndication,         "kBooleanAttributeTransmitAlarmIndication"},
    {kBooleanAttributeLaserBiasAlarm,                  "kBooleanAttributeLaserBiasAlarm"},
    {kBooleanAttributeLaserTemperatureAlarm,           "kBooleanAttributeLaserTemperatureAlarm"},
    {kBooleanAttributeTransmitLockError,               "kBooleanAttributeTransmitLockError"},
    {kBooleanAttributeRefreshCache,                    "kBooleanAttributeRefreshCache"},
    {kBooleanAttributeTCAMInit,                        "kBooleanAttributeTCAMInit"},
    {kBooleanAttributeResetTCPFlowCounter,             "kBooleanAttributeResetTCPFlowCounter"},
    {kBooleanAttributeResetIPCounter,                  "kBooleanAttributeResetIPCounter"},
    {kBooleanAttributePromiscuousMode,                 "kBooleanAttributePromiscuousMode"},
    {kBooleanAttributeRemoteDefectIndication,          "kBooleanAttributeRemoteDefectIndication"},
    {kBooleanAttributeAICM23Cbit,                      "kBooleanAttributeAICM23Cbit"},
    {kBooleanAttributeAlarmIndicationSignal,           "kBooleanAttributeAlarmIndicationSignal"},
    {kBooleanAttributeTxFault,                         "kBooleanAttributeTxFault"},
    {kBooleanAttributeRxFault,                         "kBooleanAttributeRxFault"},
    {kBooleanAttributeFault,                           "kBooleanAttributeFault"},
    {kUint32AttributeTcpFlowCounter,                   "kUint32AttributeTcpFlowCounter"},
    {kUint32AttributeIpAddressCounter,                 "kUint32AttributeIpAddressCounter"},
    {kUint32AttributeBERCounter,                       "kUint32AttributeBERCounter"},
    {kBooleanAttributeHiBER,                           "kBooleanAttributeHiBER"},
    {kUint32AttributeErrorBlockCounter,                "kUint32AttributeErrorBlockCounter"},
    {kNullAttributeClearConnections,                   "kNullAttributeClearConnections"},
    {kNullAttributeSC256Init,                          "kNullAttributeSC256Init"},
    {kUint32AttributePCIBusSpeed,                      "kUint32AttributePCIBusSpeed"},
    {kUint32AttributeBadSymbols,                       "kUint32AttributeBadSymbols"},
    {kUint32AttributeBufferSize,                       "kUint32AttributeBufferSize"},
    {kUint32AttributeB1ErrorCount,                     "kUint32AttributeB1ErrorCount"},
    {kUint32AttributeB2ErrorCount,                     "kUint32AttributeB2ErrorCount"},
    {kUint32AttributeB3ErrorCount,                     "kUint32AttributeB3ErrorCount"},
    {kUint32AttributeREIErrorCount,                    "kUint32AttributeREIErrorCount"},
    {kUint32AttributeByteCount,                        "kUint32AttributeByteCount"},
//    {kUint32AttributeCrc,                              "kUint32AttributeCrc"},
    {kUint32AttributeC2PathLabel,                      "kUint32AttributeC2PathLabel"},
    {kUint32AttributeConfigSequences,                  "kUint32AttributeConfigSequences"},
    {kUint32AttributeCableLoss,                        "kUint32AttributeCableLoss"},
    {kUint32AttributeClock,                            "kUint32AttributeClock"},
    {kUint32AttributeCRCErrors,                        "kUint32AttributeCRCErrors"},
    {kBooleanAttributeCRCError,                        "kBooleanAttributeCRCError"},
    {kUint32AttributeCrcSelect,                        "kUint32AttributeCrcSelect"},
    {kUint32AttributeHLBRangeMax,                      "kUint32AttributeHLBRangeMax"},
    {kUint32AttributeHLBRangeMin,                      "kUint32AttributeHLBRangeMin"},
    {kUint32AttributeConnectionNumber,                 "kUint32AttributeConnectionNumber"},
    {kUint32AttributeConnectionVCLabel,                "kUint32AttributeConnectionVCLabel"},
    {kUint32AttributeConnectionVCPointer,              "kUint32AttributeConnectionVCPointer"},
    {kUint32AttributeConnectionType,                   "kUint32AttributeConnectionType"},
    {kUint32AttributeDeframerRevisionID,               "kUint32AttributeDeframerRevisionID"},
    {kUint32Attribute71sChannelizedRevisionID,         "kUint32Attribute71sChannelizedRevisionID"},
    {kUint32AttributeDataPointer,                      "kUint32AttributeDataPointer"},
    {kUint32AttributeDeleteConnection,                 "kUint32AttributeDeleteConnection"},
    {kUint32AttributeE1T1FrameType,                    "kUint32AttributeE1T1FrameType"},
    {kUint32AttributeE1T1StreamNumber,                 "kUint32AttributeE1T1StreamNumber"},
    {kUint32AttributeErrorCounter,                     "kUint32AttributeErrorCounter"},
    {kUint32AttributeGetLastConnectionNumber,          "kUint32AttributeGetLastConnectionNumber"},
    {kUint32AttributeHECCount,                         "kUint32AttributeHECCount"},
    {kUint32AttributeIdleCellCount,                    "kUint32AttributeIdleCellCount"},
    {kUint32AttributeJ0PathLabel,                      "kUint32AttributeJ0PathLabel"},
    {kUint32AttributeJ1PathLabel,                      "kUint32AttributeJ1PathLabel"},
    {kUint32AttributeLEDStatus,                        "kUint32AttributeLEDStatus"},
    {kUint32AttributeLineRate,                         "kUint32AttributeLineRate"},
    {kUint32AttributeLineInterface,                    "kUint32AttributeLineInterface"},
    {kUint32AttributeLossOfCellDelineationCount,       "kUint32AttributeLossOfCellDelineationCount"},
    {kUint32AttributeForceLineRate,                    "kUint32AttributeForceLineRate"},
    {kUint32AttributeLineType,                         "kUint32AttributeLineType"},
    {kUint32AttributePeriod,                           "kUint32AttributePeriod"},
    {kUint32AttributeDutyCycle,                        "kUint32AttributeDutyCycle"},
    {kUint32AttributeLimitPointer,                     "kUint32AttributeLimitPointer"},
    {kUint32AttributeRecordPointer,                    "kUint32AttributeRecordPointer"},
    {kUint32AttributeMasterSlave,                      "kUint32AttributeMasterSlave"},
    {kUint32AttributeMem,                              "kUint32AttributeMem"},
    {kUint32AttributeMemBytes,                         "kUint32AttributeMemBytes"},
    {kUint32AttributeMode,                             "kUint32AttributeMode"},
    {kUint32AttributeMux,                              "kUint32AttributeMux"},
    {kUint32AttributePayloadType,                      "kUint32AttributePayloadType"},
    {kUint32AttributePointerState,                     "kUint32AttributePointerState"},
    {kUint32AttributeSSM,                              "kUint32AttributeSSM"},
    {kUint32AttributeSetCaptureHash,                   "kUint32AttributeSetCaptureHash"},
    {kUint32AttributeZeroCodeSuppress,                 "kUint32AttributeZeroCodeSuppress"},
    {kUint32AttributeNetworkMode,                      "kUint32AttributeNetworkMode"},
    {kUint32AttributePayloadMapping,                   "kUint32AttributePayloadMapping"},
    {kUint32AttributeRxFrames,                         "kUint32AttributeRxFrames"},
    {kUint32AttributeShortPacketErrors,                "kUint32AttributeShortPacketErrors"},
    {kUint32AttributeLongPacketErrors,                 "kUint32AttributeLongPacketErrors"},
    {kUint32AttributeSnaplength,                       "kUint32AttributeSnaplength"},
    {kUint32AttributeHDLCSnaplength,                   "kUint32AttributeHDLCSnaplength"},
    {kUint32AttributeRAWSnaplength,                    "kUint32AttributeRAWSnaplength"},
    {kUint32AttributeTemperature,                      "kUint32AttributeTemperature"},
    {kUint32AttributeTerfStripCrc,                     "kUint32AttributeTerfStripCrc"},
    {kBooleanAttributeRXErrorA,                        "kBooleanAttributeRXErrorA"},
    {kBooleanAttributeRXErrorB,                        "kBooleanAttributeRXErrorB"},
    {kBooleanAttributeRXErrorC,                        "kBooleanAttributeRXErrorC"},
    {kBooleanAttributeRXErrorD,                        "kBooleanAttributeRXErrorD"},
    {kUint32AttributeTimeMode,                         "kUint32AttributeTimeMode"},
    {kUint32AttributeTimeslotPattern,                  "kUint32AttributeTimeslotPattern"},
    {kUint32AttributeScaleRange,                       "kUint32AttributeScaleRange"},
    {kBooleanAttributeShiftDirection,                  "kBooleanAttributeShiftDirection"},
    {kBooleanAttributePhyBistEnable,                   "kBooleanAttributePhyBistEnable"},
    {kBooleanAttributePhyDLEB,                         "kBooleanAttributePhyDLEB"},
    {kBooleanAttributePhyLLEB,                         "kBooleanAttributePhyLLEB"},
    {kBooleanAttributePhyTxClockOff,                   "kBooleanAttributePhyTxClockOff"},
    {kBooleanAttributePhyKillRxClock,                  "kBooleanAttributePhyKillRxClock"},
    {kUint32AttributePhyRate,                          "kUint32AttributePhyRate"},
    {kUint32AttributePhyRefSelect,                     "kUint32AttributePhyRefSelect"},
    {kUint32AttributeConfig,                           "kUint32AttributeConfig"},
    {kBooleanAttributeDiscardData,                     "kBooleanAttributeDiscardData"},
    {kUint32AttributeTermination,                      "kUint32AttributeTermination"},
    {kUint32AttributeSonetType,                        "kUint32AttributeSonetType"},
    {kUint32AttributeRemoteErrors,                     "kUint32AttributeRemoteErrors"},
    {kUint32AttributeRxStreamCount,                    "kUint32AttributeRxStreamCount"},
    {kUint32AttributeTxStreamCount,                    "kUint32AttributeTxStreamCount"},
    {kUint32AttributeTributaryUnit,                    "kUint32AttributeTributaryUnit"},
    {kUint32AttributeDropCount,                        "kUint32AttributeDropCount"},
    {kBooleanAttributeSuppressError,                   "kBooleanAttributeSuppressError"},
    {kUint32AttributeUnsetCaptureHash,                 "kUint32AttributeUnsetCaptureHash"},
    {kUint32AttributeSC256DataAddress,                 "kUint32AttributeSC256DataAddress"},
    {kUint32AttributeSC256MaskAddress,                 "kUint32AttributeSC256MaskAddress"},
    {kUint32AttributeSC256SearchLength,                "kUint32AttributeSC256SearchLength"},
    {kUint32AttributeVCNumber,                         "kUint32AttributeVCNumber"},
    {kUint32AttributeVCSize,                           "kUint32AttributeVCSize"},
    {kUint32AttributeVCIndex,                          "kUint32AttributeVCIndex"},
    {kUint32AttributeVCMaxIndex,                       "kUint32AttributeVCMaxIndex"},
    {kUint32AttributeTUNumber,                         "kUint32AttributeTUNumber"},
    {kUint32AttributeTUG2Number,                       "kUint32AttributeTUG2Number"},
    {kUint32AttributeTUG3Number,                       "kUint32AttributeTUG3Number"},
    {kUint32AttributePortNumber,                       "kUint32AttributePortNumber"},
    {kUint32AttributeVoltage,                          "kUint32AttributeVoltage"},
    {kUint32AttributeTxFrames,                         "kUint32AttributeTxFrames"},
    {kUint32AttributePathBIPError,                     "kUint32AttributePathBIPError"},
    {kUint32AttributePathREIError,                     "kUint32AttributePathREIError"},
    {kUint32AttributeMinPktLen,                        "kUint32AttributeMinPktLen"},
    {kUint32AttributeMaxPktLen,                        "kUint32AttributeMaxPktLen"},
    {kUint32AttributeTxFDrop,                          "kUint32AttributeTxFDrop"},
    {kUint32AttributeAborts,                           "kUint32AttributeAborts"},
    {kUint32AttributeMinPktLenError,                   "kUint32AttributeMinPktLenError"},
    {kUint32AttributeMaxPktLenError,                   "kUint32AttributeMaxPktLenError"},
    {kUint32AttributeRxFDrop,                          "kUint32AttributeRxFDrop"},
    {kUint32AttributePacketCaptureThreshold,           "kUint32AttributePacketCaptureThreshold"},
    {kUint32AttributeFlowCaptureThreshold,             "kUint32AttributeFlowCaptureThreshold"},
    {kUint32AttributeSteer,                            "kUint32AttributeSteer"},
    {kUint32AttributeEthernetMode,                     "kUint32AttributeEthernetMode"},
    {kUint32AttributeFramingMode,                      "kUint32AttributeFramingMode"},
    {kBooleanAttributeReadyEnableTCPFlowModule,        "kBooleanAttributeReadyEnableTCPFlowModule"},
    {kBooleanAttributeReadyEnableIPCounterModule,      "kBooleanAttributeReadyEnableIPCounterModule"},
    {kBooleanAttributeReceiveAlarmIndication,          "kBooleanAttributeReceiveAlarmIndication"},
    {kBooleanAttributeReceiveLockError,                "kBooleanAttributeReceiveLockError"},
    {kBooleanAttributeReceivePowerAlarm,               "kBooleanAttributeReceivePowerAlarm"},
    {kBooleanAttributeDescramble,                      "kBooleanAttributeDescramble"},
    {kBooleanAttributeRXMonitorMode,                   "kBooleanAttributeRXMonitorMode"},
    {kUint32AttributeRxParityError,                    "kUint32AttributeRxParityError"},
    {kUint32AttributeTxFIFOError,                      "kUint32AttributeTxFIFOError"},
    {kUint64AttributeRxBytes,                          "kUint64AttributeRxBytes"},
    {kUint32AttributeRxBytes,                          "kUint32AttributeRxBytes"},
    {kUint64AttributeRxBytesBad,                       "kUint64AttributeRxBytesBad"},
    {kUint64AttributeBadSymbol,                        "kUint64AttributeBadSymbol"},
    {kUint64AttributeCRCFail,                          "kUint64AttributeCRCFail"},
    {kUint64AttributeRxFrames,                         "kUint64AttributeRxFrames"},
    {kUint64AttributeTxBytes,                          "kUint64AttributeTxBytes"},
    {kUint32AttributeTxBytes,                          "kUint32AttributeTxBytes"},
    {kUint64AttributeTxFrames,                         "kUint64AttributeTxFrames"},
    {kUint64AttributeInternalMACError,                 "kUint64AttributeInternalMACError"},
    {kUint64AttributeTransmitSystemError,              "kUint64AttributeTransmitSystemError"},
    {kUint64AttributeFCSErrors,                        "kUint64AttributeFCSErrors"},
    {kUint64AttributeBadPackets,                       "kUint64AttributeBadPackets"},
    {kUint64AttributeGoodPackets,                      "kUint64AttributeGoodPackets"},
    {kUint64AttributeFIFOOverrunCount,                 "kUint64AttributeFIFOOverrunCount"},
    {kStringAttributeMem,                              "kStringAttributeMem"},
    {kStructAttributeAddConnection,                    "kStructAttributeAddConnection"},
    {kStructAttributeErfMux,                           "kStructAttributeErfMux"},
    {kStructAttributeSC25672BitData,                   "kStructAttributeSC25672BitData"},
    {kStructAttributeSC25672BitMask,                   "kStructAttributeSC25672BitMask"},
    {kStructAttributeSC25672BitSearch,                 "kStructAttributeSC25672BitSearch"},
    {kStructAttributeSC256144BitSearch,                "kStructAttributeSC256144BitSearch"},
    {kBooleanAttributeCounterLatch,                    "kBooleanAttributeCounterLatch"},
    {kUint32AttributeLineSteeringMode,                 "kUint32AttributeLineSteeringMode"},
    {kUint32AttributeHostSteeringMode,                 "kUint32AttributeHostSteeringMode"},
    {kUint32AttributeIXPSteeringMode,                  "kUint32AttributeIXPSteeringMode"},
    {kBooleanAttributeDPAFault,                        "kBooleanAttributeDPAFault"},
    {kBooleanAttributeDPAStart,                        "kBooleanAttributeDPAStart"},
    {kBooleanAttributeDPADone,                         "kBooleanAttributeDPADone"},
    {kBooleanAttributeDPADcmError,                     "kBooleanAttributeDPADcmError"},
    {kBooleanAttributeDPADcmLock,                      "kBooleanAttributeDPADcmLock"},
    {kStringAttributeEthernetMACAddress,               "kStringAttributeEthernetMACAddress"},
    {kUint32AttributeCSIType,                          "kUint32AttributeCSIType"},
    {kUint32AttributeNbCounters,                       "kUint32AttributeNbCounters"},
    {kBooleanAttributeLatchClear,                      "kBooleanAttributeLatchClear"},
    {kUint32AttributeCounterDescBaseAdd,               "kUint32AttributeCounterDescBaseAdd"},
    {kUint32AttributeCounterValueBaseAdd,              "kUint32AttributeCounterValueBaseAdd"},
    {kUint32AttributeCounterID,                        "kUint32AttributeCounterID"},
    {kUint32AttributeSubFunction,                      "kUint32AttributeSubFunction"},
    {kUint32AttributeSubFunctionType,                  "kUint32AttributeSubFunctionType"},
    {kBooleanAttributeValueType,                       "kBooleanAttributeValueType"},
    {kUint32AttributeCounterSize,                      "kUint32AttributeCounterSize"},
    {kBooleanAttributeAccess,                          "kBooleanAttributeAccess"},
    {kUint64AttributeValue,                            "kUint64AttributeValue"},
    {kStructAttributeHlbRange,                         "kStructAttributeHlbRange"},
    {kBooleanAttributeTxFIFOOverflow,                  "kBooleanAttributeTxFIFOOverflow"},
    {kBooleanAttributeRxFIFOOverflow,                  "kBooleanAttributeRxFIFOOverflow"},
    {kBooleanAttributeTxFIFOFull,                      "kBooleanAttributeTxFIFOFull"},
    {kBooleanAttributeRxFIFOEmpty,                     "kBooleanAttributeRxFIFOEmpty"},
    {kBooleanAttributeSonetMode,                       "kBooleanAttributeSonetMode"},
    {kStringAttributeFactoryFirmware,                  "kStringAttributeFactoryFirmware"},
    {kStringAttributeUserFirmware,                     "kStringAttributeUserFirmware"},
    {kUint32AttributeActiveFirmware,                   "kUint32AttributeActiveFirmware"},
    {kUint32AttributeSerialID,                         "kUint32AttributeSerialID"},
    {kStringAttributePCIInfo,                          "kStringAttributePCIInfo"},
    {kUint32AttributeCoPro,                            "kUint32AttributeCoPro"},
    {kUint32AttributeStreamDropCount,                  "kUint32AttributeStreamDropCount"},
    {kBooleanAttributeIDELAY_Present,                  "kBooleanAttributeIDELAY_Present"},
    {kUint32AttributeIDELAY,                           "kUint32AttributeIDELAY "},
    {kUint32AttributeNumberOfStreams,               "kUint32AttributeNumberOfStreams"},
    {kUint32AttributeNumberOfRegisters,               "kUint32AttributeNumberOfRegisters"},
    {kBooleanAttributeSnaplengthPresent,               "kBooleanAttributeSnaplengthPresent"},
    {kUint32AttributePerStreamSnaplength,               "kUint32AttributePerStreamSnaplength"},
    {kUint32AttributeMaxSnapLen,               "kUint32AttributeMaxSnapLen"},
    {kUint32AttributeInterfaceCount,               "kUint32AttributeInterfaceCount"},
    {kBooleanAttributeClearTrigger,                    "kBooleanAttributeClearTrigger"},
    {kBooleanAttributeTriggerPending,                  "kBooleanAttributeTriggerPending"},
    {kBooleanAttributeTriggerOccured,                  "kBooleanAttributeTriggerOccured"},
    {kUint64AttributeTriggerTimestamp,                  "kUint64AttributeTriggerTimestamp"},
    {kBooleanAttributeIPFEnable,               "kBooleanAttributeIPFEnable"},
    {kBooleanAttributeIPFDropEnable,               "kBooleanAttributeIPFDropEnable"},
    {kBooleanAttributeIPFShiftColour,               "kBooleanAttributeIPFShiftColour"},
    {kBooleanAttributeIPFLinkType,               "kBooleanAttributeIPFLinkType"},
    {kBooleanAttributeIPFSelLctr,               "kBooleanAttributeIPFSelLctr"},
    {kBooleanAttributeIPFUseRXError,               "kBooleanAttributeIPFUseRXError"},
    {kBooleanAttributeIPFRulesetInterface0,               "kBooleanAttributeIPFRulesetInterface0"},
    {kBooleanAttributeIPFRulesetInterface1,               "kBooleanAttributeIPFRulesetInterface1"},
    {kUint64AttributeAbsModeOffset,                         "kUint64AttributeAbsModeOffset"},
    {kUint64AttributeConfLimit,             "kUint64AttributeConfLimit"},
	{kUint32AttributeTXV5SignalLabel,                  "kUint32AttributeTXV5SignalLabel"},
    {kStringAttributeFactoryCoproFirmware,                   "kStringAttributeFactoryCoproFirmware"},
    { kStringAttributeUserCoproFirmware, "kStringAttributeUserCoproFirmware"},
    {kUint32AttributeTransceiverIdentifier, "kUint32AttributeTransceiverIdentifier"},
    {kUint32AttributeTransceiverExtendedIdentifier, "kUint32AttributeTransceiverExtendedIdentifier"},
    {kStringAttributeTransceiverVendorName, "kStringAttributeTransceiverVendorName"},
    {kStringAttributeTransceiverVendorPN, "kStringAttributeTransceiverVendorPN"},
    {kStringAttributeTransceiverCodes, "kStringAttributeTransceiverCodes"},
    {kStringAttributeTransceiverLinkLength, "kStringAttributeTransceiverLinkLength"},
    {kStringAttributeTransceiverMedia, "kStringAttributeTransceiverMedia"},
    {kFloatAttributeTransceiverTemperature, "kFloatAttributeTransceiverTemperature"},
    {kUint32AttributeTransceiverVoltage, "kUint32AttributeTransceiverVoltage"},
    {kUint32AttributeTransceiverRxPower, "kUint32AttributeTransceiverRxPower"},
    {kUint32AttributeTransceiverTxPower, "kUint32AttributeTransceiverTxPower"},
    {kBooleanAttributeTransceiverMonitoring, "kBooleanAttributeTransceiverMonitoring"},
    {kFloatAttributeVoltage, "kFloatAttributeVoltage"},
    {kUint32AttributeTXC2PathLabel0, "kUint32AttributeTXC2PathLabel0"},
    {kUint32AttributeTXC2PathLabel1, "kUint32AttributeTXC2PathLabel1"},
    {kUint32AttributeTXC2PathLabel2, "kUint32AttributeTXC2PathLabel2"},
    {kUint32AttributeTXC2PathLabel3, "kUint32AttributeTXC2PathLabel3"},
    {kUint32AttributeTXC2PathLabel4, "kUint32AttributeTXC2PathLabel4"},
    {kUint32AttributeTXC2PathLabel5, "kUint32AttributeTXC2PathLabel5"},
    {kUint32AttributeTXC2PathLabel6, "kUint32AttributeTXC2PathLabel6"},
    {kUint32AttributeTXC2PathLabel7, "kUint32AttributeTXC2PathLabel7"},
    {kUint32AttributeTXC2PathLabel8, "kUint32AttributeTXC2PathLabel8"},
    {kUint32AttributeTXC2PathLabel9, "kUint32AttributeTXC2PathLabel9"},
    {kUint32AttributeTXC2PathLabel10, "kUint32AttributeTXC2PathLabel10"},
    {kUint32AttributeTXC2PathLabel11, "kUint32AttributeTXC2PathLabel11"},
    {kBooleanAttributeDSMBypass, "kBooleanAttributeDSMBypass" },
    {kUint64AttributeDuckTimestamp, "kUint64AttributeDuckTimestamp"},
    {kNullAttributeDefaultDS3ATM, "kNullAttributeDefaultDS3ATM"},
    {kNullAttributeDefaultDS3HDLC, "kNullAttributeDefaultDS3HDLC"},
    {kNullAttributeDefaultE3ATM, "kNullAttributeDefaultE3ATM"},
    {kNullAttributeDefaultE3HDLC, "kNullAttributeDefaultE3HDLC"},
    {kNullAttributeDefaultE3HDLCFract, "kNullAttributeDefaultE3HDLCFract"},
    {kNullAttributeDefaultKentrox, "kNullAttributeDefaultKentrox"},
    {kBooleanAttributeCellHeaderDescramble, "kBooleanAttributeCellHeaderDescramble"},
    {kUint32AttributeHDLCFraction, "kUint32AttributeHDLCFraction"},
    {kBooleanAttributeFF00Delete, "kBooleanAttributeFF00Delete"},
    {kBooleanAttributePerStreamDropCounterPresent, "kBooleanAttributePerStreamDropCounterPresent"},
    {kBooleanAttributeWriteEnable, "kBooleanAttributeWriteEnable"},
    {kBooleanAttributePerStreamAlmostFullDropPresent, "kBooleanAttributePerStreamAlmostFullDropPresent"},
    {kBooleanAttributeEnableInterfaceOverwrite, "kBooleanAttributeEnableInterfaceOverwrite"},
    {kUint32AttributeStreamAlmostFullDrop, "kUint32AttributeStreamAlmostFullDrop"},
    {kUint16AttributeForwardDestinations, "kUint16AttributeForwardDestinations"},
    {kBooleanAttributeBankSelect, "kBooleanAttributeBankSelect"},
    {kBooleanAttributeClearCounter, "kBooleanAttributeClearCounter"},
    {kBooleanAttributeByPass,"kBooleanAttributeByPass"},
    {kBooleanAttributeOutputType, "kBooleanAttributeOutputType"},
    {kUint32AttributeNumberOfOutputBits,"kUint32AttributeNumberOfOutputBits"},
    {kUint32AttributeNumberOfInputBits,"kUint32AttributeNumberOfInputBits"},
    {kUint32AttributeDeliberateDropCount,"kUint32AttributeDeliberateDropCount"},
    {kUint32AttributeAddressRegister,"kUint32AttributeAddressRegister"},
    {kInt32AttributeDataRegister,"kInt32AttributeDataRegister"},
    {kBooleanAttributeHatEncodingMode,"kBooleanAttributeHatEncodingMode"},
    {kUint32AttributeHatOutputBits ,"kUint32AttributeHatOutputBits"},
    {kUint32AttributeHatInputBits,"kUint32AttributeHatInputBits"},
    {kBooleanAttributeHashEncoding,"kBooleanAttributeHashEncoding"},
    {kUint32AttributeNtupleSelect, "kUint32AttributeNtupleSelect"},
    {kUint32AttributeSerialInterfaceEnable, "kUint32AttributeSerialInterfaceEnable"},
    {kBooleanAttributeEnergiseRtCore, "kBooleanAttributeEnergiseRtCore"},
    {kBooleanAttributeEnergiseLtCore , "kBooleanAttributeEnergiseLtCore"},
    {kBooleanAttributeLowGlitch, "kBooleanAttributeLowGlitch"},
    {kBooleanAttributeCoreBufferForceOn,"kBooleanAttributeCoreBufferForceOn"},
    {kBooleanAttributeCoreConfig,"kBooleanAttributeCoreConfig"},
    {kUint32AttributeCrossPointConnectionNumber,"kUint32AttributeCrossPointConnectionNumber"},
    {kUint32AttributeSetConnection,"kUint32AttributeSetConnection"},
    {kBooleanAttributeOutputEnable,"kBooleanAttributeOutputEnable"},
    {kUint32AttributeSerialInterfaceEnable ,"kUint32AttributeSerialInterfaceEnable"},
    {kUint32AttributePageRegister,"kUint32AttributePageRegister"},
    {kBooleanAttributeOutputEnable,"kBooleanAttributeOutputEnable"},
    {kUint32AttributeSetConnection,"kUint32AttributeSetConnection"},
    {kUint32AttributeInputISEShortTimeConstant,"kUint32AttributeInputISEShortTimeConstant"},
    {kUint32AttributeInputISEMediumTimeConstant,"kUint32AttributeInputISEMediumTimeConstant"},
    {kUint32AttributeInputISELongTimeConstant,"kUint32AttributeInputISELongTimeConstant"},
    {kBooleanAttributeInputStateTerminate,"kBooleanAttributeInputStateTerminate"},
    {kBooleanAttributeInputStateOff,"kBooleanAttributeInputStateOff"},
    {kBooleanAttributeInputStateInvert,"kBooleanAttributeInputStateInvert"},
    {kUint32AttributeInputLOSThreshold,"kUint32AttributeInputLOSThreshold"},
    {kUint32AttributeOutputPreLongDecay,"kUint32AttributeOutputPreLongDecay"},
    {kUint32AttributeOutputPreLongLevel,"kUint32AttributeOutputPreLongLevel"},
    {kUint32AttributeOutputPreShortDecay,"kUint32AttributeOutputPreShortDecay"},
    {kUint32AttributeOutputPreShortLevel,"kUint32AttributeOutputPreShortLevel"},
    {kBooleanOutputLevelTerminate,"kBooleanOutputLevelTerminate"},
    {kUint32OutputLevelPower,"kUint32OutputLevelPower"},
    {kBooleanOutputStateLOSForwarding,"kBooleanOutputStateLOSForwarding"},
    {kUint32OutputStateOperationMode,"kUint32OutputStateOperationMode"},
    {kUint32AttributeStatusPin0LOS,"kUint32AttributeStatusPin0LOS"},
    {kUint32AttributeStatusPin0LOSLatched,"kUint32AttributeStatusPin0LOSLatched"},
    {kUint32AttributeStatusPin1LOS,"kUint32AttributeStatusPin1LOS"},
    {kUint32AttributeStatusPin1LOSLatched,"kUint32AttributeStatusPin1LOSLatched"},
    {kBooleanAttributeChannelStatusLOS,"kBooleanAttributeChannelStatusLOS"},
    {kBooleanAttributeChannelStatusLOSLatched,"kBooleanAttributeChannelStatusLOSLatched"},
    {kBooleanAttributePin0Status,"kBooleanAttributePin0Status"},
    {kBooleanAttributePin1Status,"kBooleanAttributePin1Status"},
    {kBooleanAttributeInputLOS,"kBooleanAttributeInputLOS"},
    {kBooleanAttributeLA0DataPathReset, "kBooleanAttributeLA0DataPathReset"},
    {kBooleanAttributeLA1DataPathReset, "kBooleanAttributeLA1DataPathReset"},
    {kBooleanAttributeLA0AccessEnable, "kBooleanAttributeLA0AccessEnable"},
    {kBooleanAttributeLA1AccessEnable,"kBooleanAttributeLA1AccessEnable"},
    {kUint32AttributeDataBase,"kUint32AttributeDataBase"},
    {kUint32AttributeSRamMissValue,"kUint32AttributeSRamMissValue"},
    {kUint32Attributegmr,"kUint32Attributegmr"},
    {kUint32AttributeClassifierEnable,"kUint32AttributeClassifierEnable"},
    {kUint32AttributeDisableSteering,"kUint32AttributeDisableSteering"},
    {kBooleanAttributeLaneZeroPrbsError,"kBooleanAttributeLaneZeroPrbsError"},
    {kBooleanAttributeLaneOnePrbsError,"kBooleanAttributeLaneOnePrbsError"},
    {kBooleanAttributeLaneTwoPrbsError,"kBooleanAttributeLaneTwoPrbsError"},
    {kBooleanAttributeLaneThreePrbsError,"kBooleanAttributeLaneThreePrbsError"},
    {kBooleanAttributePllDetectZero,"kBooleanAttributePllDetectZero"},
    {kBooleanAttributePllDetectOne,"kBooleanAttributePllDetectOne"},
    {kBooleanAttributePLLDetect, "kBooleanAttributePLLDetect"},
    {kBooleanAttributeGTPResetDoneZero,"kBooleanAttributeGTPResetDoneZero"},
    {kBooleanAttributeGTPResetDoneOne,"kBooleanAttributeGTPResetDoneOne"},
    {kBooleanAttributeGTPResetDoneTwo,"kBooleanAttributeGTPResetDoneTwo"},
    {kBooleanAttributeGTPResetDoneThree,"kBooleanAttributeGTPResetDoneThree"},
    {kBooleanAttributeGTPReset, "kBooleanAttributeGTPReset"},
    {kBooleanAttributeGTPRxReset, "kBooleanAttributeGTPRxReset"},
    {kBooleanAttributeGTPTxReset, "kBooleanAttributeGTPTxReset"},
    {kBooleanAttributeGTPResetDone, "kBooleanAttributeGTPResetDone"},
    {kBooleanAttributeGTPPMAReset, "kBooleanAttributeGTPPMAReset"},
    {kUint32AttributeRxBufferStatZero,"kUint32AttributeRxBufferStatZero"},
    {kUint32AttributeRxBufferStatOne,"kUint32AttributeRxBufferStatOne"},
    {kUint32AttributeRxBufferStatTwo,"kUint32AttributeRxBufferStatTwo"},
    {kUint32AttributeRxBufferStatThree,"kUint32AttributeRxBufferStatThree"},
    {kBooleanAttributeRxByteAlignZero,"kBooleanAttributeRxByteAlignZero"},
    {kBooleanAttributeRxByteAlignOne,"kBooleanAttributeRxByteAlignOne"},
    {kBooleanAttributeRxByteAlignTwo,"kBooleanAttributeRxByteAlignTwo"},
    {kBooleanAttributeRxByteAlignThree,"kBooleanAttributeRxByteAlignThree"},
    {kBooleanAttributeRxChannelAlignZero,"kBooleanAttributeRxChannelAlignZero"},
    {kBooleanAttributeRxChannelAlignOne,"kBooleanAttributeRxChannelAlignOne"},
    {kBooleanAttributeRxChannelAlignTwo,"kBooleanAttributeRxChannelAlignTwo"},
    {kBooleanAttributeRxChannelAlignThree,"kBooleanAttributeRxChannelAlignThree"},
    {kBooleanAttributeRxByteAlign,"kBooleanAttributeRxByteAlign"},
    {kBooleanAttributeRxChannelAlign,"kBooleanAttributeRxChannelAlign"},
    {kUint32AttributeGtpReset,"kUint32AttributeGtpReset"},
    {kBooleanAttributeCdrZeroReset,"kBooleanAttributeCdrZeroReset"},
    {kBooleanAttributeCdrOneReset,"kBooleanAttributeCdrOneReset"},
    {kBooleanAttributeCdrTwoReset,"kBooleanAttributeCdrTwoReset"},
    {kBooleanAttributeCdrThreeReset,"kBooleanAttributeCdrThreeReset"},
    {kBooleanAttributeLaneZeroRxBufferReset,"kBooleanAttributeLaneZeroRxBufferReset"},
    {kBooleanAttributeLaneOneRxBufferReset,"kBooleanAttributeLaneOneRxBufferReset"},
    {kBooleanAttributeLaneTwoRxBufferReset,"kBooleanAttributeLaneTwoRxBufferReset"},
    {kBooleanAttributeLaneThreeRxBufferReset,"kBooleanAttributeLaneThreeRxBufferReset"},
    {kUint32AttributeTxPacketType,"kUint32AttributeTxPacketType"},
    {kBooleanAttributeTxICRCError,"kBooleanAttributeTxICRCError"},
    {kBooleanAttributeTxVCRCError,"kBooleanAttributeTxVCRCError"},
    {kBooleanAttributeRxEqLaneZero,"kBooleanAttributeRxEqLaneZero"},
    {kBooleanAttributeRxEqLaneOne,"kBooleanAttributeRxEqLaneOne"},
    {kBooleanAttributeRxEqLaneTwo,"kBooleanAttributeRxEqLaneTwo"},
    {kBooleanAttributeRxEqLaneThree,"kBooleanAttributeRxEqLaneThree"},
    {kUint32AttributeRxEqMixZero,"kUint32AttributeRxEqMixZero"},
    {kUint32AttributeRxEqMixOne,"kUint32AttributeRxEqMixOne"},
    {kUint32AttributeRxEqMixTwo,"kUint32AttributeRxEqMixTwo"},
    {kUint32AttributeRxEqMixThree,"kUint32AttributeRxEqMixThree"},
    {kBooleanAttributePktErrClear,"kBooleanAttributePktErrClear"},
    {kUint32AttributeRxPacketType,"kUint32AttributeRxPacketType"},
    {kUint32AttributeRxPacketLength,"kUint32AttributeRxPacketLength"},
    {kBooleanAttributeRxICRCValid,"kBooleanAttributeRxICRCValid"},
    {kBooleanAttributeRxVCRCValid,"kBooleanAttributeRxVCRCValid"},
    {kUint32AttributeRxPktErrCount,"kUint32AttributeRxPktErrCount"},
    {kBooleanAttributeRxLinkUp,"kBooleanAttributeRxLinkUp"},
    {kBooleanAttributeRxFrameError,"kBooleanAttributeRxFrameError"},
    {kUint32AttributeRxByteCount,"kUint32AttributeRxByteCount"},
    {kUint32AttributeRxPacketCount,"kUint32AttributeRxPacketCount"},
    {kFloatAttributeVoltageErrorLow,"kFloatAttributeVoltageErrorLow"},
    {kFloatAttributeVoltageWarningLow,"kFloatAttributeVoltageWarningLow"},
    {kFloatAttributeVoltageWarningHigh,"kFloatAttributeVoltageWarningHigh"},
    {kFloatAttributeVoltageErrorHigh,"kFloatAttributeVoltageErrorHigh"},
    {kUint32AttributeLineREIError, "kUint32AttributeLineREIError"},
    {kUint32AttributeCounterSelect, "kUint32AttributeCounterSelect"},
    {kUint32AttributeCounter1, "kUint32AttributeCounter1"},
    {kUint32AttributeCounter2, "kUint32AttributeCounter2"},
    {kUint32AttributeBoardRevision, "kUint32AttributeBoardRevision"},
    {kUint32AttributePCIDeviceID, "kUint32AttributePCIDeviceID"},
    {kUint32AttributeRawData, "kUint32AttributeRawData"},
    {kUint32AttributeChannelDetectAddr, "kUint32AttributeChannelDetectAddr"},
    {kUint32AttributeChannelDetectData, "kUint32AttributeChannelDetectData"},
    {kUint32AttributeChannelConfigMemAddr, "kUint32AttributeChannelConfigMemAddr"},
    {kUint32AttributeChannelConfigMemDataWrite, "kUint32AttributeChannelConfigMemDataWrite"},
    {kUint32AttributeChannelConfigMemDataRead, "kUint32AttributeChannelConfigMemDataRead"},
    {kUint32AttributeVCIDSelect, "kUint32AttributeVCIDSelect"},
    {kUint32AttributeSPESnaplength, "kUint32AttributeSPESnaplength"},
    {kBooleanAttributeRefreshConfigMem, "kBooleanAttributeRefreshConfigMem"},
    {kBooleanAttributeConnectionSelect, "kBooleanAttributeConnectionSelect"},
    {kUint32AttributeMatchValue, "kUint32AttributeMatchValue"},
    {kUint32AttributeMatchMask, "kUint32AttributeMatchMask"},
    {kBooleanAttributePOHCaptureMode, "kBooleanAttributePOHCaptureMode"},
    {kBooleanAttributeChannelizedMode, "kBooleanAttributeChannelizedMode"},
    {kUint32AttributeLOHMaskAddr, "kUint32AttributeLOHMaskAddr"},
    {kBooleanAttributeLOHMaskWriteEnable, "kBooleanAttributeLOHMaskWriteEnable"},
    {kUint32AttributeLOHMaskDataWrite, "kUint32AttributeLOHMaskDataWrite"},
    {kUint32AttributeLOHMaskDataRead, "kUint32AttributeLOHMaskDataRead"},
    {kBooleanAttributeVCIDForce, "kBooleanAttributeVCIDForce"},
    {kBooleanAttributeATMDemapperAvailable, "kBooleanAttributeATMDemapperAvailable"},
    {kBooleanAttributePayloadScrambleATM, "kBooleanAttributePayloadScrambleATM"},
    {kBooleanAttributePayloadScramblePoS, "kBooleanAttributePayloadScramblePoS"},
    {kUint32AttributeIRIGBAutoMode, "kUint32AttributeIRIGBAutoMode"},
    {kUint64AttributeIRIGBtimestamp, " kUint64AttributeIRIGBtimestamp"},
    {kUint32AttributeIRIGBInputSource, " kUint32AttributeIRIGBInputSource"},
    {kUint32AttributePCIBusType, "kUint32AttributePCIBusType"},
    {kBooleanAttributeDisableCopperLink, "kBooleanAttributeDisableCopperLink" },
//    kFirstAttributeCode = kBooleanAttributeActive,    kFirstAttributeCode = kBooleanAttributeActive,
//    kLastAttributeCode = kUint32AttributeIDELAY       kLastAttributeCode = kUint32AttributeIDELAY
};



const char*
framing_mode_to_string(framing_mode_t framing_mode)
{
    int count = sizeof(framing_mode_string)/sizeof(framing_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (framing_mode_string[i].val == framing_mode)
        {
            return framing_mode_string[i].string;
        }
    }
    return NULL;
}

framing_mode_t
string_to_framing_mode(const char* string)
{
    int count = sizeof(framing_mode_string)/sizeof(framing_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(framing_mode_string[i].string, string) == 0)
        {
            return framing_mode_string[i].val;
        }
    }
    return kFramingModeInvalid;
}
const char*
ethernet_mode_to_string(ethernet_mode_t ethernet_mode)
{
    int count = sizeof(ethernet_mode_string)/sizeof(ethernet_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (ethernet_mode_string[i].val == ethernet_mode)
        {
            return ethernet_mode_string[i].string;
        }
    }
    return NULL;
}

ethernet_mode_t
string_to_ethernet_mode(const char* string)
{
    int count = sizeof(ethernet_mode_string)/sizeof(ethernet_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(ethernet_mode_string[i].string, string) == 0)
        {
            return ethernet_mode_string[i].val;
        }
    }
    return kEthernetModeInvalid;
}

const char*
zero_code_to_string(zero_code_suppress_t zero_code)
{
    int count = sizeof(zero_code_string)/sizeof(zero_code_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (zero_code_string[i].val == zero_code)
        {
            return zero_code_string[i].string;
        }
    }
    return NULL;
}

zero_code_suppress_t
string_to_zero_code(const char* string)
{
    int count = sizeof(zero_code_string)/sizeof(zero_code_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(zero_code_string[i].string, string) == 0)
        {
            return zero_code_string[i].val;
        }
    }
    return kZeroCodeInvalid;
}
	
const char*
payload_mapping_to_string(payload_mapping_t payload_mapping)
{
    int count = sizeof(payload_mapping_string)/sizeof(payload_mapping_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (payload_mapping_string[i].payload_mapping == payload_mapping)
        {
            return payload_mapping_string[i].string;
        }
    }
    return NULL;
}

payload_mapping_t
string_to_payload_mapping(const char* string)
{
    int count = sizeof(payload_mapping_string)/sizeof(payload_mapping_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(payload_mapping_string[i].string, string) == 0)
        {
            return payload_mapping_string[i].payload_mapping;
        }
    }
    return kPayloadMappingDisabled;
}

dag_card_t
string_to_card_type(const char* string)
{
    int count = sizeof(card_string)/sizeof(card_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(card_string[i].string, string) == 0)
        {
            return card_string[i].card_type;
        }
    }
    return kDagUnknown;
}

const char*
card_type_to_string(dag_card_t card_type)
{
    int count = sizeof(card_string)/sizeof(card_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (card_string[i].card_type == card_type)
        {
            return card_string[i].string;
        }
    }
    return "unknown";
}

const char*
line_rate_to_string(line_rate_t line_rate)
{
    int count = sizeof(line_rate_string)/sizeof(line_rate_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (line_rate_string[i].line_rate == line_rate)
        {
            return line_rate_string[i].string;
        }
    }
    return NULL;
}

line_rate_t
string_to_line_rate(const char* string)
{
    int count = sizeof(line_rate_string)/sizeof(line_rate_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(line_rate_string[i].string, string) == 0)
        {
            return line_rate_string[i].line_rate;
        }
    }
    return kLineRateInvalid;
}

const char*
counter_select_to_string(counter_select_t counter_select)
{
    int count = sizeof(counter_select_string)/sizeof(counter_select_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (counter_select_string[i].counter_select == counter_select)
        {
            return counter_select_string[i].string;
        }
    }
    return NULL;
}

counter_select_t
string_to_counter_select(const char* string)
{
    int count = sizeof(counter_select_string)/sizeof(counter_select_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(counter_select_string[i].string, string) == 0)
        {
            return counter_select_string[i].counter_select;
        }
    }
    return kLineRateInvalid;
}



const char*
line_type_to_string(line_type_t line_type)
{
    int count = sizeof(line_type_string)/sizeof(line_type_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (line_type_string[i].line_type == line_type)
        {
            return line_type_string[i].string;
        }
    }
    return NULL;
}

line_type_t
string_to_line_type(const char* string)
{
    int count = sizeof(line_type_string)/sizeof(line_type_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(line_type_string[i].string, string) == 0)
        {
            return line_type_string[i].line_type;
        }
    }
    return kLineTypeInvalid;
}

const char*
vc_size_to_string(vc_size_t vc_size)
{
    int count = sizeof(vc_size_string)/sizeof(vc_size_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (vc_size_string[i].vc_size == vc_size)
        {
            return vc_size_string[i].string;
        }
    }
    return NULL;
}

vc_size_t
string_to_vc_size(const char* string)
{
    int count = sizeof(vc_size_string)/sizeof(vc_size_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(vc_size_string[i].string, string) == 0)
        {
            return vc_size_string[i].vc_size;
        }
    }
    return kLineTypeInvalid;
}

const char*
termination_to_string(termination_t termination)
{
    int count = sizeof(termination_string)/sizeof(termination_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (termination_string[i].termination == termination)
        {
            return termination_string[i].string;
        }
    }
    return NULL;
}

termination_t
string_to_termination(const char* string)
{
    int count = sizeof(termination_string)/sizeof(termination_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(termination_string[i].string, string) == 0)
        {
            return termination_string[i].termination;
        }
    }
    return kTerminationInvalid;
}

const char*
pci_bus_speed_to_string(pci_bus_speed_t pci_bus_speed)
{
    int count = sizeof(pci_bus_speed_string)/sizeof(pci_bus_speed_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (pci_bus_speed_string[i].pci_bus_speed == pci_bus_speed)
        {
            return pci_bus_speed_string[i].string;
        }
    }
    return NULL;
}

pci_bus_speed_t
string_to_pci_bus_speed(const char* string)
{
    int count = sizeof(pci_bus_speed_string)/sizeof(pci_bus_speed_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(pci_bus_speed_string[i].string, string) == 0)
        {
            return pci_bus_speed_string[i].pci_bus_speed;
        }
    }
    return kPCIBusSpeedUnknown;
}

const char*
pci_bus_type_to_string(pci_bus_type_t pci_bus_type)
{
    int count = sizeof(pci_bus_type_string)/sizeof(pci_bus_type_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (pci_bus_type_string[i].pci_bus_type == pci_bus_type)
        {
            return pci_bus_type_string[i].string;
        }
    }
    return pci_bus_type_string[0].string;
}

const char*
crc_to_string(crc_t crc)
{
    int count = sizeof(crc_string)/sizeof(crc_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (crc_string[i].crc == crc)
        {
            return crc_string[i].string;
        }
    }
    return NULL;
}

crc_t
string_to_crc(const char* string)
{
    int count = sizeof(crc_string)/sizeof(crc_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, crc_string[i].string) == 0)
        {
            return crc_string[i].crc;
        }
    }
    return kCrcInvalid;
}


const char*
tx_command_to_string(tx_command_t tx_command)
{
    int count = sizeof(tx_command_string)/sizeof(tx_command_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (tx_command_string[i].tx_command == tx_command)
        {
            return tx_command_string[i].string;
        }
    }
    return NULL;
}

tx_command_t
string_to_tx_command(const char* string)
{
    int count = sizeof(tx_command_string)/sizeof(tx_command_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, tx_command_string[i].string) == 0)
        {
            return tx_command_string[i].tx_command;
        }
    }
    return kTxCommandInvalid;
}


const char*
network_mode_to_string(network_mode_t network_mode)
{
    int count = sizeof(network_mode_string)/sizeof(network_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (network_mode_string[i].network_mode == network_mode)
        {
            return network_mode_string[i].string;
        }
    }
    return NULL;
}

network_mode_t
string_to_network_mode(const char* string)
{
    int count = sizeof(network_mode_string)/sizeof(network_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, network_mode_string[i].string) == 0)
        {
            return network_mode_string[i].network_mode;
        }
    }
    return kNetworkModeInvalid;
}

const char*
master_slave_to_string(master_slave_t ms)
{
    int count = sizeof(master_slave_string)/sizeof(master_slave_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (master_slave_string[i].ms == ms)
        {
            return master_slave_string[i].string;
        }
    }
    return NULL;
}

master_slave_t
string_to_master_slave(const char* string)
{
    int count = sizeof(master_slave_string)/sizeof(master_slave_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, master_slave_string[i].string) == 0)
        {
            return master_slave_string[i].ms;
        }
    }
    return kMasterSlaveInvalid;
}

const char*
erfmux_steer_to_string(erfmux_steer_t val)
{
    int count = sizeof(erfmux_steer_string)/sizeof(erfmux_steer_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (erfmux_steer_string[i].val == val)
        {
            return erfmux_steer_string[i].string;
        }
    }
    return NULL;
}

erfmux_steer_t
string_to_erfmux_steer(const char* string)
{
    int count = sizeof(erfmux_steer_string)/sizeof(erfmux_steer_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, erfmux_steer_string[i].string) == 0)
        {
            return erfmux_steer_string[i].val;
        }
    }
    return kSteerInvalid;
}

const char*
sonet_type_to_string(sonet_type_t st)
{
    int count = sizeof(sonet_type_string)/sizeof(sonet_type_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (sonet_type_string[i].st == st)
        {
            return sonet_type_string[i].string;
        }
    }
    return NULL;
}


sonet_type_t
string_to_sonet_type(const char* string)
{
    int count = sizeof(master_slave_string)/sizeof(master_slave_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, sonet_type_string[i].string) == 0)
        {
            return sonet_type_string[i].st;
        }
    }
    return kSonetTypeInvalid;
}

const char*
dag71s_channelized_rev_id_to_string(dag71s_channelized_rev_id_t id)
{
    int count = sizeof(dag71s_channelized_rev_id_string)/sizeof(dag71s_channelized_rev_id_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (dag71s_channelized_rev_id_string[i].id == id)
        {
            return dag71s_channelized_rev_id_string[i].string;
        }
    }
    return NULL;
}


dag71s_channelized_rev_id_t
string_to_dag71s_channelized_rev_id(const char* string)
{
    int count = sizeof(dag71s_channelized_rev_id_string)/sizeof(dag71s_channelized_rev_id_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, dag71s_channelized_rev_id_string[i].string) == 0)
        {
            return dag71s_channelized_rev_id_string[i].id;
        }
    }
    return kSonetTypeInvalid;
}

const char*
terf_strip_to_string(terf_strip_t val)
{
    int count = sizeof(terf_strip_string)/sizeof(terf_strip_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (terf_strip_string[i].val == val)
        {
            return terf_strip_string[i].string;
        }
    }
    return NULL;
}

terf_strip_t
string_to_terf_strip(const char* string)
{
    int count = sizeof(terf_strip_string)/sizeof(terf_strip_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, terf_strip_string[i].string) == 0)
        {
            return terf_strip_string[i].val;
        }
    }
    return kTerfStripInvalid;
}

const char*
time_mode_to_string(terf_time_mode_t val)
{
    int count = sizeof(terf_time_mode_string)/sizeof(terf_time_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (terf_time_mode_string[i].val == val)
        {
            return terf_time_mode_string[i].string;
        }
    }
    return NULL;
}

terf_time_mode_t
string_to_time_mode(const char* string)
{
    int count = sizeof(terf_time_mode_string)/sizeof(terf_time_mode_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, terf_time_mode_string[i].string) == 0)
        {
            return terf_time_mode_string[i].val;
        }
    }
    return kTrTerfTimeModeInvalid;
}


steer_t
string_to_steer(const char* string)
{
    int count = sizeof(steer_string)/sizeof(steer_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(string, steer_string[i].string) == 0)
        {
            return steer_string[i].val;
        }
    }
    return kSteerInvalid;
}
    
const char*
steer_to_string(steer_t val)
{
    int count = sizeof(steer_string)/sizeof(steer_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (steer_string[i].val == val)
        {
            return steer_string[i].string;
        }
    }
    return NULL;
}


const char*
active_firmware_to_string(dag_firmware_t firmware)
{
    int count = sizeof(active_firmware_string)/sizeof(active_firmware_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (active_firmware_string[i].firmware== firmware)
        {
            return active_firmware_string[i].string;
        }
    }
    return NULL;
}

dag_firmware_t
string_to_active_firmware(const char* string)
{
    int count = sizeof(active_firmware_string)/sizeof(active_firmware_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(active_firmware_string[i].string, string) == 0)
        {
            return active_firmware_string[i].firmware;
        }
    }
    return kFirmwareNone;
}
const char*
coprocessor_to_string(coprocessor_t copro)
{
    int count = sizeof(coprocessor_string)/sizeof(coprocessor_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (coprocessor_string[i].coprocessor == copro)
        {
            return coprocessor_string[i].string;
        }
    }
    return NULL;
}

coprocessor_t
string_to_coprocessor(const char* string)
{
    int count = sizeof(coprocessor_string)/sizeof(coprocessor_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(coprocessor_string[i].string, string) == 0)
        {
            return coprocessor_string[i].coprocessor;
        }
    }
    return kCoprocessorNotSupported;
}

/* note this 2 functions are used directly via dag_config.c and exposed to the user 
 not as the other ones via the components or attributes . This is ok 
 */
/* converts component code to string value */
const char*
dag_component_code_to_string(dag_component_code_t comp_code)
{
    int count = sizeof(dag_component_code_string)/sizeof(dag_component_code_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (dag_component_code_string[i].comp_code == comp_code)
        {
            return dag_component_code_string[i].string;
        }
    }
//note: maybe we want the sting represaentation of the code  instead of NULL
    return NULL;
}


/* converts component code string to enum value */
dag_component_code_t
string_to_dag_component_code(const char* string)
{
    int count = sizeof(dag_component_code_string)/sizeof(dag_component_code_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(dag_component_code_string[i].string, string) == 0)
        {
            return dag_component_code_string[i].comp_code;
        }
    }
    return kComponentInvalid;
}

const char*
dag_attribute_code_to_string(dag_attribute_code_t comp_code)
{
    int count = sizeof(dag_attribute_code_string)/sizeof(dag_attribute_code_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (dag_attribute_code_string[i].comp_code == comp_code)
        {
            return dag_attribute_code_string[i].string;
        }
    }
//note: maybe we want the sting represaentation of the code  instead of NULL
    return NULL;
}


/* converts attribute code string to enum value */
dag_attribute_code_t
string_to_dag_attribute_code(const char* string)
{
    int count = sizeof(dag_attribute_code_string)/sizeof(dag_attribute_code_string_map_t);
    int i;

    for (i = 0; i < count; i++)
    {
        if (strcmp(dag_attribute_code_string[i].string, string) == 0)
        {
            return dag_attribute_code_string[i].comp_code;
        }
    }
    return kAttributeInvalid;
}

