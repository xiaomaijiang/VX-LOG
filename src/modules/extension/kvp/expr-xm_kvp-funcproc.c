/* Automatically generated from xm_kvp-api.xml by codegen.pl */
#include "expr-xm_kvp-funcproc.h"


/* FUNCTIONS */

// to_kvp

nx_expr_func_t nx_api_declarations_xm_kvp_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_kvp",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_func__to_kvp,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
};


/* PROCEDURES */

// parse_kvp
// parse_kvp
const char *nx_expr_proc__parse_kvp_string_argnames[] = {
    "source", 
};
nx_value_type_t nx_expr_proc__parse_kvp_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// to_kvp
// reset_kvp

nx_expr_proc_t nx_api_declarations_xm_kvp_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_kvp",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__parse_kvp,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_kvp",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__parse_kvp,
   1,
   nx_expr_proc__parse_kvp_string_argnames,
   nx_expr_proc__parse_kvp_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_kvp",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__to_kvp,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "reset_kvp",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__reset_kvp,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_xm_kvp = {
	1,
	nx_api_declarations_xm_kvp_funcs,
	4,
	nx_api_declarations_xm_kvp_procs,
};
