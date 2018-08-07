/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

//#include <apr_lib.h>

#include "om_file.h"
#include "../../../common/module.h"
#include "../../../common/alloc.h"

#define NX_LOGMODULE NX_LOGMODULE_CORE

void nx_expr_proc__om_file_rotate_to(nx_expr_eval_ctx_t *eval_ctx UNUSED,
				     nx_module_t *module,
				     nx_expr_list_t *args)
{
    nx_expr_list_elem_t *arg;
    nx_value_t value;
    nx_om_file_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(args != NULL);
    ASSERT(eval_ctx->module != NULL);

    if ( module != eval_ctx->module )
    {
	throw_msg("private procedure %s->rotate_to() called from %s",
		  module->name, eval_ctx->module->name);
    }

    modconf = (nx_om_file_conf_t *) module->config;

    arg = NX_DLIST_FIRST(args);
    ASSERT(arg->expr != NULL);
    nx_expr_evaluate(eval_ctx, &value, arg->expr);

    if ( value.defined != TRUE )
    {
	throw_msg("filename is undef");
    }
    if ( value.type != NX_VALUE_TYPE_STRING )
    {
	nx_value_kill(&value);
	throw_msg("string type required for 'filename'");
    }

    ASSERT(module->decl != NULL);
    ASSERT(module->decl->start != NULL);
    ASSERT(module->decl->stop != NULL);
   
    module->decl->stop(module);
    nx_module_set_status(module, NX_MODULE_STATUS_STOPPED);
    CHECKERR_MSG(apr_file_rename(modconf->filename, value.string->buf, modconf->fpool),
		 "failed to rotate file '%s' to '%s'", modconf->filename,
		 value.string->buf);
    module->decl->start(module);
    nx_module_set_status(module, NX_MODULE_STATUS_RUNNING);
    log_info("om_file successfully rotated file '%s' to '%s'", modconf->filename,
	     value.string->buf);
}



void nx_expr_proc__om_file_reopen(nx_expr_eval_ctx_t *eval_ctx UNUSED,
				  nx_module_t *module,
				  nx_expr_list_t *args)
{
    nx_om_file_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(args == NULL);
    ASSERT(eval_ctx->module != NULL);

    if ( module != eval_ctx->module )
    {
	throw_msg("private procedure %s->rotate_to() called from %s",
		  module->name, eval_ctx->module->name);
    }

    modconf = (nx_om_file_conf_t *) module->config;

    om_file_close(module);
    om_file_open(module);
}



void nx_expr_func__om_file_file_name(nx_expr_eval_ctx_t *eval_ctx,
				     nx_module_t *module,
				     nx_value_t *retval,
				     int32_t num_arg,
				     nx_value_t *args UNUSED)
{
    nx_om_file_conf_t *modconf;

    ASSERT(module != NULL);
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    modconf = (nx_om_file_conf_t *) module->config;

    if ( module != eval_ctx->module )
    {
	throw_msg("private function %s->file_name() called from %s",
		  module->name, eval_ctx->module->name);
    }

    retval->type = NX_VALUE_TYPE_STRING;
    retval->string = nx_string_create(modconf->filename, -1);
    retval->defined = TRUE;
}



void nx_expr_func__om_file_file_size(nx_expr_eval_ctx_t *eval_ctx,
				     nx_module_t *module,
				     nx_value_t *retval,
				     int32_t num_arg,
				     nx_value_t *args UNUSED)
{
    nx_om_file_conf_t *modconf;
    apr_pool_t *pool;
    nx_exception_t e;
    apr_finfo_t finfo;

    ASSERT(module != NULL);
    ASSERT(retval != NULL);
    ASSERT(num_arg == 0);

    modconf = (nx_om_file_conf_t *) module->config;

    if ( module != eval_ctx->module )
    {
	throw_msg("private function %s->file_size() called from %s",
		  module->name, eval_ctx->module->name);
    }

    retval->type = NX_VALUE_TYPE_INTEGER;
    retval->integer = 0;

    if ( modconf->file == NULL )
    {
	retval->defined = FALSE;
	return;
    }
    retval->defined = TRUE;

    pool = nx_pool_create_child(module->pool);
    try
    {
	CHECKERR_MSG(apr_stat(&finfo, modconf->filename, APR_FINFO_SIZE, pool),
		     "failed to query file size information for %s", modconf->filename);
    }
    catch(e)
    {
	apr_pool_destroy(pool);
	rethrow(e);
    }

    retval->integer = finfo.size;
    apr_pool_destroy(pool);
}
