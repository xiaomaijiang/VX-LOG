/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_file_io.h>

#include "../../src/common/exception.h"
#include "../../src/core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static void* APR_THREAD_FUNC threadfunc(apr_thread_t *thd, void *data UNUSED)
{
    nx_exception_t e;
    volatile int i;
    volatile int j = 0;
    //const char *threadnum = data;

    //log_info("%s", threadnum);

    for ( i = 0; i < 100000; i++ )
    {
	try
	{
	    throw_msg("test exception");
	}
	catch(e)
	{
	    j++;
	}
    }
    ASSERT(j == i);
    //log_info("%d done", threadnum);
    
    apr_thread_exit(thd, APR_SUCCESS);

    return ( NULL );
}


static void thrower() NORETURN;
static void thrower()
{
    throw_msg("test exception");
}



static void rethrower()
{
    nx_exception_t e;

    try
    {
	thrower();
    }
    catch(e)
    {
	rethrow_msg(e, "added message in rethrow");
    }
}



int main(int argc, const char * const *argv, const char * const *env)
{
    nx_exception_t e;
    apr_status_t rv;
    apr_pool_t *pool;
    apr_file_t *file;
    apr_thread_t *thd1;
    apr_thread_t *thd2;
    volatile int i = 0;
    apr_threadattr_t *attr;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    if ( (rv = apr_pool_create(&pool, NULL)) != APR_SUCCESS )
    {
	log_aprerror(rv, "couldn't create memory pool");
	nx_abort("aborting");
    }

    try
    {
	CHECKERR_MSG(apr_file_open(&file, "thisdoesntexist", APR_READ, APR_OS_DEFAULT, pool),
		     "failed to open %s", "thefile");
    }
    catch(e)
    {
	//log_exception(e);
	i++;
    }

    try
    {
	CHECKERR(apr_file_open(&file, "thisdoesntexist", APR_READ, APR_OS_DEFAULT, pool));
    }
    catch(e)
    {
	//log_exception(e);
	i++;
    }

    try
    {
	rethrower();
    }
    catch(e)
    {
	//nx_log_exception(NX_LOGLEVEL_DEBUG, NX_LOGMODULE_CORE, &e);
	i++;
    }
    ASSERT(i == 3);
/*
    try
    {
	throw(APR_EBADF, "ebadf test");
    }
    catch(e)
    {
	log_exception(e);
    }
*/
    CHECKERR(apr_threadattr_create(&attr, pool));
    CHECKERR(apr_threadattr_detach_set(attr, 0));

    nx_thread_create(&thd1, attr, threadfunc, (void *) 0, pool);
    nx_thread_create(&thd2, attr, threadfunc, (void *) 1, pool);
    
    CHECKERR(apr_thread_join(&rv, thd1));
    CHECKERR(apr_thread_join(&rv, thd2));

    return ( 0 );
}

