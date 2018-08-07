/* Automatically generated from xm_csv-api.xml by codegen.pl */
#ifndef __NX_EXPR_FUNCPROC_xm_csv_H
#define __NX_EXPR_FUNCPROC_xm_csv_H

#include "../../../common/expr.h"
#include "../../../common/module.h"


/* FUNCTIONS */

#define nx_api_declarations_xm_csv_func_num 1
void nx_expr_func__to_csv(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);

/* PROCEDURES */

#define nx_api_declarations_xm_csv_proc_num 3
void nx_expr_proc__parse_csv(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__to_csv(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);


#endif /* __NX_EXPR_FUNCPROC_xm_csv_H */
