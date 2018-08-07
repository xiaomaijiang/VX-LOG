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
#include "../../../common/expr-parser.h"
#include "../../../common/alloc.h"

#include "om_file.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


static void om_file_create_dir(nx_module_t *module, const char *filename)
{
    char pathname[APR_PATH_MAX + 1];
    char *idx;
    apr_pool_t *pool;

    ASSERT(filename != NULL);

    idx = strrchr(filename, '/');
#ifdef WIN32
    if ( idx == NULL ) 
    {
        idx = strrchr(filename, '\\');
    }
#endif

    if ( idx == NULL )
    {
	log_debug("no directory in filename, cannot create");
	return;
    }

    pool = nx_pool_create_child(module->pool);
    ASSERT(sizeof(pathname) >= (size_t) (idx - filename + 1));
    apr_cpystrn(pathname, filename, (size_t) (idx - filename + 1));
    
    CHECKERR_MSG(apr_dir_make_recursive(pathname, APR_OS_DEFAULT, pool), 
		 "CreateDir is TRUE but couldn't create directory: %s", pathname);
    log_debug("directory '%s' created", pathname);
    apr_pool_destroy(pool);
}



void om_file_open(nx_module_t *module)
{
    nx_om_file_conf_t *omconf;
    apr_os_file_t fd;
    int flags;

    ASSERT(module != NULL);
    ASSERT(module->config != NULL);
    omconf = (nx_om_file_conf_t *) module->config;

    log_debug("om_file opening '%s'", omconf->filename);

    if ( omconf->createdir == TRUE )
    {
	om_file_create_dir(module, omconf->filename);
    }

    CHECKERR_MSG(apr_file_open(&(omconf->file), omconf->filename,
			       APR_WRITE | APR_CREATE | APR_APPEND,
			       APR_OS_DEFAULT, omconf->fpool),
		 "failed to open %s", omconf->filename);

    CHECKERR_MSG(apr_os_file_get(&fd, omconf->file),
		 "failed to get fd for %s", omconf->filename);

	//TODO: iocsocket() + FIONBIO on windows for non-blocking sockets
#ifndef WIN32
    if ( (flags = fcntl(fd, F_GETFL, 0)) == -1 )
    {
	throw_errno("fcnt failed");
    }
    if ( fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1 )
    {
	throw_errno("fcnt failed");
    }
#endif
}



void om_file_close(nx_module_t *module)
{
    nx_om_file_conf_t *omconf;

    omconf = (nx_om_file_conf_t *) module->config;

    if ( omconf->file != NULL )
    {
	if ( omconf->in_pollset == TRUE )
	{
	    nx_module_pollset_remove_file(module, omconf->file);
	    omconf->in_pollset = FALSE;
	}
	apr_file_close(omconf->file);
	omconf->file = NULL;
    }
    apr_pool_clear(omconf->fpool);
}



static void om_file_truncate(apr_file_t *file)
{
    apr_off_t offset = 0;

    CHECKERR_MSG(apr_file_seek(file, APR_SET, &offset), "om_file couldn't seek in output");
    CHECKERR_MSG(apr_file_trunc(file, 0), "om_file couldn't truncate output");
}



static boolean om_file_update_filename(nx_module_t *module, nx_logdata_t *logdata)
{
    nx_om_file_conf_t *omconf;
    nx_expr_eval_ctx_t ctx;
    nx_value_t value;

    omconf = (nx_om_file_conf_t *) module->config;

    if ( omconf->filename_expr != NULL )
    {
	ctx.module = module;
	ctx.logdata = logdata;

	nx_expr_evaluate(&ctx, &value, omconf->filename_expr);
	if ( value.defined == FALSE )
	{
	    throw_msg("%s File directive evaluated to undef", module->name);
	}

	if ( value.type != NX_VALUE_TYPE_STRING )
	{
	    throw_msg("%s File directive evaluated to '%', string type required",
		      module->name, nx_value_type_to_string(value.type));
	}
    
	if ( strcmp(value.string->buf, omconf->filename) == 0 )
	{ // filename didn't change
	    nx_value_kill(&value);
	    return ( FALSE );
	}
	// update filename
	apr_cpystrn(omconf->filename, value.string->buf, sizeof(omconf->filename));
	nx_value_kill(&value);
	return ( TRUE );
    }

    return ( FALSE );
}



