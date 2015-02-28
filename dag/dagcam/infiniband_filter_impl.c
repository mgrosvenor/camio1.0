

#include "infiniband_filter_impl.h"
#include "infiniband_proto.h"
#include "dag_config_api.h"
#include "../lib/dagcam/infini_parse.h"
#include "idt75k_lib.h"

#define NO_OF_RULESET_SUPPORTED 8
#define INFINIBAND_MAX_POSSIBLE_RULES 16384

static unified_filter_error_t infiniband_parse_file(unified_filter_handle_p, char*, unified_rule_list_t*);
static unified_filter_error_t infiniband_write_rules(unified_filter_handle_p, unified_rule_list_t*);
static unified_filter_error_t infiniband_cleanup_rules(unified_filter_handle_p, unified_rule_list_t*);
static unified_filter_error_t infiniband_dispose_filter(unified_filter_handle_p);
unified_filter_error_t infiniband_enable_ruleset(unified_filter_handle_p handle, int iface, int ruleset);
static unified_filter_error_t infiniband_print_rule_list(unified_filter_handle_p handle, unified_rule_list_t *in_list, FILE* out_stream);
static unified_filter_error_t infiniband_read_verify_rule_list(unified_filter_handle_p handle, unified_rule_list_t *in_list);
static unified_filter_error_t   infiniband_configure_filter(unified_filter_handle_p handle, unified_filter_param_t *param);
static void print_rule(int rule_number, infiniband_filter_rule_t *in_rule, FILE *out_stream);

typedef struct
{
    int max_rules;
    int rule_width;         /** 144 or 576 bit rules */
    idt75k_dev_p  dev_p;
    dag_card_ref_t card_ref;
}infiniband_filter_private_state_t;





unified_filter_error_t  infiniband_filter_creator(unified_filter_handle_p handle, unified_filter_param_t* in_params)
{
    infiniband_filter_private_state_t *filter_state = NULL;
    //shm_infiniband_state_t  previous_state;
    int res = 0;

    if ( NULL == handle )
        return kUnifiedFilterGeneralError;
    dagutil_verbose_level(1,"Creating Infiniband filter object\n");
    filter_state = (infiniband_filter_private_state_t *) malloc ( sizeof(infiniband_filter_private_state_t));
    memset( filter_state, 0, sizeof (infiniband_filter_private_state_t) );

    filter_state->max_rules = INFINIBAND_MAX_POSSIBLE_RULES;

    set_private_state(handle,(void*)filter_state);
    set_parse_file_function( handle, infiniband_parse_file);
    set_cleanup_rules_function(handle, infiniband_cleanup_rules);
    set_dispose_filter_function(handle, infiniband_dispose_filter);
    set_print_rule_function(handle, infiniband_print_rule_list);

    if( 0 ==  get_is_parse_only(handle))
    {
        set_write_rules_function(handle, infiniband_write_rules);
        set_enable_ruleset_function(handle, infiniband_enable_ruleset);
        set_read_verify_filter_function(handle , infiniband_read_verify_rule_list);
        /* set the configure function */
        set_configure_filter_function(handle , infiniband_configure_filter);

        filter_state->dev_p = (idt75k_dev_p) malloc ( sizeof(idt75k_dev_t));
        memset( filter_state->dev_p, 0, sizeof(idt75k_dev_t));
        /*RULESET INITED TO 8.dont matter what the user says.*/
        set_init_rulesets(handle, NO_OF_RULESET_SUPPORTED);
        if( 1 == get_is_initialze(handle))
        {
            
            res = tcam_get_device(filter_state->dev_p,get_dagname(handle), 0);
            res = tcam_initialise(filter_state->dev_p, filter_state->dev_p);
			if ( res != 0 )
			{
				dagutil_error ("\t Tcam  Init Failed(%d) in %s\n", res, __FUNCTION__);
                return kUnifiedFilterGeneralError;
			} 
            else
            {
				dagutil_verbose_level(1, "TCAM initialised \n");
			}
             /* initialize card_ref */
            filter_state->card_ref = dag_config_init(get_dagname(handle));
            if ( NULL == filter_state->card_ref )
            {
                dagutil_error("Could not initialize the card reference \n");
                return kUnifiedFilterGeneralError;
            }
		}
        else
        {
#if 0
            /* read previously initialized data */
            dagutil_verbose_level(2,"infiniband filter, Retrieving the previous initialized state.. \n");
            memset( &previous_state, 0 , sizeof (shm_infiniband_state_t));
            retrieve_previous_infiniband_state(&previous_state);
            if ( (0 == previous_state.previous_iface_based) && (-1 != get_current_iface ( handle )) )
            {
                dagutil_error("Previous initialize not supporting interface based filtering. Please re-initialize \n");
                return kUnifiedFilterGeneralError;
            }
            set_init_rulesets(handle, previous_state.previous_rulesets);
            set_init_interfaces(handle , previous_state.previous_ports);
#endif
        }
        /* update the max rules allowed*/
        filter_state->max_rules = INFINIBAND_MAX_POSSIBLE_RULES;
        dagutil_verbose_level(1,"infiniband: The max Rules supported would be %d\n",filter_state->max_rules);
        /* validate current interface and current iface */
        if ( (get_current_iface( handle ) > (get_init_interfaces(handle) - 1)) )
        {
            dagutil_error("Invalid interface: %d\n",get_current_iface( handle ));
            return kUnifiedFilterInvalidParameter;
        }
        if ( get_current_rule_set( handle ) > (get_init_rulesets(handle) - 1) )
        {
            dagutil_error("Invalid ruleset: %d\n",get_current_rule_set( handle ));
            return kUnifiedFilterInvalidParameter;
        }
    }
    return kUnifiedFilterSuccess;
}

