/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"
#include "../../../common/config_cache.h"
#include "../../../common/alloc.h"

#include "im_exec.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE

#define IM_EXEC_DEFAULT_POLL_INTERVAL 1 /* The number of seconds to check the files for new data */
#define IM_EXEC_DEFAULT_RESTART_INTERVAL 1 /* The number of seconds to wait between restarts */
#define IM_EXEC_DEFAULT_EAGAIN_WAIT 1000

static void im_exec_stop(nx_module_t *module);

#ifdef WIN32
static void im_exec_add_read_event(nx_module_t *module, int delay)
{
    nx_event_t *event;
    nx_im_exec_conf_t *imconf;

    imconf = (nx_im_exec_conf_t *) module->config;
    ASSERT(imconf->event == NULL);

    log_debug("im_exec_add_read_event with delay %d", delay);

    event = nx_event_new();
    event->module = module;
    event->delayed = TRUE;
    event->time = apr_time_now() + delay;
    event->type = NX_EVENT_READ;
    event->priority = module->priority;
    nx_event_add(event);
    imconf->event = event;
}
#endif


static void im_exec_add_restart_event(nx_module_t *module)
{
    nx_event_t *event;
    nx_im_exec_conf_t *imconf;

    imconf = (nx_im_exec_conf_t *) module->config;

    nx_module_stop_self(module);

    if ( imconf->restart == TRUE )
    {
	event = nx_event_new();
	event->module = module;
	event->delayed = TRUE;
	event->time = apr_time_now() + IM_EXEC_DEFAULT_RESTART_INTERVAL * APR_USEC_PER_SEC;
	event->type = NX_EVENT_MODULE_START;
	event->priority = module->priority;
	nx_event_add(event);
    }
}



static void im_exec_fill_buffer(nx_module_t *module, nx_module_input_t *input)
{
    apr_status_t rv;
    apr_size_t len;
    nx_im_exec_conf_t *imconf;

    ASSERT(input != NULL);
    ASSERT(input->buf != NULL);
    ASSERT(input->module != NULL);
    ASSERT(input->desc_type == APR_POLL_FILE);
    ASSERT(input->desc.f != NULL);

    imconf = (nx_im_exec_conf_t *) module->config;

    if ( input->bufstart == input->bufsize )
    {
	input->bufstart = 0;
	input->buflen = 0;
    }
    if ( input->buflen == 0 )
    {
	input->bufstart = 0;
    }

    ASSERT(input->bufstart + input->buflen <= input->bufsize);
    len = (apr_size_t) (input->bufsize - (input->buflen + input->bufstart));

    rv = apr_file_read(input->desc.f, input->buf + input->bufstart + input->buflen, &len);

    if ( rv != APR_SUCCESS )
    {
	if ( APR_STATUS_IS_EOF(rv) )
	{
	    throw_msg("Module %s got EOF, process exited? ", input->module->name);
	}
	else if ( APR_STATUS_IS_EAGAIN(rv) )
	{ 
#ifdef WIN32
	    // on windows EAGAIN is normal because we are using NON-BLOCKING reads
	    // unfortunately it gives EAGAIN also when there is no data 
	    if ( len > 0 )
	    {
		imconf->delay = IM_EXEC_DEFAULT_EAGAIN_WAIT;
	    }
	    else
	    {
		imconf->delay *= 2;
		if ( imconf->delay > (int) (APR_USEC_PER_SEC * IM_EXEC_DEFAULT_POLL_INTERVAL) )
		{
		    imconf->delay = (int) (APR_USEC_PER_SEC * IM_EXEC_DEFAULT_POLL_INTERVAL);
		}
	    }
	    im_exec_add_read_event(input->module, imconf->delay);
	    log_debug("got EAGAIN");
#else
	    log_error("got EAGAIN for blocking read in module %s", input->module->name);
#endif
	    ASSERT(len == 0);
	    return;
	}
	else
	{
	    throw(rv, "Module %s couldn't read from pipe", input->module->name);
	}
    }
    else
    {
	log_debug("im_exec read %d bytes", (int) len);
	imconf->delay = IM_EXEC_DEFAULT_EAGAIN_WAIT;
    }
    input->buflen += (int) len;
    ASSERT(input->buflen <= input->bufsize);
}



static void im_exec_read(nx_module_t *module)
{
    nx_logdata_t *logdata;
    nx_im_exec_conf_t *imconf;
    int evcnt = 0;

    ASSERT(module != NULL);
    imconf = (nx_im_exec_conf_t *) module->config;
    imconf->event = NULL;

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s is not running, not reading any more data", module->name);
	return;
    }
    
    im_exec_fill_buffer(module, &(module->input));
    while ( (logdata = module->input.inputfunc->func(&(module->input), module->input.inputfunc->data)) != NULL )
    {
	nx_module_add_logdata_input(module, &(module->input), logdata);
	evcnt++;
    }
