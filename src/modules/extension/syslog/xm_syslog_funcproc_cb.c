/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include <apr_lib.h>

#include "../../../common/module.h"
#include "syslog.h"
#include "xm_syslog.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

void nx_expr_func__syslog_facility_value(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					 nx_module_t *module UNUSED,
					 nx_value_t *retval,
					 int32_t num_arg,
					 nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("invalid '%s' type argument for function 'syslog_facility_value(string)'",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }
    retval->integer = nx_syslog_facility_from_string(args[0].string->buf);
    retval->defined = TRUE;
}



void nx_expr_func__syslog_facility_string(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					  nx_module_t *module UNUSED,
					  nx_value_t *retval,
					  int32_t num_arg,
					  nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_INTEGER )
    {
	throw_msg("invalid '%s' type argument for function 'syslog_facility_string(integer)'",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    nx_value_init_string(retval, nx_syslog_facility_to_string(args[0].integer));
}



void nx_expr_func__syslog_severity_value(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					 nx_module_t *module UNUSED,
					 nx_value_t *retval,
					 int32_t num_arg,
					 nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_STRING )
    {
	throw_msg("invalid '%s' type argument for function 'syslog_severity_value(string)'",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }
    retval->integer = nx_syslog_severity_from_string(args[0].string->buf);
    retval->defined = TRUE;
}



void nx_expr_func__syslog_severity_string(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					  nx_module_t *module UNUSED,
					  nx_value_t *retval,
					  int32_t num_arg,
					  nx_value_t *args)
{
    ASSERT(retval != NULL);
    ASSERT(num_arg == 1);

    if ( args[0].type != NX_VALUE_TYPE_INTEGER )
    {
	throw_msg("invalid '%s' type argument for function 'syslog_severity_string(integer)'",
		  nx_value_type_to_string(args[0].type));
    }

    retval->type = NX_VALUE_TYPE_STRING;
    if ( args[0].defined == FALSE )
    {
	retval->defined = FALSE;
	return;
    }

    nx_value_init_string(retval, nx_syslog_severity_to_string(args[0].integer));
}



void nx_expr_proc__parse_syslog_ietf(nx_expr_eval_ctx_t *eval_ctx,
				     nx_module_t *module UNUSED,
				     nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t value;

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available for parse_syslog_ietf(), possibly dropped");
    }

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
	nx_syslog_parse_rfc5424(eval_ctx->logdata, value.string->buf, value.string->len);
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
	nx_syslog_parse_rfc5424(eval_ctx->logdata, value.string->buf, value.string->len);
    }
}



void nx_expr_proc__parse_syslog(nx_expr_eval_ctx_t *eval_ctx,
				nx_module_t *module,
				nx_expr_list_t *args)
{
    nx_expr_proc__parse_syslog_ietf(eval_ctx, module, args);
}



void nx_expr_proc__parse_syslog_bsd(nx_expr_eval_ctx_t *eval_ctx,
				    nx_module_t *module UNUSED,
				    nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t value;

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available for parse_syslog_bsd(), possibly dropped");
    }

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
	nx_syslog_parse_rfc3164(eval_ctx->logdata, value.string->buf, value.string->len);
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
	nx_syslog_parse_rfc3164(eval_ctx->logdata, value.string->buf, value.string->len);
    }
}



void nx_expr_proc__to_syslog_bsd(nx_expr_eval_ctx_t *eval_ctx,
				 nx_module_t *module UNUSED,
				 nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available for to_syslog_bsd(), possibly dropped");
    }

    if ( (args != NULL) && ((arg = NX_DLIST_FIRST(args)) != NULL) )
    {
	throw_msg("unexpected arguments");
    }
    nx_logdata_to_syslog_rfc3164(eval_ctx->logdata);
}



void nx_expr_proc__to_syslog_ietf(nx_expr_eval_ctx_t *eval_ctx,
				  nx_module_t *module,
				  nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_xm_syslog_conf_t *modconf;

    ASSERT(module != NULL);
    
    modconf = (nx_xm_syslog_conf_t *) module->config;
    ASSERT(modconf != NULL);

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available for to_syslog_ietf(), possibly dropped");
    }

    if ( (args != NULL) && ((arg = NX_DLIST_FIRST(args)) != NULL) )
    {
	throw_msg("unexpected arguments");
    }
    nx_logdata_to_syslog_rfc5424(eval_ctx->logdata, modconf->ietftimestampingmt);
}



void nx_expr_proc__to_syslog_snare(nx_expr_eval_ctx_t *eval_ctx,
				   nx_module_t *module UNUSED,
				   nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_xm_syslog_conf_t *modconf;
    uint64_t evtcnt = 0;
    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available for to_syslog_snare(), possibly dropped");
    }

    modconf = (nx_xm_syslog_conf_t *) module->config;
    ASSERT(modconf != NULL);

    if ( (args != NULL) && ((arg = NX_DLIST_FIRST(args)) != NULL) )
    {
	throw_msg("unexpected arguments");
    }
    if (eval_ctx->module != NULL)
    {
	evtcnt = eval_ctx->module->evt_recvd;
    }
    nx_logdata_to_syslog_snare(eval_ctx->logdata, 
			       evtcnt,
			       modconf->snaredelimiter,
			       modconf->snarereplacement);
}
