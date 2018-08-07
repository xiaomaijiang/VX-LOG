/* Automatically generated from xm_syslog-api.xml by codegen.pl */
#include "expr-xm_syslog-funcproc.h"


/* FUNCTIONS */

// syslog_facility_value
const char *nx_expr_func__syslog_facility_value_string_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__syslog_facility_value_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// syslog_facility_string
const char *nx_expr_func__syslog_facility_string_integer_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__syslog_facility_string_integer_argtypes[] = {
    NX_VALUE_TYPE_INTEGER, 
};
// syslog_severity_value
const char *nx_expr_func__syslog_severity_value_string_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__syslog_severity_value_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// syslog_severity_string
const char *nx_expr_func__syslog_severity_string_integer_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__syslog_severity_string_integer_argtypes[] = {
    NX_VALUE_TYPE_INTEGER, 
};

nx_expr_func_t nx_api_declarations_xm_syslog_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "syslog_facility_value",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__syslog_facility_value,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__syslog_facility_value_string_argnames,
   nx_expr_func__syslog_facility_value_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "syslog_facility_string",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__syslog_facility_string,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__syslog_facility_string_integer_argnames,
   nx_expr_func__syslog_facility_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "syslog_severity_value",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__syslog_severity_value,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__syslog_severity_value_string_argnames,
   nx_expr_func__syslog_severity_value_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "syslog_severity_string",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__syslog_severity_string,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__syslog_severity_string_integer_argnames,
   nx_expr_func__syslog_severity_string_integer_argtypes,
 },
};


/* PROCEDURES */

// parse_syslog
// parse_syslog
const char *nx_expr_proc__parse_syslog_string_argnames[] = {
    "source", 
};
nx_value_type_t nx_expr_proc__parse_syslog_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// parse_syslog_bsd
// parse_syslog_bsd
const char *nx_expr_proc__parse_syslog_bsd_string_argnames[] = {
    "source", 
};
nx_value_type_t nx_expr_proc__parse_syslog_bsd_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// parse_syslog_ietf
// parse_syslog_ietf
const char *nx_expr_proc__parse_syslog_ietf_string_argnames[] = {
    "source", 
};
nx_value_type_t nx_expr_proc__parse_syslog_ietf_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// to_syslog_bsd
// to_syslog_ietf
// to_syslog_snare

nx_expr_proc_t nx_api_declarations_xm_syslog_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_syslog",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_syslog,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_syslog",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_syslog,
   1,
   nx_expr_proc__parse_syslog_string_argnames,
   nx_expr_proc__parse_syslog_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_syslog_bsd",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_syslog_bsd,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_syslog_bsd",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_syslog_bsd,
   1,
   nx_expr_proc__parse_syslog_bsd_string_argnames,
   nx_expr_proc__parse_syslog_bsd_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_syslog_ietf",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_syslog_ietf,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parse_syslog_ietf",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__parse_syslog_ietf,
   1,
   nx_expr_proc__parse_syslog_ietf_string_argnames,
   nx_expr_proc__parse_syslog_ietf_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_syslog_bsd",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__to_syslog_bsd,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_syslog_ietf",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__to_syslog_ietf,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "to_syslog_snare",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__to_syslog_snare,
   0,
   NULL,
   NULL,
 },
};

nx_module_exports_t nx_module_exports_xm_syslog = {
	4,
	nx_api_declarations_xm_syslog_funcs,
	9,
	nx_api_declarations_xm_syslog_procs,
};
