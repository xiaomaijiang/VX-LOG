/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>
#include <apr_file_info.h>
#include <apr_dso.h>

#include "../common/error_debug.h"
#include "../common/module.h"
#include "../common/cfgfile.h"
#include "../common/expr.h"
#include "../common/alloc.h"
#include "../common/atomic.h"
#include "../core/nxlog.h"

#include "job.h"
#include "modules.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


void nx_module_register_exports(const nx_ctx_t *ctx, nx_module_t *module)
{
    int i;

    if ( module->decl->exports == NULL )
    {
	return;
    }

    log_debug("module %s has %d exported functions",
	      module->name, module->decl->exports->num_func);
    for ( i = 0; i < module->decl->exports->num_func; i++ )
    {
	ASSERT(module->decl->exports->funcs != NULL);
	log_debug("registering function %s", module->decl->exports->funcs[i].name);
	if ( nx_expr_func_lookup(ctx->expr_funcs, module,
				 module->decl->exports->funcs[i].name,
				 module->decl->exports->funcs[i].type,
				 module->decl->exports->funcs[i].rettype,
				 module->decl->exports->funcs[i].num_arg,
				 module->decl->exports->funcs[i].arg_types) == NULL )
	{
	    nx_expr_func_register(ctx->pool, ctx->expr_funcs, module,
				  module->decl->exports->funcs[i].name,
				  module->decl->exports->funcs[i].type,
				  module->decl->exports->funcs[i].cb,
				  module->decl->exports->funcs[i].rettype,
				  module->decl->exports->funcs[i].num_arg,
				  module->decl->exports->funcs[i].arg_types);
	}
    }

    log_debug("module %s has %d exported procedures",
	      module->name, module->decl->exports->num_proc);
    for ( i = 0; i < module->decl->exports->num_proc; i++ )
    {
	ASSERT(module->decl->exports->procs != NULL);
	log_debug("registering procedure %s", module->decl->exports->procs[i].name);
	if ( nx_expr_proc_lookup(ctx->expr_procs, module,
				 module->decl->exports->procs[i].name,
				 module->decl->exports->procs[i].type,
				 module->decl->exports->procs[i].num_arg,
				 module->decl->exports->procs[i].arg_types) == NULL )
	{
	    nx_expr_proc_register(ctx->pool, ctx->expr_procs, module,
				  module->decl->exports->procs[i].name,
				  module->decl->exports->procs[i].type,
				  module->decl->exports->procs[i].cb,
				  module->decl->exports->procs[i].num_arg,
				  module->decl->exports->procs[i].arg_types);
	}
    }
}



void nx_module_load_dso(nx_module_t *module,
			const nx_ctx_t *ctx,
			const char *dsopath)
{
    apr_status_t rv;
    char symbol[256];
    nx_module_t *m;
    char errbuf[512];

    ASSERT(module != NULL);

    // check if dso is already loaded
    ASSERT(ctx->modules != NULL);
    for ( m = NX_DLIST_FIRST(ctx->modules);
	  m != NULL;
	  m = NX_DLIST_NEXT(m, link) )
    {
	if ( m == module )
	{ // skip self
	    continue;
	}
	if ( strcmp(m->dsoname, module->dsoname) == 0 )
	{
	    log_debug("module %s"NX_MODULE_DSO_EXTENSION" is already loaded", module->dsoname);
	    module->dso = m->dso;
	    module->decl = m->decl;
	    return;
	}
    }

    if ( (rv = apr_dso_load(&(module->dso), dsopath, ctx->pool)) != APR_SUCCESS )
    {
	errbuf[0] = '\0';
	if ( module->dso != NULL )
	{
	    apr_dso_error(module->dso, errbuf, sizeof(errbuf));
	    throw(rv, "Failed to load module from %s, %s", dsopath, errbuf);
	}
	else
	{
	    throw(rv, "Failed to load module from %s", dsopath);
	}
    }

    memset(symbol, 0, sizeof(symbol));

    apr_snprintf(symbol, sizeof(symbol), "nx_%s_module", module->dsoname);
    if ( (rv = apr_dso_sym((apr_dso_handle_sym_t *) &(module->decl), module->dso, symbol)) != APR_SUCCESS )
    {
	errbuf[0] = '\0';
	if ( module->dso != NULL )
	{
	    apr_dso_error(module->dso, errbuf, sizeof(errbuf));
	    throw(rv, "Couldn't find symbol %s in module %s: %s",
		  symbol, module->dsoname, errbuf);
	}
	else
	{
	    throw(rv, "Couldn't find symbol %s in module %s", symbol, module->dsoname);
	}
    }

    if ( module->decl->api_version != NX_MODULE_API_VERSION )
    {
	throw_msg("module api version mismatch for %s: got %d, required: %d",
		  module->name, module->decl->api_version, NX_MODULE_API_VERSION);
    }
}



