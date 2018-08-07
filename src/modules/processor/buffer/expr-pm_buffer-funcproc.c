/* Automatically generated from pm_buffer-api.xml by codegen.pl */
#include "expr-pm_buffer-funcproc.h"


/* FUNCTIONS */

// buffer_size
// buffer_count

nx_expr_func_t nx_api_declarations_pm_buffer_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "buffer_size",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_func__pm_buffer_buffer_size,
   NX_VALUE_TYPE_INTEGER,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "buffer_count",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_func__pm_buffer_buffer_count,
   NX_VALUE_TYPE_INTEGER,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_pm_buffer = {
	2,
	nx_api_declarations_pm_buffer_funcs,
	0,
	NULL,
};
