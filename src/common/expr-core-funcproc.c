/* Automatically generated from core-api.xml by codegen.pl */
#include "expr-core-funcproc.h"


/* FUNCTIONS */

// lc
const char *nx_expr_func__lc_string_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__lc_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// uc
const char *nx_expr_func__uc_string_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__uc_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// now
// type
const char *nx_expr_func__type_unknown_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__type_unknown_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, 
};
// microsecond
const char *nx_expr_func__microsecond_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__microsecond_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// second
const char *nx_expr_func__second_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__second_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// minute
const char *nx_expr_func__minute_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__minute_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// hour
const char *nx_expr_func__hour_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__hour_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// day
const char *nx_expr_func__day_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__day_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// month
const char *nx_expr_func__month_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__month_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// year
const char *nx_expr_func__year_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__year_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// fix_year
const char *nx_expr_func__fix_year_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__fix_year_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// dayofweek
const char *nx_expr_func__dayofweek_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__dayofweek_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// dayofyear
const char *nx_expr_func__dayofyear_datetime_argnames[] = {
    "datetime", 
};
nx_value_type_t nx_expr_func__dayofyear_datetime_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, 
};
// string
const char *nx_expr_func__to_string_unknown_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__to_string_unknown_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, 
};
// integer
const char *nx_expr_func__to_integer_unknown_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__to_integer_unknown_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, 
};
// datetime
const char *nx_expr_func__to_datetime_integer_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__to_datetime_integer_argtypes[] = {
    NX_VALUE_TYPE_INTEGER, 
};
// parsedate
const char *nx_expr_func__parsedate_string_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__parsedate_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// strftime
const char *nx_expr_func__strftime_datetime_string_argnames[] = {
    "datetime", "fmt", 
};
nx_value_type_t nx_expr_func__strftime_datetime_string_argtypes[] = {
    NX_VALUE_TYPE_DATETIME, NX_VALUE_TYPE_STRING, 
};
// strptime
const char *nx_expr_func__strptime_string_string_argnames[] = {
    "input", "fmt", 
};
nx_value_type_t nx_expr_func__strptime_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// hostname
// hostname_fqdn
// host_ip
// host_ip
const char *nx_expr_func__host_ip_integer_argnames[] = {
    "nth", 
};
nx_value_type_t nx_expr_func__host_ip_integer_argtypes[] = {
    NX_VALUE_TYPE_INTEGER, 
};
// get_var
const char *nx_expr_func__get_var_string_argnames[] = {
    "varname", 
};
nx_value_type_t nx_expr_func__get_var_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// get_stat
const char *nx_expr_func__get_stat_string_argnames[] = {
    "statname", 
};
nx_value_type_t nx_expr_func__get_stat_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// get_stat
const char *nx_expr_func__get_stat_string_datetime_argnames[] = {
    "statname", "time", 
};
nx_value_type_t nx_expr_func__get_stat_string_datetime_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_DATETIME, 
};
// ip4addr
const char *nx_expr_func__to_ip4addr_integer_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_func__to_ip4addr_integer_argtypes[] = {
    NX_VALUE_TYPE_INTEGER, 
};
// ip4addr
const char *nx_expr_func__to_ip4addr_integer_boolean_argnames[] = {
    "arg", "ntoa", 
};
nx_value_type_t nx_expr_func__to_ip4addr_integer_boolean_argtypes[] = {
    NX_VALUE_TYPE_INTEGER, NX_VALUE_TYPE_BOOLEAN, 
};
// substr
const char *nx_expr_func__substr_string_integer_argnames[] = {
    "src", "from", 
};
nx_value_type_t nx_expr_func__substr_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// substr
const char *nx_expr_func__substr_string_integer_integer_argnames[] = {
    "src", "from", "to", 
};
nx_value_type_t nx_expr_func__substr_string_integer_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, NX_VALUE_TYPE_INTEGER, 
};
// replace
const char *nx_expr_func__replace_string_string_string_argnames[] = {
    "subject", "src", "dst", 
};
nx_value_type_t nx_expr_func__replace_string_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// replace
const char *nx_expr_func__replace_string_string_string_integer_argnames[] = {
    "subject", "src", "dst", "count", 
};
nx_value_type_t nx_expr_func__replace_string_string_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// size
const char *nx_expr_func__str_size_string_argnames[] = {
    "str", 
};
nx_value_type_t nx_expr_func__str_size_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// dropped

