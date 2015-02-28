/*
 * Copyright (c) 2005-2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

/* Generic DAG card object (the x in dagx_impl is a variable)*/

/* Internal project headers. */
#include "../../../include/dagutil.h"
#include "../../../include/dagapi.h"
#include "../include/attribute.h"
#include "../include/card.h"
#include "../include/cards/card_initialization.h"
#include "../include/component.h"
#include "../include/util/utility.h"
#include "../include/cards/common_dagx_constants.h"
#include "../include/cards/dag4_constants.h"
#include "../include/components/pbm_component.h"
#include "../include/components/stream_component.h"
#include "../include/components/terf_component.h"
#include "../include/components/sac_component.h"
#include "../include/components/pc_component.h"
#include "../include/components/drop_component.h"
#include "../include/components/sc256.h"
#include "../include/components/gpp_component.h"
#include "../include/components/optics_component.h"
#include "../include/components/lm93.h"
#include "../include/components/mini_mac.h"
#include "../include/components/sr_gpp.h"
#include "../include/components/vsc8486_port_component.h"
#include "../include/components/vsc8476_port_component.h"
#include "../include/components/vsc8479_port_component.h"
#include "../include/attribute_factory.h"
#include "../include/components/steering_component.h"
#include "../include/components/duck_component.h"
#include "../include/components/amcc3485_component.h"
#include "../include/components/oc48_deframer_component.h"
#include "../include/components/sonet_pp_component.h"
#include "../include/components/xgmii_statistics_component.h"
#include "../include/components/dag71s_concatenated_demapper_component.h"
#include "../include/components/dag71s_concatenated_mapper_component.h"
#include "../include/components/xgmii_component.h"
#include "../include/components/dpa_component.h"
#include "../include/cards/dag71s_impl.h"
#include "../include/components/hlb_component.h"
#include "../include/components/steering_ipf_component.h"
#include "../include/components/sonic_component.h"
#include "../include/components/dag38s_port_component.h"
#include "../include/components/counters_interface_component.h"
#include "../include/components/general_component.h"
#include "../include/components/vsc8479_port_component.h"
#include "../include/components/xfp_component.h"
#include "../include/components/xge_pcs_component.h"
#include "../include/components/card_info_component.h"
#include "../include/components/lm_sensor_component.h"
#include "../include/components/cat_component.h"
#include "../include/components/stream_feature_component.h"
#include "../include/components/ipf_component.h"
#include "../include/components/hlb_new_component.h"
#include "../include/components/vsc3312.h"
#include "../include/components/framer_component.h"
#include "../include/components/infiniband_classifier.h"
#include "../include/components/memtx_component.h"
#include "../include/components/gtp_phy_component.h"
#include "../include/components/dsm_component.h"
#include "../include/components/sonet_channel_mgmt_component.h"
#include "../include/components/bfs_component.h"
#include "../include/components/irigb_component.h"
#include "../include/components/pattern_match_component.h"

/* Virtual methods for the card. */
static void dagx_dispose(DagCardPtr card);
static int dagx_post_initialize(DagCardPtr card);
static void dagx_reset(DagCardPtr card);
static void dagx_default(DagCardPtr card);

typedef struct
{
    dag_reg_module_t mModuleCode;
    uint32_t mVersion;
    ComponentCreatorRoutine mCreator;
} module_code_creator_pair_t;

