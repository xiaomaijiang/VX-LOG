/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "../../../common/event.h"
#include "../../../common/error_debug.h"

#include "om_exec.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static void om_exec_write(nx_module_t *module)
{
    nx_om_exec_conf_t *omconf;
    nx_logdata_t *logdata;
    apr_size_t nbytes;
    boolean done = FALSE;

    log_debug("om_exec_write");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s not running, not writing any more data", module->name);
	return;
    }

    omconf = (nx_om_exec_conf_t *) module->config;

    do
    {
	if ( module->output.buflen > 0 )
	{
	    nbytes = module->output.buflen;
	    //log_debug("om_exec write %d bytes: [%s]", nbytes, module->output.buf);
	    ASSERT(omconf->desc != NULL);
	    CHECKERR_MSG(apr_file_write(omconf->desc, module->output.buf + module->output.bufstart, &nbytes),
			 "apr_file_write failed in om_exec");
    
	    if ( nbytes < module->output.buflen )
	    {
		nx_module_pollset_add_file(module, omconf->desc,
					   APR_POLLOUT | APR_POLLHUP);
		nx_module_add_poll_event(module);
		done = TRUE;
	    }
	    ASSERT(nbytes <= module->output.buflen);
	    module->output.bufstart += nbytes;
	    module->output.buflen -= nbytes;
	    if ( module->output.buflen == 0 )
	    { // all bytes have been sucessfully written
		module->output.bufstart = 0;
		if ( module->output.logdata != NULL )
		{
		    nx_module_logqueue_pop(module, module->output.logdata);
		    nx_logdata_free(module->output.logdata);
		    module->output.logdata = NULL;
		}
	    }
	}

	if ( module->output.buflen == 0 )
	{
	    if ( (logdata = nx_module_logqueue_peek(module)) != NULL )
	    {
		module->output.logdata = logdata;
		module->output.outputfunc->func(&(module->output),
						module->output.outputfunc->data);
		if ( module->output.buflen == 0 )
		{ // nothing to do in case the data is zero length or already dropped
		    module->output.bufstart = 0;
		    if ( module->output.logdata != NULL )
		    {
			nx_module_logqueue_pop(module, module->output.logdata);
			nx_logdata_free(module->output.logdata);
			module->output.logdata = NULL;
		    }
		}
	    }
	    else
	    {
		done = TRUE;
	    }
	}
    } while ( done != TRUE );
}



static void om_exec_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_om_exec_conf_t *omconf;
    int argc = 0;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    omconf = apr_pcalloc(module->pool, sizeof(nx_om_exec_conf_t));
    module->config = omconf;

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "command") == 0 )
	{
	    if ( omconf->cmd != NULL )
	    {
		nx_conf_error(curr, "Command is already defined");
	    }
	    if ( argc >= NX_OM_EXEC_MAX_ARGC )
	    {
		nx_conf_error(curr, "too many arguments");
	    }
	    omconf->cmd = apr_pstrdup(module->pool, curr->args);
	    omconf->argv[argc] = omconf->cmd;
	    argc++;
	}
	else if ( strcasecmp(curr->directive, "arg") == 0 )
	{
	    if ( argc >= NX_OM_EXEC_MAX_ARGC )
	    {
		nx_conf_error(curr, "too many arguments");
	    }
	    omconf->argv[argc] = apr_pstrdup(module->pool, curr->args);
	    argc++;
	}
	else if ( strcasecmp(curr->directive, "OutputType") == 0 )
	{
	    if ( module->output.outputfunc != NULL )
	    {
		nx_conf_error(curr, "OutputType is already defined");
	    }

	    if ( curr->args != NULL )
	    {
		module->output.outputfunc = nx_module_output_func_lookup(curr->args);
	    }
	    if ( module->output.outputfunc == NULL )
	    {
		nx_conf_error(curr, "Invalid OutputType '%s'", curr->args);
	    }
	}
	else
	{
	    nx_conf_error(curr, "invalid om_exec keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( module->output.outputfunc == NULL )
    {
	module->output.outputfunc = nx_module_output_func_lookup("linebased");
    }
    ASSERT(module->output.outputfunc != NULL);

    if ( omconf->cmd == NULL )
    {
	nx_conf_error(module->directives, "'Command' missing for module om_exec");
    }
}



static void om_exec_start(nx_module_t *module)
{
    nx_om_exec_conf_t *omconf;
    apr_status_t rv;
    apr_procattr_t *pattr;
    apr_proc_t proc;
    apr_exit_why_e exitwhy;
    int exitval;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);

    omconf = (nx_om_exec_conf_t *) module->config;

    if ( omconf->desc == NULL )
    {
	CHECKERR_MSG(apr_procattr_create(&pattr, module->pool),
		     "apr_procattr_create() failed");
	CHECKERR_MSG(apr_procattr_error_check_set(pattr, 1),
		     "apr_procattr_error_check_set() failed");
	CHECKERR_MSG(apr_procattr_io_set(pattr, APR_FULL_BLOCK, APR_NO_PIPE, APR_NO_PIPE),
		     "apr_procattr_io_set() failed");
	CHECKERR_MSG(apr_procattr_cmdtype_set(pattr, APR_PROGRAM_ENV),
		     "apr_procattr_cmdtype_set() failed");
	CHECKERR_MSG(apr_proc_create(&proc, omconf->cmd, (const char* const*) omconf->argv,
				     NULL, (apr_procattr_t*)pattr, module->pool),
		     "couldn't execute process %s", omconf->cmd);

	if ( (rv = apr_proc_wait(&proc, &exitval, &exitwhy, APR_NOWAIT)) != APR_SUCCESS )
	{
	    if ( APR_STATUS_IS_CHILD_DONE(rv) )
	    {
		throw(rv, "om_exec process %s exited %s with exitval: %d",
		      omconf->cmd, exitwhy == APR_PROC_EXIT ? "normally" : "due to a signal", exitval);
	    }
	    else
	    {
		// still running OK
	    }
	}
	log_debug("om_exec successfully spawned process");

	omconf->desc = proc.in;
	ASSERT(proc.in != NULL);
    }
    else
    {
	log_debug("%s already started", omconf->cmd);
    }
}



static void om_exec_stop(nx_module_t *module)
{
    nx_om_exec_conf_t *omconf;

    ASSERT(module != NULL);

    omconf = (nx_om_exec_conf_t *) module->config;

    if ( omconf->desc != NULL )
    {
	nx_module_pollset_remove_file(module, omconf->desc);
	apr_file_close(omconf->desc);
    }
}



static void om_exec_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void om_exec_event(nx_module_t *module, nx_event_t *event)
{
    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_DATA_AVAILABLE:
	    om_exec_write(module);
	    break;
	case NX_EVENT_POLL:
	    if ( nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING )
	    {
		nx_module_pollset_poll(module, FALSE);
	    }
	    break;
	default:
	    nx_panic("invalid event type: %d", event->type);
    }
}



NX_MODULE_DECLARATION nx_om_exec_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_exec_config,		// config
    om_exec_start,		// start
    om_exec_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_exec_init,		// init
    NULL,			// shutdown
    om_exec_event,		// event
    NULL,			// info
    NULL,			// exports
};

