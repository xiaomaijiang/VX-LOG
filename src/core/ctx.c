/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include <apr_want.h>

#include "../core/nxlog.h"
#include "../common/error_debug.h"
#include "../common/cfgfile.h"
#include "../common/event.h"
#include "../common/route.h"
#include "../common/exception.h"
#include "../common/expr-core-funcproc.h"
#include "../common/alloc.h"
#include "../common/atomic.h"
#include "ctx.h"
#include "job.h"
#include "modules.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

nx_ctx_t *nx_ctx_new()
{
    nx_ctx_t *ctx;
    apr_pool_t *pool;

    pool = nx_pool_create_core();

    ctx = apr_pcalloc(pool, sizeof(nx_ctx_t));
    ctx->pool = pool;

    ctx->modules = apr_palloc(ctx->pool, sizeof(nx_module_list_t));
    NX_DLIST_INIT(ctx->modules, nx_module_t, link);

    ctx->routes = apr_palloc(ctx->pool, sizeof(nx_route_list_t));
    NX_DLIST_INIT(ctx->routes, nx_route_t, link);

    ctx->events = apr_palloc(ctx->pool, sizeof(nx_event_list_t));
    NX_DLIST_INIT(ctx->events, nx_event_t, link);

    ctx->config_cache = apr_hash_make(ctx->pool);
    CHECKERR(apr_thread_mutex_create(&(ctx->config_cache_mutex), APR_THREAD_MUTEX_UNNESTED, ctx->pool));

    ctx->resources = apr_palloc(ctx->pool, sizeof(nx_resource_list_t));
    NX_DLIST_INIT(ctx->resources, nx_resource_t, link);

    ctx->input_funcs = apr_palloc(ctx->pool, sizeof(nx_module_input_func_list_t));
    NX_DLIST_INIT(ctx->input_funcs, nx_module_input_func_t, link);

    ctx->output_funcs = apr_palloc(ctx->pool, sizeof(nx_module_output_func_list_t));
    NX_DLIST_INIT(ctx->output_funcs, nx_module_output_func_t, link);

    ctx->expr_funcs = apr_palloc(ctx->pool, sizeof(nx_expr_func_list_t));
    NX_DLIST_INIT(ctx->expr_funcs, nx_expr_func_t, link);

    ctx->expr_procs = apr_palloc(ctx->pool, sizeof(nx_expr_proc_list_t));
    NX_DLIST_INIT(ctx->expr_procs, nx_expr_proc_t, link);

    ctx->loglevel = NX_LOGLEVEL_INFO;
    ctx->formatlog = FALSE;
    ctx->flowcontrol = TRUE;

    ctx->ignoreerrors = TRUE;
    ctx->norepeat = TRUE;

    ctx->coremodule = nx_module_new(NX_MODULE_TYPE_EXTENSION, "__CORE__", 0);
    ctx->coremodule->priority = 1;
    return ( ctx );
}



