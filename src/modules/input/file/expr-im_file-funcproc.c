/* Automatically generated from im_file-api.xml by codegen.pl */
#include "expr-im_file-funcproc.h"


/* FUNCTIONS */

// file_name

nx_expr_func_t nx_api_declarations_im_file_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_name",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_func__im_file_file_name,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_im_file = {
	1,
	nx_api_declarations_im_file_funcs,
	0,
	NULL,
};
