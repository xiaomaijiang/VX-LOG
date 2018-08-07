/* Automatically generated from xm_json-api.xml by codegen.pl */
#include "expr-xm_json-funcproc.h"


/* FUNCTIONS */

// to_json

nx_expr_func_t nx_api_declarations_xm_json_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_json",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__to_json,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
};


/* PROCEDURES */

// parse_json
// parse_json
const char *nx_expr_proc__parse_json_string_argnames[] = {
    "source", 
};
nx_value_type_t nx_expr_proc__parse_json_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// to_json

nx_expr_proc_t nx_api_declarations_xm_json_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_json",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_json,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_json",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_json,
   1,
   nx_expr_proc__parse_json_string_argnames,
   nx_expr_proc__parse_json_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_json",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__to_json,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_xm_json = {
	1,
	nx_api_declarations_xm_json_funcs,
	3,
	nx_api_declarations_xm_json_procs,
};