void nx_ctx_parse_cfg(nx_ctx_t *ctx, const char *cfgpath)
{
    nx_cfgfile_t cfgfile;
    const nx_directive_t *curr;
    static const char *keywords[] = { "root", "route", "moduledir",
				      "pidfile", "input", "output",
				      "processor", "logfile", "loglevel",
				      "nocache", "cachedir", "extension", 
				      "user", "group", "rootdir", "spooldir",
				      "nofreeonexit", "panic", "threads", 
				      "ignoreerrors", "suppressrepeatinglogs",
				      "flowcontrol",
				      NULL };
    int i;
    boolean found;
    nx_cfg_parser_ctx_t cfg_ctx;
    const char *tmpstr;
    nxlog_t *nxlog;

    nxlog = nxlog_get();

    if ( cfgpath == NULL )
    {
	cfgfile.name = apr_pstrdup(ctx->pool, NX_CONFIGFILE);
    }
    else
    {
	cfgfile.name = cfgpath;
    }

    nx_cfg_open_file(&cfgfile, ctx->pool);

    ctx->cfgtree = apr_pcalloc(ctx->pool, sizeof(nx_directive_t));
    ctx->cfgtree->directive = apr_pstrdup(ctx->pool, "Root");
    ctx->cfgtree->args = NULL;
    ctx->cfgtree->first_child = NULL;

    memset(&cfg_ctx, 0, sizeof(nx_cfg_parser_ctx_t));
    cfg_ctx.pool = ctx->pool;
    cfg_ctx.root = ctx->cfgtree;
    cfg_ctx.current = ctx->cfgtree;
    cfg_ctx.next_is_child = TRUE;
    cfg_ctx.cfg = &cfgfile;

    nx_cfg_parse(&cfg_ctx);

    if ( ctx->cfgtree->first_child == NULL )
    {
	throw_msg("Empty configuration");
    }

    ctx->cfgtree = ctx->cfgtree->first_child;
    //nx_cfg_dump(ctx->cfgtree, 0);

    curr = ctx->cfgtree;

    while ( curr != NULL )
    {
	found = FALSE;
	for ( i = 0; keywords[i] != NULL; i++ )
	{
	    if ( strcasecmp(keywords[i], curr->directive) == 0 )
	    {
		found = TRUE;
		break;
	    }
	}
	if ( found != TRUE )
	{
	    nx_conf_error(curr, "Invalid keyword: %s", curr->directive);
	}
	
	if ( strcasecmp(curr->directive, "threads") == 0 )
	{
	    if ( sscanf(curr->args, "%u", &(nxlog->num_worker_thread)) != 1 )
	    {
		nx_conf_error(curr, "invalid 'Threads' count:  %s", curr->args);
	    }
	}
	curr = curr->next;
    }

    nx_cfg_get_boolean(ctx->cfgtree, "nocache", &(ctx->nocache));

    if ( nx_cfg_get_value(ctx->cfgtree, "moduledir") != NULL )
    {
	ctx->moduledir = apr_pstrdup(ctx->pool, nx_cfg_get_value(ctx->cfgtree, "moduledir"));
    }
    else
    {
	ctx->moduledir = apr_pstrdup(ctx->pool, NX_MODULEDIR);
    }

    if ( nx_cfg_get_value(ctx->cfgtree, "cachedir") != NULL )
    {
	ctx->cachedir = apr_pstrdup(ctx->pool, nx_cfg_get_value(ctx->cfgtree, "cachedir"));
    }
    else
    {
	ctx->cachedir = apr_pstrdup(ctx->pool, NX_CACHEDIR);
    }
    ctx->ccfilename = apr_psprintf(ctx->pool, "%s"NX_DIR_SEPARATOR"configcache.dat", ctx->cachedir);

    if ( nx_cfg_get_value(ctx->cfgtree, "rootdir") != NULL )
    {
	ctx->rootdir = apr_pstrdup(ctx->pool, nx_cfg_get_value(ctx->cfgtree, "rootdir"));
    }

    if ( nx_cfg_get_value(ctx->cfgtree, "spooldir") != NULL )
    {
	ctx->spooldir = apr_pstrdup(ctx->pool, nx_cfg_get_value(ctx->cfgtree, "spooldir"));
    }

    if ( (tmpstr = nx_cfg_get_value(ctx->cfgtree, "panic")) != NULL )
    {
	if ( strcasecmp(tmpstr, "hard") == 0 )
	{
	    ctx->panic_level = NX_PANIC_LEVEL_HARD;
	}
	else if ( strcasecmp(tmpstr, "soft") == 0 )
	{
	    ctx->panic_level = NX_PANIC_LEVEL_SOFT;
	}
	else if ( strcasecmp(tmpstr, "off") == 0 )
	{
	    ctx->panic_level = NX_PANIC_LEVEL_OFF;
	}
	else
	{
	    nx_conf_error(ctx->cfgtree, "Invalid 'Panic': %s", tmpstr);
	}
    }
    else
    {
	ctx->panic_level = NX_PANIC_LEVEL_SOFT;
    }

    nx_cfg_get_boolean(ctx->cfgtree, "nofreeonexit", &(ctx->nofreeonexit));
    nx_cfg_get_boolean(ctx->cfgtree, "ignoreerrors", &(ctx->ignoreerrors));
    if ( nxlog->verify_conf == TRUE )
    {
	ctx->ignoreerrors = FALSE;
    }
    nx_cfg_get_boolean(ctx->cfgtree, "suppressrepeatinglogs", &(ctx->norepeat));
    nx_cfg_get_boolean(ctx->cfgtree, "flowcontrol", &(ctx->flowcontrol));
}



void nx_ctx_init_logging(nx_ctx_t *ctx)
{
    const char *loglevelstr, *logfilename;

    ctx->formatlog = TRUE;
    loglevelstr = nx_cfg_get_value(ctx->cfgtree, "loglevel");
    if ( loglevelstr != NULL )
    {
	if ( strcasecmp(loglevelstr, "debug") == 0 )
	{
	    ctx->loglevel = NX_LOGLEVEL_DEBUG;
	}
	else if ( strcasecmp(loglevelstr, "info") == 0 )
	{
	    ctx->loglevel = NX_LOGLEVEL_INFO;
	}
	else if ( strcasecmp(loglevelstr, "warning") == 0 )
	{
	    ctx->loglevel = NX_LOGLEVEL_WARNING;
	}
	else if ( strcasecmp(loglevelstr, "error") == 0 )
	{
	    ctx->loglevel = NX_LOGLEVEL_ERROR;
	}
	else if ( strcasecmp(loglevelstr, "critical") == 0 )
	{
	    ctx->loglevel = NX_LOGLEVEL_CRITICAL;
	}
	else
	{
	    throw_msg("invalid loglevel: %s", loglevelstr);
	}
    }

    logfilename = nx_cfg_get_value(ctx->cfgtree, "logfile");
    if ( logfilename == NULL )
    {
	return;
    }

    CHECKERR_MSG(apr_file_open(&(ctx->logfile), logfilename,
			       APR_WRITE | APR_CREATE | APR_APPEND,
			       APR_OS_DEFAULT, ctx->pool),
		 "couldn't open logfile '%s' for writing", logfilename);
}



