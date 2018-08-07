/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "../../../common/module.h"
#include "xm_csv.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

void nx_expr_proc__parse_csv(nx_expr_eval_ctx_t *eval_ctx,
			     nx_module_t *module,
			     nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t value;
    nx_csv_ctx_t *ctx;
    nx_xm_csv_conf_t *modconf;
    nx_exception_t e;

    ASSERT(module != NULL);
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to parse_csv(), possibly dropped");
    }

    modconf = (nx_xm_csv_conf_t *) module->config;
    ASSERT(modconf != NULL);
    ctx = &(modconf->ctx);

    if ( (args != NULL) && ((arg = NX_DLIST_FIRST(args)) != NULL) )
    {
	ASSERT(arg->expr != NULL);
	nx_expr_evaluate(eval_ctx, &value, arg->expr);

	if ( value.defined != TRUE )
	{
	    throw_msg("source string is undef");
	}
	if ( value.type != NX_VALUE_TYPE_STRING )
	{
	    nx_value_kill(&value);
	    throw_msg("string type required for source string");
	}
	try
	{
	    nx_csv_parse(eval_ctx->logdata, ctx, value.string->buf, value.string->len);
	}
	catch(e)
	{
	    nx_value_kill(&value);
	    rethrow(e);
	}
	nx_value_kill(&value);
    }
    else
    {
	if ( nx_logdata_get_field_value(eval_ctx->logdata, "raw_event", &value) == FALSE )
	{
	    throw_msg("raw_event field missing");
	}
	if ( value.defined != TRUE )
	{
	    throw_msg("raw_event field is undef");
	}
	if ( value.type != NX_VALUE_TYPE_STRING )
	{
	    throw_msg("string type required for field 'raw_event'");
	}
	nx_csv_parse(eval_ctx->logdata, ctx, value.string->buf, value.string->len);
    }
}



void nx_expr_func__to_csv(nx_expr_eval_ctx_t *eval_ctx,
			  nx_module_t *module,
			  nx_value_t *retval,
			  int32_t num_arg,
			  nx_value_t *args UNUSED)
{
    nx_xm_csv_conf_t *modconf;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);
    ASSERT(module != NULL);
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to to_csv(), possibly dropped");
    }

    modconf = (nx_xm_csv_conf_t *) module->config;
    ASSERT(modconf != NULL);

    retval->string = nx_logdata_to_csv(&(modconf->ctx), eval_ctx->logdata);
    retval->type = NX_VALUE_TYPE_STRING;
    retval->defined = TRUE;
}



void nx_expr_proc__to_csv(nx_expr_eval_ctx_t *eval_ctx,
			  nx_module_t *module,
			  nx_expr_list_t *args UNUSED)
{
    nx_xm_csv_conf_t *modconf;
    nx_value_t *val;
    nx_string_t *csvstr;

    ASSERT(module != NULL);
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to to_csv(), possibly dropped");
    }

    modconf = (nx_xm_csv_conf_t *) module->config;
    ASSERT(modconf != NULL);

    csvstr = nx_logdata_to_csv(&(modconf->ctx), eval_ctx->logdata);

    val = nx_value_new(NX_VALUE_TYPE_STRING);
    val->string = csvstr;
    nx_logdata_set_field_value(eval_ctx->logdata, "raw_event", val);
}
