/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#include "./include/cards/common_dagx_constants.h"
#include "../../include/dagutil.h"
#include "include/attribute_factory.h"
#include "include/modules/generic_read_write.h"
#include "include/util/enum_string_table.h"
#include "include/components/vsc3312.h"
#include "include/attribute.h"

static AttributePtr make_nic_attribute(GenericReadWritePtr grw,  const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_eql_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_laser_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfppwr_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_rocket_io_power_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_tx_crc_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_link_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_peer_link_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_remote_fault_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_loss_of_frame_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_loss_of_signal_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_tx_frames_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_tx_bytes_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_rx_bytes_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_rx_frames_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfp_detect_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_remote_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_crc_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_bad_symbol_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_config_seq_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfp_tx_fault_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfp_tx_fault_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfp_tx_fault_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_crc_error_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_crc_error_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_remote_error_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_remote_error_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_remote_error_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_mini_mac_lost_sync_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_loss_of_framing_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_loss_of_framing_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_loss_of_framing_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfp_loss_of_signal_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfp_loss_of_signal_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_sfp_loss_of_signal_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_peer_link_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_peer_link_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_peer_link_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_link_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_link_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_link_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_temperature_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_voltage_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_drop_count_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_stream_drop_count_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_snap_len_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_latch_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_link_discard_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_crc_strip_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_loss_of_pointer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_lcd_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_fcl_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_oof_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_generate_ip_counter_erf_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_generate_tcp_flow_counter_erf_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_counter_modules(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_reset_ip_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_reset_tcp_flow_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_ip_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_tcp_flow_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_probability_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_probability_sampler(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_packet_capture_module(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_host_flow_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_enable_host_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_set_capture_hash(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_unset_capture_hash(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_reset_probability_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_reset_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_tcam_init(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_ready_enable_ip_counter_module(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_ready_enable_tcp_flow_module(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_refresh_cache_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_packet_capture_threshold(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_flow_capture_threshold(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_steer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_hash_table_ready(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_ethernet_mac_address_attribute(GenericReadWritePtr grw, const uint32_t*bit_masks, uint32_t len);
static AttributePtr make_promiscuous_attribute(GenericReadWritePtr grw, const uint32_t*bit_masks, uint32_t len);
static AttributePtr make_txfault_attribute(GenericReadWritePtr grw, const uint32_t*bit_masks, uint32_t len);
static AttributePtr make_rxfault_attribute(GenericReadWritePtr grw, const uint32_t*bit_masks, uint32_t len);
static AttributePtr make_highberdetect_attribute(GenericReadWritePtr grw, const uint32_t*bit_masks, uint32_t len);
static AttributePtr make_descramble_attribute(GenericReadWritePtr grw, const uint32_t*bit_masks, uint32_t len);
static AttributePtr make_scramble_attribute(GenericReadWritePtr grw, const uint32_t*bit_masks, uint32_t len);
static AttributePtr make_fault_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_lock_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_ber_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_error_block_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_rx_error_a_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_rx_error_b_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_rx_error_c_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_rx_error_d_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_time_mode_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_scale_range_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_shift_direction_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_active_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_rs422_output_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_over_out_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_over_in_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_host_input_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_host_output_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_rs422_input_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_loop_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_duck_aux_in_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_short_packet_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_long_packet_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_varlen_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_limit_pointer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_record_pointer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_hlb_range_min_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_hlb_range_max_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_crc_select_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
static AttributePtr make_pkt_min_check_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len);
	
AttributePtr
attribute_factory_make_attribute(dag_attribute_code_t code, GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    assert(bit_masks != NULL);
    switch (code)
    {

    case kBooleanAttributePMinCheck:
        result = make_pkt_min_check_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeCrcSelect:
        result = make_crc_select_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeHLBRangeMin:
        result = make_hlb_range_min_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeHLBRangeMax:
        result = make_hlb_range_max_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeLimitPointer:
        result = make_limit_pointer_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeRecordPointer:
        result = make_record_pointer_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeShortPacketErrors:
        result = make_short_packet_errors_attribute(grw, bit_masks, len);
	break;

    case kUint32AttributeLongPacketErrors:
         result = make_long_packet_errors_attribute(grw, bit_masks, len);
	break;

    case kBooleanAttributeNic:
        result = make_nic_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeEquipmentLoopback:
        result = make_eql_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLaser:
        result = make_laser_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeSfpPwr:
        result = make_sfppwr_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeVarlen:
        result = make_varlen_attribute(grw, bit_masks, len);
        break;


    case kBooleanAttributeRocketIOPower:
        result = make_rocket_io_power_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeTxCrc:
        result = make_tx_crc_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLink:
         result = make_link_attribute(grw, bit_masks, len);
         break;

    case kBooleanAttributeLock:
         result = make_lock_attribute(grw, bit_masks, len);
         break;

    case kBooleanAttributePeerLink:
        result = make_peer_link_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRemoteFault:
        result = make_remote_fault_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLossOfFrame:
        result = make_loss_of_frame_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLossOfSignal:
        result = make_loss_of_signal_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeCrcStrip:
        result = make_crc_strip_attribute(grw,bit_masks, len);
        break;

    case kBooleanAttributeLossOfPointer:
        result = make_loss_of_pointer_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLossOfCellDelineation:
        result = make_lcd_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeRefreshCache:
        result = make_refresh_cache_attribute(grw, bit_masks, len);
        break;        

    case kUint64AttributeTxBytes:
        result = make_tx_bytes_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeTxFrames:
        result = make_tx_frames_attribute(grw, bit_masks, len);
        break;
    
    case kUint32AttributeRxFrames:
        result = make_rx_frames_attribute(grw, bit_masks, len);
        break;

    case kUint64AttributeRxBytes:
        result = make_rx_bytes_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeCRCErrors:
        result = make_crc_errors_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeRemoteErrors:
        result = make_remote_errors_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeBadSymbols:
        result = make_bad_symbol_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeConfigSequences:
        result = make_config_seq_attribute(grw, bit_masks, len);
        break;
    
    case kBooleanAttributeSFPDetect:
        result = make_sfp_detect_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeSFPTxFaultCurrent:
        result = make_sfp_tx_fault_current_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeSFPTxFaultEverHi:
        result = make_sfp_tx_fault_ever_hi_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeSFPTxFaultEverLo:
        result = make_sfp_tx_fault_ever_lo_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeCRCErrorEverHi:
        result = make_crc_error_ever_hi_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeCRCErrorEverLo:
        result = make_crc_error_ever_lo_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRemoteErrorEverLo:
        result = make_remote_error_ever_lo_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRemoteErrorEverHi:
        result = make_remote_error_ever_hi_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRemoteErrorCurrent:
        result = make_remote_error_current_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeMiniMacLostSync:
        result = make_mini_mac_lost_sync_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLossOfFramingCurrent:
        result = make_loss_of_framing_current_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLossOfFramingEverLo:
        result = make_loss_of_framing_ever_lo_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLossOfFramingEverHi:
        result = make_loss_of_framing_ever_hi_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeSFPLossOfSignalEverLo:
        result = make_sfp_loss_of_signal_ever_lo_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeSFPLossOfSignalEverHi:
        result = make_sfp_loss_of_signal_ever_hi_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeSFPLossOfSignalCurrent:
        result = make_sfp_loss_of_signal_current_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributePeerLinkEverHi:
        result = make_peer_link_ever_hi_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributePeerLinkEverLo:
        result = make_peer_link_ever_lo_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributePeerLinkCurrent:
        result = make_peer_link_current_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLinkCurrent:
        result = make_link_current_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLinkEverHi:
        result = make_link_ever_hi_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeLinkEverLo:
        result = make_link_ever_lo_attribute(grw, bit_masks, len);
        break;
        
    case kBooleanAttributeLinkDiscard:
        result = make_link_discard_attribute(grw, bit_masks, len);
        break;
    
    case kUint32AttributeTemperature:
        result = make_temperature_attribute(grw, bit_masks, len);
        break;
    case kUint32AttributeVoltage:
        result = make_voltage_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeDropCount:
        result = make_drop_count_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeStreamDropCount:
        result = make_stream_drop_count_attribute(grw, bit_masks, len);
        break;
    case kUint32AttributeSnaplength:
        result = make_snap_len_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeCounterLatch:
        result = make_latch_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeFacilityLoopback:
        result = make_fcl_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeOutOfFrame:
        result = make_oof_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeGenerateIPCounterErf:
        result = make_generate_ip_counter_erf_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeEnableCounterModules:
        result = make_enable_counter_modules(grw, bit_masks, len);
        break;

    case kBooleanAttributeGenerateTCPFlowCounterErf:
        result = make_generate_tcp_flow_counter_erf_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeResetIPCounter:
        result = make_reset_ip_counter_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeResetTCPFlowCounter:
        result = make_reset_tcp_flow_counter_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeEnableTCPFlowCounter:
        result = make_enable_tcp_flow_counter_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeEnableIPAddressCounter:
        result = make_enable_ip_counter_attribute(grw, bit_masks, len);
        break;
    
    case kBooleanAttributeEnableHostHashTable:
        result = make_enable_host_hash_table(grw, bit_masks, len);
        break;

    case kBooleanAttributeEnableHostFlowTable:
        result = make_enable_host_flow_table(grw, bit_masks, len);
        break;

    case kBooleanAttributeEnableProbabilitySampler:
        result = make_enable_probability_sampler(grw, bit_masks, len);
        break;

    case kBooleanAttributeEnablePacketCaptureModules:
        result = make_enable_packet_capture_module(grw, bit_masks, len);
        break;

    case kBooleanAttributeEnableProbabilityHashTable:
        result = make_enable_probability_hash_table(grw, bit_masks, len);
        break;

    case kUint32AttributeSetCaptureHash:
        result = make_set_capture_hash(grw, bit_masks, len);
        break;

    case kUint32AttributeUnsetCaptureHash:
        result = make_unset_capture_hash(grw, bit_masks, len);
        break;

    case kBooleanAttributeResetProbabilityHashTable:
        result = make_reset_probability_hash_table(grw, bit_masks, len);
        break;

    case kBooleanAttributeResetHashTable:
        result = make_reset_hash_table(grw, bit_masks, len);
        break;

    case kBooleanAttributeTCAMInit:
        result = make_tcam_init(grw, bit_masks, len);
        break;

    case kBooleanAttributeReadyEnableIPCounterModule:
        result = make_ready_enable_ip_counter_module(grw, bit_masks, len);
        break;

    case kBooleanAttributeReadyEnableTCPFlowModule:
        result = make_ready_enable_tcp_flow_module(grw, bit_masks, len);
        break;
    case kUint32AttributeSteer:
        result = make_steer_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributePacketCaptureThreshold:
        result = make_packet_capture_threshold(grw, bit_masks, len);
        break;

    case kUint32AttributeFlowCaptureThreshold:
        result = make_flow_capture_threshold(grw, bit_masks, len);
        break;

    case kBooleanAttributeHashTableReady:
        result = make_hash_table_ready(grw, bit_masks, len);
        break;
    case kStringAttributeEthernetMACAddress:
        result = make_ethernet_mac_address_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributePromiscuousMode:
        result = make_promiscuous_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeFault:
        result = make_fault_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeTxFault:
        result = make_txfault_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeRxFault:
        result = make_rxfault_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeHighBitErrorRateDetected:
        result = make_highberdetect_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeDescramble:
        result = make_descramble_attribute(grw, bit_masks, len);
        break;
    case kBooleanAttributeScramble:
        result = make_scramble_attribute(grw, bit_masks, len);
        break;        

    case kUint32AttributeBERCounter:
        result = make_ber_counter_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeErrorBlockCounter:
        result = make_error_block_counter_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRXErrorA:
        result = make_rx_error_a_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRXErrorB:
        result = make_rx_error_b_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRXErrorC:
        result = make_rx_error_c_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeRXErrorD:
        result = make_rx_error_d_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeTimeMode:
        result = make_time_mode_attribute(grw, bit_masks, len);
        break;

    case kUint32AttributeScaleRange:
        result = make_scale_range_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeShiftDirection:
        result = make_shift_direction_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeActive:
	result = make_active_attribute(grw, bit_masks, len);
	break;
	
    case kBooleanAttributeDUCKRS422Input:
        result = make_duck_rs422_input_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeDUCKRS422Output:
        result = make_duck_rs422_output_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeDUCKLoop:
        result = make_duck_loop_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeDUCKHostOutput:
        result = make_duck_host_output_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeDUCKHostInput:
        result = make_duck_host_input_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeDUCKOverOutput:
        result = make_duck_over_out_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeDUCKOverInput:
        result = make_duck_over_in_attribute(grw, bit_masks, len);
        break;

    case kBooleanAttributeDUCKAuxInput:
        result = make_duck_aux_in_attribute(grw, bit_masks, len);
        break;
    default:
		dagutil_warning("Requested attribute code %d not found\n", code);
        NULL_RETURN_WV(NULL, NULL);
        break;
    }
    return result;
}


AttributePtr
make_pkt_min_check_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributePMinCheck);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "pmincheck");
    	attribute_set_description(result, "Predefined Minimum Check.");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

/************************************************************************************/

static void*
attribute_crc_select_get_value(AttributePtr attribute)
{
   crc_t crc = kCrcInvalid;
   uint32_t register_value = 0;
   GenericReadWritePtr crc_grw = NULL;
   uint32_t *masks;
   
   if (1 == valid_attribute(attribute))
   {
	crc_grw = attribute_get_generic_read_write_object(attribute);
	register_value = grw_read(crc_grw);
	masks = attribute_get_masks(attribute);
	register_value &= masks[0];
	if (register_value) 
		crc = kCrc32;
	else 
		crc = kCrcOff;
   }
   attribute_set_value_array(attribute, &crc, sizeof(crc));
   return (void *)attribute_get_value_array(attribute);
}


static void
attribute_crc_select_set_value(AttributePtr attribute, void* value, int length)
{
	uint32_t register_value = 0;
	GenericReadWritePtr crc_grw = NULL;    
	crc_t crc = *(crc_t*)value;
	uint32_t *masks;

	if (1 == valid_attribute(attribute))
	{
		crc_grw = attribute_get_generic_read_write_object(attribute);
		register_value = grw_read(crc_grw);
		masks = attribute_get_masks(attribute);

	switch(crc)
	{
		case kCrcInvalid:
			break;
		case kCrcOff:
			register_value &= ~(masks[0]); /* clear */
			break;
		case kCrc16:
			break;	// Do nothing as crc16 is not valid for Ethernet
		case kCrc32:
			register_value |= masks[0]; /* set */
			break;
        }
        grw_write(crc_grw, register_value);
    }
}

static void
crc_select_value_to_string(AttributePtr attribute)
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
crc_select_value_from_string(AttributePtr attribute, const char* string)
{
  if (1 == valid_attribute(attribute))
  {
      if (string)
      {
          crc_t mode = string_to_crc(string);
          attribute_crc_select_set_value(attribute, (void*)&mode, sizeof(mode));
      }
  }
}

AttributePtr
make_crc_select_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeCrcSelect);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "crc_select");
    attribute_set_description(result, "Select the CRC size.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_setvalue_routine(result, attribute_crc_select_set_value);
    attribute_set_getvalue_routine(result, attribute_crc_select_get_value);
    attribute_set_to_string_routine(result, crc_select_value_to_string);
    attribute_set_from_string_routine(result, crc_select_value_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

/************************************************************************************/


AttributePtr
make_hlb_range_max_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kUint32AttributeHLBRangeMax);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "max");
    	attribute_set_description(result, "Maximum value for range on this stream");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeUint32);
    	attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    	attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    	attribute_set_to_string_routine(result, attribute_uint32_to_string);
    	attribute_set_from_string_routine(result, attribute_uint32_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

AttributePtr
make_hlb_range_min_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kUint32AttributeHLBRangeMin);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "min");
    	attribute_set_description(result, "Minimum value for range on this stream");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeUint32);
    	attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    	attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    	attribute_set_to_string_routine(result, attribute_uint32_to_string);
   	attribute_set_from_string_routine(result, attribute_uint32_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

AttributePtr
make_duck_aux_in_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributeDUCKAuxInput);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "aux_in");
    	attribute_set_description(result, "DAG Auxilliary synchronisation input enabled.");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

AttributePtr
make_varlen_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributeVarlen);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "varlen");
    	attribute_set_description(result, "Enable or disable variable length capture");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}


