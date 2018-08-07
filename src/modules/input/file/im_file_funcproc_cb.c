/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

//#include <apr_lib.h>

#include "im_file.h"
#include "../../../common/module.h"
#include "../../../common/alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE


void nx_expr_func__im_file_file_name(nx_expr_eval_ctx_t *eval_ctx,
				     nx_module_t *module,
				     nx_value_t *retval,
				     int32_t num_arg,
				     nx_value_t *args UNUSED)
{
    const char *filename;

    ASSERT(module != NULL);
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    if ( module != eval_ctx->module )
    {
	throw_msg("private function %s->file_name() called from %s",
		  module->name, eval_ctx->module->name);
    }
    if ( eval_ctx->input == NULL )
    {
	log_error("no input available for file_name() in %s",
		  eval_ctx->module->name);
    }

    filename = nx_module_input_name_get(eval_ctx->input);

    retval->type = NX_VALUE_TYPE_STRING;
    if ( filename != NULL )
    {
	retval->string = nx_string_create(filename, -1);
	retval->defined = TRUE;
    }
    else
    {
	retval->defined = FALSE;
    }
}



