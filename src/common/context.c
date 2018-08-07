/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include "exception.h"
#include "context.h"


#define NX_LOGMODULE NX_LOGMODULE_CORE


static nx_context_t _global_context;
static apr_threadkey_t *_context_key = NULL;

#include <apr_portable.h>

struct exception_context *nx_get_exception_context()
{
    nx_context_t *context;
    apr_status_t rv;

    if ( _context_key == NULL )
    {
	nx_abort("context_key is NULL, aborting.");
    }
    if ( (rv = apr_threadkey_private_get((void **) &context, _context_key)) != APR_SUCCESS )
    {
	log_aprerror(rv, "couldn't get context by threadkey");
	nx_abort("aborting");
    }

    return ( &(context->exception_context) );
}



nx_context_t *nx_get_context()
{
    return ( &_global_context );
}



nx_context_t *nx_init_context()
{
    nx_context_t *context = NULL;
    apr_status_t rv;

    context = nx_get_context();

    memset(context, 0, sizeof(nx_context_t));
    if ( (rv = apr_pool_create(&(context->pool), NULL)) != APR_SUCCESS )
    {
	log_aprerror(rv, "couldn't create memory pool");
	nx_abort("aborting");
    }

    init_exception_context(&(context->exception_context));

    if ( (rv = apr_threadkey_private_create(&_context_key, NULL, context->pool)) != APR_SUCCESS )
    {
	log_aprerror(rv, "couldn't create threadkey");
	nx_abort("aborting");
    }

    if ( (rv = apr_threadkey_private_set(context, _context_key)) != APR_SUCCESS )
    {
	log_aprerror(rv, "couldn't set context by threadkey");
	nx_abort("aborting");
    }

    return ( context );
}



apr_threadkey_t *nx_get_context_key()
{
    ASSERT(_context_key != NULL);
    return ( _context_key );
}
