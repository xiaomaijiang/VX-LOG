/* Automatically generated from xm_fileop-api.xml by codegen.pl */
#include "expr-xm_fileop-funcproc.h"


/* FUNCTIONS */

// file_read
const char *nx_expr_func__xm_fileop_file_read_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_read_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_exists
const char *nx_expr_func__xm_fileop_file_exists_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_exists_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_basename
const char *nx_expr_func__xm_fileop_file_basename_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_basename_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_dirname
const char *nx_expr_func__xm_fileop_file_dirname_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_dirname_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_mtime
const char *nx_expr_func__xm_fileop_file_mtime_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_mtime_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_ctime
const char *nx_expr_func__xm_fileop_file_ctime_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_ctime_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_type
const char *nx_expr_func__xm_fileop_file_type_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_type_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_size
const char *nx_expr_func__xm_fileop_file_size_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_size_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_inode
const char *nx_expr_func__xm_fileop_file_inode_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_func__xm_fileop_file_inode_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// dir_temp_get
// dir_exists
const char *nx_expr_func__xm_fileop_dir_exists_string_argnames[] = {
    "path", 
};
nx_value_type_t nx_expr_func__xm_fileop_dir_exists_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};

nx_expr_func_t nx_api_declarations_xm_fileop_funcs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_read",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_read,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__xm_fileop_file_read_string_argnames,
   nx_expr_func__xm_fileop_file_read_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_exists",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_exists,
   NX_VALUE_TYPE_BOOLEAN,
   1,
   nx_expr_func__xm_fileop_file_exists_string_argnames,
   nx_expr_func__xm_fileop_file_exists_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_basename",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_basename,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__xm_fileop_file_basename_string_argnames,
   nx_expr_func__xm_fileop_file_basename_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_dirname",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_dirname,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__xm_fileop_file_dirname_string_argnames,
   nx_expr_func__xm_fileop_file_dirname_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_mtime",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_mtime,
   NX_VALUE_TYPE_DATETIME,
   1,
   nx_expr_func__xm_fileop_file_mtime_string_argnames,
   nx_expr_func__xm_fileop_file_mtime_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_ctime",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_ctime,
   NX_VALUE_TYPE_DATETIME,
   1,
   nx_expr_func__xm_fileop_file_ctime_string_argnames,
   nx_expr_func__xm_fileop_file_ctime_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_type",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_type,
   NX_VALUE_TYPE_STRING,
   1,
   nx_expr_func__xm_fileop_file_type_string_argnames,
   nx_expr_func__xm_fileop_file_type_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_size",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_size,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__xm_fileop_file_size_string_argnames,
   nx_expr_func__xm_fileop_file_size_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_inode",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_file_inode,
   NX_VALUE_TYPE_INTEGER,
   1,
   nx_expr_func__xm_fileop_file_inode_string_argnames,
   nx_expr_func__xm_fileop_file_inode_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "dir_temp_get",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_dir_temp_get,
   NX_VALUE_TYPE_STRING,
   0,
   NULL,
   NULL,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "dir_exists",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_func__xm_fileop_dir_exists,
   NX_VALUE_TYPE_BOOLEAN,
   1,
   nx_expr_func__xm_fileop_dir_exists_string_argnames,
   nx_expr_func__xm_fileop_dir_exists_string_argtypes,
 },
};


/* PROCEDURES */

