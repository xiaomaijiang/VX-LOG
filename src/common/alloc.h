/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_ALLOC_H
#define __NX_ALLOC_H

#include "types.h"

#define NX_MAX_ALLOCATOR_SIZE (1024 * 512) /* 512Kb */

void nx_pool_mutex_set(apr_thread_mutex_t *mutex);
apr_pool_t *nx_pool_create_child(apr_pool_t *parent);
apr_pool_t *nx_pool_create_core();

#endif	/* __NX_ALLOC_H */
