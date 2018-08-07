/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/exception.h"
#include "../../../common/expr-parser.h"

#include <apr_lib.h>
#include "pm_evcorr.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define NX_PM_EVCORR_DEFAULT_CONTEXT_CLEANTIME 60 /* 60 sec */

static apr_time_t get_timeval(nx_pm_evcorr_conf_t *modconf,
			      nx_logdata_t *logdata)
{
    if ( modconf->timefield == NULL )
    {
	modconf->timeval = apr_time_now();
    }
    else
    {
	nx_value_t val;

	if ( (nx_logdata_get_field_value(logdata, modconf->timefield, &val) == TRUE) &&
	     (val.defined == TRUE) && (val.type == NX_VALUE_TYPE_DATETIME) )
	{
	    modconf->timeval = val.datetime;
	}
	else
	{
	    log_warn("TimeField '%s' not found in event, using current time",  modconf->timefield);
	    modconf->timeval = apr_time_now();
	}
    }
    
    return ( modconf->timeval );
}



static void *get_context_hashval(nx_expr_eval_ctx_t *eval_ctx,
				 apr_hash_t *context,
				 nx_expr_t *context_expr,
				 apr_size_t valsize,
				 void **hkey,
				 apr_ssize_t *hkeylen)
{
    nx_value_t contextkeyval;
    const void *key = NULL;
    void *keydupe = NULL;
    apr_ssize_t keylen;
    void *hashval = NULL;

    nx_expr_evaluate(eval_ctx, &contextkeyval, context_expr);
    if ( contextkeyval.defined == TRUE )
    {
	if ( contextkeyval.type == NX_VALUE_TYPE_STRING )
	{
	    key = contextkeyval.string->buf;
	    keylen = contextkeyval.string->len;
	}
	else if ( contextkeyval.type == NX_VALUE_TYPE_INTEGER )
	{
	    key = &(contextkeyval.integer);
	    keylen = sizeof(int64_t);
	}
	else if ( contextkeyval.type == NX_VALUE_TYPE_DATETIME )
	{
	    key = &(contextkeyval.datetime);
	    keylen = sizeof(apr_time_t);
	}
	else if ( contextkeyval.type == NX_VALUE_TYPE_BOOLEAN )
	{
	    key = &(contextkeyval.boolean);
	    keylen = sizeof(boolean);
	}
	else if ( contextkeyval.type == NX_VALUE_TYPE_IP4ADDR )
	{
	    key = &(contextkeyval.ip4addr);
	    keylen = 4;
	}
	else if ( contextkeyval.type == NX_VALUE_TYPE_IP6ADDR )
	{
	    key = &(contextkeyval.ip6addr);
	    keylen = 16;
	}
	else 
	{ 
	    keydupe = nx_value_to_string(&contextkeyval);
	    keylen = APR_HASH_KEY_STRING;
	}
	hashval = apr_hash_get(context, key, keylen);
	if ( hashval == NULL )
	{
	    hashval = calloc(1, valsize);
	    if ( keydupe == NULL )
	    {
		keydupe = malloc((size_t) keylen);
		memcpy(keydupe, key, (size_t) keylen);
	    }
	    ASSERT(hashval != NULL);
	    apr_hash_set(context, keydupe, keylen, hashval);
	}
	if ( hkey != NULL )
	{
	    *hkey = keydupe;
	    *hkeylen = keylen;
	}
    }
    nx_value_kill(&contextkeyval);

    return ( hashval );
}



static void process_suppressed(nx_expr_eval_ctx_t *eval_ctx,
			       apr_time_t timeval,
			       nx_pm_evcorr_rule_t *rule)
{
    nx_value_t value;
    apr_time_t *matched = NULL;

    nx_expr_evaluate(eval_ctx, &value, rule->suppressed.cond);
    if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
	 (value.defined == TRUE) &&
	 (value.boolean == TRUE) )
    {
	if ( rule->suppressed.context_expr != NULL )
	{
	    matched = get_context_hashval(eval_ctx,
					  rule->suppressed.context,
					  rule->suppressed.context_expr,
					  sizeof(apr_time_t), NULL, NULL);
	}
	if ( matched == NULL )
	{
	    matched = &(rule->suppressed.matched);
	}

	if ( (*matched == 0) ||
	     (*matched + (rule->suppressed.interval * APR_USEC_PER_SEC) <= timeval) )
	{
	    *matched = timeval;
	    nx_expr_statement_list_execute(eval_ctx, rule->suppressed.exec);
	}
    }
}



