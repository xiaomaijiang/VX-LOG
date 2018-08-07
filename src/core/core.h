/*
 * This file is part of the nxlog log collector tool.
 * See the file LICENSE in the source root for licensing terms.
 * Website: http://nxlog.org
 * Author: Botond Botyanszki <botond.botyanszki@nxlog.org>
 */

#ifndef __NX_CORE_H
#define __NX_CORE_H

#include <apr_thread_proc.h>
#include <apr_thread_cond.h>

boolean nx_init(int *argc, char const *const **argv, char const *const **env);

void nx_thread_create(apr_thread_t **thread,
		      apr_threadattr_t *attr,
		      apr_thread_start_t func,
		      void *data,
		      apr_pool_t *pool);

#endif	/* __NX_CORE_H */