void nx_ctx_free(nx_ctx_t *ctx)
{
    ASSERT(ctx != NULL);
    if ( ctx->pool != NULL )
    {
	apr_pool_destroy(ctx->pool);
    }
}



static nx_jobgroup_t *nx_ctx_get_jobgroup(nx_ctx_t *ctx,
					  int priority)
{
    nx_jobgroup_t *jobgroup = NULL;
    nx_jobgroup_t *retval = NULL;

    ASSERT(ctx != NULL);
    ASSERT(priority > 0);

    for ( jobgroup = NX_DLIST_FIRST(ctx->jobgroups);
	  jobgroup != NULL;
	  jobgroup = NX_DLIST_NEXT(jobgroup, link) )
    {
	if ( jobgroup->priority == priority )
	{
	    return ( jobgroup );
	}
    }

    retval = apr_pcalloc(ctx->pool, sizeof(nx_jobgroup_t));
    log_debug("jobgroup created with priority %d", priority);
    retval->priority = priority;
    NX_DLIST_INIT(&(retval->jobs), nx_job_t, link);
    for ( jobgroup = NX_DLIST_FIRST(ctx->jobgroups);
	  (jobgroup != NULL) && (jobgroup->priority < priority);
	  jobgroup = NX_DLIST_NEXT(jobgroup, link) );
    if ( jobgroup == NULL )
    {
	NX_DLIST_INSERT_TAIL(ctx->jobgroups, retval, link);
	//NX_DLIST_CHECK(ctx->jobgroups, link);
    }
    else
    {
	NX_DLIST_INSERT_BEFORE(ctx->jobgroups, jobgroup, retval, link);
    }

    return ( retval );
}