nx_expr_func_t nx_api_declarations_core_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "lc",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__lc,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__lc_string_argnames,
   nx_expr_func__lc_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "uc",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__uc,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__uc_string_argnames,
   nx_expr_func__uc_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "now",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__now,
   NX_VALUE_TYPE_DATETIME,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "type",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__type,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__type_unknown_argnames,
   nx_expr_func__type_unknown_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "microsecond",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__microsecond,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__microsecond_datetime_argnames,
   nx_expr_func__microsecond_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "second",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__second,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__second_datetime_argnames,
   nx_expr_func__second_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "minute",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__minute,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__minute_datetime_argnames,
   nx_expr_func__minute_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "hour",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__hour,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__hour_datetime_argnames,
   nx_expr_func__hour_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "day",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__day,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__day_datetime_argnames,
   nx_expr_func__day_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "month",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__month,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__month_datetime_argnames,
   nx_expr_func__month_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "year",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__year,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__year_datetime_argnames,
   nx_expr_func__year_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "fix_year",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__fix_year,
   NX_VALUE_TYPE_DATETIME,
   1,
   nx_expr_func__fix_year_datetime_argnames,
   nx_expr_func__fix_year_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "dayofweek",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__dayofweek,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__dayofweek_datetime_argnames,
   nx_expr_func__dayofweek_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "dayofyear",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__dayofyear,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__dayofyear_datetime_argnames,
   nx_expr_func__dayofyear_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "string",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__to_string,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__to_string_unknown_argnames,
   nx_expr_func__to_string_unknown_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "integer",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__to_integer,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__to_integer_unknown_argnames,
   nx_expr_func__to_integer_unknown_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "datetime",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__to_datetime,
   NX_VALUE_TYPE_DATETIME,
   1,
   nx_expr_func__to_datetime_integer_argnames,
   nx_expr_func__to_datetime_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "parsedate",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__parsedate,
   NX_VALUE_TYPE_DATETIME,
   1,
   nx_expr_func__parsedate_string_argnames,
   nx_expr_func__parsedate_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "strftime",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__strftime,
   NX_VALUE_TYPE_STRING,
   2,
   nx_expr_func__strftime_datetime_string_argnames,
   nx_expr_func__strftime_datetime_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "strptime",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__strptime,
   NX_VALUE_TYPE_DATETIME,
   2,
   nx_expr_func__strptime_string_string_argnames,
   nx_expr_func__strptime_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "hostname",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__hostname,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "hostname_fqdn",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__hostname_fqdn,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "host_ip",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__host_ip,
   NX_VALUE_TYPE_IP4ADDR,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "host_ip",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__host_ip,
   NX_VALUE_TYPE_IP4ADDR,
   1,
   nx_expr_func__host_ip_integer_argnames,
   nx_expr_func__host_ip_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "get_var",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__get_var,
   NX_VALUE_TYPE_UNKNOWN,
   1,
   nx_expr_func__get_var_string_argnames,
   nx_expr_func__get_var_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "get_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__get_stat,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__get_stat_string_argnames,
   nx_expr_func__get_stat_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "get_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__get_stat,
   NX_VALUE_TYPE_INTEGER,
   2,
   nx_expr_func__get_stat_string_datetime_argnames,
   nx_expr_func__get_stat_string_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "ip4addr",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__to_ip4addr,
   NX_VALUE_TYPE_IP4ADDR,
   1,
   nx_expr_func__to_ip4addr_integer_argnames,
   nx_expr_func__to_ip4addr_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "ip4addr",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__to_ip4addr,
   NX_VALUE_TYPE_IP4ADDR,
   2,
   nx_expr_func__to_ip4addr_integer_boolean_argnames,
   nx_expr_func__to_ip4addr_integer_boolean_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "substr",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__substr,
   NX_VALUE_TYPE_STRING,
   2,
   nx_expr_func__substr_string_integer_argnames,
   nx_expr_func__substr_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "substr",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__substr,
   NX_VALUE_TYPE_STRING,
   3,
   nx_expr_func__substr_string_integer_integer_argnames,
   nx_expr_func__substr_string_integer_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "replace",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__replace,
   NX_VALUE_TYPE_STRING,
   3,
   nx_expr_func__replace_string_string_string_argnames,
   nx_expr_func__replace_string_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "replace",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__replace,
   NX_VALUE_TYPE_STRING,
   4,
   nx_expr_func__replace_string_string_string_integer_argnames,
   nx_expr_func__replace_string_string_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "size",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__str_size,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__str_size_string_argnames,
   nx_expr_func__str_size_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "dropped",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__dropped,
   NX_VALUE_TYPE_BOOLEAN,
   0,
   NULL,
   NULL,
 },
};


