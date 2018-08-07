/* Automatically generated from pm_blocker-api.xml by codegen.pl */
#include "expr-pm_blocker-funcproc.h"


/* FUNCTIONS */

// is_blocking

nx_expr_func_t nx_api_declarations_pm_blocker_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "is_blocking",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_func__pm_blocker_is_blocking,
   NX_VALUE_TYPE_BOOLEAN,
   0,
   NULL,
   NULL,
 },
};


/* PROCEDURES */

// block
const char *nx_expr_proc__pm_blocker_block_boolean_argnames[] = {
    "mode", 
};
nx_value_type_t nx_expr_proc__pm_blocker_block_boolean_argtypes[] = {
    NX_VALUE_TYPE_BOOLEAN, 
};

nx_expr_proc_t nx_api_declarations_pm_blocker_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "block",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__pm_blocker_block,
   1,
   nx_expr_proc__pm_blocker_block_boolean_argnames,
   nx_expr_proc__pm_blocker_block_boolean_argtypes,
 },
};

nx_module_exports_t nx_module_exports_pm_blocker = {
	1,
	nx_api_declarations_pm_blocker_funcs,
	1,
	nx_api_declarations_pm_blocker_procs,
};
