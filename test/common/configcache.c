/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/common/config_cache.h"
#include "../../src/core/nxlog.h"
#include "../../src/core/core.h"
#include "../../src/core/modules.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

nxlog_t nxlog;


int main(int argc, const char * const *argv, const char * const *env)
{
    int64_t intval;
    const char *stringval;

    nx_init(&argc, &argv, &env);

    memset(&nxlog, 0, sizeof(nxlog_t));
    nxlog_set(&nxlog);
    nxlog.ctx = nx_ctx_new();
//    nxlog.ctx->loglevel = NX_LOGLEVEL_DEBUG;
    nxlog.ctx->loglevel = NX_LOGLEVEL_INFO;

    intval = 42;
    ASSERT(nx_config_cache_get_int("test", "ikey1", &intval) == FALSE);
    ASSERT(intval == 42);

    stringval = NULL;
    ASSERT(nx_config_cache_get_string("test", "skey2", &stringval) == FALSE);
    ASSERT(stringval == NULL);

    nx_config_cache_set_int("test", "ikey1", 4242);
    ASSERT(nx_config_cache_get_int("test", "ikey1", &intval) == TRUE);
    ASSERT(intval == 4242);

    nx_config_cache_set_int("test", "ikey2", 42);
    ASSERT(nx_config_cache_get_int("test", "ikey2", &intval) == TRUE);
    ASSERT(intval == 42);

    nx_config_cache_set_string("test", "skey1", "test1");
    ASSERT(nx_config_cache_get_string("test", "skey1", &stringval) == TRUE);
    ASSERT(strcmp(stringval, "test1") == 0);

    nx_config_cache_set_string("test", "skey2", "test2");
    ASSERT(nx_config_cache_get_string("test", "skey2", &stringval) == TRUE);
    ASSERT(strcmp(stringval, "test2") == 0);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
