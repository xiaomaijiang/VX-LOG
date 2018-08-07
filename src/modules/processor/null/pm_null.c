/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

static nx_logdata_t *pm_null_process(nx_module_t *module UNUSED, nx_logdata_t *logdata)
{
    ASSERT ( logdata != NULL );

    log_debug("nx_pm_null_process()");

    //log_debug("processing: [%s]", logdata->data);

    // This is the place to check/modify logdata

    return ( logdata );
}



static void pm_null_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;

    log_debug("nx_pm_null_data_available()");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }
    
    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    logdata = pm_null_process(module, logdata);

    // Other processors would check/modify logdata here

    // add logdata to the next modules' queue
    nx_module_progress_logdata(module, logdata);
}



static void pm_null_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_null_data_available(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_pm_null_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    NULL,			// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_null_event,		// event
    NULL,			// info
    NULL,			// exports
};
