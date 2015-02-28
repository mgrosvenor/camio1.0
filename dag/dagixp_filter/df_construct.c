/**********************************************************************
*
* Copyright (c) 2004-2005 Endace Technology Ltd, Hamilton, New Zealand.
* All rights reserved.
*
* This source code is proprietary to Endace Technology Limited and no part
* of it may be redistributed, published or disclosed except as outlined in
* the written contract supplied with this product.
*
* $Id: df_construct.c,v 1.6 2006/09/19 07:14:29 vladimir Exp $
*
**********************************************************************/

/**********************************************************************
* FILE:         df_construct.c
* DESCRIPTION:  dagixp_filter API functions used to create and manage the 
*               collection of rules.
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
 
 CONSTANTS

---------------------------------------------------------------------*/




/*--------------------------------------------------------------------
 
 GLOBALS

---------------------------------------------------------------------*/






/*--------------------------------------------------------------------
 
 MACROS

---------------------------------------------------------------------*/

#if defined(_WIN32)

	#define INADDR4_TO_ARRAY(array, inaddr)    \
		{ array[0] = inaddr->S_un.S_un_b.s_b1; \
		  array[1] = inaddr->S_un.S_un_b.s_b2; \
		  array[2] = inaddr->S_un.S_un_b.s_b3; \
		  array[3] = inaddr->S_un.S_un_b.s_b4; }

	#define ARRAY_TO_INADDR4(inaddr, array)    \
		{ inaddr->S_un.S_un_b.s_b1 = array[0]; \
		  inaddr->S_un.S_un_b.s_b2 = array[1]; \
		  inaddr->S_un.S_un_b.s_b3 = array[2]; \
		  inaddr->S_un.S_un_b.s_b4 = array[3]; }

	#define INADDR6_TO_ARRAY(array, inaddr)   \
		{ int i;                              \
		  for (i=0; i<16; i++)                \
		    array[i] = inaddr->u.Byte[i];     \
		}

	#define ARRAY_TO_INADDR6(inaddr, array)   \
		{ int i;                              \
		  for (i=0; i<16; i++)                \
		    inaddr->u.Byte[i] = array[i];     \
		}

#else 
		  
	#define INADDR4_TO_ARRAY(array, inaddr)    \
		{ uint32_t addr = ntohl(inaddr->s_addr);        \
		  array[0] = (uint8_t) ((addr >> 24) & 0xFF);   \
		  array[1] = (uint8_t) ((addr >> 16) & 0xFF);   \
		  array[2] = (uint8_t) ((addr >>  8) & 0xFF);   \
		  array[3] = (uint8_t) ((addr      ) & 0xFF);   }

	#define ARRAY_TO_INADDR4(inaddr, array)    \
		{ inaddr->s_addr  = ((uint32_t)array[0] << 24); \
		  inaddr->s_addr |= ((uint32_t)array[1] << 16); \
		  inaddr->s_addr |= ((uint32_t)array[2] <<  8); \
		  inaddr->s_addr |= ((uint32_t)array[3]      ); \
		  inaddr->s_addr  = htonl(inaddr->s_addr);      }

	#define INADDR6_TO_ARRAY(array, inaddr)   \
		{ int i;                              \
		  for (i=0; i<16; i++)                \
		    array[i] = inaddr->s6_addr[i];    \
		}

	#define ARRAY_TO_INADDR6(inaddr, array)   \
		{ int i;                              \
		  for (i=0; i<16; i++)                \
		    inaddr->s6_addr[i] = array[i];    \
		}

#endif




/*--------------------------------------------------------------------
 
 FUNCTION:      prv_dispose_icmp_type_rule
 
 DESCRIPTION:   Called by the ADT library when a icmp type rule is being removed
                from a ip rule icmp type list.
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
prv_dispose_icmp_type_rule (AdtPtr data)
{
	free(data);	
}



/*--------------------------------------------------------------------
 
 FUNCTION:      prv_dispose_port_rule
 
 DESCRIPTION:   Called by the ADT library when a port filter is being removed
                from a ip rule port list.
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
prv_dispose_port_rule (AdtPtr data)
{
	free(data);	
}



/*--------------------------------------------------------------------
 
 FUNCTION:      prv_dispose_rule
 
 DESCRIPTION:   Called by the ADT library when a rule is being removed
                from a ruleset list.
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
prv_dispose_rule (AdtPtr data)
{
	df_rule_meta_t * meta_p;
	
	
	/* grab the signature of the list item to determine how to delete the rule */
	meta_p = (df_rule_meta_t*) data;
	switch (meta_p->signature)
	{
		
		/* Dispose of the ipv4 rule */
		case SIG_IPV4_RULE: {
			df_ipv4_rule_t* ipv4_rule_p = (df_ipv4_rule_t*) data;
			
			if (ipv4_rule_p->port_rules)
				adt_array_list_dispose(ipv4_rule_p->port_rules);
			
			if (ipv4_rule_p->icmp_type_rules)
				adt_array_list_dispose(ipv4_rule_p->icmp_type_rules);

			free (data);
			break;
		}
		
		
		/* Dispose of the ipv6 rule */
		case SIG_IPV6_RULE: {
			df_ipv6_rule_t* ipv6_rule_p = (df_ipv6_rule_t*) data;
			
			if (ipv6_rule_p->port_rules)
				adt_array_list_dispose(ipv6_rule_p->port_rules);
			
			if (ipv6_rule_p->icmp_type_rules)
				adt_array_list_dispose(ipv6_rule_p->icmp_type_rules);

			free (data);
			break;
		}
		
		
		/* Unknown structure being disposed */
		default:
			dagutil_warning ("dispose routine called on unknown object.\n");	
			break;
	}
	
}