unified_filter_error_t infiniband_parse_file(unified_filter_handle_p handle, char *file_name, unified_rule_list_t *list)
{
    FILE *fin = NULL;
	int retval = 0;
    infiniband_filter_private_state_t *filter_state = NULL;
    if ( kInfinibandFilter != get_filter_type(handle) )
    {
        dagutil_error("Invalid Filtertype(%d) for infiniband filter in %s\n",get_filter_type(handle),__FUNCTION__);
        return kUnifiedFilterInvalidFilterType;
    }

    filter_state = (infiniband_filter_private_state_t*)get_private_state(handle);
    if( NULL == filter_state)
    {
        dagutil_error("Unable to get the private state of infiniband filter\n");
        return kUnifiedFilterGeneralError;
    }
    fin = fopen(file_name,"r");
	if( fin == NULL )
	{
		dagutil_error("Filter file is missing or no access\n");
		return kUnifiedFilterFileOpenError;
	}
    /* assume the rule width is 144 bit */
    list->rule_width = 144;
    
    scanner_set_stdout(stdout);

    infinirestart(fin);
    memset( &infini_filter_rule, 0, sizeof (infiniband_filter_rule_t));
    while(1)
	{
        infiniband_filter_rule_t *this_rule = NULL;
        unified_rule_node_p this_node = NULL;
        retval = infinilex();
		if(retval == T_RULE_DONE)
		{
            if( list->count >=  filter_state->max_rules)
            {
                dagutil_warning("Number of rules exceeded the maximum allowed-%d\n",filter_state->max_rules);
                continue;
            }
            

            this_rule = (infiniband_filter_rule_t*) malloc ( sizeof(infiniband_filter_rule_t));
            this_node = (unified_rule_node_p) malloc ( sizeof(unified_rule_node_t));
            if ( (NULL == this_rule)  || (NULL == this_node))
            {
                dagutil_error("malloc Error in Rule parsing.\n");
                return kUnifiedFilterGeneralError;
            }
            memset(this_node , 0 , sizeof (unified_rule_node_t));
		  //copy the parsed rule into the ruleset
            memcpy( this_rule,&infini_filter_rule,sizeof(infiniband_filter_rule_t) );
            this_node->generic_rule = (unified_rule_p) this_rule;
		    if (NULL != list->head)
            {
                list->tail->next = this_node;
                list->tail = this_node;
            }
            else
            {
                list->head = this_node;
                list->tail = this_node;
            }
            list->count++;
            
		}
		else if (retval == T_RULE_CONTINUE)
		{
			printf("This state is unused please contact suppot@endace.com \
 and send the rule file used and this line print out. retval: %d rules_count:%d\n",retval,list->count);
		}
		else if ( retval < 0 )
		{
			printf(" errors flex returns: %d at rule: %d\n",retval,list->count);
			break;
		}
		else if (retval == 0)
		{
			break;
			
		} else {
			printf("Unknown state please contact suppot@endace.com \
 and send the rule file used and this line print out. retval: %d rules_count:%d\n",retval,list->count);
		}
    
    memset( &infini_filter_rule, 0, sizeof (infiniband_filter_rule_t));
	}	
    if ( list->count > 0)
    {
        list->filter_type = kInfinibandFilter;
    }
	
	/* clean up */
	fclose(fin);
    if ( 0 == retval)
    {
       dagutil_verbose_level(1,"Ruleset file parsed successifully, %d rules have been created.Rule Length is %d bits\n", list->count,list->rule_width);
	   return kUnifiedFilterSuccess;
    }
    else
    {
        return kUnifiedFilterParseError;
    }
}