AttributePtr
make_duck_over_out_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributeDUCKOverOutput);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "over_out");
    	attribute_set_description(result, "DAG synchronisation output source is internal DUCK clock.");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

AttributePtr
make_duck_over_in_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributeDUCKOverInput);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "over_in");
    	attribute_set_description(result, "DAG internal synchronisation input enabled.");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

AttributePtr
make_duck_host_output_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributeDUCKHostOutput);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "host_out");
    	attribute_set_description(result, "DAG synchronisation output source is from Host PC.");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

AttributePtr
make_duck_host_input_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributeDUCKHostInput);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "host_in");
    	attribute_set_description(result, "Host generated synchronisation input enabled. (not used)");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

AttributePtr
make_duck_loop_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    	AttributePtr result = NULL;

    	result = attribute_init(kBooleanAttributeDUCKLoop);
    	attribute_set_generic_read_write_object(result, grw);
    	attribute_set_name(result, "loop");
    	attribute_set_description(result, "DAG synchronisation output source is selected DUCK input signal.");
    	attribute_set_config_status(result, kDagAttrConfig);
    	attribute_set_valuetype(result, kAttributeBoolean);
    	attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    	attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    	attribute_set_to_string_routine(result, attribute_boolean_to_string);
    	attribute_set_from_string_routine(result, attribute_boolean_from_string);
    	attribute_set_masks(result, bit_masks, len);
    	return result;
}

