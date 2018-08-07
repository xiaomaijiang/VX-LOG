/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

//#include <apr_lib.h>

#include "pm_buffer.h"
#include "../../../common/module.h"
#include "../../../common/alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

void nx_expr_func__pm_buffer_buffer_size(nx_expr_eval_ctx_t *eval_ctx,
					 nx_module_t *module,
					 nx_value_t *retval,
					 int32_t num_arg,
					 nx_value_t *args UNUSED)
{
    nx_pm_buffer_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    modconf = (nx_pm_buffer_conf_t *) module->config;

    if ( module != eval_ctx->module )
    {
	throw_msg("private function %s->buffer_size() called from %s",
		  module->name, eval_ctx->module->name);
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    retval->defined = TRUE;
    retval->integer = (int64_t) modconf->buffer_size;
}



void nx_expr_func__pm_buffer_buffer_count(nx_expr_eval_ctx_t *eval_ctx,
					  nx_module_t *module,
					  nx_value_t *retval,
					  int32_t num_arg,
					  nx_value_t *args UNUSED)
{
    nx_pm_buffer_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    modconf = (nx_pm_buffer_conf_t *) module->config;

    if ( module != eval_ctx->module )
    {
	throw_msg("private function %s->buffer_count() called from %s",
		  module->name, eval_ctx->module->name);
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    retval->defined = TRUE;
    retval->integer = (int64_t) nx_logqueue_size(modconf->queue);
}