static void process_pair(nx_expr_eval_ctx_t *eval_ctx,
			 apr_time_t timeval,
			 nx_pm_evcorr_rule_t *rule)
{
    nx_value_t value;
    apr_time_t *matched = NULL;
    void *hkey = NULL;
    apr_ssize_t hkeylen;
    
    if ( rule->pair.context_expr != NULL )
    {
	matched = get_context_hashval(eval_ctx,
				      rule->pair.context,
				      rule->pair.context_expr,
				      sizeof(apr_time_t),
				      &hkey, &hkeylen);
    }
    if ( matched == NULL )
    {
	matched = &(rule->pair.matched);
    }

    if ( *matched > 0 )
    { // TriggerCondtion was already matched
	log_debug("triggercond was matched before");
	nx_expr_evaluate(eval_ctx, &value, rule->pair.requiredcond);
	if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
	     (value.defined == TRUE) &&
	     (value.boolean == TRUE) )
	{ // RequiredCondition matches
	    log_debug("requiredcond matches");
	    if ( (rule->pair.interval == 0) ||
		 (*matched + (rule->pair.interval * APR_USEC_PER_SEC) > timeval) )
	    {
		*matched = 0; // reset match
		nx_expr_statement_list_execute(eval_ctx, rule->pair.exec);
		if ( hkey != NULL )
		{ // free hash entry
		    apr_hash_set(rule->absence.context, hkey, hkeylen, NULL);
		    free(hkey);
		    free(matched);
		}
	    }
	}
	else
	{   // RequiredCondition didn't match, evaluate TriggerCondition again
	    // in case the timestamp can be updated
	    nx_expr_evaluate(eval_ctx, &value, rule->pair.triggercond);
	    if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
		 (value.defined == TRUE) &&
		 (value.boolean == TRUE) )
	    { // TriggerCondition matches
		log_debug("requiredcond didn't match but triggercond matches");
		*matched = timeval;
	    }
	}
    }
    else
    { // TriggerCondition was never true, evaluate it
	nx_expr_evaluate(eval_ctx, &value, rule->pair.triggercond);
	if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
	     (value.defined == TRUE) &&
	     (value.boolean == TRUE) )
	{ // TriggerCondition matches
	    log_debug("first time match on triggercond");
	    *matched = timeval;
	}
    }
    nx_value_kill(&value);
}



static void process_absence(nx_module_t *module,
			    nx_expr_eval_ctx_t *eval_ctx,
			    apr_time_t timeval,
			    nx_pm_evcorr_rule_t *rule)
{
    nx_value_t value;
    nx_event_t *event;
    nx_pm_evcorr_rule_absence_data_t *absence = NULL;
    void *hkey = NULL;
    apr_ssize_t hkeylen;

    if ( rule->absence.context_expr != NULL )
    {
	absence = get_context_hashval(eval_ctx,
				      rule->absence.context,
				      rule->absence.context_expr,
				      sizeof(nx_pm_evcorr_rule_absence_data_t),
				      &hkey, &hkeylen);
    }
    if ( absence == NULL )
    {
	absence = &(rule->absence.data);
    }
    ASSERT((absence->rule == NULL) || (absence->rule == rule));
    absence->rule = rule;

    if ( absence->matched > 0 )
    { // trigger condition was already matched
	log_debug("triggercond was matched before");

	// check if we are over the time already
	if ( timeval > absence->matched + rule->absence.interval * APR_USEC_PER_SEC )
	{ // time window is over the limit
	    absence->matched = 0; // reset match
	    nx_expr_statement_list_execute(eval_ctx, rule->absence.exec);
	    if ( absence->event != NULL )
	    {
		nx_event_remove(absence->event);
		nx_event_free(absence->event);
		absence->event = NULL;
	    }
	    if ( hkey != NULL )
	    { // free hash entry
		apr_hash_set(rule->absence.context, hkey, hkeylen, NULL);
		free(hkey);
		free(absence);
	    }
	}
	else
	{
	    nx_expr_evaluate(eval_ctx, &value, rule->absence.requiredcond);
	    if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
		 (value.defined == TRUE) &&
		 (value.boolean == TRUE) )
	    { // reqiredcondition matches
		log_debug("requiredcond matches");
		if ( absence->matched + (rule->absence.interval * APR_USEC_PER_SEC) > timeval )
		{ // we are within the interval, remove the event
		    absence->matched = 0; // reset match
		    if ( absence->event != NULL )
		    {
			nx_event_remove(absence->event);
			nx_event_free(absence->event);
			absence->event = NULL;
		    }
		    if ( hkey != NULL )
		    { // free hash entry
			apr_hash_set(rule->absence.context, hkey, hkeylen, NULL);
			free(hkey);
			free(absence);
		    }
		}
		//else we are over the Interval time and the timer event should have fired
	    }
	    else
	    {   // requiredcondition didn't match
	    }
	    nx_value_kill(&value);
	}
    }
    else
    { // triggercondition was never true, evaluate it
	nx_expr_evaluate(eval_ctx, &value, rule->absence.triggercond);
	if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
	     (value.defined == TRUE) &&
	     (value.boolean == TRUE) )
	{ // triggercondition matches
	    log_debug("first time match on triggercond");
	    absence->matched = timeval;
	    // add event
	    event = nx_event_new();
	    ASSERT(absence->event == NULL);
	    absence->event = event;
	    event->module = module;
	    event->type = NX_EVENT_MODULE_SPECIFIC;
	    event->delayed = TRUE;
	    event->time = apr_time_now() + APR_USEC_PER_SEC * rule->absence.interval;
	    event->data = (void *) absence;
	    event->priority = module->priority;
	    nx_event_add(event);
	}
	nx_value_kill(&value);
    }
}


#define NX_PM_EVCORR_DEFAULT_THRESHOLDED_WINDOW_SIZE 100