/************************************************************************************/

void*
attribute_time_mode_get_value(AttributePtr attribute)
{
    	uint32_t register_value;
    	uint32_t value = 0;
    	GenericReadWritePtr time_mode_grw = NULL;

    	if (1 == valid_attribute(attribute))
    	{
        	time_mode_grw = attribute_get_generic_read_write_object(attribute);
	        assert(time_mode_grw);
	        register_value = grw_read(time_mode_grw);
	        register_value = ((register_value >> 6) & 0x03);
	        switch(register_value)
        	{
            	case 0x00:
                	value = kTrTerfNoDelay;
                	break;
	            case 0x02:
        	        value = kTrTerfRelative;
                	break;
        	}
        	attribute_set_value_array(attribute, &value, sizeof(value));
            return (void *)attribute_get_value_array(attribute);
     	}
    	return NULL;
}

void
attribute_time_mode_set_value(AttributePtr attribute, void* value, int length)
{
    DagCardPtr card =NULL;
    uint32_t register_value = 0;
    GenericReadWritePtr time_mode_grw = NULL;    
    terf_time_mode_t time_mode = *(terf_time_mode_t*)value;

    card = attribute_get_card(attribute);
    if (1 == valid_attribute(attribute))
    {
        time_mode_grw = attribute_get_generic_read_write_object(attribute);
        register_value = grw_read(time_mode_grw);
        register_value &= ~(0x03 << 6);
        switch(time_mode)
        {
            case kTrTerfTimeModeInvalid:
                /* nothing */
                break;
            case kTrTerfNoDelay:
                register_value |= (0x00 << 6);
                break;
            case kTrTerfRelative:
                register_value |= (0x02 << 6);
                break;
        }
        grw_write(time_mode_grw, register_value);
    }
}


