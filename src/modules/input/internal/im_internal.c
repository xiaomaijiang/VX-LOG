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
#include "../../../common/date.h"

#include "im_internal.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static void im_internal_log(nx_module_t *module, void *data)
{
    nx_im_internal_conf_t *imconf;
    nx_logdata_t *logdata;
    nx_internal_log_t *log;
    char tmpstr[30];
    apr_time_t now;
    nx_value_t *val;
    char *loglevelstr;
    const nx_string_t *hoststr;

    ASSERT(module != NULL);
    ASSERT(data != NULL);

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    { //WARNING events are dropped if module is paused
	// TODO: it should have its own queue.
	return;
    }

    log = (nx_internal_log_t *) data;
    ASSERT(log->msg != NULL);
    imconf = (nx_im_internal_conf_t *) module->config;

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

    loglevelstr = nx_loglevel_to_string(log->level);
    nx_logdata_set_integer(logdata, "SeverityValue", log->level);
    nx_logdata_set_string(logdata, "Severity", loglevelstr);
    nx_string_append(logdata->raw_event, loglevelstr, -1);
    nx_string_append(logdata->raw_event, " ", 1);

    nx_logdata_set_string(logdata, "SourceName", PACKAGE);
    nx_logdata_set_integer(logdata, "ProcessID", imconf->pid);
    nx_logdata_set_string(logdata, "Message", log->msg);
    nx_string_append(logdata->raw_event, log->msg, -1);

    if ( log->errorcode != APR_SUCCESS )
    {
	nx_logdata_set_integer(logdata, "ErrorCode", log->errorcode);
    }

    nx_module_add_logdata_input(module, NULL, logdata);
}



static void im_internal_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_internal_conf_t *imconf;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_internal_conf_t));
    module->config = imconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    imconf->pid = (int) getpid();
}



static void im_internal_event(nx_module_t *module, nx_event_t *event)
{
    im_internal_log(module, (void *) event);
}



NX_MODULE_DECLARATION nx_im_internal_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    im_internal_config,		// config
    NULL,			// start
    NULL,	 		// stop
    NULL,			// pause
    NULL,			// resume
    NULL,			// init
    NULL,			// shutdown
    im_internal_event,		// event
    NULL,			// info
    NULL,			// exports
};