static void process_thresholded(nx_expr_eval_ctx_t *eval_ctx,
				apr_time_t timeval,
				nx_pm_evcorr_rule_t *rule)
{
    nx_value_t value;
    size_t i;
    nx_pm_evcorr_rule_thresholded_data_t *thresholded = NULL;
    void *hkey = NULL;
    apr_ssize_t hkeylen;

    if ( rule->thresholded.context_expr != NULL )
    {
	thresholded = get_context_hashval(eval_ctx,
					  rule->thresholded.context,
					  rule->thresholded.context_expr,
					  sizeof(nx_pm_evcorr_rule_thresholded_data_t),
					  &hkey, &hkeylen);
    }
    if ( thresholded == NULL )
    {
	thresholded = &(rule->thresholded.data);
    }

    nx_expr_evaluate(eval_ctx, &value, rule->thresholded.cond);
    if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
	 (value.defined == TRUE) &&
	 (value.boolean == TRUE) )
    { // condition matches
	if ( thresholded->matchlist == NULL )
	{
	    thresholded->matchlist = malloc(sizeof(apr_time_t) * NX_PM_EVCORR_DEFAULT_THRESHOLDED_WINDOW_SIZE);
	    ASSERT(thresholded->matchlist != NULL);
	    thresholded->matchlist_size = NX_PM_EVCORR_DEFAULT_THRESHOLDED_WINDOW_SIZE;
	    thresholded->num_match = 0;
	}

	if ( (thresholded->num_match + 1 >= rule->thresholded.threshold) ||
	     (thresholded->num_match + 1 >= thresholded->matchlist_size) )
	{
	    for ( i = 0; i < thresholded->num_match; i++ )
	    {
		if ( timeval <= thresholded->matchlist[i] + rule->thresholded.interval * APR_USEC_PER_SEC )
		{
		    break;
		}
	    }
	    if ( i > 0 )
	    {
		memmove(thresholded->matchlist, &(thresholded->matchlist[i]),
			(thresholded->num_match - i) * sizeof(apr_time_t));
		ASSERT(thresholded->num_match >= i);
		thresholded->num_match -= i;
		if ( (thresholded->num_match < thresholded->matchlist_size / 2) && 
		     (thresholded->matchlist_size > NX_PM_EVCORR_DEFAULT_THRESHOLDED_WINDOW_SIZE) )
		{
		    thresholded->matchlist = realloc(thresholded->matchlist,
						     (thresholded->matchlist_size / 2) *
						     sizeof(apr_time_t));
		    ASSERT(thresholded->matchlist != NULL);
		}
	    }
	}

	if ( thresholded->num_match + 1 >= thresholded->matchlist_size )
	{
	    thresholded->matchlist_size = (thresholded->matchlist_size * 3) / 2;
	    thresholded->matchlist = realloc(thresholded->matchlist,
					     thresholded->matchlist_size * sizeof(apr_time_t));
	    ASSERT(thresholded->matchlist != NULL);
	}
	thresholded->matchlist[thresholded->num_match] = timeval;
	(thresholded->num_match)++;

	if ( thresholded->num_match >= rule->thresholded.threshold )
	{
	    nx_expr_statement_list_execute(eval_ctx, rule->thresholded.exec);
	}
    }
    nx_value_kill(&value);
}



static boolean process_stop(nx_expr_eval_ctx_t *eval_ctx,
			    nx_pm_evcorr_rule_t *rule)
{
    nx_value_t value;
    boolean retval = FALSE;

    nx_expr_evaluate(eval_ctx, &value, rule->stop.cond);
    if ( (value.type == NX_VALUE_TYPE_BOOLEAN) &&
	 (value.defined == TRUE) &&
	 (value.boolean == TRUE) )
    {
	retval = TRUE;
	if ( rule->stop.exec != NULL )
	{
	    nx_expr_statement_list_execute(eval_ctx, rule->stop.exec);
	}
    }
    nx_value_kill(&value);

    return ( retval );
}



static nx_logdata_t *pm_evcorr_process(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_pm_evcorr_conf_t *modconf;
    nx_pm_evcorr_rule_t *rule;
    nx_expr_eval_ctx_t eval_ctx;
    apr_time_t timeval;
    boolean stop = FALSE;

    ASSERT(logdata != NULL);
    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    modconf = (nx_pm_evcorr_conf_t *) module->config;

    log_debug("nx_pm_evcorr_process()");

    nx_expr_eval_ctx_init(&eval_ctx, logdata, module, NULL);

    // there might be a small delay caused by the exec blocks in rules down the list
    // but we assume it is negligable and don't calculate time before each rule
    timeval = get_timeval(modconf, logdata);

    for ( rule = NX_DLIST_FIRST(modconf->rules);
	  (rule != NULL) && (stop == FALSE);
	  rule = NX_DLIST_NEXT(rule, link) )
    {
	switch ( rule->type )
	{
	    case NX_PM_EVCORR_RULE_TYPE_SIMPLE:
		nx_expr_statement_list_execute(&eval_ctx, rule->simple.exec);
		break;
	    case NX_PM_EVCORR_RULE_TYPE_SUPPRESSED:
		process_suppressed(&eval_ctx, timeval, rule);
		break;
	    case NX_PM_EVCORR_RULE_TYPE_PAIR:
		process_pair(&eval_ctx, timeval, rule);
		break;
	    case NX_PM_EVCORR_RULE_TYPE_ABSENCE:
		process_absence(module, &eval_ctx, timeval, rule);
		break;
	    case NX_PM_EVCORR_RULE_TYPE_THRESHOLDED:
		process_thresholded(&eval_ctx, timeval, rule);
		break;
	    case NX_PM_EVCORR_RULE_TYPE_STOP:
		stop = process_stop(&eval_ctx, rule);
		break;
	    default:
		nx_panic("invalid rule type %d", rule->type);
	}

	if ( eval_ctx.logdata == NULL )
	{ // dropped
	    nx_module_logqueue_pop(module, logdata);
	    nx_logdata_free(logdata);
	    nx_expr_eval_ctx_destroy(&eval_ctx);
	    return ( NULL );
	}
    }

    nx_expr_eval_ctx_destroy(&eval_ctx);

    return ( logdata );
}



