
#ifndef UNIFIED_FILTER_INTERFACE_H
#define UNIFIED_FILTER_INTERFACE_H
/** \defgroup UFI Unified Filtering API.
 */
/*@{*/
/**
 * Filtering error codes
 */ 
typedef enum 
{
    kUnifiedFilterSuccess,
    kUnifiedFilterGeneralError,
    kUnifiedFilterInvalidFilterType,
    kUnifiedFilterFileOpenError,
    kUnifiedFilterParseError,
    kUnifiedFilterInvalidParameter,
    kUnifiedFilterInvalidHandle,
    kUnifiedFilterDAGOpenError,
    kUnifiedFilterTCAMWriteError,
    kUnifiedFilterTCAMReadError,
}unified_filter_error_t;

typedef void* unified_filter_handle_p;
typedef void* unified_rule_p;
/**
 * Supported filter types
 */ 
typedef enum 
{
    kInvalidFilterType = 0,
    kInfinibandFilter,
    kIPFV2Filter,
	kBFSFilter,
}unified_filter_type_t;
/**
 * Generic filter_rule structure
 */ 
/* rule structure */
typedef struct unified_rule_node_s
{
    unified_rule_p generic_rule; /**< the actual rule. Opaque type. the actual structure would be handled by each filter*/
    struct unified_rule_node_s  *next; /**< next pointer*/
}unified_rule_node_t, *unified_rule_node_p;
/**
 * Generic rule list structure
 */ 
typedef struct
{
    int count;
    int rule_width;
    unified_filter_type_t filter_type;
    unified_rule_node_p head;
    unified_rule_node_p tail;
}unified_rule_list_t;
/**
 *  Enum for different mapping possible for the filtered ERF.\n
 */
typedef enum 
{
    kMappingUnknown, /**<\n*/
    kMappingRxError, /**< colour mapped to Rx Error*/
    kMappingLossCntrl, /**< colour mapped to Loss Counter*/
    kMappingPadOffset, /**< colour mapped to Pad Offset*/
}unified_filter_mapping_t;
/**
 *  Different link type . Currently contains only IPF specific types.\n
 */
typedef enum
{
    kLinkUnknown, 
    kLinkEthernet,
    kLinkHDLC,
    kLinkPPP, 
}unified_filter_linktype_t;
/**
 * Filtering configuration parameters \n
 */ 
typedef struct unified_filter_param {
    char *dagname;
    uint8_t initialise;
    uint8_t parse_only;
    int init_interfaces;
    int init_rulesets;

    int cur_iface;
    int cur_ruleset;

    int tcam_width;
    unified_filter_type_t type;
    unified_filter_linktype_t linktype;
    unified_filter_mapping_t mapping;
    uint8_t is_no_drop;
} unified_filter_param_t;

typedef unified_filter_error_t  (*funct_ptr_creator_t)(unified_filter_handle_p, unified_filter_param_t* );
typedef unified_filter_error_t (*funct_ptr_parse_file_t)(unified_filter_handle_p, char*, unified_rule_list_t*);  
typedef unified_filter_error_t (*funct_ptr_write_rules_t)(unified_filter_handle_p, unified_rule_list_t*);
typedef unified_filter_error_t (*funct_ptr_read_verify_rules_t)(unified_filter_handle_p, unified_rule_list_t*);
typedef unified_filter_error_t (*funct_ptr_cleanup_rules_t)(unified_filter_handle_p, unified_rule_list_t*);
typedef unified_filter_error_t (*funct_ptr_dispose_filter_t)(unified_filter_handle_p);
typedef unified_filter_error_t (*funct_ptr_enable_ruleset_t)(unified_filter_handle_p, int , int);
typedef unified_filter_error_t (*funct_ptr_print_rule_list_t)(unified_filter_handle_p, unified_rule_list_t* , FILE*);
typedef unified_filter_error_t (*funct_ptr_configure_filter_t) (unified_filter_handle_p handle, unified_filter_param_t *param);



/** \defgroup UNIFIED_FILTER_INTERFACE_FUNCTIONS
 */
/*@{*/
/** 
 * @brief       creates the appropriate filter and returns the handle to the filter.
 *
 * @param[in] param     the structure unified_filter_param_t;
 * @return              handle to the filter created.
 * 
 */

unified_filter_handle_p filter_factory_get_filter(unified_filter_param_t *param);
/** 
 * @brief               parses the specified rule file.
 *
 * @param[in] handle     the structure unified_filter_handle_p - handle to the filter;
 * @param[in] filename   filename - name of the file to be parsed;
 * @param[in] list       list - the generic rule list structure;
 * @return               the last error code generated.
 * 
 */

unified_filter_error_t parse_rule_file(unified_filter_handle_p handle, char* filename,unified_rule_list_t *list);
/** 
 * @brief               encodes the rules to the TCAM format and write into the TCAM.
 *
 * @param[in] handle    the structure unified_filter_handle_p - handle to the filter;
 * @param[in] list      list - the generic rule list structure;
 * @return              the last error code generated.
 * 
 */

unified_filter_error_t write_rules(unified_filter_handle_p handle, unified_rule_list_t *list);
/** 
 * @brief               to cleanup the rules in the list if any.
 *
 * @param[in] handle    handle to the filter;
 * @param[in] list      the generic rule list structure;
 * @return              the last error code generated.
 * 
 */

unified_filter_error_t cleanup_rules(unified_filter_handle_p handle, unified_rule_list_t *list);
/** 
 * @brief               to dispose the filter handle once everything with that is done.
 *
 * @param[in] handle    handle to the filter;
 * @return              the last error code generated.
 * 
 */

unified_filter_error_t dispose_filter(unified_filter_handle_p handle);
/** 
 * @brief               enablese the ruleset for the specified interface and ruleset.
 *
 * @param[in] handle     handle to the filter;
 * @param[in] ruleset    the ruleset which will be enabled
 * @param[in] interface  the interface which will be enabled.
 * @return               the last error code generated.
 * 
 */

unified_filter_error_t enable_ruleset(unified_filter_handle_p handle,int ruleset, int iface);
/** 
 * @brief               displays the parsed rule file.mostly for debugging/verification.
 *
 * @param[in] handle      handle to the filter;
 * @param[in] in_list     the generic rule list structure;
 * @param[in] out_stream  the file to which the parsed file is displayed.
 * @return                the last error code generated.
 * 
 */
unified_filter_error_t print_rule_list(unified_filter_handle_p handle,unified_rule_list_t *in_list, FILE* out_stream);
/** 
 * @brief                 to configure the filter classifier.
 *
 * @param[in] handle      handle to the filter;
 * @param[in] param       the structure unified_filter_param_t;
 * @return                the last error code generated.
 * 
 */
unified_filter_error_t configure_filter(unified_filter_handle_p handle,unified_filter_param_t *param);
/** 
 * @brief                 to read from TCAM and verify the rules.
 *
 * @param[in] handle      handle to the filter;
 * @param[in] list        the structure unified_filter_param_t;
 * @return                the last error code generated.
 * 
 */

unified_filter_error_t read_verify_rules(unified_filter_handle_p handle, unified_rule_list_t *list);
/** 
 * @brief                 returns the filter type.
 *
 * @param[in] handle      handle to the filter;
 * @return                the last error code generated.
 * 
 */
unified_filter_type_t get_filter_type(unified_filter_handle_p handle);


/*@}*/
/*@}*/
#endif //UNIFIED_FILTER_INTERFACE_H
