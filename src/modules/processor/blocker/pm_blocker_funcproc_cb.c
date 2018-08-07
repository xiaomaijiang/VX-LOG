/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#include "pm_blocker.h"
#include "../../../common/module.h"
#include "../../../common/alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_MODULE


void nx_expr_proc__pm_blocker_block(nx_expr_eval_ctx_t *eval_ctx,
				    nx_module_t *module,
				    nx_expr_list_t *args)
{
    nx_pm_blocker_conf_t *modconf;
    nx_expr_list_elem_t *mode;
    nx_value_t blockmode;

    ASSERT(module != NULL);

    modconf = (nx_pm_blocker_conf_t *) module->config;

    ASSERT(args != NULL);
    mode = NX_DLIST_FIRST(args);

    nx_expr_evaluate(eval_ctx, &blockmode, mode->expr);

    if ( blockmode.defined != TRUE )
    {
	throw_msg("'mode' is undef");
    }
    if ( blockmode.type != NX_VALUE_TYPE_BOOLEAN )
    {
	nx_value_kill(&blockmode);
	throw_msg("boolean type required for 'mode'");
    }

    modconf->block = blockmode.boolean;
    if ( blockmode.boolean == FALSE )
    {
	nx_module_data_available(module);
    }
}



void nx_expr_func__pm_blocker_is_blocking(nx_expr_eval_ctx_t *eval_ctx UNUSED,
					  nx_module_t *module,
					  nx_value_t *retval,
					  int32_t num_arg,
					  nx_value_t *args UNUSED)
{
    nx_pm_blocker_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    modconf = (nx_pm_blocker_conf_t *) module->config;

    retval->type = NX_VALUE_TYPE_BOOLEAN;
    retval->boolean = modconf->block;
    retval->defined = TRUE;
}


