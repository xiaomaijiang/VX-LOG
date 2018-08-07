/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "../../../common/module.h"
#include "xm_xml.h"

#include "xml.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


void nx_expr_proc__parse_xml(nx_expr_eval_ctx_t *eval_ctx,
			      nx_module_t *module,
			      nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t value;
    nx_xm_xml_conf_t *modconf;
    nx_exception_t e;
    nx_xml_parser_ctx_t ctx;

    ASSERT(module != NULL);
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to parse_xml(), possibly dropped");
    }

    modconf = (nx_xm_xml_conf_t *) module->config;
    ASSERT(modconf != NULL);

    memset(&ctx, 0, sizeof(nx_xml_parser_ctx_t));
    ctx.logdata = eval_ctx->logdata;

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
	    nx_xml_parse(&ctx, value.string->buf, value.string->len);
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
	nx_xml_parse(&ctx, value.string->buf, value.string->len);
    }
}



void nx_expr_func__to_xml(nx_expr_eval_ctx_t *eval_ctx,
			   nx_module_t *module,
			   nx_value_t *retval,
			   int32_t num_arg,
			   nx_value_t *args UNUSED)
{
    nx_xm_xml_conf_t *modconf;
    nx_xml_parser_ctx_t ctx;

    memset(&ctx, 0, sizeof(nx_xml_parser_ctx_t));
    ctx.logdata = eval_ctx->logdata;

    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);
    ASSERT(module != NULL);
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to to_xml(), possibly dropped");
    }

    modconf = (nx_xm_xml_conf_t *) module->config;
    ASSERT(modconf != NULL);

    retval->string = nx_logdata_to_xml(&ctx);
    retval->type = NX_VALUE_TYPE_STRING;
    retval->defined = TRUE;
}



void nx_expr_proc__to_xml(nx_expr_eval_ctx_t *eval_ctx,
			   nx_module_t *module,
			   nx_expr_list_t *args UNUSED)
{
    nx_xm_xml_conf_t *modconf;
    nx_value_t *val;
    nx_string_t *xmlstr;
    nx_xml_parser_ctx_t ctx;

    memset(&ctx, 0, sizeof(nx_xml_parser_ctx_t));
    ctx.logdata = eval_ctx->logdata;

    ASSERT(module != NULL);
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to to_xml(), possibly dropped");
    }

    modconf = (nx_xm_xml_conf_t *) module->config;
    ASSERT(modconf != NULL);

    xmlstr = nx_logdata_to_xml(&ctx);

    val = nx_value_new(NX_VALUE_TYPE_STRING);
    val->string = xmlstr;
    nx_logdata_set_field_value(eval_ctx->logdata, "raw_event", val);
}
