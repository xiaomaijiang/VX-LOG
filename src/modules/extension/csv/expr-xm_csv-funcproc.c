/* Automatically generated from xm_csv-api.xml by codegen.pl */
#include "expr-xm_csv-funcproc.h"


/* FUNCTIONS */

// to_csv

nx_expr_func_t nx_api_declarations_xm_csv_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_csv",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_func__to_csv,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
};


/* PROCEDURES */

// parse_csv
// parse_csv
const char *nx_expr_proc__parse_csv_string_argnames[] = {
    "source", 
};
nx_value_type_t nx_expr_proc__parse_csv_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// to_csv

nx_expr_proc_t nx_api_declarations_xm_csv_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_csv",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__parse_csv,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_csv",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__parse_csv,
   1,
   nx_expr_proc__parse_csv_string_argnames,
   nx_expr_proc__parse_csv_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_csv",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__to_csv,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_xm_csv = {
	1,
	nx_api_declarations_xm_csv_funcs,
	3,
	nx_api_declarations_xm_csv_procs,
};
