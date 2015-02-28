

#include "dagapi.h"
#include "dag_config.h"
#include "dag_component.h"
#include "filter_common.h"
#include "ipf_v2_filter_impl.h"
#include "infiniband_filter_impl.h"
#include "bfs_filter_impl.h"

typedef struct{
    char    *m_dagname;
    unified_filter_type_t m_filter_type;
    uint8_t m_is_parse_only;
    uint8_t m_is_initialize;
    /* TCAM is conifigured into init_interfaces X init_rulesets databases */
    int  m_init_interfaces;
    int m_init_rulesets;

    /* the current ruleset ( database ) where to load the rules */
    int m_current_rule_set;
    /* the current port number */
    int m_current_iface ;
    /* total interface count */
    int m_total_iface_count ;
    /* dag iom */
    uint8_t    * m_iom;
    /* private state data specefic to each filter */
    void *m_private_state;
    /* function pointers */
    funct_ptr_parse_file_t          m_fp_parse_file;
    funct_ptr_write_rules_t         m_fp_write_rules;
    funct_ptr_cleanup_rules_t       m_fp_cleanup_rules;
    funct_ptr_dispose_filter_t      m_fp_dispose_filter;
    funct_ptr_enable_ruleset_t      m_fp_enable_ruleset;
    funct_ptr_print_rule_list_t     m_fp_print_rule_list;
    funct_ptr_configure_filter_t    m_fp_configure_filter;
    funct_ptr_read_verify_rules_t   m_fp_read_verify_rules;
}unified_filter_structure_t;
typedef struct 
{
    dag_reg_module_t reg_module;
    int version;
    unified_filter_type_t filter_type;
    funct_ptr_creator_t fp_creator;
}filter_map_array_t;

static const filter_map_array_t filter_map_array[]=
{
    {DAG_REG_INFINICLASSIFIER, -1, kInfinibandFilter,infiniband_filter_creator },
	{DAG_REG_PPF, 4, kBFSFilter, bfs_filter_creator },
	{DAG_REG_PPF, -1, kIPFV2Filter, ipf_v2_filter_creator },
};

static int is_valid_handle( unified_filter_structure_t *filter_handle);
static unified_filter_error_t initialize_from_firmware_modules(unified_filter_structure_t *result, unified_filter_param_t *param);
static unified_filter_error_t  initialize_filter_of_given_type(unified_filter_structure_t *result, unified_filter_param_t *param);
static void check_filter_for_completenes(unified_filter_structure_t *result);

static int get_interface_count(char *dagname);
static unified_filter_error_t generic_cleanup_rules(unified_filter_handle_p handle, unified_rule_list_t *list);
unified_filter_handle_p filter_factory_get_filter(unified_filter_param_t *param)
{
    unified_filter_structure_t *result = NULL;
    unified_filter_error_t error_code = kUnifiedFilterSuccess;
    
    result = (unified_filter_structure_t*) malloc ( sizeof(unified_filter_structure_t));
    memset(result, 0, sizeof(unified_filter_structure_t));


	if ( NULL == result )
    {
        dagutil_error("Could not create filter object \n");
        return NULL;
    }
    dagutil_verbose_level(2, "\nDagname:\t %s \nInit Iface:\t %d\nInit Ruleset:\t %d\nCur Iface:\t %d\nCur Ruleset:\t %d\nInitialized:\t %u\nParseonly:\t %u\nFilterType:\t %d\n",param->dagname,param->init_interfaces, param->init_rulesets, param->cur_iface, param->cur_ruleset,param->initialise, param->parse_only, param->type);

    if ( ( 0 == param->init_interfaces ) || ( 0 == param->init_interfaces ) )
    {
        dagutil_error("Init interface count or Init Rulesets are given as 0\n");
        return NULL;
    }
    result->m_dagname = (char*) malloc( strlen ( param->dagname) + 1 );
    strcpy( result->m_dagname, param->dagname);

    result->m_current_rule_set  = param->cur_ruleset;
    result->m_current_iface     = param->cur_iface;
    result->m_is_initialize     = param->initialise;

    /* assign a default clean up rules - the filter implementation should use customized one if they r necessary */
    result->m_fp_cleanup_rules  = generic_cleanup_rules;

    if (1 == param->parse_only)
    {
        error_code = initialize_filter_of_given_type(result, param);
    }
    else
    {
        
        /* validate init interface count */
        result->m_total_iface_count = get_interface_count(param->dagname);
        if (-1 == param->init_interfaces)
        {
            if (result->m_is_initialize)
                dagutil_warning("Init interface was not given. Making it as %d\n", result->m_total_iface_count);
            result->m_init_interfaces   = 1;
        }
        else if ( ( param->init_interfaces > result->m_total_iface_count)  )
        {
            /* if initializing give out a warning */
            if (result->m_is_initialize)
                dagutil_warning("Init interface was %d, more than what the card has. Making it to %d\n", param->init_interfaces, result->m_total_iface_count);
            result->m_init_interfaces = result->m_total_iface_count;
        }
        else
        {
            result->m_init_interfaces = param->init_interfaces;
        }

        if ( -1 == param->init_rulesets )
        {
            if (result->m_is_initialize)
                dagutil_warning("Init ruleset was not given. Making it as %d\n", 1);
            result->m_init_rulesets = 1;
        }
        else
        {
            result->m_init_rulesets = param->init_rulesets;
        }

        if ( kInvalidFilterType == param->type )
        {
           
		error_code = initialize_from_firmware_modules(result, param);
        }
        else
        {
            error_code = initialize_filter_of_given_type(result, param);
        }
    }
    
    if ( kUnifiedFilterSuccess != error_code)
    {
        dagutil_error("Filter Creation failed. Error code %d\n",error_code);
        if ( result->m_dagname )
        {
            free ( result->m_dagname);
        }
        if ( result->m_private_state)
        {
            free ( result->m_private_state );
        }
        free (result);
        result = NULL;
    }

    /* check the fitler handle for completeness and print warnings */
    check_filter_for_completenes(result);
    return (unified_filter_handle_p)result;
}

