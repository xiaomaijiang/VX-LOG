/* Automatically generated from xm_kvp-api.xml by codegen.pl */
#ifndef __NX_EXPR_FUNCPROC_xm_kvp_H
#define __NX_EXPR_FUNCPROC_xm_kvp_H

#include "../../../common/expr.h"
#include "../../../common/module.h"


/* FUNCTIONS */

#define nx_api_declarations_xm_kvp_func_num 1
void nx_expr_func__to_kvp(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);

/* PROCEDURES */

#define nx_api_declarations_xm_kvp_proc_num 4
void nx_expr_proc__parse_kvp(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__to_kvp(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__reset_kvp(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);


#endif /* __NX_EXPR_FUNCPROC_xm_kvp_H */