static void pm_evcorr_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;

    log_debug("nx_pm_evcorr_data_available()");
    
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    logdata = pm_evcorr_process(module, logdata);

    //nx_logdata_dump_fields(logdata);

    // add logdata to the next module's queue
    if ( logdata != NULL )
    {
	nx_module_progress_logdata(module, logdata);
    }
}



static void pm_evcorr_absence_event(nx_module_t *module,
				    nx_pm_evcorr_rule_absence_data_t *absence)
{
    nx_expr_eval_ctx_t eval_ctx;
    nx_pm_evcorr_rule_t	*rule;

    ASSERT(absence != NULL);

    eval_ctx.module = module;
    eval_ctx.logdata = NULL;
    ASSERT(absence->rule != NULL);
    rule = absence->rule;
    nx_expr_statement_list_execute(&eval_ctx, rule->absence.exec);
    absence->matched = 0;
    absence->event = NULL;
}



static void parse_int(const nx_directive_t *curr, int64_t *val, const char *name)
{
    unsigned int i;
    const char *ptr;

    ASSERT(curr != NULL);

    if ( *val != 0 )
    {
	nx_conf_error(curr, "'%s' is already defined", name);
    }
    for ( ptr = curr->args; *ptr != '\0'; ptr++ )
    {
	if ( ! apr_isdigit(*ptr) )
	{
	    nx_conf_error(curr, "invalid %s '%s', digit expected", name, curr->args);
	}
    }	    
    if ( sscanf(curr->args, "%u", &i) != 1 )
    {
	nx_conf_error(curr, "invalid %s '%s'", name, curr->args);
    }
    *val = (int64_t) i;
}



static void parse_condition(nx_module_t *module, 
			    nx_expr_t **cond,
			    const nx_directive_t *curr,
			    apr_pool_t *pool)
{
    nx_exception_t e;

    if ( *cond != NULL )
    {
	nx_conf_error(curr, "'Condition' already defined");
    }
    try
    {
	*cond = nx_expr_parse(module, curr->args, pool, curr->filename,
			      curr->line_num, curr->argsstart);
	if ( *cond == NULL )
	{
	    throw_msg("invalid or empty expression for Condition: '%s'", curr->args);
	}
	if ( (*cond)->rettype != NX_VALUE_TYPE_BOOLEAN )
	{
	    throw_msg("boolean type required in expression, found '%s'",
		      nx_value_type_to_string((*cond)->rettype));
	}
    }
    catch(e)
    {
	log_exception(e);
	nx_conf_error(curr, "invalid expression in 'Condition'");
    }
}