/* The components will created in the order specified in the array */
module_code_creator_pair_t
module_creator_map[] =
{
    {DAG_REG_GENERAL, 0, general_get_new_component},
    {DAG_REG_GPP, 0, gpp_get_new_gpp},
    {DAG_REG_SRGPP, 0, sr_gpp_get_new_component},
    {DAG_REG_SRGPP, 1, sr_gpp_get_new_component},
    {DAG_REG_PBM, 0, pbm_get_new_pbm},
    {DAG_REG_PBM, 1, pbm_get_new_pbm},
    {DAG_REG_PBM, 2, pbm_get_new_pbm},
    {DAG_REG_PBM, 3, pbm_get_new_pbm},
    {DAG_REG_MINIMAC, 0, mini_mac_get_new_component},
    {DAG_REG_MINIMAC, 1, mini_mac_get_new_component},
    {DAG_REG_MINIMAC, 2, mini_mac_get_new_component},
    {DAG_REG_STEERING, 0, steering_ipf_get_new_steering_ipf},
    {DAG_REG_STEERING, 1, steering_get_new_steering},
    {DAG_REG_TERF64, 0, terf_get_new_terf},
    {DAG_REG_TERF64, 1, terf_get_new_terf},
    {DAG_REG_TERF64, 2, terf_get_new_terf},
    {DAG_REG_TERF64, 3, terf_get_new_terf},
    {DAG_REG_VSC8476, 0, vsc8476_get_new_port},
    {DAG_REG_DROP, 0, drop_get_new_component},
    {DAG_REG_DUCK, 0, duck_get_new_component},
    {DAG_REG_DUCK, 1, duck_get_new_component},
    {DAG_REG_DUCK, 2, duck_get_new_component},
    {DAG_REG_DUCK, 3, duck_get_new_component},
    {DAG_REG_DUCK, 4, duck_get_new_component},
    {DAG_REG_DUCK, 5, duck_get_new_component},
    {DAG_REG_DUCK, 6, duck_get_new_component},
    {DAG_REG_DUCK, 7, duck_get_new_component},
    {DAG_REG_AMCC3485, 0, amcc3485_get_new_component},
    {DAG_REG_XGMII, 0, xgmii_get_new_component},
    {DAG_REG_XGMII, 1, xgmii_get_new_component},
    {DAG_REG_E1T1_HDLC_DEMAP, 4, concatenated_atm_pos_demapper_get_new_component},
    {DAG_REG_E1T1_HDLC_MAP, 4, concatenated_atm_pos_mapper_get_new_component},
    {DAG_REG_SONIC_3, 1, sonic_v1_get_new_component},
    {DAG_REG_SONIC_3, 3, sonic_get_new_component},
    {DAG_REG_SONIC_3, 4, sonic_get_new_component}, 
    {DAG_REG_SONIC_3, 4, sonic_get_new_component},
    {DAG_REG_RAW_SMBBUS,4,vsc3312_get_new_component},    
    {DAG_REG_HLB, 0, hlb_get_new_component},
    {DAG_REG_HLB,1,hat_get_new_component},
    {DAG_REG_OC48_DEFRAMER, 0, oc48_deframer_get_new_component},
    {DAG_REG_OC48_DEFRAMER, 1, oc48_deframer_get_new_component},
    {DAG_REG_DPA, 0, dpa_get_new_component},
    {DAG_REG_DPA, 1, dpa_get_new_component},
    {DAG_REG_COUNT_INTERF, 0, counters_interface_get_new_component},
    {DAG_REG_COUNT_INTERF, 1, counters_interface_get_new_component},
    {DAG_REG_VSC8479, 0, vsc8479_get_new_component},
    {DAG_REG_VSC8479, 1, vsc8479_get_new_component},
    {DAG_REG_SONET_PP, 0, sonet_pp_get_new_component},
    {DAG_REG_XFP, 0, xfp_get_new_component},
    {DAG_REG_XFP, 1, xfp_get_new_component},
    {DAG_REG_XGE_PCS, 0, xge_pcs_get_new_component},
    {DAG_REG_XGE_PCS, 1, xge_pcs_get_new_component},
    {DAG_REG_CAT,0,cat_get_new_component          },
    {DAG_REG_STREAM_FTR, 0, stream_feature_get_new_component},
    {DAG_REG_PPF, 0, ipf_get_new_component},
    {DAG_REG_PPF, 1, ipf_get_new_component},
    {DAG_REG_PPF, 2, ipf_get_new_component},
    {DAG_REG_PPF, 3, ipf_get_new_component},
    {DAG_REG_INFI_FRAMER,0,framer_get_new_component},
    {DAG_REG_INFINICLASSIFIER,2,infiniband_classifier_get_new_component},
    {DAG_REG_MEMTX, 0,memtx_get_new_component},
    {DAG_REG_GTP_PHY, 0,gtp_phy_get_new_component},
    {DAG_REG_HMM, 0, dsm_get_new_component},
    {DAG_REG_SONET_CHANNEL_DETECT, 0, sonet_channel_mgmt_get_new_component},
    {DAG_REG_VSC8486, 0, vsc8486_get_new_port},
    {DAG_REG_PPF, 4, bfs_get_new_component},
    {DAG_REG_IRIGB,0, irigb_get_new_component},
    {DAG_REG_PATTERN_MATCH, 0, pattern_match_get_new_component}

};

static void
dagx_dispose(DagCardPtr card)
{
    if (1 == valid_card(card))
    {
        ComponentPtr root = NULL;
#if 0
        ComponentPtr component = NULL;
        AttributePtr attribute = NULL;
        int component_count = 0;
        int i = 0;
#endif
        root = card_get_root_component(card);
        component_dispose(root);
#if 0
        component_count = component_get_subcomponent_count(root);
        for (i = 0; i < component_count; i++)
        {
            component = component_get_indexed_subcomponent(root, i);
            while (component_has_attributes(component) == 1)
            {
                attribute = component_get_indexed_attribute(component, 0);
                component_dispose_attribute(component, attribute);
            }
            component_dispose(component);
        }
#endif
    }
}


