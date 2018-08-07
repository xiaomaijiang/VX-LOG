/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_LOGQUEUE_H
#define __NX_LOGQUEUE_H

#include <apr_thread_proc.h>
#include "dlist.h"
#include "logdata.h"

#define NX_LOGQUEUE_LIMIT 100 /* FIXME */

typedef struct nx_logqueue_list_t nx_logqueue_list_t;

NX_DLIST_HEAD(nx_logqueue_list_t, nx_logdata_t);

typedef struct nx_logqueue_t
{
    apr_pool_t 		*pool;
    apr_thread_mutex_t	*mutex;
    nx_logqueue_list_t *list;		///< List of logdata structures (the real queue)
    int			size;		///< number of elements in the queue
    int			limit;
    const char		*name;
    const char		*basedir;
    boolean		needpop;	///< TRUE after nx_logqueue_peek has been called
} nx_logqueue_t;


nx_logqueue_t *nx_logqueue_new(apr_pool_t *pool,
			       const char *name);
void nx_logqueue_init(nx_logqueue_t *logqueue);
int nx_logqueue_push(nx_logqueue_t *logqueue, nx_logdata_t *logdata);
int nx_logqueue_peek(nx_logqueue_t *logqueue, nx_logdata_t **logdata);
int nx_logqueue_pop(nx_logqueue_t *logqueue, nx_logdata_t *logdata);
void nx_logqueue_lock(nx_logqueue_t *logqueue);
void nx_logqueue_unlock(nx_logqueue_t *logqueue);
int nx_logqueue_size(nx_logqueue_t *logqueue);
apr_size_t nx_logqueue_to_file(nx_logqueue_t *logqueue);
apr_size_t nx_logqueue_from_file(nx_logqueue_t *logqueue);

#endif	/* __NX_LOGQUEUE_H */