nx_module_t *nx_module_new(nx_module_type_t type, const char *name, apr_size_t bufsize)
{
    nx_module_t *module;
    apr_pool_t *pool;

    ASSERT(name != NULL);

    pool = nx_pool_create_core();

    module = apr_pcalloc(pool, sizeof(nx_module_t));
    module->pool = pool;

    CHECKERR(apr_thread_mutex_create(&(module->mutex), APR_THREAD_MUTEX_UNNESTED,
				     module->pool));

    module->routes = apr_array_make(module->pool, 5, sizeof(nx_route_t *));
    module->priority = 99; // modules with no routes will have this
    module->type = type;
    module->name = apr_pstrdup(pool, name);
#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size after alloc is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif

    switch ( type )
    {
	case NX_MODULE_TYPE_INPUT:
	    module->input.buf = apr_pcalloc(module->pool, bufsize);
	    module->input.bufsize = (int) bufsize;
	    module->input.pool = module->pool;
	    module->input.module = module;
	    break;
	case NX_MODULE_TYPE_PROCESSOR:
	    module->queue = nx_logqueue_new(module->pool, module->name);
	    break;
	case NX_MODULE_TYPE_OUTPUT:
	    module->queue = nx_logqueue_new(module->pool, module->name);
	    module->output.buf = apr_pcalloc(module->pool, bufsize);
	    module->output.bufsize = bufsize;
	    module->output.pool = module->pool;
	    module->output.module = module;
	    break;
	case NX_MODULE_TYPE_EXTENSION:
	    break;
	default:
	    nx_panic("invalid module type");
    }

#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size after logqueue alloc is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif

    return ( module );
}



