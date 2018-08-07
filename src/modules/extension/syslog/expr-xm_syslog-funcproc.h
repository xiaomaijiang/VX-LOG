/* Automatically generated from xm_syslog-api.xml by codegen.pl */
#ifndef __NX_EXPR_FUNCPROC_xm_syslog_H
#define __NX_EXPR_FUNCPROC_xm_syslog_H

#include "../../../common/expr.h"
#include "../../../common/module.h"


/* FUNCTIONS */

#define nx_api_declarations_xm_syslog_func_num 4
void nx_expr_func__syslog_facility_value(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);
void nx_expr_func__syslog_facility_string(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);
void nx_expr_func__syslog_severity_value(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);
void nx_expr_func__syslog_severity_string(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_value_t *retval, int32_t num_arg, nx_value_t *args);

/* PROCEDURES */

#define nx_api_declarations_xm_syslog_proc_num 9
void nx_expr_proc__parse_syslog(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__parse_syslog_bsd(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__parse_syslog_ietf(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__to_syslog_bsd(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__to_syslog_ietf(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);
void nx_expr_proc__to_syslog_snare(nx_expr_eval_ctx_t *eval_ctx, nx_module_t *module, nx_expr_list_t *args);


#endif /* __NX_EXPR_FUNCPROC_xm_syslog_H */
