/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/common/date.h"
#include "../../src/common/alloc.h"
#include "../../src/core/nxlog.h"
#include "../../src/core/core.h"


#define NX_LOGMODULE NX_LOGMODULE_TEST

nxlog_t nxlog;

#define LOOPCNT 1000

int main(int argc UNUSED, const char * const *argv, const char * const *env UNUSED)
{
    int i, j;
    apr_pool_t *pool, *mainpool;
    apr_allocator_t *allocator;
    nx_logdata_t *logdata;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);

    mainpool = nx_pool_create_core();

    for ( i = 0; i < LOOPCNT; i++ )
    {
	pool = nx_pool_create_core();
	allocator = apr_pool_allocator_get(pool);
	ASSERT(apr_allocator_owner_get(allocator) == pool);
  
	for ( j = 0; j < LOOPCNT; j++ )
	{
	    logdata = apr_palloc(pool, sizeof(nx_logdata_t));
	}
	apr_pool_destroy(pool);
    }

    for ( i = 0; i < LOOPCNT; i++ )
    {
	pool = nx_pool_create_child(mainpool);
  
	for ( j = 0; j < LOOPCNT; j++ )
	{
	    logdata = apr_palloc(pool, sizeof(nx_logdata_t));
	}
	apr_pool_destroy(pool);
    }

    apr_terminate();

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