static void nx_module_add(const nx_ctx_t *ctx,
			  const nx_directive_t *modconf,
			  const char *instancename,
			  nx_module_type_t type)
{
    const char *modulename;
    const char *typedir = NULL;
    char dsoname[4096];
    nx_module_t *tmpmodule, *module;
    boolean gotflowcontrol = FALSE;
    apr_size_t bufsize = 0;

    ASSERT(modconf != NULL);

    modulename = nx_cfg_get_value(modconf, "Module");
    if ( (modulename == NULL) || (strlen(modulename) == 0) )
    {
	nx_conf_error(modconf, "Module missing");
    }
    if ( (instancename == NULL) || (strlen(instancename) == 0) )
    {
	nx_conf_error(modconf, "Module instance name missing");
    }

    for ( tmpmodule = NX_DLIST_FIRST(ctx->modules);
          tmpmodule != NULL;
	  tmpmodule = NX_DLIST_NEXT(tmpmodule, link) )
    {
	if ( strcasecmp(tmpmodule->name, instancename) == 0 )
	{
	    nx_conf_error(modconf, "module '%s' is already defined", instancename);
	}
    }

    log_debug("Setting up module '%s' using %s", instancename == NULL ? "" : instancename,
	      modulename);
    switch ( type )
    {
	case NX_MODULE_TYPE_INPUT:
	    bufsize = NX_MODULE_DEFAULT_INPUT_BUFSIZE;
	    break;
	case NX_MODULE_TYPE_OUTPUT:
	    bufsize = NX_MODULE_DEFAULT_OUTPUT_BUFSIZE;
	    break;
	default:
	    break;
    }
    module = nx_module_new(type, instancename, bufsize);

    module->directives = modconf;
    module->dsoname = apr_pstrdup(module->pool, modulename);

#ifndef __ANDROID__
    switch ( type )
    {
	case NX_MODULE_TYPE_INPUT:
	    typedir = "input";
	    break;
	case NX_MODULE_TYPE_PROCESSOR:
	    typedir = "processor";
	    break;
	case NX_MODULE_TYPE_OUTPUT:
	    typedir = "output";
	    break;
	case NX_MODULE_TYPE_EXTENSION:
	    typedir = "extension";
	    break;
	default:
	    nx_panic("invalid module type");
    }
    apr_snprintf(dsoname, sizeof(dsoname),
		 "%s"NX_DIR_SEPARATOR"%s"NX_DIR_SEPARATOR"%s"NX_MODULE_DSO_EXTENSION,
		 ctx->moduledir, typedir, modulename);
#else
    // For android we store the modules as /data/data/org.nxlog/lib/lib_im_xxx.so
    // Only this naming scheme is allowed for shared libs
    apr_snprintf(dsoname, sizeof(dsoname),
		 "%s"NX_DIR_SEPARATOR"lib_%s"NX_MODULE_DSO_EXTENSION,
		 ctx->moduledir, modulename);
#endif		 
    nx_module_load_dso(module, ctx, dsoname);

    if ( module->decl->type != module->type )
    {
	throw_msg("cannot use %s module %s as %s",
		  nx_module_type_to_string(module->decl->type), module->name,
		  nx_module_type_to_string(module->type));
    }

    nx_module_register_exports(ctx, module);

    if ( (type == NX_MODULE_TYPE_INPUT) || (type == NX_MODULE_TYPE_PROCESSOR) )
    { // use global FlowControl value which can be overridden below
	module->flowcontrol = ctx->flowcontrol;
    }

    while ( modconf != NULL )
    {
	if ( strcasecmp(modconf->directive, "module") == 0 )
	{
	}
	else if ( strcasecmp(modconf->directive, "FlowControl") == 0 )
	{
	    if ( gotflowcontrol == TRUE )
	    {
		nx_conf_error(modconf, "'FlowControl' flag already defined");
	    }
	    nx_cfg_get_boolean(modconf, "FlowControl", &(module->flowcontrol));
	    gotflowcontrol = TRUE;

	    switch ( type )
	    {
		case NX_MODULE_TYPE_INPUT:
		case NX_MODULE_TYPE_PROCESSOR:
		    break;
		case NX_MODULE_TYPE_OUTPUT:
		case NX_MODULE_TYPE_EXTENSION:
		    nx_conf_error(modconf, "'FlowControl' is only supported by Input and Processor modules");
		    break;
		default:
		    nx_panic("invalid module type");
	    }
	}
	modconf = modconf->next;
    }
    if ( module->queue != NULL )
    {
	nx_logqueue_init(module->queue);
    }
    NX_DLIST_INSERT_TAIL(ctx->modules, module, link);

    if ( (type == NX_MODULE_TYPE_INPUT) || (type == NX_MODULE_TYPE_PROCESSOR) )
    {
	log_debug("FlowControl %s for %s", module->flowcontrol ? "enabled" : "disabled", module->name);
    }

#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size after new is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif
}



void nx_ctx_config_modules(nx_ctx_t *ctx)
{
    const nx_directive_t * volatile curr = ctx->cfgtree;
    nx_module_t * volatile module;
    nx_exception_t e;

    ASSERT(ctx->cfgtree != NULL);

    while ( curr != NULL )
    {
	try
	{
	    if ( strcasecmp(curr->directive, "input") == 0 )
	    {
		if ( curr->first_child == NULL )
		{
		    nx_conf_error(curr, "empty 'Input' block");
		}

		nx_module_add(ctx, curr->first_child, curr->args, NX_MODULE_TYPE_INPUT);
	    }
	    else if ( strcasecmp(curr->directive, "processor") == 0 )
	    {
		if ( curr->first_child == NULL )
		{
		    nx_conf_error(curr, "empty 'Processor' block");
		}
		
		nx_module_add(ctx, curr->first_child, curr->args, NX_MODULE_TYPE_PROCESSOR);
	    }
	    else if ( strcasecmp(curr->directive, "output") == 0 )
	    {
		if ( curr->first_child == NULL )
		{
		    nx_conf_error(curr, "empty 'Output' block");
		}
		nx_module_add(ctx, curr->first_child, curr->args, NX_MODULE_TYPE_OUTPUT);
	    }
	    else if ( strcasecmp(curr->directive, "extension") == 0 )
	    {
		if ( curr->first_child == NULL )
		{
		    nx_conf_error(curr, "empty 'Extension' block");
		}
		
		nx_module_add(ctx, curr->first_child, curr->args, NX_MODULE_TYPE_EXTENSION);
	    }
	}
	catch(e)
	{
	    if ( ctx->ignoreerrors != TRUE )
	    {
		rethrow(e);
	    }
	    log_exception(e);
	}
	curr = curr->next;
    }

    for ( module = NX_DLIST_FIRST(ctx->modules);
          module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	try
	{
	    nx_module_config(module);
	}
	catch(e)
	{
	    module->has_config_errors = TRUE;
	    if ( ctx->ignoreerrors != TRUE )
	    {
		rethrow(e);
	    }
	    log_exception(e);
	}
#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size after config is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif
    }
}



