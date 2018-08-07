/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_PM_EVCORR_H
#define __NX_PM_EVCORR_H

#include "../../../common/types.h"
#include "../../../common/dlist.h"


typedef enum nx_pm_evcorr_rule_type_t
{
    NX_PM_EVCORR_RULE_TYPE_SIMPLE = 1,
    NX_PM_EVCORR_RULE_TYPE_SUPPRESSED,
    NX_PM_EVCORR_RULE_TYPE_PAIR,
    NX_PM_EVCORR_RULE_TYPE_ABSENCE,
    NX_PM_EVCORR_RULE_TYPE_THRESHOLDED,
    NX_PM_EVCORR_RULE_TYPE_STOP,
} nx_pm_evcorr_rule_type_t;


typedef struct nx_pm_evcorr_match_time_list_t nx_pm_evcorr_match_time_list_t;
NX_DLIST_HEAD(nx_pm_evcorr_match_time_list_t, nx_pm_evcorr_match_time_t);

typedef struct nx_pm_evcorr_rule_t nx_pm_evcorr_rule_t ;

typedef struct nx_pm_evcorr_rule_absence_data_t
{
    nx_pm_evcorr_rule_t	*rule;  ///< back pointer to the rule
    nx_event_t		*event; ///< event which will fire with NX_EVENT_TYPE_MODULE_SPECIFIC
    apr_time_t		matched; ///< last time when TriggerCondition evaluated to true
} nx_pm_evcorr_rule_absence_data_t;


typedef struct nx_pm_evcorr_rule_thresholded_data_t
{
    size_t			num_match; ///< current number of matches stored in matchlist
    size_t			matchlist_size; //< size of matchlist
    apr_time_t			*matchlist; ///< list of matched times
} nx_pm_evcorr_rule_thresholded_data_t;



struct nx_pm_evcorr_rule_t
{
    NX_DLIST_ENTRY(nx_pm_evcorr_rule_t) link;
    nx_pm_evcorr_rule_type_t type;
    union
    {
	struct simple
	{
	    nx_expr_statement_list_t	*exec;	///< Statement blocks to execute
	} simple;

	/* match input event and execute an action list, but ignore the following
	 * matching events for the next t seconds.*/
	struct suppressed
	{
	    apr_time_t			matched; ///< last time when condition evaluated to true, without context
	    nx_expr_t			*context_expr;
	    apr_hash_t			*context; ///< hash to store context keys with values (matched)
	    nx_expr_t			*cond;
	    int64_t			interval;
	    nx_expr_statement_list_t	*exec;	///< Statement blocks to execute
	} suppressed;

	/* If TriggerCondition is true, wait Interval seconds for RequiredCondition to be true
	 * and then do the Exec. If Interval is 0, there is no window on matching. */
	struct pair
	{
	    apr_time_t			matched; ///< last time when TriggerCondition evaluated to true, without context
	    nx_expr_t			*context_expr;
	    apr_hash_t			*context; ///< hash to store context keys with values (matched)
	    nx_expr_t			*triggercond;
	    nx_expr_t			*requiredcond;
	    int64_t			interval;
	    nx_expr_statement_list_t	*exec;	///< Statement blocks to execute
	} pair;

	/* If TriggerCondition is true, wait Interval seconds for RequiredCondition to be true. 
	 * If RequiredCondition does not become true within the specified interval then do the Exec*/
	struct absence
	{
	    nx_expr_t			*context_expr;
	    apr_hash_t			*context; ///< hash to store context keys with values (absence_data)
	    nx_expr_t			*triggercond;
	    nx_expr_t			*requiredcond;
	    int64_t			interval;
	    nx_expr_statement_list_t	*exec;	///< Statement blocks to execute
	    nx_pm_evcorr_rule_absence_data_t data; ///< used without a context
	} absence;

	/* If the number of events exceeed the given threshold within the interval do the Exec */
	struct thresholded
	{
	    nx_expr_t			*cond;
	    nx_expr_t			*context_expr;
	    apr_hash_t			*context; ///< hash to store context keys with values (thresholded_data)
	    size_t			threshold;
	    int64_t			interval;
	    nx_expr_statement_list_t	*exec;	///< Statement blocks to execute
	    nx_pm_evcorr_rule_thresholded_data_t data; ///< used without a context
	} thresholded;

	/* Stop processing the rules when the condition matches. Subsequent rules will not
	   be checked. */
	struct stop
	{
	    nx_expr_t			*cond;
	    nx_expr_statement_list_t	*exec;	///< Statement blocks to execute
	} stop;

    };
};


typedef struct nx_pm_evcorr_rule_list_t nx_pm_evcorr_rule_list_t;
NX_DLIST_HEAD(nx_pm_evcorr_rule_list_t, nx_pm_evcorr_rule_t);


typedef struct nx_pm_evcorr_conf_t
{
    const char			*timefield;
    apr_time_t			timeval; ///< either current time or value of the timefield updated by get_timeval()
    nx_pm_evcorr_rule_list_t	*rules;
    int64_t			context_cleantime; ///< interval for context cleanup
    nx_event_t			*cleanup_event; ///< event for context cleanup
} nx_pm_evcorr_conf_t;



#endif	/* __NX_PM_EVCORR_H */