#ifdef WIN32
    if ( nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING )
    {
	if ( imconf->event == NULL )
	{
	    if ( evcnt == 0 )
	    {
		im_exec_add_read_event(module, (int) (APR_USEC_PER_SEC * IM_EXEC_DEFAULT_POLL_INTERVAL));
	    }
	    else
	    {
		im_exec_add_read_event(module, 0);
	    }
	}
    }
#endif
}



static void im_exec_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_im_exec_conf_t *imconf;
    int argc = 0;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    imconf = apr_pcalloc(module->pool, sizeof(nx_im_exec_conf_t));
    module->config = imconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "command") == 0 )
	{
	    if ( imconf->cmd != NULL )
	    {
		nx_conf_error(curr, "Command is already defined");
	    }
	    if ( argc >= NX_IM_EXEC_MAX_ARGC )
	    {
		nx_conf_error(curr, "too many arguments");
	    }
	    imconf->cmd = apr_pstrdup(module->pool, curr->args);
	    imconf->argv[argc] = imconf->cmd;
	    argc++;
	}
	else if ( strcasecmp(curr->directive, "arg") == 0 )
	{
	    if ( argc >= NX_IM_EXEC_MAX_ARGC )
	    {
		nx_conf_error(curr, "too many arguments");
	    }
	    imconf->argv[argc] = apr_pstrdup(module->pool, curr->args);
	    argc++;
	}
	else if ( strcasecmp(curr->directive, "InputType") == 0 )
	{
	    if ( imconf->inputfunc != NULL )
	    {
		nx_conf_error(curr, "InputType is already defined");
	    }

	    if ( curr->args != NULL )
	    {
		imconf->inputfunc = nx_module_input_func_lookup(curr->args);
	    }
	    if ( imconf->inputfunc == NULL )
	    {
		nx_conf_error(curr, "Invalid InputType '%s'", curr->args);
	    }
	}
	else if ( strcasecmp(curr->directive, "restart") == 0 )
	{
	}
	else
	{
	    nx_conf_error(curr, "invalid keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    nx_cfg_get_boolean(module->directives, "restart", &(imconf->restart));

    if ( imconf->inputfunc == NULL )
    {
	imconf->inputfunc = nx_module_input_func_lookup("linebased");
    }
    ASSERT(imconf->inputfunc != NULL);

    if ( imconf->cmd == NULL )
    {
	nx_conf_error(module->directives, "'Command' missing for module im_exec");
    }

    imconf->delay = IM_EXEC_DEFAULT_EAGAIN_WAIT;
    module->input.pool = nx_pool_create_child(module->pool);
}



static void im_exec_start(nx_module_t *module)
{
    nx_im_exec_conf_t *imconf;
    apr_status_t rv;
    apr_procattr_t *pattr;
    apr_exit_why_e exitwhy;
    int exitval;

    ASSERT(module->config != NULL);

    imconf = (nx_im_exec_conf_t *) module->config;

    if ( module->input.desc.f == NULL )
    {
	CHECKERR_MSG(apr_procattr_create(&pattr, module->input.pool),
		     "apr_procattr_create() failed");
	CHECKERR_MSG(apr_procattr_error_check_set(pattr, 1),
		     "apr_procattr_error_check_set() failed");
#ifdef WIN32
	CHECKERR_MSG(apr_procattr_io_set(pattr, APR_NO_PIPE, APR_FULL_NONBLOCK,
					 APR_NO_PIPE),
		     "apr_procattr_io_set() failed");
#else
	CHECKERR_MSG(apr_procattr_io_set(pattr, APR_NO_PIPE, APR_FULL_BLOCK,
					 APR_NO_PIPE),
		     "apr_procattr_io_set() failed");
#endif
	CHECKERR_MSG(apr_procattr_cmdtype_set(pattr, APR_PROGRAM_ENV),
		     "apr_procattr_cmdtype_set() failed");
	CHECKERR_MSG(apr_proc_create(&(imconf->proc), imconf->cmd, (const char* const*)imconf->argv,
				     NULL, (apr_procattr_t*)pattr, module->input.pool),
		     "couldn't execute process %s", imconf->cmd);
	if ( (rv = apr_proc_wait(&(imconf->proc), &exitval, &exitwhy, APR_NOWAIT)) != APR_SUCCESS )
	{
	    if ( APR_STATUS_IS_CHILD_DONE(rv) )
	    {
		throw(rv, "im_exec process %s exited %s with exitval: %d",
		      imconf->cmd, exitwhy == APR_PROC_EXIT ? "normally" : "due to a signal",
		      exitval);
	    }
	    else
	    {
		// still running OK
	    }
	}
	log_debug("im_exec successfully spawned process");
	imconf->running = TRUE;

	module->input.desc_type = APR_POLL_FILE;
	module->input.desc.f = imconf->proc.out;
	module->input.module = module;
	module->input.inputfunc = imconf->inputfunc;
#ifdef WIN32
	im_exec_add_read_event(module, 0);
#else
	nx_module_pollset_add_file(module, module->input.desc.f, APR_POLLIN | APR_POLLHUP);
#endif
    }
    else
    {
	log_debug("%s already executed", imconf->cmd);
    }
#ifndef WIN32
    nx_module_add_poll_event(module);
#endif
}



static void im_exec_stop(nx_module_t *module)
{
    nx_im_exec_conf_t *imconf;
    int i;
    boolean sigterm_sent = FALSE;
    apr_exit_why_e exitwhy;
    int exitval;
    int sig_num;
    apr_status_t rv;
    apr_status_t stopped = APR_SUCCESS;

    ASSERT(module != NULL);
    imconf = (nx_im_exec_conf_t *) module->config;
    ASSERT(imconf != NULL);

    log_debug("im_exec stopped");

    if ( module->input.desc.f != NULL )
    {
	apr_file_close(module->input.desc.f);
	apr_pool_clear(module->input.pool);
	module->input.desc.f = NULL;
    }

    if ( imconf->running == TRUE )
    {
	for ( i = 0; i < 50; i++ )
	{
	    if ( (rv = apr_proc_wait(&(imconf->proc), &exitval, &exitwhy, APR_NOWAIT)) != APR_SUCCESS )
	    {
		if ( APR_STATUS_IS_CHILD_DONE(rv) )
		{
		    rv = APR_SUCCESS;
		    break;
		}
		else if ( i >= 30 )
		{ // still running, kill it after 3 sec
		    if ( sigterm_sent == FALSE )
		    {
			sigterm_sent = TRUE;
#ifdef WIN32
			sig_num = 1;
#else
			sig_num = SIGTERM;
#endif
		    }
		    else
		    {
			if ( i <= 31 )
			{
			    log_warn("process %s did not exit, killing it.", imconf->cmd);
			}
#ifdef WIN32
			sig_num = 1;
#else
			sig_num = SIGKILL;
#endif
		    }
		    if ( (rv = apr_proc_kill(&(imconf->proc), sig_num)) != APR_SUCCESS )
		    {
			stopped = rv;
		    }
		}
	    }
	    apr_sleep(APR_USEC_PER_SEC / 10);
	}
	if ( !((stopped == APR_SUCCESS) || APR_STATUS_IS_ENOPROC(stopped)) )
	{
	    log_aprerror(stopped, "im_exec couldn't stop process");
	}
    }
    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}



static void im_exec_pause(nx_module_t *module)
{
    nx_im_exec_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_exec_conf_t *) module->config;

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
}



