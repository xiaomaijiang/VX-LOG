/* Automatically generated from pm_blocker-api.xml by codegen.pl */
#ifndef __NX_EXPR_FUNCPROC_pm_blocker_H
#define __NX_EXPR_FUNCPROC_pm_blocker_H

#include "../../../common/expr.h"
#include "../../../common/module.h"


/* FUNCTIONS */

#define nx_api_declarations_pm_blocker_func_num 1
void nx_expr_func__pm_blocker_is_blocking(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);

/* PROCEDURES */

#define nx_api_declarations_pm_blocker_proc_num 1
void nx_expr_proc__pm_blocker_block(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);


#endif /* __NX_EXPR_FUNCPROC_pm_blocker_H */
