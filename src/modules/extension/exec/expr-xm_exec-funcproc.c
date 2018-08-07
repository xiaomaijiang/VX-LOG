/* Automatically generated from xm_exec-api.xml by codegen.pl */
#include "expr-xm_exec-funcproc.h"


/* PROCEDURES */

// exec
const char *nx_expr_proc__exec_string_varargs_argnames[] = {
    "command", "args", 
};
nx_value_type_t nx_expr_proc__exec_string_varargs_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_EXPR_VALUE_TYPE_VARARGS, 
};
// exec_async
const char *nx_expr_proc__exec_async_string_varargs_argnames[] = {
    "command", "args", 
};
nx_value_type_t nx_expr_proc__exec_async_string_varargs_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_EXPR_VALUE_TYPE_VARARGS, 
};

nx_expr_proc_t nx_api_declarations_xm_exec_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "exec",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__exec,
   2,
   nx_expr_proc__exec_string_varargs_argnames,
   nx_expr_proc__exec_string_varargs_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "exec_async",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__exec_async,
   2,
   nx_expr_proc__exec_async_string_varargs_argnames,
   nx_expr_proc__exec_async_string_varargs_argtypes,
 },
};

nx_module_exports_t nx_module_exports_xm_exec = {
	0,
	NULL,
	2,
	nx_api_declarations_xm_exec_procs,
};
