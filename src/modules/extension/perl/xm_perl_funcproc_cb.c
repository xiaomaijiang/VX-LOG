/*
 * This file is part of the nxlog log collector tool.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 * License:
 * Copyright (C) 2012 by Botond Botyanszki
 * This library is free software; you can redistribute it and/or modify
 * it under the same terms as Perl itself, either Perl version 5.8.5 or,
 * at your option, any later version of Perl 5 you may have available.
 */

#include "../../../common/module.h"
#include "xm_perl.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


static void xm_perl_call(nx_expr_eval_ctx_t *eval_ctx,
			 nx_module_t *module,
			 nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t value;
    nx_xm_perl_conf_t *modconf;
    int cnt;
    nx_exception_t e;

    if ( eval_ctx->logdata == NULL )
    {
	throw_msg("no logdata available to xm_perl->call(), possibly dropped");
    }

    ASSERT(module != NULL);
    modconf = (nx_xm_perl_conf_t *) module->config;

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg != NULL);
    ASSERT(arg->expr != NULL);
    nx_expr_evaluate(eval_ctx, &value, arg->expr);

    if ( value.defined != TRUE )
    {
	throw_msg("'subroutine' string is undef");
    }
    if ( value.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&value);
	throw_msg("string type required for 'subroutine'");
    }

    log_debug("calling perl subroutine: %s", value.string->buf);

    PERL_SET_CONTEXT(modconf->perl_interpreter);
    dTHXa(modconf->perl_interpreter);
    dSP;
    ENTER;
    SAVETMPS;
    PUSHMARK(SP);

    XPUSHs(sv_2mortal(newSViv(PTR2IV(eval_ctx->logdata))));
    PUTBACK;

    cnt = call_pv(value.string->buf, G_EVAL | G_DISCARD);

    SPAGAIN;

    try
    {
	/* check $@ */
	if ( SvTRUE(ERRSV) )
	{
	    log_error("perl subroutine %s failed with an error: \'%s\'",
		      value.string->buf, SvPV_nolen(ERRSV));
	}
/*
    else
    {
	if ( cnt != 0 )
	{
	    log_warn("perl subroutine %s should not return anything, got %d items",
		     value.string->buf, cnt);
	}
    }
*/
    }
    catch(e)
    {
	PUTBACK;
	FREETMPS;
	LEAVE;
	nx_value_kill(&value);
	rethrow(e);
    }
    PUTBACK;
    FREETMPS;
    LEAVE;

    log_debug("perl subroutine %s finished", value.string->buf);

    nx_value_kill(&value);
}




void nx_expr_proc__xm_perl_perl_call(nx_expr_eval_ctx_t *eval_ctx,
				nx_module_t *module,
				nx_expr_list_t *args)
{
    xm_perl_call(eval_ctx, module, args);
}



void nx_expr_proc__xm_perl_call(nx_expr_eval_ctx_t *eval_ctx,
				nx_module_t *module,
				nx_expr_list_t *args)
{
    xm_perl_call(eval_ctx, module, args);
}
