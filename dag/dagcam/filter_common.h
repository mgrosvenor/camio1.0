
#ifndef UNIFIED_FILTER_COMMON_H
#define UNIFIED_FILTER_COMMON_H
#include "dagutil.h"
#include "dagreg.h"
#include "unified_filter_interface.h"

unified_filter_error_t  set_private_state(unified_filter_handle_p handle, char* state);
unified_filter_error_t  set_parse_file_function(unified_filter_handle_p handle, funct_ptr_parse_file_t f_ptr);
unified_filter_error_t  set_write_rules_function(unified_filter_handle_p handle, funct_ptr_write_rules_t f_ptr);
unified_filter_error_t  set_cleanup_rules_function(unified_filter_handle_p handle, funct_ptr_cleanup_rules_t f_ptr);
unified_filter_error_t  set_dispose_filter_function(unified_filter_handle_p handle, funct_ptr_dispose_filter_t f_ptr);
unified_filter_error_t  set_enable_ruleset_function(unified_filter_handle_p handle, funct_ptr_enable_ruleset_t f_ptr);
unified_filter_error_t  set_print_rule_function(unified_filter_handle_p handle, funct_ptr_print_rule_list_t f_ptr);
unified_filter_error_t  set_configure_filter_function(unified_filter_handle_p handle, funct_ptr_configure_filter_t f_ptr);
unified_filter_error_t  set_read_verify_filter_function(unified_filter_handle_p handle, funct_ptr_read_verify_rules_t f_ptr);
unified_filter_error_t  set_init_interfaces(unified_filter_handle_p handle, int value);
unified_filter_error_t  set_init_rulesets(unified_filter_handle_p handle, int value);


int     get_init_interfaces(unified_filter_handle_p handle);
int     get_init_rulesets(unified_filter_handle_p handle);
int     get_current_rule_set(unified_filter_handle_p handle);
int     get_current_iface(unified_filter_handle_p handle);
uint8_t*    get_iom(unified_filter_handle_p handle);
void*       get_private_state(unified_filter_handle_p handle);
uint8_t     get_is_parse_only(unified_filter_handle_p handle);
uint8_t     get_is_initialze(unified_filter_handle_p handle);
char*       get_dagname(unified_filter_handle_p handle);
unified_filter_type_t get_filter_type(unified_filter_handle_p handle);

#endif