/* PROCEDURES */

// log_debug
const char *nx_expr_proc__log_debug_unknown_varargs_argnames[] = {
    "arg", "args", 
};
nx_value_type_t nx_expr_proc__log_debug_unknown_varargs_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, NX_EXPR_VALUE_TYPE_VARARGS, 
};
// debug
const char *nx_expr_proc__debug_unknown_varargs_argnames[] = {
    "arg", "args", 
};
nx_value_type_t nx_expr_proc__debug_unknown_varargs_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, NX_EXPR_VALUE_TYPE_VARARGS, 
};
// log_info
const char *nx_expr_proc__log_info_unknown_varargs_argnames[] = {
    "arg", "args", 
};
nx_value_type_t nx_expr_proc__log_info_unknown_varargs_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, NX_EXPR_VALUE_TYPE_VARARGS, 
};
// log_warning
const char *nx_expr_proc__log_warning_unknown_varargs_argnames[] = {
    "arg", "args", 
};
nx_value_type_t nx_expr_proc__log_warning_unknown_varargs_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, NX_EXPR_VALUE_TYPE_VARARGS, 
};
// log_error
const char *nx_expr_proc__log_error_unknown_varargs_argnames[] = {
    "arg", "args", 
};
nx_value_type_t nx_expr_proc__log_error_unknown_varargs_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, NX_EXPR_VALUE_TYPE_VARARGS, 
};
// delete
const char *nx_expr_proc__delete_unknown_argnames[] = {
    "arg", 
};
nx_value_type_t nx_expr_proc__delete_unknown_argtypes[] = {
    NX_VALUE_TYPE_UNKNOWN, 
};
// create_var
const char *nx_expr_proc__create_var_string_argnames[] = {
    "varname", 
};
nx_value_type_t nx_expr_proc__create_var_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// create_var
const char *nx_expr_proc__create_var_string_integer_argnames[] = {
    "varname", "lifetime", 
};
nx_value_type_t nx_expr_proc__create_var_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// create_var
const char *nx_expr_proc__create_var_string_datetime_argnames[] = {
    "varname", "expiry", 
};
nx_value_type_t nx_expr_proc__create_var_string_datetime_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_DATETIME, 
};
// delete_var
const char *nx_expr_proc__delete_var_string_argnames[] = {
    "varname", 
};
nx_value_type_t nx_expr_proc__delete_var_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// set_var
const char *nx_expr_proc__set_var_string_unknown_argnames[] = {
    "varname", "value", 
};
nx_value_type_t nx_expr_proc__set_var_string_unknown_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_UNKNOWN, 
};
// create_stat
const char *nx_expr_proc__create_stat_string_string_argnames[] = {
    "statname", "type", 
};
nx_value_type_t nx_expr_proc__create_stat_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// create_stat
const char *nx_expr_proc__create_stat_string_string_integer_argnames[] = {
    "statname", "type", "interval", 
};
nx_value_type_t nx_expr_proc__create_stat_string_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// create_stat
const char *nx_expr_proc__create_stat_string_string_integer_datetime_argnames[] = {
    "statname", "type", "interval", "time", 
};
nx_value_type_t nx_expr_proc__create_stat_string_string_integer_datetime_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, NX_VALUE_TYPE_DATETIME, 
};
// create_stat
const char *nx_expr_proc__create_stat_string_string_integer_datetime_integer_argnames[] = {
    "statname", "type", "interval", "time", "lifetime", 
};
nx_value_type_t nx_expr_proc__create_stat_string_string_integer_datetime_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, NX_VALUE_TYPE_DATETIME, NX_VALUE_TYPE_INTEGER, 
};
// create_stat
const char *nx_expr_proc__create_stat_string_string_integer_datetime_datetime_argnames[] = {
    "statname", "type", "interval", "time", "expiry", 
};
nx_value_type_t nx_expr_proc__create_stat_string_string_integer_datetime_datetime_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, NX_VALUE_TYPE_DATETIME, NX_VALUE_TYPE_DATETIME, 
};
// add_stat
const char *nx_expr_proc__add_stat_string_integer_argnames[] = {
    "statname", "value", 
};
nx_value_type_t nx_expr_proc__add_stat_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// add_stat
const char *nx_expr_proc__add_stat_string_integer_datetime_argnames[] = {
    "statname", "value", "time", 
};
nx_value_type_t nx_expr_proc__add_stat_string_integer_datetime_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, NX_VALUE_TYPE_DATETIME, 
};
// sleep
const char *nx_expr_proc__sleep_integer_argnames[] = {
    "interval", 
};
nx_value_type_t nx_expr_proc__sleep_integer_argtypes[] = {
    NX_VALUE_TYPE_INTEGER, 
};
// drop
// rename_field
const char *nx_expr_proc__rename_field_string_string_argnames[] = {
    "old", "new", 
};
nx_value_type_t nx_expr_proc__rename_field_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// reroute
const char *nx_expr_proc__reroute_string_argnames[] = {
    "routename", 
};
nx_value_type_t nx_expr_proc__reroute_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// add_to_route
const char *nx_expr_proc__add_to_route_string_argnames[] = {
    "routename", 
};
nx_value_type_t nx_expr_proc__add_to_route_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};