unified_filter_error_t  initialize_filter_of_given_type(unified_filter_structure_t *result, unified_filter_param_t *param)
{
    int i = 0;
    int filter_array_count= sizeof(filter_map_array)/sizeof(filter_map_array_t);

    result->m_is_parse_only = 1;
    result->m_filter_type = param->type;
    for (i = 0; i < filter_array_count; i++)
    {
        if ( filter_map_array[i].filter_type == result->m_filter_type)
        {
            return filter_map_array[i].fp_creator(result, param);
        }
    }
    dagutil_error("Could not find the initializer for the filter type %d\n",result->m_filter_type);
    return kUnifiedFilterInvalidFilterType;
}
unified_filter_error_t  initialize_from_firmware_modules(unified_filter_structure_t *result, unified_filter_param_t *param)
{
    int dagfd = -1;
    dag_reg_t  regs[DAG_REG_MAX_ENTRIES];
    int count = 0;
    int i;
    int filter_array_count= sizeof(filter_map_array)/sizeof(filter_map_array_t);

    printf("initialize from firmware modules \n");
    dagfd = dag_open (result->m_dagname);
    if ( dagfd < 0 )
    {
        dagutil_error("dag_open failed \n");
        return kUnifiedFilterDAGOpenError ;
    }
    
    result->m_iom = dag_iom(dagfd);
    if ( result->m_iom == NULL )
    {
        dagutil_error("dag_iom failed \n");
        return kUnifiedFilterDAGOpenError;
    }
    for (i = 0; i < filter_array_count; i++)
    {
        count = dag_reg_find((char*) (result->m_iom), filter_map_array[i].reg_module, regs);
        if ( count > 0 )
        {
            if( (-1 == filter_map_array[i].version) ||(regs[0].version == filter_map_array[i].version ) )
            {
                result->m_filter_type = filter_map_array[i].filter_type;
                return filter_map_array[i].fp_creator(result, param);
            }
        }
    }
    dagutil_error("Could not find the any known filters in the Card\n");
    return kUnifiedFilterInvalidFilterType;
}

unified_filter_error_t set_init_interfaces(unified_filter_handle_p handle, int value)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_init_interfaces = value;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t set_init_rulesets(unified_filter_handle_p handle, int value)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_init_rulesets = value;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}


unified_filter_error_t set_private_state(unified_filter_handle_p handle, char* state)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_private_state = state;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t set_parse_file_function(unified_filter_handle_p handle, funct_ptr_parse_file_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_parse_file = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}
unified_filter_error_t set_write_rules_function(unified_filter_handle_p handle, funct_ptr_write_rules_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_write_rules = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t set_cleanup_rules_function(unified_filter_handle_p handle, funct_ptr_cleanup_rules_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_cleanup_rules = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t set_dispose_filter_function(unified_filter_handle_p handle, funct_ptr_dispose_filter_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_dispose_filter = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}
unified_filter_error_t set_enable_ruleset_function(unified_filter_handle_p handle, funct_ptr_enable_ruleset_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_enable_ruleset = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t set_print_rule_function(unified_filter_handle_p handle, funct_ptr_print_rule_list_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_print_rule_list = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t  set_configure_filter_function(unified_filter_handle_p handle, funct_ptr_configure_filter_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_configure_filter = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t  set_read_verify_filter_function(unified_filter_handle_p handle, funct_ptr_read_verify_rules_t f_ptr)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        filter_handle->m_fp_read_verify_rules = f_ptr;
        return kUnifiedFilterSuccess;
    }
    return kUnifiedFilterInvalidHandle;
}

int get_init_interfaces(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_init_interfaces;
    }
    return 0;
}

int  get_init_rulesets(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_init_rulesets;
    }
    return 0;
}
int  get_current_rule_set(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_current_rule_set;
    }
    return 0;
}