static void pm_evcorr_config(nx_module_t *module)
{
    const nx_directive_t *curr, *curr2;
    nx_pm_evcorr_conf_t *modconf;
    nx_pm_evcorr_rule_t *rule;
    boolean uses_context = FALSE;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_conf_t));
    module->config = modconf;

    modconf->rules = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_rule_list_t));
    NX_DLIST_INIT(modconf->rules, nx_pm_evcorr_rule_t, link);
    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "TimeField") == 0 )
	{
	    modconf->timefield = apr_pstrdup(module->pool, curr->args);
	}
	else if ( strcasecmp(curr->directive, "Simple") == 0 )
	{
	    curr2 = curr->first_child;
	    if ( curr2 == NULL )
	    {
		nx_conf_error(curr, "empty rule 'Simple'");
	    }

	    rule = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_rule_t));
	    rule->type = NX_PM_EVCORR_RULE_TYPE_SIMPLE;
	    while ( curr2 != NULL )
	    {
		if ( strcasecmp(curr2->directive, "Exec") == 0 )
		{
		}
		else
		{
		    nx_conf_error(curr2, "invalid directive %s in rule 'Simple'", curr2->directive);
		}
		curr2 = curr2->next;
	    }
	    rule->simple.exec = nx_module_parse_exec_block(module, module->pool, curr->first_child);
	    if ( rule->simple.exec == NULL )
	    {
		nx_conf_error(curr, "'Exec' missing from rule 'Simple'");
	    }
	    NX_DLIST_INSERT_TAIL(modconf->rules, rule, link);
	}
	else if ( strcasecmp(curr->directive, "Suppressed") == 0 )
	{
	    curr2 = curr->first_child;
	    if ( curr2 == NULL )
	    {
		nx_conf_error(curr, "empty rule 'Suppressed'");
	    }

	    rule = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_rule_t));
	    rule->type = NX_PM_EVCORR_RULE_TYPE_SUPPRESSED;
	    while ( curr2 != NULL )
	    {
		if ( strcasecmp(curr2->directive, "Exec") == 0 )
		{
		}
		else if ( strcasecmp(curr2->directive, "Condition") == 0 )
		{
		    parse_condition(module, &(rule->suppressed.cond), curr2, module->pool);
		}
		else if ( strcasecmp(curr2->directive, "Interval") == 0 )
		{
		    parse_int(curr2, &(rule->suppressed.interval), "Interval");
		}
		else if ( strcasecmp(curr2->directive, "Context") == 0 )
		{
		    if ( rule->suppressed.context_expr != NULL )
		    {
			nx_conf_error(curr2, "'Context' already defined");
		    }
		    rule->suppressed.context_expr = nx_expr_parse(module, curr2->args, module->pool,
								  curr2->filename, curr2->line_num,
								  curr2->argsstart);
		    if ( rule->suppressed.context_expr == NULL )
		    {
			throw_msg("invalid or empty expression for Context: '%s'", curr2->args);
		    }
		    if ( rule->suppressed.context_expr->type == NX_EXPR_TYPE_VALUE )
		    {
			nx_conf_error(curr2, "using a constant value for 'Context' is useless");
		    }
		    rule->suppressed.context = apr_hash_make(module->pool);
		    uses_context = TRUE;
		}
		else
		{
		    nx_conf_error(curr, "invalid directive %s in rule 'Suppressed'", curr2->directive);
		}
		curr2 = curr2->next;
	    }
	    rule->suppressed.exec = nx_module_parse_exec_block(module, module->pool, curr->first_child);
	    if ( rule->suppressed.exec == NULL )
	    {
		nx_conf_error(curr, "'Exec' missing from rule 'Suppressed'");
	    }
	    if ( rule->suppressed.cond == NULL )
	    {
		nx_conf_error(curr, "'Condition' missing from rule 'Suppressed'");
	    }
	    NX_DLIST_INSERT_TAIL(modconf->rules, rule, link);
	}
	else if ( strcasecmp(curr->directive, "Pair") == 0 )
	{
	    curr2 = curr->first_child;
	    if ( curr2 == NULL )
	    {
		nx_conf_error(curr, "empty rule 'Pair'");
	    }

	    rule = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_rule_t));
	    rule->type = NX_PM_EVCORR_RULE_TYPE_PAIR;
	    while ( curr2 != NULL )
	    {
		if ( strcasecmp(curr2->directive, "Exec") == 0 )
		{
		}
		else if ( strcasecmp(curr2->directive, "TriggerCondition") == 0 )
		{
		    parse_condition(module, &(rule->pair.triggercond), curr2, module->pool);
		}
		else if ( strcasecmp(curr2->directive, "RequiredCondition") == 0 )
		{
		    parse_condition(module, &(rule->pair.requiredcond), curr2, module->pool);
		}
		else if ( strcasecmp(curr2->directive, "Interval") == 0 )
		{
		    parse_int(curr2, &(rule->pair.interval), "Interval");
		}
		else if ( strcasecmp(curr2->directive, "Context") == 0 )
		{
		    if ( rule->pair.context_expr != NULL )
		    {
			nx_conf_error(curr2, "'Context' already defined");
		    }
		    rule->pair.context_expr = nx_expr_parse(module, curr2->args, module->pool,
							    curr2->filename, curr2->line_num,
							    curr2->argsstart);
		    if ( rule->pair.context_expr == NULL )
		    {
			throw_msg("invalid or empty expression for Context: '%s'", curr2->args);
		    }
		    if ( rule->pair.context_expr->type == NX_EXPR_TYPE_VALUE )
		    {
			nx_conf_error(curr2, "using a constant value for 'Context' is useless");
		    }
		    rule->pair.context = apr_hash_make(module->pool);
		    uses_context = TRUE;
		}
		else
		{
		    nx_conf_error(curr, "invalid directive %s in rule 'Pair'", curr2->directive);
		}
		curr2 = curr2->next;
	    }
	    rule->pair.exec = nx_module_parse_exec_block(module, module->pool, curr->first_child);
	    if ( rule->pair.exec == NULL )
	    {
		nx_conf_error(curr, "'Exec' missing from rule 'Pair'");
	    }
	    if ( rule->pair.triggercond == NULL )
	    {
		nx_conf_error(curr, "'TriggerCondition' missing from rule 'Pair'");
	    }
	    if ( rule->pair.requiredcond == NULL )
	    {
		nx_conf_error(curr, "'RequiredCondition' missing from rule 'Pair'");
	    }
	    NX_DLIST_INSERT_TAIL(modconf->rules, rule, link);
	}
	else if ( strcasecmp(curr->directive, "Absence") == 0 )
	{
	    curr2 = curr->first_child;
	    if ( curr2 == NULL )
	    {
		nx_conf_error(curr, "empty rule 'Absence'");
	    }

	    rule = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_rule_t));
	    rule->type = NX_PM_EVCORR_RULE_TYPE_ABSENCE;
	    while ( curr2 != NULL )
	    {
		if ( strcasecmp(curr2->directive, "Exec") == 0 )
		{
		}
		else if ( strcasecmp(curr2->directive, "TriggerCondition") == 0 )
		{
		    parse_condition(module, &(rule->absence.triggercond), curr2, module->pool);
		}
		else if ( strcasecmp(curr2->directive, "RequiredCondition") == 0 )
		{
		    parse_condition(module, &(rule->absence.requiredcond), curr2, module->pool);
		}
		else if ( strcasecmp(curr2->directive, "Interval") == 0 )
		{
		    parse_int(curr2, &(rule->absence.interval), "Interval");
		    if ( rule->absence.interval == 0 )
		    {
			nx_conf_error(curr2, "'Interval' in 'Absence' cannot be zero");
		    }
		}
		else if ( strcasecmp(curr2->directive, "Context") == 0 )
		{
		    if ( rule->absence.context_expr != NULL )
		    {
			nx_conf_error(curr2, "'Context' already defined");
		    }
		    rule->absence.context_expr = nx_expr_parse(module, curr2->args, module->pool,
							       curr2->filename, curr2->line_num,
							       curr2->argsstart);
		    if ( rule->absence.context_expr == NULL )
		    {
			throw_msg("invalid or empty expression for Context: '%s'", curr2->args);
		    }
		    if ( rule->absence.context_expr->type == NX_EXPR_TYPE_VALUE )
		    {
			nx_conf_error(curr2, "using a constant value for 'Context' is useless");
		    }
		    rule->absence.context = apr_hash_make(module->pool);
		    uses_context = TRUE;
		}
		else
		{
		    nx_conf_error(curr, "invalid directive %s in rule 'Absence'", curr2->directive);
		}
		curr2 = curr2->next;
	    }
	    rule->absence.exec = nx_module_parse_exec_block(module, module->pool, curr->first_child);
	    if ( rule->absence.exec == NULL )
	    {
		nx_conf_error(curr, "'Exec' missing from rule 'Absence'");
	    }
	    if ( rule->absence.triggercond == NULL )
	    {
		nx_conf_error(curr, "'TriggerCondition' missing from rule 'Absence'");
	    }
	    if ( rule->absence.requiredcond == NULL )
	    {
		nx_conf_error(curr, "'RequiredCondition' missing from rule 'Absence'");
	    }
	    NX_DLIST_INSERT_TAIL(modconf->rules, rule, link);
	}
	else if ( strcasecmp(curr->directive, "Thresholded") == 0 )
	{
	    curr2 = curr->first_child;
	    if ( curr2 == NULL )
	    {
		nx_conf_error(curr, "empty rule 'Thresholded'");
	    }

	    rule = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_rule_t));
	    rule->type = NX_PM_EVCORR_RULE_TYPE_THRESHOLDED;
	    
	    while ( curr2 != NULL )
	    {
		if ( strcasecmp(curr2->directive, "Exec") == 0 )
		{
		}
		else if ( strcasecmp(curr2->directive, "Condition") == 0 )
		{
		    parse_condition(module, &(rule->thresholded.cond), curr2, module->pool);
		}
		else if ( strcasecmp(curr2->directive, "Interval") == 0 )
		{
		    parse_int(curr2, &(rule->thresholded.interval), "Interval");
		    if ( rule->thresholded.interval == 0 )
		    {
			nx_conf_error(curr2, "'Interval' in 'Thresholded' cannot be zero");
		    }
		}
		else if ( strcasecmp(curr2->directive, "Threshold") == 0 )
		{
		    parse_int(curr2, (int64_t *) &(rule->thresholded.threshold), "Threshold");
		    if ( rule->thresholded.threshold == 0 )
		    {
			nx_conf_error(curr2, "'Threshold' in 'Thresholded' cannot be zero");
		    }
		}
		else if ( strcasecmp(curr2->directive, "Context") == 0 )
		{
		    if ( rule->thresholded.context_expr != NULL )
		    {
			nx_conf_error(curr2, "'Context' already defined");
		    }
		    rule->thresholded.context_expr = nx_expr_parse(module, curr2->args, module->pool,
								   curr2->filename, curr2->line_num,
								   curr2->argsstart);
		    if ( rule->thresholded.context_expr == NULL )
		    {
			throw_msg("invalid or empty expression for Context: '%s'", curr2->args);
		    }
		    if ( rule->thresholded.context_expr->type == NX_EXPR_TYPE_VALUE )
		    {
			nx_conf_error(curr2, "using a constant value for 'Context' is useless");
		    }
		    rule->thresholded.context = apr_hash_make(module->pool);
		    uses_context = TRUE;
		}
		else
		{
		    nx_conf_error(curr, "invalid directive %s in rule 'Thresholded'", curr2->directive);
		}
		curr2 = curr2->next;
	    }
	    rule->thresholded.exec = nx_module_parse_exec_block(module, module->pool, curr->first_child);
	    if ( rule->thresholded.exec == NULL )
	    {
		nx_conf_error(curr, "'Exec' missing from rule 'Thresholded'");
	    }
	    if ( rule->thresholded.cond == NULL )
	    {
		nx_conf_error(curr, "'Condition' missing from rule 'Thresholded'");
	    }
	    NX_DLIST_INSERT_TAIL(modconf->rules, rule, link);
	}
	else if ( strcasecmp(curr->directive, "Stop") == 0 )
	{
	    curr2 = curr->first_child;
	    if ( curr2 == NULL )
	    {
		nx_conf_error(curr, "empty rule 'Thresholded'");
	    }

	    rule = apr_pcalloc(module->pool, sizeof(nx_pm_evcorr_rule_t));
	    rule->type = NX_PM_EVCORR_RULE_TYPE_STOP;
	    
	    while ( curr2 != NULL )
	    {
		if ( strcasecmp(curr2->directive, "Exec") == 0 )
		{
		}
		else if ( strcasecmp(curr2->directive, "Condition") == 0 )
		{
		    parse_condition(module, &(rule->stop.cond), curr2, module->pool);
		}
		else
		{
		    nx_conf_error(curr, "invalid directive %s in rule 'Thresholded'", curr2->directive);
		}
		curr2 = curr2->next;
	    }
	    rule->stop.exec = nx_module_parse_exec_block(module, module->pool, curr->first_child);
	    if ( rule->stop.cond == NULL )
	    {
		nx_conf_error(curr, "'Condition' missing from rule 'Stop'");
	    }
	    NX_DLIST_INSERT_TAIL(modconf->rules, rule, link);
	}
	else if ( strcasecmp(curr->directive, "ContextCleanTime") == 0 )
	{
	    parse_int(curr, &(modconf->context_cleantime), "ContextCleanTime");
	}
	else
	{
	    nx_conf_error(curr, "invalid pm_evcorr keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( uses_context == TRUE )
    {
	if ( modconf->context_cleantime == 0 )
	{ // use default if not specified
	    modconf->context_cleantime = NX_PM_EVCORR_DEFAULT_CONTEXT_CLEANTIME;
	}
    }
    else if ( modconf->context_cleantime != 0 )
    {
	log_warn("ContextCleanTime defined but no correlation rules use Context");
	modconf->context_cleantime = 0; // don't install the cleanup timer
    }
}



static void free_hash_item(apr_hash_t *hash,
			   void *value,
			   void *key,
			   apr_ssize_t keylen)
{

    apr_hash_set(hash, key, keylen, NULL);
    free(key);
    free(value);
}



static void pm_evcorr_suppressed_clean(nx_module_t *module,
				       nx_pm_evcorr_rule_t *rule,
				       boolean clearall)
{
    apr_time_t matched;
    nx_pm_evcorr_conf_t *modconf;
    apr_hash_index_t *idx;
    void *val;
    void *key;
    apr_ssize_t keylen;

    if ( clearall == TRUE )
    {
	while ( (idx = apr_hash_first(NULL, rule->suppressed.context)) != NULL )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);
	    free_hash_item(rule->suppressed.context, val, key, keylen);
	}
    }
    else
    {
	for ( idx = apr_hash_first(NULL, rule->suppressed.context); idx != NULL; )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);
	    idx = apr_hash_next(idx);
	    matched = *((apr_time_t *) val);
	    modconf = (nx_pm_evcorr_conf_t *) module->config;
	    
	    if ( matched + (rule->suppressed.interval * APR_USEC_PER_SEC) <= modconf->timeval )
	    {
		free_hash_item(rule->suppressed.context, val, key, keylen);
	    }
	}
    }
}