/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_create_ruleset
 
 DESCRIPTION:   Creates a new ruleset, returns the handle to the ruleset
                if successiful, otherwise NULL.
 
 PARAMETERS:    none

 RETURNS:       The handle to the new ruleset or if an error occuired
                NULL.

 HISTORY:       

---------------------------------------------------------------------*/
RulesetH
dagixp_filter_create_ruleset (void)
{
	df_ruleset_t*	ruleset_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return NULL;
	}


	/* allocate the ruleset */
	ruleset_p = malloc(sizeof(df_ruleset_t));
	if (ruleset_p == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
	
	memset (ruleset_p, 0, sizeof(df_ruleset_t));
	ruleset_p->signature = SIG_RULESET;
	
	
	/* create the empty rule list for this ruleset */
	ruleset_p->rule_list = adt_list_init(kListInterfaceIterator, kListRepresentationDynamic, prv_dispose_rule);
	if ( ruleset_p->rule_list == NULL )
	{
		free (ruleset_p);
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
		
	
	
	return (RulesetH)ruleset_p;
}


	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_delete_ruleset
 
 DESCRIPTION:   Deletes a ruleset created by the dagixp_filter_create_ruleset
                function. Upon return the ruleset handle should no longer
                be used, doing so will result in unpredictable behaviour.
 
 PARAMETERS:    ruleset     IN      handle returned by dagixp_filter_create_ruleset
 
 RETURNS:       -1 if an error occuried, otherwise 0. If returned -1
                call dagixp_filter_get_last_error to get the error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_delete_ruleset (RulesetH ruleset_h)
{
	df_ruleset_t* ruleset_p = (df_ruleset_t*) (ruleset_h);
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	/* delete the rule list */
	if ( ruleset_p->rule_list )
		adt_list_dispose (ruleset_p->rule_list);
	
	/* free the memory */
	free (ruleset_p);
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_empty_ruleset
 
 DESCRIPTION:   Removes all the rules from a ruleset.
 
 PARAMETERS:    ruleset     IN      handle returned by dagixp_filter_create_ruleset
 
 RETURNS:       -1 if an error occuried, otherwise 0. If returned -1
                call dagixp_filter_get_last_error to get the error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_empty_ruleset (RulesetH ruleset_h)
{
	df_ruleset_t*	ruleset_p = (df_ruleset_t*) (ruleset_h);
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	/* empty the rule list */
	if ( ruleset_p->rule_list )
		adt_list_make_empty (ruleset_p->rule_list);
	
	return 0;
}	
	



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_ruleset_filter_count
 
 DESCRIPTION:   Returns the number of the rules in the ruleset
 
 PARAMETERS:    ruleset     IN      handle returned by dagixp_filter_create_ruleset
 
 RETURNS:       -1 if an error occuried, otherwise a positive number
                indicating the number of rules in the ruleset. If 
                returned -1 call dagixp_filter_get_last_error to get the 
                error code.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_ruleset_rule_count (RulesetH ruleset_h)
{
	df_ruleset_t* ruleset_p = (df_ruleset_t*) (ruleset_h);
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* get a count of the number of rules */
	if ( ruleset_p->rule_list )
		return adt_list_items (ruleset_p->rule_list);
	else
		return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_ruleset_get_rule_at
 
 DESCRIPTION:   Returns the handle to a rule at a specified index,
                if the index or ruleset is invalid NULL is returned.
 
 PARAMETERS:    ruleset_h   IN      handle returned by dagixp_filter_create_ruleset
                index       IN      the index of the of the rule to
                                    get.
 
 RETURNS:       NULL if an error occuried, otherwise a handle to the
                rule at the index. If an error occuried use 
                dagixp_filter_get_last_error() to retreive the error code.

 HISTORY:       

---------------------------------------------------------------------*/
RuleH
dagixp_filter_ruleset_get_rule_at (RulesetH ruleset_h, uint32_t index)
{
	df_ruleset_t* ruleset_p = (df_ruleset_t*) (ruleset_h);
	RuleH         rule_h;
	uint32_t      count;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return NULL;
	}


	/* sanity checking */
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return NULL;
	}
	

	/* check the filter list has been created */
	if ( ruleset_p->rule_list == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
	
	/* check the index is not off the end of the array */
	count = adt_list_items (ruleset_p->rule_list);
	if ( index >= count )
	{
		dagixp_filter_set_last_error (EINVAL);
		return NULL;
	}
	
	
	/* get the item at the index (adt list indicies start at 1 not 0) */
	rule_h = (RuleH) adt_list_get_indexed_item (ruleset_p->rule_list, (index + 1));
	if (rule_h == NULL)
		dagixp_filter_set_last_error (EINVAL);
		
	return rule_h;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_remove_rule
 
 DESCRIPTION:   Removes a rule from a ruleset given the rule handle,
                if this function returns a successiful error code the
                rule_h is no longer valid, using it will result in
                unpredicable behaviour.
 
 PARAMETERS:    ruleset       IN    handle returned by dagixp_filter_create_ruleset
                rule_h        IN    the index of the of the rule to
                                    get.
 
 RETURNS:       -1 if an error occuried, otherwise a positive number
                indicating the number of rules in the ruleset. If 
                returned -1 call dagixp_filter_get_last_error to get the 
                error code.

 HISTORY:       

---------------------------------------------------------------------*/
extern int
dagixp_filter_remove_rule (RulesetH ruleset_h, RuleH rule_h)
{
	df_ruleset_t* ruleset_p = (df_ruleset_t*) (ruleset_h);
	IteratorPtr   iterator_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}
	
	/* sanity checking */
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	/* check the rule list has been created */
	if ( ruleset_p->rule_list == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	iterator_p = adt_list_get_iterator(ruleset_p->rule_list);
	adt_iterator_find (iterator_p, rule_h);
	if ( (iterator_p == NULL) || !adt_iterator_is_in_list(iterator_p) )
	{
		adt_iterator_dispose (iterator_p);
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	adt_iterator_remove (iterator_p);
	adt_iterator_dispose (iterator_p);
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      prv_insert_rule
 
 DESCRIPTION:   Inserts a rule into the correct location with the rule
                list based on it's priority.
 
 PARAMETERS:    data            IN      Pointer to the entry being removed

 RETURNS:       nothing

 HISTORY:       

---------------------------------------------------------------------*/
static void
prv_insert_rule (ListPtr list, df_rule_meta_t* meta_p)
{
	IteratorPtr     iterator_p;
	IteratorPtr     insert_point_p;
	df_rule_meta_t* cur_meta_p;


	/* create two new iterators one that shadows the other */
	iterator_p = adt_list_get_iterator (list);
	insert_point_p = adt_list_get_iterator (list);

	adt_iterator_first (iterator_p);
	adt_iterator_zeroth (insert_point_p);
	

	while (adt_iterator_is_in_list(iterator_p))
	{
		cur_meta_p = (df_rule_meta_t*) adt_iterator_retrieve (iterator_p);
		if ( cur_meta_p->priority > meta_p->priority )
			break;
			
		adt_iterator_advance (insert_point_p);
		adt_iterator_advance (iterator_p);
	}
		
		
	
	adt_iterator_add (insert_point_p, meta_p);
	
	adt_iterator_dispose (iterator_p);
	adt_iterator_dispose (insert_point_p);
	
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_create_ipv4_rule
 
 DESCRIPTION:   Creates a blank rule (accepts or rejects all packets
                based on the value of the action argument) and attaches
                it to a ruleset. Every argument bar the ruleset can
                be changed after the rule is created by calling the
                appropriate rule meta data function.
 
 PARAMETERS:    ruleset     IN      handle to the ruleset to add the
                                    new rule to.
                action      IN      The action of the new rule, either
                                    accept or reject.
                rule_tag    IN      The rule tag, packets accepted by
                                    this rule, have the tag value copied
                                    into the ERF header.
                priority    IN      The priority of the rule, rules
                                    with the lowest value for the priority
                                    are applied first.
 
 RETURNS:       NULL if an error occuried, otherwise a handle to the new
                rule.

 NOTES:         The new rule will have the following default parameters
                stream:    0
                steering:  host
                snap_len:  65535
                
 
 HISTORY:       

---------------------------------------------------------------------*/
RuleH
dagixp_filter_create_ipv4_rule (RulesetH ruleset_h, action_t action, uint16_t rule_tag, uint16_t priority)
{
	df_ruleset_t*     ruleset_p = (df_ruleset_t*) (ruleset_h);
	df_ipv4_rule_t*   rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return NULL;
	}

	

	/* sanity checking */
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return NULL;
	}
	
	/* if the filter list has not yet been created return an error */
	if ( ruleset_p->rule_list == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
	
	
	/* create the rule ... */
	rule_p = (df_ipv4_rule_t*) malloc(sizeof(df_ipv4_rule_t));
	if ( rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
	
	
	/* ... populate the rule structure ... */
	memset (rule_p, 0, sizeof(df_ipv4_rule_t));
	rule_p->meta.signature  = SIG_IPV4_RULE;
	
	rule_p->meta.ruleset    = ruleset_h;
	rule_p->meta.action     = (action == kAccept) ? 1 : 0;
	rule_p->meta.steering   = 0;
	rule_p->meta.snap_len   = 0xFFF8;
	rule_p->meta.rule_tag   = rule_tag & 0x3FFF;
	rule_p->meta.priority   = priority;
	
	rule_p->protocol        = kAllProtocols;
	
	
	/* ... create the port rule list and the icmp type list ... */
	rule_p->port_rules      = adt_array_list_init(prv_dispose_port_rule);
	rule_p->icmp_type_rules = adt_array_list_init(prv_dispose_icmp_type_rule);
	
	
	/* ... add to the rule list (in order) */
	prv_insert_rule(ruleset_p->rule_list, (df_rule_meta_t*)rule_p);
	
	
	return (RuleH)rule_p;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_create_ipv6_rule
 
 DESCRIPTION:   Creates a blank rule (accepts or rejects all packets
                based on the value of the action argument) and attaches
                it to a ruleset. Every argument bar the ruleset can
                be changed after the rule is created by calling the
                appropriate rule meta data function.
 
 PARAMETERS:    ruleset     IN      handle to the ruleset to add the
                                    new rule to.
                action      IN      The action of the new rule, either
                                    accept or reject.
                rule_tag    IN      Packets accepted by this rule,
                                    have the tag value copied
                                    into the ERF header.
                priority    IN      The priority of the rule, rules
                                    with the lowest value for the priority
                                    are applied first.

 RETURNS:       NULL if an error occuried, otherwise a handle to the new
                rule.

 NOTES:         The new rule will have the following default parameters
                stream:    0
                steering:  host
                snap_len:  65535
                
 
 HISTORY:       

---------------------------------------------------------------------*/
RuleH
dagixp_filter_create_ipv6_rule (RulesetH ruleset_h, action_t action, uint16_t rule_tag, uint16_t priority)
{
	df_ruleset_t*     ruleset_p = (df_ruleset_t*) (ruleset_h);
	df_ipv6_rule_t*   rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return NULL;
	}
	

	/* sanity checking */
	if ( (ruleset_p == NULL) || (ruleset_p->signature != SIG_RULESET) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return NULL;
	}
	
	/* if the rule list has not yet been created return an error */
	if ( ruleset_p->rule_list == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
		
	
	/* create the rule ... */
	rule_p = (df_ipv6_rule_t*) malloc(sizeof(df_ipv6_rule_t));
	if ( rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return NULL;
	}
	
	
	/* ... populate the rule structure ... */
	memset (rule_p, 0, sizeof(df_ipv6_rule_t));
	rule_p->meta.signature  = SIG_IPV6_RULE;
	
	rule_p->meta.ruleset    = ruleset_h;
	rule_p->meta.action     = (action == kAccept) ? 1 : 0;
	rule_p->meta.steering   = 0;
	rule_p->meta.snap_len   = 0xFFF8;
	rule_p->meta.rule_tag   = rule_tag & 0x3FFF;
	rule_p->meta.priority   = priority;

	rule_p->protocol        = kAllProtocols;
	

	/* ... create the port rule list and the icmp type list ... */
	rule_p->port_rules      = adt_array_list_init(prv_dispose_port_rule);
	rule_p->icmp_type_rules = adt_array_list_init(prv_dispose_icmp_type_rule);

	
	/* ... add to the rule list (in order) */
	prv_insert_rule(ruleset_p->rule_list, (df_rule_meta_t*)rule_p);


	
	return (RuleH)rule_p;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_rule_type
 
 DESCRIPTION:   Retreives the type of a rule, either IPv4 or IPv6.
 
 PARAMETERS:    rule_h          IN    handle to the rule to get the type of

 RETURNS:       -1 if an error occuried, otherwise one of the constants
                kIPv4RuleType or kIPv6RuleType.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_rule_type (RuleH rule_h)
{
	df_ipv4_rule_t* rule_p = (df_ipv4_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( rule_p == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	switch (rule_p->meta.signature)
	{
		case SIG_IPV4_RULE:
			return kIPv4RuleType;
		
		case SIG_IPV6_RULE:
			return kIPv6RuleType;
		
		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_ipv4_source_field
                dagixp_filter_set_ipv4_dest_field
 
 DESCRIPTION:   Sets the rule to use for the ipv4 source/destination field,
                the supplied rule handle must have been returned by
                a successiful call to dagixp_filter_create_ipv4_rule().
 
 PARAMETERS:    rule_h          IN    handle to the ipv4 rule returned
                                      by dagixp_filter_create_ipv4_rule.
                source/dest     IN    the source/destination address rule 
                                      to apply to the ip rule.

 RETURNS:       -1 if an error occuried, 

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_ipv4_source_field (RuleH rule_h, struct in_addr *src, struct in_addr *mask)
{
	df_ipv4_rule_t* rule_p = (df_ipv4_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV4_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	if ( src == NULL || mask == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	/* copy over the address and mask */
	INADDR4_TO_ARRAY (rule_p->source.addr, src);
	INADDR4_TO_ARRAY (rule_p->source.mask, mask);
	
	return 0;
}

int
dagixp_filter_set_ipv4_dest_field (RuleH rule_h, struct in_addr *dst, struct in_addr *mask)
{
	df_ipv4_rule_t* rule_p = (df_ipv4_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV4_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	if ( dst == NULL || mask == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* copy over the address and mask */
	INADDR4_TO_ARRAY (rule_p->dest.addr, dst);
	INADDR4_TO_ARRAY (rule_p->dest.mask, mask);

	
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_ipv4_source_field
                dagixp_filter_get_ipv4_dest_field
 
 DESCRIPTION:   Gets the rule to use for the ipv4 source/destination field,
                the supplied rule handle must have been returned by
                a successiful call to dagixp_filter_create_ipv4_rule or
                dagixp_filter_ruleset_get_rule_at
 
 PARAMETERS:    rule_h          IN    handle to the ipv4 rule returned
                                      by dagixp_filter_create_ipv4_rule.
                source/dest     OUT   the source/destination address rule 
                                      to apply to the ip rule.

 RETURNS:       -1 if an error occuried, 

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_ipv4_source_field (RuleH rule_h, struct in_addr *src, struct in_addr *mask)
{
	df_ipv4_rule_t* rule_p = (df_ipv4_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV4_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	/* copy over the address and mask */
	if ( src != NULL )
		ARRAY_TO_INADDR4 (src, rule_p->source.addr);
	
	if ( mask != NULL )
		ARRAY_TO_INADDR4 (mask, rule_p->source.mask);
	
	return 0;
}

int
dagixp_filter_get_ipv4_dest_field (RuleH rule_h, struct in_addr *dst, struct in_addr *mask)
{
	df_ipv4_rule_t* rule_p = (df_ipv4_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV4_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	
	
	/* copy over the address and mask */
	if ( dst != NULL )
		ARRAY_TO_INADDR4 (dst, rule_p->dest.addr);

	if ( mask != NULL )
		ARRAY_TO_INADDR4 (mask, rule_p->dest.mask);

	
	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_ipv6_source_field
                dagixp_filter_set_ipv6_dest_field
 
 DESCRIPTION:   Sets the rule to use for the ipv6 source/destination field,
                the supplied rule handle must have been returned by
                a successiful call to dagixp_filter_create_ipv6_rule().
 
 PARAMETERS:    rule_h          IN    handle to the ipv6 rule returned
                                      by dagixp_filter_create_ipv6_rule.
                source/dest     IN    the source/destination address rule 
                                      to apply to the ip rule.

 RETURNS:       -1 if an error occuried, 

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_ipv6_source_field (RuleH rule_h, struct in6_addr *src, struct in6_addr *mask)
{
	df_ipv6_rule_t *  rule_p = (df_ipv6_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV6_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	if ( src == NULL || mask == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	
	/* copy the addr and mask to the rule */
	INADDR6_TO_ARRAY (rule_p->source.addr, src);
	INADDR6_TO_ARRAY (rule_p->source.mask, mask);


	return 0;
}


int
dagixp_filter_set_ipv6_dest_field (RuleH rule_h, struct in6_addr *dst, struct in6_addr *mask)
{
	df_ipv6_rule_t  * rule_p = (df_ipv6_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV6_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	if ( dst == NULL || mask == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	
	/* copy the addr and mask to the rule */
	INADDR6_TO_ARRAY (rule_p->dest.addr, dst);
	INADDR6_TO_ARRAY (rule_p->dest.mask, mask);


	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_ipv6_source_field
                dagixp_filter_get_ipv6_dest_field
 
 DESCRIPTION:   Gets the rule to use for the ipv6 source/destination field,
                the supplied rule handle must have been returned by
                a successiful call to dagixp_filter_create_ipv6_rule or
                dagixp_filter_ruleset_get_rule_at
 
 PARAMETERS:    rule_h          IN    handle to the ipv6 rule returned
                                      by dagixp_filter_create_ipv6_rule.
                source/dest     OUT   the source/destination address rule 
                                      to apply to the ip rule.

 RETURNS:       -1 if an error occuried, 

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_ipv6_source_field (RuleH rule_h, struct in6_addr *src, struct in6_addr *mask)
{
	df_ipv6_rule_t *  rule_p = (df_ipv6_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV6_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* copy the addr and mask from the rule */
	if ( src != NULL )
		ARRAY_TO_INADDR6 (src, rule_p->source.addr);
	if ( mask != NULL )
		ARRAY_TO_INADDR6 (mask, rule_p->source.mask);


	return 0;
}


int
dagixp_filter_get_ipv6_dest_field (RuleH rule_h, struct in6_addr *dst, struct in6_addr *mask)
{
	df_ipv6_rule_t  * rule_p = (df_ipv6_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}


	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV6_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* copy the addr and mask from the rule */
	if ( dst != NULL )
		ARRAY_TO_INADDR6 (dst, rule_p->dest.addr);
	if ( mask != NULL )
		ARRAY_TO_INADDR6 (mask, rule_p->dest.mask);

	return 0;
}




/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_protocol_field
 
 DESCRIPTION:   Sets the higher-level protocol to filter on, this is not
                a bitmask filter, all 8-bits of the ip protocol field must
                match the supplied value for the rule to match.
 
 PARAMETERS:    rule_h          IN    handle to the ipv6 rule returned
                                      by dagixp_filter_create_ipv6_rule.
                protocol        IN    the protocol number to rule on.

 RETURNS:       -1 if an error occuried, 

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_protocol_field (RuleH rule_h, uint8_t protocol)
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
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}


	/* set the protocol */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			((df_ipv4_rule_t*)rule_h)->protocol = protocol;
			break;

		case SIG_IPV6_RULE:
			((df_ipv6_rule_t*)rule_h)->protocol = protocol;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_protocol_field
 
 DESCRIPTION:   Gets the higher-level protocol that is used to filter on.
 
 PARAMETERS:    rule_h          IN    handle to the ipv6 rule returned
                                      by dagixp_filter_create_ipv6_rule.
                protocol        OUT   the protocol number to filter on.

 RETURNS:       -1 if an error occuried, 

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_protocol_field (RuleH rule_h, uint8_t *protocol)
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
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	if ( protocol == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}


	/* set the protocol */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			*protocol = ((df_ipv4_rule_t*)rule_h)->protocol;
			break;

		case SIG_IPV6_RULE:
			*protocol = ((df_ipv6_rule_t*)rule_h)->protocol;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	return 0;
}


/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_add_source_port_bitmask
                dagixp_filter_add_dest_port_bitmask
 
 DESCRIPTION:   Adds a source/destination port rule to the list of 
                port rules. This function can be successifully called
                even if a protocol is not specified or not a TCP/UDP/STCP
                type protocol.
 
 PARAMETERS:    rule_h          IN    handle to either an ipv4 or ipv6 rule
                source/dest     IN    the bitmask port rule to add to
                                      the list of port rules.

 RETURNS:       0 if the port bitmask rule was added, otherwise -1

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_add_source_port_bitmask (RuleH rule_h, uint16_t port, uint16_t mask)
{
	df_rule_meta_t*   meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr      port_list;
	df_port_rule_t*   port_rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}


	/* grab the port rule list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			port_list = ((df_ipv4_rule_t*)rule_h)->port_rules;
			break;

		case SIG_IPV6_RULE:
			port_list = ((df_ipv6_rule_t*)rule_h)->port_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* check if the port rule list exists */
	if (port_list == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	
	/* add the bitmask rule onto the end */
	port_rule_p = malloc (sizeof(df_port_rule_t));
	if ( port_rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	port_rule_p->flags = DFFLAG_SOURCE | DFFLAG_BITMASK;
	port_rule_p->rule.bitmask.mask  = mask;
	port_rule_p->rule.bitmask.value = port;
	
	if ( adt_array_list_add_last(port_list, port_rule_p) == 0 )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	return 0;
}


int
dagixp_filter_add_dest_port_bitmask (RuleH rule_h, uint16_t port, uint16_t mask)
{
	df_rule_meta_t*   meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr      port_list;
	df_port_rule_t*   port_rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}


	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			port_list = ((df_ipv4_rule_t*)rule_h)->port_rules;
			break;

		case SIG_IPV6_RULE:
			port_list = ((df_ipv6_rule_t*)rule_h)->port_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* check if the port filter list exists */
	if (port_list == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	
	/* add the bitmask filter onto the end */
	port_rule_p = malloc (sizeof(df_port_rule_t));
	if ( port_rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	port_rule_p->flags = DFFLAG_DEST | DFFLAG_BITMASK;
	port_rule_p->rule.bitmask.mask  = mask;
	port_rule_p->rule.bitmask.value = port;
	
	if ( adt_array_list_add_last(port_list, port_rule_p) == 0 )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	return 0;
}
	


/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_add_source_port_range
                dagixp_filter_add_dest_port_range
 
 DESCRIPTION:   Adds a source/destination port filter to the list of 
                port filters. This function can be successifully called
                even if a protocol is not specified or not a TCP/UDP/STCP
                type protocol.
 
 PARAMETERS:    filter_handle   IN    handle to either an ipv4 or ipv6 filter
                min_port        IN    the minimum port number of the range (inclusive)
                max_port        IN    the maximum port number of the range (inclusive)

 RETURNS:       0 if the port bitmask filter was added, otherwise -1

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_add_source_port_range (RuleH rule_h, uint16_t min_port, uint16_t max_port)
{
	df_rule_meta_t*   meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr      port_list;
	df_port_rule_t*   port_rule_p;
	
	
	
	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			port_list = ((df_ipv4_rule_t*)rule_h)->port_rules;
			break;

		case SIG_IPV6_RULE:
			port_list = ((df_ipv6_rule_t*)rule_h)->port_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* check if the port filter list exists */
	if (port_list == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	/* add the bitmask filter onto the end */
	port_rule_p = malloc (sizeof(df_port_rule_t));
	if ( port_rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	port_rule_p->flags = DFFLAG_SOURCE | DFFLAG_RANGE;
	port_rule_p->rule.range.min = min_port;
	port_rule_p->rule.range.max = max_port;
	
	if ( adt_array_list_add_last(port_list, port_rule_p) == 0 )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	return 0;
}

int
dagixp_filter_add_dest_port_range (RuleH rule_h, uint16_t min_port, uint16_t max_port)
{
	df_rule_meta_t*   meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr      port_list;
	df_port_rule_t*   port_rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			port_list = ((df_ipv4_rule_t*)rule_h)->port_rules;
			break;

		case SIG_IPV6_RULE:
			port_list = ((df_ipv6_rule_t*)rule_h)->port_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* check if the port filter list exists */
	if (port_list == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	
	/* add the bitmask filter onto the end */
	port_rule_p = malloc (sizeof(df_port_rule_t));
	if ( port_rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	port_rule_p->flags = DFFLAG_DEST | DFFLAG_RANGE;
	port_rule_p->rule.range.min = min_port;
	port_rule_p->rule.range.max = max_port;
	
	if ( adt_array_list_add_last(port_list, port_rule_p) == 0 )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	return 0;
}







/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_port_list_count
 
 DESCRIPTION:   Gets the number of both source and destination port entries
                currently attached to the rules.
 
 PARAMETERS:    rule_h          IN    handle to a rule to get the number of
                                      port filters from.

 RETURNS:       -1 if an error occuried, otherwise a positive number indicating
	            the number of entires in both the source and destination port
                lists.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_port_list_count (RuleH rule_h)
{
	df_rule_meta_t*   meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr      port_list;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}	
	
	
	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			port_list = ((df_ipv4_rule_t*)rule_h)->port_rules;
			break;

		case SIG_IPV6_RULE:
			port_list = ((df_ipv6_rule_t*)rule_h)->port_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	return adt_array_list_items (port_list);
}

	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_port_list_entry
 
 DESCRIPTION:   Gets an entry in the list of both destination and source
                port filters.
 
 PARAMETERS:    rule_h          IN    handle to the rule containing the
                                      port filters.
                entry           OUT   pointer to an entry structure that
                                      is populated by this function.

 RETURNS:       -1 if an error occuried, otherwise 0

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_port_list_entry (RuleH rule_h, uint32_t index, port_entry_t * entry)
{
	df_rule_meta_t*   meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr      port_list;
	df_port_rule_t*   port_rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	if ( entry == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			port_list = ((df_ipv4_rule_t*)rule_h)->port_rules;
			break;

		case SIG_IPV6_RULE:
			port_list = ((df_ipv6_rule_t*)rule_h)->port_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* sanity check the index given */
	if ( index >= adt_array_list_items(port_list) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	
	
	/* get a pointer to the port list entry and copy the details to the user supplied structure */
	port_rule_p = (df_port_rule_t*) adt_array_list_get_indexed_item (port_list, (index + 1));
	if ( port_rule_p == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	if ( port_rule_p->flags & DFFLAG_SOURCE )
		entry->port_type = kSourcePort;
	else
		entry->port_type = kDestinationPort;
		
	
	if ( port_rule_p->flags & DFFLAG_BITMASK )
	{
		entry->rule_type = kBitmask;
		entry->data.bitmask.value = port_rule_p->rule.bitmask.value;
		entry->data.bitmask.mask  = port_rule_p->rule.bitmask.mask;
	}
	
	else
	{
		entry->rule_type = kRange;
		entry->data.range.min_port = port_rule_p->rule.range.min;
		entry->data.range.max_port = port_rule_p->rule.range.max;
	}
	
	return 0;
}





/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_add_icmp_type_bitmask
 
 DESCRIPTION:   Adds an icmp type bitmask filter to the list of icmp
                filters.
 
 PARAMETERS:    filter_handle   IN    handle to the ipv6 filter returned
                                      by dagixp_filter_create_ipv6_filter.
                type            IN    the type value and mask to filter on.

 RETURNS:       -1 if an error occuried, otherwise 0

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_add_icmp_type_bitmask (RuleH rule_h, uint8_t type, uint8_t mask)
{
	df_rule_meta_t*        meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr           icmp_list;
	df_icmp_type_rule_t*   icmp_rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}


	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			icmp_list = ((df_ipv4_rule_t*)rule_h)->icmp_type_rules;
			break;

		case SIG_IPV6_RULE:
			icmp_list = ((df_ipv6_rule_t*)rule_h)->icmp_type_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* check if the icmp filter list exists */
	if (icmp_list == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	
	/* add the bitmask filter onto the end */
	icmp_rule_p = malloc (sizeof(df_icmp_type_rule_t));
	if ( icmp_rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	icmp_rule_p->flags = DFFLAG_BITMASK;
	icmp_rule_p->rule.bitmask.mask  = mask;
	icmp_rule_p->rule.bitmask.value = type;
	
	if ( adt_array_list_add_last(icmp_list, icmp_rule_p) == 0 )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	return 0;
}



/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_add_icmp_type_range
 
 DESCRIPTION:   Adds an icmp type bitmask filter to the list of icmp
                filters.
 
 PARAMETERS:    filter_handle   IN    handle to either an ipv4 or ipv6 filter
                min_type        IN    the minimum type number of the range (inclusive)
                max_type        IN    the maximum type number of the range (inclusive)

 RETURNS:       -1 if an error occuried, otherwise 0

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_add_icmp_type_range (RuleH rule_h, uint8_t min_type, uint8_t max_type)
{
	df_rule_meta_t*        meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr           icmp_list;
	df_icmp_type_rule_t*   icmp_rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}


	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			icmp_list = ((df_ipv4_rule_t*)rule_h)->icmp_type_rules;
			break;

		case SIG_IPV6_RULE:
			icmp_list = ((df_ipv6_rule_t*)rule_h)->icmp_type_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* check if the icmp filter list exists */
	if (icmp_list == NULL)
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}
	
	
	/* add the bitmask filter onto the end */
	icmp_rule_p = malloc (sizeof(df_icmp_type_rule_t));
	if ( icmp_rule_p == NULL )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	icmp_rule_p->flags = DFFLAG_RANGE;
	icmp_rule_p->rule.range.min = min_type;
	icmp_rule_p->rule.range.max = max_type;
	
	if ( adt_array_list_add_last(icmp_list, icmp_rule_p) == 0 )
	{
		dagixp_filter_set_last_error (ENOMEM);
		return -1;
	}

	return 0;
}











/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_icmp_type_list_count
 
 DESCRIPTION:   Gets the number of both source and destination port entries
                currently attached to the rules.
 
 PARAMETERS:    rule_h          IN    handle to a rule to get the number of
                                      port filters from.

 RETURNS:       -1 if an error occuried, otherwise a positive number indicating
	            the number of entires in both the source and destination port
                lists.

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_icmp_type_list_count (RuleH rule_h)
{
	df_rule_meta_t*   meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr      icmp_type_list;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}	
	
	
	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			icmp_type_list = ((df_ipv4_rule_t*)rule_h)->icmp_type_rules;
			break;

		case SIG_IPV6_RULE:
			icmp_type_list = ((df_ipv6_rule_t*)rule_h)->icmp_type_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	return adt_array_list_items (icmp_type_list);
}

	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_icmp_type_list_entry
 
 DESCRIPTION:   Gets an entry in the list of both destination and source
                port filters.
 
 PARAMETERS:    rule_h          IN    handle to the rule containing the
                                      port filters.
                entry           OUT   pointer to an entry structure that
                                      is populated by this function.

 RETURNS:       -1 if an error occuried, otherwise 0

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_icmp_type_list_entry (RuleH rule_h, uint32_t index, icmp_entry_t * entry)
{
	df_rule_meta_t*      meta_p = (df_rule_meta_t*) rule_h;
	ArrayListPtr         icmp_type_list;
	df_icmp_type_rule_t* icmp_rule_p;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (meta_p == NULL) || !ISIPRULE_SIGNATURE(meta_p->signature) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	if ( entry == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	/* grab the port filter list */
	switch (meta_p->signature)
	{
		case SIG_IPV4_RULE:
			icmp_type_list = ((df_ipv4_rule_t*)rule_h)->icmp_type_rules;
			break;

		case SIG_IPV6_RULE:
			icmp_type_list = ((df_ipv6_rule_t*)rule_h)->icmp_type_rules;
			break;

		default:
			dagixp_filter_set_last_error (EINVAL);
			return -1;
	}
	
	
	/* saniy check the index given */
	if ( index >= adt_array_list_items(icmp_type_list) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}
	
	
	
	
	/* get a pointer to the port list entry and copy the details to the user supplied structure */
	icmp_rule_p = (df_icmp_type_rule_t*) adt_array_list_get_indexed_item (icmp_type_list, (index + 1));
	if ( icmp_rule_p == NULL )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}

	
	if ( icmp_rule_p->flags & DFFLAG_BITMASK )
	{
		entry->rule_type = kBitmask;
		entry->data.bitmask.value = icmp_rule_p->rule.bitmask.value;
		entry->data.bitmask.mask  = icmp_rule_p->rule.bitmask.mask;
	}
	
	else
	{
		entry->rule_type = kRange;
		entry->data.range.min_type = icmp_rule_p->rule.range.min;
		entry->data.range.max_type = icmp_rule_p->rule.range.max;
	}
	
	return 0;
}

	
/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_set_ipv6_flow_label_field
 
 DESCRIPTION:   
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_set_ipv6_flow_label_field (RuleH rule_h, uint32_t flow, uint32_t mask)
{
	df_ipv6_rule_t* rule_p = (df_ipv6_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV6_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}


	/* set the flow filter */
	rule_p->flow.mask = mask & 0xFFFFF;
	rule_p->flow.flow = flow & 0xFFFFF;
	return 0;
}

/*--------------------------------------------------------------------
 
 FUNCTION:      dagixp_filter_get_ipv6_flow_label_field
 
 DESCRIPTION:   
 
 PARAMETERS:    

 RETURNS:       

 HISTORY:       

---------------------------------------------------------------------*/
int
dagixp_filter_get_ipv6_flow_label_field (RuleH rule_h, uint32_t *flow, uint32_t *mask)
{
	df_ipv6_rule_t* rule_p = (df_ipv6_rule_t*) rule_h;
	

	/* check to make sure the library has been opened correctly */
	dagixp_filter_set_last_error (0);
	if (!dagixp_filter_is_initialised())
	{
		dagixp_filter_set_last_error (ENOTOPEN);
		return -1;
	}

	
	/* sanity checking */
	if ( (rule_p == NULL) || (rule_p->meta.signature != SIG_IPV6_RULE) )
	{
		dagixp_filter_set_last_error (EINVAL);
		return -1;
	}



	/* set the flow filter */
	if ( mask != NULL )
		*mask = rule_p->flow.mask;
	
	if ( flow != NULL )
		*flow = rule_p->flow.flow;
	
	return 0;
}
