/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_getopt.h>
#include "../common/error_debug.h"
#include "../common/expr-parser.h"
#include "../common/alloc.h"
#include "../core/nxlog.h"
#include "../core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

nxlog_t nxlog;

static nx_module_t *load_modules(nx_ctx_t *ctx, const char *currmodule)
{
    nx_module_t *module;
    nx_module_t * volatile retval = NULL;
    char pathname[APR_PATH_MAX];
    char dsoname[APR_PATH_MAX];
    const char *types[] = { "extension", "input", "processor", "output" };
    int i;
    apr_dir_t *dir = NULL;
    apr_status_t rv;
    apr_finfo_t finfo;
    size_t len;
    nx_exception_t e;
    char modulename[128];

    ASSERT(ctx->pool != NULL);

    if ( ctx->moduledir == NULL )
    {
	ctx->moduledir = apr_pstrdup(ctx->pool, NX_MODULEDIR);
    }

    for ( i = 0; i < 4; i++ )
    {
	apr_snprintf(pathname, sizeof(pathname), "%s"NX_DIR_SEPARATOR"%s",
		     ctx->moduledir, types[i]);
	
	dir = NULL;
	CHECKERR_MSG(apr_dir_open(&dir, pathname, ctx->pool),
		     "failed to open directory %s", pathname);
	for ( ; ; )
	{
	    rv = apr_dir_read(&finfo, APR_FINFO_NAME, dir);
	    if ( APR_STATUS_IS_ENOENT(rv) )
	    {
		break;
	    }
	    if ( rv != APR_SUCCESS )
	    {
		throw(rv, "readdir failed on %s", pathname);
	    }

	    len = strlen(finfo.name);

	    if ( len <= strlen(NX_MODULE_DSO_EXTENSION) + 3 )
	    {
		continue;
	    }
	    if ( !((strncmp(finfo.name, "xm_", 3) == 0) ||
		   (strncmp(finfo.name, "im_", 3) == 0) ||
		   (strncmp(finfo.name, "pm_", 3) == 0) ||
		   (strncmp(finfo.name, "om_", 3) == 0)) )
	    {
		continue;
	    }
	    apr_cpystrn(modulename, finfo.name, len - 2);
	    try
	    {
		apr_snprintf(dsoname, sizeof(dsoname), "%s"NX_DIR_SEPARATOR"%s",
			     pathname, finfo.name);
		module = apr_pcalloc(ctx->pool, sizeof(nx_module_t));
		module->dsoname = apr_pstrdup(ctx->pool, modulename);
		module->name = apr_pstrdup(ctx->pool, modulename);
		module->pool = ctx->pool;
		
		if ( (currmodule != NULL) && (strcmp(currmodule, module->name) == 0) )
		{
		    retval = module;
		}
		nx_module_load_dso(module, ctx, dsoname);
		nx_module_register_exports(ctx, module);
	    }
	    catch(e)
	    {
		log_exception(e);
	    }
  	}
    	CHECKERR(apr_dir_close(dir));
    }

    return ( retval );
}



int main(int argc, const char * const *argv, const char * const *env)
{
    apr_pool_t *pool;
    apr_file_t *input;
    char inputstr[10000];
    apr_size_t inputlen;
    nx_expr_statement_list_t *statements = NULL;
    nx_exception_t e;
    const char *modulename = NULL;
    int i;
    nx_module_t *module;

    nx_init(&argc, &argv, &env);

    memset(&nxlog, 0, sizeof(nxlog_t));
    nxlog_set(&nxlog);
    nxlog.ctx = nx_ctx_new();
    nxlog.ctx->moduledir = NULL;
//    nxlog.ctx->loglevel = NX_LOGLEVEL_DEBUG;
    nxlog.ctx->loglevel = NX_LOGLEVEL_INFO;
    pool = nx_pool_create_child(NULL);


    for ( i = 1; i < argc; i++ )
    {
	if ( strcmp(argv[i], "-m") == 0 )
	{
	    if ( i + 1 >= argc )
	    {
		log_error("missing argument for %s", argv[i]);
	    }
	    nxlog.ctx->moduledir = apr_pstrdup(pool, argv[i + 1]);
	    i++;
	}
	else if ( strncmp(argv[i], "--moduledir=", strlen("--moduledir=")) == 0 )
	{
	    nxlog.ctx->moduledir = apr_pstrdup(pool, argv[i] + strlen("--moduledir="));
	}
	else if ( argv[i][0] != '-' )
	{
	    modulename = argv[i];
	}
	else
	{
	    log_error("invalid argument(s): %s", argv[i]);
	    exit(-1);
	}
    }

    nx_ctx_register_builtins(nxlog.ctx);

    module = load_modules(nxlog.ctx, modulename);

    CHECKERR_MSG(apr_file_open_stdin(&input, pool), "couldn't open stdin");

    memset(inputstr, 0, sizeof(inputstr));
    inputlen = sizeof(inputstr);
    CHECKERR(apr_file_read(input, inputstr, &inputlen));
    apr_file_close(input);

    try
    {
	statements = nx_expr_parse_statements(module, inputstr, pool, NULL, 1, 1);
    }
    catch(e)
    {
	log_exception(e);
	exit(1);
    }
    ASSERT(statements != NULL);

    apr_pool_destroy(pool);

    return ( 0 );
}