static void pm_evcorr_pair_clean(nx_module_t *module,
				 nx_pm_evcorr_rule_t *rule,
				 boolean clearall)
{
    apr_time_t matched;
    nx_pm_evcorr_conf_t *modconf;
    apr_hash_index_t *idx;
    void *val;
    void *key;
    apr_ssize_t keylen;

    if ( clearall == TRUE )
    {
	while ( (idx = apr_hash_first(NULL, rule->pair.context)) != NULL )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);
	    free_hash_item(rule->pair.context, val, key, keylen);
	}
    }
    else
    {
	for ( idx = apr_hash_first(NULL, rule->pair.context); idx != NULL; )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);

	    idx = apr_hash_next(idx);
	    matched = *((apr_time_t *) val);
	    modconf = (nx_pm_evcorr_conf_t *) module->config;
	
	    if ( (rule->pair.interval > 0 ) &&
		 (matched + (rule->pair.interval * APR_USEC_PER_SEC) <= modconf->timeval) )
	    {
		free_hash_item(rule->pair.context, val, key, keylen);
	    }
	}
    }
}



static void pm_evcorr_absence_clean(nx_module_t *module,
				    nx_pm_evcorr_rule_t *rule,
				    boolean clearall)
{
    nx_pm_evcorr_conf_t *modconf;
    apr_hash_index_t *idx;
    void *val;
    void *key;
    apr_ssize_t keylen;
    nx_pm_evcorr_rule_absence_data_t *absence;

    if ( clearall == TRUE )
    {
	while ( (idx = apr_hash_first(NULL, rule->absence.context)) != NULL )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);
	    absence = (nx_pm_evcorr_rule_absence_data_t *) val;

	    if ( absence->event != NULL )
	    {
		nx_event_remove(absence->event);
		nx_event_free(absence->event);
		absence->event = NULL;
	    }
	    free_hash_item(rule->absence.context, val, key, keylen);
	}
    }
    else
    {
	for ( idx = apr_hash_first(NULL, rule->absence.context); idx != NULL; )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);
	    idx = apr_hash_next(idx);
	    absence = (nx_pm_evcorr_rule_absence_data_t *) val;

	    modconf = (nx_pm_evcorr_conf_t *) module->config;
	    if ( absence->matched + (rule->absence.interval * APR_USEC_PER_SEC) <= modconf->timeval )
	    {
		if ( absence->event != NULL )
		{
		    nx_event_remove(absence->event);
		    nx_event_free(absence->event);
		    absence->event = NULL;
		}
		free_hash_item(rule->absence.context, val, key, keylen);
	    }
	}
    }
}



