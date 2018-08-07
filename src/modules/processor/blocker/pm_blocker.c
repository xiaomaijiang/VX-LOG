/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"

#include "pm_blocker.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE



static void pm_blocker_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;
    nx_pm_blocker_conf_t *modconf;

    log_debug("nx_pm_blocker_data_available()");
    
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not writing any more data", module->name);
	return;
    }

    modconf = (nx_pm_blocker_conf_t *) module->config;

    if ( modconf->block == TRUE )
    {
	log_debug("pm_blocker blocking dataflow");
	return;
    }

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    // check again in case it was set to block in Exec
    if ( modconf->block == TRUE )
    {
	log_debug("pm_blocker blocking dataflow");
	return;
    }

    // add logdata to the next modules' queue
    nx_module_progress_logdata(module, logdata);
}



static void pm_blocker_config(nx_module_t *module)
{
    nx_pm_blocker_conf_t *modconf;
    const nx_directive_t *curr;

    modconf = apr_pcalloc(module->pool, sizeof(nx_pm_blocker_conf_t));
    module->config = modconf;

    modconf->block = FALSE;

    curr = module->directives;
    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	curr = curr->next;
    }
}



static void pm_blocker_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_blocker_data_available(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



extern nx_module_exports_t nx_module_exports_pm_blocker;

NX_MODULE_DECLARATION nx_pm_blocker_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    pm_blocker_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_blocker_event,		// event
    NULL,			// info
    &nx_module_exports_pm_blocker, //exports
};
