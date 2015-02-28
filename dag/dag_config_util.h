/*
 * Copyright (c) 2006 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */

#ifndef DAG_CONFIG_UTIL_H
#define DAG_CONFIG_UTIL_H


#include "dag_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \defgroup ConfigAndStatusAPI Configuration and Status API
 * The interface that exposes the Components and Attributes that configures the card..
 */
/*@{*/
/**
 * \defgroup ConfigUtil Configuration Utility
 * */
/*@{*/

/**
  Used to configure the memory allocation of the card as a whole.
     \param card_ref card reference
     \param  text e.g. "mem=32:32"
     \return 0 if success -1 if error 
  */
int dag_config_util_domemconfig(dag_card_ref_t card, const char* text);

/**
 * Utility function to configure an attribute.
 * \param card_ref card reference
 * \param root_component root component
 * \param component_code owner of attribute
 * \param component_index index of component
 * \param attribute_code attribute to configure
 * \param lval new attribute value
 */
void dag_config_util_set_value_to_lval(dag_card_ref_t card_ref, dag_component_t root_component,
                                    dag_component_code_t component_code, int component_index, dag_attribute_code_t attribute_code, int lval);

/**
 * Utility function to configure an attribute which also includes a read back to verify that an attribute was correctly configured. If not, a warning message is printed.
 * \param card_ref card reference
 * \param root_component root component
 * \param component_code owner of attribute
 * \param component_index index of component
 * \param attribute_code attribute to configure
 * \param lval new attribute value
 * \param token_name name of token being set
 */
void dag_config_util_set_value_to_lval_verify(dag_card_ref_t card_ref, dag_component_t root_component,
                                    dag_component_code_t component_code, int component_index, dag_attribute_code_t attribute_code, int lval, char *token_name);

/* used to configure the hlb association table of the card as a whole
 * \param card_ref card reference
 * \param text e.g. "hlb=0-10.1:10.1-30.0:40-80
 * \return 0 if success -1 if error
 */
int dag_config_util_do_hlb_config(dag_card_ref_t card, const char* text);

/**
 * Set value of attribute to that of value_p.
 *
 * value_p can point to all number-like attribute values but not
 * strings or structures.
 *
 * \param card_ref card reference
 * \param uuid attrubite uuid
 * \param value_p pointer to the value to be set
 */
void dag_config_util_set_attribute(dag_card_ref_t card_ref, attr_uuid_t uuid, void* value_p);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DAG_CONFIG_UTIL_H */
/*@}*/
/*@}*/