static int
dagx_post_initialize(DagCardPtr card)
{    
    if (1 == valid_card(card))
    {
        int reg_count = 0;
        int module_count = sizeof(module_creator_map)/sizeof(module_code_creator_pair_t);
        dag_reg_t registers[DAG_REG_MAX_ENTRIES];
        int module_counts[DAG_REG_END];
        int i = 0;
        int j = 0;
        ComponentPtr root = card_get_root_component(card);
        ComponentPtr lm_sens_comp = NULL;

        /* Add card info component */
        component_add_subcomponent(root, card_info_get_new_component(card, 0));
		
        /* Add the LM sensors component (note that this is a wrapper for all LM devices) */
        lm_sens_comp = lm_sensor_get_new_component (card);
        if ( lm_sens_comp != NULL )
            component_add_subcomponent(root, lm_sens_comp);

        /* Iterate over the register enumeration table and load components as needed.
         * We do the creation of the components in dagx_post_initialize and not 
         * in dagx_initialize because the register enumeration table is not valid at that stage.
         * */

        memset(module_counts, 0, sizeof(module_counts));
        memset(registers, 0, sizeof(registers));
        reg_count = dag_reg_find((char*)card_get_iom_address(card), DAG_REG_START, registers);
        for (j = 0; j < module_count; j++)
        {
            for (i = 0; i < reg_count; i++)
            {
                if (module_creator_map[j].mModuleCode == registers[i].module &&
                    module_creator_map[j].mVersion == registers[i].version)
                {
                    ComponentPtr comp = NULL;
                    assert(registers[i].module < DAG_REG_END);
                    comp = module_creator_map[j].mCreator(card, module_counts[registers[i].module]);
                    assert(comp);
                    component_add_subcomponent(root, comp);
                    /* Special case addition of memory stream components. */
                    if (registers[i].module == DAG_REG_PBM)
                    {
                        int x = 0;
                        int dagfd = 0;
                        int tx_streams = 0;
                        int rx_streams = 0;
                        int streammax = 0;

                        dagfd = card_get_fd(card);
                        /* Get the number of RX Streams */
                        rx_streams = dag_rx_get_stream_count(dagfd);
                        /* Get the Number of TX Streams */
                        tx_streams = dag_tx_get_stream_count(dagfd);

                        streammax = dagutil_max(rx_streams*2, tx_streams*2); 
                        for (x = 0;x < streammax; x++)
                        {
                            component_add_subcomponent(root, get_new_stream(card, x));
                        }
                    }
                    dagutil_verbose_level(4, "Creating module 0x%x, version %u, index %d\n", registers[i].module, registers[i].version, module_counts[registers[i].module]);
	            /* We count the number of modules of the same type that appear in the register enumeration table
		     * regardless of the version number associated with the module. This means that 
		     * if the module DAG_REG_SMBUS v1 is the 1st SMBus module to appear it will have index 0.
		     * If there is a 2nd SMBus module v0 it will have index 1. This corresponds with the 
		     * way card_get_register_address (and indirectly dag_reg_table_find) works. Neither of those 
		     * functions make a distincton on the index of the module based on the version number of the module.
		     */
                    module_counts[registers[i].module]++;
                }
            }
        }
    }
    return 1;
}

static void
dagx_reset(DagCardPtr card)
{    
    if (1 == valid_card(card))
    {
        int i = 0;
        ComponentPtr root = card_get_root_component(card);
        card_reset_datapath(card);
        for(i = 0; i < component_get_subcomponent_count(root); i++)
        {
            ComponentPtr any_component = NULL;
            any_component = component_get_indexed_subcomponent(root, i);
            component_reset(any_component);
        }
	
    }
}

static void
dagx_default(DagCardPtr card)
{
    if (1 == valid_card(card))
    { 
        int i = 0;
        ComponentPtr root = card_get_root_component(card);
        card_reset_datapath(card);
        for(i = 0; i < component_get_subcomponent_count(root); i++)
        {
            ComponentPtr any_component = NULL;
            any_component = component_get_indexed_subcomponent(root, i);
            component_default(any_component);
        }
    }
}

dag_err_t
dagx_initialize(DagCardPtr card)
{
    ComponentPtr root_component = NULL;
    /* Set up virtual methods. */
    card_set_dispose_routine(card, dagx_dispose);
    card_set_post_initialize_routine(card, dagx_post_initialize);
    card_set_reset_routine(card, dagx_reset);
    card_set_default_routine(card, dagx_default);

    root_component = component_init(kComponentRoot, card);
    card_set_root_component(card, root_component);
    return kDagErrNone;
}

