/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_meta.c,v 1.4 2006/03/28 04:24:01 ben Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         df_meta.c
* DESCRIPTION:  dagixp_filter API functions used to get and set the meta-data
*               of the filters.
*
*
* HISTORY:      15-12-05 Initial revision
*
**********************************************************************/

/* System headers */
#include "dag_platform.h"


/* Endace headers */
#include "dagixp_filter.h"
#include "dagutil.h"
#include "df_types.h"
#include "df_internal.h"





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_priority_reorder
 
 DESCRIPTION:   Reorders the ruleset based on the new priority of a rule.
                The function assumes that the list is already order from
                highest priority to lowest priority.
                
 
 PARAMETERS:    rule_h           IN       The rule that is going to have its
                                          changed and therefore the list needs
                                          needs to be reordered.
                new_priority     IN       The new priority of the above rule.

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
dagixp_filter_priority_reorder (ListPtr list, RuleH rule_h, uint16_t new_priority)
{
	df_rule_meta_t* entry_p;
	IteratorPtr     iterator_target_p;
	IteratorPtr     iterator_source_p;
	IteratorPtr     iterator_p;
	uint32_t        count;
	uint32_t        advance = 0;
	
	
	
	
	/* check if we need to do any sorting at all */
	count = adt_list_items (list);
	if ( count < 2 )
		return;
	
	
	
	
	
	
	/* find the current iterator for the rule */
	iterator_source_p = adt_list_get_iterator (list);
	adt_iterator_find (iterator_source_p, rule_h);
	
	if ( !adt_iterator_is_in_list(iterator_source_p) )
		return;
	
	
	/* find the place where the item should go (avoid the duplicate item) */
	iterator_target_p = adt_list_get_iterator (list);
	adt_iterator_first (iterator_target_p);
	
	while (adt_iterator_is_in_list(iterator_target_p))
	{
		if ( adt_iterator_is_equal(iterator_target_p, iterator_source_p) )
		{
			advance = 1;
		}
		
		entry_p = (df_rule_meta_t*) adt_iterator_retrieve (iterator_target_p);
		if ( new_priority <= entry_p->priority )
			break;
		
		adt_iterator_advance (iterator_target_p);
	}
	
	
	/* if ran off the end of the list we should swap with the last element */
	if ( !adt_iterator_is_in_list(iterator_target_p) )
		adt_iterator_last(iterator_target_p);


	
	
	
	
	/* the iterator pointer is now set to the target position, to move the
	 * item to the correct index we swap the items until we get to the right position */
	iterator_p = adt_list_get_iterator (list);
	adt_iterator_first (iterator_p);
	
	
	/* check for the special case where the position hasn't changed */
	if ( adt_iterator_is_equal(iterator_source_p, iterator_target_p) )
	{
		/* do nothing */
	}
	
	else if ( advance == 1 )
	{
		adt_iterator_retreat (iterator_target_p);
		
		adt_iterator_equate (iterator_p, iterator_source_p);
		adt_iterator_advance (iterator_p);

		while ( !adt_iterator_is_equal(iterator_source_p, iterator_target_p) )
		{
			adt_iterator_swap (iterator_p, iterator_source_p);
			adt_iterator_advance (iterator_p);
			adt_iterator_advance (iterator_source_p);
		}
	}

	else
	{
		adt_iterator_equate (iterator_p, iterator_source_p);
		adt_iterator_retreat (iterator_p);
		
		while ( !adt_iterator_is_equal(iterator_source_p, iterator_target_p) )
		{
			adt_iterator_swap (iterator_p, iterator_source_p);
			adt_iterator_retreat (iterator_p);
			adt_iterator_retreat (iterator_source_p);
		}
	}
	
	
	/* clean up */
	adt_iterator_dispose (iterator_p);

	adt_iterator_dispose (iterator_target_p);
	adt_iterator_dispose (iterator_source_p);
	
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_rule_priority
                dagixp_filter_get_rule_priority
 
 DESCRIPTION:   Gets and sets the priority of the specified filter.
 
 PARAMETERS:    rule_handle    IN       Handle to the filter, that
                                          will have the priority set
                                          or returned.
                priority         IN/OUT   The priority to set or the
                                          variable to put the priority
                                          in.

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_rule_priority (RuleH rule_h, uint16_t priority)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	df_ruleset_t*   ruleset_p;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	ruleset_p = (df_ruleset_t*) meta_p->ruleset;
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	/* now we have to reorder the ruleset list to make sure the rules are in priority order */
	dagixp_filter_priority_reorder (ruleset_p->rule_list, rule_h, priority);


	/* set the new priority */
	meta_p->priority = priority;
	
	
	
	
	return 0;
}