void
time_mode_value_to_string(AttributePtr attribute)
{
    if (1 == valid_attribute(attribute))
    {
        const char* temp = NULL;
        terf_time_mode_t time_mode = *(terf_time_mode_t*)attribute_time_mode_get_value(attribute);
        temp = time_mode_to_string(time_mode);
        attribute_set_to_string(attribute, temp);
    }
}

void
time_mode_value_from_string(AttributePtr attribute, const char* string)
{
    if (1 == valid_attribute(attribute))
    {
        terf_time_mode_t time_mode = string_to_time_mode(string);
        attribute_time_mode_set_value(attribute, (void*)&time_mode, sizeof(steer_t));
    }
}

AttributePtr
make_time_mode_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeTimeMode);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "time_mode");
    attribute_set_description(result, "Determine the timed release mode");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_setvalue_routine(result, attribute_time_mode_set_value);
    attribute_set_getvalue_routine(result, attribute_time_mode_get_value);
    attribute_set_to_string_routine(result, time_mode_value_to_string);
    attribute_set_from_string_routine(result, time_mode_value_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

/************************************************************************************/

AttributePtr
make_record_pointer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;
    result = attribute_init(kUint32AttributeRecordPointer);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "record_pointer");
    attribute_set_description(result, "The point where the next record will be written. Firmware updates this value everytime it writes to the memory hole.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_limit_pointer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;
    result = attribute_init(kUint32AttributeLimitPointer);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "limit_pointer");
    attribute_set_description(result, "The point where the next read will take place from. The dagapi updates this pointer.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_duck_rs422_output_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeDUCKRS422Output);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rs422_out");
    attribute_set_description(result, "DAG synchronisation output source is RS422 input signal.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}


AttributePtr
make_duck_rs422_input_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeDUCKRS422Input);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rs422_in");
    attribute_set_description(result, "External RS422 synchronisation input enabled.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_hash_table_ready(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeHashTableReady);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "hash_table_ready");
    attribute_set_description(result, "High if the hash table is ready to be loaded with another hash.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_packet_capture_threshold(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kUint32AttributePacketCaptureThreshold);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "packet_cap_threshold");
    attribute_set_description(result, "Threshold for the packet capture.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_lock_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeLock);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "lock");
    attribute_set_description(result, "Indicates if the device is locked to the stream.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_flow_capture_threshold(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kUint32AttributeFlowCaptureThreshold);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "flow_cap_threshold");
    attribute_set_description(result, "Threshold for the flow capture.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_ready_enable_tcp_flow_module(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeReadyEnableTCPFlowModule);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "ready_enable_tcp_flow");
    attribute_set_description(result, "High if the tcp flow module is ready to be enabled.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}


AttributePtr
make_ready_enable_ip_counter_module(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeReadyEnableIPCounterModule);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "ready_enable_ip_counter");
    attribute_set_description(result, "High if the ip counter module is ready to be enabled.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_tcam_init(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeTCAMInit);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "tcam_init");
    attribute_set_description(result, "Tell the module if the TCAM has been initialized.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_reset_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeResetHashTable);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "reset_hash_table");
    attribute_set_description(result, "Reset the hash table.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value_with_clear);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_reset_probability_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeResetProbabilityHashTable);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "reset_prob_hash_table");
    attribute_set_description(result, "Reset the probability hash table.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value_with_clear);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_set_capture_hash(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kUint32AttributeSetCaptureHash);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "set_capture_hash");
    attribute_set_description(result, "Capture flows that hash to the given value.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_unset_capture_hash(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kUint32AttributeUnsetCaptureHash);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "unset_capture_hash");
    attribute_set_description(result, "Don't capture flows that hash to the given value.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_enable_probability_sampler(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnableProbabilitySampler);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_probability_sampler");
    attribute_set_description(result, "Enable the probability sampler module.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
    
AttributePtr
make_enable_host_flow_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnableHostFlowTable);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_host_flow_table");
    attribute_set_description(result, "Enable the host flow table module.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_enable_host_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnableHostHashTable);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_host_hash_table");
    attribute_set_description(result, "Enable the host hash table module.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
    
AttributePtr
make_enable_packet_capture_module(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnablePacketCaptureModules);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_packet_capture_module");
    attribute_set_description(result, "Enable the packet capture module.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_enable_probability_hash_table(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnableProbabilityHashTable);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_probability_hash_table");
    attribute_set_description(result, "Enable the probability hash table.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_enable_tcp_flow_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnableTCPFlowCounter);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_tcp_flow_counter");
    attribute_set_description(result, "Enable the TCP Flow counter.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value_with_clear);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_enable_ip_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnableIPAddressCounter);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_ip_counter");
    attribute_set_description(result, "Enable the IP counter.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value_with_clear);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_reset_ip_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeResetIPCounter);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "reset_ip_counter");
    attribute_set_description(result, "Reset the IP counter module");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value_with_clear);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_reset_tcp_flow_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeResetTCPFlowCounter);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "reset_tcp_flow_counter");
    attribute_set_description(result, "Reset the IP counter module");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_reset_tcp_flow);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

/*Attribute pertaining to the IP and TCP Flow counter module used with the SC256*/
AttributePtr
make_enable_counter_modules(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeEnableCounterModules);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "enable_counter_modules");
    attribute_set_description(result, "Use to enable/disable the ip counter modules.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
    
/*Attribute pertaining to the IP and TCP Flow counter module. Set this to write the IP counter erf to the memory hole.*/
AttributePtr
make_generate_ip_counter_erf_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeGenerateIPCounterErf);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "generate_ip_counter_erfs");
    attribute_set_description(result, "Use to generate ERF type 13 to the memory hole.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value_with_clear);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

/*Attribute pertaining to the IP and TCP Flow counter module used with the SC256. Set this attribute to write the TCP flow erf to the memory hole */
AttributePtr
make_generate_tcp_flow_counter_erf_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeGenerateTCPFlowCounterErf);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "generate_tcp_flow_counter_erf");
    attribute_set_description(result, "Use to write ERF type 14 to the memory hole.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value_with_clear);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_ber_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeBERCounter);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "ber_counter");
    attribute_set_description(result, "Counter of the number of BER errors seen. Cleared on read.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_error_block_counter_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeErrorBlockCounter);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "error_block_counter");
    attribute_set_description(result, "Counter of the number of error blocks since the the register was last accessed using an MDIO.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_short_packet_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeShortPacketErrors);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "short_packet_errors");
    attribute_set_description(result, "Count of short packet errors");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_long_packet_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeLongPacketErrors);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "long_packet_errors");
    attribute_set_description(result, "Count of long packet errors");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_snap_len_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeSnaplength);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "snap_length");
    attribute_set_description(result, "Change the snap length");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_setvalue_routine(result, attribute_snaplen_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_drop_count_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;
    
    result = attribute_init(kUint32AttributeDropCount);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "drop_count");
    attribute_set_description(result, "Packets dropped by the card.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_stream_drop_count_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;
    
    result = attribute_init(kUint32AttributeStreamDropCount);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "stream_drop_count");
    attribute_set_description(result, "Packets dropped by this stream.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_voltage_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeVoltage);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "voltage");
    attribute_set_description(result, "Read the voltage (in Volts).");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_temperature_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeTemperature);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "temperature");
    attribute_set_description(result, "Read the temperature (in degrees C).");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_laser_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLaser);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "laser");
    attribute_set_description(result, "Enable or disable the laser");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_eql_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeEquipmentLoopback);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "eql");
    attribute_set_description(result, "Enable or disable equipment loopback");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_sfppwr_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSfpPwr);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "sfppwr");
    attribute_set_description(result, "Enable or disable power to the sfp module");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    /*
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    */
    /* Use a specialized sfppwr set value function because we want to test if the sfp module can support the bit rate */
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_rocket_io_power_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRocketIOPower);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rocket_io_power");
    attribute_set_description(result, "Enable or disable the power to the rocket io");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    /* attribute_set_setvalue_routine(result, attribute_boolean_set_value);*/
    attribute_set_setvalue_routine(result, attribute_rocket_io_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_tx_crc_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeTxCrc);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "tx_crc");
    attribute_set_description(result, "Enable or disable CRC appending onto transmitted frames");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_nic_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeNic);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "auto_neg");
    attribute_set_description(result, "Enable or disable ethernet auto-negotiation");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_peer_link_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributePeerLink);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "peer_link");
    attribute_set_description(result, "Does the peer think a link is up?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_loss_of_signal_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLossOfSignal);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "los");
    attribute_set_description(result, "Loss of Signal");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_loss_of_frame_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLossOfFrame);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "lof");
    attribute_set_description(result, "Loss of Frame");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}


