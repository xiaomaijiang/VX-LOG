/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "resource.h"
#include "error_debug.h"
#include "module.h"
#include "../core/ctx.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


void nx_resource_put(const char *name, nx_resource_type_t type, const void *data)
{
    nx_ctx_t *ctx;
    nx_resource_t *resource;

    ASSERT(data != NULL);

    ctx = nx_ctx_get();
    resource = apr_pcalloc(ctx->pool, sizeof(nx_resource_t));
    resource->type = type;
    resource->name = apr_pstrdup(ctx->pool, name);
    resource->data = data;

    log_debug("storing data for resource '%s'", name);

    nx_lock();
    NX_DLIST_INSERT_TAIL(ctx->resources, resource, link);
    nx_unlock();
}



const void *nx_resource_get(const char *name, nx_resource_type_t type)
{
    nx_ctx_t *ctx;
    nx_resource_t *resource;
    const void *retval = NULL;

    ASSERT(name != NULL);

    log_debug("resource data request for '%s'", name);
    ctx = nx_ctx_get();
    nx_lock();
    for ( resource = NX_DLIST_FIRST(ctx->resources);
	  resource != NULL;
	  resource = NX_DLIST_NEXT(resource, link) )
    {
	if ( (resource->type == type) && (strcasecmp(name, resource->name) == 0) )
	{
	    retval = resource->data;
	    log_debug("found resource data for '%s'", name);
	    break;
	}
    }
    nx_unlock();

    return ( retval );
}

