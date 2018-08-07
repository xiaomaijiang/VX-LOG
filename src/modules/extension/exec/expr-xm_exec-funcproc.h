/* Automatically generated from xm_exec-api.xml by codegen.pl */
#ifndef __NX_EXPR_FUNCPROC_xm_exec_H
#define __NX_EXPR_FUNCPROC_xm_exec_H

#include "../../../common/expr.h"
#include "../../../common/module.h"

#define nx_api_declarations_xm_exec_func_num 0

/* PROCEDURES */

#define nx_api_declarations_xm_exec_proc_num 2
void nx_expr_proc__exec(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__exec_async(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);


#endif /* __NX_EXPR_FUNCPROC_xm_exec_H */
