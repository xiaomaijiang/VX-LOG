/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <unistd.h>
#include <apr_lib.h>
#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/date.h"

#include "im_mark.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_MARK_DEFAULT_MARK_INTERVAL 30 /* minutes */
#define IM_MARK_DEFAULT_MARK "-- MARK --"

static void im_mark_read(nx_module_t *module)
{
    nx_im_mark_conf_t *imconf;
    nx_logdata_t *logdata;
    nx_event_t *event;
    char buf[250];
    char tmpstr[30];
    apr_time_t now;
    nx_value_t *val;
    const nx_string_t *hoststr;

    ASSERT(module != NULL);

    imconf = (nx_im_mark_conf_t *) module->config;
    imconf->event = NULL;

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }
 
    apr_snprintf(buf, sizeof(buf), "%s", imconf->mark);

    logdata = nx_logdata_new();

    now = apr_time_now();
    nx_date_to_iso(tmpstr, sizeof(tmpstr), now);
    nx_string_append(logdata->raw_event, tmpstr, -1);
    nx_string_append(logdata->raw_event, " ", 1);
    nx_logdata_set_datetime(logdata, "EventTime", now);

    hoststr = nx_get_hostname();
    nx_string_append(logdata->raw_event, hoststr->buf, (int) hoststr->len);
    nx_string_append(logdata->raw_event, " ", 1);
    val = nx_value_new(NX_VALUE_TYPE_STRING);
    val->string = nx_string_clone(hoststr);
    nx_logdata_set_field_value(logdata, "Hostname", val);

    nx_string_append(logdata->raw_event, "INFO ", 5);
    nx_string_append(logdata->raw_event, buf, -1);

    nx_logdata_set_string(logdata, "Message", buf);
    nx_logdata_set_integer(logdata, "SeverityValue", NX_LOGLEVEL_INFO);
    nx_logdata_set_string(logdata, "Severity", nx_loglevel_to_string(NX_LOGLEVEL_INFO));
    nx_logdata_set_string(logdata, "SourceName", PACKAGE);
    nx_logdata_set_integer(logdata, "ProcessID", imconf->pid);
    //nx_logdata_to_syslog_rfc3164(logdata);
    nx_module_add_logdata_input(module, NULL, logdata);

    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->type = NX_EVENT_READ;
    event->delayed = TRUE;
    event->time = now + APR_USEC_PER_SEC * imconf->mark_interval * 60;
    event->priority = module->priority;
    nx_event_add(event);
}



static void im_mark_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_mark_conf_t *imconf;
    unsigned int mark_interval;
    const char *ptr;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_mark_conf_t));
    module->config = imconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "MarkInterval") == 0 )
	{
	    if ( imconf->mark_interval != 0 )
	    {
		nx_conf_error(curr, "MarkInterval is already defined");
	    }
	    for ( ptr = curr->args; *ptr != '\0'; ptr++ )
	    {
		if ( ! apr_isdigit(*ptr) )
		{
		    nx_conf_error(curr, "invalid MarkInterval '%s', digit expected", curr->args);
		}
	    }	    
	    if ( sscanf(curr->args, "%u", &mark_interval) != 1 )
	    {
		nx_conf_error(curr, "invalid MarkInterval '%s'", curr->args);
	    }
	    imconf->mark_interval = (int) mark_interval;
	}
	else if ( strcasecmp(curr->directive, "Mark") == 0 )
	{
	    if ( imconf->mark != NULL )
	    {
		nx_conf_error(curr, "Mark is already defined");
	    }
	    imconf->mark = curr->args;
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( imconf->mark_interval == 0 )
    {
	imconf->mark_interval = IM_MARK_DEFAULT_MARK_INTERVAL;
    }
    if ( imconf->mark == NULL )
    {
	imconf->mark = IM_MARK_DEFAULT_MARK;
    }
}



static void im_mark_start(nx_module_t *module)
{
    nx_im_mark_conf_t *imconf;
    nx_event_t *event;
 
    ASSERT(module->config != NULL);

    imconf = (nx_im_mark_conf_t *) module->config;

    log_debug("mark interval: %d", imconf->mark_interval);
  
    imconf->pid = (int) getpid();

    ASSERT(imconf->event == NULL);
    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->type = NX_EVENT_READ;
    event->delayed = TRUE;
    event->time = apr_time_now() + APR_USEC_PER_SEC * imconf->mark_interval * 60;
    event->priority = module->priority;
    nx_event_add(event);
}



static void im_mark_stop(nx_module_t *module)
{
    nx_im_mark_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);
    imconf = (nx_im_mark_conf_t *) module->config;

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}



static void im_mark_pause(nx_module_t *module)
{
    nx_im_mark_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_mark_conf_t *) module->config;

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}



static void im_mark_resume(nx_module_t *module)
{
    nx_im_mark_conf_t *imconf;
    nx_event_t *event;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_mark_conf_t *) module->config;

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



static void im_mark_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_mark_read(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_im_mark_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    im_mark_config,		// config
    im_mark_start,		// start
    im_mark_stop, 		// stop
    im_mark_pause,		// pause
    im_mark_resume,		// resume
    NULL,			// init
    NULL,			// shutdown
    im_mark_event,		// event
    NULL,			// info
    NULL,			// exports
};
