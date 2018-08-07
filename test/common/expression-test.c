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
#include "../../src/common/date.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

nxlog_t nxlog;

typedef struct testpair
{
    const char *condition;
    nx_value_type_t type;
    struct result
    {
	int64_t integer;
	boolean boolean;
	const char *string;
    } result;
} testpair;


// valid expressions
static testpair testcases[] =
{
    { "TRUE", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "FALSE", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   
    { "true", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "false", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   
    { "True", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "False", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   

    { "TRUE == TRUE", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "FALSE == FALSE", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "TRUE != TRUE", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   
    { "FALSE != FALSE", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   

    { "FALSE and FALSE", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   
    { "FALSE and TRUE", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   
    { "TRUE and FALSE", NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },   

    { "TRUE or FALSE", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "FALSE or TRUE", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   

    { "not FALSE", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   

    { "1k == 1k", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1024 == 1k", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1M == 1M", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1K * 1K == 1M", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1G == 1G", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1G == 1M * 1K", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1 == 1", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1 != 2", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1 != (2 + 3)", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "1 == (2 - 1)", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "2 == (10 / 5)", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "2 == (2 / 1 / 1)", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "10 == (5 * 2)", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "2 == (5 % 3)", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   

    { "127.0.0.1 == 127.0.0.1", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "127.0.0.1 != 192.168.1.1", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   

    { "1970-01-01 00:00:00", NX_VALUE_TYPE_DATETIME, { .string = "1970-01-01 00:00:00" } },   
    { "2010-12-31 23:59:59", NX_VALUE_TYPE_DATETIME, { .string = "2010-12-31 23:59:59" } },   


    { "'test' == 'test'", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   
    { "'test' != 'Test'", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   

    { "'test' =~ /^test/", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },
    { "'test' !~ /^Test/",  NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },

    { "$Message", NX_VALUE_TYPE_STRING, {.string = "Test message"} },
    { "$Message == 'Test message'", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },
    { "$Message =~ /^Test/", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },

    { "lc(\"Test Case\")", NX_VALUE_TYPE_STRING, { .string = "test case" } },

    { "($SyslogFacilityValue == syslog_facility_value('kern')) OR "
      "($SyslogSeverity == syslog_severity_value('error')) OR "
      "($Message =~ /Test message/)", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },

    { "undef == undef", NX_VALUE_TYPE_BOOLEAN, { .boolean = TRUE } },   

    { NULL, NX_VALUE_TYPE_BOOLEAN, { .boolean = FALSE } },
};

// FIXME testcases for invalid expressions


static const char *undefs[] =
{
    "undef == 1",
    "1 == undef",
    "undef == TRUE",
    "undef == FALSE",
    "TRUE == undef",
    "FALSE == undef",

    "undef != 1",
    "1 != undef",
    "undef != TRUE",
    "undef != FALSE",
    "TRUE != undef",
    "FALSE != undef",
    NULL,
};

static void loadmodule(const char *modulename, const char *type, nx_ctx_t *ctx)
{
    nx_module_t *module;
    char dsoname[4096];

    module = apr_pcalloc(ctx->pool, sizeof(nx_module_t));
    module->dsoname = modulename;
    module->name = modulename;

    apr_snprintf(dsoname, sizeof(dsoname),
		 "%s"NX_DIR_SEPARATOR"%s"NX_DIR_SEPARATOR"%s"NX_MODULE_DSO_EXTENSION,
		 ".."NX_DIR_SEPARATOR".."NX_DIR_SEPARATOR"src"NX_DIR_SEPARATOR"modules", type, modulename);

    nx_module_load_dso(module, ctx, dsoname);
    nx_module_register_exports(ctx, module);
}



int main(int argc, const char * const *argv, const char * const *env)
{
    nx_expr_t *expr;
    volatile int i;
    apr_pool_t *pool;
    nx_expr_eval_ctx_t eval_ctx;
    nx_logdata_t *logdata;
    nx_value_t result;
    const char *message = "plain message";
    nx_value_t *value;
    char datestr[30];
    nx_exception_t e;

    nx_init(&argc, &argv, &env);

    memset(&nxlog, 0, sizeof(nxlog_t));
    nxlog_set(&nxlog);
    nxlog.ctx = nx_ctx_new();
    //nxlog.ctx->loglevel = NX_LOGLEVEL_DEBUG;
    nxlog.ctx->loglevel = NX_LOGLEVEL_INFO;
    nx_ctx_register_builtins(nxlog.ctx);

    logdata = nx_logdata_new_logline(message, (int) strlen(message));
    value = nx_value_new_string("Test message");
    nx_logdata_append_field_value(logdata, "Message", value);
    nx_expr_eval_ctx_init(&eval_ctx, logdata, NULL, NULL);
    loadmodule("xm_syslog", "extension", nxlog.ctx);

    for ( i = 0; testcases[i].condition != NULL; i++ )
    {
	pool = nx_pool_create_child(NULL);
	try
	{
	    expr = nx_expr_parse(NULL, testcases[i].condition, pool, NULL, 1, 1);
	    ASSERT(expr != NULL);
	    memset(&result, 0, sizeof(nx_value_t));
	    nx_expr_evaluate(&eval_ctx, &result, expr);
	}
	catch(e)
	{
	    log_error("testcase %d failed: %s", i, testcases[i].condition);
	    log_exception(e);
	    exit(1);
	}
	ASSERT(result.type == testcases[i].type);
	ASSERT(result.defined == TRUE);

	switch ( testcases[i].type )
	{
	    case NX_VALUE_TYPE_STRING:
		if ( strcmp(result.string->buf, testcases[i].result.string) != 0 )
		{
		    nx_abort("expected '%s' for expression '%s', got '%s'", 
			     testcases[i].result.string,
			     testcases[i].condition,
			     result.string->buf);
		}
		break;
	    case NX_VALUE_TYPE_INTEGER:
		if ( result.integer != testcases[i].result.integer )
		{
		    nx_abort("expected '%ld' for expression '%s'", 
			     testcases[i].result.integer,
			     testcases[i].condition);
		}
		break;
	    case NX_VALUE_TYPE_BOOLEAN:
		if ( result.boolean != testcases[i].result.boolean )
		{
		    nx_abort("expected '%s' for expression '%s'", 
			     testcases[i].result.boolean == TRUE ? "TRUE" : "FALSE",
			     testcases[i].condition);
		}
		break;
	    case NX_VALUE_TYPE_DATETIME:
		CHECKERR(nx_date_to_iso(datestr, sizeof(datestr), result.datetime));
		if ( strcmp(datestr, testcases[i].result.string) != 0 )
		{
		    nx_abort("expected '%s' for expression '%s', got '%s'", 
			     testcases[i].result.string,
			     testcases[i].condition,
			     datestr);
		}
		break;
	    default:
		nx_abort("unhandled type: %d", testcases[i].type);
	}
	apr_pool_destroy(pool);
    }


    for ( i = 0; undefs[i] != NULL; i++ )
    {
	pool = nx_pool_create_child(NULL);
	expr = nx_expr_parse(NULL, undefs[i], pool, NULL, 1, 1);
	ASSERT(expr != NULL);
	memset(&result, 0, sizeof(nx_value_t));
	result.defined = TRUE;
	nx_expr_evaluate(&eval_ctx, &result, expr);
	ASSERT(result.defined == FALSE);
	apr_pool_destroy(pool);
    }

    nx_expr_eval_ctx_destroy(&eval_ctx);

    printf("%s:	OK\n", argv[0]);
    return ( 0 );
}
