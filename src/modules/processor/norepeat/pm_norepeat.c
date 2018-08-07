/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>
#include <unistd.h>
#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"

#include "pm_norepeat.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static boolean nx_logdata_eq(nx_pm_norepeat_conf_t *modconf, 
			     nx_logdata_t *l1,
			     nx_logdata_t *l2)
{
    nx_value_t val1, val2;
    boolean retval = FALSE;
    const char *field;
    boolean f1, f2;
    int i;

    ASSERT(l1 != NULL);
    ASSERT(l2 != NULL);

    if ( modconf->fields == NULL )
    {
	if ( (nx_logdata_get_field_value(l1, "Message", &val1) == TRUE) &&
	     (nx_logdata_get_field_value(l2, "Message", &val2) == TRUE) &&
	     (nx_value_eq(&val1, &val2) == TRUE) )
	{
	    retval = TRUE;
	}
    }
    else
    {
	retval = TRUE;
	for ( i = 0; i < modconf->fields->nelts; i++ )
	{
	    field = ((const char **)modconf->fields->elts)[i];
	    f1 = nx_logdata_get_field_value(l1, field, &val1);
	    f2 = nx_logdata_get_field_value(l2, field, &val2);
	    if ( (f1 == FALSE) && (f2 == FALSE) )
	    {
		continue;
	    }
	    else if ( (f1 == TRUE) && (f2 == TRUE) )
	    {
		if ( nx_value_eq(&val1, &val2) != TRUE )
		{
		    retval = FALSE;
		}
	    }
	    else
	    {
		retval = FALSE;
	    }
	}
    }

    return ( retval );
}



static void pm_norepeat_add_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_pm_norepeat_conf_t *modconf;

    modconf = (nx_pm_norepeat_conf_t *) module->config;

    if ( modconf->event == NULL )
    {
	event = nx_event_new();
	event->module = module;
	event->delayed = TRUE;
	event->type = NX_EVENT_WRITE;
	event->time = apr_time_now() + APR_USEC_PER_SEC;
	event->priority = module->priority;
	nx_event_add(event);
    }
    else
    {
	nx_event_remove(modconf->event);
	modconf->event->time = apr_time_now() + APR_USEC_PER_SEC;
	nx_event_add(modconf->event);
    }
}



static void pm_norepeat_log_repeat(nx_module_t *module)
{
    nx_logdata_t *logdata;
    char msgbuf[256];
    int len;
    nx_pm_norepeat_conf_t *modconf;
    nx_value_t eventtime;

    modconf = (nx_pm_norepeat_conf_t *) module->config;

    if ( modconf->repeatcnt == 1 )
    {
	module->queue->needpop = FALSE;
	nx_module_progress_logdata(module, modconf->logdata);
	module->queue->needpop = TRUE;
	modconf->logdata = NULL;
    }
    else if ( modconf->repeatcnt > 1 )
    {
	len = apr_snprintf(msgbuf, sizeof(msgbuf), "last message repeated %d times",
			   modconf->repeatcnt);
	logdata = nx_logdata_new_logline(msgbuf, len);
	nx_logdata_set_string(logdata, "Message", msgbuf);
	nx_logdata_set_integer(logdata, "SeverityValue", NX_LOGLEVEL_INFO);
	nx_logdata_set_string(logdata, "Severity", nx_loglevel_to_string(NX_LOGLEVEL_INFO));
	if ( nx_logdata_get_field_value(modconf->logdata, "EventTime", &eventtime) == TRUE )
	{
	    ASSERT(eventtime.type == NX_VALUE_TYPE_DATETIME);
	    nx_logdata_set_datetime(logdata, "EventTime", eventtime.datetime);
	}
	else
	{
	    nx_logdata_set_datetime(logdata, "EventTime", apr_time_now());
	}
	nx_logdata_set_string(logdata, "SourceName", PACKAGE);
	nx_logdata_set_integer(logdata, "ProcessID", modconf->pid);
	module->queue->needpop = FALSE;
	nx_module_progress_logdata(module, logdata);
	module->queue->needpop = TRUE;
	nx_logdata_free(modconf->logdata);
	modconf->logdata = NULL;
    }
    else // modconf->repeatcnt == 0
    {
	nx_logdata_free(modconf->logdata);
	modconf->logdata = NULL;
    }	
    modconf->repeatcnt = 0;
}