AttributePtr
make_remote_fault_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRemoteFault);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "remote_fault");
    attribute_set_description(result, "Is there a remote fault?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_link_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLink);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "link");
    attribute_set_description(result, "Is there a link?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_tx_frames_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeTxFrames);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "tx_frames");
    attribute_set_description(result, "Count of transmitted frames.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_tx_bytes_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint64AttributeTxBytes);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "tx_bytes");
    attribute_set_description(result, "Count of transmitted bytes.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint64);
    attribute_set_getvalue_routine(result, attribute_uint64_get_value);
    attribute_set_to_string_routine(result, attribute_uint64_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}


AttributePtr
make_rx_frames_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint32AttributeRxFrames);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rx_frames");
    attribute_set_description(result, "Count of received frames.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_rx_bytes_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint64AttributeRxBytes);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rx_bytes");
    attribute_set_description(result, "Count of received bytes.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint64);
    attribute_set_getvalue_routine(result, attribute_uint64_get_value);
    attribute_set_to_string_routine(result, attribute_uint64_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_crc_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint32AttributeCRCErrors);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "fcs_errors");
    attribute_set_description(result, "A count of the frames received that do not pass the FCS check");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_remote_errors_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint32AttributeRemoteErrors);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "remote_errors");
    attribute_set_description(result, "Count of remote errors.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_bad_symbol_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint32AttributeBadSymbols);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "bad_symbols");
    attribute_set_description(result, "Count of bad symbols.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_config_seq_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint32AttributeConfigSequences);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "config_seq");
    attribute_set_description(result, "Count of configuration sequences.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_sfp_detect_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSFPDetect);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "sfp_detect");
    attribute_set_description(result, "Is the SFP there?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}


static AttributePtr
make_sfp_tx_fault_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSFPTxFaultCurrent);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "sfp_tx_fault_current");
    attribute_set_description(result, "Currently a fault in the SFP");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_sfp_tx_fault_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSFPTxFaultEverHi);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "sfp_tx_fault_ever_hi");
    attribute_set_description(result, "Have we ever seen a tx fault?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_sfp_tx_fault_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSFPTxFaultEverLo);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "sfp_tx_fault_ever_lo");
    attribute_set_description(result, "Have we never seen a transmit fault?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_crc_error_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeCRCErrorEverHi);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "crc_error_ever_hi");
    attribute_set_description(result, "Have we ever seen a crc error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_crc_error_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeCRCErrorEverLo);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "crc_error_ever_lo");
    attribute_set_description(result, "Have we never seen a crc error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_remote_error_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRemoteErrorEverLo);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "remote_error_ever_lo");
    attribute_set_description(result, "Have we never seen a remote error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_remote_error_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRemoteErrorEverHi);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "remote_error_ever_hi");
    attribute_set_description(result, "Have we ever seen a remote error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_remote_error_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRemoteErrorCurrent);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "remote_error_current");
    attribute_set_description(result, "Are we currently seeing a remote error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_mini_mac_lost_sync_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeMiniMacLostSync);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "mini_mac_lost_sync");
    attribute_set_description(result, "Has the mini mac lost sync?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_loss_of_framing_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLossOfFramingCurrent);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "loss_of_framing_current");
    attribute_set_description(result, "Is there currently a loss of frame?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
    
static AttributePtr
make_loss_of_framing_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLossOfFramingEverLo);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "loss_of_framing_ever_lo");
    attribute_set_description(result, "Have we never had a loss of frame?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_loss_of_framing_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLossOfFramingEverHi);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "loss_of_framing_ever_hi");
    attribute_set_description(result, "Have we ever had a loss of frame?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_sfp_loss_of_signal_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSFPLossOfSignalEverLo);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "loss_of_signal_ever_lo");
    attribute_set_description(result, "Have we never had a loss of signal?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_sfp_loss_of_signal_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSFPLossOfSignalEverHi);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "loss_of_signal_ever_hi");
    attribute_set_description(result, "Have we ever had a loss of signal?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_sfp_loss_of_signal_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeSFPLossOfSignalCurrent);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "loss_of_signal_current");
    attribute_set_description(result, "Is there currently a loss of signal?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_peer_link_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributePeerLinkEverLo);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "peer_link_ever_lo");
    attribute_set_description(result, "Have we never had a peer link error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_peer_link_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributePeerLinkEverHi);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "peer_link_ever_hi");
    attribute_set_description(result, "Have we ever had a peer link error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_peer_link_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributePeerLinkCurrent);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "peer_link_current");
    attribute_set_description(result, "Is there currently a peer link error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_link_ever_lo_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLinkEverLo);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "link_ever_lo");
    attribute_set_description(result, "Has there never been a link error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_link_ever_hi_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLinkEverHi);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "link_ever_hi");
    attribute_set_description(result, "Has there ever been a link error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static AttributePtr
make_link_current_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeLinkCurrent);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "link_current");
    attribute_set_description(result, "Is there currently a link error?");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_getvalue_routine(result, attribute_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_latch_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
   AttributePtr result = NULL;

   result = attribute_init(kBooleanAttributeCounterLatch);
   attribute_set_generic_read_write_object(result, grw);
   attribute_set_name(result, "rx_and_tx_latch_clear");
   attribute_set_description(result, "'Latch and Clear' the receive/transmit statistics counters");
   attribute_set_config_status(result, kDagAttrConfig);
   attribute_set_valuetype(result, kAttributeBoolean);
   attribute_set_setvalue_routine(result, attribute_boolean_set_value);
   attribute_set_getvalue_routine(result, attribute_boolean_get_value);
   attribute_set_to_string_routine(result, attribute_boolean_to_string);
   attribute_set_masks(result, bit_masks, len);
   return result;
}
AttributePtr
make_link_discard_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeLinkDiscard);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "link_discard");
    attribute_set_description(result, "Enable/Disable the crc bytes discarding.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_crc_strip_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeCrcStrip);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "crc_strip");
    attribute_set_description(result, "Strip the crc");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_loss_of_pointer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeLossOfPointer);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "lop");
    attribute_set_description(result, "Loss Of Pointer");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_oof_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeOutOfFrame);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "oof");
    attribute_set_description(result, "Out Of Frame");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_lcd_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeLossOfCellDelineation);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "lcd");
    attribute_set_description(result, "Loss Of Cell Delineation");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_fcl_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeFacilityLoopback);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "fcl");
    attribute_set_description(result, "Enable or disable facilty loopback");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_refresh_cache_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeRefreshCache);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "refresh_cache");
    attribute_set_description(result, "'Latch and Clear' the extended statistics");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_refresh_cache_set_value);
    attribute_set_getvalue_routine(result, attribute_refresh_cache_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_steer_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    result = attribute_init(kUint32AttributeSteer);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "steer");
    attribute_set_description(result, "Enable or disable steering of received packets into specific memory holes");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_getvalue_routine(result, attribute_steer_get_value);
    attribute_set_setvalue_routine(result, attribute_steer_set_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

static void
ethernet_mac_address_to_string(AttributePtr attribute)
{
	if(1 == valid_attribute(attribute))
	{
		char *mac_address = NULL;
		mac_address = (char*)attribute_get_value(attribute);
		if(NULL != mac_address)
		{
			attribute_set_to_string(attribute,mac_address);
		}
		return;
	}
}
AttributePtr
make_ethernet_mac_address_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;
    result = attribute_init(kStringAttributeEthernetMACAddress);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "mac_address");
    attribute_set_description(result, "MAC address for ethernet port");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeString);
    attribute_set_getvalue_routine(result, attribute_mac_get_value);
    attribute_set_to_string_routine(result,ethernet_mac_address_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_promiscuous_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributePromiscuousMode);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "promisc");
    attribute_set_description(result, "Ethernet promiscuous mode");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_fault_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeFault);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "fault");
    attribute_set_description(result, "Indicates a fault in the receive or transmit link");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_txfault_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeTxFault);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "tx_fault");
    attribute_set_description(result, "Indicates a fault in the transmit data path.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_rxfault_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeRxFault);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rx_fault");
    attribute_set_description(result, "Indicates a fault in the receive data path.");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_highberdetect_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeHighBitErrorRateDetected);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "high_ber");
    attribute_set_description(result, "High 'Bit Error Rate' detected");
    attribute_set_config_status(result, kDagAttrStatus);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_descramble_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeDescramble);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "descramble");
    attribute_set_description(result, "Enable or disable receive descrambling");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}
