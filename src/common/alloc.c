/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "types.h"
#include "exception.h"
#include "alloc.h"


#define NX_LOGMODULE NX_LOGMODULE_CORE

static int abort_func(int code) NORETURN; 
static int abort_func(int code)
{
    throw(code, "memory pool allocation error");
}



apr_pool_t *nx_pool_create_child(apr_pool_t *parent)
{
    apr_pool_t *pool;
    apr_allocator_t *allocator;

    CHECKERR(apr_pool_create_ex(&pool, parent, &abort_func, NULL));
    if ( parent == NULL )
    {
	allocator = apr_pool_allocator_get(pool);
	ASSERT(allocator != NULL);
	apr_allocator_max_free_set(allocator, NX_MAX_ALLOCATOR_SIZE);
    }

    return ( pool );
}



apr_pool_t *nx_pool_create_core()
{
    apr_pool_t *pool;
    apr_allocator_t *allocator;
    apr_thread_mutex_t *mutex;

    CHECKERR(apr_allocator_create(&allocator));
    
#ifdef HAVE_APR_POOL_CREATE_UNMANAGED_EX
    // older apr does not have this
    CHECKERR(apr_pool_create_unmanaged_ex(&pool, &abort_func, allocator)); 
#else
    CHECKERR(apr_pool_create_ex(&pool, NULL, &abort_func, allocator)); 
#endif
    allocator = apr_pool_allocator_get(pool);
    apr_allocator_owner_set(allocator, pool);
    ASSERT(allocator != NULL);
    apr_allocator_max_free_set(allocator, NX_MAX_ALLOCATOR_SIZE);
    CHECKERR(apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_UNNESTED, pool));
    apr_allocator_mutex_set(allocator, mutex);

    return ( pool );
}