int
dagixp_filter_get_rule_priority (RuleH rule_h, uint16_t *priority)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	if ( priority == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	*priority = meta_p->priority;

	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_rule_action
                dagixp_filter_get_rule_action
 
 DESCRIPTION:   Gets and sets the action of the specified filter.
 
 PARAMETERS:    rule_handle    IN       Handle to the filter, that
                                          will have the priority set
                                          or returned.
                action           IN/OUT   The action to set or the
                                          variable to put the action
                                          in.

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_rule_action (RuleH rule_h, action_t action)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	

	meta_p->action = (action == kAccept) ? 1 : 0;

	return 0;
}


int
dagixp_filter_get_rule_action (RuleH rule_h, action_t *action)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	if ( action == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	*action = (meta_p->action == 1) ? kAccept : kReject;
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_rule_snap_length
                dagixp_filter_get_rule_snap_length
 
 DESCRIPTION:   Gets and sets the snap length of the specified filter.
 
 PARAMETERS:    rule_handle    IN       Handle to the filter, that
                                          will have the snap length set
                                          or returned.
                priority         IN/OUT   The priority to set or the
                                          variable to put the snap length
                                          in.

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_rule_snap_length (RuleH rule_h, uint16_t snap_length)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* round down to the nearest 64-bit aligned value */
	if ( (snap_length & 0x7) != 0 )
		snap_length &= 0xFFF8;
	
	/* snap lengths must be at least 24 bytes */
	if ( snap_length < 24 )
		snap_length = 24;
		
	meta_p->snap_len = snap_length;
	return 0;
}


int
dagixp_filter_get_rule_snap_length (RuleH rule_h, uint16_t *snap_length)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	if ( snap_length == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	*snap_length = meta_p->snap_len;
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_rule_steering
                dagixp_filter_get_rule_steering
 
 DESCRIPTION:   Gets and sets the steering of the specified filter.
 
 PARAMETERS:    rule_handle    IN       Handle to the filter, that
                                          will have the steering set
                                          or returned.
                steering         IN/OUT   The steering to set or the
                                          variable to put the steering
                                          in.

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_rule_steering (RuleH rule_h, steering_t steering)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	meta_p->steering = (steering == kHost) ? 0 : 1;
	return 0;
}


int
dagixp_filter_get_rule_steering (RuleH rule_h, steering_t *steering)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	if ( steering == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	*steering = (meta_p->steering == 1) ? kLine : kHost;
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_rule_tag
                dagixp_filter_get_rule_tag
 
 DESCRIPTION:   Gets and sets the tag of the specified rule.
 
 PARAMETERS:    rule_handle    IN       Handle to the filter, that
                                          will have the tag set
                                          or returned.
                priority         IN/OUT   The tag to set or the
                                          variable to put the tag
                                          in.

 RETURNS:       0 if no error occuried, otherwise -1.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_rule_tag (RuleH rule_h, uint16_t tag)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	meta_p->rule_tag = (tag & 0x3FFF);
	return 0;
}


int
dagixp_filter_get_rule_tag (RuleH rule_h, uint16_t *tag)
{
	df_rule_meta_t* meta_p = (df_rule_meta_t*) rule_h;
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	if ( tag == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	*tag = meta_p->rule_tag;
	return 0;
}
