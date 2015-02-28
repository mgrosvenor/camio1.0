/*
 * Copyright (c) 2005 Endace Technology Ltd, Hamilton, New Zealand.
 * All rights reserved.
 *
 * This source code is proprietary to Endace Technology Limited and no part
 * of it may be redistributed, published or disclosed except as outlined in
 * the written contract supplied with this product.
 *
 */


/* File header. */
#include "../include/cards/card_initialization.h"

/* Public API headers. */

/* Internal project headers. */
#include "../include/card_types.h"

/* C Standard Library headers. */
#include <assert.h>


/* Factory initialization routine. */
dag_err_t
initialize_card(dag_card_t card_type, DagCardPtr card)
{
	assert(kFirstDagCard <= card_type);
	assert(card_type <= kLastDagCard);

	switch (card_type)
	{
		case kDag37t:
            		return dag37t_initialize(card);

		case kDag37t4:
			return dag37t_initialize(card);

		case kDag43ge:
            		return dag43ge_initialize(card);
        
        	case kDag71s:
            		return dag71s_initialize(card);

		case kDag37ge:
            		return dag37ge_initialize(card);

        	case kDag70ge:
            		return dag70ge_initialize(card);

        	case kDag37d:
            		return dag37d_initialize(card);

        	case kDag43s:
            		return dag43s_initialize(card);

		case kDag60:
		case kDag61:
		case kDag62:
            		return dag62se_initialize(card);
		case kDag70s:
			return kDagErrCardNotSupported;
			break;
	
		default:
			return dagx_initialize(card);
			break;
	}
	
	return kDagErrCardNotSupported;
}