AttributePtr
make_scramble_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeScramble);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "scramble");
    attribute_set_description(result, "Enable or disable scrambling");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_rx_error_a_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRXErrorA);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rx_error_a");
    attribute_set_description(result, "Mask for the RX-Error bit in ERF header");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_rx_error_b_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRXErrorB);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rx_error_b");
    attribute_set_description(result, "Mask for the RX-Error bit in ERF header for the 2nd ouput port if available");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_rx_error_c_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRXErrorC);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rx_error_c");
    attribute_set_description(result, "Mask for the RX-Error bit in ERF header for the 3rd ouput port if available");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_rx_error_d_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeRXErrorD);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "rx_error_d");
    attribute_set_description(result, "Mask for the RX-Error bit in ERF header for the 4th ouput port if available");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}


AttributePtr
make_scale_range_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kUint32AttributeScaleRange);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "scale_range");
    attribute_set_description(result, "Number of logical shifts performed between 2 packet's timestamps");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeUint32);
    attribute_set_setvalue_routine(result, attribute_uint32_set_value);
    attribute_set_getvalue_routine(result, attribute_uint32_get_value);
    attribute_set_to_string_routine(result, attribute_uint32_to_string);
    attribute_set_from_string_routine(result, attribute_uint32_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_shift_direction_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;    
    
    result = attribute_init(kBooleanAttributeShiftDirection);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "shift_direction");
    attribute_set_description(result, "Determines the shift direction, left or right, multiplication or division");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}

AttributePtr
make_active_attribute(GenericReadWritePtr grw, const uint32_t* bit_masks, uint32_t len)
{
    AttributePtr result = NULL;

    result = attribute_init(kBooleanAttributeActive);
    attribute_set_generic_read_write_object(result, grw);
    attribute_set_name(result, "active");
    attribute_set_description(result, "Enable or disable this link/port.");
    attribute_set_config_status(result, kDagAttrConfig);
    attribute_set_valuetype(result, kAttributeBoolean);
    attribute_set_setvalue_routine(result, attribute_boolean_set_value);
    attribute_set_getvalue_routine(result, attribute_boolean_get_value);
    attribute_set_to_string_routine(result, attribute_boolean_to_string);
    attribute_set_from_string_routine(result, attribute_boolean_from_string);
    attribute_set_masks(result, bit_masks, len);
    return result;
}