// file_cycle
const char *nx_expr_proc__xm_fileop_file_cycle_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_cycle_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_cycle
const char *nx_expr_proc__xm_fileop_file_cycle_string_integer_argnames[] = {
    "file", "max", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_cycle_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// file_rename
const char *nx_expr_proc__xm_fileop_file_rename_string_string_argnames[] = {
    "old", "new", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_rename_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// file_copy
const char *nx_expr_proc__xm_fileop_file_copy_string_string_argnames[] = {
    "src", "dst", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_copy_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// file_remove
const char *nx_expr_proc__xm_fileop_file_remove_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_remove_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_remove
const char *nx_expr_proc__xm_fileop_file_remove_string_datetime_argnames[] = {
    "file", "older", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_remove_string_datetime_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_DATETIME, 
};
// file_link
const char *nx_expr_proc__xm_fileop_file_link_string_string_argnames[] = {
    "src", "dst", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_link_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// file_append
const char *nx_expr_proc__xm_fileop_file_append_string_string_argnames[] = {
    "src", "dst", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_append_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// file_write
const char *nx_expr_proc__xm_fileop_file_write_string_string_argnames[] = {
    "file", "value", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_write_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// file_truncate
const char *nx_expr_proc__xm_fileop_file_truncate_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_truncate_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// file_truncate
const char *nx_expr_proc__xm_fileop_file_truncate_string_integer_argnames[] = {
    "file", "offset", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_truncate_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// file_chown
const char *nx_expr_proc__xm_fileop_file_chown_string_integer_integer_argnames[] = {
    "file", "uid", "gid", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_chown_string_integer_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, NX_VALUE_TYPE_INTEGER, 
};
// file_chown
const char *nx_expr_proc__xm_fileop_file_chown_usr_grp_string_string_string_argnames[] = {
    "file", "user", "group", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_chown_usr_grp_string_string_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_STRING, 
};
// file_chmod
const char *nx_expr_proc__xm_fileop_file_chmod_string_integer_argnames[] = {
    "file", "mode", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_chmod_string_integer_argtypes[] = {
    NX_VALUE_TYPE_STRING, NX_VALUE_TYPE_INTEGER, 
};
// file_touch
const char *nx_expr_proc__xm_fileop_file_touch_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_proc__xm_fileop_file_touch_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// dir_make
const char *nx_expr_proc__xm_fileop_dir_make_string_argnames[] = {
    "path", 
};
nx_value_type_t nx_expr_proc__xm_fileop_dir_make_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};
// dir_remove
const char *nx_expr_proc__xm_fileop_dir_remove_string_argnames[] = {
    "file", 
};
nx_value_type_t nx_expr_proc__xm_fileop_dir_remove_string_argtypes[] = {
    NX_VALUE_TYPE_STRING, 
};

nx_expr_proc_t nx_api_declarations_xm_fileop_procs[] = {
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_cycle",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_cycle,
   1,
   nx_expr_proc__xm_fileop_file_cycle_string_argnames,
   nx_expr_proc__xm_fileop_file_cycle_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_cycle",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_cycle,
   2,
   nx_expr_proc__xm_fileop_file_cycle_string_integer_argnames,
   nx_expr_proc__xm_fileop_file_cycle_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_rename",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_rename,
   2,
   nx_expr_proc__xm_fileop_file_rename_string_string_argnames,
   nx_expr_proc__xm_fileop_file_rename_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_copy",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_copy,
   2,
   nx_expr_proc__xm_fileop_file_copy_string_string_argnames,
   nx_expr_proc__xm_fileop_file_copy_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_remove",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_remove,
   1,
   nx_expr_proc__xm_fileop_file_remove_string_argnames,
   nx_expr_proc__xm_fileop_file_remove_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_remove",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_remove,
   2,
   nx_expr_proc__xm_fileop_file_remove_string_datetime_argnames,
   nx_expr_proc__xm_fileop_file_remove_string_datetime_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_link",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_link,
   2,
   nx_expr_proc__xm_fileop_file_link_string_string_argnames,
   nx_expr_proc__xm_fileop_file_link_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_append",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_append,
   2,
   nx_expr_proc__xm_fileop_file_append_string_string_argnames,
   nx_expr_proc__xm_fileop_file_append_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_write",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_write,
   2,
   nx_expr_proc__xm_fileop_file_write_string_string_argnames,
   nx_expr_proc__xm_fileop_file_write_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_truncate",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_truncate,
   1,
   nx_expr_proc__xm_fileop_file_truncate_string_argnames,
   nx_expr_proc__xm_fileop_file_truncate_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_truncate",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_truncate,
   2,
   nx_expr_proc__xm_fileop_file_truncate_string_integer_argnames,
   nx_expr_proc__xm_fileop_file_truncate_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_chown",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_chown,
   3,
   nx_expr_proc__xm_fileop_file_chown_string_integer_integer_argnames,
   nx_expr_proc__xm_fileop_file_chown_string_integer_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_chown",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_chown_usr_grp,
   3,
   nx_expr_proc__xm_fileop_file_chown_usr_grp_string_string_string_argnames,
   nx_expr_proc__xm_fileop_file_chown_usr_grp_string_string_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_chmod",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_chmod,
   2,
   nx_expr_proc__xm_fileop_file_chmod_string_integer_argnames,
   nx_expr_proc__xm_fileop_file_chmod_string_integer_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "file_touch",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_file_touch,
   1,
   nx_expr_proc__xm_fileop_file_touch_string_argnames,
   nx_expr_proc__xm_fileop_file_touch_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "dir_make",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_dir_make,
   1,
   nx_expr_proc__xm_fileop_dir_make_string_argnames,
   nx_expr_proc__xm_fileop_dir_make_string_argtypes,
 },
 {
   { .next = NULL, .prev = NULL },
   NULL,
   "dir_remove",
   NX_EXPR_FUNCPROC_TYPE_GLOBAL,
   nx_expr_proc__xm_fileop_dir_remove,
   1,
   nx_expr_proc__xm_fileop_dir_remove_string_argnames,
   nx_expr_proc__xm_fileop_dir_remove_string_argtypes,
 },
};

nx_module_exports_t nx_module_exports_xm_fileop = {
	11,
	nx_api_declarations_xm_fileop_funcs,
	17,
	nx_api_declarations_xm_fileop_procs,
};