void nx_ctx_init_jobs(nx_ctx_t *ctx)
{
    nx_jobgroup_t *jobgroup = NULL;
    nx_job_t *job;
    nx_module_t *module;

    ASSERT(ctx != NULL);
    ctx->jobgroups = apr_palloc(ctx->pool, sizeof(nx_jobgroups_t));
    NX_DLIST_INIT(ctx->jobgroups, nx_jobgroup_t, link);

    for ( module = NX_DLIST_FIRST(ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	jobgroup = nx_ctx_get_jobgroup(ctx, module->priority);
	job = apr_pcalloc(ctx->pool, sizeof(nx_job_t));
	NX_DLIST_INSERT_TAIL(&(jobgroup->jobs), job, link);
	module->job = job;
    }
    if ( jobgroup != NULL )
    {
	ctx->coremodule->job = apr_pcalloc(ctx->pool, sizeof(nx_job_t));
	NX_DLIST_INSERT_TAIL(&(jobgroup->jobs), ctx->coremodule->job, link);
    }
}



boolean nx_ctx_next_job(nx_ctx_t *ctx,
			nx_job_t **jobresult,
			nx_event_t **eventresult)
{
    nx_jobgroup_t *jobgroup;
    nx_job_t *job;
    nx_event_t *event = NULL;

    ASSERT(ctx != NULL);
    for ( jobgroup = NX_DLIST_FIRST(ctx->jobgroups);
	  jobgroup != NULL;
	  jobgroup = NX_DLIST_NEXT(jobgroup, link) )
    {
	if ( jobgroup->last == NULL )
	{
	    job = NX_DLIST_FIRST(&(jobgroup->jobs));
	}
	else
	{
	    if ( jobgroup->last == NX_DLIST_LAST(&(jobgroup->jobs)) )
	    {
		job = NX_DLIST_FIRST(&(jobgroup->jobs));
	    }
	    else
	    {
		job = NX_DLIST_NEXT(jobgroup->last, link);
	    }
	}
	jobgroup->last = job;

	do
	{
	    ASSERT(job != NULL);
	    if ( (nx_atomic_read32(&(job->busy)) != TRUE) &&
		 (NX_DLIST_EMPTY(&(job->events)) != TRUE) )
	    {
		event = NX_DLIST_FIRST(&(job->events));
		NX_DLIST_REMOVE(&(job->events), event, link);
		nx_atomic_sub32(&(job->event_cnt), 1);
		//log_debug("event 0x%lx removed in nx_ctx_next_job", event);
		*jobresult = job;
		*eventresult = event;
		return ( TRUE );
	    }
	    else
	    {
		//log_debug("job busy: %d, empty: %d", job->busy, NX_DLIST_EMPTY(&(job->events)));
	    }
	    if ( job == NX_DLIST_LAST(&(jobgroup->jobs)) )
	    {
		job = NX_DLIST_FIRST(&(jobgroup->jobs));
	    }
	    else
	    {
		job = NX_DLIST_NEXT(job, link);
	    }
	} while ( job != jobgroup->last );
    }

    return ( FALSE );
}



boolean nx_ctx_has_jobs(nx_ctx_t *ctx)
{
    nx_jobgroup_t *jobgroup;
    nx_job_t *job;

    ASSERT(ctx != NULL);
    for ( jobgroup = NX_DLIST_FIRST(ctx->jobgroups);
	  jobgroup != NULL;
	  jobgroup = NX_DLIST_NEXT(jobgroup, link) )
    {
	for ( job = NX_DLIST_FIRST(&(jobgroup->jobs));
	      job != NULL;
	      job = NX_DLIST_NEXT(job, link) )
	{
	    if ( (job->busy != TRUE) && (NX_DLIST_EMPTY(&(job->events)) != TRUE) )
	    {
		return ( TRUE );
	    }
	}
    }

    return ( FALSE );
}



nx_module_t *nx_ctx_module_for_job(nx_ctx_t *ctx, nx_job_t *job)
{
    nx_module_t *module;
    nx_route_t *route;
    int i;

    for ( route = NX_DLIST_FIRST(ctx->routes);
	  route != NULL;
	  route = NX_DLIST_NEXT(route, link) )
    {
	for ( i = 0; i < route->modules->nelts; i++ )
	{
	    module = ((nx_module_t **)route->modules->elts)[i];
	    if ( module->job == job )
	    {
		return ( module );
	    }
	}
    }

    return ( NULL );
}



static void nx_ctx_register_builtin_inputfuncs()
{
    nx_module_input_func_register(NULL, "linebased",
				  &nx_module_input_func_linereader, NULL, NULL);
    
    nx_module_input_func_register(NULL, "dgram",
				  &nx_module_input_func_dgramreader, NULL, NULL);
    nx_module_input_func_register(NULL, "binary",
				  &nx_module_input_func_binaryreader, NULL, NULL);
}



static void nx_ctx_register_builtin_outputfuncs()
{
    nx_module_output_func_register(NULL, "linebased",
				   &nx_module_output_func_linewriter, NULL);
    
    nx_module_output_func_register(NULL, "dgram",
				   &nx_module_output_func_dgramwriter, NULL);

    nx_module_output_func_register(NULL, "binary",
				   &nx_module_output_func_binarywriter, NULL);
}



static void nx_ctx_register_builtin_expr_funcs(nx_ctx_t *ctx)
{
    int i;
    extern nx_expr_func_t nx_api_declarations_core_funcs[];
    
    for ( i = 0; i < nx_api_declarations_core_func_num; i++ )
    {
	nx_expr_func_register(ctx->pool, ctx->expr_funcs, NULL,
			      nx_api_declarations_core_funcs[i].name,
			      nx_api_declarations_core_funcs[i].type,
			      nx_api_declarations_core_funcs[i].cb,
			      nx_api_declarations_core_funcs[i].rettype,
			      nx_api_declarations_core_funcs[i].num_arg,
			      nx_api_declarations_core_funcs[i].arg_types);
    }
}


static void nx_ctx_register_builtin_expr_procs(nx_ctx_t *ctx)
{
    int i;
    extern nx_expr_proc_t nx_api_declarations_core_procs[];
    
    for ( i = 0; i < nx_api_declarations_core_proc_num; i++ )
    {
	nx_expr_proc_register(ctx->pool, ctx->expr_procs, NULL,
			      nx_api_declarations_core_procs[i].name,
			      nx_api_declarations_core_procs[i].type,
			      nx_api_declarations_core_procs[i].cb,
			      nx_api_declarations_core_procs[i].num_arg,
			      nx_api_declarations_core_procs[i].arg_types);

    }
}


void nx_ctx_register_builtins(nx_ctx_t *ctx)
{
    nx_ctx_register_builtin_inputfuncs();
    nx_ctx_register_builtin_outputfuncs();
    nx_ctx_register_builtin_expr_funcs(ctx);
    nx_ctx_register_builtin_expr_procs(ctx);
}

