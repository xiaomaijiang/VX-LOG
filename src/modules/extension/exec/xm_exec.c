/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/error_debug.h"

#include "xm_exec.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

extern nx_module_exports_t nx_module_exports_xm_exec;

static void xm_exec_config(nx_module_t *module)
{
    nx_xm_exec_conf_t *imconf;

    imconf = apr_pcalloc(module->pool, sizeof(nx_xm_exec_conf_t));
    module->config = imconf;
}


static void xm_exec_start(nx_module_t *module)
{
    nx_xm_exec_conf_t *imconf;
    nx_event_t *event;

    ASSERT(module->config != NULL);

    imconf = (nx_xm_exec_conf_t *) module->config;
  
    ASSERT(imconf->event == NULL);
    event = nx_event_new();
    event->module = module;
    event->delayed = FALSE;
    event->type = NX_EVENT_MODULE_SPECIFIC;
    event->priority = module->priority;
    nx_event_add(event);
    imconf->event = event;
}



static void xm_exec_stop(nx_module_t *module)
{
    nx_xm_exec_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);
    imconf = (nx_xm_exec_conf_t *) module->config;

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}


/* This is needed to reap processes spawned by exec_async() */
static void xm_exec_reap(nx_module_t *module)
{
    nx_xm_exec_conf_t *imconf;
    nx_event_t *event;
    apr_exit_why_e exitwhy;
    int exitval;
    apr_status_t rv;
    apr_proc_t proc;

    ASSERT(module->config != NULL);

    imconf = (nx_xm_exec_conf_t *) module->config;

    while ( APR_STATUS_IS_CHILD_DONE(rv = apr_proc_wait_all_procs(&proc, &exitval,
								  &exitwhy, APR_NOWAIT, NULL)) )
    {
	log_debug("process %d reaped by xm_exec_reap", proc.pid);

	if ( proc.pid > 0 )
	{
	    if ( exitwhy == APR_PROC_EXIT )
	    { //normal termination
		if ( exitval != 0 )
		{
		    log_error("subprocess '%d' returned a non-zero exit value of %d",
			      proc.pid, exitval);
		}
	    }
	    else
	    {
		log_error("subprocess '%d' was terminated by a signal", proc.pid); 
	    }
	}
	else
	{ // on windows we get PID = -1 and it would result in an endless loop during exit
	    break;
	}
    }

    nx_module_remove_events_by_type(module, NX_EVENT_MODULE_SPECIFIC);
    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_MODULE_SPECIFIC;
    event->delayed = TRUE;
    event->time = apr_time_now() + APR_USEC_PER_SEC;
    event->priority = module->priority;
    nx_event_add(event);
    imconf->event = event;
}



static void xm_exec_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_MODULE_SPECIFIC:
	    xm_exec_reap(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_xm_exec_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_EXTENSION,
    NULL,			// capabilities
    xm_exec_config,		// config
    xm_exec_start,		// start
    xm_exec_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    xm_exec_event,		// event
    NULL,			// info
    &nx_module_exports_xm_exec, //exports
};