static void im_exec_resume(nx_module_t *module)
{
    nx_im_exec_conf_t *imconf;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    imconf = (nx_im_exec_conf_t *) module->config;

    log_debug("im_exec resumed");

    if ( imconf->event != NULL )
    {
	nx_event_remove(imconf->event);
	nx_event_free(imconf->event);
	imconf->event = NULL;
    }
#ifdef WIN32
    im_exec_add_read_event(module, 0);
#else
    nx_module_add_poll_event(module);
#endif
    //we could directly go to poll here
}



static void im_exec_init(nx_module_t *module)
{
#ifndef WIN32
    nx_module_pollset_init(module);
#endif
}



static void im_exec_event(nx_module_t *module, nx_event_t *event)
{
    nx_im_exec_conf_t *imconf;
    nx_exception_t e;

    ASSERT(event != NULL);

    imconf = (nx_im_exec_conf_t *) module->config;

    switch ( event->type )
    {
	case NX_EVENT_DISCONNECT:
	    log_warn("im_exec process %s exited", imconf->cmd);
	    imconf->running = FALSE;
	    im_exec_add_restart_event(module);
	    break;
	case NX_EVENT_READ:
	    try
	    {
		im_exec_read(module);
	    }
	    catch(e)
	    {
		log_exception(e);
		im_exec_add_restart_event(module);
	    }
	    break;
	case NX_EVENT_POLL:
#ifdef WIN32
	    nx_panic("pollset based im_exec implementation doesn't work on windows");
#else
	    if ( nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING )
	    {
		nx_module_pollset_poll(module, TRUE);
	    }
	    break;
#endif
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_im_exec_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_INPUT,
    NULL,			// capabilities
    im_exec_config,		// config
    im_exec_start,		// start
    im_exec_stop, 		// stop
    im_exec_pause,		// pause
    im_exec_resume,		// resume
    im_exec_init,		// init
    NULL,			// shutdown
    im_exec_event,		// event
    NULL,			// info
    NULL,			// exports
};

