/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "om_http.h"
#include "../../../common/module.h"
#include "../../../common/alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


void nx_expr_proc__om_http_set_http_request_path(nx_expr_eval_ctx_t *eval_ctx UNUSED,
						 nx_module_t *module,
						 nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t value;
    nx_om_http_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);

    if ( module != eval_ctx->module )
    {
	throw_msg("private procedure %s->set_http_request_path() called from %s",
		  module->name, eval_ctx->module->name);
    }

    modconf = (nx_om_http_conf_t *) module->config;

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg->expr != NULL);
    nx_expr_evaluate(eval_ctx, &value, arg->expr);

    if ( value.defined != TRUE )
    {
	throw_msg("path is undef");
    }
    if ( value.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&value);
	throw_msg("string type required for 'path'");
    }

    if ( value.string->len >= sizeof(modconf->server.path) )
    {
	nx_value_kill(&value);
	throw_msg("oversized path passed to set_http_request_path");
    }

    // TODO: URLencode
    apr_cpystrn(modconf->server.path, value.string->buf, sizeof(modconf->server.path));

    nx_value_kill(&value);
}
