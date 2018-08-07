/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_portable.h>

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/resource.h"

#include "im_kernel.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#ifdef HAVE_SYS_KLOG_H
# include <sys/klog.h>
#endif

// FIXME: make these configurable maybe
#define IM_KERNEL_READ_INTERVAL 1


static void im_kernel_read(nx_module_t *module)
{
    nx_event_t *event;
    nx_im_kernel_conf_t *imconf;
    nx_logdata_t *logdata;
    boolean got_data = FALSE;
    int bytesread = 0;

    ASSERT(module != NULL);
    imconf = (nx_im_kernel_conf_t *) module->config;
    imconf->event = NULL;

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not reading any more data", module->name);
	return;
    }

    if ( (logdata = module->input.inputfunc->func(&(module->input), module->input.inputfunc->data)) != NULL )
    {
	nx_module_add_logdata_input(module, NULL, logdata);
	got_data = TRUE;
    }
    else
    {
#ifdef HAVE_KLOGCTL
	if ( klogctl(9, NULL, 0) > 0 )
	{
	    module->input.bufstart = 0;
	    bytesread = klogctl(2, module->input.buf, module->input.bufsize);
	    if ( bytesread < module->input.bufsize )
	    {
		module->input.buf[bytesread] = '\0';
	    }
	    log_debug("read %d bytes from kernel log buffer", bytesread);
	    //log_debug("read: [%s]", module->input.buf);
	}
	else
	{
	    log_debug("no data available in kernel log buffer");
	}
#endif
	if ( bytesread < 0 )
	{
	    throw_errno("klogctl failed");
	}
	else if ( bytesread > 0 )
	{
	    module->input.buflen = (apr_size_t) bytesread;

	    if ( (logdata = module->input.inputfunc->func(&(module->input), module->input.inputfunc->data)) != NULL )
	    {
		//log_debug("read2: [%s]", logdata->data);
		nx_logdata_set_string(logdata, "SourceName", "kernel");
		nx_module_add_logdata_input(module, NULL, logdata);
		got_data = TRUE;
	    }
	}
    }
    event = nx_event_new();
    event->module = module;
    event->type = NX_EVENT_READ;
    if ( got_data != TRUE )
    {
	event->delayed = TRUE;
	event->time = apr_time_now() + APR_USEC_PER_SEC * imconf->read_interval;
    }
    else
    {
	event->delayed = FALSE;
    }
    event->priority = module->priority;
    nx_event_add(event);
    imconf->event = event;
}



static void im_kernel_config(nx_module_t *module)
{
    nx_im_kernel_conf_t *imconf;
    const nx_directive_t *curr;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_kernel_conf_t));
    module->config = imconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	// FIXME: add config options
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }
    module->input.inputfunc = nx_module_input_func_lookup("linebased");
    ASSERT(module->input.inputfunc != NULL);
    imconf->read_interval = IM_KERNEL_READ_INTERVAL;
}



static void im_kernel_init(nx_module_t *module)
{
#ifdef HAVE_KLOGCTL
    int log_buf_size;

    ASSERT(module != NULL);

    log_buf_size = klogctl(10, NULL, 0);
    log_debug("kernel log buffer size is %d bytes", log_buf_size);
    module->input.buf = apr_pcalloc(module->pool, (apr_size_t) log_buf_size);
    module->input.bufsize = log_buf_size;
#endif
}



static void im_kernel_start(nx_module_t *module)
{
    nx_im_kernel_conf_t *imconf;
    nx_event_t *event;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_kernel_conf_t *) module->config;

    ASSERT(imconf->event == NULL);
    event = nx_event_new();
    imconf->event = event;
    event->module = module;
    event->delayed = FALSE;
    event->type = NX_EVENT_READ;
    event->priority = module->priority;
    nx_event_add(event);

    log_debug("im_kernel started");
}



static void im_kernel_stop(nx_module_t *module)
{
    nx_im_kernel_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);
    imconf = (nx_im_kernel_conf_t *) module->config;

    imconf->event = NULL;
}



static void im_kernel_pause(nx_module_t *module)
{
    nx_im_kernel_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_kernel_conf_t *) module->config;

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}



static void im_kernel_resume(nx_module_t *module)
{
    nx_im_kernel_conf_t *imconf;
    nx_event_t *event;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_kernel_conf_t *) module->config;

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



static void im_kernel_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_READ:
	    im_kernel_read(module);
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_im_kernel_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    "CAP_SYS_ADMIN",		// capabilities needed on linux //FIXME: CAP_SYSLOG for newer kernels
    im_kernel_config,		// config
    im_kernel_start,		// start
    im_kernel_stop, 		// stop
    im_kernel_pause,		// pause
    im_kernel_resume,		// resume
    im_kernel_init,		// init
    NULL,			// shutdown
    im_kernel_event,		// event
    NULL,			// info
    NULL,			// exports
};
