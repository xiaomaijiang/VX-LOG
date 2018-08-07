/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../src/common/error_debug.h"
#include "../../src/common/expr-parser.h"
#include "../../src/common/alloc.h"
#include "../../src/core/nxlog.h"
#include "../../src/core/core.h"
#include "../../src/core/modules.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

nxlog_t nxlog;

static nx_module_t *loadmodule(const char *modulename,
			       const char *type,
			       nx_ctx_t *ctx)
{
    nx_module_t *module;
    char dsoname[4096];

    module = apr_pcalloc(ctx->pool, sizeof(nx_module_t));
    module->dsoname = modulename;
    module->name = modulename;
    module->pool = ctx->pool;
    CHECKERR(apr_thread_mutex_create(&(module->mutex), APR_THREAD_MUTEX_UNNESTED, module->pool));

    apr_snprintf(dsoname, sizeof(dsoname),
		 "%s"NX_DIR_SEPARATOR"%s"NX_DIR_SEPARATOR"%s"NX_MODULE_DSO_EXTENSION,
		 ".."NX_DIR_SEPARATOR".."NX_DIR_SEPARATOR"src"NX_DIR_SEPARATOR"modules", type, modulename);

    nx_module_load_dso(module, ctx, dsoname);

    return ( module );
}



static void load_conf(const char *fname, nx_ctx_t *ctx)
{
    const char *confname;
    apr_file_t *file;

    confname = apr_psprintf(ctx->pool, "%s.conf", fname);
    
    if ( apr_file_open(&file, confname, APR_READ | APR_BUFFERED,
		       APR_OS_DEFAULT, ctx->pool) != APR_SUCCESS )
    {
	return;
    }
    nx_ctx_parse_cfg(ctx, confname);
    nxlog.ctx->moduledir = ".."NX_DIR_SEPARATOR".."NX_DIR_SEPARATOR"src"NX_DIR_SEPARATOR"modules";
    nx_ctx_config_modules(ctx);
}



static void fail(const char *errormsg) NORETURN;
static void fail(const char *errormsg)
{
    log_error("%s", errormsg);
    exit(1);
}



int main(int argc, const char * const *argv, const char * const *env)
{
    apr_pool_t *pool = NULL;
    nx_expr_eval_ctx_t eval_ctx;
    nx_logdata_t *logdata;
    nx_value_t result;
    const char *message = "<6> Oct 12 12:49:06 host app[12345]: kernel message";
    //nx_value_t *value;
    apr_file_t *input;
    char inputstr[10000];
    apr_size_t inputlen;
    nx_expr_statement_list_t *statements;
    int i;
    nx_module_t *module = NULL;
    nx_exception_t e;

    if ( argc != 2 )
    {
	fail("filename argument required");
    }


    nx_init(&argc, &argv, &env);
    nxlog_init(&nxlog);
    nxlog.ctx->loglevel = NX_LOGLEVEL_INFO;
    for ( i = 0; env[i] != NULL; i++ )
    {
	if (strncmp(env[i], "DEBUG=", 6) == 0)
	{
	    nxlog.ctx->loglevel = NX_LOGLEVEL_DEBUG;
	    break;
	}
    }
    nxlog.ctx->norepeat = FALSE;

    try
    {
	//nx_ctx_register_builtins(nxlog.ctx);
    
	pool = nx_pool_create_child(NULL);

	module = loadmodule("xm_syslog", "extension", nxlog.ctx);
	load_conf(argv[1], nxlog.ctx);

	CHECKERR_MSG(apr_file_open(&input, argv[1],
				   APR_READ | APR_BUFFERED, APR_OS_DEFAULT,
				   pool), "couldn't open file %s", argv[1]);

	memset(inputstr, 0, sizeof(inputstr));
	inputlen = sizeof(inputstr);
	CHECKERR(apr_file_read(input, inputstr, &inputlen));

	statements = nx_expr_parse_statements(NULL, inputstr, pool, NULL, 1, 0);
	ASSERT(statements != NULL);

	for ( i = 0; i < 1; i++ )
	{
	    logdata = nx_logdata_new_logline(message, (int) strlen(message));
	    nx_expr_eval_ctx_init(&eval_ctx, logdata, module, NULL);

	    nx_expr_statement_list_execute(&eval_ctx, statements);
	    memset(&result, 0, sizeof(nx_value_t));
	    if ( nx_logdata_get_field_value(logdata, "success", &result) != TRUE )
	    {
		fail("test didn't return 'success' field");
	    }
	    if ( result.type != NX_VALUE_TYPE_BOOLEAN )
	    {
		fail("boolean type required for 'success' field");
	    }
	    if ( result.defined != TRUE )
	    {
		fail("test failed: returned 'success' is undef");
	    }
	    if ( result.boolean != TRUE )
	    {
		fail("test failed: returned 'success' is FALSE");
	    }

	    nx_logdata_free(logdata);
	    nx_expr_eval_ctx_destroy(&eval_ctx);
	}
    }
    catch(e)
    {
	log_exception_msg(e, "test failed");
	exit(1);
    }

    apr_pool_destroy(pool);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
