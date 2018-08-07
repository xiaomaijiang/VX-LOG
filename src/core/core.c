/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include "../common/exception.h"
#include "../common/error_debug.h"
#include "../common/context.h"
#include "../common/alloc.h"
#include "../common/atomic.h"
#include "core.h"


#define NX_LOGMODULE NX_LOGMODULE_CORE

#ifdef NX_NOATOMIC
apr_thread_mutex_t *nx_atomic_mutex;
#endif

struct thread_start_data
{
    apr_thread_start_t	startfunc;
    void		*arg;
    apr_status_t	rv;
    apr_thread_cond_t	*cond;
    apr_thread_mutex_t	*mutex;
};
typedef struct thread_start_data thread_start_data;


boolean nx_init(int *argc, char const *const **argv, char const *const **env)
{
    apr_status_t rv;
    nx_context_t *context;
    apr_thread_mutex_t *mutex;

    if ( (rv = apr_app_initialize(argc, argv, env)) != APR_SUCCESS )
    {
	log_aprerror(rv, "apr initialization failed");
	nx_abort("aborting");;
    }

    if ( (context = nx_init_context()) == NULL )
    {
	return ( FALSE );
    }

    ASSERT(apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_UNNESTED, context->pool) == APR_SUCCESS);
    nx_logger_mutex_set(mutex);

#ifdef NX_NOATOMIC
    ASSERT(apr_thread_mutex_create(&nx_atomic_mutex, APR_THREAD_MUTEX_UNNESTED, context->pool) == APR_SUCCESS);
#endif

    return ( TRUE );
}



static void * APR_THREAD_FUNC _thread_helper(apr_thread_t *thd, void *d)
{
    thread_start_data *data = d;
    nx_context_t thread_context;
    void *arg, *ret;
    apr_thread_start_t startfunc;

    startfunc = data->startfunc;
    arg	= data->arg;

    memset(&thread_context, 0, sizeof(nx_context_t));
    init_exception_context(&thread_context.exception_context);

    ASSERT(apr_thread_mutex_lock(data->mutex) == APR_SUCCESS);

    data->rv = apr_threadkey_private_set(&thread_context, nx_get_context_key());

    ASSERT(apr_thread_cond_signal(data->cond) == APR_SUCCESS);
    ASSERT(apr_thread_mutex_unlock(data->mutex) == APR_SUCCESS);

    ret	= startfunc(thd, arg);

    return ( ret );
}



void nx_thread_create(apr_thread_t **thread,
		      apr_threadattr_t *attr,
		      apr_thread_start_t func,
		      void *data,
		      apr_pool_t *pool)
{
    apr_status_t rv;
    thread_start_data thread_data;

    ASSERT(func != NULL);

    memset(&thread_data, 0, sizeof(thread_data));

    ASSERT(apr_thread_mutex_create(&(thread_data.mutex), APR_THREAD_MUTEX_UNNESTED, pool) == APR_SUCCESS);
    ASSERT(apr_thread_cond_create(&(thread_data.cond), pool) == APR_SUCCESS);

    thread_data.startfunc = func;
    thread_data.arg = data;
    thread_data.rv = APR_SUCCESS;
    ASSERT(apr_thread_mutex_lock(thread_data.mutex) == APR_SUCCESS);

    rv = apr_thread_create(thread, attr, _thread_helper, (void *) &thread_data, pool);

    ASSERT(apr_thread_cond_wait(thread_data.cond, thread_data.mutex) == APR_SUCCESS );
    ASSERT(apr_thread_mutex_unlock(thread_data.mutex) == APR_SUCCESS);
    apr_thread_cond_destroy(thread_data.cond);
    apr_thread_mutex_destroy(thread_data.mutex);

    CHECKERR_MSG(rv, "couldn't create thread");
}

