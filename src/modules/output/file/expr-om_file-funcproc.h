/* Automatically generated from om_file-api.xml by codegen.pl */
#ifndef __NX_EXPR_FUNCPROC_om_file_H
#define __NX_EXPR_FUNCPROC_om_file_H

#include "../../../common/expr.h"
#include "../../../common/module.h"


/* FUNCTIONS */

#define nx_api_declarations_om_file_func_num 2
void nx_expr_func__om_file_file_name(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);
void nx_expr_func__om_file_file_size(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);

/* PROCEDURES */

#define nx_api_declarations_om_file_proc_num 2
void nx_expr_proc__om_file_rotate_to(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__om_file_reopen(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);


#endif /* __NX_EXPR_FUNCPROC_om_file_H */
