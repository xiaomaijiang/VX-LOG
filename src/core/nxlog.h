/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NXLOG_H
#define __NXLOG_H

#include <apr_poll.h>
#include <apr_thread_cond.h>
#include <apr_thread_proc.h>

#include "../common/types.h"
#include "../common/error_debug.h"
#include "../common/config_cache.h"
#include "ctx.h"


#define NX_POLL_TIMEOUT (APR_USEC_PER_SEC / 20)
//#define NX_POLL_TIMEOUT (APR_USEC_PER_SEC * 10)


typedef struct nxlog_t
{
    apr_pool_t 		*pool;
    char		*cfgfile;
    int 		uid;		///< uid to drop to
    int 		gid;    	///< gid to drop to
    const char		*chroot; 	///< chroot to this directory
    boolean		foreground; 	///< TRUE if should not daemonize
    boolean		daemonized; 	///< TRUE if daemonized
    apr_uint32_t	reload_request;	///< non-zero if nxlog is to be reloaded
    boolean		terminate_request;
    boolean		terminating;
    apr_time_t		started;	///< Time of start TODO: store this in UTC

    int			pid;
    const char		*pidfile;
    apr_file_t		*pidf;
    boolean		verify_conf;	///< verify configuration file then exit
    boolean		do_stop;	///< stop a running instance
    boolean		do_restart;	///< restart a running instance
    unsigned int	num_worker_thread;
    apr_thread_t	**worker_threads;
    apr_thread_cond_t	*worker_cond;
    apr_uint32_t	*worker_threads_running; ///< non-zero if running (array)
    apr_thread_cond_t	*event_cond;
    apr_thread_t	*event_thread;

    apr_uint32_t	event_thread_running; ///< non-zero if running
    apr_thread_mutex_t	*mutex;
    nx_ctx_t		*ctx;		///< configuration context
    apr_thread_mutex_t	**openssl_locks;
    
    nx_string_t		hostname;
    nx_string_t		hostname_fqdn;
} nxlog_t;

nxlog_t *nxlog_get();
void nxlog_init(nxlog_t *nxlog);
void nxlog_init_poller(nxlog_t *nxlog);
void nxlog_reload(nxlog_t *nxlog);
void nxlog_shutdown(nxlog_t *nxlog);
void nxlog_create_threads(nxlog_t *nxlog);
void nxlog_wait_threads(nxlog_t *nxlog);
boolean nxlog_data_available();
void nxlog_mainloop(nxlog_t *nxlog, boolean offline);
void nxlog_set(nxlog_t *nxlog);
void nxlog_dump_info();
#endif	/* __NXLOG_H */
