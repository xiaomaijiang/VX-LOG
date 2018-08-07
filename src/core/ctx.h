/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_CTX_H
#define __NX_CTX_H

#include "../common/types.h"
#include "../common/module.h"
#include "../common/error_debug.h"
#include "../common/config_cache.h"
#include "../common/event.h"
#include "../common/resource.h"
#include "../common/expr.h"

typedef struct nx_module_list_t nx_module_list_t;
NX_DLIST_HEAD(nx_module_list_t, nx_module_t);

typedef struct nx_route_list_t nx_route_list_t;
NX_DLIST_HEAD(nx_route_list_t, nx_route_t);

typedef struct nx_event_list_t nx_event_list_t;
NX_DLIST_HEAD(nx_event_list_t, nx_event_t);

typedef struct nx_worker_list_t nx_worker_list_t;
NX_DLIST_HEAD(nx_worker_list_t, nx_event_t);

typedef struct nx_jobgroups_t nx_jobgroups_t;
NX_DLIST_HEAD(nx_jobgroups_t, nx_jobgroup_t);

typedef struct nx_resource_list_t nx_resource_list_t;
NX_DLIST_HEAD(nx_resource_list_t, nx_resource_t);

typedef struct nx_module_input_func_list_t nx_module_input_func_list_t;
NX_DLIST_HEAD(nx_module_input_func_list_t, nx_module_input_func_decl_t);

typedef struct nx_module_output_func_list_t nx_module_output_func_list_t;
NX_DLIST_HEAD(nx_module_output_func_list_t, nx_module_output_func_decl_t);


typedef struct nx_ctx_t
{
    apr_pool_t 		*pool;
    nx_directive_t	*cfgtree; ///< configuration parsed into directives

    char		*moduledir;
    nx_loglevel_t	loglevel;
    nx_panic_level_t	panic_level;
    apr_file_t		*logfile;
    boolean		formatlog;	///< output with timestamp+severity
    boolean		norepeat;
    char		norepeat_buf[NX_LOGBUF_SIZE];
    apr_time_t		norepeat_time;
    int			norepeat_cnt;
    apr_size_t		norepeat_offs;
    nx_loglevel_t	norepeat_loglevel;
    boolean		nofreeonexit; ///< do not free memory on exit: used for debugging with valgrind to allow stack traces with dso modules
    boolean		ignoreerrors; ///< try to ignore configuration errors and start anyway
    boolean		flowcontrol;  ///< use flow-control in all modules (default: true)
    char		*cachedir;
    char		*rootdir;
    char		*spooldir;

    boolean		nocache;
    char		*ccfilename;
    apr_file_t		*ccfile;	///< file descriptor for config cache for writes
    apr_hash_t		*config_cache;
    apr_thread_mutex_t	*config_cache_mutex;

    nx_module_list_t	*modules;	///< linked list of modules loaded
    nx_route_list_t	*routes;	///< linked list of routes
    nx_event_list_t	*events;	///< list of pending events
    nx_resource_list_t	*resources;	///< list of registered resources
    nx_jobgroups_t 	*jobgroups;	///< list of jobgroups
    nx_module_input_func_list_t *input_funcs; ///< list of registered input functions
    nx_module_output_func_list_t *output_funcs; ///< list of registered output functions
    nx_expr_func_list_t *expr_funcs; 	///< list of registered functions
    nx_expr_proc_list_t *expr_procs;	///< list of registered procedures
    nx_module_t		*coremodule;
} nx_ctx_t;

nx_ctx_t *nx_ctx_new();
nx_ctx_t *nx_ctx_get();
void nx_ctx_parse_cfg(nx_ctx_t *ctx, const char *cfgfile);
void nx_ctx_init_logging(nx_ctx_t *ctx);
void nx_ctx_free(nx_ctx_t *ctx);

void nx_ctx_init_jobs(nx_ctx_t *ctx);
boolean nx_ctx_next_job(nx_ctx_t *ctx,
			nx_job_t **jobresult,
			nx_event_t **eventresult);
boolean nx_ctx_has_jobs(nx_ctx_t *ctx);
nx_module_t *nx_ctx_module_for_job(nx_ctx_t *ctx, nx_job_t *job);
void nx_ctx_register_builtins(nx_ctx_t *ctx);
void nx_module_register_exports(const nx_ctx_t *ctx, nx_module_t *module);
void nx_module_load_dso(nx_module_t *module,
			const nx_ctx_t *ctx,
			const char *dsopath);

#endif	/* __NX_CTX_H */
