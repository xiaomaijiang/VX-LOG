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
#include "../../../common/alloc.h"

#include <apr_lib.h>
#include "pm_pattern.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static nx_logdata_t *pm_pattern_process(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_pm_pattern_conf_t *modconf;
    const nx_pattern_t *pattern = NULL;

    ASSERT(logdata != NULL);
    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    modconf = (nx_pm_pattern_conf_t *) module->config;

    log_debug("nx_pm_pattern_process()");
    ASSERT(modconf->patterndb != NULL);

    logdata = nx_patterndb_match_logdata(module, logdata, modconf->patterndb, &pattern);
    if ( pattern != NULL )
    {
	log_debug("Pattern %ld matched", pattern->id);
    }
    else
    {
	//log_info("didn't match");
    }

    return ( logdata );
}



static void pm_pattern_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;

    log_debug("nx_pm_pattern_data_available()");
    
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    logdata = pm_pattern_process(module, logdata);

    //nx_logdata_dump_fields(logdata);

    // add logdata to the next module's queue
    if ( logdata != NULL )
    {
	nx_module_progress_logdata(module, logdata);
    }
}



static void pm_pattern_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_pattern_data_available(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



static void pm_pattern_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_pm_pattern_conf_t *modconf;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    modconf = apr_pcalloc(module->pool, sizeof(nx_pm_pattern_conf_t));
    module->config = modconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "PatternFile") == 0 )
	{
	    modconf->patternfile = apr_pstrdup(module->pool, curr->args);
	}
	else
	{
	    nx_conf_error(curr, "invalid pm_pattern keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( modconf->patternfile == NULL )
    {
	nx_conf_error(module->directives, "'PatternFile' missing for module %s", module->name);
    }
}



static void pm_pattern_start(nx_module_t *module)
{
    nx_pm_pattern_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    modconf = (nx_pm_pattern_conf_t *) module->config;

    modconf->patterndb = nx_patterndb_parse(module->pool, modconf->patternfile);
    if ( NX_DLIST_FIRST(modconf->patterndb->groups) == NULL )
    {
	log_warn("no pattern groups found");
    }
}



static void pm_pattern_stop(nx_module_t *module)
{
    nx_pm_pattern_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    modconf = (nx_pm_pattern_conf_t *) module->config;
    if ( modconf->patterndb != NULL )
    {
	apr_pool_destroy(modconf->patterndb->pool);
	modconf->patterndb = NULL;
    }
}



NX_MODULE_DECLARATION nx_pm_pattern_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    pm_pattern_config,		// config
    pm_pattern_start,		// start
    pm_pattern_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_pattern_event,		// event
    NULL,			// info
    NULL,			// exports
};