unified_filter_error_t infiniband_write_rules(unified_filter_handle_p handle, unified_rule_list_t *list)
{
    infiniband_filter_private_state_t *filter_state = NULL;
    infiniband_filter_rule_t *current_rule ;
    unified_rule_node_p current = NULL ;
    unified_filter_error_t ret_val = kUnifiedFilterSuccess;
    int res = 0;
    int index = 0;
    if ( kInfinibandFilter != get_filter_type(handle) || (kInfinibandFilter != list->filter_type))
    {
        dagutil_error("Invalid Filtertype(%d) for infiniband filter in %s\n",get_filter_type(handle),__FUNCTION__);
        return kUnifiedFilterInvalidFilterType;
    }

    filter_state = (infiniband_filter_private_state_t*)get_private_state(handle);
    if( NULL == filter_state)
    {
        dagutil_error("Unable to get the private state of infiniband  filter in %s\n",__FUNCTION__);
        return kUnifiedFilterGeneralError;
    }

    if ( NULL == filter_state->dev_p )
    {
        dagutil_error("TCAM handle is not defind \n");
        return kUnifiedFilterGeneralError;
    }
    current = list->head;
    while(NULL != current)
    {
        current_rule = (infiniband_filter_rule_t*)(current->generic_rule);
        res = infiniband_write_entry( filter_state->dev_p,get_current_rule_set(handle),index, current_rule,0);
        if ( 0 != res )
        {
            dagutil_error("TCAM write failed for index %d\n",index );
            ret_val = kUnifiedFilterTCAMWriteError;
            break;
        }
	current = current->next;
	index = index + 1;
    }
    
    return ret_val;
}
unified_filter_error_t infiniband_cleanup_rules(unified_filter_handle_p handle, unified_rule_list_t *list)
{
    unified_rule_node_p current = NULL , next = NULL;
    dagutil_verbose_level(1,"in %s .Rule count is %d\n",__FUNCTION__,list->count);
    if ( kInfinibandFilter != get_filter_type(handle) )
    {
        dagutil_error("Invalid Filtertype(%d) for infiniband filter in %s\n",get_filter_type(handle),__FUNCTION__);
        return kUnifiedFilterInvalidFilterType;
    }
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

unified_filter_error_t infiniband_dispose_filter(unified_filter_handle_p handle)
{
    infiniband_filter_private_state_t *filter_state = NULL;
    //shm_infiniband_state_t  current_state;
    if ( kInfinibandFilter != get_filter_type(handle) )
    {
        dagutil_error("Invalid Filtertype(%d) for infiniband filter in %s\n",get_filter_type(handle),__FUNCTION__);
        return kUnifiedFilterInvalidFilterType;
    }

    filter_state = (infiniband_filter_private_state_t*)get_private_state(handle);
    if( NULL == filter_state)
    {
        dagutil_error("Unable to get the private state of infiniband filter\n");
        return kUnifiedFilterGeneralError;
    }
    #if 0
    memset( &current_state, 0 , sizeof (shm_infiniband_state_t));
    current_state.previous_iface_based = filter_state->is_interface_filtering;
    current_state.previous_ports =  (filter_state->ptr_filter_header)?(filter_state->ptr_filter_header->interface_number):get_init_interfaces(handle);
    current_state.previous_rulesets = (filter_state->ptr_filter_header)?(filter_state->ptr_filter_header->rule_set):get_init_rulesets(handle);
    store_current_infiniband_state(&current_state);
#endif

     /* dispose the card_ref */
    if ( NULL != filter_state->card_ref)
    {
        dag_config_dispose(filter_state->card_ref);
    }

    /* remove the filter state's malloc-ed memory here */
    if ( filter_state->dev_p)
    {
        free(filter_state->dev_p);
        filter_state->dev_p = NULL;
    }
    return kUnifiedFilterSuccess;
}

unified_filter_error_t infiniband_print_rule_list(unified_filter_handle_p handle, unified_rule_list_t *in_list, FILE* out_stream)
{
    unified_rule_node_p current = NULL;
    infiniband_filter_rule_t *ib_rule = NULL;
    int rule_count = 0;
    if ( kInfinibandFilter != get_filter_type(handle) )
    {
        dagutil_error("Invalid Filtertype(%d) for infiniband filter in %s\n",get_filter_type(handle),__FUNCTION__);
        return kUnifiedFilterInvalidFilterType;
    }

    if( NULL == in_list )
    {
        dagutil_error("NULL rule list in %s\n",__FUNCTION__);
        return kUnifiedFilterInvalidParameter;
    }
    
    if ( NULL == out_stream )
    {
        dagutil_error("NULL out stream in  %s\n",__FUNCTION__);
        return kUnifiedFilterInvalidParameter;
    }
    if ( in_list->count <= 0 )
    {
        dagutil_warning("No Rules be printed \n");
    }
    
    current = in_list->head;
    while( current)
    {
        ib_rule = (infiniband_filter_rule_t*) current->generic_rule;
        if( NULL == ib_rule )
        {
            dagutil_verbose_level(3,"Skipping NULL rule in %s\n", __FUNCTION__);
            continue;
        }
        rule_count++;
        print_rule(rule_count, ib_rule,out_stream);
        current = current->next;
    }
    return kUnifiedFilterSuccess;
}
unified_filter_error_t infiniband_enable_ruleset(unified_filter_handle_p handle, int iface, int ruleset)
{
    infiniband_filter_private_state_t *filter_state = NULL;
    dag_component_t root_component = 0, ib_classifier_component = 0;
    attr_uuid_t current_attribute = 0;
    
    if ( kInfinibandFilter != get_filter_type(handle) )
    {
        dagutil_error("Invalid Filtertype(%d) for infiniband filter in %s\n",get_filter_type(handle),__FUNCTION__);
        return kUnifiedFilterInvalidFilterType;
    }

    filter_state = (infiniband_filter_private_state_t*)get_private_state(handle);
    if( NULL == filter_state)
    {
        dagutil_error("Unable to get the private state of infiniband filter\n");
        return kUnifiedFilterGeneralError;
    }
    
    if ( ruleset > (get_init_rulesets(handle) - 1) )
    {
        dagutil_error("Invalid ruleset. Should be less than %d\n", get_init_rulesets(handle));
        return kUnifiedFilterInvalidParameter;
    }
    
    if ( iface > (get_init_interfaces(handle) - 1) )
    {
        dagutil_error("Invalid interface. Should be less than %d\n", get_init_interfaces(handle));
        return kUnifiedFilterInvalidParameter;
    }
   
    if ( NULL == filter_state->card_ref)
    {
        dagutil_error(" Card ref NULL in %s\n",__FUNCTION__);
        return kUnifiedFilterGeneralError;
    }
    root_component = dag_config_get_root_component(filter_state->card_ref);
    if ( NULL == root_component )
    {
        dagutil_error(" root_component  NULL in %s\n",__FUNCTION__);
        return kUnifiedFilterGeneralError;
    }
    ib_classifier_component = dag_component_get_subcomponent(root_component, kComponentInfinibandClassifier, 0 );
    if ( NULL == ib_classifier_component )
    {
        dagutil_error(" infiniband classifier_component  NULL in %s\n",__FUNCTION__);
        return kUnifiedFilterGeneralError;
    }

    current_attribute = dag_component_get_attribute_uuid( ib_classifier_component, kUint32AttributeDataBase );
    if (current_attribute )
    {
        dag_config_set_uint32_attribute(filter_state->card_ref, current_attribute, (uint32_t)ruleset);
        return kUnifiedFilterInvalidParameter;
    }
    return kUnifiedFilterSuccess;
}

unified_filter_error_t infiniband_read_verify_rule_list(unified_filter_handle_p handle, unified_rule_list_t *in_list)
{
     infiniband_filter_private_state_t *filter_state = NULL;
    
    if ( kInfinibandFilter != get_filter_type(handle) || (kInfinibandFilter != in_list->filter_type))
    {
        dagutil_error("Invalid Filtertype(%d) for infiniband filter in %s\n",get_filter_type(handle),__FUNCTION__);
        return kUnifiedFilterInvalidFilterType;
    }

    filter_state = (infiniband_filter_private_state_t*)get_private_state(handle);
    if( NULL == filter_state)
    {
        dagutil_error("Unable to get the private state of infiniband filter in %s\n",__FUNCTION__);
        return kUnifiedFilterGeneralError;
    }
    
    return kUnifiedFilterSuccess;
}

void print_rule(int rule_number, infiniband_filter_rule_t *in_rule, FILE *out_stream)
{
    fprintf(out_stream,"Rule Number: %d\n", rule_number);
    fprintf( out_stream, "User tag: %d, action: %s, steering: %d \n", 
                in_rule->user_tag, (in_rule->action == 0)?"through":"drop" , in_rule->user_class );
    fprintf( out_stream, "service_level data: 0x%01x, service_level mask: 0x%01x \n",  in_rule->service_level.data, in_rule->service_level.mask );
    fprintf( out_stream, "lnh.data: 0x%01x, lnh.mask: 0x%01x\n",  in_rule->lnh.data, in_rule->lnh.mask);
    fprintf( out_stream, "src_local_id.data: 0x%04x, src_local_id.mask: 0x%04x\n",  in_rule->src_local_id.data, in_rule->src_local_id.mask);
    fprintf( out_stream, "dest_local_id.data: 0x%04x, dest_local_id.mask: 0x%04x\n", in_rule->dest_local_id.data, in_rule->dest_local_id.mask);
    fprintf( out_stream, "opcode.data: 0x%01x, opcode.mask: 0x%01x\n", in_rule->opcode.data,in_rule->opcode.mask);
    fprintf( out_stream, "dest_qp.data: 0x%06x, dest_qp.mask: 0x%06x \n", in_rule->dest_qp.data, in_rule->dest_qp.mask);
    fprintf( out_stream, "src_qp.data: 0x%06x, src_qp.mask: 0x%06x \n", in_rule->src_qp.data, in_rule->src_qp.mask);
return;
}

unified_filter_error_t   infiniband_configure_filter(unified_filter_handle_p handle, unified_filter_param_t *param)
{
 return kUnifiedFilterSuccess;
}


