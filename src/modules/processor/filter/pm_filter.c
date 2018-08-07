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
#include "pm_filter.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static nx_logdata_t *pm_filter_process(nx_module_t *module, nx_logdata_t * volatile logdata)
{
    nx_pm_filter_conf_t *modconf;
    nx_expr_eval_ctx_t eval_ctx;
    nx_value_t value;
    nx_exception_t e;

    ASSERT(logdata != NULL);
    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    modconf = (nx_pm_filter_conf_t *) module->config;

    log_debug("nx_pm_filter_process()");
    
    nx_expr_eval_ctx_init(&eval_ctx, logdata, module, NULL);
    try
    {
	nx_expr_evaluate(&eval_ctx, &value, modconf->condition);
    }
    catch(e)
    { // there was an error evaluating the boolean condition
	nx_expr_eval_ctx_destroy(&eval_ctx);
	log_exception(e);
	return ( logdata );
    }
    nx_expr_eval_ctx_destroy(&eval_ctx);

    ASSERT(value.type == NX_VALUE_TYPE_BOOLEAN);
    if ( value.boolean == TRUE )
    {
	return ( logdata );
    }

    log_debug("condition is FALSE, discarding logline");
    nx_module_logqueue_pop(module, logdata);
    nx_logdata_free(logdata);

    return ( NULL );
}



static void pm_filter_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;

    log_debug("nx_pm_filter_data_available()");
    
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    logdata = pm_filter_process(module, logdata);

    // add logdata to the next module's queue
    if ( logdata != NULL )
    {
	nx_module_progress_logdata(module, logdata);
    }
}



static void pm_filter_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_filter_data_available(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



static void pm_filter_config(nx_module_t *module)
{
    const nx_directive_t * volatile curr;
    nx_pm_filter_conf_t *modconf;
    nx_exception_t e;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_pm_filter_conf_t));
    module->config = modconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "condition") == 0 )
	{
	    if ( modconf->condition != NULL )
	    {
		nx_conf_error(curr, "'Condition' is already defined");
	    }
	    log_debug("parsing condition: [%s]", curr->args);
	    
	    try
	    {
		modconf->condition = nx_expr_parse(module, curr->args, module->pool, curr->filename,
						   curr->line_num, curr->argsstart);
		if ( modconf->condition == NULL )
		{
		    throw_msg("invalid or empty expression for Condition: '%s'", curr->args);
		}
		if ( modconf->condition->rettype != NX_VALUE_TYPE_BOOLEAN )
		{
		    throw_msg("boolean type required in expression, found '%s'",
			      nx_value_type_to_string(modconf->condition->rettype));
		}
	    }
	    catch(e)
	    {
		log_exception(e);
		nx_conf_error(curr, "invalid expression in 'Condition'");
	    }
	}
	else
	{
	    nx_conf_error(curr, "invalid pm_filter keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( modconf->condition == NULL )
    {
	nx_conf_error(module->directives, "'Condition' missing for module %s", module->name);
    }
}



NX_MODULE_DECLARATION nx_pm_filter_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    pm_filter_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_filter_event,		// event
    NULL,			// info
    NULL,			// exports
};