static void pm_evcorr_thresholded_clean(nx_module_t *module,
					nx_pm_evcorr_rule_t *rule,
					boolean clearall)
{
    nx_pm_evcorr_conf_t *modconf;
    apr_hash_index_t *idx;
    void *val;
    void *key;
    apr_ssize_t keylen;
    nx_pm_evcorr_rule_thresholded_data_t *thresholded;

    if ( clearall == TRUE )
    {
	while ( (idx = apr_hash_first(NULL, rule->thresholded.context)) != NULL )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);
	    thresholded = (nx_pm_evcorr_rule_thresholded_data_t *) val;

	    if ( thresholded->matchlist != NULL )
	    {
		free(thresholded->matchlist);
		thresholded->matchlist = NULL;
	    }
	    free_hash_item(rule->thresholded.context, val, key, keylen);
	}
    }
    else
    {
	for ( idx = apr_hash_first(NULL, rule->thresholded.context); idx != NULL; )
	{
	    apr_hash_this(idx, (const void **) &key, &keylen, &val);
	    thresholded = (nx_pm_evcorr_rule_thresholded_data_t *) val;
	    idx = apr_hash_next(idx);
	    modconf = (nx_pm_evcorr_conf_t *) module->config;

	    ASSERT(thresholded != NULL);
	    if ( (thresholded->matchlist != NULL) &&
		 ((thresholded->matchlist[thresholded->num_match - 1] + 
		   (rule->thresholded.interval * APR_USEC_PER_SEC)) <= modconf->timeval) )
	    {
		if ( thresholded->matchlist != NULL )
		{
		    free(thresholded->matchlist);
		    thresholded->matchlist = NULL;
		}
		free_hash_item(rule->thresholded.context, val, key, keylen);
	    }
	}
    }
}