static void om_file_write(nx_module_t *module)
{
    nx_om_file_conf_t *omconf;
    nx_logdata_t *logdata;
    apr_size_t nbytes;
    boolean done = FALSE;

    log_debug("om_file_write");

    if ( nx_module_get_status(module) != NX_MODULE_STATUS_RUNNING )
    {
	log_debug("module %s stopped, not writing any more data", module->name);
	return;
    }

    omconf = (nx_om_file_conf_t *) module->config;

    do
    {
	if ( module->output.buflen > 0 )
	{
	    nbytes = module->output.buflen;
	    //log_debug("om_file write %d bytes: [%s]", nbytes, module->output.buf);
	    if ( omconf->file == NULL )
	    {
		om_file_open(module);
	    }
	    ASSERT(omconf->file != NULL);
	    CHECKERR_MSG(apr_file_write(omconf->file, module->output.buf + module->output.bufstart, &nbytes),
			 "apr_file_write failed");

	    if ( omconf->sync == TRUE )
	    {
#ifdef HAVE_APR_FILE_SYNC
		CHECKERR_MSG(apr_file_sync(omconf->file),
			     "apr_file_sync failed on %s", omconf->filename);
#endif
	    }

	    if ( nbytes < module->output.buflen )
	    {
		if ( omconf->in_pollset == FALSE )
		{
		    nx_module_pollset_add_file(module, omconf->file, APR_POLLOUT);
		    nx_module_add_poll_event(module);
		    omconf->in_pollset = TRUE;
		}
		done = TRUE;
	    }
	    ASSERT(nbytes <= module->output.buflen);
	    module->output.bufstart += nbytes;
	    module->output.buflen -= nbytes;
	    if ( module->output.buflen == 0 )
	    { // all bytes have been sucessfully written to the file
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
		if ( om_file_update_filename(module, logdata) == TRUE )
		{
		    om_file_close(module);
		    om_file_open(module);
		}
		if ( omconf->truncate == TRUE )
		{
		    om_file_truncate(omconf->file);
		}
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



static void om_file_config(nx_module_t *module)
{
    const nx_directive_t *curr;
    nx_om_file_conf_t * volatile omconf;
    nx_exception_t e;

    ASSERT(module->directives != NULL);
    curr = module->directives;

    omconf = apr_pcalloc(module->pool, sizeof(nx_om_file_conf_t));
    module->config = omconf;

    omconf->fpool = nx_pool_create_child(module->pool);

    while ( curr != NULL )
    {
	if ( nx_module_common_keyword(curr->directive) == TRUE )
	{
	}
	else if ( strcasecmp(curr->directive, "file") == 0 )
	{
	    if ( omconf->filename_expr != NULL )
	    {
		nx_conf_error(curr, "file is already defined");
	    }
	    
	    try
	    {
		omconf->filename_expr = nx_expr_parse(module, curr->args, module->pool,
						      curr->filename, curr->line_num, curr->argsstart);
		if ( omconf->filename_expr == NULL )
		{
		    throw_msg("couldn't parse '%s'", curr->args);
		}
		if ( !((omconf->filename_expr->rettype == NX_VALUE_TYPE_STRING) ||
		       (omconf->filename_expr->rettype == NX_VALUE_TYPE_UNKNOWN)) )
		{
		    throw_msg("string type required in expression, found '%s'",
			      nx_value_type_to_string(omconf->filename_expr->rettype));
		}
	    }
	    catch(e)
	    {
		log_exception(e);
		nx_conf_error(curr, "invalid expression in 'File', string type required");
	    }
	    if ( omconf->filename_expr != NULL )
	    {
		if ( omconf->filename_expr->type == NX_EXPR_TYPE_VALUE )
		{
		    ASSERT(omconf->filename_expr->value.defined == TRUE);
		    apr_cpystrn(omconf->filename, omconf->filename_expr->value.string->buf,
				sizeof(omconf->filename));
		    omconf->filename_expr = NULL;
		}
	    }
	}
	else if ( strcasecmp(curr->directive, "truncate") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "createdir") == 0 )
	{
	}
	else if ( strcasecmp(curr->directive, "sync") == 0 )
	{
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
	    nx_conf_error(curr, "invalid om_file keyword: %s", curr->directive);
	}
	curr = curr->next;
    }

    if ( module->output.outputfunc == NULL )
    {
	module->output.outputfunc = nx_module_output_func_lookup("linebased");
    }
    ASSERT(module->output.outputfunc != NULL);

    omconf->truncate = FALSE;
    nx_cfg_get_boolean(module->directives, "truncate", &(omconf->truncate));

    omconf->sync = FALSE;
    nx_cfg_get_boolean(module->directives, "sync", &(omconf->sync));
#ifndef HAVE_APR_FILE_SYNC
    if ( omconf->sync == TRUE )
    {
	nx_conf_error(module->directives, "'Sync' requested but is not supported by apr library (need libapr 1.4 or higher)");
    }
#endif

    omconf->createdir = FALSE;
    nx_cfg_get_boolean(module->directives, "createdir", &(omconf->createdir));

    if ( (omconf->filename_expr == NULL) && (strlen(omconf->filename) == 0) )
    {
	nx_conf_error(module->directives, "'File' missing for module om_file");
    }
}



static void om_file_start(nx_module_t *module)
{
    nx_om_file_conf_t *omconf;

    ASSERT(module != NULL);

    omconf = (nx_om_file_conf_t *) module->config;

    if ( omconf->filename_expr == NULL )
    { // if filename is not dynamic, open it right away
	om_file_open(module);
    }
}



static void om_file_stop(nx_module_t *module)
{
    ASSERT(module != NULL);

    om_file_close(module);
}



static void om_file_init(nx_module_t *module)
{
    nx_module_pollset_init(module);
}



static void om_file_event(nx_module_t *module, nx_event_t *event)
{
    nx_om_file_conf_t *omconf;

    ASSERT(event != NULL);

    switch ( event->type )
    {
	case NX_EVENT_WRITE:
	    omconf = (nx_om_file_conf_t *) module->config;
	    omconf->in_pollset = FALSE;
	    // fallthrough
	case NX_EVENT_DATA_AVAILABLE:
	    om_file_write(module);
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



extern nx_module_exports_t nx_module_exports_om_file;

NX_MODULE_DECLARATION nx_om_file_module =
{
    NX_MODULE_API_VERSION,
    NX_MODULE_TYPE_OUTPUT,
    NULL,			// capabilities
    om_file_config,		// config
    om_file_start,		// start
    om_file_stop, 		// stop
    NULL,			// pause
    NULL,			// resume
    om_file_init,		// init
    NULL,			// shutdown
    om_file_event,		// event
    NULL,			// info
    &nx_module_exports_om_file, //exports
};