int  get_current_iface(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_current_iface;
    }
    return -1;
}
uint8_t* get_iom(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_iom;
    }
    return 0;
}
void* get_private_state(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_private_state;
    }
    return 0;
}

uint8_t get_is_parse_only(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_is_parse_only;
    }
    return 0;
}

uint8_t get_is_initialze(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_is_initialize;
    }
    return 0;
}

unified_filter_type_t get_filter_type(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_filter_type;
    }
    return 0;
}

char* get_dagname(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    if( is_valid_handle(filter_handle) )
    {
        return filter_handle->m_dagname;
    }
    return 0;
}
int is_valid_handle( unified_filter_structure_t *filter_handle)
{
    return ( NULL != filter_handle )?1:0;
}

unified_filter_error_t parse_rule_file(unified_filter_handle_p handle, char* filename,unified_rule_list_t *list)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_fp_parse_file )
        {
            return filter_handle->m_fp_parse_file(handle, filename,list);
        }
        else
        {
            dagutil_error("Invalid function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t write_rules(unified_filter_handle_p handle, unified_rule_list_t *list)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_is_parse_only)
        {
            dagutil_warning("Filter created for only parsing. Not attempting to write to TCAM \n");
            return kUnifiedFilterSuccess;
        }
        if ( filter_handle->m_fp_write_rules )
        {
            return filter_handle->m_fp_write_rules( handle, list);
        }
        else
        {
            dagutil_error("Invalid function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }
    return kUnifiedFilterInvalidHandle;
}
unified_filter_error_t cleanup_rules(unified_filter_handle_p handle, unified_rule_list_t *list)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_fp_cleanup_rules )
        {
            return filter_handle->m_fp_cleanup_rules(handle, list);
        }
        else
        {
            dagutil_error("Invalid function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }
    return kUnifiedFilterInvalidHandle;
}
unified_filter_error_t dispose_filter(unified_filter_handle_p handle)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_fp_dispose_filter )
        {
            return filter_handle->m_fp_dispose_filter(handle);
        }
        else
        {
            dagutil_error("Invalid  function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }

    if ( filter_handle->m_dagname )
    {
        free (filter_handle->m_dagname);
        filter_handle->m_dagname = NULL;
    }
     if ( filter_handle->m_private_state )
    {
        free (filter_handle->m_private_state);
        filter_handle->m_private_state = NULL;
    }
    return kUnifiedFilterInvalidHandle;
}


unified_filter_error_t enable_ruleset(unified_filter_handle_p handle,int ruleset, int iface)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_fp_enable_ruleset )
        {
            int i = 0;
            unified_filter_error_t ret_val = kUnifiedFilterSuccess;
            /* interface is given as -1. Set the ruleset for all the interfaces */
            if ( -1 == iface ) 
            {
                for ( i = 0; i < filter_handle->m_total_iface_count ; i++)
                {
                    ret_val = filter_handle->m_fp_enable_ruleset(handle, ruleset, i);
                    if ( kUnifiedFilterSuccess != ret_val ) 
                    {
                        dagutil_error("In %s: Setting ruleset failed for interface: %d\n",__FUNCTION__, i );
                        return ret_val;
                    }
                }
            }
            else /* a valid iface number is specified . Do it for only that iface */
            {
                return filter_handle->m_fp_enable_ruleset(handle, ruleset, iface);
            }
        }
        else
        {
            dagutil_error("Invalid  function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }
    return kUnifiedFilterInvalidHandle;
}

unified_filter_error_t print_rule_list(unified_filter_handle_p handle, unified_rule_list_t *in_list , FILE *out_stream)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_fp_print_rule_list )
        {
            return filter_handle->m_fp_print_rule_list(handle, in_list, out_stream);
        }
        else
        {
            dagutil_error("Invalid  function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }
    return kUnifiedFilterInvalidHandle;
}


unified_filter_error_t configure_filter(unified_filter_handle_p handle, unified_filter_param_t *param)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_fp_configure_filter )
        {
            return filter_handle->m_fp_configure_filter(handle, param);
        }
        else
        {
            dagutil_error("Invalid  function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }
    return kUnifiedFilterInvalidHandle;
}
unified_filter_error_t read_verify_rules(unified_filter_handle_p handle, unified_rule_list_t *list)
{
    unified_filter_structure_t *filter_handle;
    filter_handle = (unified_filter_structure_t*)handle;
    dagutil_verbose_level(3,"In %s\n", __FUNCTION__);
    if( is_valid_handle(filter_handle))
    {
        if ( filter_handle->m_is_parse_only)
        {
            dagutil_warning("Filter created for only parsing. Not attempting to verify \n");
            return kUnifiedFilterSuccess;
        }
        if ( filter_handle->m_fp_read_verify_rules )
        {
            return filter_handle->m_fp_read_verify_rules( handle, list);
        }
        else
        {
            dagutil_error("Invalid function pointer in %s\n",__FUNCTION__);
            return kUnifiedFilterGeneralError;
        }
    }
    return kUnifiedFilterInvalidHandle;
}
int get_interface_count(char *dag_name)
{
    dag_component_t root = NULL;
	dag_component_t gpp_comp = NULL;
	dag_component_t port_comp = NULL;
	dag_component_t phy_comp = NULL;
    dag_card_ref_t card_ref = NULL;
	dag_component_t cps_comp = NULL;
    uint32_t gpp_count = 0;
	uint32_t iface_count = 0;
	attr_uuid_t attr = kNullAttributeUuid;
    int i = 0;

    card_ref = dag_config_init(dag_name);
    if ( NULL == card_ref )
    {
        dagutil_error("dag_config_init failed \n");
        return 0;
    }
	root = dag_config_get_root_component(card_ref);
    if ( NULL == root)
    {
        dagutil_error("root component is NULL\n");
        return 0;
    }
    gpp_count = dag_component_get_subcomponent_count_of_type(root, kComponentGpp);
    for ( i = 0; i < gpp_count; i++)
    {
        gpp_comp = dag_component_get_subcomponent(root, kComponentGpp, i);
        if(gpp_comp != NULL)
	   {
            attr = dag_component_get_attribute_uuid(gpp_comp,kUint32AttributeInterfaceCount);
            if(attr)
            iface_count += dag_config_get_uint32_attribute(card_ref,attr);
	   }
    }
	if ( 0 == iface_count)
	{
		port_comp = dag_component_get_subcomponent(root, kComponentPort, 0);
		if (port_comp != NULL)
			iface_count = dag_component_get_subcomponent_count_of_type(root, kComponentPort);
		else
		{
			phy_comp = dag_component_get_subcomponent(root, kComponentPhy, 0);
			if (phy_comp != NULL)
				iface_count = dag_component_get_subcomponent_count_of_type(root, kComponentPhy);
			else
			{
				/*This code was added to support Dag 8.5 cards.It does not have GPP component.*/
				cps_comp = dag_component_get_subcomponent(root,kComponentInfiniBandFramer,0);
				if (cps_comp != NULL)
					iface_count = dag_component_get_subcomponent_count_of_type(root,kComponentInfiniBandFramer);
			}	
		}
	}
    dag_config_dispose(card_ref);
	return iface_count;
}

void check_filter_for_completenes(unified_filter_structure_t *result)
{
    if ( 0 == is_valid_handle( result) )
        return;
    if ( NULL == result->m_fp_cleanup_rules )
    {
        dagutil_warning("No Function to clean up rule list. May cause some memory leak \n");
    }
    if ( NULL == result->m_fp_dispose_filter)
    {
            dagutil_warning("No Function dispose filter. May cause some memory leak \n");
    }
    if ( NULL == result->m_fp_write_rules)
    {
        if ( 0 == result->m_is_parse_only )
        {
            dagutil_warning("No Function to write rules into TCAM.\n");
        }
    }
    if ( NULL == result->m_fp_print_rule_list)
    {
        if ( 1 == result->m_is_parse_only )
        {
            dagutil_warning("No Function to print the parsed rules.\n");
        }
    }
    return;
}

unified_filter_error_t generic_cleanup_rules(unified_filter_handle_p handle, unified_rule_list_t *list)
{
    unified_rule_node_p current = NULL , next = NULL;
    dagutil_verbose_level(1,"in %s .Rule count is %d\n",__FUNCTION__,list->count);
   
    current = list->head;
    while( current)
    {
        free ( current->generic_rule);
        next = current->next;
        free (current);
        current = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    return kUnifiedFilterSuccess;
}
