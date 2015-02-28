/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


#ifndef CARD_INITIALIZATION_H
#define CARD_INITIALIZATION_H

/* Public API headers. */
#include "dag_config.h"

/* Internal project headers. */
#include "../card_types.h"

/* C Standard Library headers. */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


	/* Factory initialization routine. */
	dag_err_t initialize_card(dag_card_t card_type, DagCardPtr card);
	
	/* Per-card initialization routine declarations, all collected in one place. */
	dag_err_t dag37t_initialize(DagCardPtr card);
	dag_err_t dag43ge_initialize(DagCardPtr card);
	dag_err_t dag71s_initialize(DagCardPtr card);
	dag_err_t dag37ge_initialize(DagCardPtr card);
	dag_err_t dag38s_initialize(DagCardPtr card);
    dag_err_t dag45g_initialize(DagCardPtr card);
    dag_err_t dag70ge_initialize(DagCardPtr card);
    dag_err_t dag43s_initialize(DagCardPtr card);
    dag_err_t dag62se_initialize(DagCardPtr card);
    dag_err_t dag37d_initialize(DagCardPtr card);
    dag_err_t dag82x_initialize(DagCardPtr card);
    dag_err_t dagx_initialize(DagCardPtr card);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CARD_INITIALIZATION_H */