void nx_ctx_init_modules(nx_ctx_t *ctx)
{
    int volatile num_input = 0;
    nx_module_t * volatile module;
    nx_exception_t e;

    for ( module = NX_DLIST_FIRST(ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	try
	{
	    nx_module_init(module);
	    if ( module->type == NX_MODULE_TYPE_INPUT )
	    {
		num_input++;
	    }
	}
	catch(e)
	{
	    if ( ctx->ignoreerrors == TRUE )
	    {
		log_exception(e);
	    }
	    else
	    {
		rethrow(e);
	    }
	}
#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size after init is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif
    }

    if ( num_input == 0 )
    {
	log_warn("no functional input modules!");
    }
}


static boolean check_module_routes(nx_module_t *module)
{
    nx_module_t *tmpmodule;
    nx_route_t *route;
    int i, j;
    int num_input = 0;
    int num_output = 0;

    for ( i = 0; i < module->routes->nelts; i++ )
    {
	route = ((nx_route_t **) module->routes->elts)[i];

	for ( j = 0; j < route->modules->nelts; j++ )
	{
	    tmpmodule = ((nx_module_t **) route->modules->elts)[j];
	    if ( tmpmodule->type == NX_MODULE_TYPE_INPUT )
	    {
		num_input++;
	    }
	    else if ( tmpmodule->type == NX_MODULE_TYPE_OUTPUT )
	    {
		num_output++;
	    }
	}
	if ( num_input == 0 )
	{
	    return ( FALSE );
	}
	if ( num_output == 0 )
	{
	    return ( FALSE );
	}
    }

    // all routes of the module are valid
    return ( TRUE );
}



void nx_ctx_start_modules(nx_ctx_t *ctx)
{
    nx_module_t * volatile module;
    nxlog_t *nxlog;

    for ( module = NX_DLIST_FIRST(ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	if ( nx_module_get_status(module) == NX_MODULE_STATUS_RUNNING )
	{
	    continue;
	}
	if ( (module->refcount == 0) && (module->type != NX_MODULE_TYPE_EXTENSION) )
	{
	    log_warn("not starting unused module %s", module->name);
	    continue;
	}

	if ( check_module_routes(module) != TRUE )
	{
	    log_warn("not starting module %s because it is part of an incomplete route", module->name);
	    continue;
	}

	nx_module_start(module);

#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size after start is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif
    }
/*
	for ( i = 0; i < 100; i++ )
	{
	    status = nx_module_get_status(module);
	    if ( !((status == NX_MODULE_STATUS_RUNNING) || (status == NX_MODULE_STATUS_PAUSED)) )
	    {
		apr_sleep(APR_USEC_PER_SEC / 10);
	    }
	    else
	    {
		break;
	    }
	}
	if ( i == 100 ) 
	{
	    if ( module->job != NULL )
	    {
		if ( nx_atomic_read32(&(module->job->busy)) == TRUE )
		{
		    log_error("failed to start module %s, module is busy", module->name);
		}
		else
		{
		    try
		    {
			nx_module_start_self(module);
		    }
		    catch(e)
		    {
			log_exception(e);
		    }
		}
	    }
	}
    }
*/
    // if a module is busy but has fired an event for itself
    nxlog = nxlog_get();
    nx_lock();
    apr_thread_cond_signal(nxlog->worker_cond);
    nx_unlock();
}



void nx_ctx_stop_modules(nx_ctx_t *ctx, nx_module_type_t type)
{
    nx_module_t * volatile module;
    int i;
    nx_exception_t e;
    nx_module_status_t status;

    log_debug("stopping %s modules", nx_module_type_to_string(type));

    for ( module = NX_DLIST_FIRST(ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	if ( module->type != type )
	{
	    continue;
	}
	log_debug("stopping module %s", module->name);
#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size before stop is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif
    	for ( i = 0; i < 100; i++ )
	{
	    status = nx_module_get_status(module);
	    if ( (status == NX_MODULE_STATUS_RUNNING) || (status == NX_MODULE_STATUS_PAUSED) )
	    {
		nx_module_stop(module);
		apr_sleep(APR_USEC_PER_SEC / 10);
	    }
	    else
	    {
		break;
	    }
	}
	if ( i == 100 ) 
	{
	    if ( module->job != NULL )
	    {
		if ( nx_atomic_read32(&(module->job->busy)) == TRUE )
		{
		    log_error("failed to stop module %s, module is busy", module->name);
		}
		else
		{
		    try
		    {
			nx_module_stop_self(module);
		    }
		    catch(e)
		    {
			log_exception(e);
		    }
		}
	    }
	}
#if (APR_POOL_DEBUG == 1)
	log_info("%s module pool size after stop is %"APR_SIZE_T_FMT" non-recursive, %"APR_SIZE_T_FMT" with children",
		 module->name, apr_pool_num_bytes(module->pool, FALSE),
		 apr_pool_num_bytes(module->pool, TRUE));
#endif
    }
}



/**
 * This is called after the threads are stopped, so async mode
 * does not work here.
 */
void nx_ctx_shutdown_modules(nx_ctx_t *ctx, nx_module_type_t type)
{
    nx_module_t * volatile module, *tmp;
    volatile int i;
    nx_exception_t e;

    log_debug("shutdown_modules: %s", nx_module_type_to_string(type));

    ASSERT(ctx->modules != NULL);
    for ( module = NX_DLIST_FIRST(ctx->modules); module != NULL; )
    {
	tmp = module;
	module = NX_DLIST_NEXT(module, link);
	if ( tmp->type != type )
	{
	    continue;
	}
	if ( nx_module_get_status(tmp) == NX_MODULE_STATUS_UNINITIALIZED )
	{
	    continue;
	}

	NX_DLIST_REMOVE(ctx->modules, tmp, link);
	if ( tmp->job != NULL )
	{
	    // wait 10 sec at most
	    for ( i = 0; i < 100; i++ )
	    {
		if ( nx_atomic_read32(&(tmp->job->busy)) == TRUE )
		{
		    nx_module_shutdown(tmp);
		    apr_sleep(APR_USEC_PER_SEC / 10);
		}
		else
		{
		    try
		    {
			nx_module_shutdown_self(tmp);
		    }
		    catch(e)
		    {
			log_exception(e);
		    }
		    break;
		}
	    }
	    if ( i == 100 )
	    {
		log_error("failed to shutdown module %s, module is busy", tmp->name);
	    }
	}
	else
	{
	    try
	    {
		nx_module_shutdown_self(tmp);
	    }
	    catch(e)
	    {
		log_exception(e);
	    }
	}
    }
}



void nx_ctx_save_queues(nx_ctx_t *ctx)
{
    nx_module_t * volatile module;
    nx_exception_t e;

    // FIXME: limit module and route names to 64 chars 

    ASSERT(ctx != NULL);

    ASSERT(ctx->modules != NULL);
    for ( module = NX_DLIST_FIRST(ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	// input modules do not have a queue
	if ( module->queue != NULL )
	{
	    try
	    {
		nx_logqueue_to_file(module->queue);
	    }
	    catch(e)
	    {
		// just log the error, stopping completely does not help
		log_exception(e);
	    }
	}
    }
}



void nx_ctx_restore_queues(nx_ctx_t *ctx)
{
    nx_module_t * volatile module;
    nx_exception_t e;

    // FIXME: limit module and route names to 64 chars 

    ASSERT(ctx != NULL);

    ASSERT(ctx->modules != NULL);
    for ( module = NX_DLIST_FIRST(ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	// input modules do not have a queue
	if ( module->queue != NULL )
	{
	    try
	    {
		nx_logqueue_from_file(module->queue);
	    }
	    catch(e)
	    {
		// log the error and continue
		log_exception(e);
	    }
	}
    }
}
