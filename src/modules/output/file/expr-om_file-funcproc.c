/* Automatically generated from om_file-api.xml by codegen.pl */
#include "expr-om_file-funcproc.h"


/* FUNCTIONS */

// file_name
// file_size

nx_expr_func_t nx_api_declarations_om_file_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_name",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_func__om_file_file_name,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_size",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_func__om_file_file_size,
   NX_VALUE_TYPE_INTEGER,
   0,
   NULL,
   NULL,
 },
};


/* PROCEDURES */

// rotate_to
const char *nx_expr_proc__om_file_rotate_to_string_argnames[] = {
    "filename", 
};
nx_value_type_t nx_expr_proc__om_file_rotate_to_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// reopen

nx_expr_proc_t nx_api_declarations_om_file_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "rotate_to",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_proc__om_file_rotate_to,
   1,
   nx_expr_proc__om_file_rotate_to_string_argnames,
   nx_expr_proc__om_file_rotate_to_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "reopen",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_proc__om_file_reopen,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_om_file = {
	2,
	nx_api_declarations_om_file_funcs,
	2,
	nx_api_declarations_om_file_procs,
};
