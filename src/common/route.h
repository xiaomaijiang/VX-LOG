/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_ROUTE_H
#define __NX_ROUTE_H

#include <apr_thread_proc.h>

#include "types.h"
#include "logqueue.h"
#include "dlist.h"


typedef struct nx_route_t
{
    NX_DLIST_ENTRY(nx_route_t) link;
    apr_pool_t 		*pool;
    int 		refcount;
    const char		*name;
    int 		priority; ///< from 1 to 100, 100 is the lowest
    apr_thread_mutex_t	*mutex;
    apr_array_header_t	*modules;
} nx_route_t;


#endif	/* __NX_ROUTE_H */
