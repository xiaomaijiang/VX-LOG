/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "router.h"
#include "modules.h"
#include "../common/cfgfile.h"
#include "../common/error_debug.h"
#include "../common/alloc.h"

#include <apr_lib.h>
#include <apr_thread_proc.h>
#include <apr_tables.h>

#define NX_LOGMODULE NX_LOGMODULE_CORE



static void nx_route_add_module(const char *start,
				const char *end,
				const nx_directive_t *routeconf,
				const nx_ctx_t *ctx,
				nx_route_t *route)
{
    char modname[128];
    int len;
    nx_module_t *module;
    nx_exception_t e;

    for ( ; apr_isspace(*start) && (end > start); start++ ); // trim leading space
    for ( ; apr_isspace(*end) && (end > start); end-- ); // chop trailing space
    end++;
    ASSERT(start < end);

    len = (int) (end - start) + 1;

    if ( len >= (int) sizeof(modname) )
    {
	nx_conf_error(routeconf, "module name too long: %s", start);
    }
    
    memset(modname, 0, sizeof(modname));
    apr_cpystrn(modname, start, (apr_size_t) len);

    // find module
    for ( module = NX_DLIST_FIRST(ctx->modules);
	  module != NULL;
	  module = NX_DLIST_NEXT(module, link) )
    {
	if ( strcasecmp(module->name, modname) == 0 )
	{
	    break;
	}
    }
    if ( module == NULL )
    {
	if ( ctx->ignoreerrors != TRUE )
	{
	    nx_conf_error(routeconf, "module '%s' not declared", modname);
	}
	else
	{
	    try
	    {
		nx_conf_error(routeconf, "module '%s' is not declared", modname);
	    }
	    catch(e)
	    {
		log_exception(e);
	    }
	    return;
	}
    }
    if ( module->has_config_errors )
    {
	if ( ctx->ignoreerrors != TRUE )
	{
	    nx_conf_error(routeconf, "module '%s' has configuration errors", modname);
	}
	else
	{
	    try
	    {
		nx_conf_error(routeconf, "module '%s' has configuration errors, not adding to route '%s'",
			      modname, route->name);
	    }
	    catch(e)
	    {
		log_exception(e);
	    }
	    return;
	}
    }

    if ( (module->type == NX_MODULE_TYPE_PROCESSOR) && (module->routes->nelts >= 1) )
    {
	nx_route_t *r2;
	
	r2 = ((nx_route_t **)module->routes->elts)[0];
	if ( ctx->ignoreerrors != TRUE )
	{
	    nx_conf_error(routeconf, "cannot add processor module '%s' to route '%s' "
			  "because it is already added to route '%s', you should define another instance",
			  modname, route->name, r2->name);
	}
	else
	{
	    try
	    {
		nx_conf_error(routeconf, "cannot add processor module '%s' to route '%s' "
			      "because it is already added to route '%s', you should define another instance",
			      modname, route->name, r2->name);
	    }
	    catch(e)
	    {
		log_exception(e);
	    }
	    return;
	}
  
    }

    log_debug("adding module %s to route %s", module->name, route->name);

    *((nx_route_t **) apr_array_push(module->routes)) = route;
    *((nx_module_t **) apr_array_push(route->modules)) = module;
    
    if ( module->priority == 0 )
    {
	module->priority = route->priority;
    }
    else if ( module->priority > route->priority )
    {
	module->priority = route->priority;
    }
    (module->refcount)++;
}



/**
 * Cannot remove from the array, just decrement the refcount
 * So that it can be marked as unused.
 */
static void nx_route_remove_modules(nx_route_t *route)
{
    nx_module_t *module;
    int i;

    ASSERT(route != NULL);

    for ( i = 0; i < route->modules->nelts; i++ )
    {
	module = ((nx_module_t **) route->modules->elts)[i];
	(module->refcount)--;
    }
}



static void nx_route_free(nx_route_t **route)
{
    ASSERT(route != NULL);

    if ( *route == NULL )
    {
	return;
    }
    apr_pool_destroy((*route)->pool);

    *route = NULL;
}



static nx_route_t *nx_route_new(const nx_ctx_t *ctx)
{
    nx_route_t *route;
    apr_pool_t *pool;

    pool = nx_pool_create_child(ctx->pool);
    route = apr_pcalloc(pool, sizeof(nx_route_t));
    route->pool = pool;
    route->modules = apr_array_make(pool, 5, sizeof(nx_module_t *));

    return ( route );
}