static void pm_evcorr_clean_rules(nx_module_t *module, boolean clearall)
{
    nx_pm_evcorr_conf_t *modconf;
    nx_pm_evcorr_rule_t *rule;

    modconf = (nx_pm_evcorr_conf_t *) module->config;

    for ( rule = NX_DLIST_FIRST(modconf->rules);
	  rule != NULL;
	  rule = NX_DLIST_NEXT(rule, link) )
    {
	switch ( rule->type )
	{
	    case NX_PM_EVCORR_RULE_TYPE_SIMPLE:
		break;
	    case NX_PM_EVCORR_RULE_TYPE_SUPPRESSED:
		if ( rule->suppressed.context != NULL )
		{
		    pm_evcorr_suppressed_clean(module, rule, clearall);
		}
		break;
	    case NX_PM_EVCORR_RULE_TYPE_PAIR:
		if ( rule->pair.context != NULL )
		{
		    pm_evcorr_pair_clean(module, rule, clearall);
		}
		break;
	    case NX_PM_EVCORR_RULE_TYPE_STOP:
		break;
	    case NX_PM_EVCORR_RULE_TYPE_ABSENCE:
		if ( rule->absence.context != NULL )
		{
		    pm_evcorr_absence_clean(module, rule, clearall);
		}
		else
		{
		    if ( rule->absence.data.event != NULL )
		    {
			nx_event_remove(rule->absence.data.event);
			nx_event_free(rule->absence.data.event);
		    }
		}
		break;
	    case NX_PM_EVCORR_RULE_TYPE_THRESHOLDED:
		if ( rule->absence.context != NULL )
		{
		    pm_evcorr_thresholded_clean(module, rule, clearall);
		}
		else
		{
		    if ( rule->thresholded.data.matchlist != NULL )
		    {
			free(rule->thresholded.data.matchlist);
			rule->thresholded.data.matchlist = NULL;
			rule->thresholded.data.matchlist_size = 0;
		    }
		}
		break;
	    default:
		nx_panic("invalid rule type %d", rule->type);
	}
    }
}



static void pm_evcorr_add_cleanup_event(nx_module_t *module)
{
    nx_pm_evcorr_conf_t *modconf;
    nx_event_t *event;

    ASSERT(module != NULL);

    modconf = (nx_pm_evcorr_conf_t *) module->config;

    if ( modconf->context_cleantime > 0 )
    {
	event = nx_event_new();
	ASSERT(modconf->cleanup_event == NULL);
	modconf->cleanup_event = event;
	event->module = module;
	// kind of a hack, but we already use MODULE_SPECIFIC for the absence rule
	event->type = NX_EVENT_RECONNECT; 
	event->delayed = TRUE;
	event->time = apr_time_now() + APR_USEC_PER_SEC * modconf->context_cleantime;
	event->priority = module->priority;
	nx_event_add(event);
    }
}



static void pm_evcorr_start(nx_module_t *module)
{
    pm_evcorr_add_cleanup_event(module);
}



static void pm_evcorr_stop(nx_module_t *module)
{
    nx_pm_evcorr_conf_t *modconf;

    ASSERT(module != NULL);

    modconf = (nx_pm_evcorr_conf_t *) module->config;

    if ( modconf->cleanup_event != NULL )
    {
	nx_event_remove(modconf->cleanup_event);
	nx_event_free(modconf->cleanup_event);
	modconf->cleanup_event = NULL;
    }

    pm_evcorr_clean_rules(module, TRUE);
}



static void pm_evcorr_event(nx_module_t *module, nx_event_t *event)
{
    nx_pm_evcorr_conf_t *modconf;

    ASSERT(event != NULL);

    modconf = (nx_pm_evcorr_conf_t *) module->config;

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_evcorr_data_available(module);
	    break;
	case NX_EVENT_RECONNECT:
	    pm_evcorr_clean_rules(module, FALSE);
	    modconf->cleanup_event = NULL;
	    pm_evcorr_add_cleanup_event(module);
	    break;
	case NX_EVENT_MODULE_SPECIFIC:
	    pm_evcorr_absence_event(module, (nx_pm_evcorr_rule_absence_data_t *) event->data);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_pm_evcorr_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    pm_evcorr_config, 		// config
    pm_evcorr_start,		// start
    pm_evcorr_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_evcorr_event,		// event
    NULL,			// info
    NULL,			// exports
};
