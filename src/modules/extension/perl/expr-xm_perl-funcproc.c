/* Automatically generated from xm_perl-api.xml by codegen.pl */
#include "expr-xm_perl-funcproc.h"


/* PROCEDURES */

// call
const char *nx_expr_proc__xm_perl_call_string_argnames[] = {
    "subroutine", 
};
nx_value_type_t nx_expr_proc__xm_perl_call_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// perl_call
const char *nx_expr_proc__xm_perl_perl_call_string_argnames[] = {
    "subroutine", 
};
nx_value_type_t nx_expr_proc__xm_perl_perl_call_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};

nx_expr_proc_t nx_api_declarations_xm_perl_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "call",
   NX_EXPR_FUNCPROC_TYPE_PUBLIC,
   nx_expr_proc__xm_perl_call,
   1,
   nx_expr_proc__xm_perl_call_string_argnames,
   nx_expr_proc__xm_perl_call_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "perl_call",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_perl_perl_call,
   1,
   nx_expr_proc__xm_perl_perl_call_string_argnames,
   nx_expr_proc__xm_perl_perl_call_string_argtypes,
 },
};

nx_module_exports_t nx_module_exports_xm_perl = {
	0,
	NULL,
	2,
	nx_api_declarations_xm_perl_procs,
};