static boolean nx_add_route(const nx_ctx_t *ctx,
			    const nx_directive_t *routeconf,
			    const char *routename)
{
    const char *path = NULL;
    const char *curr, *start, *pathend;
    int num_arrow = 0;
    int arrows_read = 0;
    nx_route_t *route = NULL;
    const nx_directive_t *directive;
    int priority = 10;
    nx_exception_t e;
    int volatile num_input = 0;
    int volatile num_output = 0;
    int i;
    nx_module_t *module;

    ASSERT ( routeconf != NULL );

    if ( (routeconf->first_child == NULL) ||
	 ((path = nx_cfg_get_value(routeconf->first_child, "path")) == NULL) )
    {
	nx_conf_error(routeconf, "path missing from route '%s'", routename);
    }
    routeconf = routeconf->first_child;
    for ( directive = routeconf; directive != NULL; directive = directive->next )
    {
	if ( strcasecmp(directive->directive, "path") == 0 )
	{
	}
	else if ( strcasecmp(directive->directive, "priority") == 0 )
	{
	    priority = atoi(directive->args);
	    if ( (priority < 1) || (priority > 100) )
	    {
		nx_conf_error(directive, "invalid priority: %d", priority);
	    }
	    priority += 10; // offset priority by 10, 1-10 is for system use
	}
	else
	{
	    nx_conf_error(directive, "invalid keyword under <Route>: %s",
			   directive->directive);
	}
    }

    route = nx_route_new(ctx);
    route->name = apr_pstrdup(route->pool, routename);
    route->priority = priority;

    // parse path
    log_debug("parsing path: %s", path);

    start = path;
    pathend = path + strlen(path);

    for ( curr = start; curr <= pathend - 2; curr++ )
    {
	if ( strncmp(curr, "=>", 2) == 0 )
	{
	    num_arrow++;
	}
    }

    if ( num_arrow == 0 )
    {
	nx_route_free(&route);
	nx_conf_error(routeconf, "invalid path");
    }

    CHECKERR(apr_thread_mutex_create(&(route->mutex), APR_THREAD_MUTEX_UNNESTED, route->pool));

    try
    {
	for ( curr = start; curr <= pathend; curr++ )
	{
	    if ( apr_isspace(*curr) )
	    {
		continue;
	    }
	    if ( (*curr == ',') || (*curr == '\0') || (strncmp(curr, "=>", 2) == 0)  )
	    {
		if ( start == curr )
		{
		    nx_conf_error(routeconf, "invalid leading comma/arrow in path %s", path);
		}
		
		if ( start >= curr - 1 )
		{
		    nx_conf_error(routeconf, "invalid path %s", path);
		}
		nx_route_add_module(start, curr - 1, routeconf, ctx, route);
	    
		if ( (*curr == ',') || (*curr == '\0') )
		{
		    if ( curr + 1 < pathend )
		    {
			start = curr + 1;
		    }
		    else if ( *curr == ',' )
		    {
			nx_conf_error(routeconf, "invalid comma/arrow in path %s", path);
		    }
		}
		else
		{ // =>
		    ASSERT(*curr == '=');
		    if ( curr + 2 < pathend )
		    {
			start = curr + 2;
		    }
		    else
		    {
			nx_conf_error(routeconf, "invalid arrow in path %s", path);
		    }
		
		    curr++;
		    arrows_read++;
		}
	    }
	}
    }
    catch(e)
    {
	if ( ctx->ignoreerrors == TRUE )
	{
	    log_exception(e);
	    return ( FALSE );
	}
	else
	{
	    nx_route_free(&route);
	    rethrow(e);
	}
    }

    for ( i = 0; i < route->modules->nelts; i++ )
    {
	module = ((nx_module_t **) route->modules->elts)[i];
	if ( module->type == NX_MODULE_TYPE_INPUT )
	{
	    num_input++;
	}
	else if ( module->type == NX_MODULE_TYPE_OUTPUT )
	{
	    num_output++;
	}
    }

    try
    {
	if ( num_input == 0 )
	{
	    nx_conf_error(routeconf, "route %s is not functional without input modules, ignored", route->name);
	}
	if ( num_output == 0 )
	{
	    nx_conf_error(routeconf, "route %s is not functional without output modules, ignored", route->name);
	}
    }
    catch(e)
    {
	if ( ctx->ignoreerrors == TRUE )
	{
	    nx_route_remove_modules(route);
	    log_exception(e);
	    return ( FALSE );
	}
	else
	{
	    nx_route_free(&route);
	    rethrow(e);
	}
    }

    NX_DLIST_INSERT_TAIL(ctx->routes, route, link);

    return ( TRUE );
}



void nx_ctx_init_routes(nx_ctx_t *ctx)
{
    const nx_directive_t *curr = ctx->cfgtree;
    int num_routes = 0;

    ASSERT(ctx->cfgtree != NULL);

    while ( curr != NULL )
    {
	if ( strcasecmp(curr->directive, "route") == 0 )
	{
	    if ( nx_add_route(ctx, curr, curr->args) == TRUE )
	    {
		num_routes++;
	    }
	}
	curr = curr->next;
    }

    if ( num_routes == 0 )
    {
	log_warn("no routes defined!");
    }
}

