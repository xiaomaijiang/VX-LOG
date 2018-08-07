/* Automatically generated from om_http-api.xml by codegen.pl */
#include "expr-om_http-funcproc.h"


/* PROCEDURES */

// set_http_request_path
const char *nx_expr_proc__om_http_set_http_request_path_string_argnames[] = {
    "path", 
};
nx_value_type_t nx_expr_proc__om_http_set_http_request_path_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};

nx_expr_proc_t nx_api_declarations_om_http_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "set_http_request_path",
   NX_EXPR_FUNCPROC_TYPE_PRIVATE,
   nx_expr_proc__om_http_set_http_request_path,
   1,
   nx_expr_proc__om_http_set_http_request_path_string_argnames,
   nx_expr_proc__om_http_set_http_request_path_string_argtypes,
 },
};

nx_module_exports_t nx_module_exports_om_http = {
	0,
	NULL,
	1,
	nx_api_declarations_om_http_procs,
};
