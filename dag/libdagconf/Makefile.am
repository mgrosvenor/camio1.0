SUBDIRS = include
lib_LTLIBRARIES = libdagconf.la

libdagconf_la_CPPFLAGS = -I${top_builddir}/include -I${top_srcdir}/include -I${srcdir}/include -I${top_srcdir}/include/dagdsm -I${top_srcdir}/include/adt
libdagconf_la_CFLAGS = @DAG_CFLAGS@ @BYTESWAP@ @LARGEFILE@
libdagconf_la_LDFLAGS = -version-number @DAG_SO_VERSION@

libdagconf_la_LIBADD = ${top_builddir}/lib/libdag.la \
		       ${top_builddir}/lib/dagcam/libinfinicam.la 

libdagconf_la_SOURCES = \
        dag_component.c \
        dag_config.c \
        dag_config_util.c \
        generic_attribute.c \
        generic_card.c \
        generic_component.c \
        attribute_factory.c \
        create_attribute.c \
        cards/card_initialization.c \
        cards/dag37ge_impl.c \
        cards/dag37t_impl.c \
        cards/dag38s_impl.c \
        cards/dag37d_impl.c \
        components/dag38s_port_component.c \
        cards/dag43ge_impl.c \
        cards/dag43s_impl.c \
        components/dag43s_port_component.c \
        components/vsc8476_port_component.c \
        components/vsc8486_port_component.c \
        components/vsc8479_port_component.c \
        components/xgmii_statistics_component.c \
        components/xgmii_component.c \
        cards/dag62se_impl.c \
        components/dag62se_port_component.c \
        cards/dag70ge_impl.c \
        cards/dagx_impl.c \
        components/dag71s_channelized_demapper_component.c \
        components/dag71s_channelized_mapper_component.c \
        components/dag71s_concatenated_demapper_component.c \
        components/dag71s_concatenated_mapper_component.c \
        components/dag71s_e1t1_component.c \
        components/dag71s_erfmux_component.c \
	components/dag37t_mux_component.c \
        cards/dag71s_impl.c \
        components/dag71s_phy_component.c \
        components/dag71s_port_component.c \
        components/dag37d_port_component.c \
        components/gpp_component.c \
        components/dpa_component.c \
        components/led_controller_component.c \
        components/lm63.c \
        components/lm93.c \
        components/mini_mac.c \
        components/mini_mac_statistics_component.c \
        components/optics_component.c \
        components/xfp_component.c \
        components/duck_component.c \
        components/pbm_component.c \
        components/pc_component.c \
        components/amcc3485_component.c \
        components/oc48_deframer_component.c \
        components/sonet_pp_component.c \
        components/sac_component.c \
        components/sc256.c \
        components/sr_gpp.c \
        components/steering_component.c \
        components/steering_ipf_component.c \
        components/drop_component.c \
        components/stream_component.c \
        components/terf_component.c \
        components/tr_terf_component.c \
        components/hlb_component.c \
        components/sonic_component.c \
        components/counters_interface_component.c \
        components/counter_component.c \
        components/general_component.c \
        components/dag37t_connection_component.c \
        components/dag71s_connection_component.c \
        components/xge_pcs_component.c \
        components/card_info_component.c \
        components/lm_sensor_component.c \
	util/enum_string_table.c \
        components/stream_feature_component.c \
        components/ipf_component.c \
        modules/generic_read_write.c \
        modules/latch.c \
        util/logger.c \
        modules/raw_smbus_module.c \
        util/set.c \
        util/utility.c \
        channel_config/dag37t_add_chan.c \
        channel_config/dag37t_api.c \
        channel_config/dag37t_chan_cfg.c \
        channel_config/dag37t_delete_chan.c \
        channel_config/dag37t_map_functions.c \
        channel_config/dagmapper.c \
	components/cat_component.c \
	components/hlb_new_component.c \
	components/vsc3312.c \
	components/framer_component.c \
	components/infiniband_classifier.c \
	components/memtx_component.c \
	components/gtp_phy_component.c \
	components/dsm_component.c \
	components/sonet_channel_mgmt_component.c \
	components/sonet_connection_component.c \
	components/bfs_classifier_component.c \
	components/irigb_component.c \
	components/pattern_match_component.c