static void pm_norepeat_write(nx_module_t *module)
{
    nx_pm_norepeat_conf_t *modconf;

    ASSERT(module != NULL);

    modconf = (nx_pm_norepeat_conf_t *) module->config;

    if ( modconf->logdata != NULL )
    {
	pm_norepeat_log_repeat(module);
    }
}



static nx_logdata_t *pm_norepeat_process(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_pm_norepeat_conf_t *modconf;
 
    ASSERT ( logdata != NULL );

    modconf = (nx_pm_norepeat_conf_t *) module->config;

    log_debug("nx_pm_norepeat_process()");

    //log_debug("processing: [%s]", logdata->data);

    if ( modconf->logdata == NULL )
    {
	modconf->logdata = nx_logdata_clone(logdata);
    }
    else
    {
	if ( nx_logdata_eq(modconf, modconf->logdata, logdata) == TRUE )
	{
	    (modconf->repeatcnt)++;
	    pm_norepeat_add_event(module);
	    nx_module_logqueue_pop(module, logdata);
	    nx_logdata_free(logdata);
	    return ( NULL );
	}
	else
	{
	    pm_norepeat_log_repeat(module);
	    ASSERT(modconf->logdata == NULL);
	    modconf->logdata = nx_logdata_clone(logdata);
	    modconf->repeatcnt = 0;
	}
    }

    return ( logdata );
}



static void pm_norepeat_data_available(nx_module_t *module)
{
    nx_logdata_t *logdata;
    nx_pm_norepeat_conf_t *modconf;

    log_debug("nx_pm_norepeat_data_available()");
    
    modconf = (nx_pm_norepeat_conf_t *) module->config;

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not processing any more data", module->name);
	return;
    }

    if ( (logdata = nx_module_logqueue_peek(module)) == NULL )
    {
	return;
    }

    logdata = pm_norepeat_process(module, logdata);

    // add logdata to the next modules' queue
    if ( logdata != NULL )
    {
	nx_module_progress_logdata(module, logdata);
    }
}



static void pm_norepeat_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_pm_norepeat_conf_t *modconf;
    const char *start, *ptr, *end;
    char *field = NULL;
    apr_size_t len;

    modconf = apr_pcalloc(module->pool, sizeof(nx_pm_norepeat_conf_t));
    module->config = modconf;

    curr = module->directives;
    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "CheckFields") == 0 )
	{
	    if ( modconf->fields != NULL )
	    {
		nx_conf_error(curr, "CheckFields already defined");
	    }
	    ptr = curr->args;
	    for ( ; (ptr != NULL) && apr_isspace(*ptr); (ptr)++ );
	    if ( (curr->args == NULL) || (strlen(ptr) == 0) )
	    {
		nx_conf_error(curr, "value missing for CheckFields");
	    }
	    end = ptr + strlen(ptr);
	    modconf->fields = apr_array_make(module->pool, 5, sizeof(const char *));
	    while ( *ptr != '\0' )
	    {
		start = ptr;
		for ( ; !((*ptr == '\0') || apr_isspace(*ptr) || (*ptr == ',')); ptr++ );
		if ( ptr > start )
		{
		    len = (apr_size_t) (ptr - start + 1);
		    field = apr_palloc(module->pool, len);
		    apr_cpystrn(field, start, len);
		}
		*((const char **) apr_array_push(modconf->fields)) = field;
		for ( ; apr_isspace(*ptr) || (*ptr == ','); ptr++ );
	    }
	}
	else
	{
	    nx_conf_error(curr, "invalid pm_norepeat keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    modconf->pid = (int) getpid();
}



static void pm_norepeat_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    pm_norepeat_data_available(module);
	    break;
	case NX_EVENT_WRITE:
	    pm_norepeat_write(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_pm_norepeat_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_PROCESSOR,
    NULL,			// capabilities
    pm_norepeat_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    pm_norepeat_event,		// event
    NULL,			// info
    NULL,			// exports
};