nx_expr_proc_t nx_api_declarations_core_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "log_debug",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__log_debug,
   2,
   nx_expr_proc__log_debug_unknown_varargs_argnames,
   nx_expr_proc__log_debug_unknown_varargs_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "debug",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__debug,
   2,
   nx_expr_proc__debug_unknown_varargs_argnames,
   nx_expr_proc__debug_unknown_varargs_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "log_info",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__log_info,
   2,
   nx_expr_proc__log_info_unknown_varargs_argnames,
   nx_expr_proc__log_info_unknown_varargs_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "log_warning",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__log_warning,
   2,
   nx_expr_proc__log_warning_unknown_varargs_argnames,
   nx_expr_proc__log_warning_unknown_varargs_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "log_error",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__log_error,
   2,
   nx_expr_proc__log_error_unknown_varargs_argnames,
   nx_expr_proc__log_error_unknown_varargs_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "delete",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__delete,
   1,
   nx_expr_proc__delete_unknown_argnames,
   nx_expr_proc__delete_unknown_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_var",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_var,
   1,
   nx_expr_proc__create_var_string_argnames,
   nx_expr_proc__create_var_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_var",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_var,
   2,
   nx_expr_proc__create_var_string_integer_argnames,
   nx_expr_proc__create_var_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_var",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_var,
   2,
   nx_expr_proc__create_var_string_datetime_argnames,
   nx_expr_proc__create_var_string_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "delete_var",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__delete_var,
   1,
   nx_expr_proc__delete_var_string_argnames,
   nx_expr_proc__delete_var_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "set_var",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__set_var,
   2,
   nx_expr_proc__set_var_string_unknown_argnames,
   nx_expr_proc__set_var_string_unknown_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_stat,
   2,
   nx_expr_proc__create_stat_string_string_argnames,
   nx_expr_proc__create_stat_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_stat,
   3,
   nx_expr_proc__create_stat_string_string_integer_argnames,
   nx_expr_proc__create_stat_string_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_stat,
   4,
   nx_expr_proc__create_stat_string_string_integer_datetime_argnames,
   nx_expr_proc__create_stat_string_string_integer_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_stat,
   5,
   nx_expr_proc__create_stat_string_string_integer_datetime_integer_argnames,
   nx_expr_proc__create_stat_string_string_integer_datetime_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "create_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__create_stat,
   5,
   nx_expr_proc__create_stat_string_string_integer_datetime_datetime_argnames,
   nx_expr_proc__create_stat_string_string_integer_datetime_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "add_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__add_stat,
   2,
   nx_expr_proc__add_stat_string_integer_argnames,
   nx_expr_proc__add_stat_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "add_stat",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__add_stat,
   3,
   nx_expr_proc__add_stat_string_integer_datetime_argnames,
   nx_expr_proc__add_stat_string_integer_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "sleep",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__sleep,
   1,
   nx_expr_proc__sleep_integer_argnames,
   nx_expr_proc__sleep_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "drop",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__drop,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "rename_field",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__rename_field,
   2,
   nx_expr_proc__rename_field_string_string_argnames,
   nx_expr_proc__rename_field_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "reroute",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__reroute,
   1,
   nx_expr_proc__reroute_string_argnames,
   nx_expr_proc__reroute_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "add_to_route",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__add_to_route,
   1,
   nx_expr_proc__add_to_route_string_argnames,
   nx_expr_proc__add_to_route_string_argtypes,
 },
};

nx_module_exports_t nx_module_exports_core = {
	35,
	nx_api_declarations_core_funcs,
	23,
	nx_api_declarations_core_procs,
};
