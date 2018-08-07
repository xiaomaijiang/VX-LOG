/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <unistd.h>
#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"

#include "im_testgen.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static void im_testgen_read(nx_module_t *module)
{
    nx_im_testgen_conf_t *imconf;
    nx_logdata_t *logdata;
    char timebuf[40];
    int i;
    nx_event_t *event;
    char buf[250];

    ASSERT(module != NULL);

    imconf = (nx_im_testgen_conf_t *) module->config;
    imconf->event = NULL;
    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    log_debug("module %s looping while generating test input", module->name);

    for ( i = 0; i < 10 ; i++ )
    {
	if ( imconf->maxcount != 0 )
	{
	    if ( imconf->counter >= imconf->maxcount )
	    {
		log_info("maxcount %ld reached", (long int) imconf->maxcount);
		return;
	    }
	}
	memset(timebuf, 0, sizeof(timebuf));
	CHECKERR(apr_ctime(timebuf, apr_time_now()));
	apr_snprintf(buf, (size_t) module->input.bufsize,
		     "%ld@%s", (long int) imconf->counter, timebuf);
	logdata = nx_logdata_new_logline(buf, (int) strlen(buf));
	log_debug("generated line: [%s]", buf);
	nx_logdata_set_integer(logdata, "SeverityValue", NX_LOGLEVEL_INFO);
	nx_logdata_set_datetime(logdata, "EventTime", apr_time_now());
	nx_logdata_set_string(logdata, "SourceName", PACKAGE);
	nx_logdata_set_integer(logdata, "ProcessID", imconf->pid);
	nx_module_add_logdata_input(module, NULL, logdata);

	(imconf->counter)++;
    }

    if ( nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING )
    {
	event = nx_event_new();
	imconf->event = event;
	event->module = module;
	event->type = NX_EVENT_READ;
	event->delayed = FALSE;
	event->priority = module->priority;
	nx_event_add(event);
    }
}


static void im_testgen_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_testgen_conf_t *imconf;
    unsigned int maxcount;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_testgen_conf_t));
    module->config = imconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "MaxCount") == 0 )
	{
	    if ( imconf->maxcount != 0 )
	    {
		nx_conf_error(curr, "MaxCount is already defined");
	    }
	    if ( sscanf(curr->args, "%u", &maxcount) != 1 )
	    {
		nx_conf_error(curr, "invalid MaxCount: %s", curr->args);
	    }
	    imconf->maxcount = (long long int) maxcount;
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    imconf->pid = (int) getpid();
}



static void im_testgen_start(nx_module_t *module)
{
    nx_im_testgen_conf_t *imconf;
    nx_event_t *event;
 
    ASSERT(module->config != NULL);
    imconf = (nx_im_testgen_conf_t *) module->config;

    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->type = NX_EVENT_READ;
    event->delayed = FALSE;
    event->priority = module->priority;
    nx_event_add(event);
}



static void im_testgen_pause(nx_module_t *module)
{
    nx_im_testgen_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_testgen_conf_t *) module->config;

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}



static void im_testgen_resume(nx_module_t *module)
{
    nx_im_testgen_conf_t *imconf;
    nx_event_t *event;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_testgen_conf_t *) module->config;

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->delayed = FALSE;
    event->type = NX_EVENT_READ;
    event->priority = module->priority;
    nx_event_add(event);
}



static void im_testgen_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_testgen_read(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_im_testgen_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    im_testgen_config,		// config
    im_testgen_start,		// start
    NULL,	 		// stop
    im_testgen_pause,		// pause
    im_testgen_resume,		// resume
    NULL,			// init
    NULL,			// shutdown
    im_testgen_event,		// event
    NULL,			// info
    NULL,			// exports
};
