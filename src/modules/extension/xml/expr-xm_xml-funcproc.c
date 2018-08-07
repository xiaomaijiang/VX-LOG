/* Automatically generated from xm_xml-api.xml by codegen.pl */
#include "expr-xm_xml-funcproc.h"


/* FUNCTIONS */

// to_xml

nx_expr_func_t nx_api_declarations_xm_xml_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_xml",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__to_xml,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
};


/* PROCEDURES */

// parse_xml
// parse_xml
const char *nx_expr_proc__parse_xml_string_argnames[] = {
    "source", 
};
nx_value_type_t nx_expr_proc__parse_xml_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// to_xml

nx_expr_proc_t nx_api_declarations_xm_xml_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_xml",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_xml,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_xml",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_xml,
   1,
   nx_expr_proc__parse_xml_string_argnames,
   nx_expr_proc__parse_xml_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_xml",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__to_xml,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_xm_xml = {
	1,
	nx_api_declarations_xm_xml_funcs,
	3,
	nx_api_declarations_xm_xml_procs,
};
