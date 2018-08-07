/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <unistd.h>
#include <apr_getopt.h>

#include "../common/error_debug.h"
#include "../common/event.h"
#include "../common/alloc.h"
#include "../core/nxlog.h"
#include "../core/modules.h"
#include "../core/router.h"
#include "../core/ctx.h"
#include "../core/core.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

static void print_usage()
{
    printf(
	   " nxlog-processor [-h/help] [-c/conf conffile] [-v/verify]\n"
	   "   [-h] print help\n"
	   "   [-c conffile] specify an alternate config file\n"
	   "   [-v] verify configuration file syntax\n"
	   );
}


static void parse_cmd_line(nxlog_t *nxlog, int argc, const char * const *argv)
{
    const char *opt_arg;
    apr_status_t rv;
    apr_getopt_t *opt;
    int ch;

    static const apr_getopt_option_t options[] = {
	{ "help", 'h', 0, "print help" }, 
	{ "conf", 'c', 1, "configuration file" }, 
	{ "verify", 'v', 0, "verify configuration file syntax" },
	{ NULL, 0, 1, NULL }, 
    };
	
    apr_getopt_init(&opt, nxlog->pool, argc, argv);
    while ( (rv = apr_getopt_long(opt, options, &ch, &opt_arg)) == APR_SUCCESS )
    {
	switch ( ch )
	{
	    case 'c':	/* configuration file */
		nxlog->cfgfile = apr_pstrdup(nxlog->pool, opt_arg);
		break;
	    case 'h':	/* help */
		print_usage();
		exit(-1);
	    case 'v':	/* verify */
		nxlog->verify_conf = TRUE;
		break;
	    default:
		print_usage();
		exit(-1);
	}
    }

    if ( (rv != APR_SUCCESS) && (rv != APR_EOF) )
    {
        throw(rv, "Could not parse options");
    }
}



/**
 * The whole thing starts here
 */

int main(int argc, const char * const *argv, const char * const *env)
{
    nxlog_t nxlog;
    nx_exception_t e;

    ASSERT(nx_init(&argc, &argv, &env) == TRUE);
    
    //atexit(nxlog_exit_function);

    nxlog_init(&nxlog);

    try
    {
	// read cmd line
	parse_cmd_line(&nxlog, argc, argv);

	// load and parse config
	nx_ctx_parse_cfg(nxlog.ctx, nxlog.cfgfile);

	nx_ctx_init_logging(nxlog.ctx);

	// read config cache
	nx_config_cache_read();

	// load DSO and read and verify module config
	nx_ctx_config_modules(nxlog.ctx);

	if ( nxlog.verify_conf == TRUE )
	{
	    log_info("configuration OK");
	    exit(0);
	}

	// initialize modules
	nx_ctx_init_modules(nxlog.ctx);

	// initialize log routes
	nx_ctx_init_routes(nxlog.ctx);

	nx_ctx_init_jobs(nxlog.ctx);

	// setup threadpool
	nxlog_create_threads(&nxlog);

	nx_ctx_start_modules(nxlog.ctx);
    }
    catch(e)
    {
	log_exception(e);
	exit(1);
    }

    // mainloop
    nxlog_mainloop(&nxlog, TRUE);

    nxlog_shutdown(&nxlog);

    nx_ctx_free(nxlog.ctx);
    apr_pool_destroy(nxlog.pool);
    apr_terminate();

    return ( 0 );
}
