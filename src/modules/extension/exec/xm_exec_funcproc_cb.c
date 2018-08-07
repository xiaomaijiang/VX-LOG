/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "../../../common/module.h"
#include "../../../common/alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

#define NX_XM_EXEC_MAX_ARGC 64

static void parse_args(nx_expr_eval_ctx_t *eval_ctx,
		       apr_pool_t *pool,
		       const char **cmd, 
		       const char **argv,
		       nx_expr_list_t *args)
{
    int argc = 0;
    nx_value_t value;
    nx_expr_list_elem_t *arg;

    ASSERT(args != NULL);

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);
    nx_expr_evaluate(eval_ctx, &value, arg->expr);

    if ( value.defined != TRUE )
    {
	throw_msg("command is undef");
    }
    if ( value.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&value);
	throw_msg("string type required for command");
    }
    *cmd = apr_pstrdup(pool, value.string->buf);
    nx_value_kill(&value);
    
    argv[argc] = NULL;
    for ( ; arg != NULL; arg = NX_DLIST_NEXT(arg, link) )
    {
	if ( argc >= NX_XM_EXEC_MAX_ARGC - 1)
	{
	    apr_pool_destroy(pool);
	    throw_msg("too many arguments passed to exec(), limit is %d", NX_XM_EXEC_MAX_ARGC - 1);
	}
	nx_expr_evaluate(eval_ctx, &value, arg->expr);
	if ( value.defined != TRUE )
	{
	    throw_msg("arg passed to exec() is undef");
	}
	if ( value.type != NX_VALUE_TYPE_STRING )
	{
	    nx_value_kill(&value);
	    throw_msg("string type required for exec() argument");
	}
	argv[argc] = apr_pstrdup(pool, value.string->buf);
	nx_value_kill(&value);
	argc++;
	argv[argc] = NULL;
    }
}


void nx_expr_proc__exec(nx_expr_eval_ctx_t *eval_ctx,
			nx_module_t *module UNUSED,
			nx_expr_list_t *args)
{
    const char *cmd = NULL;
    const char *argv[NX_XM_EXEC_MAX_ARGC];
    const char *env = NULL;
    nx_exception_t e;
    apr_procattr_t *pattr;
    apr_proc_t proc;
    apr_exit_why_e exitwhy;
    int exitval;
    apr_pool_t *pool;

    pool = nx_pool_create_core();
    parse_args(eval_ctx, pool, &cmd, argv, args);
    
    try
    {
	CHECKERR(apr_procattr_create(&pattr, pool));
	CHECKERR(apr_procattr_error_check_set(pattr, 1));
	CHECKERR(apr_procattr_io_set(pattr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE));
	CHECKERR(apr_procattr_cmdtype_set(pattr, APR_PROGRAM_ENV));
	CHECKERR_MSG(apr_proc_create(&proc, (const char * const) cmd, (const char * const *) argv,
				     (const char * const *) env, (apr_procattr_t*) pattr, pool),
		     "couldn't execute process %s", cmd);
	log_debug("process %d forked with exec()", proc.pid);
    }
    catch(e)
    {
	apr_pool_destroy(pool);
	rethrow(e);
    }

    apr_proc_wait(&proc, &exitval, &exitwhy, APR_WAIT);
    if ( proc.pid > 0 )
    {
	if ( exitwhy == APR_PROC_EXIT )
	{ //normal termination
	    if ( exitval != 0 )
	    {
		log_error("subprocess '%s' returned a non-zero exit value of %d",
			  cmd, exitval);
	    }
	}
	else
	{
	    log_error("subprocess '%s' was terminated by a signal", cmd); 
	}
    }

    apr_pool_destroy(pool);
}



void nx_expr_proc__exec_async(nx_expr_eval_ctx_t *eval_ctx,
			      nx_module_t *module,
			      nx_expr_list_t *args)
{
    const char *cmd;
    const char *argv[NX_XM_EXEC_MAX_ARGC];
    const char *env = NULL;
    nx_exception_t e;
    apr_procattr_t *pattr;
    apr_proc_t proc;
    apr_pool_t *pool;
    apr_exit_why_e exitwhy;
    int exitval;
    apr_status_t rv;

    ASSERT(module != NULL);

    pool = nx_pool_create_core();
    parse_args(eval_ctx, pool, &cmd, argv, args);

    try
    {
	CHECKERR(apr_procattr_create(&pattr, pool));
	CHECKERR(apr_procattr_error_check_set(pattr, 1));
	// to avoid excess zombies
	//CHECKERR(apr_procattr_detach_set(pattr, 1));
	CHECKERR(apr_procattr_io_set(pattr, APR_NO_PIPE, APR_NO_PIPE, APR_NO_PIPE));
	CHECKERR(apr_procattr_cmdtype_set(pattr, APR_PROGRAM_ENV));
	CHECKERR_MSG(apr_proc_create(&proc, (const char * const) cmd, (const char * const *) argv,
				     (const char * const *) env, (apr_procattr_t*) pattr, pool),
		     "couldn't execute process %s", cmd);
	log_debug("process %d forked with exec_async()", proc.pid);
    }
    catch(e)
    {
	apr_pool_destroy(pool);
	rethrow(e);
    }

    while ( APR_STATUS_IS_CHILD_DONE(rv = apr_proc_wait_all_procs(&proc, &exitval,
								  &exitwhy, APR_NOWAIT, pool)) )
    {
	log_debug("process %d reaped", proc.pid);

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

    apr_pool_destroy(pool);
}
