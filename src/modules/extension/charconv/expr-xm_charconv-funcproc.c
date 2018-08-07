/* Automatically generated from xm_charconv-api.xml by codegen.pl */
#include "expr-xm_charconv-funcproc.h"


/* FUNCTIONS */

// convert
const char *nx_expr_func__convert_string_string_string_argnames[] = {
    "source", "srcencoding", "dstencoding", 
};
nx_value_type_t nx_expr_func__convert_string_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};

nx_expr_func_t nx_api_declarations_xm_charconv_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "convert",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__convert,
   NX_VALUE_TYPE_STRING,
   3,
   nx_expr_func__convert_string_string_string_argnames,
   nx_expr_func__convert_string_string_string_argtypes,
 },
};


/* PROCEDURES */

// convert_fields
const char *nx_expr_proc__convert_fields_string_string_argnames[] = {
    "srcencoding", "dstencoding", 
};
nx_value_type_t nx_expr_proc__convert_fields_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};

nx_expr_proc_t nx_api_declarations_xm_charconv_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "convert_fields",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__convert_fields,
   2,
   nx_expr_proc__convert_fields_string_string_argnames,
   nx_expr_proc__convert_fields_string_string_argtypes,
 },
};

nx_module_exports_t nx_module_exports_xm_charconv = {
	1,
	nx_api_declarations_xm_charconv_funcs,
	1,
	nx_api_declarations_xm_charconv_procs,
};
